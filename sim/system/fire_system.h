#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class FireSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        Grid<f32> newFire = world.fire;

        for (i32 y = 0; y < world.height; y++)
        {
            for (i32 x = 0; x < world.width; x++)
            {
                f32 current = world.fire.At(x, y);

                if (current > 0.0f)
                {
                    // Fire burns out slowly
                    newFire.At(x, y) = std::max(0.0f, current - burnRate);

                    // Spread to neighbors
                    SpreadFire(world, newFire, x, y, current);

                    // Fire heats the local temperature
                    world.temperature.At(x, y) += current * 0.05f;

                    // Fire reduces humidity
                    world.humidity.At(x, y) = std::max(0.0f, world.humidity.At(x, y) - current * 0.02f);
                }
                else
                {
                    // Spontaneous ignition: hot + dry + random chance
                    f32 temp = world.temperature.At(x, y);
                    f32 hum = world.humidity.At(x, y);

                    if (temp > 50.0f && hum < 30.0f && world.random.Next01() < 0.005f)
                    {
                        newFire.At(x, y) = 40.0f;
                    }
                }
            }
        }

        world.fire = newFire;
    }

private:
    static constexpr f32 burnRate = 0.5f;
    static constexpr f32 spreadChance = 0.05f;
    static constexpr f32 spreadThreshold = 15.0f;

    void SpreadFire(WorldState& world, Grid<f32>& newFire, i32 x, i32 y, f32 intensity)
    {
        if (intensity < spreadThreshold) return;

        static const i32 dx[] = {-1, 1, 0, 0};
        static const i32 dy[] = {0, 0, -1, 1};

        for (i32 i = 0; i < 4; i++)
        {
            i32 nx = x + dx[i];
            i32 ny = y + dy[i];

            if (!world.fire.InBounds(nx, ny)) continue;

            // Fire spreads more easily to dry areas
            f32 humidity = world.humidity.At(nx, ny);
            f32 adjustedChance = spreadChance * (1.0f - humidity / 100.0f);

            if (world.fire.At(nx, ny) <= 0.0f && world.random.Next01() < adjustedChance)
            {
                newFire.At(nx, ny) = intensity * 0.6f;
            }
        }
    }
};
