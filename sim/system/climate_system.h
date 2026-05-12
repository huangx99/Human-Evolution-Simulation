#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class ClimateSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        auto& env = world.Env();
        auto& sim = world.Sim();
        Tick t = sim.clock.currentTick;

        for (i32 y = 0; y < env.height; y++)
        {
            for (i32 x = 0; x < env.width; x++)
            {
                f32 baseTemp = 20.0f + 10.0f * std::sin(static_cast<f32>(t) * 0.01f + x * 0.1f);
                f32 noise = sim.random.Next01() * 2.0f - 1.0f;
                env.temperature.WriteNext(x, y) = baseTemp + noise;
            }
        }

        for (i32 y = 0; y < env.height; y++)
        {
            for (i32 x = 0; x < env.width; x++)
            {
                f32 temp = env.temperature.At(x, y);
                f32 baseHumidity = 70.0f - (temp - 20.0f) * 1.5f;
                f32 noise = sim.random.Next01() * 5.0f - 2.5f;
                env.humidity.WriteNext(x, y) = std::max(0.0f, std::min(100.0f, baseHumidity + noise));
            }
        }

        if (t % 100 == 0)
        {
            f32 angle = sim.random.Next01() * 6.2831853f;
            env.wind.x = std::cos(angle);
            env.wind.y = std::sin(angle);
        }
    }
};
