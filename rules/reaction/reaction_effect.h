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

struct ReactionEffect
{
    EffectType type;

    // For AddState / RemoveState
    MaterialState state = MaterialState::None;

    // For AddCapability / RemoveCapability
    Capability capability = Capability::None;

    // For ModifyField / EmitSmell / EmitSmoke
    FieldId field = FieldId::None;
    f32 delta = 0.0f;       // additive change to field
    f32 setTo = 0.0f;       // absolute set (if delta is 0, use setTo)
};
