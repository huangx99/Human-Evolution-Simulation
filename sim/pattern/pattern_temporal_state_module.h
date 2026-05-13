#pragma once

// PatternTemporalStateModule: cross-tick state for pattern detectors.
//
// Stores consecutive-tick counters used by detectors that need to track
// temporal persistence (e.g. "agent avoided zone for 3 consecutive ticks").
//
// This is derived pattern detection state — it only affects derived Pattern
// output, not simulation state. Excluded from FullHash by design.
//
// Generic: reusable for any pattern detector that needs temporal windows.
// Keys are (PatternTypeId, sourceRecordId) pairs.

#include "sim/world/module_registry.h"
#include "sim/pattern/pattern_id.h"
#include <vector>
#include <set>
#include <algorithm>

struct PatternTemporalCounter
{
    PatternTypeId patternType;
    u64 sourceRecordId = 0;      // e.g. GroupKnowledgeRecord::id
    u32 consecutiveTicks = 0;
    Tick lastUpdatedTick = 0;
};

struct PatternTemporalStateModule : public IModule
{
    std::vector<PatternTemporalCounter> counters;

    // Find an existing counter. Returns nullptr if not found.
    PatternTemporalCounter* Find(PatternTypeId type, u64 sourceRecordId)
    {
        for (auto& c : counters)
        {
            if (c.patternType == type && c.sourceRecordId == sourceRecordId)
                return &c;
        }
        return nullptr;
    }

    const PatternTemporalCounter* Find(PatternTypeId type, u64 sourceRecordId) const
    {
        for (const auto& c : counters)
        {
            if (c.patternType == type && c.sourceRecordId == sourceRecordId)
                return &c;
        }
        return nullptr;
    }

    // Find or create a counter.
    PatternTemporalCounter& GetOrCreate(PatternTypeId type, u64 sourceRecordId)
    {
        if (auto* existing = Find(type, sourceRecordId))
            return *existing;

        counters.push_back({type, sourceRecordId, 0, 0});
        return counters.back();
    }

    // Remove counters whose sourceRecordId is not in the active set.
    void PruneStale(const std::set<u64>& activeSourceIds)
    {
        counters.erase(
            std::remove_if(counters.begin(), counters.end(),
                [&activeSourceIds](const PatternTemporalCounter& c) {
                    return activeSourceIds.find(c.sourceRecordId) == activeSourceIds.end();
                }),
            counters.end());
    }
};
