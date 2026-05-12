#pragma once

#include "core/types/types.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/field_id.h"
#include <cstdint>

enum class EffectType : u8
{
    AddState,           // set a state flag on entity
    RemoveState,        // clear a state flag from entity
    AddCapability,      // add a capability to entity
    RemoveCapability,   // remove a capability from entity
    ModifyField,        // change a numeric field value
    EmitEvent,          // publish an event
    EmitSmell,          // add to smell field
    EmitSmoke,          // add to smoke field
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
    FieldId field = FieldId::None;
    FieldModifyMode mode = FieldModifyMode::Add;
    f32 value = 0.0f;       // delta (Add mode) or absolute (Set mode)

    // For EmitSmell / EmitSmoke
    f32 delta = 0.0f;       // additive change to smell/smoke field
};
