#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

// OWNERSHIP: Engine (sim/system/)
// READS: EnvironmentModule (fire, temperature), InformationModule (smell, danger), AgentModule (agents)
// WRITES: AgentModule (perception fields: nearestFire, localTemperature, nearestSmell, localDanger)
// PHASE: SimPhase::Perception

class AgentPerceptionSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        auto& env = world.Env();
        auto& info = world.Info();
        auto& agentMod = world.Agents();

        for (auto& agent : agentMod.agents)
        {
            i32 x = agent.position.x;
            i32 y = agent.position.y;

            if (!info.smell.InBounds(x, y)) continue;

            agent.localTemperature = env.temperature.At(x, y);
            agent.nearestSmell = 0.0f;
            agent.nearestFire = 0.0f;

            i32 scanRadius = 3;
            for (i32 sy = -scanRadius; sy <= scanRadius; sy++)
            {
                for (i32 sx = -scanRadius; sx <= scanRadius; sx++)
                {
                    i32 nx = x + sx;
                    i32 ny = y + sy;

                    if (!info.smell.InBounds(nx, ny)) continue;

                    f32 dist = std::sqrt(static_cast<f32>(sx * sx + sy * sy));
                    if (dist > scanRadius) continue;

                    f32 smellVal = info.smell.At(nx, ny) / (1.0f + dist);
                    if (smellVal > agent.nearestSmell) agent.nearestSmell = smellVal;

                    f32 fireVal = env.fire.At(nx, ny) / (1.0f + dist);
                    if (fireVal > agent.nearestFire) agent.nearestFire = fireVal;
                }
            }
        }
    }
};
