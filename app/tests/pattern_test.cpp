#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/pattern/pattern_module.h"
#include "sim/pattern/pattern_registry.h"
#include "sim/pattern/pattern_detection_system.h"
#include "sim/pattern/detectors/high_frequency_path_detector.h"
#include "sim/pattern/detectors/stable_field_zone_detector.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "test_framework.h"

// Test: PatternRegistry basic registration
TEST(pattern_registry_basic)
{
    auto& reg = PatternRegistry::Instance();

    // Core types should be registered (by WorldState constructor in earlier tests)
    PatternTypeId pathId = reg.FindByKey(PatternTypes::HighFrequencyPath());
    ASSERT_TRUE(pathId.index != 0);

    PatternTypeId stableId = reg.FindByKey(PatternTypes::StableFieldZone());
    ASSERT_TRUE(stableId.index != 0);

    // Names should be correct
    ASSERT_TRUE(reg.GetName(pathId) == "high_frequency_path");
    ASSERT_TRUE(reg.GetName(stableId) == "stable_field_zone");

    return true;
}

// Test: PatternModule add and query
TEST(pattern_module_add_query)
{
    PatternModule pm;

    PatternRecord r1;
    r1.typeKey = PatternTypes::HighFrequencyPath();
    r1.typeId = PatternTypeId(1);
    r1.x = 5; r1.y = 5;
    r1.confidence = 0.8f;
    u64 id1 = pm.Add(r1);

    PatternRecord r2;
    r2.typeKey = PatternTypes::StableFieldZone();
    r2.typeId = PatternTypeId(2);
    r2.x = 10; r2.y = 10;
    r2.confidence = 0.9f;
    pm.Add(r2);

    ASSERT_EQ(pm.Count(), 2u);
    ASSERT_TRUE(id1 == 1);

    // FindByType
    auto paths = pm.FindByType(PatternTypes::HighFrequencyPath());
    ASSERT_EQ(static_cast<i32>(paths.size()), 1);
    ASSERT_EQ(paths[0]->x, 5);

    // FindNear
    auto near = pm.FindNear(6, 5, 2);
    ASSERT_EQ(static_cast<i32>(near.size()), 1);
    ASSERT_EQ(near[0]->x, 5);

    // Update
    ASSERT_TRUE(pm.Update(id1, 100, 0.95f, 20.0f, 3));
    auto paths2 = pm.FindByType(PatternTypes::HighFrequencyPath());
    ASSERT_EQ(paths2[0]->confidence, 0.95f);
    ASSERT_EQ(paths2[0]->observationCount, 3u);

    return true;
}

// Test: PatternModule prune
TEST(pattern_module_prune)
{
    PatternModule pm;

    for (i32 i = 0; i < 10; i++)
    {
        PatternRecord r;
        r.typeKey = PatternTypes::HighFrequencyPath();
        r.confidence = static_cast<f32>(i) / 10.0f;
        r.x = i;
        pm.Add(r);
    }

    ASSERT_EQ(pm.Count(), 10u);
    pm.Prune(5);
    ASSERT_EQ(pm.Count(), 5u);

    // Highest confidence should survive
    auto all = pm.All();
    for (const auto& p : all)
        ASSERT_TRUE(p.confidence >= 0.5f);

    return true;
}

// Test: HighFrequencyPathDetector — agent walks same area repeatedly
TEST(pattern_high_frequency_path)
{
    HumanEvolutionRulePack rulePack;
    WorldState world(16, 16, 42, rulePack);
    const auto& envCtx = rulePack.GetHumanEvolutionContext().environment;

    // Spawn one agent at a fixed position
    world.SpawnAgent(8, 8);

    // Create a scheduler with only the pattern detection system
    Scheduler scheduler;

    PatternRuntimeConfig config;
    config.observationWindowTicks = 10;  // small window for testing

    auto patternSys = std::make_unique<PatternDetectionSystem>(config);
    patternSys->AddDetector(std::make_unique<HighFrequencyPathDetector>(5));
    scheduler.AddSystem(SimPhase::Analysis, std::move(patternSys));

    // Run 20 ticks — agent stays at (8,8) every tick
    for (i32 i = 0; i < 20; i++)
        scheduler.Tick(world);

    auto& patterns = world.Patterns();

    // Should detect a HighFrequencyPath at (8,8)
    auto paths = patterns.FindByType(PatternTypes::HighFrequencyPath());
    ASSERT_TRUE(!paths.empty());

    bool found = false;
    for (const auto& p : paths)
    {
        if (p->x == 8 && p->y == 8)
        {
            found = true;
            ASSERT_TRUE(p->confidence > 0.0f);
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// Test: StableFieldZoneDetector — fire field stays above threshold
TEST(pattern_stable_field_zone)
{
    HumanEvolutionRulePack rulePack;
    WorldState world(16, 16, 42, rulePack);
    const auto& envCtx = rulePack.GetHumanEvolutionContext().environment;

    // Set fire at (8,8) above threshold — it will persist because no FireSystem
    // (we only run pattern detection, not the full simulation)
    auto& fm = world.Fields();

    Scheduler scheduler;

    PatternRuntimeConfig config;
    config.observationWindowTicks = 10;

    auto patternSys = std::make_unique<PatternDetectionSystem>(config);
    patternSys->AddDetector(std::make_unique<HighFrequencyPathDetector>(5));

    // Register test pattern type
    PatternKey testFireKey("test.stable_fire");
    PatternRegistry::Instance().Register(testFireKey, "test_stable_fire");

    FieldWatchSpec fireWatch;
    fireWatch.patternKey = testFireKey;
    fireWatch.field = envCtx.fire;
    fireWatch.threshold = 0.3f;
    fireWatch.minDuration = 5;
    patternSys->AddDetector(std::make_unique<StableFieldZoneDetector>(
        std::vector<FieldWatchSpec>{fireWatch}));
    scheduler.AddSystem(SimPhase::Analysis, std::move(patternSys));

    // Seed fire: fill both buffers, then set (8,8) = 1.0 in next, swap to current.
    // After this: current(8,8) = 1.0, next(8,8) = 1.0
    fm.FillBoth(envCtx.fire, 0.0f);
    fm.WriteNext(envCtx.fire, 8, 8, 1.0f);
    fm.SwapField(envCtx.fire);
    fm.WriteNext(envCtx.fire, 8, 8, 1.0f);

    // Run 20 ticks. Re-seed fire into next buffer before each tick so it persists.
    for (i32 i = 0; i < 20; i++)
    {
        fm.WriteNext(envCtx.fire, 8, 8, 1.0f);
        scheduler.Tick(world);
    }

    auto& patterns = world.Patterns();

    // Should detect a StableFieldZone
    auto zones = patterns.FindByType(testFireKey);
    ASSERT_TRUE(!zones.empty());

    bool found = false;
    for (const auto& p : zones)
    {
        if (p->x == 8 && p->y == 8)
        {
            found = true;
            ASSERT_TRUE(p->confidence > 0.0f);
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// Test: Pattern system is read-only — doesn't modify agents or fields
TEST(pattern_read_only)
{
    HumanEvolutionRulePack rulePack;
    WorldState world(16, 16, 42, rulePack);
    world.SpawnAgent(5, 5);

    auto& fm = world.Fields();
    const auto& envCtx = rulePack.GetHumanEvolutionContext().environment;

    // Record initial state
    i32 agentXBefore = world.Agents().agents[0].position.x;
    i32 agentYBefore = world.Agents().agents[0].position.y;
    f32 fireBefore = fm.Read(envCtx.fire, 5, 5);

    // Run pattern detection only
    Scheduler scheduler;
    PatternRuntimeConfig config;
    config.observationWindowTicks = 5;
    auto patternSys = std::make_unique<PatternDetectionSystem>(config);
    patternSys->AddDetector(std::make_unique<HighFrequencyPathDetector>(3));
    scheduler.AddSystem(SimPhase::Analysis, std::move(patternSys));

    for (i32 i = 0; i < 10; i++)
        scheduler.Tick(world);

    // State should be unchanged
    ASSERT_EQ(world.Agents().agents[0].position.x, agentXBefore);
    ASSERT_EQ(world.Agents().agents[0].position.y, agentYBefore);
    ASSERT_EQ(fm.Read(envCtx.fire, 5, 5), fireBefore);

    return true;
}

// Test: Pattern determinism — same seed produces same patterns
TEST(pattern_determinism)
{
    auto run = [](u64 seed) -> u32 {
        HumanEvolutionRulePack rulePack;
        WorldState world(16, 16, seed, rulePack);
        world.SpawnAgent(5, 5);
        world.SpawnAgent(10, 10);

        const auto& envCtx = rulePack.GetHumanEvolutionContext().environment;
        auto& fm = world.Fields();

        Scheduler scheduler;
        // Only climate + pattern (no fire spread, no agent movement)
        scheduler.AddSystem(SimPhase::Environment, std::make_unique<ClimateSystem>(envCtx));

        PatternRuntimeConfig config;
        config.observationWindowTicks = 20;
        auto patternSys = std::make_unique<PatternDetectionSystem>(config);
        patternSys->AddDetector(std::make_unique<HighFrequencyPathDetector>(5));
        scheduler.AddSystem(SimPhase::Analysis, std::move(patternSys));

        for (i32 i = 0; i < 100; i++)
            scheduler.Tick(world);

        return world.Patterns().Count();
    };

    // Same seed → same pattern count
    ASSERT_EQ(run(42), run(42));
    // Different seed → likely different (but we only check same-seed determinism)

    return true;
}
