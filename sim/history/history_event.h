#pragma once

#include "core/types/types.h"
#include "sim/history/history_id.h"
#include <vector>
#include <string>

// HistoryEvent: a significant historical occurrence derived from Pattern/World/Agent state.
//
// HistoryEvents are derived data — they do NOT influence simulation behavior.
// Not included in world hash (HashTier::Full / Audit).
// Not replayed (replay only reapplies commands).

struct HistoryEvent
{
    u64 id = 0;                        // unique instance id
    HistoryKey typeKey;                // stable type identity
    HistoryTypeId typeId;              // runtime type handle

    Tick tick = 0;                     // when it happened
    i32 x = 0;
    i32 y = 0;

    f32 magnitude = 0.0f;             // how significant
    f32 confidence = 0.0f;            // [0, 1] how certain

    std::vector<u64> sourcePatterns;  // PatternRecord ids that triggered this
    std::vector<EntityId> involvedEntities; // agents/entities involved

    std::string title;                // debug name, e.g. "First Stable Fire Usage"
};
