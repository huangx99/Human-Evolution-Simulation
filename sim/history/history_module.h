#pragma once

#include "sim/world/module_registry.h"
#include "sim/history/history_event.h"
#include <vector>

// HistoryModule: stores HistoryEvents for a world.
//
// HistoryEvents are derived data — they do NOT influence simulation behavior.
// Not included in world hash (HashTier::Full / Audit).
//
// First version: no pruning, all events kept.

class HistoryModule : public IModule
{
public:
    u64 Add(const HistoryEvent& event)
    {
        HistoryEvent copy = event;
        copy.id = nextEventId_++;
        events_.push_back(std::move(copy));
        return events_.back().id;
    }

    bool HasEventType(HistoryKey typeKey) const
    {
        for (const auto& e : events_)
            if (e.typeKey == typeKey) return true;
        return false;
    }

    const std::vector<HistoryEvent>& Events() const { return events_; }

    u32 Count() const { return static_cast<u32>(events_.size()); }

    void Clear()
    {
        events_.clear();
        nextEventId_ = 1;
    }

private:
    std::vector<HistoryEvent> events_;
    u64 nextEventId_ = 1;
};
