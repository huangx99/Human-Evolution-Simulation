#pragma once

// HighFrequencyPathDetector: detects cells that agents repeatedly visit.
//
// Uses a sliding window of cell visit counts. When a cell's visit count
// exceeds a threshold within the window, a HighFrequencyPath pattern is emitted.
//
// This detector does NOT know about roads, trails, or paths.
// It only knows: "this cell was visited many times recently."

#include "sim/pattern/i_pattern_detector.h"
#include "sim/pattern/pattern_registry.h"
#include <unordered_map>
#include <vector>
#include <utility>

class HighFrequencyPathDetector : public IPatternDetector
{
public:
    explicit HighFrequencyPathDetector(u32 visitThreshold = 10)
        : visitThreshold_(visitThreshold) {}

    void Observe(const PatternObservation& obs) override
    {
        if (obs.type != PatternSignalType::AgentPosition) return;

        u64 key = CellKey(obs.x, obs.y);
        visitCounts_[key]++;
    }

    void EndWindow(Tick currentTick, PatternModule& patterns, const PatternRuntimeConfig& config) override
    {
        PatternKey typeKey = PatternTypes::HighFrequencyPath();
        PatternTypeId typeId = PatternRegistry::Instance().FindByKey(typeKey);
        if (!typeId) return;

        for (const auto& [key, count] : visitCounts_)
        {
            if (count >= visitThreshold_)
            {
                auto [cx, cy] = UnpackKey(key);

                // Check if pattern already exists at this location
                bool found = false;
                for (const auto& existing : patterns.FindByType(typeKey))
                {
                    if (existing->x == cx && existing->y == cy)
                    {
                        f32 confidence = std::min(1.0f, static_cast<f32>(count) / (visitThreshold_ * 3.0f));
                        patterns.Update(existing->id, currentTick, confidence,
                                        static_cast<f32>(count), existing->observationCount + 1);
                        found = true;
                        break;
                    }
                }

                if (!found && count >= visitThreshold_)
                {
                    PatternRecord record;
                    record.typeKey = typeKey;
                    record.typeId = typeId;
                    record.firstDetectedTick = currentTick;
                    record.lastObservedTick = currentTick;
                    record.x = cx;
                    record.y = cy;
                    record.confidence = std::min(1.0f, static_cast<f32>(count) / (visitThreshold_ * 3.0f));
                    record.magnitude = static_cast<f32>(count);
                    record.observationCount = 1;
                    patterns.Add(record);
                }
            }
        }

        // Decay: decrement all counts (sliding window effect)
        for (auto it = visitCounts_.begin(); it != visitCounts_.end(); )
        {
            if (it->second <= 1)
                it = visitCounts_.erase(it);
            else
                it->second -= 1;
        }
    }

    void Reset() override
    {
        visitCounts_.clear();
    }

private:
    u32 visitThreshold_;
    std::unordered_map<u64, u32> visitCounts_;

    static u64 CellKey(i32 x, i32 y)
    {
        return (static_cast<u64>(static_cast<u32>(x)) << 32) |
                static_cast<u64>(static_cast<u32>(y));
    }

    static std::pair<i32, i32> UnpackKey(u64 key)
    {
        return {
            static_cast<i32>(static_cast<u32>(key >> 32)),
            static_cast<i32>(static_cast<u32>(key & 0xFFFFFFFF))
        };
    }
};
