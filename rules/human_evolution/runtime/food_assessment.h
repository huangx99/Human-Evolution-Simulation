#pragma once

// FoodAssessment: unified food evaluation for all systems.
//
// Now delegates to BehaviorTable instead of hardcoded if-else chains.
// BehaviorTable is the single source of truth for food properties.
// FoodValidity and FoodEffect are defined in behavior_table.h.

#include "core/types/types.h"
#include "sim/ecology/ecology_entity.h"
#include "sim/ecology/behavior_table.h"

struct Agent;

struct FoodAssessment
{
    FoodValidity validity = FoodValidity::NotFood;
    f32 nutrition = 0.0f;
    f32 risk = 0.0f;
    bool consumable = false;
};

struct FoodAssessor
{
    static FoodAssessment Assess(const EcologyEntity& e, const Agent& agent)
    {
        FoodEffect effect = BehaviorTable::Instance().GetFoodEffect(
            e.material, e.state, agent);
        return {effect.validity, effect.nutrition, effect.risk, effect.consumable};
    }

    static bool IsConsumable(const EcologyEntity& e, const Agent& agent)
    {
        return Assess(e, agent).consumable;
    }

    static bool IsFoodTarget(const EcologyEntity& e, const Agent& agent)
    {
        auto v = Assess(e, agent).validity;
        return v == FoodValidity::EdibleFresh || v == FoodValidity::EdibleRisky;
    }
};
