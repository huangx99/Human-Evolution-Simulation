#pragma once

#include "sim/command/command.h"
#include <vector>

struct WorldState;

class CommandBuffer
{
public:
    void Submit(const Command& cmd)
    {
        pending.push_back(cmd);
    }

    void Push(const Command& cmd)
    {
        pending.push_back(cmd);
    }

    void Apply(WorldState& world);

    bool IsSpatialDirty() const { return spatialDirty; }
    void ClearSpatialDirty() { spatialDirty = false; }

    const std::vector<Command>& GetHistory() const { return history; }
    void ClearHistory() { history.clear(); }

private:
    std::vector<Command> pending;
    std::vector<Command> history;
    bool spatialDirty = false;

    void Execute(WorldState& world, const Command& cmd);
};
