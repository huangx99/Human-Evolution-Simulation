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
        // Execute all systems in phase order
        for (size_t p = 0; p < static_cast<size_t>(SimPhase::Count); p++)
        {
            for (auto& system : systems[p])
            {
                system->Update(world);
            }
        }

        // CommandApply phase: apply pending commands
        world.commands.Apply(world);

        // EndTick phase: swap field buffers and step clock
        world.Env().SwapAll();
        world.Info().SwapAll();
        world.Sim().clock.Step();
    }

private:
    std::array<std::vector<std::unique_ptr<ISystem>>,
               static_cast<size_t>(SimPhase::Count)> systems;
};
