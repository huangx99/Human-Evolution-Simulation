#pragma once

#include "sim/cultural_trace/cultural_trace_id.h"
#include "core/types/types.h"
#include <vector>

struct CulturalTraceRecord
{
    u64 id = 0;
    CulturalTraceTypeId typeId;

    f32 confidence = 0.0f;
    u32 reinforcementCount = 0;

    std::vector<u64> sourcePatternIds;
    std::vector<u64> sourceGroupKnowledgeRecordIds;

    Tick firstObservedTick = 0;
    Tick lastReinforcedTick = 0;
};
