#pragma once

#include "core/types/types.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/affordance.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/material_id.h"
#include "sim/ecology/field_id.h"

enum class PredicateType : u8
{
    HasCapability,      // entity at cell has this capability
    HasAffordance,      // entity at cell has this affordance
    HasState,           // entity at cell is in this state
    HasMaterial,        // entity at cell is this material
    FieldGreaterThan,   // numeric field > value
    FieldLessThan,      // numeric field < value
    FieldEquals,        // numeric field ~= value
    NearbyCapability,   // entity within radius has this capability
    NearbyState,        // entity within radius has this state
};

struct SemanticPredicate
{
    PredicateType type;

    // For HasCapability / NearbyCapability
    Capability capability = Capability::None;

    // For HasAffordance
    Affordance affordance = Affordance::None;

    // For HasState / NearbyState
    MaterialState state = MaterialState::None;

    // For HasMaterial
    MaterialId material = MaterialId::None;

    // For FieldGreaterThan / FieldLessThan / FieldEquals
    FieldId field = FieldId::None;
    f32 value = 0.0f;

    // For NearbyCapability / NearbyState
    i32 radius = 0;
};
