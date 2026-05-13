#pragma once

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include <cmath>

// AgentActionSystem: executes agent behavior based on perception and action state.
// Moved from sim/system/ — depends on HumanEvolution field bindings.

class AgentActionSystem : public ISystem
{
public:
    explicit AgentActionSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : envCtx_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& fm = ctx.GetFieldModule();
        auto& sim = ctx.Sim();
        auto& commands = ctx.Commands();
        auto tick = ctx.CurrentTick();

        for (auto& agent : ctx.Agents().agents)
        {
            // Hunger increases every tick
            commands.Submit(tick, ModifyHungerCommand{agent.id, 0.5f});

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
                    if (!fm.InBounds(envCtx_.fire, nx, ny)) continue;
                    f32 fireVal = fm.Read(envCtx_.fire, nx, ny);
                    if (fireVal < bestFire)
                    {
                        bestFire = fireVal;
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
                    if (!fm.InBounds(envCtx_.smell, nx, ny)) continue;
                    f32 smellVal = fm.Read(envCtx_.smell, nx, ny);
                    if (smellVal > bestSmell)
                    {
                        bestSmell = smellVal;
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
            if (fm.InBounds(envCtx_.temperature, newX, newY))
            {
                commands.Submit(tick, MoveAgentCommand{agent.id, newX, newY});
            }

            // Eating: submit FeedAgent command
            if (fm.InBounds(envCtx_.smell, agent.position.x, agent.position.y))
            {
                if (fm.Read(envCtx_.smell, agent.position.x, agent.position.y) > 20.0f)
                {
                    commands.Submit(tick, FeedAgentCommand{agent.id, 5.0f});
                }
            }

            // Heat damage: submit DamageAgent command
            if (agent.localTemperature > 50.0f)
            {
                f32 damage = (agent.localTemperature - 50.0f) * 0.1f;
                commands.Submit(tick, DamageAgentCommand{agent.id, damage});
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Command, AccessMode::Write}
        };
        static const char* const DEPS[] = {"AgentDecisionSystem"};
        return {"AgentActionSystem", SimPhase::Action, READS, 2, WRITES, 1, DEPS, 1, true, false};
    }

private:
    HumanEvolution::EnvironmentContext envCtx_;
};
