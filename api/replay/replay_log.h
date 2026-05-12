#pragma once

#include "core/types/types.h"
#include "sim/command/command.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

struct ReplayEntry
{
    Tick tick;
    Command command;
};

struct ReplayLog
{
    u64 seed = 0;
    i32 worldWidth = 0;
    i32 worldHeight = 0;
    std::vector<ReplayEntry> entries;

    // Record initial world setup
    struct InitialSetup
    {
        std::vector<std::pair<i32, i32>> firePositions;
        std::vector<f32> fireIntensities;
        std::vector<std::pair<i32, i32>> agentPositions;
    };

    InitialSetup setup;

    void Record(Tick tick, const Command& cmd)
    {
        entries.push_back({tick, cmd});
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
        for (const auto& entry : entries)
        {
            os << "  tick=" << entry.tick
               << " cmd=" << static_cast<i32>(entry.command.type)
               << " entity=" << entry.command.entityId
               << " pos=" << entry.command.x << "," << entry.command.y
               << " target=" << entry.command.targetX << "," << entry.command.targetY
               << " value=" << entry.command.value << "\n";
        }

        return os.str();
    }
};
