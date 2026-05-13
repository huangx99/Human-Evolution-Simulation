#pragma once

#include "sim/command/command.h"
#include <vector>

struct WorldState;

class CommandBuffer
{
public:
    // Submit a command (takes ownership)
    void Submit(Tick tick, std::unique_ptr<CommandBase> cmd)
    {
        pending.push_back({tick, std::move(cmd)});
    }

    // Convenience: submit a typed command (auto-wraps)
    template<typename T, std::enable_if_t<std::is_base_of_v<CommandBase, std::decay_t<T>>, int> = 0>
    void Submit(Tick tick, T&& cmd)
    {
        pending.push_back({tick, std::make_unique<std::decay_t<T>>(std::forward<T>(cmd))});
    }

    // Push a command (for replay — takes ownership)
    void Push(Tick tick, std::unique_ptr<CommandBase> cmd)
    {
        pending.push_back({tick, std::move(cmd)});
    }

    // Convenience: push a typed command (creates via clone)
    template<typename T>
    void Push(Tick tick, const T& cmd)
    {
        static_assert(std::is_base_of_v<CommandBase, T>, "T must inherit from CommandBase");
        pending.push_back({tick, std::make_unique<T>(cmd)});
    }

    void Apply(WorldState& world)
    {
        for (auto& qc : pending)
        {
            if (qc.command)
                qc.command->Execute(world);
        }

        history.insert(history.end(),
            std::make_move_iterator(pending.begin()),
            std::make_move_iterator(pending.end()));
        pending.clear();
    }

    bool IsSpatialDirty() const { return spatialDirty; }
    void ClearSpatialDirty() { spatialDirty = false; }
    void SetSpatialDirty() { spatialDirty = true; }

    const std::vector<QueuedCommand>& GetHistory() const { return history; }
    size_t HistoryCount() const { return history.size(); }
    void ClearHistory() { history.clear(); }

private:
    std::vector<QueuedCommand> pending;
    std::vector<QueuedCommand> history;
    bool spatialDirty = false;
};
