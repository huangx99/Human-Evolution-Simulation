#pragma once

#include "sim/event/event.h"
#include <vector>
#include <functional>
#include <array>

using EventHandler = std::function<void(const Event&)>;

class EventBus
{
public:
    // Subscribe a handler for a specific event type
    void Subscribe(EventType type, EventHandler handler)
    {
        handlers[static_cast<size_t>(type)].push_back(std::move(handler));
    }

    // Enqueue event for dispatch at EventResolve phase
    void Emit(const Event& event)
    {
        pending.push_back(event);
    }

    // Process all queued events: dispatch handlers, then archive
    void Dispatch()
    {
        for (const auto& event : pending)
        {
            for (auto& handler : handlers[static_cast<size_t>(event.type)])
            {
                handler(event);
            }
        }
        archive.insert(archive.end(), pending.begin(), pending.end());
        pending.clear();
    }

    // Get all archived events
    const std::vector<Event>& GetArchive() const { return archive; }

    // Get archived events of a specific type
    std::vector<Event> GetArchive(EventType type) const
    {
        std::vector<Event> result;
        for (const auto& e : archive)
        {
            if (e.type == type) result.push_back(e);
        }
        return result;
    }

    // Get archived events in a tick range [from, to)
    std::vector<Event> GetArchive(Tick from, Tick to) const
    {
        std::vector<Event> result;
        for (const auto& e : archive)
        {
            if (e.tick >= from && e.tick < to) result.push_back(e);
        }
        return result;
    }

    // Clear pending queue
    void ClearPending() { pending.clear(); }

    // Clear everything (handlers, pending, archive)
    void Clear()
    {
        for (auto& vec : handlers) vec.clear();
        pending.clear();
        archive.clear();
    }

private:
    std::array<std::vector<EventHandler>,
               static_cast<size_t>(EventType::Count)> handlers;
    std::vector<Event> pending;
    std::vector<Event> archive;
};
