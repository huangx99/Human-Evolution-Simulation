#pragma once

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/cognitive/memory_record.h"
#include "sim/cognitive/perceived_stimulus.h"
#include <algorithm>
#include <vector>

// MemoryConsolidationConfig is engine-level tuning only: it is based on
// generic memory mechanics and sense channels, never concrete world concepts.
// RulePacks may pass different values later without moving domain semantics here.
struct MemoryConsolidationConfig
{
    Tick internalMergeWindow = 12;
    Tick spatialMergeWindow = 8;

    i32 visionMergeRadius = 3;
    i32 touchMergeRadius = 2;
    i32 thermalMergeRadius = 3;
    i32 smellMergeRadius = 4;
    i32 soundMergeRadius = 5;
    i32 socialMergeRadius = 5;

    u32 stableReinforcementThreshold = 3;
    f32 stableMinConfidence = 0.5f;
    f32 stableMinStrength = 0.5f;
};

// MemoryConsolidation turns repeated short-term evidence into reinforcement.
// Keep this concept-agnostic: merge decisions use owner/concept identity, sense,
// time window, and spatial radius only. Domain meaning belongs in RulePacks.
struct MemoryConsolidation
{
    static MemoryRecord* TryMergeMemory(std::vector<MemoryRecord>& memories,
                                        const MemoryRecord& incoming,
                                        Tick now,
                                        const MemoryConsolidationConfig& config)
    {
        if (incoming.sense == SenseType::Internal)
            return TryMergeInternalMemory(memories, incoming, now, config);

        return TryMergeSpatialMemory(memories, incoming, now, config);
    }

    static bool ShouldStabilizeMemory(const MemoryRecord& memory,
                                      const MemoryConsolidationConfig& config)
    {
        return memory.kind == MemoryKind::ShortTerm &&
               memory.reinforcementCount >= config.stableReinforcementThreshold &&
               memory.confidence >= config.stableMinConfidence &&
               memory.strength >= config.stableMinStrength;
    }

private:
    static MemoryRecord* TryMergeInternalMemory(std::vector<MemoryRecord>& memories,
                                                const MemoryRecord& incoming,
                                                Tick now,
                                                const MemoryConsolidationConfig& config)
    {
        for (auto it = memories.rbegin(); it != memories.rend(); ++it)
        {
            auto& existing = *it;
            if (!CanMergeBase(existing, incoming, now, config.internalMergeWindow))
                continue;

            ReinforceMemory(existing, incoming, now);
            return &existing;
        }

        return nullptr;
    }

    static MemoryRecord* TryMergeSpatialMemory(std::vector<MemoryRecord>& memories,
                                               const MemoryRecord& incoming,
                                               Tick now,
                                               const MemoryConsolidationConfig& config)
    {
        const i32 radius = MergeRadiusForSense(incoming.sense, config);
        const i32 radiusSq = radius * radius;

        for (auto it = memories.rbegin(); it != memories.rend(); ++it)
        {
            auto& existing = *it;
            if (!CanMergeBase(existing, incoming, now, config.spatialMergeWindow))
                continue;
            if (DistanceSq(existing.location, incoming.location) > radiusSq)
                continue;

            ReinforceMemory(existing, incoming, now);
            return &existing;
        }

        return nullptr;
    }

    static bool CanMergeBase(const MemoryRecord& existing,
                             const MemoryRecord& incoming,
                             Tick now,
                             Tick mergeWindow)
    {
        if (existing.ownerId != incoming.ownerId)
            return false;
        if (existing.subject != incoming.subject)
            return false;
        if (existing.sense != incoming.sense)
            return false;
        if (existing.kind != MemoryKind::ShortTerm || incoming.kind != MemoryKind::ShortTerm)
            return false;
        if (existing.lastReinforcedTick > now)
            return false;
        return (now - existing.lastReinforcedTick) <= mergeWindow;
    }

    static void ReinforceMemory(MemoryRecord& existing,
                                const MemoryRecord& incoming,
                                Tick now)
    {
        // Prefer the stronger observation location. A weighted average would be
        // tempting, but integer replacement is deterministic and easier to audit.
        const bool incomingIsAtLeastAsStrong = incoming.strength >= existing.strength;

        existing.strength = std::max(existing.strength, incoming.strength);
        existing.emotionalWeight = incoming.emotionalWeight;
        existing.confidence = std::max(existing.confidence, incoming.confidence);
        if (incomingIsAtLeastAsStrong)
            existing.location = incoming.location;
        existing.lastReinforcedTick = now;
        existing.sourceStimulusId = incoming.sourceStimulusId;
        existing.reinforcementCount++;
        existing.resultTags = incoming.resultTags;
    }

    static i32 MergeRadiusForSense(SenseType sense,
                                   const MemoryConsolidationConfig& config)
    {
        switch (sense)
        {
        case SenseType::Vision:  return config.visionMergeRadius;
        case SenseType::Touch:   return config.touchMergeRadius;
        case SenseType::Thermal: return config.thermalMergeRadius;
        case SenseType::Smell:   return config.smellMergeRadius;
        case SenseType::Sound:   return config.soundMergeRadius;
        case SenseType::Social:  return config.socialMergeRadius;
        case SenseType::Internal:return 0;
        default:                 return config.visionMergeRadius;
        }
    }

    static i32 DistanceSq(const Vec2i& a, const Vec2i& b)
    {
        const i32 dx = a.x - b.x;
        const i32 dy = a.y - b.y;
        return dx * dx + dy * dy;
    }
};
