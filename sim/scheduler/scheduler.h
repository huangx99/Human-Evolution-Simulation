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
            for (auto& system : systems[p])
            {
                system->Update(world);
            }
        }
        world.sim.clock.Step();
    }

private:
    std::array<std::vector<std::unique_ptr<ISystem>>,
               static_cast<size_t>(SimPhase::Count)> systems;
};
