#pragma once

#include "sim/event/event.h"
#include <vector>
#include <functional>
#include <array>

using EventHandler = std::function<void(const Event&)>;

class EventBus
{
public:
    void Subscribe(EventType type, EventHandler handler)
    {
        handlers[static_cast<size_t>(type)].push_back(std::move(handler));
    }

    void Publish(const Event& event)
    {
        for (auto& handler : handlers[static_cast<size_t>(event.type)])
        {
            handler(event);
        }
    }

    void Clear()
    {
        for (auto& vec : handlers)
        {
            vec.clear();
        }
    }

private:
    std::array<std::vector<EventHandler>,
               static_cast<size_t>(EventType::Count)> handlers;
};
