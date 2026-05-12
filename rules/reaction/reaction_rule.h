#pragma once

#include "core/types/types.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/affordance.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/field_id.h"
#include <vector>
#include <string>
#include <cmath>

// DEPRECATED: Use SemanticReactionRule for new code.
// Kept for backward compatibility during migration.
// New rules MUST use SemanticReactionSystem with HasCapability/HasState/HasMaterial predicates.
// Do NOT add new ElementId values. Do NOT add new ReactionRule rules.

// Identifies a material/element in the world
enum class [[deprecated("Use MaterialId + Capability predicates instead")]] ElementId : u16
{
    None = 0,
    Fire,
    Water,
    DryGrass,
    Smoke,
    Ash,
    Wood,
    Stone,
    Grass,
    Dirt,
    Sand,
    Count
};

enum class ConditionOp : u8
{
    Less,
    Greater,
    Equal,
    LessEqual,
    GreaterEqual
};

struct FieldCondition
{
    FieldId field = FieldId::None;
    ConditionOp op;
    f32 value;
};

struct CapabilityCondition
{
    Capability requiredCapabilities = Capability::None;
    Affordance requiredAffordances = Affordance::None;
    MaterialState requiredStates = MaterialState::None;
};

enum class OutputType : u8
{
    IncreaseFire,
    DecreaseFire,
    EmitSmell,
    EmitSmoke,
    SetDanger,
    DamageAgent,
    Custom
};

struct ReactionOutput
{
    OutputType type;
    f32 value = 0.0f;
    std::string scentType;
    std::string customId;
};

// DEPRECATED: Use SemanticReactionRule instead.
struct [[deprecated("Use SemanticReactionRule instead")]] ReactionRule
{
    std::string id;
    std::string name;
    std::vector<ElementId> inputs;
    std::vector<FieldCondition> fieldConditions;
    std::vector<CapabilityCondition> capabilityConditions;
    std::vector<ReactionOutput> outputs;
    f32 baseProbability = 1.0f;
    bool repeatable = false;
    std::vector<FieldCondition>& conditions = fieldConditions;
};

inline bool EvaluateCondition(const FieldCondition& cond, f32 fieldValue)
{
    switch (cond.op)
    {
    case ConditionOp::Less:         return fieldValue < cond.value;
    case ConditionOp::Greater:      return fieldValue > cond.value;
    case ConditionOp::Equal:        return std::abs(fieldValue - cond.value) < 0.01f;
    case ConditionOp::LessEqual:    return fieldValue <= cond.value;
    case ConditionOp::GreaterEqual: return fieldValue >= cond.value;
    default: return false;
    }
}

using Condition = FieldCondition;
