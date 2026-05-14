#pragma once

// FoodSourceSystem: rule-layer food source emission.
//
// Edible + digestible ecology entities continuously emit into the food field.
// SmellSystem only transports generic smell; this system defines which ecology
// objects are actually food.
//
// PHASE: SimPhase::Propagation, after SmellSystem.

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include <algorithm>
#include <cmath>

class FoodSourceSystem : public ISystem
{
public:
    explicit FoodSourceSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : envCtx_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& fields = ctx.World().Fields();
        i32 w = ctx.World().Width();
        i32 h = ctx.World().Height();

        for (i32 y = 0; y < h; ++y)
        {
            for (i32 x = 0; x < w; ++x)
            {
                if (!fields.InBounds(envCtx_.food, x, y)) continue;
                fields.WriteNext(envCtx_.food, x, y, fields.Read(envCtx_.food, x, y) * decayRate);
            }
        }

        for (const auto& entity : ctx.World().Ecology().entities.All())
        {
            if (!IsActiveFood(entity)) continue;

            for (i32 y = entity.y - emissionRadius; y <= entity.y + emissionRadius; ++y)
            {
                for (i32 x = entity.x - emissionRadius; x <= entity.x + emissionRadius; ++x)
                {
                    if (!fields.InBounds(envCtx_.food, x, y)) continue;

                    i32 dx = x - entity.x;
                    i32 dy = y - entity.y;
                    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy));
                    if (dist > emissionRadius) continue;

                    f32 falloff = 1.0f / (1.0f + dist);
                    f32 amount = BaseEmission(entity) * falloff;
                    f32& food = fields.WriteNextRef(envCtx_.food, x, y);
                    food = std::min(maxFoodSignal, food + amount);
                }
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Ecology, AccessMode::Read},
            {ModuleTag::Information, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Information, AccessMode::Write}
        };
        static const char* const DEPS[] = {"SmellSystem"};
        return {"FoodSourceSystem", SimPhase::Propagation, READS, 2, WRITES, 1, DEPS, 1, true, false};
    }

private:
    HumanEvolution::EnvironmentContext envCtx_;

    static bool IsActiveFood(const EcologyEntity& entity)
    {
        return entity.HasCapability(Capability::Edible) &&
               entity.HasCapability(Capability::Digestible) &&
               !entity.HasCapability(Capability::Toxic) &&
               !entity.HasState(MaterialState::Burning) &&
               !entity.HasState(MaterialState::Dead) &&
               !entity.HasState(MaterialState::Decomposing);
    }

    static f32 BaseEmission(const EcologyEntity& entity)
    {
        if (entity.material == MaterialId::Fruit) return 36.0f;
        if (entity.material == MaterialId::Flesh) return 24.0f;
        return 18.0f;
    }

    static constexpr i32 emissionRadius = 18;
    static constexpr f32 decayRate = 0.82f;
    static constexpr f32 maxFoodSignal = 90.0f;
};
