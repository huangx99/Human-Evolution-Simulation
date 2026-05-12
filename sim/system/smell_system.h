#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class SmellSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        Grid<f32> newSmell(world.width, world.height, 0.0f);

        for (i32 y = 0; y < world.height; y++)
        {
            for (i32 x = 0; x < world.width; x++)
            {
                f32 current = world.smell.At(x, y);

                // Step 1: Natural decay
                f32 value = current * decayRate;

                // Step 2: Diffusion - lose some to neighbors, gain from neighbors
                f32 lossToNeighbors = 0.0f;
                f32 gainFromNeighbors = 0.0f;

                static const i32 dx[] = {-1, 1, 0, 0};
                static const i32 dy[] = {0, 0, -1, 1};

                for (i32 i = 0; i < 4; i++)
                {
                    i32 nx = x + dx[i];
                    i32 ny = y + dy[i];

                    if (world.smell.InBounds(nx, ny))
                    {
                        lossToNeighbors += value * diffusionRate;
                        gainFromNeighbors += world.smell.At(nx, ny) * diffusionRate * 0.25f;
                    }
                }

                value = value - lossToNeighbors + gainFromNeighbors;

                // Step 3: Wind - carry smell downwind
                i32 wx = x - static_cast<i32>(world.wind.x);
                i32 wy = y - static_cast<i32>(world.wind.y);
                if (world.smell.InBounds(wx, wy))
                {
                    value += world.smell.At(wx, wy) * windStrength;
                }

                newSmell.At(x, y) = std::max(0.0f, value);
            }
        }

        // Emit food smell from burning cells
        for (i32 y = 0; y < world.height; y++)
        {
            for (i32 x = 0; x < world.width; x++)
            {
                if (world.fire.At(x, y) > 5.0f)
                {
                    newSmell.At(x, y) += world.fire.At(x, y) * 0.5f;
                }
            }
        }

        world.smell = newSmell;
    }

private:
    static constexpr f32 decayRate = 0.90f;
    static constexpr f32 diffusionRate = 0.05f;
    static constexpr f32 windStrength = 0.02f;
};
