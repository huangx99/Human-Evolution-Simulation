#pragma once

// SmellSystem: FIELD PROPAGATION ONLY.
//
// This system handles:
//   - Smell value decay over time
//   - Smell diffusion to adjacent cells
//   - Wind-driven smell transport
//
// PHASE: SimPhase::Propagation

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include <cmath>

class SmellSystem : public ISystem
{
public:
    explicit SmellSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : env_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& fields = ctx.Fields();
        auto& fm = ctx.GetFieldModule();
        i32 w = ctx.World().Width();
        i32 h = ctx.World().Height();

        // Copy current to next
        fields.CopyCurrentToNext(env_.smell);

        // Propagation: decay, diffusion, wind transport
        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                f32 current = fm.Read(env_.smell, x, y);
                f32 value = current * decayRate;

                f32 lossToNeighbors = 0.0f;
                f32 gainFromNeighbors = 0.0f;

                static const i32 dx[] = {-1, 1, 0, 0};
                static const i32 dy[] = {0, 0, -1, 1};

                for (i32 i = 0; i < 4; i++)
                {
                    i32 nx = x + dx[i];
                    i32 ny = y + dy[i];

                    if (fm.InBounds(env_.smell, nx, ny))
                    {
                        lossToNeighbors += value * diffusionRate;
                        gainFromNeighbors += fm.Read(env_.smell, nx, ny) * diffusionRate * 0.25f;
                    }
                }

                value = value - lossToNeighbors + gainFromNeighbors;

                f32 wx = fm.Read(env_.windX, 0, 0);
                f32 wy = fm.Read(env_.windY, 0, 0);
                i32 windSrcX = x - static_cast<i32>(wx);
                i32 windSrcY = y - static_cast<i32>(wy);
                if (fm.InBounds(env_.smell, windSrcX, windSrcY))
                {
                    value += fm.Read(env_.smell, windSrcX, windSrcY) * windStrength;
                }

                fields.WriteNext(env_.smell, x, y, std::max(0.0f, value));
            }
        }

        // Fire emits smell
        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                if (fm.Read(env_.fire, x, y) > 5.0f)
                {
                    fields.WriteNext(env_.smell, x, y, fm.Read(env_.smell, x, y) + fm.Read(env_.fire, x, y) * 0.5f);
                }
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Information, AccessMode::Read},
            {ModuleTag::Environment, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Information, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"SmellSystem", SimPhase::Propagation, READS, 2, WRITES, 1, DEPS, 0, true, true};
    }

private:
    static constexpr f32 decayRate = 0.90f;
    static constexpr f32 diffusionRate = 0.05f;
    static constexpr f32 windStrength = 0.02f;

    HumanEvolution::EnvironmentContext env_;
};
