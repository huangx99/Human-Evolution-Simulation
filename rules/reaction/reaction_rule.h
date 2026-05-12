#pragma once

#include "core/types/types.h"
#include <vector>
#include <string>
#include <functional>

enum class ConditionOp : u8
{
    Less,
    Greater,
    Equal,
    LessEqual,
    GreaterEqual
};

struct Condition
{
    std::string field;      // "temperature", "humidity", "fire", "smell", "danger"
    ConditionOp op;
    f32 value;
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

    // Inputs: what elements must be present
    std::vector<std::string> inputs;  // "fire", "dry_grass", "water", etc.

    // Conditions: field-based checks
    std::vector<Condition> conditions;

    // Outputs: what happens when reaction triggers
    std::vector<ReactionOutput> outputs;

    f32 baseProbability = 1.0f;
    bool repeatable = false;  // can trigger every tick
};

// Evaluate a condition against world fields
using FieldGetter = std::function<f32(i32 x, i32 y)>;

inline bool EvaluateCondition(const Condition& cond, f32 fieldValue)
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
