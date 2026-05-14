#include "sim/world/world_state.h"
#include "sim/cultural_trace/cultural_trace_module.h"
#include "sim/cultural_trace/cultural_trace_registry.h"
#include "sim/group_knowledge/group_knowledge_module.h"
#include "sim/pattern/pattern_module.h"
#include "sim/scheduler/scheduler.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/systems/cultural_trace_detection_system.h"
#include "rules/human_evolution/systems/collective_avoidance_system.h"
#include "rules/human_evolution/systems/group_knowledge_aggregation_system.h"
#include "test_framework.h"

// Helper: create a scheduler with the full pipeline
static Scheduler CreateFullScheduler(const HumanEvolutionContext& ctx)
{
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Analysis, std::make_unique<GroupKnowledgeAggregationSystem>(
        ctx.concepts, ctx.groupKnowledge.sharedDangerZone));
    scheduler.AddSystem(SimPhase::Analysis, std::make_unique<CollectiveAvoidanceSystem>(
        ctx.socialPatterns.collectiveAvoidance,
        PatternKey("human_evolution.collective_avoidance"),
        ctx.groupKnowledge.sharedDangerZone));
    scheduler.AddSystem(SimPhase::Analysis, std::make_unique<CulturalTraceDetectionSystem>(
        ctx.culturalTraces.dangerAvoidanceTrace,
        PatternKey("human_evolution.collective_avoidance"),
        ctx.groupKnowledge.sharedDangerZone));
    return scheduler;
}

// Helper: directly add a GroupKnowledgeRecord
static GroupKnowledgeRecord& AddDangerZone(GroupKnowledgeModule& gk, GroupKnowledgeTypeId typeId,
                                            Vec2i origin, f32 radius, f32 confidence, Tick tick)
{
    return gk.CreateRecord(typeId, origin, radius, confidence, tick);
}

// Helper: directly add a PatternRecord
static u64 AddPattern(PatternModule& pm, PatternKey key, PatternTypeId typeId,
                      i32 x, i32 y, f32 confidence, u32 observationCount, Tick tick)
{
    PatternRecord rec;
    rec.typeKey = key;
    rec.typeId = typeId;
    rec.x = x;
    rec.y = y;
    rec.firstDetectedTick = tick;
    rec.lastObservedTick = tick;
    rec.confidence = confidence;
    rec.magnitude = 5.0f;
    rec.observationCount = observationCount;
    return pm.Add(rec);
}

// === Test 1: Registry basics ===

TEST(cultural_trace_registry_can_register)
{
    CulturalTraceKey key = MakeCulturalTraceKey("test.my_trace");
    auto& reg = CulturalTraceRegistry::Instance();
    auto id = reg.Register(key, "my_trace");

    ASSERT_TRUE(id.index > 0);
    ASSERT_EQ(reg.FindByKey(key).index, id.index);
    ASSERT_EQ(reg.GetName(id), "my_trace");

    // Idempotent
    auto id2 = reg.Register(key, "my_trace");
    ASSERT_EQ(id.index, id2.index);

    return true;
}

// === Test 2: WorldState contains module ===

TEST(world_state_contains_cultural_trace_module)
{
    WorldState world(32, 32, 42);

    auto& ct = world.CulturalTrace();
    ASSERT_EQ(ct.records.size(), 0u);

    return true;
}

// === Test 3: RulePack registers types ===

TEST(rulepack_registers_cultural_trace_types)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);

    auto& reg = CulturalTraceRegistry::Instance();
    const auto& ctx = rp.GetHumanEvolutionContext();

    ASSERT_TRUE(ctx.culturalTraces.dangerAvoidanceTrace.index > 0);
    ASSERT_EQ(reg.GetName(ctx.culturalTraces.dangerAvoidanceTrace), "danger_avoidance_trace");

    return true;
}

// === Test 4: Requires GroupKnowledge (pattern alone is not enough) ===

TEST(danger_avoidance_trace_requires_group_knowledge)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    // Only pattern, no GroupKnowledge
    AddPattern(world.Patterns(),
               PatternKey("human_evolution.collective_avoidance"),
               ctx.socialPatterns.collectiveAvoidance,
               20, 20, 0.8f, 5, 0);

    auto scheduler = CreateFullScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_EQ(world.CulturalTrace().records.size(), 0u);

    return true;
}

// === Test 5: Requires pattern (GroupKnowledge alone is not enough) ===

TEST(danger_avoidance_trace_requires_collective_avoidance_pattern)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    // Only GroupKnowledge, no pattern
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto scheduler = CreateFullScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_EQ(world.CulturalTrace().records.size(), 0u);

    return true;
}

// === Test 6: Requires spatial alignment ===

TEST(danger_avoidance_trace_requires_spatial_alignment)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    // GK at (10,10), pattern at (40,40) — too far apart
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {10, 10}, 5.0f, 0.8f, 0);
    AddPattern(world.Patterns(),
               PatternKey("human_evolution.collective_avoidance"),
               ctx.socialPatterns.collectiveAvoidance,
               40, 40, 0.8f, 5, 0);

    auto scheduler = CreateFullScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_EQ(world.CulturalTrace().records.size(), 0u);

    return true;
}

// === Test 7: Created when all conditions met ===

TEST(danger_avoidance_trace_created_from_stable_sources)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    // GK at (20,20) with good confidence
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);
    // Pattern at (21,21) — within merge radius, good confidence + observations
    AddPattern(world.Patterns(),
               PatternKey("human_evolution.collective_avoidance"),
               ctx.socialPatterns.collectiveAvoidance,
               21, 21, 0.8f, 5, 0);

    auto scheduler = CreateFullScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_TRUE(world.CulturalTrace().records.size() > 0);
    const auto& trace = world.CulturalTrace().records[0];
    ASSERT_EQ(trace.typeId.index, ctx.culturalTraces.dangerAvoidanceTrace.index);
    ASSERT_TRUE(trace.confidence > 0.0f);
    ASSERT_EQ(trace.reinforcementCount, 1u);

    return true;
}

// === Test 8: Same sources don't reinforce every tick ===

TEST(same_sources_do_not_reinforce_every_tick)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);
    AddPattern(world.Patterns(),
               PatternKey("human_evolution.collective_avoidance"),
               ctx.socialPatterns.collectiveAvoidance,
               20, 20, 0.8f, 5, 0);

    auto scheduler = CreateFullScheduler(ctx);

    // Run multiple ticks — should still be 1 trace
    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    ASSERT_EQ(world.CulturalTrace().records.size(), 1u);
    ASSERT_EQ(world.CulturalTrace().records[0].reinforcementCount, 1u);

    return true;
}

// === Test 9: Newer pattern observation reinforces ===

TEST(newer_pattern_observation_reinforces_trace)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);
    u64 patternId = AddPattern(world.Patterns(),
                               PatternKey("human_evolution.collective_avoidance"),
                               ctx.socialPatterns.collectiveAvoidance,
                               20, 20, 0.8f, 5, 0);

    auto scheduler = CreateFullScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_EQ(world.CulturalTrace().records.size(), 1u);
    f32 confBefore = world.CulturalTrace().records[0].confidence;
    u32 countBefore = world.CulturalTrace().records[0].reinforcementCount;

    Tick evidenceTick = world.Sim().clock.currentTick;
    ASSERT_TRUE(world.Patterns().Update(patternId, evidenceTick, 0.8f, 5.0f, 6));
    scheduler.Tick(world);

    ASSERT_EQ(world.CulturalTrace().records.size(), 1u);
    ASSERT_TRUE(world.CulturalTrace().records[0].confidence > confBefore);
    ASSERT_EQ(world.CulturalTrace().records[0].reinforcementCount, countBefore + 1);

    return true;
}

// === Test 10: Newer group knowledge reinforces ===

TEST(newer_group_knowledge_reinforces_trace)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    auto& zone = AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                               {20, 20}, 5.0f, 0.8f, 0);
    AddPattern(world.Patterns(),
               PatternKey("human_evolution.collective_avoidance"),
               ctx.socialPatterns.collectiveAvoidance,
               20, 20, 0.8f, 5, 0);

    auto scheduler = CreateFullScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_EQ(world.CulturalTrace().records.size(), 1u);
    f32 confBefore = world.CulturalTrace().records[0].confidence;
    u32 countBefore = world.CulturalTrace().records[0].reinforcementCount;

    Tick evidenceTick = world.Sim().clock.currentTick;
    world.GroupKnowledge().ReinforceRecord(zone.id, 0.0f, 1, evidenceTick, evidenceTick);
    scheduler.Tick(world);

    ASSERT_EQ(world.CulturalTrace().records.size(), 1u);
    ASSERT_TRUE(world.CulturalTrace().records[0].confidence > confBefore);
    ASSERT_EQ(world.CulturalTrace().records[0].reinforcementCount, countBefore + 1);

    return true;
}

// === Test 11: Does not write to command module ===

TEST(cultural_trace_does_not_write_command)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);
    AddPattern(world.Patterns(),
               PatternKey("human_evolution.collective_avoidance"),
               ctx.socialPatterns.collectiveAvoidance,
               20, 20, 0.8f, 5, 0);

    auto cmdBefore = world.commands.GetHistory().size();
    auto scheduler = CreateFullScheduler(ctx);
    scheduler.Tick(world);
    auto cmdAfter = world.commands.GetHistory().size();

    ASSERT_EQ(cmdBefore, cmdAfter);

    return true;
}

// === Test 12: Does not write to memory module ===

TEST(cultural_trace_does_not_write_memory)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(20, 27);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);
    AddPattern(world.Patterns(),
               PatternKey("human_evolution.collective_avoidance"),
               ctx.socialPatterns.collectiveAvoidance,
               20, 20, 0.8f, 5, 0);

    auto memBefore = world.Cognitive().agentMemories[a1].size();
    auto scheduler = CreateFullScheduler(ctx);
    scheduler.Tick(world);
    auto memAfter = world.Cognitive().agentMemories[a1].size();

    ASSERT_EQ(memBefore, memAfter);

    return true;
}

// === Test 13: Does not modify agents ===

TEST(cultural_trace_does_not_modify_agents)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(20, 27);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);
    AddPattern(world.Patterns(),
               PatternKey("human_evolution.collective_avoidance"),
               ctx.socialPatterns.collectiveAvoidance,
               20, 20, 0.8f, 5, 0);

    auto& agent = world.Agents().agents[0];
    auto pos = agent.position;
    auto hunger = agent.hunger;

    auto scheduler = CreateFullScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_EQ(agent.position.x, pos.x);
    ASSERT_EQ(agent.position.y, pos.y);
    ASSERT_TRUE(agent.hunger == hunger);

    return true;
}

// === Test 14: System is in pipeline ===

TEST(cultural_trace_system_in_pipeline)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);

    auto systems = rp.CreateSystems();
    bool found = false;
    for (const auto& reg : systems)
    {
        auto desc = reg.system->Descriptor();
        if (std::string(desc.name) == "CulturalTraceDetectionSystem")
        {
            ASSERT_TRUE(reg.phase == SimPhase::Analysis);
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);

    return true;
}
