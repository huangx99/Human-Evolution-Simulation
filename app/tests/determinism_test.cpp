#include "test_framework.h"
#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/scheduler/phase.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/environment/climate_system.h"
#include "rules/human_evolution/environment/fire_system.h"
#include "rules/human_evolution/environment/smell_system.h"
#include "sim/system/agent_perception_system.h"
#include "sim/system/agent_decision_system.h"
#include "sim/system/agent_action_system.h"
#include "sim/runtime/simulation_hash.h"
#include "sim/runtime/replay.h"
#include "api/snapshot/world_snapshot.h"

static HumanEvolutionRulePack g_rulePack;

static WorldSnapshot RunSimulation(u64 seed, i32 ticks)
{
    WorldState world(32, 32, seed);
    world.Init(g_rulePack);

    world.Env().fire.WriteNext(16, 16) = 80.0f;
    world.Env().fire.WriteNext(17, 16) = 60.0f;
    world.Env().fire.WriteNext(8, 8) = 60.0f;
    world.Env().fire.Swap();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(10, 20);
    world.SpawnAgent(25, 10);

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Environment,  std::make_unique<ClimateSystem>());
    scheduler.AddSystem(SimPhase::Propagation,  std::make_unique<FireSystem>());
    scheduler.AddSystem(SimPhase::Propagation,  std::make_unique<SmellSystem>());
    scheduler.AddSystem(SimPhase::Perception,   std::make_unique<AgentPerceptionSystem>());
    scheduler.AddSystem(SimPhase::Decision,     std::make_unique<AgentDecisionSystem>());
    scheduler.AddSystem(SimPhase::Action,       std::make_unique<AgentActionSystem>());

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
    ASSERT_EQ(a.Serialize(), b.Serialize());
    return true;
}

TEST(determinism_different_seed)
{
    WorldSnapshot a = RunSimulation(42, 500);
    WorldSnapshot b = RunSimulation(99, 500);
    ASSERT_TRUE(a.Serialize() != b.Serialize());
    return true;
}

TEST(determinism_long_run)
{
    WorldSnapshot a = RunSimulation(42, 2000);
    WorldSnapshot b = RunSimulation(42, 2000);
    ASSERT_EQ(a.Serialize(), b.Serialize());
    return true;
}

TEST(determinism_hash_proof)
{
    auto makeWorld = [](u64 seed) {
        WorldState world(16, 16, seed);
        world.Init(g_rulePack);
        world.Env().fire.WriteNext(8, 8) = 50.0f;
        world.Env().fire.Swap();
        world.SpawnAgent(4, 4);
        return world;
    };

    auto makeScheduler = []() {
        Scheduler scheduler;
        scheduler.AddSystem(SimPhase::Environment, std::make_unique<ClimateSystem>());
        scheduler.AddSystem(SimPhase::Propagation, std::make_unique<FireSystem>());
        scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SmellSystem>());
        scheduler.AddSystem(SimPhase::Perception,  std::make_unique<AgentPerceptionSystem>());
        scheduler.AddSystem(SimPhase::Decision,    std::make_unique<AgentDecisionSystem>());
        scheduler.AddSystem(SimPhase::Action,      std::make_unique<AgentActionSystem>());
        return scheduler;
    };

    WorldState a = makeWorld(42);
    WorldState b = makeWorld(42);
    auto sa = makeScheduler();
    auto sb = makeScheduler();

    for (i32 i = 0; i < 1000; i++)
    {
        sa.Tick(a);
        sb.Tick(b);
        ASSERT_EQ(a.lastTickHash, b.lastTickHash);
    }

    // Different seed → different hash
    WorldState c = makeWorld(99);
    auto sc = makeScheduler();
    sc.Tick(c);
    ASSERT_TRUE(a.lastTickHash != c.lastTickHash);

    return true;
}

TEST(minimal_replay)
{
    // Verify replay determinism: re-applying the same commands onto a restored
    // state produces the same result as the first replay.
    // Note: replay is command re-apply, NOT re-simulation. Systems that modify
    // fields directly (Climate, Fire, Smell) are not replayed.

    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    world.Env().fire.WriteNext(8, 8) = 80.0f;
    world.Env().fire.WriteNext(9, 8) = 60.0f;
    world.Env().fire.Swap();
    world.SpawnAgent(4, 4);
    world.SpawnAgent(10, 10);

    // Manually push some commands to generate history
    world.commands.Push(1, MoveAgentCommand{1, 5, 5});
    world.commands.Push(1, IgniteFireCommand{12, 12, 90.0f});
    world.commands.Push(2, DamageAgentCommand{2, 10.0f});
    world.commands.Push(2, EmitSmellCommand{5, 5, 50.0f});
    world.commands.Push(3, MoveAgentCommand{2, 11, 11});

    // Apply commands (moves them to history)
    world.commands.Apply(world);

    // Save state after applying commands
    auto saved = SaveWorldState(world);
    auto historySize = world.commands.GetHistory().size();

    // Continue applying more commands
    world.commands.Push(4, FeedAgentCommand{1, 20.0f});
    world.commands.Push(4, ModifyHungerCommand{2, -15.0f});
    world.commands.Push(5, SetDangerCommand{8, 8, 75.0f});
    world.commands.Push(5, AddEntityStateCommand{1, 0x01});
    world.commands.Apply(world);

    // Extract commands from tick 4+
    auto& allCommands = world.commands.GetHistory();
    std::vector<QueuedCommand> replayCmds(
        allCommands.begin() + historySize, allCommands.end());

    // First replay: restore + apply commands
    WorldState replay1(16, 16, 42);
    replay1.Init(g_rulePack);
    RestoreWorld(replay1, saved);
    ReplayEngine::Replay(replay1, replayCmds, 4, 6);

    // Second replay: same restore + same commands → must be identical
    WorldState replay2(16, 16, 42);
    replay2.Init(g_rulePack);
    RestoreWorld(replay2, saved);
    ReplayEngine::Replay(replay2, replayCmds, 4, 6);

    // Replay determinism: both replays produce the same hash
    ASSERT_EQ(ComputeWorldHash(replay1, HashTier::Full),
              ComputeWorldHash(replay2, HashTier::Full));

    // Verify commands actually took effect (sanity check)
    const Agent* a1 = replay1.Agents().Find(1);
    ASSERT_TRUE(a1 != nullptr);
    ASSERT_EQ(a1->position.x, 5);
    ASSERT_EQ(a1->position.y, 5);
    ASSERT_TRUE(a1->hunger < 100.0f); // FeedAgentCommand reduced hunger

    return true;
}
