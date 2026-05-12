#include "sim/command/command_buffer.h"
#include "sim/world/world_state.h"
#include "sim/ecology/field_id.h"

void CommandBuffer::Apply(WorldState& world)
{
    for (const auto& cmd : pending)
    {
        Execute(world, cmd);
    }
    history.insert(history.end(), pending.begin(), pending.end());
    pending.clear();
}

void CommandBuffer::Execute(WorldState& world, const Command& cmd)
{
    switch (cmd.type)
    {
    case CommandType::MoveAgent:
    {
        for (auto& agent : world.Agents().agents)
        {
            if (agent.id == cmd.entityId)
            {
                if (world.Env().temperature.InBounds(cmd.targetX, cmd.targetY))
                {
                    agent.position.x = cmd.targetX;
                    agent.position.y = cmd.targetY;
                }
                break;
            }
        }
        break;
    }
    case CommandType::IgniteFire:
    {
        if (world.Env().fire.InBounds(cmd.x, cmd.y))
            world.Env().fire.WriteNext(cmd.x, cmd.y) = cmd.value;
        break;
    }
    case CommandType::ExtinguishFire:
    {
        if (world.Env().fire.InBounds(cmd.x, cmd.y))
            world.Env().fire.WriteNext(cmd.x, cmd.y) = 0.0f;
        break;
    }
    case CommandType::EmitSmell:
    {
        if (world.Info().smell.InBounds(cmd.x, cmd.y))
            world.Info().smell.WriteNext(cmd.x, cmd.y) += cmd.value;
        break;
    }
    case CommandType::SetDanger:
    {
        if (world.Info().danger.InBounds(cmd.x, cmd.y))
            world.Info().danger.WriteNext(cmd.x, cmd.y) = cmd.value;
        break;
    }
    case CommandType::DamageAgent:
    {
        for (auto& agent : world.Agents().agents)
        {
            if (agent.id == cmd.entityId)
            {
                agent.health = std::max(0.0f, agent.health - cmd.value);
                break;
            }
        }
        break;
    }
    case CommandType::FeedAgent:
    {
        for (auto& agent : world.Agents().agents)
        {
            if (agent.id == cmd.entityId)
            {
                agent.hunger = std::max(0.0f, agent.hunger - cmd.value);
                break;
            }
        }
        break;
    }
    case CommandType::SetAgentAction:
    {
        for (auto& agent : world.Agents().agents)
        {
            if (agent.id == cmd.entityId)
            {
                agent.currentAction = static_cast<AgentAction>(cmd.x);
                break;
            }
        }
        break;
    }
    case CommandType::ModifyHunger:
    {
        for (auto& agent : world.Agents().agents)
        {
            if (agent.id == cmd.entityId)
            {
                agent.hunger = std::max(0.0f, std::min(100.0f, agent.hunger + cmd.value));
                break;
            }
        }
        break;
    }
    case CommandType::AddEntityState:
    {
        auto* entity = world.Ecology().entities.Find(cmd.entityId);
        if (entity)
            entity->AddState(static_cast<MaterialState>(static_cast<u32>(cmd.value)));
        break;
    }
    case CommandType::RemoveEntityState:
    {
        auto* entity = world.Ecology().entities.Find(cmd.entityId);
        if (entity)
            entity->state = static_cast<MaterialState>(
                static_cast<u32>(entity->state) & ~static_cast<u32>(cmd.value));
        break;
    }
    case CommandType::AddEntityCapability:
    {
        auto* entity = world.Ecology().entities.Find(cmd.entityId);
        if (entity)
            entity->AddCapability(static_cast<Capability>(static_cast<u32>(cmd.value)));
        break;
    }
    case CommandType::RemoveEntityCapability:
    {
        auto* entity = world.Ecology().entities.Find(cmd.entityId);
        if (entity)
            entity->extraCapabilities = static_cast<Capability>(
                static_cast<u32>(entity->extraCapabilities) & ~static_cast<u32>(cmd.value));
        break;
    }
    case CommandType::ModifyFieldValue:
    {
        FieldId field = static_cast<FieldId>(cmd.targetX);
        f32 newVal = cmd.value;
        if (cmd.targetY == 1) // absolute set
        {
            // setTo is already in cmd.value
        }
        else
        {
            // delta mode: read current + add
            f32 current = 0.0f;
            auto& env = world.Env();
            auto& info = world.Info();
            switch (field)
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
        auto& env = world.Env();
        auto& info = world.Info();
        switch (field)
        {
        case FieldId::Temperature: if (env.temperature.InBounds(cmd.x, cmd.y)) env.temperature.WriteNext(cmd.x, cmd.y) = newVal; break;
        case FieldId::Humidity:    if (env.humidity.InBounds(cmd.x, cmd.y)) env.humidity.WriteNext(cmd.x, cmd.y) = newVal; break;
        case FieldId::Fire:        if (env.fire.InBounds(cmd.x, cmd.y)) env.fire.WriteNext(cmd.x, cmd.y) = newVal; break;
        case FieldId::Smell:       if (info.smell.InBounds(cmd.x, cmd.y)) info.smell.WriteNext(cmd.x, cmd.y) = newVal; break;
        case FieldId::Danger:      if (info.danger.InBounds(cmd.x, cmd.y)) info.danger.WriteNext(cmd.x, cmd.y) = newVal; break;
        case FieldId::Smoke:       if (info.smoke.InBounds(cmd.x, cmd.y)) info.smoke.WriteNext(cmd.x, cmd.y) = newVal; break;
        default: break;
        }
        break;
    }
    case CommandType::EmitSmoke:
    {
        if (world.Info().smoke.InBounds(cmd.x, cmd.y))
            world.Info().smoke.WriteNext(cmd.x, cmd.y) += cmd.value;
        break;
    }
    default:
        break;
    }
}
