#pragma once

// FireSystem: FIELD PROPAGATION ONLY.
//
// This system handles:
//   - Fire intensity decay over time
//   - Fire spread to adjacent cells (physics-based, not entity-based)
//   - Temperature/humidity side-effects of existing fire
//   - Danger field propagation
//
// PHASE: SimPhase::Propagation

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include <cmath>

class FireSystem : public ISystem
{
public:
    explicit FireSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : env_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& sim = ctx.Sim();
        auto& fields = ctx.Fields();
        auto& fm = ctx.GetFieldModule();
        i32 w = ctx.World().Width();
        i32 h = ctx.World().Height();

        // Copy current to next so we can read current while writing next
        fields.CopyCurrentToNext(env_.fire);
        fields.CopyCurrentToNext(env_.danger);

        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                f32 current = fm.Read(env_.fire, x, y);

                if (current > 0.0f)
                {
                    // Propagation: fire decays, spreads, heats, dries
                    fields.WriteNext(env_.fire, x, y, std::max(0.0f, current - burnRate));
                    SpreadFire(ctx, x, y, current);
                    fields.WriteNext(env_.temperature, x, y, fm.Read(env_.temperature, x, y) + current * 0.05f);
                    fields.WriteNext(env_.humidity, x, y, std::max(0.0f, fm.Read(env_.humidity, x, y) - current * 0.02f));
                    fields.WriteNext(env_.danger, x, y, std::max(fm.Read(env_.danger, x, y), current));
                }
                else
                {
                    f32 temp = fm.Read(env_.temperature, x, y);
                    f32 hum = fm.Read(env_.humidity, x, y);
                    if (temp > 50.0f && hum < 30.0f && sim.random.Next01() < 0.005f)
                    {
                        fields.WriteNext(env_.fire, x, y, 40.0f);
                        ctx.Events().Emit({EventType::FireStarted, sim.clock.currentTick, 0, x, y, 40.0f});
                    }
                }
            }
        }

        // Decay danger
        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                fields.WriteNext(env_.danger, x, y, fm.Read(env_.danger, x, y) * 0.9f);
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Environment, AccessMode::Read},
            {ModuleTag::Information, AccessMode::Read},
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Environment, AccessMode::Write},
            {ModuleTag::Information, AccessMode::Write},
            {ModuleTag::Event, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"FireSystem", SimPhase::Propagation, READS, 3, WRITES, 3, DEPS, 0, true, true};
    }

private:
    static constexpr f32 burnRate = 0.5f;
    static constexpr f32 spreadChance = 0.05f;
    static constexpr f32 spreadThreshold = 15.0f;

    HumanEvolution::EnvironmentContext env_;

    void SpreadFire(SystemContext& ctx, i32 x, i32 y, f32 intensity)
    {
        if (intensity < spreadThreshold) return;

        auto& sim = ctx.Sim();
        auto& fields = ctx.Fields();
        auto& fm = ctx.GetFieldModule();

        static const i32 dx[] = {-1, 1, 0, 0};
        static const i32 dy[] = {0, 0, -1, 1};

        for (i32 i = 0; i < 4; i++)
        {
            i32 nx = x + dx[i];
            i32 ny = y + dy[i];

            if (!fields.InBounds(env_.fire, nx, ny)) continue;

            f32 hum = fm.Read(env_.humidity, nx, ny);
            f32 adjustedChance = spreadChance * (1.0f - hum / 100.0f);

            if (fm.Read(env_.fire, nx, ny) <= 0.0f && sim.random.Next01() < adjustedChance)
            {
                fields.WriteNext(env_.fire, nx, ny, intensity * 0.6f);
                ctx.Events().Emit({EventType::FireStarted, sim.clock.currentTick, 0, nx, ny, intensity * 0.6f});
            }
        }
    }
};
