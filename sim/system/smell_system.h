#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class SmellSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        auto& env = world.Env();
        auto& info = world.Info();

        i32 w = info.width;
        i32 h = info.height;

        // Copy current to next
        info.smell.CopyCurrentToNext();

        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                f32 current = info.smell.At(x, y);
                f32 value = current * decayRate;

                f32 lossToNeighbors = 0.0f;
                f32 gainFromNeighbors = 0.0f;

                static const i32 dx[] = {-1, 1, 0, 0};
                static const i32 dy[] = {0, 0, -1, 1};

                for (i32 i = 0; i < 4; i++)
                {
                    i32 nx = x + dx[i];
                    i32 ny = y + dy[i];

                    if (info.smell.InBounds(nx, ny))
                    {
                        lossToNeighbors += value * diffusionRate;
                        gainFromNeighbors += info.smell.At(nx, ny) * diffusionRate * 0.25f;
                    }
                }

                value = value - lossToNeighbors + gainFromNeighbors;

                i32 wx = x - static_cast<i32>(env.wind.x);
                i32 wy = y - static_cast<i32>(env.wind.y);
                if (info.smell.InBounds(wx, wy))
                {
                    value += info.smell.At(wx, wy) * windStrength;
                }

                info.smell.WriteNext(x, y) = std::max(0.0f, value);
            }
        }

        // Fire emits smell
        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                if (env.fire.At(x, y) > 5.0f)
                {
                    info.smell.WriteNext(x, y) += env.fire.At(x, y) * 0.5f;
                }
            }
        }
    }

private:
    static constexpr f32 decayRate = 0.90f;
    static constexpr f32 diffusionRate = 0.05f;
    static constexpr f32 windStrength = 0.02f;
};
