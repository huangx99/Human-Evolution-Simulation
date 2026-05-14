#include "sim/world/world_state.h"
#include "sim/history/history_module.h"
#include "sim/history/history_registry.h"
#include "sim/group_knowledge/group_knowledge_module.h"
#include "sim/pattern/pattern_module.h"
#include "sim/cultural_trace/cultural_trace_module.h"
#include "sim/scheduler/scheduler.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/systems/cultural_trace_detection_system.h"
#include "rules/human_evolution/systems/collective_avoidance_system.h"
#include "rules/human_evolution/systems/group_knowledge_aggregation_system.h"
#include "rules/human_evolution/history/social_history_detector.h"
#include "sim/history/history_detection_system.h"
#include "test_framework.h"

// Helper: create a scheduler with full pipeline + history
static Scheduler CreateFullSchedulerWithHistory(const HumanEvolutionContext& ctx)
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

    auto historySys = std::make_unique<HistoryDetectionSystem>();
    historySys->AddDetector(std::make_unique<SocialHistoryDetector>(
        HistoryKey("human_evolution.first_shared_danger_memory"),
        HistoryKey("human_evolution.first_collective_avoidance"),
        HistoryKey("human_evolution.first_danger_avoidance_trace"),
        ctx.groupKnowledge.sharedDangerZone,
        PatternKey("human_evolution.collective_avoidance"),
        ctx.culturalTraces.dangerAvoidanceTrace));
    scheduler.AddSystem(SimPhase::History, std::move(historySys));

    return scheduler;
}

// Helper: create a scheduler with ONLY the history system (for isolated tests)
static Scheduler CreateHistoryOnlyScheduler(const HumanEvolutionContext& ctx)
{
    Scheduler scheduler;
    auto historySys = std::make_unique<HistoryDetectionSystem>();
    historySys->AddDetector(std::make_unique<SocialHistoryDetector>(
        HistoryKey("human_evolution.first_shared_danger_memory"),
        HistoryKey("human_evolution.first_collective_avoidance"),
        HistoryKey("human_evolution.first_danger_avoidance_trace"),
        ctx.groupKnowledge.sharedDangerZone,
        PatternKey("human_evolution.collective_avoidance"),
        ctx.culturalTraces.dangerAvoidanceTrace));
    scheduler.AddSystem(SimPhase::History, std::move(historySys));
    return scheduler;
}

// Helper: directly add GroupKnowledgeRecord
static GroupKnowledgeRecord& AddDangerZone(GroupKnowledgeModule& gk, GroupKnowledgeTypeId typeId,
                                            Vec2i origin, f32 radius, f32 confidence, Tick tick)
{
    return gk.CreateRecord(typeId, origin, radius, confidence, tick);
}

// Helper: directly add PatternRecord
static void AddPattern(PatternModule& pm, PatternKey key, PatternTypeId typeId,
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
    pm.Add(rec);
}

// Helper: directly add CulturalTraceRecord
static void AddTrace(CulturalTraceModule& ct, CulturalTraceTypeId typeId,
                     f32 confidence, Tick tick)
{
    ct.AddOrReinforce(typeId, {1}, {1}, confidence, tick, tick);
}

// === Test 1: Registry registers social history types ===

TEST(history_registry_registers_social_history_types)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);

    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& reg = HistoryRegistry::Instance();

    ASSERT_TRUE(ctx.history.firstSharedDangerMemory.index > 0);
    ASSERT_TRUE(ctx.history.firstCollectiveAvoidance.index > 0);
    ASSERT_TRUE(ctx.history.firstDangerAvoidanceTrace.index > 0);

    ASSERT_EQ(reg.GetName(ctx.history.firstSharedDangerMemory), "first_shared_danger_memory");
    ASSERT_EQ(reg.GetName(ctx.history.firstCollectiveAvoidance), "first_collective_avoidance");
    ASSERT_EQ(reg.GetName(ctx.history.firstDangerAvoidanceTrace), "first_danger_avoidance_trace");

    return true;
}

// === Test 2: First shared danger memory created from GroupKnowledge ===

TEST(first_shared_danger_memory_created_from_group_knowledge)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto scheduler = CreateHistoryOnlyScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_TRUE(world.History().HasEventType(
        HistoryKey("human_evolution.first_shared_danger_memory")));

    return true;
}

// === Test 3: First collective avoidance created from pattern ===

TEST(first_collective_avoidance_created_from_pattern)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddPattern(world.Patterns(),
               PatternKey("human_evolution.collective_avoidance"),
               ctx.socialPatterns.collectiveAvoidance,
               20, 20, 0.8f, 5, 0);

    auto scheduler = CreateHistoryOnlyScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_TRUE(world.History().HasEventType(
        HistoryKey("human_evolution.first_collective_avoidance")));

    return true;
}

// === Test 4: First danger avoidance trace created from cultural trace ===

TEST(first_danger_avoidance_trace_created_from_cultural_trace)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddTrace(world.CulturalTrace(), ctx.culturalTraces.dangerAvoidanceTrace,
             0.7f, 0);

    auto scheduler = CreateHistoryOnlyScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_TRUE(world.History().HasEventType(
        HistoryKey("human_evolution.first_danger_avoidance_trace")));

    return true;
}

// === Test 5: Event uses source tick and location ===

TEST(history_event_uses_source_tick_and_location)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {15, 25}, 5.0f, 0.8f, 7);

    auto scheduler = CreateHistoryOnlyScheduler(ctx);
    scheduler.Tick(world);

    // Find the event
    bool found = false;
    for (const auto& evt : world.History().Events())
    {
        if (evt.typeKey == HistoryKey("human_evolution.first_shared_danger_memory"))
        {
            ASSERT_EQ(evt.tick, 7u);  // source firstObservedTick
            ASSERT_EQ(evt.x, 15);
            ASSERT_EQ(evt.y, 25);
            ASSERT_NEAR(evt.confidence, 0.8f, 0.001f);
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// === Test 6: Same shared danger not recorded twice ===

TEST(same_shared_danger_not_recorded_twice)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto scheduler = CreateHistoryOnlyScheduler(ctx);

    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    u32 count = 0;
    for (const auto& evt : world.History().Events())
    {
        if (evt.typeKey == HistoryKey("human_evolution.first_shared_danger_memory"))
            count++;
    }
    ASSERT_EQ(count, 1u);

    return true;
}

// === Test 7: Same collective avoidance not recorded twice ===

TEST(same_collective_avoidance_not_recorded_twice)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddPattern(world.Patterns(),
               PatternKey("human_evolution.collective_avoidance"),
               ctx.socialPatterns.collectiveAvoidance,
               20, 20, 0.8f, 5, 0);

    auto scheduler = CreateHistoryOnlyScheduler(ctx);

    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    u32 count = 0;
    for (const auto& evt : world.History().Events())
    {
        if (evt.typeKey == HistoryKey("human_evolution.first_collective_avoidance"))
            count++;
    }
    ASSERT_EQ(count, 1u);

    return true;
}

// === Test 8: Same trace not recorded twice ===

TEST(same_trace_not_recorded_twice)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddTrace(world.CulturalTrace(), ctx.culturalTraces.dangerAvoidanceTrace,
             0.7f, 0);

    auto scheduler = CreateHistoryOnlyScheduler(ctx);

    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    u32 count = 0;
    for (const auto& evt : world.History().Events())
    {
        if (evt.typeKey == HistoryKey("human_evolution.first_danger_avoidance_trace"))
            count++;
    }
    ASSERT_EQ(count, 1u);

    return true;
}

// === Test 9: Does not write to command module ===

TEST(social_history_does_not_write_command)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto cmdBefore = world.commands.GetHistory().size();
    auto scheduler = CreateHistoryOnlyScheduler(ctx);
    scheduler.Tick(world);
    auto cmdAfter = world.commands.GetHistory().size();

    ASSERT_EQ(cmdBefore, cmdAfter);

    return true;
}

// === Test 10: Does not write to memory module ===

TEST(social_history_does_not_write_memory)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(20, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto memBefore = world.Cognitive().agentMemories[a1].size();
    auto scheduler = CreateHistoryOnlyScheduler(ctx);
    scheduler.Tick(world);
    auto memAfter = world.Cognitive().agentMemories[a1].size();

    ASSERT_EQ(memBefore, memAfter);

    return true;
}

// === Test 11: Does not modify agents ===

TEST(social_history_does_not_modify_agents)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(20, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto& agent = world.Agents().agents[0];
    auto pos = agent.position;
    auto hunger = agent.hunger;

    auto scheduler = CreateHistoryOnlyScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_EQ(agent.position.x, pos.x);
    ASSERT_EQ(agent.position.y, pos.y);
    ASSERT_TRUE(agent.hunger == hunger);

    return true;
}

// === Test 12: System is in pipeline ===

TEST(social_history_system_in_pipeline)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);

    auto systems = rp.CreateSystems();
    bool found = false;
    for (const auto& reg : systems)
    {
        auto desc = reg.system->Descriptor();
        if (std::string(desc.name) == "HistoryDetectionSystem")
        {
            ASSERT_TRUE(reg.phase == SimPhase::History);
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);

    return true;
}

TEST(social_history_descriptor_declares_detector_access)
{
    HistoryDetectionSystem system;
    auto desc = system.Descriptor();

    bool readsGroupKnowledge = false;
    bool readsCulturalTrace = false;
    bool historyReadWrite = false;

    for (size_t i = 0; i < desc.readCount; i++)
    {
        if (desc.reads[i].module == ModuleTag::GroupKnowledge &&
            desc.reads[i].mode == AccessMode::Read)
            readsGroupKnowledge = true;
        if (desc.reads[i].module == ModuleTag::CulturalTrace &&
            desc.reads[i].mode == AccessMode::Read)
            readsCulturalTrace = true;
    }

    for (size_t i = 0; i < desc.writeCount; i++)
    {
        if (desc.writes[i].module == ModuleTag::History &&
            desc.writes[i].mode == AccessMode::ReadWrite)
            historyReadWrite = true;
    }

    ASSERT_TRUE(readsGroupKnowledge);
    ASSERT_TRUE(readsCulturalTrace);
    ASSERT_TRUE(historyReadWrite);

    return true;
}
