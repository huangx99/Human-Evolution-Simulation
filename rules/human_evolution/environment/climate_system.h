#pragma once

// ClimateSystem: FIELD PROPAGATION ONLY.
//
// This system handles:
//   - Temperature oscillation (day/night cycle via sine wave)
//   - Humidity derivation from temperature
//   - Wind direction randomization
//
// OWNERSHIP: Engine (sim/system/)
// READS: FieldModule (temperature, humidity), SimulationModule (random)
// WRITES: FieldModule (temperature, humidity) via FieldWriter
// PHASE: SimPhase::Environment

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include <cmath>

class ClimateSystem : public ISystem
{
public:
    void Update(SystemContext& ctx) override
    {
        if (!initialized_)
        {
            auto& fm = ctx.GetFieldModule();
            temperature_ = fm.FindByKey(FieldKey("human_evolution.temperature"));
            humidity_    = fm.FindByKey(FieldKey("human_evolution.humidity"));
            windX_       = fm.FindByKey(FieldKey("human_evolution.wind_x"));
            windY_       = fm.FindByKey(FieldKey("human_evolution.wind_y"));
            width_ = ctx.World().Width();
            height_ = ctx.World().Height();
            initialized_ = true;
        }

        auto& sim = ctx.Sim();
        auto& fields = ctx.Fields();
        auto& fm = ctx.GetFieldModule();
        Tick t = ctx.CurrentTick();

        for (i32 y = 0; y < height_; y++)
        {
            for (i32 x = 0; x < width_; x++)
            {
                f32 baseTemp = 20.0f + 10.0f * std::sin(static_cast<f32>(t) * 0.01f + x * 0.1f);
                f32 noise = sim.random.Next01() * 2.0f - 1.0f;
                fields.WriteNext(temperature_, x, y, baseTemp + noise);
            }
        }

        for (i32 y = 0; y < height_; y++)
        {
            for (i32 x = 0; x < width_; x++)
            {
                f32 temp = fm.Read(temperature_, x, y);
                f32 baseHumidity = 70.0f - (temp - 20.0f) * 1.5f;
                f32 noise = sim.random.Next01() * 5.0f - 2.5f;
                fields.WriteNext(humidity_, x, y, std::max(0.0f, std::min(100.0f, baseHumidity + noise)));
            }
        }

        if (t % 100 == 0)
        {
            f32 angle = sim.random.Next01() * 6.2831853f;
            fields.WriteNext(windX_, 0, 0, std::cos(angle));
            fields.WriteNext(windY_, 0, 0, std::sin(angle));
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Environment, AccessMode::ReadWrite},
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Environment, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"ClimateSystem", SimPhase::Environment, READS, 2, WRITES, 1, DEPS, 0, true, true};
    }

private:
    bool initialized_ = false;
    FieldIndex temperature_, humidity_, windX_, windY_;
    i32 width_ = 0, height_ = 0;
};
