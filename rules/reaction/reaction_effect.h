#pragma once

#include "core/types/types.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/material_state.h"
#include "sim/field/field_id.h"
#include <cstdint>

// Architecture: state/field mutations go through CommandBuffer (deterministic, replayable).
// EmitEvent goes directly through EventBus (side-effect free notification, not a state mutation).
// Do NOT add new effects that bypass CommandBuffer for state changes.
enum class EffectType : u8
{
    AddState,           // set a state flag on entity       → CommandBuffer
    RemoveState,        // clear a state flag from entity   → CommandBuffer
    AddCapability,      // add a capability to entity       → CommandBuffer
    RemoveCapability,   // remove a capability from entity  → CommandBuffer
    ModifyField,        // change a numeric field value     → CommandBuffer
    EmitEvent,          // publish an event notification    → EventBus (direct, not Command)
    EmitSmell,          // add to smell field               → CommandBuffer
    EmitSmoke,          // add to smoke field               → CommandBuffer
};

enum class FieldModifyMode : u8
{
    Add,    // additive: current + value
    Set,    // absolute: replace with value
};

struct ReactionEffect
{
    EffectType type;

    // For AddState / RemoveState
    MaterialState state = MaterialState::None;

    // For AddCapability / RemoveCapability
    Capability capability = Capability::None;

    // For ModifyField
    FieldKey field;
    FieldModifyMode mode = FieldModifyMode::Add;
    f32 value = 0.0f;       // delta (Add mode) or absolute (Set mode)

    // For EmitSmell / EmitSmoke
    f32 delta = 0.0f;       // additive change to smell/smoke field
};
