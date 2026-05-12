#pragma once

#include "sim/command/command.h"
#include <vector>

struct WorldState;

class CommandBuffer
{
public:
    // Submit a typed command. Template constrains T to valid command types.
    // Auto-wraps into Command variant via MakeCommand.
    template<typename T>
    void Submit(Tick tick, T&& cmd)
    {
        static_assert(IsCommandType<std::decay_t<T>>::value,
                      "Unsupported command type");
        pending.push_back({tick, MakeCommand(std::forward<T>(cmd))});
    }

    // Push alias for Submit (backward compat, e.g. tests)
    template<typename T>
    void Push(Tick tick, T&& cmd)
    {
        Submit(tick, std::forward<T>(cmd));
    }

    void Apply(WorldState& world);

    bool IsSpatialDirty() const { return spatialDirty; }
    void ClearSpatialDirty() { spatialDirty = false; }

    const std::vector<QueuedCommand>& GetHistory() const { return history; }
    size_t HistoryCount() const { return history.size(); }
    void ClearHistory() { history.clear(); }

private:
    std::vector<QueuedCommand> pending;
    std::vector<QueuedCommand> history;
    bool spatialDirty = false;
};
