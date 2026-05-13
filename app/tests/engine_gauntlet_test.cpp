#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/runtime/simulation_hash.h"
#include "sim/runtime/replay.h"
#include "sim/pattern/pattern_module.h"
#include "sim/pattern/pattern_registry.h"
#include "sim/pattern/pattern_detection_system.h"
#include "sim/pattern/detectors/high_frequency_path_detector.h"
#include "sim/pattern/detectors/stable_field_zone_detector.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "test_framework.h"

static HumanEvolutionRulePack g_rulePack;
static const auto& g_envCtx = g_rulePack.GetHumanEvolutionContext().environment;

// Test 1: RulePack bootstrap — WorldState + Scheduler come up correctly
TEST(engine_gauntlet_rulepack_bootstrap)
{
    WorldState world(16, 16, 42, g_rulePack);

    // Fields registered by RulePack
    ASSERT_NEAR(world.Fields().Read(g_envCtx.fire, 0, 0), 0.0f, 0.001f);
    ASSERT_NEAR(world.Fields().Read(g_envCtx.temperature, 0, 0), 20.0f, 0.001f);

    // Agent module works
    EntityId id = world.SpawnAgent(5, 5);
    ASSERT_TRUE(id > 0);
    ASSERT_EQ(world.Agents().agents.size(), 1u);

    // Pattern module accessible and empty
    ASSERT_EQ(world.Patterns().Count(), 0u);

    // RuleContext stored
    ASSERT_TRUE(world.RuleContext() != nullptr);

    // Scheduler factory produces systems
    auto scheduler = CreateHumanEvolutionScheduler(g_envCtx);
    ASSERT_TRUE(scheduler.SystemCount() > 0);

    return true;
}

// Test 2: Determinism — two identical worlds produce identical hashes for 1000 ticks
TEST(engine_gauntlet_determinism_1000_ticks)
{
    auto makeWorld = [](u64 seed) {
        WorldState world(32, 32, seed, g_rulePack);
        world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
        world.Fields().WriteNext(g_envCtx.fire, 17, 16, 60.0f);
        world.Fields().SwapAll();
        world.SpawnAgent(5, 5);
        world.SpawnAgent(10, 20);
        world.SpawnAgent(25, 10);
        return world;
    };

    WorldState a = makeWorld(42);
    WorldState b = makeWorld(42);
    auto sa = CreateHumanEvolutionScheduler(g_envCtx);
    auto sb = CreateHumanEvolutionScheduler(g_envCtx);

    for (i32 i = 0; i < 1000; i++)
    {
        sa.Tick(a);
        sb.Tick(b);
        ASSERT_EQ(ComputeWorldHash(a, HashTier::Full),
                  ComputeWorldHash(b, HashTier::Full));
    }

    ASSERT_EQ(a.lastTickHash, b.lastTickHash);
    ASSERT_EQ(a.Patterns().Count(), b.Patterns().Count());

    return true;
}

// Test 3: Replay roundtrip — two replays from same snapshot produce identical hash
TEST(engine_gauntlet_replay_roundtrip)
{
    WorldState world(16, 16, 42, g_rulePack);
    world.Fields().WriteNext(g_envCtx.fire, 8, 8, 80.0f);
    world.Fields().SwapAll();
    world.SpawnAgent(4, 4);
    world.SpawnAgent(10, 10);

    auto scheduler = CreateHumanEvolutionScheduler(g_envCtx);

    // Run to tick 50, generating command history
    for (i32 i = 0; i < 50; i++)
        scheduler.Tick(world);

    // Save snapshot and commands
    auto saved = SaveWorldState(world);
    auto& allCommands = world.commands.GetHistory();
    std::vector<QueuedCommand> replayCmds(allCommands.begin(), allCommands.end());

    // Replay 1: restore + apply commands
    WorldState replay1(16, 16, 42, g_rulePack);
    RestoreWorld(replay1, saved);
    ReplayEngine::Replay(replay1, replayCmds, 0, 50);

    // Replay 2: same restore + same commands
    WorldState replay2(16, 16, 42, g_rulePack);
    RestoreWorld(replay2, saved);
    ReplayEngine::Replay(replay2, replayCmds, 0, 50);

    // Both replays must produce identical hash
    ASSERT_EQ(ComputeWorldHash(replay1, HashTier::Full),
              ComputeWorldHash(replay2, HashTier::Full));

    return true;
}

// Test 4: Pattern is derived data — enabling/disabling pattern doesn't change world hash
TEST(engine_gauntlet_pattern_is_derived_data)
{
    auto makeWorld = [](u64 seed) {
        WorldState world(32, 32, seed, g_rulePack);
        world.Fields().WriteNext(g_envCtx.fire, 16, 16, 80.0f);
        world.Fields().WriteNext(g_envCtx.fire, 17, 16, 60.0f);
        world.Fields().WriteNext(g_envCtx.fire, 8, 8, 60.0f);
        world.Fields().SwapAll();
        world.SpawnAgent(5, 5);
        world.SpawnAgent(10, 20);
        world.SpawnAgent(25, 10);
        return world;
    };

    WorldState withPattern = makeWorld(42);
    WorldState withoutPattern = makeWorld(42);

    // Both schedulers use identical base systems; only difference is pattern detection
    auto schedulerA = CreateHumanEvolutionScheduler(g_envCtx);
    // Add PatternDetectionSystem to schedulerA
    PatternRegistry::Instance().Register(
        PatternKey("human_evolution.stable_fire_zone"), "stable_fire_zone");
    {
        auto patternSys = std::make_unique<PatternDetectionSystem>();
        patternSys->AddDetector(std::make_unique<HighFrequencyPathDetector>(10));
        FieldWatchSpec fireWatch;
        fireWatch.patternKey = PatternKey("human_evolution.stable_fire_zone");
        fireWatch.field = g_envCtx.fire;
        fireWatch.threshold = 0.5f;
        fireWatch.minDuration = 50;
        patternSys->AddDetector(std::make_unique<StableFieldZoneDetector>(
            std::vector<FieldWatchSpec>{fireWatch}));
        schedulerA.AddSystem(SimPhase::Analysis, std::move(patternSys));
    }
    auto schedulerB = CreateHumanEvolutionSchedulerWithoutPattern(g_envCtx);

    for (i32 i = 0; i < 500; i++)
    {
        schedulerA.Tick(withPattern);
        schedulerB.Tick(withoutPattern);
    }

    // World hashes must be identical — Pattern is a pure observer
    ASSERT_EQ(ComputeWorldHash(withPattern, HashTier::Full),
              ComputeWorldHash(withoutPattern, HashTier::Full));

    // Pattern-enabled world should have generated patterns
    ASSERT_TRUE(withPattern.Patterns().Count() > 0);

    // Pattern-disabled world should have none
    ASSERT_EQ(withoutPattern.Patterns().Count(), 0u);

    return true;
}

// Test 5: Phase ordering — Analysis reads post-swap state
TEST(engine_gauntlet_phase_ordering)
{
    HumanEvolutionRulePack rp;
    // Register fields via a temporary WorldState so rp's FieldIndex values are valid
    { WorldState tmp(16, 16, 42, rp); }
    const auto& ec = rp.GetHumanEvolutionContext().environment;

    PatternRuntimeConfig config;
    config.observationWindowTicks = 10;

    // Register test pattern type
    PatternKey fireKey("test.gauntlet_fire");
    PatternRegistry::Instance().Register(fireKey, "test_gauntlet_fire");

    FieldWatchSpec fireWatch;
    fireWatch.patternKey = fireKey;
    fireWatch.field = ec.fire;
    fireWatch.threshold = 0.3f;
    fireWatch.minDuration = 5;

    // Part A: fire in NEXT buffer → swap → Analysis should see it
    {
        WorldState w(16, 16, 42, rp);
        auto& fm = w.Fields();
        fm.FillBoth(ec.fire, 0.0f);

        Scheduler sched;
        auto pSys = std::make_unique<PatternDetectionSystem>(config);
        pSys->AddDetector(std::make_unique<StableFieldZoneDetector>(
            std::vector<FieldWatchSpec>{fireWatch}));
        sched.AddSystem(SimPhase::Analysis, std::move(pSys));

        for (i32 i = 0; i < 20; i++)
        {
            fm.WriteNext(ec.fire, 8, 8, 1.0f);
            sched.Tick(w);
        }

        auto zones = w.Patterns().FindByType(fireKey);
        ASSERT_TRUE(!zones.empty());
    }

    // Part B: fire in CURRENT buffer only (not written to next) → swap zeros it → no detection
    {
        WorldState w(16, 16, 42, rp);
        auto& fm = w.Fields();
        fm.FillBoth(ec.fire, 0.0f);

        // Put fire in current only, then swap will zero it
        fm.WriteNext(ec.fire, 8, 8, 1.0f);
        fm.SwapField(ec.fire);
        // Now current(8,8)=1.0, next(8,8)=0.0
        // After FieldSwap: current becomes 0.0

        Scheduler sched;
        auto pSys = std::make_unique<PatternDetectionSystem>(config);
        pSys->AddDetector(std::make_unique<StableFieldZoneDetector>(
            std::vector<FieldWatchSpec>{fireWatch}));
        sched.AddSystem(SimPhase::Analysis, std::move(pSys));

        for (i32 i = 0; i < 20; i++)
            sched.Tick(w);

        // Pattern should NOT be detected because fire gets zeroed by swap
        auto zones = w.Patterns().FindByType(fireKey);
        ASSERT_TRUE(zones.empty());
    }

    return true;
}
