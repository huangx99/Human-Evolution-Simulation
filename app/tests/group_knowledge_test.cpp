#include "sim/world/world_state.h"
#include "sim/group_knowledge/group_knowledge_registry.h"
#include "sim/group_knowledge/group_knowledge_module.h"
#include "sim/runtime/simulation_hash.h"
#include "sim/scheduler/scheduler.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/systems/group_knowledge_aggregation_system.h"
#include "test_framework.h"

// === Test 1: Registry basics ===

TEST(group_knowledge_registry_can_register)
{
    GroupKnowledgeKey key = MakeGroupKnowledgeKey("test.my_knowledge");
    auto& reg = GroupKnowledgeRegistry::Instance();
    auto id = reg.Register(key, "my_knowledge");

    ASSERT_TRUE(id.index > 0);
    ASSERT_EQ(reg.FindByKey(key).index, id.index);
    ASSERT_EQ(reg.GetName(id), "my_knowledge");

    // Idempotent: same key returns same id
    auto id2 = reg.Register(key, "my_knowledge");
    ASSERT_EQ(id.index, id2.index);

    return true;
}

// === Test 2: WorldState contains GroupKnowledgeModule ===

TEST(world_state_contains_group_knowledge_module)
{
    WorldState world(32, 32, 42);

    // Should not crash — module is registered by default
    auto& gk = world.GroupKnowledge();
    ASSERT_EQ(gk.records.size(), 0u);

    return true;
}

// === Test 3: RulePack registers group knowledge types ===

TEST(rulepack_registers_group_knowledge_types)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);

    auto& reg = GroupKnowledgeRegistry::Instance();
    const auto& ctx = rp.GetHumanEvolutionContext();

    ASSERT_TRUE(ctx.groupKnowledge.sharedDangerZone.index > 0);
    ASSERT_TRUE(ctx.groupKnowledge.safePath.index > 0);
    ASSERT_TRUE(ctx.groupKnowledge.resourceCluster.index > 0);

    ASSERT_EQ(reg.GetName(ctx.groupKnowledge.sharedDangerZone), "shared_danger_zone");
    ASSERT_EQ(reg.GetName(ctx.groupKnowledge.safePath), "safe_path");
    ASSERT_EQ(reg.GetName(ctx.groupKnowledge.resourceCluster), "resource_cluster");

    return true;
}

// === Test 4: Group knowledge changes full hash ===

TEST(active_group_knowledge_changes_full_hash)
{
    HumanEvolutionRulePack rp1;
    WorldState world1(32, 32, 42, rp1);

    HumanEvolutionRulePack rp2;
    WorldState world2(32, 32, 42, rp2);

    u64 hash1_before = ComputeWorldHash(world1, HashTier::Full);
    u64 hash2_before = ComputeWorldHash(world2, HashTier::Full);
    ASSERT_EQ(hash1_before, hash2_before);

    // Add a record to world2
    const auto& ctx2 = rp2.GetHumanEvolutionContext();
    world2.GroupKnowledge().CreateRecord(ctx2.groupKnowledge.sharedDangerZone,
                                          {10, 10}, 5.0f, 0.8f, 1);

    u64 hash2_after = ComputeWorldHash(world2, HashTier::Full);
    ASSERT_TRUE(hash1_before != hash2_after);

    return true;
}

// Helper: create a scheduler with only the aggregation system
static Scheduler CreateAggregationScheduler(const HumanEvolutionContext& ctx)
{
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Analysis, std::make_unique<GroupKnowledgeAggregationSystem>(
        ctx.concepts, ctx.groupKnowledge.sharedDangerZone));
    return scheduler;
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

// === Test 5: Single agent memory does NOT create group knowledge ===

TEST(single_agent_memory_does_not_create_group_knowledge)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(5, 5);
    AddMemory(world.Cognitive(), a1, ctx.concepts.fire, {5, 5}, 0.8f, 0.9f, 0);

    auto scheduler = CreateAggregationScheduler(ctx);
    scheduler.Tick(world);

    // Single agent — no group knowledge should be created
    ASSERT_EQ(world.GroupKnowledge().records.size(), 0u);

    return true;
}

// === Test 6: Same agent, multiple memories — still no group knowledge ===

TEST(same_agent_multiple_memories_does_not_create_group_knowledge)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(5, 5);
    for (int i = 0; i < 5; i++)
        AddMemory(world.Cognitive(), a1, ctx.concepts.fire, {5 + i, 5}, 0.8f, 0.9f, 0);

    auto scheduler = CreateAggregationScheduler(ctx);
    scheduler.Tick(world);

    // Same agent — no group knowledge
    ASSERT_EQ(world.GroupKnowledge().records.size(), 0u);

    return true;
}

// === Test 7: Two agents, same area — creates shared danger zone ===

TEST(two_agents_same_area_create_shared_danger_zone)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(5, 5);
    EntityId a2 = world.SpawnAgent(6, 6);

    AddMemory(world.Cognitive(), a1, ctx.concepts.fire, {5, 5}, 0.8f, 0.9f, 0);
    AddMemory(world.Cognitive(), a2, ctx.concepts.fire, {6, 6}, 0.7f, 0.85f, 0);

    auto scheduler = CreateAggregationScheduler(ctx);
    scheduler.Tick(world);

    // Two different agents, same area → group knowledge created
    ASSERT_TRUE(world.GroupKnowledge().records.size() > 0);
    const auto& rec = world.GroupKnowledge().records[0];
    ASSERT_TRUE(rec.contributors >= 2);
    ASSERT_TRUE(rec.confidence > 0.0f);

    return true;
}

// === Test 8: Far apart memories do NOT merge ===

TEST(far_apart_memories_do_not_merge)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(5, 5);
    EntityId a2 = world.SpawnAgent(5, 5);
    EntityId a3 = world.SpawnAgent(50, 50);
    EntityId a4 = world.SpawnAgent(51, 51);

    // Cluster 1: agents a1, a2 near (5,5)
    AddMemory(world.Cognitive(), a1, ctx.concepts.fire, {5, 5}, 0.8f, 0.9f, 0);
    AddMemory(world.Cognitive(), a2, ctx.concepts.fire, {6, 6}, 0.7f, 0.85f, 0);

    // Cluster 2: agents a3, a4 near (50,50) — far from cluster 1
    AddMemory(world.Cognitive(), a3, ctx.concepts.danger, {50, 50}, 0.6f, 0.7f, 0);
    AddMemory(world.Cognitive(), a4, ctx.concepts.danger, {51, 51}, 0.5f, 0.6f, 0);

    auto scheduler = CreateAggregationScheduler(ctx);
    scheduler.Tick(world);

    // Should create 2 separate records, not merge
    ASSERT_EQ(world.GroupKnowledge().records.size(), 2u);

    return true;
}

// === Test 9: Observed flee memory contributes ===

TEST(observed_flee_memory_contributes)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(5, 5);
    EntityId a2 = world.SpawnAgent(6, 6);

    // Both agents have observedFlee memories (social/abstract danger)
    AddMemory(world.Cognitive(), a1, ctx.concepts.observedFlee, {5, 5}, 0.6f, 0.7f, 0);
    AddMemory(world.Cognitive(), a2, ctx.concepts.observedFlee, {6, 6}, 0.5f, 0.6f, 0);

    auto scheduler = CreateAggregationScheduler(ctx);
    scheduler.Tick(world);

    // observedFlee is danger-related → should create group knowledge
    ASSERT_TRUE(world.GroupKnowledge().records.size() > 0);

    return true;
}

// === Test 10: Old memory does NOT contribute ===

TEST(old_memory_does_not_contribute)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(5, 5);
    EntityId a2 = world.SpawnAgent(6, 6);

    // Advance clock so tick=0 is old
    world.Sim().clock.currentTick = 100;

    AddMemory(world.Cognitive(), a1, ctx.concepts.fire, {5, 5}, 0.8f, 0.9f, 0);
    AddMemory(world.Cognitive(), a2, ctx.concepts.fire, {6, 6}, 0.7f, 0.85f, 0);

    auto scheduler = CreateAggregationScheduler(ctx);
    scheduler.Tick(world);

    // Old memories outside time window → no group knowledge
    ASSERT_EQ(world.GroupKnowledge().records.size(), 0u);

    return true;
}

// === Test 11: Aggregation does NOT write to commands ===

TEST(aggregation_does_not_write_command)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(5, 5);
    EntityId a2 = world.SpawnAgent(6, 6);

    AddMemory(world.Cognitive(), a1, ctx.concepts.fire, {5, 5}, 0.8f, 0.9f, 0);
    AddMemory(world.Cognitive(), a2, ctx.concepts.fire, {6, 6}, 0.7f, 0.85f, 0);

    auto cmdCountBefore = world.commands.GetHistory().size();
    auto scheduler = CreateAggregationScheduler(ctx);
    scheduler.Tick(world);
    auto cmdCountAfter = world.commands.GetHistory().size();

    ASSERT_EQ(cmdCountBefore, cmdCountAfter);

    return true;
}

// === Test 12: Aggregation does NOT write to cognitive module ===

TEST(aggregation_does_not_write_memory)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(5, 5);
    EntityId a2 = world.SpawnAgent(6, 6);

    AddMemory(world.Cognitive(), a1, ctx.concepts.fire, {5, 5}, 0.8f, 0.9f, 0);
    AddMemory(world.Cognitive(), a2, ctx.concepts.fire, {6, 6}, 0.7f, 0.85f, 0);

    auto memCountBefore = world.Cognitive().agentMemories[a1].size()
                        + world.Cognitive().agentMemories[a2].size();
    auto scheduler = CreateAggregationScheduler(ctx);
    scheduler.Tick(world);
    auto memCountAfter = world.Cognitive().agentMemories[a1].size()
                       + world.Cognitive().agentMemories[a2].size();

    ASSERT_EQ(memCountBefore, memCountAfter);

    return true;
}

// === Test 13: Aggregation does NOT modify agents ===

TEST(aggregation_does_not_modify_agents)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(5, 5);
    EntityId a2 = world.SpawnAgent(6, 6);

    AddMemory(world.Cognitive(), a1, ctx.concepts.fire, {5, 5}, 0.8f, 0.9f, 0);
    AddMemory(world.Cognitive(), a2, ctx.concepts.fire, {6, 6}, 0.7f, 0.85f, 0);

    auto& agent1 = world.Agents().agents[0];
    auto& agent2 = world.Agents().agents[1];

    auto pos1 = agent1.position;
    auto pos2 = agent2.position;
    auto hunger1 = agent1.hunger;
    auto hunger2 = agent2.hunger;

    auto scheduler = CreateAggregationScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_EQ(agent1.position.x, pos1.x);
    ASSERT_EQ(agent1.position.y, pos1.y);
    ASSERT_EQ(agent2.position.x, pos2.x);
    ASSERT_EQ(agent2.position.y, pos2.y);
    ASSERT_TRUE(agent1.hunger == hunger1);
    ASSERT_TRUE(agent2.hunger == hunger2);

    return true;
}
