#pragma once

// CulturalTraceDetectionSystem: detects cultural traces from stable
// GroupKnowledge + SocialPattern combinations.
//
// First version: danger_avoidance_trace
//   Triggered when:
//   - shared_danger_zone exists (GroupKnowledge)
//   - collective_avoidance pattern exists nearby (Pattern)
//   - Both meet confidence/observation thresholds
//
// PHASE: SimPhase::Analysis (after CollectiveAvoidanceSystem)
//
// READS: GroupKnowledge, Pattern, Simulation
// WRITES: CulturalTrace
// Does NOT write to Agent, Command, Memory, Pattern, PatternTemporalState.

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/group_knowledge/group_knowledge_module.h"
#include "sim/pattern/pattern_module.h"
#include "sim/cultural_trace/cultural_trace_module.h"
#include <vector>
#include <algorithm>
#include <cmath>

class CulturalTraceDetectionSystem : public ISystem
{
public:
    CulturalTraceDetectionSystem(CulturalTraceTypeId dangerAvoidanceType,
                                  PatternKey avoidancePatternKey,
                                  GroupKnowledgeTypeId dangerZoneType)
        : dangerAvoidanceType_(dangerAvoidanceType)
        , avoidancePatternKey_(avoidancePatternKey)
        , dangerZoneType_(dangerZoneType) {}

    void Update(SystemContext& ctx) override
    {
        auto& gk = ctx.GroupKnowledge();
        auto& patterns = ctx.Patterns();
        auto& traces = ctx.CulturalTrace();
        Tick tick = ctx.CurrentTick();

        // Get all shared_danger_zone records
        auto zones = gk.FindByType(dangerZoneType_);
        if (zones.empty()) return;

        // Get all collective_avoidance patterns
        auto avoidancePatterns = patterns.FindByType(avoidancePatternKey_);
        if (avoidancePatterns.empty()) return;

        // Sort for determinism
        std::vector<const GroupKnowledgeRecord*> sortedZones(zones.begin(), zones.end());
        std::sort(sortedZones.begin(), sortedZones.end(),
            [](const GroupKnowledgeRecord* a, const GroupKnowledgeRecord* b) {
                return a->id < b->id;
            });

        std::vector<const PatternRecord*> sortedPatterns(avoidancePatterns.begin(), avoidancePatterns.end());
        std::sort(sortedPatterns.begin(), sortedPatterns.end(),
            [](const PatternRecord* a, const PatternRecord* b) {
                return a->id < b->id;
            });

        for (const auto* zone : sortedZones)
        {
            // Gate: zone confidence must meet threshold
            if (zone->confidence < minZoneConfidence) continue;

            // Find matching avoidance pattern (spatial alignment)
            for (const auto* pattern : sortedPatterns)
            {
                if (pattern->confidence < minPatternConfidence) continue;
                if (pattern->observationCount < minPatternObservations) continue;

                f32 dx = static_cast<f32>(pattern->x - zone->origin.x);
                f32 dy = static_cast<f32>(pattern->y - zone->origin.y);
                f32 distSq = dx * dx + dy * dy;

                if (distSq > mergeRadiusSq) continue;

                // Both sources qualify — create or reinforce trace
                traces.AddOrReinforce(
                    dangerAvoidanceType_,
                    {pattern->id},
                    {zone->id},
                    zone->confidence * pattern->confidence,
                    tick);
                break;  // one pattern per zone is enough
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Simulation, AccessMode::Read},
            {ModuleTag::GroupKnowledge, AccessMode::Read},
            {ModuleTag::Pattern, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::CulturalTrace, AccessMode::Write}
        };
        static const char* const DEPS[] = {
            "GroupKnowledgeAggregationSystem",
            "CollectiveAvoidanceSystem"
        };
        return {"CulturalTraceDetectionSystem", SimPhase::Analysis,
                READS, 3, WRITES, 1, DEPS, 2, true, false};
    }

private:
    CulturalTraceTypeId dangerAvoidanceType_;
    PatternKey avoidancePatternKey_;
    GroupKnowledgeTypeId dangerZoneType_;

    static constexpr f32 minZoneConfidence = 0.35f;
    static constexpr f32 minPatternConfidence = 0.6f;
    static constexpr u32 minPatternObservations = 3;
    static constexpr f32 mergeRadiusSq = 25.0f;  // 5.0^2
};
