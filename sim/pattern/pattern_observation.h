#pragma once

#include "core/types/types.h"

// PatternSignalType: categories of raw observations fed to detectors.

enum class PatternSignalType : u8
{
    AgentPosition,      // agent moved to (x, y)
    AgentAction,        // agent performed an action
    FieldValue,         // field at (x, y) has value
    EntityPresence,     // entity at (x, y)
};

// PatternObservation: a single raw fact fed to detectors.
//
// Detectors consume observations and accumulate evidence.
// The detection system populates observations each tick from world state.

struct PatternObservation
{
    Tick tick = 0;
    PatternSignalType type = PatternSignalType::AgentPosition;
    EntityId entityId = 0;
    i32 x = 0;
    i32 y = 0;
    f32 value = 0.0f;
};
