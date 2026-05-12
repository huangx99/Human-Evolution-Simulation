#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class AgentActionSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        auto& env = world.Env();
        auto& info = world.Info();
        auto& sim = world.Sim();

        for (auto& agent : world.Agents().agents)
        {
            // Hunger increases every tick
            world.commands.Submit(Command{
                CommandType::ModifyHunger, sim.clock.currentTick,
                agent.id, 0, 0, 0, 0, 0.5f
            });

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
                    if (!env.fire.InBounds(nx, ny)) continue;
                    if (env.fire.At(nx, ny) < bestFire)
                    {
                        bestFire = env.fire.At(nx, ny);
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
                    if (!info.smell.InBounds(nx, ny)) continue;
                    if (info.smell.At(nx, ny) > bestSmell)
                    {
                        bestSmell = info.smell.At(nx, ny);
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
                i32 dir = sim.random.NextRange(0, 4);
                static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1},{0,0}};
                dx = offsets[dir][0];
                dy = offsets[dir][1];
                break;
            }
            default:
                break;
            }

            // Submit move command
            i32 newX = agent.position.x + dx;
            i32 newY = agent.position.y + dy;
            if (env.temperature.InBounds(newX, newY))
            {
                world.commands.Submit(Command{
                    CommandType::MoveAgent, sim.clock.currentTick,
                    agent.id, 0, 0, newX, newY, 0.0f
                });
            }

            // Eating: submit FeedAgent command
            if (info.smell.InBounds(agent.position.x, agent.position.y))
            {
                if (info.smell.At(agent.position.x, agent.position.y) > 20.0f)
                {
                    world.commands.Submit(Command{
                        CommandType::FeedAgent, sim.clock.currentTick,
                        agent.id, 0, 0, 0, 0, 5.0f
                    });
                }
            }

            // Heat damage: submit DamageAgent command
            if (agent.localTemperature > 50.0f)
            {
                f32 damage = (agent.localTemperature - 50.0f) * 0.1f;
                world.commands.Submit(Command{
                    CommandType::DamageAgent, sim.clock.currentTick,
                    agent.id, 0, 0, 0, 0, damage
                });
            }
        }
    }
};
