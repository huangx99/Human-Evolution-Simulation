#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class SmellSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        i32 w = world.info.width;
        i32 h = world.info.height;
        Grid<f32> newSmell(w, h, 0.0f);

        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                f32 current = world.info.smell.At(x, y);

                f32 value = current * decayRate;

                f32 lossToNeighbors = 0.0f;
                f32 gainFromNeighbors = 0.0f;

                static const i32 dx[] = {-1, 1, 0, 0};
                static const i32 dy[] = {0, 0, -1, 1};

                for (i32 i = 0; i < 4; i++)
                {
                    i32 nx = x + dx[i];
                    i32 ny = y + dy[i];

                    if (world.info.smell.InBounds(nx, ny))
                    {
                        lossToNeighbors += value * diffusionRate;
                        gainFromNeighbors += world.info.smell.At(nx, ny) * diffusionRate * 0.25f;
                    }
                }

                value = value - lossToNeighbors + gainFromNeighbors;

                i32 wx = x - static_cast<i32>(world.env.wind.x);
                i32 wy = y - static_cast<i32>(world.env.wind.y);
                if (world.info.smell.InBounds(wx, wy))
                {
                    value += world.info.smell.At(wx, wy) * windStrength;
                }

                newSmell.At(x, y) = std::max(0.0f, value);
            }
        }

        // Fire emits smell (food source)
        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                if (world.env.fire.At(x, y) > 5.0f)
                {
                    newSmell.At(x, y) += world.env.fire.At(x, y) * 0.5f;
                }
            }
        }

        world.info.smell = newSmell;
    }

private:
    static constexpr f32 decayRate = 0.90f;
    static constexpr f32 diffusionRate = 0.05f;
    static constexpr f32 windStrength = 0.02f;
};
