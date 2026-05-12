#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class ClimateSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        Tick t = world.clock.currentTick;

        // Temperature: slow sine wave + noise
        for (i32 y = 0; y < world.height; y++)
        {
            for (i32 x = 0; x < world.width; x++)
            {
                f32 baseTemp = 20.0f + 10.0f * std::sin(static_cast<f32>(t) * 0.01f + x * 0.1f);
                f32 noise = world.random.Next01() * 2.0f - 1.0f;
                world.temperature.At(x, y) = baseTemp + noise;
            }
        }

        // Humidity: inverse correlation with temperature + noise
        for (i32 y = 0; y < world.height; y++)
        {
            for (i32 x = 0; x < world.width; x++)
            {
                f32 temp = world.temperature.At(x, y);
                f32 baseHumidity = 70.0f - (temp - 20.0f) * 1.5f;
                f32 noise = world.random.Next01() * 5.0f - 2.5f;
                world.humidity.At(x, y) = std::max(0.0f, std::min(100.0f, baseHumidity + noise));
            }
        }

        // Wind: changes direction every ~100 ticks
        if (t % 100 == 0)
        {
            f32 angle = world.random.Next01() * 6.2831853f;
            world.wind.x = std::cos(angle);
            world.wind.y = std::sin(angle);
        }
    }
};
