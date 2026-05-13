#pragma once

#include "sim/command/command.h"
#include <vector>
#include <type_traits>

struct WorldState;

class CommandBuffer
{
public:
    // Submit a typed command. Template constrains T to valid command types.
    // Auto-wraps into Command variant via MakeCommand.
    // Disabled when T is already Command to avoid double-wrapping.
    template<typename T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, Command>, int> = 0>
    void Submit(Tick tick, T&& cmd)
    {
        static_assert(IsCommandType<std::decay_t<T>>::value,
                      "Unsupported command type");
        pending.push_back({tick, MakeCommand(std::forward<T>(cmd))});
    }

    // Push a typed command (wraps via MakeCommand)
    template<typename T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, Command>, int> = 0>
    void Push(Tick tick, T&& cmd)
    {
        Submit(tick, std::forward<T>(cmd));
    }

    // Push an already-wrapped Command (for replay)
    void Push(Tick tick, const Command& cmd)
    {
        pending.push_back({tick, cmd});
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
