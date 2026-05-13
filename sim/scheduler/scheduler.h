#pragma once

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/runtime/simulation_hash.h"
#include "sim/scheduler/phase.h"
#include <vector>
#include <memory>
#include <array>
#include <iostream>
#include <cassert>
#include <cstring>

class Scheduler
{
public:
    void AddSystem(SimPhase phase, std::unique_ptr<ISystem> system)
    {
        const auto& desc = system->Descriptor();

        // Validate phase consistency: descriptor phase must match registration phase
        assert(desc.phase == phase && "SystemDescriptor phase mismatch with registration phase");

        // Log registration
        std::cout << "[Scheduler] Registered: " << desc.name
                  << " (phase=" << PhaseName(phase)
                  << ", reads=" << desc.readCount
                  << ", writes=" << desc.writeCount
                  << ", deps=" << desc.dependsOnCount
                  << ", det=" << (desc.deterministic ? "Y" : "N")
                  << ", par=" << (desc.parallelSafe ? "Y" : "N")
                  << ")" << std::endl;

        systems[static_cast<size_t>(phase)].push_back(std::move(system));
    }

    void Tick(WorldState& world)
    {
        // Print system table on first tick
        if (!tablePrinted)
        {
            PrintSystemTable();
            tablePrinted = true;
        }

        for (size_t p = 0; p < static_cast<size_t>(SimPhase::Count); p++)
        {
            SimPhase phase = static_cast<SimPhase>(p);

            // Run registered systems for this phase
            for (auto& system : systems[p])
            {
                const auto& desc = system->Descriptor();
                SystemContext ctx(world, &desc);
                system->Update(ctx);
            }

            // Phase-specific built-in behavior
            switch (phase)
            {
            case SimPhase::BeginTick:
                if (!world.spatial.IsInitialized())
                    world.RebuildSpatial();
                world.Cognitive().ClearFrame();
                break;
            case SimPhase::CommandApply:
                world.commands.Apply(world);
                break;
            case SimPhase::EventResolve:
                world.events.Dispatch();
                break;
            case SimPhase::FieldSwap:
                world.Fields().SwapAll();
                break;
            case SimPhase::Snapshot:
                world.lastTickHash = ComputeWorldHash(world, HashTier::Full);
                break;
            case SimPhase::EndTick:
                world.Sim().clock.Step();
                break;
            default:
                break;
            }
        }
    }

    // Count total registered systems
    size_t SystemCount() const
    {
        size_t count = 0;
        for (auto& vec : systems)
            count += vec.size();
        return count;
    }

    // Store arbitrary user data that must outlive the scheduler's systems.
    // Used by test helpers to keep policy objects alive.
    void SetUserData(std::shared_ptr<void> data)
    {
        userData_ = std::move(data);
    }

private:
    std::array<std::vector<std::unique_ptr<ISystem>>,
               static_cast<size_t>(SimPhase::Count)> systems;
    bool tablePrinted = false;
    std::shared_ptr<void> userData_;

    static const char* PhaseName(SimPhase phase)
    {
        switch (phase)
        {
        case SimPhase::BeginTick:     return "BeginTick";
        case SimPhase::Environment:   return "Environment";
        case SimPhase::Reaction:      return "Reaction";
        case SimPhase::Propagation:   return "Propagation";
        case SimPhase::Perception:    return "Perception";
        case SimPhase::Decision:      return "Decision";
        case SimPhase::Action:        return "Action";
        case SimPhase::CommandApply:  return "CommandApply";
        case SimPhase::EventResolve:  return "EventResolve";
        case SimPhase::FieldSwap:     return "FieldSwap";
        case SimPhase::Analysis:      return "Analysis";
        case SimPhase::History:       return "History";
        case SimPhase::Snapshot:      return "Snapshot";
        case SimPhase::EndTick:       return "EndTick";
        default:                      return "Unknown";
        }
    }

    void PrintSystemTable()
    {
        std::cout << "\n=== System Table ===" << std::endl;
        std::cout << "Phase            | System                  | R  | W  | Dep | Det | Par" << std::endl;
        std::cout << "-----------------|-------------------------|----|----|-----|-----|----" << std::endl;

        for (size_t p = 0; p < static_cast<size_t>(SimPhase::Count); p++)
        {
            for (auto& system : systems[p])
            {
                const auto& desc = system->Descriptor();
                std::cout << PhaseName(desc.phase);
                // Pad phase name to 17 chars
                size_t phaseLen = strlen(PhaseName(desc.phase));
                for (size_t i = phaseLen; i < 17; i++) std::cout << ' ';

                std::cout << "| ";
                std::cout << desc.name;
                // Pad system name to 24 chars
                size_t nameLen = strlen(desc.name);
                for (size_t i = nameLen; i < 24; i++) std::cout << ' ';

                std::cout << "| " << desc.readCount;
                if (desc.readCount < 10) std::cout << ' ';
                std::cout << " | " << desc.writeCount;
                if (desc.writeCount < 10) std::cout << ' ';
                std::cout << " | " << desc.dependsOnCount;
                if (desc.dependsOnCount < 10) std::cout << "   ";
                else std::cout << "  ";
                std::cout << "| " << (desc.deterministic ? "Y" : "N") << "   ";
                std::cout << "| " << (desc.parallelSafe ? "Y" : "N");

                std::cout << std::endl;
            }
        }
        std::cout << "==================\n" << std::endl;
    }
};
