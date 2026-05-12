#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class AgentPerceptionSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        for (auto& agent : world.agents.agents)
        {
            i32 x = agent.position.x;
            i32 y = agent.position.y;

            if (!world.info.smell.InBounds(x, y)) continue;

            agent.localTemperature = world.env.temperature.At(x, y);
            agent.nearestSmell = 0.0f;
            agent.nearestFire = 0.0f;

            i32 scanRadius = 3;
            for (i32 sy = -scanRadius; sy <= scanRadius; sy++)
            {
                for (i32 sx = -scanRadius; sx <= scanRadius; sx++)
                {
                    i32 nx = x + sx;
                    i32 ny = y + sy;

                    if (!world.info.smell.InBounds(nx, ny)) continue;

                    f32 dist = std::sqrt(static_cast<f32>(sx * sx + sy * sy));
                    if (dist > scanRadius) continue;

                    f32 smellVal = world.info.smell.At(nx, ny) / (1.0f + dist);
                    if (smellVal > agent.nearestSmell) agent.nearestSmell = smellVal;

                    f32 fireVal = world.env.fire.At(nx, ny) / (1.0f + dist);
                    if (fireVal > agent.nearestFire) agent.nearestFire = fireVal;
                }
            }
        }
    }
};
