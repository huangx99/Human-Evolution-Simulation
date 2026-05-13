#pragma once

// CollectiveAvoidanceSystem: detects collective avoidance patterns.
//
// When a shared_danger_zone exists and multiple agents are nearby but NO agent
// is inside the zone for consecutive ticks, a collective_avoidance pattern is
// emitted.
//
// First version: proximity-based avoidance detector.
// It does not infer path planning or intent.
//
// Phase 2.3 first implementation; may migrate to IPatternDetector once
// PatternRuntime supports module-rich detectors.
//
// PHASE: SimPhase::Analysis (after GroupKnowledgeAggregationSystem)
//
// READS: Simulation, Agent, GroupKnowledge, PatternTemporalState
// WRITES: Pattern, PatternTemporalState
// Does NOT write to Command, Memory, or Agent modules.

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/group_knowledge/group_knowledge_module.h"
#include "sim/pattern/pattern_module.h"
#include "sim/pattern/pattern_id.h"
#include "sim/pattern/pattern_temporal_state_module.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <set>

class CollectiveAvoidanceSystem : public ISystem
{
public:
    CollectiveAvoidanceSystem(PatternTypeId type, PatternKey key,
                              GroupKnowledgeTypeId dangerZoneType)
        : type_(type), key_(key), dangerZoneType_(dangerZoneType) {}

    void Update(SystemContext& ctx) override
    {
        auto& gk = ctx.GroupKnowledge();
        auto& agents = ctx.Agents();
        auto& patterns = ctx.Patterns();
        auto& temporalState = ctx.PatternTemporalState();
        Tick tick = ctx.CurrentTick();

        // Get all shared_danger_zone records
        auto zones = gk.FindByType(dangerZoneType_);
        if (zones.empty()) return;

        // Build deterministic sorted agent list
        std::vector<const Agent*> sortedAgents;
        sortedAgents.reserve(agents.agents.size());
        for (const auto& a : agents.agents)
            sortedAgents.push_back(&a);
        std::sort(sortedAgents.begin(), sortedAgents.end(),
            [](const Agent* a, const Agent* b) { return a->id < b->id; });

        // Sort zones by record id for determinism
        std::vector<const GroupKnowledgeRecord*> sortedZones(zones.begin(), zones.end());
        std::sort(sortedZones.begin(), sortedZones.end(),
            [](const GroupKnowledgeRecord* a, const GroupKnowledgeRecord* b) {
                return a->id < b->id;
            });

        // Track active zone ids for stale state cleanup
        std::set<u64> activeZoneIds;

        for (const auto* zone : sortedZones)
        {
            activeZoneIds.insert(zone->id);

            f32 zoneRadiusSq = zone->radius * zone->radius;
            f32 nearbyRadiusSq = (zone->radius * nearbyMultiplier) * (zone->radius * nearbyMultiplier);

            u32 insideCount = 0;
            u32 nearbyCount = 0;

            for (const auto* agent : sortedAgents)
            {
                f32 dx = static_cast<f32>(agent->position.x - zone->origin.x);
                f32 dy = static_cast<f32>(agent->position.y - zone->origin.y);
                f32 distSq = dx * dx + dy * dy;

                if (distSq <= zoneRadiusSq)
                    insideCount++;
                else if (distSq <= nearbyRadiusSq)
                    nearbyCount++;
            }

            // If any agent is inside the zone, reset counter
            if (insideCount > 0)
            {
                auto* counter = temporalState.Find(type_, zone->id);
                if (counter)
                    counter->consecutiveTicks = 0;
                continue;
            }

            // If enough agents nearby but none inside, increment
            if (nearbyCount >= minNearbyAgents)
            {
                auto& counter = temporalState.GetOrCreate(type_, zone->id);
                counter.consecutiveTicks++;
                counter.lastUpdatedTick = tick;

                // If threshold met, emit or reinforce pattern
                if (counter.consecutiveTicks >= avoidanceThreshold)
                    EmitOrReinforce(patterns, *zone, tick);
            }
        }

        // Cleanup stale temporal state
        temporalState.PruneStale(activeZoneIds);
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Simulation, AccessMode::Read},
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::GroupKnowledge, AccessMode::Read},
            {ModuleTag::PatternTemporalState, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Pattern, AccessMode::ReadWrite},
            {ModuleTag::PatternTemporalState, AccessMode::ReadWrite}
        };
        static const char* const DEPS[] = {"GroupKnowledgeAggregationSystem"};
        return {"CollectiveAvoidanceSystem", SimPhase::Analysis,
                READS, 4, WRITES, 2, DEPS, 1, true, false};
    }

private:
    PatternTypeId type_;
    PatternKey key_;
    GroupKnowledgeTypeId dangerZoneType_;

    static constexpr f32 nearbyMultiplier = 1.5f;
    static constexpr u32 minNearbyAgents = 2;
    static constexpr u32 avoidanceThreshold = 3;
    static constexpr f32 emitConfidence = 0.7f;
    static constexpr f32 mergeRadiusSq = 25.0f;  // 5.0^2

    void EmitOrReinforce(PatternModule& patterns, const GroupKnowledgeRecord& zone, Tick tick)
    {
        // Dedup: find existing pattern by type + proximity
        auto existing = patterns.FindByType(key_);

        const PatternRecord* bestMatch = nullptr;
        f32 bestDistSq = mergeRadiusSq + 1.0f;

        for (const auto* p : existing)
        {
            f32 dx = static_cast<f32>(p->x - zone.origin.x);
            f32 dy = static_cast<f32>(p->y - zone.origin.y);
            f32 distSq = dx * dx + dy * dy;

            if (distSq <= mergeRadiusSq)
            {
                // Select nearest; tie-break by id (smaller wins)
                if (distSq < bestDistSq || (distSq == bestDistSq && p->id < bestMatch->id))
                {
                    bestMatch = p;
                    bestDistSq = distSq;
                }
            }
        }

        if (bestMatch)
        {
            patterns.Update(bestMatch->id, tick, emitConfidence,
                            zone.radius, bestMatch->observationCount + 1);
        }
        else
        {
            PatternRecord rec;
            rec.typeKey = key_;
            rec.typeId = type_;
            rec.x = zone.origin.x;
            rec.y = zone.origin.y;
            rec.firstDetectedTick = tick;
            rec.lastObservedTick = tick;
            rec.confidence = emitConfidence;
            rec.magnitude = zone.radius;
            rec.observationCount = 1;
            patterns.Add(rec);
        }
    }
};
