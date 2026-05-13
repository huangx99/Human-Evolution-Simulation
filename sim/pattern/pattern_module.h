#pragma once

#include "sim/world/module_registry.h"
#include "sim/pattern/pattern_record.h"
#include "sim/pattern/pattern_registry.h"
#include <vector>
#include <algorithm>

// PatternModule: stores detected patterns for a world.
//
// Patterns are derived data — recalculated during replay.
// Not included in world hash (HashTier::Full / Audit).

class PatternModule : public IModule
{
public:
    // Add a new pattern. Returns the assigned id.
    u64 Add(PatternRecord record)
    {
        record.id = nextPatternId_++;
        patterns_.push_back(record);
        return record.id;
    }

    // Update an existing pattern (by id). Returns false if not found.
    bool Update(u64 id, Tick tick, f32 confidence, f32 magnitude, u32 observationCount)
    {
        for (auto& p : patterns_)
        {
            if (p.id == id)
            {
                p.lastObservedTick = tick;
                p.confidence = confidence;
                p.magnitude = magnitude;
                p.observationCount = observationCount;
                return true;
            }
        }
        return false;
    }

    // Query: all patterns
    const std::vector<PatternRecord>& All() const { return patterns_; }

    // Query: patterns of a specific type
    std::vector<const PatternRecord*> FindByType(PatternKey typeKey) const
    {
        std::vector<const PatternRecord*> result;
        for (const auto& p : patterns_)
        {
            if (p.typeKey == typeKey)
                result.push_back(&p);
        }
        return result;
    }

    // Query: patterns near a position within radius
    std::vector<const PatternRecord*> FindNear(i32 x, i32 y, i32 radius) const
    {
        std::vector<const PatternRecord*> result;
        for (const auto& p : patterns_)
        {
            if (std::abs(p.x - x) <= radius && std::abs(p.y - y) <= radius)
                result.push_back(&p);
        }
        return result;
    }

    u64 NextPatternId() const { return nextPatternId_; }
    u32 Count() const { return static_cast<u32>(patterns_.size()); }

    // Enforce max pattern limit. Remove lowest-confidence patterns.
    void Prune(u32 maxPatterns)
    {
        if (patterns_.size() <= maxPatterns) return;

        std::sort(patterns_.begin(), patterns_.end(),
            [](const PatternRecord& a, const PatternRecord& b) {
                return a.confidence > b.confidence;
            });
        patterns_.resize(maxPatterns);
    }

    // Reset for testing
    void Clear()
    {
        patterns_.clear();
        nextPatternId_ = 1;
    }

private:
    std::vector<PatternRecord> patterns_;
    u64 nextPatternId_ = 1;
};
