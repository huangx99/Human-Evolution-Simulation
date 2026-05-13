#pragma once

#include "core/types/types.h"
#include "sim/command/command.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

struct ReplayLog
{
    u64 seed = 0;
    i32 worldWidth = 0;
    i32 worldHeight = 0;
    std::vector<QueuedCommand> entries;

    // Record initial world setup
    struct InitialSetup
    {
        std::vector<std::pair<i32, i32>> firePositions;
        std::vector<f32> fireIntensities;
        std::vector<std::pair<i32, i32>> agentPositions;
    };

    InitialSetup setup;

    void Record(Tick tick, std::unique_ptr<CommandBase> cmd)
    {
        entries.push_back({tick, std::move(cmd)});
    }

    std::string Serialize() const
    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(2);

        os << "seed=" << seed << "\n";
        os << "world=" << worldWidth << "x" << worldHeight << "\n";

        os << "initial_fires=" << setup.firePositions.size() << "\n";
        for (size_t i = 0; i < setup.firePositions.size(); i++)
        {
            os << "  fire " << setup.firePositions[i].first
               << "," << setup.firePositions[i].second
               << " intensity=" << setup.fireIntensities[i] << "\n";
        }

        os << "initial_agents=" << setup.agentPositions.size() << "\n";
        for (const auto& pos : setup.agentPositions)
        {
            os << "  agent " << pos.first << "," << pos.second << "\n";
        }

        os << "commands=" << entries.size() << "\n";
        for (const auto& qc : entries)
        {
            os << "  tick=" << qc.tick
               << " kind=" << static_cast<u16>(qc.command->GetKind());
            os << "\n";
        }

        return os.str();
    }
};
