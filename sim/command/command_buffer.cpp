#include "sim/command/command_buffer.h"
#include "sim/world/world_state.h"
#include "sim/ecology/field_id.h"

// === ExecuteOne overloads — one per concrete command type ===

static void ExecuteOne(WorldState& world, const MoveAgentCommand& cmd)
{
    for (auto& agent : world.Agents().agents)
    {
        if (agent.id == cmd.id)
        {
            if (world.Env().temperature.InBounds(cmd.x, cmd.y))
            {
                agent.position.x = cmd.x;
                agent.position.y = cmd.y;
            }
            break;
        }
    }
}

static void ExecuteOne(WorldState& world, const SetAgentActionCommand& cmd)
{
    for (auto& agent : world.Agents().agents)
    {
        if (agent.id == cmd.id)
        {
            agent.currentAction = cmd.action;
            break;
        }
    }
}

static void ExecuteOne(WorldState& world, const DamageAgentCommand& cmd)
{
    for (auto& agent : world.Agents().agents)
    {
        if (agent.id == cmd.id)
        {
            agent.health = std::max(0.0f, agent.health - cmd.amount);
            break;
        }
    }
}

static void ExecuteOne(WorldState& world, const FeedAgentCommand& cmd)
{
    for (auto& agent : world.Agents().agents)
    {
        if (agent.id == cmd.id)
        {
            agent.hunger = std::max(0.0f, agent.hunger - cmd.amount);
            break;
        }
    }
}

static void ExecuteOne(WorldState& world, const ModifyHungerCommand& cmd)
{
    for (auto& agent : world.Agents().agents)
    {
        if (agent.id == cmd.id)
        {
            agent.hunger = std::max(0.0f, std::min(100.0f, agent.hunger + cmd.delta));
            break;
        }
    }
}

static void ExecuteOne(WorldState& world, const IgniteFireCommand& cmd)
{
    if (world.Env().fire.InBounds(cmd.x, cmd.y))
        world.Env().fire.WriteNext(cmd.x, cmd.y) = cmd.intensity;
}

static void ExecuteOne(WorldState& world, const ExtinguishFireCommand& cmd)
{
    if (world.Env().fire.InBounds(cmd.x, cmd.y))
        world.Env().fire.WriteNext(cmd.x, cmd.y) = 0.0f;
}

static void ExecuteOne(WorldState& world, const EmitSmellCommand& cmd)
{
    if (world.Info().smell.InBounds(cmd.x, cmd.y))
        world.Info().smell.WriteNext(cmd.x, cmd.y) += cmd.amount;
}

static void ExecuteOne(WorldState& world, const SetDangerCommand& cmd)
{
    if (world.Info().danger.InBounds(cmd.x, cmd.y))
        world.Info().danger.WriteNext(cmd.x, cmd.y) = cmd.value;
}

static void ExecuteOne(WorldState& world, const AddEntityStateCommand& cmd)
{
    auto* entity = world.Ecology().entities.Find(cmd.id);
    if (entity)
        entity->AddState(static_cast<MaterialState>(cmd.state));
}

static void ExecuteOne(WorldState& world, const RemoveEntityStateCommand& cmd)
{
    auto* entity = world.Ecology().entities.Find(cmd.id);
    if (entity)
        entity->state = static_cast<MaterialState>(
            static_cast<u32>(entity->state) & ~cmd.state);
}

static void ExecuteOne(WorldState& world, const AddEntityCapabilityCommand& cmd)
{
    auto* entity = world.Ecology().entities.Find(cmd.id);
    if (entity)
        entity->AddCapability(static_cast<Capability>(cmd.capability));
}

static void ExecuteOne(WorldState& world, const RemoveEntityCapabilityCommand& cmd)
{
    auto* entity = world.Ecology().entities.Find(cmd.id);
    if (entity)
        entity->extraCapabilities = static_cast<Capability>(
            static_cast<u32>(entity->extraCapabilities) & ~cmd.capability);
}

static void ExecuteOne(WorldState& world, const ModifyFieldValueCommand& cmd)
{
    auto& env = world.Env();
    auto& info = world.Info();

    // Bounds check — reject commands with invalid coordinates
    bool inBounds = false;
    switch (cmd.field)
    {
    case FieldId::Temperature: inBounds = env.temperature.InBounds(cmd.x, cmd.y); break;
    case FieldId::Humidity:    inBounds = env.humidity.InBounds(cmd.x, cmd.y); break;
    case FieldId::Fire:        inBounds = env.fire.InBounds(cmd.x, cmd.y); break;
    case FieldId::Smell:       inBounds = info.smell.InBounds(cmd.x, cmd.y); break;
    case FieldId::Danger:      inBounds = info.danger.InBounds(cmd.x, cmd.y); break;
    case FieldId::Smoke:       inBounds = info.smoke.InBounds(cmd.x, cmd.y); break;
    default: break;
    }
    if (!inBounds) return;

    f32 newVal = cmd.value;
    if (cmd.mode == 0) // Add mode: current + value
    {
        f32 current = 0.0f;
        switch (cmd.field)
        {
        case FieldId::Temperature: current = env.temperature.At(cmd.x, cmd.y); break;
        case FieldId::Humidity:    current = env.humidity.At(cmd.x, cmd.y); break;
        case FieldId::Fire:        current = env.fire.At(cmd.x, cmd.y); break;
        case FieldId::Smell:       current = info.smell.At(cmd.x, cmd.y); break;
        case FieldId::Danger:      current = info.danger.At(cmd.x, cmd.y); break;
        case FieldId::Smoke:       current = info.smoke.At(cmd.x, cmd.y); break;
        default: break;
        }
        newVal = current + cmd.value;
    }

    switch (cmd.field)
    {
    case FieldId::Temperature: env.temperature.WriteNext(cmd.x, cmd.y) = newVal; break;
    case FieldId::Humidity:    env.humidity.WriteNext(cmd.x, cmd.y) = newVal; break;
    case FieldId::Fire:        env.fire.WriteNext(cmd.x, cmd.y) = newVal; break;
    case FieldId::Smell:       info.smell.WriteNext(cmd.x, cmd.y) = newVal; break;
    case FieldId::Danger:      info.danger.WriteNext(cmd.x, cmd.y) = newVal; break;
    case FieldId::Smoke:       info.smoke.WriteNext(cmd.x, cmd.y) = newVal; break;
    default: break;
    }
}

static void ExecuteOne(WorldState& world, const EmitSmokeCommand& cmd)
{
    if (world.Info().smoke.InBounds(cmd.x, cmd.y))
        world.Info().smoke.WriteNext(cmd.x, cmd.y) += cmd.amount;
}

// === Spatial dirty check ===

static bool IsSpatialDirtyCommand(const Command& cmd)
{
    return std::visit([](const auto& inner) -> bool {
        return std::visit([](const auto& typed) -> bool {
            using T = std::decay_t<decltype(typed)>;
            return std::is_same_v<T, AddEntityStateCommand> ||
                   std::is_same_v<T, RemoveEntityStateCommand> ||
                   std::is_same_v<T, AddEntityCapabilityCommand> ||
                   std::is_same_v<T, RemoveEntityCapabilityCommand>;
        }, inner);
    }, cmd);
}

// === CommandBuffer::Apply ===

void CommandBuffer::Apply(WorldState& world)
{
    for (const auto& qc : pending)
    {
        std::visit([&](const auto& inner) {
            std::visit([&](const auto& typed) {
                ExecuteOne(world, typed);
            }, inner);
        }, qc.command);

        if (IsSpatialDirtyCommand(qc.command))
            spatialDirty = true;
    }

    history.insert(history.end(), pending.begin(), pending.end());
    pending.clear();

    if (spatialDirty)
    {
        world.RebuildSpatial();
        spatialDirty = false;
    }
}
