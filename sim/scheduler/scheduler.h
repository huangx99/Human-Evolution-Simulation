#pragma once

#include "sim/system/i_system.h"
#include <vector>
#include <memory>

class Scheduler
{
public:
    void AddSystem(std::unique_ptr<ISystem> system)
    {
        systems.push_back(std::move(system));
    }

    void Tick(WorldState& world)
    {
        for (auto& system : systems)
        {
            system->Update(world);
        }
        world.clock.Step();
    }

private:
    std::vector<std::unique_ptr<ISystem>> systems;
};
