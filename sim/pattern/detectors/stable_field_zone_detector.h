#pragma once

// StableFieldZoneDetector: detects regions where a field stays above a threshold.
//
// Configured with a FieldIndex and threshold. Tracks how long each cell
// has been continuously above the threshold. When duration exceeds
// minDuration, a StableFieldZone pattern is emitted.
//
// This detector does NOT know what the field means.
// RulePack configures: "watch field X, threshold Y, duration Z, emit pattern P."

#include "sim/pattern/i_pattern_detector.h"
#include "sim/pattern/pattern_registry.h"
#include "sim/field/field_id.h"
#include <unordered_map>
#include <vector>
#include <utility>

// FieldWatchSpec: configuration for one field watch.

struct FieldWatchSpec
{
    PatternKey patternKey;       // pattern type to emit
    FieldIndex field;            // field to watch
    f32 threshold = 0.5f;        // value must exceed this
    u32 minDuration = 50;        // must be above threshold for this many ticks
};

class StableFieldZoneDetector : public IPatternDetector
{
public:
    explicit StableFieldZoneDetector(std::vector<FieldWatchSpec> watches)
        : watches_(std::move(watches)) {}

    void Observe(const PatternObservation& obs) override
    {
        // This detector doesn't use PatternObservations — it reads fields directly.
        (void)obs;
    }

    void ScanWorld(Tick currentTick, const FieldModule& fm, i32 worldW, i32 worldH) override
    {
        for (const auto& watch : watches_)
        {
            auto& state = states_[watch.patternKey.value];

            for (i32 y = 0; y < worldH; y++)
            {
                for (i32 x = 0; x < worldW; x++)
                {
                    u64 key = CellKey(x, y);
                    f32 value = fm.Read(watch.field, x, y);

                    if (value > watch.threshold)
                    {
                        auto it = state.cellDurations.find(key);
                        if (it != state.cellDurations.end())
                            it->second++;
                        else
                            state.cellDurations[key] = 1;
                    }
                    else
                    {
                        state.cellDurations.erase(key);
                    }
                }
            }

        }
    }

    void EndWindow(Tick currentTick, PatternModule& patterns, const PatternRuntimeConfig& config) override
    {
        for (const auto& watch : watches_)
        {
            auto& state = states_[watch.patternKey.value];
            PatternTypeId typeId = PatternRegistry::Instance().FindByKey(watch.patternKey);
            if (!typeId) continue;

            for (const auto& [key, duration] : state.cellDurations)
            {
                if (duration < watch.minDuration) continue;

                auto [cx, cy] = UnpackKey(key);

                // Check if pattern already exists at this location
                bool found = false;
                for (const auto& existing : patterns.FindByType(watch.patternKey))
                {
                    if (existing->x == cx && existing->y == cy)
                    {
                        f32 confidence = std::min(1.0f, static_cast<f32>(duration) / (watch.minDuration * 3.0f));
                        patterns.Update(existing->id, currentTick, confidence,
                                        static_cast<f32>(duration), existing->observationCount + 1);
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    PatternRecord record;
                    record.typeKey = watch.patternKey;
                    record.typeId = typeId;
                    record.firstDetectedTick = currentTick;
                    record.lastObservedTick = currentTick;
                    record.x = cx;
                    record.y = cy;
                    record.confidence = std::min(1.0f, static_cast<f32>(duration) / (watch.minDuration * 3.0f));
                    record.magnitude = static_cast<f32>(duration);
                    record.observationCount = 1;
                    patterns.Add(record);
                }
            }
        }
    }

    void Reset() override
    {
        states_.clear();
    }

private:
    std::vector<FieldWatchSpec> watches_;

    struct WatchState
    {
        std::unordered_map<u64, u32> cellDurations;  // cell key → consecutive ticks above threshold
    };
    std::unordered_map<u64, WatchState> states_;  // pattern key value → state

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
