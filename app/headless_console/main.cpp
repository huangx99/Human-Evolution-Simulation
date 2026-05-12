#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/system/climate_system.h"
#include "sim/system/fire_system.h"
#include "sim/system/smell_system.h"
#include "sim/system/agent_system.h"
#include "api/snapshot/world_snapshot.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <string>

struct RunConfig
{
    u64 seed = 42;
    i32 ticks = 300;
    std::string dumpPath;
    bool quiet = false;
};

RunConfig ParseArgs(int argc, char* argv[])
{
    RunConfig cfg;
    for (int i = 1; i < argc; i++)
    {
        if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
        {
            cfg.seed = std::stoull(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--ticks") == 0 && i + 1 < argc)
        {
            cfg.ticks = std::stoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--dump") == 0 && i + 1 < argc)
        {
            cfg.dumpPath = argv[++i];
        }
        else if (std::strcmp(argv[i], "--quiet") == 0)
        {
            cfg.quiet = true;
        }
    }
    return cfg;
}

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
    std::cout << "Fire cells: " << fireCount << " (max: "
              << std::fixed << std::setprecision(1) << maxFire << ")" << std::endl;

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

        std::cout << "Agent_" << agent.id
                  << " pos=(" << agent.position.x << "," << agent.position.y << ")"
                  << " hunger=" << std::fixed << std::setprecision(0) << agent.hunger
                  << " health=" << agent.health
                  << " action=" << actionStr << std::endl;
    }

    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    RunConfig cfg = ParseArgs(argc, argv);

    std::cout << "Human Evolution Simulation - Phase 1" << std::endl;
    std::cout << "Seed: " << cfg.seed << "  Ticks: " << cfg.ticks << std::endl;
    std::cout << "====================================" << std::endl << std::endl;

    WorldState world(32, 32, cfg.seed);

    // Ignite fires
    world.fire.At(16, 16) = 80.0f;
    world.fire.At(17, 16) = 60.0f;
    world.fire.At(16, 17) = 60.0f;
    world.fire.At(15, 16) = 40.0f;
    world.fire.At(8, 8) = 60.0f;
    world.fire.At(9, 8) = 40.0f;

    // Spawn agents
    world.SpawnAgent(5, 5);
    world.SpawnAgent(10, 20);
    world.SpawnAgent(25, 10);

    // Register systems in fixed order
    Scheduler scheduler;
    scheduler.AddSystem(std::make_unique<ClimateSystem>());
    scheduler.AddSystem(std::make_unique<FireSystem>());
    scheduler.AddSystem(std::make_unique<SmellSystem>());
    scheduler.AddSystem(std::make_unique<AgentSystem>());

    i32 printInterval = cfg.ticks / 12;
    if (printInterval < 1) printInterval = 1;

    for (i32 i = 0; i < cfg.ticks; i++)
    {
        scheduler.Tick(world);
        if (!cfg.quiet)
        {
            PrintWorldState(world, printInterval);
        }
    }

    // Dump final snapshot
    if (!cfg.dumpPath.empty())
    {
        WorldSnapshot snap = WorldSnapshot::Capture(world);
        std::string data = snap.Serialize();
        std::ofstream ofs(cfg.dumpPath);
        if (ofs.is_open())
        {
            ofs << data;
            ofs.close();
            std::cout << "Snapshot dumped to: " << cfg.dumpPath << std::endl;
        }
        else
        {
            std::cerr << "Failed to open dump file: " << cfg.dumpPath << std::endl;
            return 1;
        }
    }

    std::cout << "Simulation complete." << std::endl;
    return 0;
}
