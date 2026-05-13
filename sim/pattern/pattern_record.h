#pragma once

#include "sim/pattern/pattern_id.h"
#include "core/types/types.h"

// PatternRecord: a detected long-term structure in the simulation.
//
// Patterns are derived data — they describe what has been observed over time.
// They do NOT influence simulation behavior. They are facts, not interpretations.

struct PatternRecord
{
    u64 id = 0;                      // unique instance id
    PatternKey typeKey;              // stable type identity
    PatternTypeId typeId;            // runtime type handle

    Tick firstDetectedTick = 0;
    Tick lastObservedTick = 0;

    EntityId ownerId = 0;            // 0 = world-level pattern
    i32 x = 0;
    i32 y = 0;

    f32 confidence = 0.0f;           // [0, 1] how certain the detection is
    f32 magnitude = 0.0f;            // how strong / large the pattern is
    u32 observationCount = 0;        // how many observations contributed
};
