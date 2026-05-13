#pragma once

// MassDeathDetector: emits a HistoryEvent when a cluster of agent deaths
// occurs within a short time window.
//
// Reads from EventBus archive (EventType::AgentDied events).
// Uses rolling window + cooldown to avoid duplicate events.
//
// This detector lives in rules/ because "mass death" is a Human Evolution
// semantic interpretation, not an engine concept.

#include "sim/history/i_history_detector.h"
#include "sim/history/history_module.h"
#include "sim/history/history_registry.h"
#include "sim/system/system_context.h"
#include "sim/event/event.h"

class MassDeathDetector : public IHistoryDetector
{
public:
    struct Config
    {
        HistoryKey eventKey;
        u32 windowTicks = 10;       // rolling window size
        u32 threshold = 3;          // deaths in window to trigger
        u32 cooldownTicks = 50;     // minimum ticks between triggers
    };

    explicit MassDeathDetector(Config config) : config_(std::move(config)) {}

    void Scan(SystemContext& ctx, HistoryModule& history) override
    {
        Tick now = ctx.CurrentTick();

        // Cooldown check (skip if never triggered yet)
        if (lastTriggerTick_ > 0 && now < lastTriggerTick_ + config_.cooldownTicks)
            return;

        // Count AgentDied events in [now - windowTicks, now] (inclusive of current tick)
        Tick windowStart = (now >= config_.windowTicks) ? (now - config_.windowTicks) : 0;
        auto deaths = ctx.Events().GetArchive(windowStart, now + 1);

        u32 deathCount = 0;
        i32 sumX = 0, sumY = 0;
        std::vector<EntityId> deadIds;

        for (const auto& e : deaths)
        {
            if (e.type == EventType::AgentDied)
            {
                deathCount++;
                sumX += e.x;
                sumY += e.y;
                deadIds.push_back(e.entityId);
            }
        }

        if (deathCount >= config_.threshold)
        {
            HistoryEvent evt;
            evt.typeKey = config_.eventKey;
            evt.typeId = HistoryRegistry::Instance().FindByKey(config_.eventKey);
            evt.tick = now;
            evt.x = sumX / static_cast<i32>(deathCount);
            evt.y = sumY / static_cast<i32>(deathCount);
            evt.magnitude = static_cast<f32>(deathCount);
            evt.confidence = 1.0f;
            evt.involvedEntities = std::move(deadIds);
            evt.title = "Mass Death Event";

            history.Add(evt);
            lastTriggerTick_ = now;
        }
    }

private:
    Config config_;
    Tick lastTriggerTick_ = 0;
};
