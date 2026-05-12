#pragma once

#include "sim/system/i_system.h"
#include "sim/scheduler/phase.h"
#include <vector>
#include <memory>
#include <array>

class Scheduler
{
public:
    void AddSystem(SimPhase phase, std::unique_ptr<ISystem> system)
    {
        systems[static_cast<size_t>(phase)].push_back(std::move(system));
    }

    void Tick(WorldState& world)
    {
        for (size_t p = 0; p < static_cast<size_t>(SimPhase::Count); p++)
        {
            SimPhase phase = static_cast<SimPhase>(p);

            // Run registered systems for this phase
            for (auto& system : systems[p])
            {
                system->Update(world);
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
            case SimPhase::EndTick:
                world.Env().SwapAll();
                world.Info().SwapAll();
                world.Sim().clock.Step();
                break;
            default:
                break;
            }
        }
    }

private:
    std::array<std::vector<std::unique_ptr<ISystem>>,
               static_cast<size_t>(SimPhase::Count)> systems;
};
