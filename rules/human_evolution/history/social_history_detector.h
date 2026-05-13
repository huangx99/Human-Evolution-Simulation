#pragma once

// SocialHistoryDetector: emits one-time HistoryEvents for the first occurrences
// of social emergence milestones:
//   - FirstSharedDangerMemory: first shared_danger_zone in GroupKnowledge
//   - FirstCollectiveAvoidance: first collective_avoidance pattern
//   - FirstDangerAvoidanceTrace: first danger_avoidance_trace
//
// Each event fires once and never again. Uses fired_ flags + HasEventType
// for double dedup.
//
// Lives in rules/ because these are Human Evolution semantics.

#include "sim/history/i_history_detector.h"
#include "sim/history/history_module.h"
#include "sim/history/history_registry.h"
#include "sim/group_knowledge/group_knowledge_module.h"
#include "sim/pattern/pattern_module.h"
#include "sim/cultural_trace/cultural_trace_module.h"
#include "sim/system/system_context.h"

class SocialHistoryDetector : public IHistoryDetector
{
public:
    SocialHistoryDetector(HistoryKey sharedDangerKey,
                          HistoryKey avoidanceKey,
                          HistoryKey traceKey,
                          GroupKnowledgeTypeId dangerZoneType,
                          PatternKey avoidancePatternKey,
                          CulturalTraceTypeId dangerTraceType)
        : sharedDangerKey_(sharedDangerKey)
        , avoidanceKey_(avoidanceKey)
        , traceKey_(traceKey)
        , dangerZoneType_(dangerZoneType)
        , avoidancePatternKey_(avoidancePatternKey)
        , dangerTraceType_(dangerTraceType) {}

    void Scan(SystemContext& ctx, HistoryModule& history) override
    {
        // 1. First Shared Danger Memory
        if (!firedSharedDanger_ && !history.HasEventType(sharedDangerKey_))
        {
            auto zones = ctx.GroupKnowledge().FindByType(dangerZoneType_);
            for (const auto* zone : zones)
            {
                if (zone->confidence >= minZoneConfidence)
                {
                    Emit(history, sharedDangerKey_, "First Shared Danger Memory",
                         zone->firstObservedTick, zone->origin.x, zone->origin.y,
                         zone->confidence);
                    firedSharedDanger_ = true;
                    break;
                }
            }
        }

        // 2. First Collective Avoidance
        if (!firedAvoidance_ && !history.HasEventType(avoidanceKey_))
        {
            auto patterns = ctx.Patterns().FindByType(avoidancePatternKey_);
            for (const auto* p : patterns)
            {
                if (p->observationCount >= minObservations)
                {
                    Emit(history, avoidanceKey_, "First Collective Avoidance",
                         p->firstDetectedTick, p->x, p->y, p->confidence);
                    firedAvoidance_ = true;
                    break;
                }
            }
        }

        // 3. First Danger Avoidance Trace
        if (!firedTrace_ && !history.HasEventType(traceKey_))
        {
            auto traces = ctx.CulturalTrace().FindByType(dangerTraceType_);
            if (!traces.empty())
            {
                const auto* t = traces[0];
                Emit(history, traceKey_, "First Danger Avoidance Trace",
                     t->firstObservedTick, 0, 0, t->confidence);
                firedTrace_ = true;
            }
        }
    }

private:
    HistoryKey sharedDangerKey_;
    HistoryKey avoidanceKey_;
    HistoryKey traceKey_;
    GroupKnowledgeTypeId dangerZoneType_;
    PatternKey avoidancePatternKey_;
    CulturalTraceTypeId dangerTraceType_;

    bool firedSharedDanger_ = false;
    bool firedAvoidance_ = false;
    bool firedTrace_ = false;

    static constexpr f32 minZoneConfidence = 0.35f;
    static constexpr u32 minObservations = 3;

    void Emit(HistoryModule& history, HistoryKey key, const char* title,
              Tick tick, i32 x, i32 y, f32 confidence)
    {
        HistoryEvent evt;
        evt.typeKey = key;
        evt.typeId = HistoryRegistry::Instance().FindByKey(key);
        evt.tick = tick;
        evt.x = x;
        evt.y = y;
        evt.confidence = confidence;
        evt.title = title;
        history.Add(evt);
    }
};
