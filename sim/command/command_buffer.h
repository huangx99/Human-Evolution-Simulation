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

    void Apply(WorldState& world);

    const std::vector<Command>& GetHistory() const { return history; }
    void ClearHistory() { history.clear(); }

private:
    std::vector<Command> pending;
    std::vector<Command> history;

    void Execute(WorldState& world, const Command& cmd);
};
