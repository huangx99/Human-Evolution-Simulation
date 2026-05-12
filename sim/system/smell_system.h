#pragma once

// SmellSystem: FIELD PROPAGATION ONLY.
//
// This system handles:
//   - Smell value decay over time
//   - Smell diffusion to adjacent cells
//   - Wind-driven smell transport
//
// This system must NOT handle:
//   - "What produces smell" → that's SemanticReactionSystem (e.g. Dead+Flesh → EmitSmell)
//   - "Rotting meat smells worse than rotting grass" → entity capabilities, not field logic
//   - "Fire produces smoke smell" → should be a SemanticReactionRule (Burning → EmitSmell)
//
// If you need entity-aware smell behavior, create a SemanticReactionRule instead.
//
// OWNERSHIP: Engine (sim/system/)
// READS: InformationModule (smell)
// WRITES: InformationModule (smell) via WriteNext
// PHASE: SimPhase::Propagation

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

        // Propagation: decay, diffusion, wind transport
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

        // BOUNDARY NOTE: "fire emits smell" is a semantic judgment.
        // Ideally this should be a SemanticReactionRule:
        //   HasCapability(HeatEmission) → EmitSmell(delta = intensity * 0.5)
        // Kept here as a simple coupling for now. Move to reaction system when
        // fire smell needs to depend on fuel type (wood smoke vs stone heat).
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
