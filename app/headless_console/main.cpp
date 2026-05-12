#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/system/climate_system.h"
#include "sim/system/fire_system.h"
#include "sim/system/smell_system.h"
#include "sim/system/agent_system.h"
#include <iostream>
#include <iomanip>

void PrintWorldState(const WorldState& world, i32 interval)
{
    if (world.clock.currentTick % interval != 0) return;

    std::cout << "=== Tick: " << world.clock.currentTick << " ===" << std::endl;

    i32 cx = world.width / 2;
    i32 cy = world.height / 2;
    std::cout << "Temperature (center): " << std::fixed << std::setprecision(1)
              << world.temperature.At(cx, cy) << " C" << std::endl;
    std::cout << "Humidity (center): " << world.humidity.At(cx, cy) << "%" << std::endl;
    std::cout << "Wind: (" << std::fixed << std::setprecision(2)
              << world.wind.x << ", " << world.wind.y << ")" << std::endl;

    // Count fire cells and max fire intensity
    i32 fireCount = 0;
    f32 maxFire = 0.0f;
    for (i32 y = 0; y < world.height; y++)
    {
        for (i32 x = 0; x < world.width; x++)
        {
            f32 f = world.fire.At(x, y);
            if (f > 0.0f)
            {
                fireCount++;
                if (f > maxFire) maxFire = f;
            }
        }
    }
    std::cout << "Fire cells: " << fireCount << " (max intensity: "
              << std::fixed << std::setprecision(1) << maxFire << ")" << std::endl;

    // Agent status
    for (const auto& agent : world.agents)
    {
        const char* actionStr = "idle";
        switch (agent.currentAction)
        {
        case AgentAction::MoveToFood: actionStr = "move_to_food"; break;
        case AgentAction::Flee:       actionStr = "flee"; break;
        case AgentAction::Wander:     actionStr = "wander"; break;
        case AgentAction::Idle:       actionStr = "idle"; break;
        }

        std::cout << "Agent_" << agent.id << ":" << std::endl;
        std::cout << "  pos: (" << agent.position.x << ", " << agent.position.y << ")" << std::endl;
        std::cout << "  hunger: " << std::fixed << std::setprecision(0) << agent.hunger << std::endl;
        std::cout << "  health: " << agent.health << std::endl;
        std::cout << "  smell: " << std::fixed << std::setprecision(1) << agent.nearestSmell << std::endl;
        std::cout << "  fire: " << agent.nearestFire << std::endl;
        std::cout << "  action: " << actionStr << std::endl;
    }

    std::cout << std::endl;
}

int main()
{
    std::cout << "Human Evolution Simulation - Phase 1" << std::endl;
    std::cout << "====================================" << std::endl << std::endl;

    WorldState world(32, 32, 42);

    // Ignite fires at two locations
    world.fire.At(16, 16) = 80.0f;
    world.fire.At(17, 16) = 60.0f;
    world.fire.At(16, 17) = 60.0f;
    world.fire.At(15, 16) = 40.0f;

    world.fire.At(8, 8) = 60.0f;
    world.fire.At(9, 8) = 40.0f;

    // Spawn agents at various locations
    world.SpawnAgent(5, 5);
    world.SpawnAgent(10, 20);
    world.SpawnAgent(25, 10);

    // Register systems in fixed order per design doc
    Scheduler scheduler;
    scheduler.AddSystem(std::make_unique<ClimateSystem>());
    scheduler.AddSystem(std::make_unique<FireSystem>());
    scheduler.AddSystem(std::make_unique<SmellSystem>());
    scheduler.AddSystem(std::make_unique<AgentSystem>());

    i32 totalTicks = 300;
    i32 printInterval = 25;

    std::cout << "Starting simulation (" << totalTicks << " ticks)..." << std::endl << std::endl;

    for (i32 i = 0; i < totalTicks; i++)
    {
        scheduler.Tick(world);
        PrintWorldState(world, printInterval);
    }

    std::cout << "Simulation complete." << std::endl;
    return 0;
}
