#include "sim/world/world_state.h"
#include "sim/group_knowledge/group_knowledge_module.h"
#include "sim/pattern/pattern_module.h"
#include "sim/pattern/pattern_temporal_state_module.h"
#include "sim/pattern/pattern_registry.h"
#include "sim/runtime/simulation_hash.h"
#include "sim/scheduler/scheduler.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/systems/collective_avoidance_system.h"
#include "rules/human_evolution/systems/group_knowledge_aggregation_system.h"
#include "test_framework.h"

// Helper: create a scheduler with aggregation + avoidance systems
static Scheduler CreateAvoidanceScheduler(const HumanEvolutionContext& ctx)
{
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Analysis, std::make_unique<GroupKnowledgeAggregationSystem>(
        ctx.concepts, ctx.groupKnowledge.sharedDangerZone));
    scheduler.AddSystem(SimPhase::Analysis, std::make_unique<CollectiveAvoidanceSystem>(
        ctx.socialPatterns.collectiveAvoidance,
        PatternKey("human_evolution.collective_avoidance"),
        ctx.groupKnowledge.sharedDangerZone));
    return scheduler;
}

// Helper: directly create a danger zone in GroupKnowledgeModule
static GroupKnowledgeRecord& AddDangerZone(GroupKnowledgeModule& gk, GroupKnowledgeTypeId typeId,
                                            Vec2i origin, f32 radius, f32 confidence, Tick tick)
{
    return gk.CreateRecord(typeId, origin, radius, confidence, tick);
}

// Helper: add a memory to an agent
static void AddMemory(CognitiveModule& cog, EntityId agentId, ConceptTypeId concept,
                       Vec2i location, f32 strength, f32 confidence, Tick tick)
{
    auto& mems = cog.GetAgentMemories(agentId);
    MemoryRecord mem;
    mem.id = cog.nextMemoryId++;
    mem.ownerId = agentId;
    mem.subject = concept;
    mem.location = location;
    mem.strength = strength;
    mem.confidence = confidence;
    mem.createdTick = tick;
    mems.push_back(mem);
}

// === Test 1: Basic detection ===

TEST(collective_avoidance_basic_detection)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    // Create danger zone at (20,20) radius=5
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    // 3 agents nearby but outside zone (distance ~8 from center, within 1.5x5=7.5... need closer)
    // Place at (20, 27): dist = 7.0, within nearbyBound = 7.5
    EntityId a1 = world.SpawnAgent(20, 27);
    EntityId a2 = world.SpawnAgent(22, 27);
    EntityId a3 = world.SpawnAgent(18, 27);

    auto scheduler = CreateAvoidanceScheduler(ctx);

    // Run 5 ticks (exceeds threshold=3)
    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    // Should have 1 collective_avoidance pattern
    auto patterns = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
    ASSERT_TRUE(patterns.size() > 0);
    ASSERT_TRUE(patterns[0]->confidence > 0.0f);

    return true;
}

// === Test 2: No detection when agents inside zone ===

TEST(collective_avoidance_no_detection_agents_inside)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    // All agents inside zone
    EntityId a1 = world.SpawnAgent(20, 20);
    EntityId a2 = world.SpawnAgent(21, 20);
    EntityId a3 = world.SpawnAgent(20, 21);

    auto scheduler = CreateAvoidanceScheduler(ctx);

    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    auto patterns = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
    ASSERT_EQ(patterns.size(), 0u);

    return true;
}

// === Test 3: Reset on intrusion ===

TEST(collective_avoidance_reset_on_intrusion)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    // 3 agents nearby but outside
    EntityId a1 = world.SpawnAgent(20, 27);
    EntityId a2 = world.SpawnAgent(22, 27);
    EntityId a3 = world.SpawnAgent(18, 27);

    auto scheduler = CreateAvoidanceScheduler(ctx);

    // Tick 1-2: accumulate (below threshold)
    scheduler.Tick(world);
    scheduler.Tick(world);

    // Move agent a1 inside zone → resets counter
    world.Agents().Find(a1)->position = {20, 20};
    scheduler.Tick(world);

    // Move agent back outside
    world.Agents().Find(a1)->position = {20, 27};
    scheduler.Tick(world);
    scheduler.Tick(world);

    // Counter was reset, only 2 more ticks → total after reset = 2 < 3
    auto patterns = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
    ASSERT_EQ(patterns.size(), 0u);

    return true;
}

// === Test 4: Consecutive tick threshold ===

TEST(collective_avoidance_consecutive_tick_threshold)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    EntityId a1 = world.SpawnAgent(20, 27);
    EntityId a2 = world.SpawnAgent(22, 27);
    EntityId a3 = world.SpawnAgent(18, 27);

    auto scheduler = CreateAvoidanceScheduler(ctx);

    // Tick 1-2: below threshold
    scheduler.Tick(world);
    scheduler.Tick(world);

    auto patterns2 = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
    ASSERT_EQ(patterns2.size(), 0u);

    // Tick 3: at threshold
    scheduler.Tick(world);

    auto patterns3 = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
    ASSERT_EQ(patterns3.size(), 1u);

    return true;
}

// === Test 5: Multiple zones independent ===

TEST(collective_avoidance_multiple_zones_independent)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    // Zone 1 at (10,10) — agents nearby
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {10, 10}, 5.0f, 0.8f, 0);
    // Zone 2 at (40,40) — no agents nearby
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {40, 40}, 5.0f, 0.8f, 0);

    // 3 agents near zone 1 only
    EntityId a1 = world.SpawnAgent(10, 17);
    EntityId a2 = world.SpawnAgent(12, 17);
    EntityId a3 = world.SpawnAgent(8, 17);

    auto scheduler = CreateAvoidanceScheduler(ctx);

    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    auto patterns = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
    ASSERT_EQ(patterns.size(), 1u);
    ASSERT_EQ(patterns[0]->x, 10);
    ASSERT_EQ(patterns[0]->y, 10);

    return true;
}

// === Test 6: Dedup same zone — only 1 pattern, observationCount grows ===

TEST(collective_avoidance_dedup_same_zone)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    EntityId a1 = world.SpawnAgent(20, 27);
    EntityId a2 = world.SpawnAgent(22, 27);
    EntityId a3 = world.SpawnAgent(18, 27);

    auto scheduler = CreateAvoidanceScheduler(ctx);

    // Run 10 ticks (well past threshold)
    for (int i = 0; i < 10; i++)
        scheduler.Tick(world);

    auto patterns = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
    ASSERT_EQ(patterns.size(), 1u);
    ASSERT_TRUE(patterns[0]->observationCount > 1);

    return true;
}

// === Test 7: Determinism ===

TEST(collective_avoidance_determinism)
{
    auto runOnce = []() -> std::vector<PatternRecord>
    {
        HumanEvolutionRulePack rp;
        WorldState world(64, 64, 42, rp);
        const auto& ctx = rp.GetHumanEvolutionContext();

        AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                      {20, 20}, 5.0f, 0.8f, 0);

        world.SpawnAgent(20, 27);
        world.SpawnAgent(22, 27);
        world.SpawnAgent(18, 27);

        auto scheduler = CreateAvoidanceScheduler(ctx);

        for (int i = 0; i < 5; i++)
            scheduler.Tick(world);

        auto found = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
        std::vector<PatternRecord> result;
        for (const auto* p : found)
            result.push_back(*p);
        return result;
    };

    auto r1 = runOnce();
    auto r2 = runOnce();

    ASSERT_EQ(r1.size(), r2.size());
    for (size_t i = 0; i < r1.size(); i++)
    {
        ASSERT_EQ(r1[i].x, r2[i].x);
        ASSERT_EQ(r1[i].y, r2[i].y);
        ASSERT_NEAR(r1[i].confidence, r2[i].confidence, 0.001f);
        ASSERT_EQ(r1[i].observationCount, r2[i].observationCount);
    }

    return true;
}

// === Test 8: Boundary nearby multiplier ===

TEST(collective_avoidance_boundary_nearby_multiplier)
{
    // Zone at (20,20) radius=5 → nearbyBound = 7.5
    // Agent at (20,27): dist = 7.0 → within bound
    {
        HumanEvolutionRulePack rp;
        WorldState world(64, 64, 42, rp);
        const auto& ctx = rp.GetHumanEvolutionContext();

        AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                      {20, 20}, 5.0f, 0.8f, 0);

        world.SpawnAgent(20, 27);
        world.SpawnAgent(22, 27);
        world.SpawnAgent(18, 27);

        auto scheduler = CreateAvoidanceScheduler(ctx);
        for (int i = 0; i < 5; i++)
            scheduler.Tick(world);

        auto patterns = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
        ASSERT_EQ(patterns.size(), 1u);
    }

    // Agent at (20,28): dist = 8.0 → outside bound (7.5)
    {
        HumanEvolutionRulePack rp;
        WorldState world(64, 64, 42, rp);
        const auto& ctx = rp.GetHumanEvolutionContext();

        AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                      {20, 20}, 5.0f, 0.8f, 0);

        world.SpawnAgent(20, 28);
        world.SpawnAgent(22, 28);
        world.SpawnAgent(18, 28);

        auto scheduler = CreateAvoidanceScheduler(ctx);
        for (int i = 0; i < 5; i++)
            scheduler.Tick(world);

        auto patterns = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
        ASSERT_EQ(patterns.size(), 0u);
    }

    return true;
}

// === Test 9: Does not write to command module ===

TEST(collective_avoidance_does_not_write_command)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    world.SpawnAgent(20, 27);
    world.SpawnAgent(22, 27);
    world.SpawnAgent(18, 27);

    auto cmdCountBefore = world.commands.GetHistory().size();
    auto scheduler = CreateAvoidanceScheduler(ctx);
    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);
    auto cmdCountAfter = world.commands.GetHistory().size();

    ASSERT_EQ(cmdCountBefore, cmdCountAfter);

    return true;
}

// === Test 10: Does not write to memory module ===

TEST(collective_avoidance_does_not_write_memory)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(20, 27);
    EntityId a2 = world.SpawnAgent(22, 27);
    EntityId a3 = world.SpawnAgent(18, 27);

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto memCountBefore = world.Cognitive().agentMemories[a1].size()
                        + world.Cognitive().agentMemories[a2].size()
                        + world.Cognitive().agentMemories[a3].size();

    auto scheduler = CreateAvoidanceScheduler(ctx);
    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    auto memCountAfter = world.Cognitive().agentMemories[a1].size()
                       + world.Cognitive().agentMemories[a2].size()
                       + world.Cognitive().agentMemories[a3].size();

    ASSERT_EQ(memCountBefore, memCountAfter);

    return true;
}

// === Test 11: Does not modify agents ===

TEST(collective_avoidance_does_not_modify_agents)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    EntityId a1 = world.SpawnAgent(20, 27);
    EntityId a2 = world.SpawnAgent(22, 27);

    auto& agent1 = world.Agents().agents[0];
    auto& agent2 = world.Agents().agents[1];
    auto pos1 = agent1.position;
    auto pos2 = agent2.position;
    auto hunger1 = agent1.hunger;
    auto hunger2 = agent2.hunger;

    auto scheduler = CreateAvoidanceScheduler(ctx);
    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    ASSERT_EQ(agent1.position.x, pos1.x);
    ASSERT_EQ(agent1.position.y, pos1.y);
    ASSERT_EQ(agent2.position.x, pos2.x);
    ASSERT_EQ(agent2.position.y, pos2.y);
    ASSERT_TRUE(agent1.hunger == hunger1);
    ASSERT_TRUE(agent2.hunger == hunger2);

    return true;
}

// === Test 12: Does not modify group knowledge ===

TEST(collective_avoidance_does_not_modify_group_knowledge)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    world.SpawnAgent(20, 27);
    world.SpawnAgent(22, 27);
    world.SpawnAgent(18, 27);

    auto gkCountBefore = world.GroupKnowledge().records.size();
    auto gkConfBefore = world.GroupKnowledge().records[0].confidence;
    auto gkReinforcedBefore = world.GroupKnowledge().records[0].lastReinforcedTick;

    auto scheduler = CreateAvoidanceScheduler(ctx);
    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    ASSERT_EQ(world.GroupKnowledge().records.size(), gkCountBefore);
    ASSERT_NEAR(world.GroupKnowledge().records[0].confidence, gkConfBefore, 0.001f);
    ASSERT_EQ(world.GroupKnowledge().records[0].lastReinforcedTick, gkReinforcedBefore);

    return true;
}

// === Test 13: No nearby agents → no pattern ===

TEST(collective_avoidance_no_nearby_agents)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    // Agents far from zone
    world.SpawnAgent(0, 0);
    world.SpawnAgent(1, 1);

    auto scheduler = CreateAvoidanceScheduler(ctx);
    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    auto patterns = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
    ASSERT_EQ(patterns.size(), 0u);

    return true;
}

// === Test 14: Only one agent nearby → no pattern ===

TEST(collective_avoidance_only_one_agent_nearby)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    // Only 1 agent nearby
    world.SpawnAgent(20, 27);

    auto scheduler = CreateAvoidanceScheduler(ctx);
    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    auto patterns = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
    ASSERT_EQ(patterns.size(), 0u);

    return true;
}

// === Test 15: State persists across scheduler ticks ===

TEST(collective_avoidance_state_persists_across_ticks)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    world.SpawnAgent(20, 27);
    world.SpawnAgent(22, 27);
    world.SpawnAgent(18, 27);

    auto scheduler = CreateAvoidanceScheduler(ctx);

    // Tick 1
    scheduler.Tick(world);
    auto* counter1 = world.PatternTemporalState().Find(
        ctx.socialPatterns.collectiveAvoidance, 1u);
    ASSERT_TRUE(counter1 != nullptr);
    ASSERT_EQ(counter1->consecutiveTicks, 1u);

    // Tick 2 — state should persist
    scheduler.Tick(world);
    auto* counter2 = world.PatternTemporalState().Find(
        ctx.socialPatterns.collectiveAvoidance, 1u);
    ASSERT_TRUE(counter2 != nullptr);
    ASSERT_EQ(counter2->consecutiveTicks, 2u);

    return true;
}

// === Test 16: Pattern merge selects nearest existing ===

TEST(collective_avoidance_pattern_merge_selects_nearest)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    // Create zone close to an existing pattern
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    // Manually add a pattern at (21, 21) — close to zone origin
    PatternRecord existingRec;
    existingRec.typeKey = PatternKey("human_evolution.collective_avoidance");
    existingRec.typeId = ctx.socialPatterns.collectiveAvoidance;
    existingRec.x = 21;
    existingRec.y = 21;
    existingRec.firstDetectedTick = 0;
    existingRec.lastObservedTick = 0;
    existingRec.confidence = 0.5f;
    existingRec.magnitude = 5.0f;
    existingRec.observationCount = 1;
    world.Patterns().Add(existingRec);

    world.SpawnAgent(20, 27);
    world.SpawnAgent(22, 27);
    world.SpawnAgent(18, 27);

    auto scheduler = CreateAvoidanceScheduler(ctx);
    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    // Should have reinforced the existing pattern, not created a new one
    auto patterns = world.Patterns().FindByType(PatternKey("human_evolution.collective_avoidance"));
    ASSERT_EQ(patterns.size(), 1u);
    ASSERT_EQ(patterns[0]->x, 21);  // existing pattern location
    ASSERT_EQ(patterns[0]->y, 21);
    ASSERT_TRUE(patterns[0]->observationCount > 1);

    return true;
}

// === Test 17: State not in FullHash ===

TEST(collective_avoidance_state_not_in_full_hash)
{
    // PatternTemporalState is derived state — should not affect FullHash.
    // Verify by computing hash before and after manually modifying temporal state.

    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    world.SpawnAgent(20, 27);
    world.SpawnAgent(22, 27);

    auto scheduler = CreateAvoidanceScheduler(ctx);
    scheduler.Tick(world);

    u64 hashBefore = ComputeWorldHash(world, HashTier::Full);

    // Manually inject temporal state — should NOT change FullHash
    world.PatternTemporalState().GetOrCreate(ctx.socialPatterns.collectiveAvoidance, 1u);
    world.PatternTemporalState().counters.back().consecutiveTicks = 999;
    world.PatternTemporalState().counters.back().lastUpdatedTick = 500;

    u64 hashAfter = ComputeWorldHash(world, HashTier::Full);
    ASSERT_EQ(hashBefore, hashAfter);

    return true;
}
