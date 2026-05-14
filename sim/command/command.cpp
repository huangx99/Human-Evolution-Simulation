#include "sim/command/command.h"
#include "sim/world/world_state.h"
#include "sim/runtime/simulation_hash.h"
#include <algorithm>

// === Engine command Execute implementations ===

void MoveAgentCommand::Execute(WorldState& world) const
{
    for (auto& agent : world.Agents().agents)
    {
        if (agent.id == id)
        {
            if (!agent.alive) break;

            // Bounds check using first registered field (any spatial field works)
            auto& fm = world.Fields();
            if (fm.FieldCount() > 0)
            {
                FieldIndex firstField(1);
                if (fm.InBounds(firstField, x, y))
                {
                    agent.position.x = x;
                    agent.position.y = y;
                }
            }
            break;
        }
    }
}

void SetAgentActionCommand::Execute(WorldState& world) const
{
    for (auto& agent : world.Agents().agents)
    {
        if (agent.id == id)
        {
            if (!agent.alive) break;

            agent.currentAction = action;
            break;
        }
    }
}

void DamageAgentCommand::Execute(WorldState& world) const
{
    for (auto& agent : world.Agents().agents)
    {
        if (agent.id == id)
        {
            if (!agent.alive) break;

            f32 prevHealth = agent.health;
            agent.health = std::max(0.0f, agent.health - amount);
            // Emit death event on health->0 transition
            // TRANSITIONAL DEBT: EventType::AgentDied should become RulePack-registered
            // when EventRegistry lands.
            if (prevHealth > 0.0f && agent.health <= 0.0f && agent.alive)
            {
                agent.alive = false;
                world.events.Emit({EventType::AgentDied,
                    world.Sim().clock.currentTick, agent.id,
                    agent.position.x, agent.position.y, amount});
            }
            break;
        }
    }
}

void FeedAgentCommand::Execute(WorldState& world) const
{
    for (auto& agent : world.Agents().agents)
    {
        if (agent.id == id)
        {
            if (!agent.alive) break;

            agent.hunger = std::max(0.0f, agent.hunger - amount);
            break;
        }
    }
}

void ModifyHungerCommand::Execute(WorldState& world) const
{
    for (auto& agent : world.Agents().agents)
    {
        if (agent.id == id)
        {
            if (!agent.alive) break;

            agent.hunger = std::max(0.0f, std::min(100.0f, agent.hunger + delta));
            break;
        }
    }
}

void AddEntityStateCommand::Execute(WorldState& world) const
{
    auto* entity = world.Ecology().entities.Find(id);
    if (entity)
    {
        entity->AddState(static_cast<MaterialState>(state));
        entity->stateChangedTick = world.Sim().clock.currentTick;
    }
}

void RemoveEntityStateCommand::Execute(WorldState& world) const
{
    auto* entity = world.Ecology().entities.Find(id);
    if (entity)
        entity->state = static_cast<MaterialState>(
            static_cast<u32>(entity->state) & ~state);
}

void AddEntityCapabilityCommand::Execute(WorldState& world) const
{
    auto* entity = world.Ecology().entities.Find(id);
    if (entity)
        entity->AddCapability(static_cast<Capability>(capability));
}

void RemoveEntityCapabilityCommand::Execute(WorldState& world) const
{
    auto* entity = world.Ecology().entities.Find(id);
    if (entity)
        entity->extraCapabilities = static_cast<Capability>(
            static_cast<u32>(entity->extraCapabilities) & ~capability);
}

void ModifyFieldValueCommand::Execute(WorldState& world) const
{
    auto& fm = world.Fields();
    auto idx = fm.FindByKey(field);
    if (!idx) return;
    if (!fm.InBounds(idx, x, y)) return;

    f32 newVal = value;
    if (mode == 0) // Add mode: current + value
        newVal = fm.Read(idx, x, y) + value;

    fm.WriteNext(idx, x, y, newVal);
}

// === SerializeData implementations ===

void MoveAgentCommand::SerializeData(std::vector<u8>& buf) const
{
    AppendBytes(buf, id);
    AppendBytes(buf, x);
    AppendBytes(buf, y);
}

void SetAgentActionCommand::SerializeData(std::vector<u8>& buf) const
{
    AppendBytes(buf, id);
    auto a = static_cast<u8>(action);
    AppendBytes(buf, a);
}

void DamageAgentCommand::SerializeData(std::vector<u8>& buf) const
{
    AppendBytes(buf, id);
    AppendBytes(buf, amount);
}

void FeedAgentCommand::SerializeData(std::vector<u8>& buf) const
{
    AppendBytes(buf, id);
    AppendBytes(buf, amount);
}

void ModifyHungerCommand::SerializeData(std::vector<u8>& buf) const
{
    AppendBytes(buf, id);
    AppendBytes(buf, delta);
}

void AddEntityStateCommand::SerializeData(std::vector<u8>& buf) const
{
    AppendBytes(buf, id);
    AppendBytes(buf, state);
}

void RemoveEntityStateCommand::SerializeData(std::vector<u8>& buf) const
{
    AppendBytes(buf, id);
    AppendBytes(buf, state);
}

void AddEntityCapabilityCommand::SerializeData(std::vector<u8>& buf) const
{
    AppendBytes(buf, id);
    AppendBytes(buf, capability);
}

void RemoveEntityCapabilityCommand::SerializeData(std::vector<u8>& buf) const
{
    AppendBytes(buf, id);
    AppendBytes(buf, capability);
}

void ModifyFieldValueCommand::SerializeData(std::vector<u8>& buf) const
{
    AppendBytes(buf, x);
    AppendBytes(buf, y);
    AppendBytes(buf, field.value);
    AppendBytes(buf, mode);
    AppendBytes(buf, value);
}

// === FeedHash implementations ===

void MoveAgentCommand::FeedHash(SimHash& h) const
{
    h.FeedU64(id);
    h.FeedI32(x);
    h.FeedI32(y);
}

void SetAgentActionCommand::FeedHash(SimHash& h) const
{
    h.FeedU64(id);
    h.FeedI32(static_cast<i32>(action));
}

void DamageAgentCommand::FeedHash(SimHash& h) const
{
    h.FeedU64(id);
    h.FeedF32(amount);
}

void FeedAgentCommand::FeedHash(SimHash& h) const
{
    h.FeedU64(id);
    h.FeedF32(amount);
}

void ModifyHungerCommand::FeedHash(SimHash& h) const
{
    h.FeedU64(id);
    h.FeedF32(delta);
}

void AddEntityStateCommand::FeedHash(SimHash& h) const
{
    h.FeedU64(id);
    h.FeedU32(state);
}

void RemoveEntityStateCommand::FeedHash(SimHash& h) const
{
    h.FeedU64(id);
    h.FeedU32(state);
}

void AddEntityCapabilityCommand::FeedHash(SimHash& h) const
{
    h.FeedU64(id);
    h.FeedU32(capability);
}

void RemoveEntityCapabilityCommand::FeedHash(SimHash& h) const
{
    h.FeedU64(id);
    h.FeedU32(capability);
}

void ModifyFieldValueCommand::FeedHash(SimHash& h) const
{
    h.FeedI32(x);
    h.FeedI32(y);
    h.FeedU64(field.value);
    h.FeedI32(mode);
    h.FeedF32(value);
}

// === CommandRegistry: register core engine factories ===

static std::unique_ptr<CommandBase> DeserializeMoveAgent(const u8* data, size_t size)
{
    auto id = ReadBytes<EntityId>(data, size);
    auto x = ReadBytes<i32>(data, size);
    auto y = ReadBytes<i32>(data, size);
    auto cmd = std::make_unique<MoveAgentCommand>();
    cmd->id = id; cmd->x = x; cmd->y = y;
    return cmd;
}

static std::unique_ptr<CommandBase> DeserializeDamageAgent(const u8* data, size_t size)
{
    auto id = ReadBytes<EntityId>(data, size);
    auto amount = ReadBytes<f32>(data, size);
    auto cmd = std::make_unique<DamageAgentCommand>();
    cmd->id = id; cmd->amount = amount;
    return cmd;
}

static std::unique_ptr<CommandBase> DeserializeFeedAgent(const u8* data, size_t size)
{
    auto id = ReadBytes<EntityId>(data, size);
    auto amount = ReadBytes<f32>(data, size);
    auto cmd = std::make_unique<FeedAgentCommand>();
    cmd->id = id; cmd->amount = amount;
    return cmd;
}

static std::unique_ptr<CommandBase> DeserializeModifyHunger(const u8* data, size_t size)
{
    auto id = ReadBytes<EntityId>(data, size);
    auto delta = ReadBytes<f32>(data, size);
    auto cmd = std::make_unique<ModifyHungerCommand>();
    cmd->id = id; cmd->delta = delta;
    return cmd;
}

static std::unique_ptr<CommandBase> DeserializeModifyFieldValue(const u8* data, size_t size)
{
    auto x = ReadBytes<i32>(data, size);
    auto y = ReadBytes<i32>(data, size);
    auto fv = ReadBytes<u64>(data, size);
    auto mode = ReadBytes<i32>(data, size);
    auto value = ReadBytes<f32>(data, size);
    auto cmd = std::make_unique<ModifyFieldValueCommand>();
    cmd->x = x; cmd->y = y; cmd->field = FieldKey::FromRaw(fv); cmd->mode = mode; cmd->value = value;
    return cmd;
}

CommandRegistry::CommandRegistry()
{
    Register(CommandKind::MoveAgent, DeserializeMoveAgent);
    Register(CommandKind::DamageAgent, DeserializeDamageAgent);
    Register(CommandKind::FeedAgent, DeserializeFeedAgent);
    Register(CommandKind::ModifyHunger, DeserializeModifyHunger);
    Register(CommandKind::ModifyFieldValue, DeserializeModifyFieldValue);
}
