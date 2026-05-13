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
        for (const auto& qc : entries)
        {
            os << "  tick=" << qc.tick
               << " kind=" << static_cast<u16>(GetCommandKind(qc.command));

            // Extract common fields via visit
            std::visit([&](const auto& inner) {
                std::visit([&](const auto& typed) {
                    using T = std::decay_t<decltype(typed)>;
                    if constexpr (std::is_same_v<T, MoveAgentCommand>)
                        os << " entity=" << typed.id << " pos=" << typed.x << "," << typed.y;
                    else if constexpr (std::is_same_v<T, SetAgentActionCommand>)
                        os << " entity=" << typed.id << " action=" << static_cast<int>(typed.action);
                    else if constexpr (std::is_same_v<T, DamageAgentCommand>)
                        os << " entity=" << typed.id << " amount=" << typed.amount;
                    else if constexpr (std::is_same_v<T, FeedAgentCommand>)
                        os << " entity=" << typed.id << " amount=" << typed.amount;
                    else if constexpr (std::is_same_v<T, ModifyHungerCommand>)
                        os << " entity=" << typed.id << " delta=" << typed.delta;
                    else if constexpr (std::is_same_v<T, IgniteFireCommand>)
                        os << " pos=" << typed.x << "," << typed.y << " intensity=" << typed.intensity;
                    else if constexpr (std::is_same_v<T, ExtinguishFireCommand>)
                        os << " pos=" << typed.x << "," << typed.y;
                    else if constexpr (std::is_same_v<T, EmitSmellCommand>)
                        os << " pos=" << typed.x << "," << typed.y << " amount=" << typed.amount;
                    else if constexpr (std::is_same_v<T, SetDangerCommand>)
                        os << " pos=" << typed.x << "," << typed.y << " value=" << typed.value;
                    else if constexpr (std::is_same_v<T, AddEntityStateCommand>)
                        os << " entity=" << typed.id << " state=0x" << std::hex << typed.state << std::dec;
                    else if constexpr (std::is_same_v<T, RemoveEntityStateCommand>)
                        os << " entity=" << typed.id << " state=0x" << std::hex << typed.state << std::dec;
                    else if constexpr (std::is_same_v<T, AddEntityCapabilityCommand>)
                        os << " entity=" << typed.id << " cap=0x" << std::hex << typed.capability << std::dec;
                    else if constexpr (std::is_same_v<T, RemoveEntityCapabilityCommand>)
                        os << " entity=" << typed.id << " cap=0x" << std::hex << typed.capability << std::dec;
                    else if constexpr (std::is_same_v<T, ModifyFieldValueCommand>)
                        os << " pos=" << typed.x << "," << typed.y
                           << " field=" << typed.field.value
                           << " mode=" << typed.mode << " value=" << typed.value;
                    else if constexpr (std::is_same_v<T, EmitSmokeCommand>)
                        os << " pos=" << typed.x << "," << typed.y << " amount=" << typed.amount;
                }, inner);
            }, qc.command);

            os << "\n";
        }

        return os.str();
    }
};
