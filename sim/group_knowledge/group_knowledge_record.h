#pragma once

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/group_knowledge/group_knowledge_id.h"

// GroupKnowledgeRecord: a piece of shared knowledge that emerged from convergent
// individual observations. Not a direct world truth — an emergent social fact.
//
// Lifecycle:
//   1. CognitiveMemorySystem produces MemoryRecords from individual perception.
//   2. GroupKnowledgeAggregationSystem scans memories, clusters spatially,
//      and creates/reinforces GroupKnowledgeRecords.
//   3. Downstream systems (CognitiveAttention, Decision) can query the module
//      to bias behavior based on what "the group knows."

struct GroupKnowledgeRecord
{
    u64 id = 0;
    GroupKnowledgeTypeId typeId;
    Vec2i origin;                   // center of the knowledge zone
    f32 radius = 0.0f;              // how far this knowledge extends
    f32 confidence = 0.0f;          // aggregated confidence (0..1)
    u32 contributors = 0;           // number of distinct agents that contributed
    Tick firstObservedTick = 0;
    Tick lastReinforcedTick = 0;
};
