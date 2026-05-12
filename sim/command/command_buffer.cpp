#include "sim/command/command_buffer.h"
#include "sim/world/world_state.h"

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
    default:
        break;
    }
}
