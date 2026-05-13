#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/runtime/simulation_hash.h"
#include "sim/history/history_module.h"
#include "sim/history/history_registry.h"
#include "sim/history/history_detection_system.h"
#include "sim/pattern/pattern_detection_system.h"
#include "sim/pattern/pattern_registry.h"
#include "sim/pattern/detectors/high_frequency_path_detector.h"
#include "sim/pattern/detectors/stable_field_zone_detector.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/history/first_stable_fire_detector.h"
#include "rules/human_evolution/history/mass_death_detector.h"
#include "test_framework.h"

static HumanEvolutionRulePack g_rulePack;
static const auto& g_envCtx = g_rulePack.GetHumanEvolutionContext().environment;

// Test 1: FirstStableFireUsage fires once from stable_fire_zone pattern
TEST(history_first_stable_fire_usage)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ec = rp.GetHumanEvolutionContext().environment;
    auto& fm = world.Fields();

    // Seed persistent fire at (8,8)
    fm.FillBoth(ec.fire, 0.0f);
    fm.WriteNext(ec.fire, 8, 8, 1.0f);
    fm.SwapField(ec.fire);
    fm.WriteNext(ec.fire, 8, 8, 1.0f);

    // Register pattern type
    PatternKey stableFireKey("human_evolution.stable_fire_zone");
    PatternRegistry::Instance().Register(stableFireKey, "stable_fire_zone");

    // Build scheduler: pattern detection + history detection
    Scheduler scheduler;

    PatternRuntimeConfig patConfig;
    patConfig.observationWindowTicks = 10;
    auto patternSys = std::make_unique<PatternDetectionSystem>(patConfig);
    patternSys->AddDetector(std::make_unique<HighFrequencyPathDetector>(10));
    FieldWatchSpec fireWatch;
    fireWatch.patternKey = stableFireKey;
    fireWatch.field = ec.fire;
    fireWatch.threshold = 0.5f;
    fireWatch.minDuration = 5;
    patternSys->AddDetector(std::make_unique<StableFieldZoneDetector>(
        std::vector<FieldWatchSpec>{fireWatch}));
    scheduler.AddSystem(SimPhase::Analysis, std::move(patternSys));

    HistoryKey firstFireKey("human_evolution.first_stable_fire_usage");
    HistoryRegistry::Instance().Register(firstFireKey, "first_stable_fire_usage");

    auto historySys = std::make_unique<HistoryDetectionSystem>();
    historySys->AddDetector(std::make_unique<FirstStableFireUsageDetector>(
        firstFireKey, stableFireKey, 0.3f, 5));
    scheduler.AddSystem(SimPhase::History, std::move(historySys));

    // Run 100 ticks, re-seeding fire each tick
    for (i32 i = 0; i < 100; i++)
    {
        fm.WriteNext(ec.fire, 8, 8, 1.0f);
        scheduler.Tick(world);
    }

    auto& history = world.History();

    // Should have exactly 1 FirstStableFireUsage event
    auto events = 0;
    for (const auto& e : history.Events())
    {
        if (e.typeKey == firstFireKey)
        {
            events++;
            ASSERT_EQ(e.x, 8);
            ASSERT_EQ(e.y, 8);
            ASSERT_TRUE(!e.sourcePatterns.empty());
        }
    }
    ASSERT_EQ(events, 1);

    return true;
}

// Test 2: MassDeath from multiple agent deaths in short window
TEST(history_mass_death)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);

    // Spawn 5 agents at same position
    for (i32 i = 0; i < 5; i++)
        world.SpawnAgent(8, 8);

    // Register history type
    HistoryKey massDeathKey("human_evolution.mass_death");
    HistoryRegistry::Instance().Register(massDeathKey, "mass_death");

    // Build scheduler with only history detection (no agent action systems)
    Scheduler scheduler;

    MassDeathDetector::Config cfg;
    cfg.eventKey = massDeathKey;
    cfg.windowTicks = 10;
    cfg.threshold = 3;
    cfg.cooldownTicks = 50;

    auto historySys = std::make_unique<HistoryDetectionSystem>();
    historySys->AddDetector(std::make_unique<MassDeathDetector>(std::move(cfg)));
    scheduler.AddSystem(SimPhase::History, std::move(historySys));

    // Tick 1: damage 3 agents to death
    world.commands.Push(1, DamageAgentCommand{1, 200.0f});
    world.commands.Push(1, DamageAgentCommand{2, 200.0f});
    world.commands.Push(1, DamageAgentCommand{3, 200.0f});
    scheduler.Tick(world);

    auto& history = world.History();

    // Should have a MassDeath event
    bool found = false;
    for (const auto& e : history.Events())
    {
        if (e.typeKey == massDeathKey)
        {
            found = true;
            ASSERT_TRUE(e.magnitude >= 3.0f);
            ASSERT_TRUE(e.confidence == 1.0f);
            ASSERT_TRUE(!e.involvedEntities.empty());
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// Test 3: History is derived data — on/off doesn't change FullHash
TEST(history_is_derived_data)
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

    WorldState withHistory = makeWorld(42);
    WorldState withoutHistory = makeWorld(42);

    // Both use same base systems; withHistory also has HistoryDetectionSystem
    auto schedulerA = CreateHumanEvolutionScheduler(g_envCtx);
    // Add history system to A
    HistoryKey massDeathKey("human_evolution.mass_death");
    HistoryRegistry::Instance().Register(massDeathKey, "mass_death");

    MassDeathDetector::Config cfg;
    cfg.eventKey = massDeathKey;
    cfg.windowTicks = 10;
    cfg.threshold = 3;
    cfg.cooldownTicks = 50;
    auto histSys = std::make_unique<HistoryDetectionSystem>();
    histSys->AddDetector(std::make_unique<MassDeathDetector>(std::move(cfg)));
    schedulerA.AddSystem(SimPhase::History, std::move(histSys));

    auto schedulerB = CreateHumanEvolutionSchedulerWithoutPattern(g_envCtx);

    for (i32 i = 0; i < 300; i++)
    {
        schedulerA.Tick(withHistory);
        schedulerB.Tick(withoutHistory);
    }

    // FullHash must be identical — History is derived data
    ASSERT_EQ(ComputeWorldHash(withHistory, HashTier::Full),
              ComputeWorldHash(withoutHistory, HashTier::Full));

    return true;
}

// Test 4: History event references a pattern from PatternModule
TEST(history_after_pattern)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ec = rp.GetHumanEvolutionContext().environment;
    auto& fm = world.Fields();

    // Seed persistent fire
    fm.FillBoth(ec.fire, 0.0f);
    fm.WriteNext(ec.fire, 8, 8, 1.0f);
    fm.SwapField(ec.fire);
    fm.WriteNext(ec.fire, 8, 8, 1.0f);

    PatternKey stableFireKey("human_evolution.stable_fire_zone");
    PatternRegistry::Instance().Register(stableFireKey, "stable_fire_zone");
    HistoryKey firstFireKey("human_evolution.first_stable_fire_usage");
    HistoryRegistry::Instance().Register(firstFireKey, "first_stable_fire_usage");

    Scheduler scheduler;

    // Pattern system
    PatternRuntimeConfig patConfig;
    patConfig.observationWindowTicks = 10;
    auto patternSys = std::make_unique<PatternDetectionSystem>(patConfig);
    patternSys->AddDetector(std::make_unique<HighFrequencyPathDetector>(10));
    FieldWatchSpec fireWatch;
    fireWatch.patternKey = stableFireKey;
    fireWatch.field = ec.fire;
    fireWatch.threshold = 0.5f;
    fireWatch.minDuration = 5;
    patternSys->AddDetector(std::make_unique<StableFieldZoneDetector>(
        std::vector<FieldWatchSpec>{fireWatch}));
    scheduler.AddSystem(SimPhase::Analysis, std::move(patternSys));

    // History system
    auto historySys = std::make_unique<HistoryDetectionSystem>();
    historySys->AddDetector(std::make_unique<FirstStableFireUsageDetector>(
        firstFireKey, stableFireKey, 0.3f, 5));
    scheduler.AddSystem(SimPhase::History, std::move(historySys));

    for (i32 i = 0; i < 100; i++)
    {
        fm.WriteNext(ec.fire, 8, 8, 1.0f);
        scheduler.Tick(world);
    }

    // Find the history event
    const HistoryEvent* found = nullptr;
    for (const auto& e : world.History().Events())
    {
        if (e.typeKey == firstFireKey)
        {
            found = &e;
            break;
        }
    }
    ASSERT_TRUE(found != nullptr);
    ASSERT_TRUE(!found->sourcePatterns.empty());

    // Verify the referenced pattern exists in PatternModule
    u64 patternId = found->sourcePatterns[0];
    bool patternExists = false;
    for (const auto& p : world.Patterns().All())
    {
        if (p.id == patternId)
        {
            patternExists = true;
            ASSERT_EQ(p.typeKey, stableFireKey);
            break;
        }
    }
    ASSERT_TRUE(patternExists);

    return true;
}
