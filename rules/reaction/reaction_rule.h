#pragma once

#include "core/types/types.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/affordance.h"
#include "sim/ecology/material_state.h"
#include <vector>
#include <string>
#include <functional>
#include <cmath>

// Identifies a material/element in the world
enum class ElementId : u16
{
    None = 0,

    // Environment elements
    Fire,
    Water,
    DryGrass,
    Smoke,
    Ash,

    // Future: materials
    Wood,
    Stone,
    Grass,
    Dirt,
    Sand,

    Count
};

// Identifies a readable/writable field on a cell
enum class FieldId : u16
{
    None = 0,

    // Environment fields
    Temperature,
    Humidity,
    Fire,
    WindSpeed,

    // Information fields
    Smell,
    Danger,
    Smoke,

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

// Field-based condition: checks a numeric field value
struct FieldCondition
{
    FieldId field = FieldId::None;
    ConditionOp op;
    f32 value;
};

// Capability-based condition: checks entity capabilities/states at a cell
struct CapabilityCondition
{
    // At least one entity at the cell must have ALL of these capabilities
    Capability requiredCapabilities = Capability::None;

    // At least one entity at the cell must have ALL of these affordances
    Affordance requiredAffordances = Affordance::None;

    // At least one entity at the cell must be in ALL of these states
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
    std::string scentType;  // for EmitSmell
    std::string customId;   // for Custom
};

struct ReactionRule
{
    std::string id;
    std::string name;

    // Inputs: what elements must be present at the cell
    std::vector<ElementId> inputs;

    // Field conditions: numeric field checks (temperature > 50, etc.)
    std::vector<FieldCondition> fieldConditions;

    // Capability conditions: entity capability/state checks
    std::vector<CapabilityCondition> capabilityConditions;

    // Outputs: what happens when reaction triggers
    std::vector<ReactionOutput> outputs;

    f32 baseProbability = 1.0f;
    bool repeatable = false;  // can trigger every tick

    // Backward compat alias
    std::vector<FieldCondition>& conditions = fieldConditions;
};

// Evaluate a field condition
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

// Legacy alias
using Condition = FieldCondition;
