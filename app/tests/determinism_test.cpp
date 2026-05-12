#include "test_framework.h"
#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/system/climate_system.h"
#include "sim/system/fire_system.h"
#include "sim/system/smell_system.h"
#include "sim/system/agent_system.h"
#include "api/snapshot/world_snapshot.h"

static WorldSnapshot RunSimulation(u64 seed, i32 ticks)
{
    WorldState world(32, 32, seed);

    world.fire.At(16, 16) = 80.0f;
    world.fire.At(17, 16) = 60.0f;
    world.fire.At(8, 8) = 60.0f;

    world.SpawnAgent(5, 5);
    world.SpawnAgent(10, 20);
    world.SpawnAgent(25, 10);

    Scheduler scheduler;
    scheduler.AddSystem(std::make_unique<ClimateSystem>());
    scheduler.AddSystem(std::make_unique<FireSystem>());
    scheduler.AddSystem(std::make_unique<SmellSystem>());
    scheduler.AddSystem(std::make_unique<AgentSystem>());

    for (i32 i = 0; i < ticks; i++)
    {
        scheduler.Tick(world);
    }

    return WorldSnapshot::Capture(world);
}

TEST(determinism_same_seed)
{
    WorldSnapshot a = RunSimulation(42, 500);
    WorldSnapshot b = RunSimulation(42, 500);

    std::string sa = a.Serialize();
    std::string sb = b.Serialize();

    ASSERT_EQ(sa, sb);
    return true;
}

TEST(determinism_different_seed)
{
    WorldSnapshot a = RunSimulation(42, 500);
    WorldSnapshot b = RunSimulation(99, 500);

    std::string sa = a.Serialize();
    std::string sb = b.Serialize();

    ASSERT_TRUE(sa != sb);
    return true;
}

TEST(determinism_long_run)
{
    WorldSnapshot a = RunSimulation(42, 2000);
    WorldSnapshot b = RunSimulation(42, 2000);

    ASSERT_EQ(a.Serialize(), b.Serialize());
    return true;
}
