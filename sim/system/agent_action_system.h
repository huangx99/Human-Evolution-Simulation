#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class AgentActionSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        for (auto& agent : world.agents.agents)
        {
            agent.hunger = std::min(100.0f, agent.hunger + 0.5f);

            i32 dx = 0, dy = 0;

            switch (agent.currentAction)
            {
            case AgentAction::Flee:
            {
                i32 bestDx = 0, bestDy = 0;
                f32 bestFire = agent.nearestFire;

                static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                for (auto& off : offsets)
                {
                    i32 nx = agent.position.x + off[0];
                    i32 ny = agent.position.y + off[1];
                    if (!world.env.fire.InBounds(nx, ny)) continue;
                    if (world.env.fire.At(nx, ny) < bestFire)
                    {
                        bestFire = world.env.fire.At(nx, ny);
                        bestDx = off[0];
                        bestDy = off[1];
                    }
                }
                dx = bestDx;
                dy = bestDy;
                break;
            }
            case AgentAction::MoveToFood:
            {
                i32 bestDx = 0, bestDy = 0;
                f32 bestSmell = 0.0f;

                static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                for (auto& off : offsets)
                {
                    i32 nx = agent.position.x + off[0];
                    i32 ny = agent.position.y + off[1];
                    if (!world.info.smell.InBounds(nx, ny)) continue;
                    if (world.info.smell.At(nx, ny) > bestSmell)
                    {
                        bestSmell = world.info.smell.At(nx, ny);
                        bestDx = off[0];
                        bestDy = off[1];
                    }
                }
                dx = bestDx;
                dy = bestDy;
                break;
            }
            case AgentAction::Wander:
            {
                i32 dir = world.sim.random.NextRange(0, 4);
                static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1},{0,0}};
                dx = offsets[dir][0];
                dy = offsets[dir][1];
                break;
            }
            case AgentAction::Idle:
            default:
                break;
            }

            i32 newX = agent.position.x + dx;
            i32 newY = agent.position.y + dy;

            if (world.env.temperature.InBounds(newX, newY))
            {
                agent.position.x = newX;
                agent.position.y = newY;
            }

            if (world.info.smell.InBounds(agent.position.x, agent.position.y))
            {
                if (world.info.smell.At(agent.position.x, agent.position.y) > 20.0f)
                {
                    agent.hunger = std::max(0.0f, agent.hunger - 5.0f);
                }
            }

            if (agent.localTemperature > 50.0f)
            {
                agent.health -= (agent.localTemperature - 50.0f) * 0.1f;
            }
            agent.health = std::max(0.0f, std::min(100.0f, agent.health));
        }
    }
};
