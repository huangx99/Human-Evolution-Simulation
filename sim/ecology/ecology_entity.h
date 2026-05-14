#pragma once

#include "sim/ecology/material_id.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/affordance.h"
#include "sim/ecology/material_db.h"
#include "core/types/types.h"
#include <string>

// An entity in the ecology world.
// Composed of: base Material + current State + extra Capability/Affordance.
// The "extra" fields are for constructed/composite objects (e.g. torch = wood + burning + light).
struct EcologyEntity
{
    EntityId id = 0;
    std::string name;               // human-readable label (for debug/display)

    MaterialId material = MaterialId::None;
    MaterialState state = MaterialState::None;

    // Extra capabilities beyond what the base material provides
    Capability extraCapabilities = Capability::None;

    // Extra affordances beyond what the base material provides
    Affordance extraAffordances = Affordance::None;

    // Position in the world (if applicable)
    i32 x = -1;
    i32 y = -1;

    // Tick when the state last changed (used by StateDurationGreaterThan predicate)
    Tick stateChangedTick = 0;

    // --- Computed accessors ---

    // Total capabilities = material defaults + extras
    Capability GetCapabilities() const
    {
        return MaterialDB::GetCapabilities(material) | extraCapabilities;
    }

    // Total affordances = material defaults + extras
    Affordance GetAffordances() const
    {
        return MaterialDB::GetAffordances(material) | extraAffordances;
    }

    // Check if this entity has a specific capability
    bool HasCapability(Capability cap) const
    {
        return ::HasCapability(GetCapabilities(), cap);
    }

    // Check if this entity has a specific affordance
    bool HasAffordance(Affordance aff) const
    {
        return ::HasAffordance(GetAffordances(), aff);
    }

    // Check if this entity is in a specific state
    bool HasState(MaterialState s) const
    {
        return ::HasState(state, s);
    }

    // Add a state
    void AddState(MaterialState s)
    {
        state = state | s;
    }

    // Add extra capability
    void AddCapability(Capability cap)
    {
        extraCapabilities = extraCapabilities | cap;
    }

    // Add extra affordance
    void AddAffordance(Affordance aff)
    {
        extraAffordances = extraAffordances | aff;
    }
};
