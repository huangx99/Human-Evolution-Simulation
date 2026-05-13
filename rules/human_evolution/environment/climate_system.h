#pragma once

// ClimateSystem: FIELD PROPAGATION ONLY.
//
// This system handles:
//   - Temperature oscillation (day/night cycle via sine wave)
//   - Humidity derivation from temperature
//   - Wind direction randomization
//
// PHASE: SimPhase::Environment

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include <cmath>

class ClimateSystem : public ISystem
{
public:
    explicit ClimateSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : env_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& sim = ctx.Sim();
        auto& fields = ctx.Fields();
        auto& fm = ctx.GetFieldModule();
        Tick t = ctx.CurrentTick();
        i32 w = ctx.World().Width();
        i32 h = ctx.World().Height();

        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                f32 baseTemp = 20.0f + 10.0f * std::sin(static_cast<f32>(t) * 0.01f + x * 0.1f);
                f32 noise = sim.random.Next01() * 2.0f - 1.0f;
                fields.WriteNext(env_.temperature, x, y, baseTemp + noise);
            }
        }

        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                f32 temp = fm.Read(env_.temperature, x, y);
                f32 baseHumidity = 70.0f - (temp - 20.0f) * 1.5f;
                f32 noise = sim.random.Next01() * 5.0f - 2.5f;
                fields.WriteNext(env_.humidity, x, y, std::max(0.0f, std::min(100.0f, baseHumidity + noise)));
            }
        }

        if (t % 100 == 0)
        {
            f32 angle = sim.random.Next01() * 6.2831853f;
            fields.WriteNext(env_.windX, 0, 0, std::cos(angle));
            fields.WriteNext(env_.windY, 0, 0, std::sin(angle));
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
    HumanEvolution::EnvironmentContext env_;
};
