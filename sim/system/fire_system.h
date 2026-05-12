#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class FireSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        auto& env = world.Env();
        auto& info = world.Info();
        auto& sim = world.Sim();

        // Copy current to next so we can read current while writing next
        env.fire.CopyCurrentToNext();
        info.danger.CopyCurrentToNext();

        for (i32 y = 0; y < env.height; y++)
        {
            for (i32 x = 0; x < env.width; x++)
            {
                f32 current = env.fire.At(x, y);

                if (current > 0.0f)
                {
                    env.fire.WriteNext(x, y) = std::max(0.0f, current - burnRate);
                    SpreadFire(world, x, y, current);
                    env.temperature.WriteNext(x, y) += current * 0.05f;
                    env.humidity.WriteNext(x, y) = std::max(0.0f, env.humidity.At(x, y) - current * 0.02f);
                    info.danger.WriteNext(x, y) = std::max(info.danger.At(x, y), current);
                }
                else
                {
                    f32 temp = env.temperature.At(x, y);
                    f32 hum = env.humidity.At(x, y);
                    if (temp > 50.0f && hum < 30.0f && sim.random.Next01() < 0.005f)
                    {
                        env.fire.WriteNext(x, y) = 40.0f;
                        world.events.Emit({EventType::FireStarted, sim.clock.currentTick, 0, x, y, 40.0f});
                    }
                }
            }
        }

        // Decay danger
        for (i32 y = 0; y < env.height; y++)
        {
            for (i32 x = 0; x < env.width; x++)
            {
                info.danger.WriteNext(x, y) = info.danger.At(x, y) * 0.9f;
            }
        }
    }

private:
    static constexpr f32 burnRate = 0.5f;
    static constexpr f32 spreadChance = 0.05f;
    static constexpr f32 spreadThreshold = 15.0f;

    void SpreadFire(WorldState& world, i32 x, i32 y, f32 intensity)
    {
        if (intensity < spreadThreshold) return;

        auto& env = world.Env();
        auto& sim = world.Sim();

        static const i32 dx[] = {-1, 1, 0, 0};
        static const i32 dy[] = {0, 0, -1, 1};

        for (i32 i = 0; i < 4; i++)
        {
            i32 nx = x + dx[i];
            i32 ny = y + dy[i];

            if (!env.fire.InBounds(nx, ny)) continue;

            f32 humidity = env.humidity.At(nx, ny);
            f32 adjustedChance = spreadChance * (1.0f - humidity / 100.0f);

            if (env.fire.At(nx, ny) <= 0.0f && sim.random.Next01() < adjustedChance)
            {
                env.fire.WriteNext(nx, ny) = intensity * 0.6f;
                world.events.Emit({EventType::FireStarted, sim.clock.currentTick, 0, nx, ny, intensity * 0.6f});
            }
        }
    }
};
