#pragma once

// FireSystem: FIELD PROPAGATION ONLY.
//
// This system handles:
//   - Fire intensity decay over time
//   - Fire spread to adjacent cells (physics-based, not entity-based)
//   - Temperature/humidity side-effects of existing fire
//   - Danger field propagation
//
// OWNERSHIP: Engine (sim/system/)
// READS: FieldModule (fire, temperature, humidity, danger), SimulationModule (random)
// WRITES: FieldModule (fire, temperature, humidity, danger) via FieldWriter
// PHASE: SimPhase::Propagation

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include <cmath>

class FireSystem : public ISystem
{
public:
    void Update(SystemContext& ctx) override
    {
        if (!initialized_)
        {
            auto& fm = ctx.GetFieldModule();
            fire_        = fm.FindByKey(FieldKey("human_evolution.fire"));
            temperature_ = fm.FindByKey(FieldKey("human_evolution.temperature"));
            humidity_    = fm.FindByKey(FieldKey("human_evolution.humidity"));
            danger_      = fm.FindByKey(FieldKey("human_evolution.danger"));
            width_ = ctx.World().Width();
            height_ = ctx.World().Height();
            initialized_ = true;
        }

        auto& sim = ctx.Sim();
        auto& fields = ctx.Fields();
        auto& fm = ctx.GetFieldModule();

        // Copy current to next so we can read current while writing next
        fields.CopyCurrentToNext(fire_);
        fields.CopyCurrentToNext(danger_);

        for (i32 y = 0; y < height_; y++)
        {
            for (i32 x = 0; x < width_; x++)
            {
                f32 current = fm.Read(fire_, x, y);

                if (current > 0.0f)
                {
                    // Propagation: fire decays, spreads, heats, dries
                    fields.WriteNext(fire_, x, y, std::max(0.0f, current - burnRate));
                    SpreadFire(ctx, x, y, current);
                    fields.WriteNext(temperature_, x, y, fm.Read(temperature_, x, y) + current * 0.05f);
                    fields.WriteNext(humidity_, x, y, std::max(0.0f, fm.Read(humidity_, x, y) - current * 0.02f));
                    fields.WriteNext(danger_, x, y, std::max(fm.Read(danger_, x, y), current));
                }
                else
                {
                    f32 temp = fm.Read(temperature_, x, y);
                    f32 hum = fm.Read(humidity_, x, y);
                    if (temp > 50.0f && hum < 30.0f && sim.random.Next01() < 0.005f)
                    {
                        fields.WriteNext(fire_, x, y, 40.0f);
                        ctx.Events().Emit({EventType::FireStarted, sim.clock.currentTick, 0, x, y, 40.0f});
                    }
                }
            }
        }

        // Decay danger
        for (i32 y = 0; y < height_; y++)
        {
            for (i32 x = 0; x < width_; x++)
            {
                fields.WriteNext(danger_, x, y, fm.Read(danger_, x, y) * 0.9f);
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

    bool initialized_ = false;
    FieldIndex fire_, temperature_, humidity_, danger_;
    i32 width_ = 0, height_ = 0;

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

            if (!fields.InBounds(fire_, nx, ny)) continue;

            f32 hum = fm.Read(humidity_, nx, ny);
            f32 adjustedChance = spreadChance * (1.0f - hum / 100.0f);

            if (fm.Read(fire_, nx, ny) <= 0.0f && sim.random.Next01() < adjustedChance)
            {
                fields.WriteNext(fire_, nx, ny, intensity * 0.6f);
                ctx.Events().Emit({EventType::FireStarted, sim.clock.currentTick, 0, nx, ny, intensity * 0.6f});
            }
        }
    }
};
