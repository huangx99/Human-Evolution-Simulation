#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class FireSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        Grid<f32> newFire = world.env.fire;

        for (i32 y = 0; y < world.env.height; y++)
        {
            for (i32 x = 0; x < world.env.width; x++)
            {
                f32 current = world.env.fire.At(x, y);

                if (current > 0.0f)
                {
                    newFire.At(x, y) = std::max(0.0f, current - burnRate);
                    SpreadFire(world, newFire, x, y, current);
                    world.env.temperature.At(x, y) += current * 0.05f;
                    world.env.humidity.At(x, y) = std::max(0.0f, world.env.humidity.At(x, y) - current * 0.02f);

                    // Fire emits danger
                    world.info.danger.At(x, y) = std::max(world.info.danger.At(x, y), current);
                }
                else
                {
                    f32 temp = world.env.temperature.At(x, y);
                    f32 hum = world.env.humidity.At(x, y);
                    if (temp > 50.0f && hum < 30.0f && world.sim.random.Next01() < 0.005f)
                    {
                        newFire.At(x, y) = 40.0f;
                        world.events.Publish({EventType::FireStarted, world.sim.clock.currentTick, 0, x, y, 40.0f});
                    }
                }
            }
        }

        // Decay danger field
        for (i32 y = 0; y < world.env.height; y++)
        {
            for (i32 x = 0; x < world.env.width; x++)
            {
                world.info.danger.At(x, y) *= 0.9f;
            }
        }

        world.env.fire = newFire;
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

            if (!world.env.fire.InBounds(nx, ny)) continue;

            f32 humidity = world.env.humidity.At(nx, ny);
            f32 adjustedChance = spreadChance * (1.0f - humidity / 100.0f);

            if (world.env.fire.At(nx, ny) <= 0.0f && world.sim.random.Next01() < adjustedChance)
            {
                newFire.At(nx, ny) = intensity * 0.6f;
                world.events.Publish({EventType::FireStarted, world.sim.clock.currentTick, 0, nx, ny, intensity * 0.6f});
            }
        }
    }
};
