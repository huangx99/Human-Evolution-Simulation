#pragma once

// FirstStableFireUsageDetector: emits a one-time HistoryEvent when a stable fire
// zone pattern is first detected.
//
// This detector lives in rules/ because "fire" and "stable fire usage" are
// Human Evolution semantics, not engine concepts.

#include "sim/history/i_history_detector.h"
#include "sim/history/history_module.h"
#include "sim/history/history_registry.h"
#include "sim/pattern/pattern_module.h"
#include "sim/system/system_context.h"

class FirstStableFireUsageDetector : public IHistoryDetector
{
public:
    explicit FirstStableFireUsageDetector(HistoryKey eventKey, PatternKey sourcePatternKey,
                                          f32 minConfidence, u32 minObservations)
        : eventKey_(eventKey)
        , sourcePatternKey_(sourcePatternKey)
        , minConfidence_(minConfidence)
        , minObservations_(minObservations)
    {}

    void Scan(SystemContext& ctx, HistoryModule& history) override
    {
        if (fired_) return;

        auto& patterns = ctx.Patterns();
        auto matches = patterns.FindByType(sourcePatternKey_);

        for (const auto* p : matches)
        {
            if (p->confidence >= minConfidence_ &&
                p->observationCount >= minObservations_)
            {
                HistoryEvent evt;
                evt.typeKey = eventKey_;
                evt.typeId = HistoryRegistry::Instance().FindByKey(eventKey_);
                evt.tick = ctx.CurrentTick();
                evt.x = p->x;
                evt.y = p->y;
                evt.magnitude = p->magnitude;
                evt.confidence = p->confidence;
                evt.sourcePatterns.push_back(p->id);
                evt.title = "First Stable Fire Usage";

                history.Add(evt);
                fired_ = true;
                return;
            }
        }
    }

private:
    HistoryKey eventKey_;
    PatternKey sourcePatternKey_;
    f32 minConfidence_;
    u32 minObservations_;
    bool fired_ = false;
};
