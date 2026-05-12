#include "sim/command/command_buffer.h"
#include "sim/world/world_state.h"
#include <algorithm>

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
        for (auto& agent : world.agents.agents)
        {
            if (agent.id == cmd.entityId)
            {
                if (world.env.temperature.InBounds(cmd.targetX, cmd.targetY))
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
        if (world.env.fire.InBounds(cmd.x, cmd.y))
            world.env.fire.At(cmd.x, cmd.y) = cmd.value;
        break;
    }
    case CommandType::ExtinguishFire:
    {
        if (world.env.fire.InBounds(cmd.x, cmd.y))
            world.env.fire.At(cmd.x, cmd.y) = 0.0f;
        break;
    }
    case CommandType::EmitSmell:
    {
        if (world.info.smell.InBounds(cmd.x, cmd.y))
            world.info.smell.At(cmd.x, cmd.y) += cmd.value;
        break;
    }
    case CommandType::SetDanger:
    {
        if (world.info.danger.InBounds(cmd.x, cmd.y))
            world.info.danger.At(cmd.x, cmd.y) = cmd.value;
        break;
    }
    case CommandType::DamageAgent:
    {
        for (auto& agent : world.agents.agents)
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
        for (auto& agent : world.agents.agents)
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
        for (auto& agent : world.agents.agents)
        {
            if (agent.id == cmd.entityId)
            {
                agent.currentAction = static_cast<AgentAction>(cmd.x);
                break;
            }
        }
        break;
    }
    default:
        break;
    }
}
