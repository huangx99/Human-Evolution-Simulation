#include "sim/world/world_state.h"
#include "sim/cognitive/awareness_cooldown_module.h"
#include "sim/scheduler/scheduler.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/systems/group_knowledge_aggregation_system.h"
#include "rules/human_evolution/systems/group_knowledge_awareness_system.h"
#include "sim/system/cognitive_attention_system.h"
#include "sim/system/cognitive_memory_system.h"
#include "rules/human_evolution/systems/human_evolution_memory_inference_policy.h"
#include "test_framework.h"

// Helper: create a scheduler with GroupKnowledgeAwarenessSystem + Attention + Memory
static Scheduler CreateAwarenessScheduler(const HumanEvolutionContext& ctx,
                                          HumanEvolutionMemoryInferencePolicy* policy)
{
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<GroupKnowledgeAwarenessSystem>(
        ctx.concepts.groupDangerEvidence, ctx.groupKnowledge.sharedDangerZone));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveMemorySystem>(policy));
    return scheduler;
}

// Helper: create a scheduler with ONLY the awareness system (for isolated tests)
static Scheduler CreateAwarenessOnlyScheduler(const HumanEvolutionContext& ctx)
{
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<GroupKnowledgeAwarenessSystem>(
        ctx.concepts.groupDangerEvidence, ctx.groupKnowledge.sharedDangerZone));
    return scheduler;
}

// Helper: directly add a GroupKnowledgeRecord
static GroupKnowledgeRecord& AddDangerZone(GroupKnowledgeModule& gk, GroupKnowledgeTypeId typeId,
                                            Vec2i origin, f32 radius, f32 confidence, Tick tick)
{
    return gk.CreateRecord(typeId, origin, radius, confidence, tick);
}

// === Test 1: Awareness emits stimulus near shared danger zone ===

TEST(awareness_emits_stimulus_near_shared_danger_zone)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(20, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto scheduler = CreateAwarenessOnlyScheduler(ctx);
    scheduler.Tick(world);

    // Agent is at zone center, should receive stimulus
    bool found = false;
    for (const auto& stim : world.Cognitive().frameStimuli)
    {
        if (stim.observerId == a1 && stim.sense == SenseType::Social)
        {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// === Test 2: Far agent does not receive stimulus ===

TEST(far_agent_does_not_receive_stimulus)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(50, 50);  // far from zone
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto scheduler = CreateAwarenessOnlyScheduler(ctx);
    scheduler.Tick(world);

    for (const auto& stim : world.Cognitive().frameStimuli)
    {
        if (stim.observerId == a1 && stim.sense == SenseType::Social)
        {
            ASSERT_TRUE(false);  // should not reach here
        }
    }

    return true;
}

// === Test 3: Awareness stimulus has correct fields ===

TEST(awareness_stimulus_has_correct_fields)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(22, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto scheduler = CreateAwarenessOnlyScheduler(ctx);
    scheduler.Tick(world);

    bool found = false;
    for (const auto& stim : world.Cognitive().frameStimuli)
    {
        if (stim.observerId == a1 && stim.sense == SenseType::Social)
        {
            ASSERT_EQ(stim.concept.index, ctx.concepts.groupDangerEvidence.index);
            ASSERT_EQ(stim.sense, SenseType::Social);
            ASSERT_EQ(stim.location.x, 20);
            ASSERT_EQ(stim.location.y, 20);
            ASSERT_TRUE(stim.intensity > 0.0f);
            ASSERT_NEAR(stim.confidence, 0.8f, 0.001f);
            ASSERT_TRUE(stim.distance > 0.0f);
            ASSERT_EQ(stim.sourceEntityId, 0u);  // group knowledge is not an entity
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// === Test 4: Awareness stimulus enters memory via attention ===

TEST(awareness_stimulus_enters_memory_via_attention)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(20, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.9f, 0);

    HumanEvolutionMemoryInferencePolicy policy(ctx.concepts);
    auto scheduler = CreateAwarenessScheduler(ctx, &policy);

    // Run a few ticks so memory can form
    for (int i = 0; i < 5; i++)
        scheduler.Tick(world);

    // Check that agent has at least one memory
    auto& mems = world.Cognitive().agentMemories[a1];
    ASSERT_TRUE(mems.size() > 0);

    // Check that at least one memory is related to groupDangerEvidence concept
    bool found = false;
    for (const auto& mem : mems)
    {
        if (mem.subject.index == ctx.concepts.groupDangerEvidence.index)
        {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// === Test 5: Awareness does not create memory directly ===

TEST(awareness_does_not_create_memory_directly)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(20, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto memBefore = world.Cognitive().agentMemories[a1].size();
    auto scheduler = CreateAwarenessOnlyScheduler(ctx);
    scheduler.Tick(world);
    auto memAfter = world.Cognitive().agentMemories[a1].size();

    // Awareness system only writes frameStimuli, not agentMemories
    ASSERT_EQ(memBefore, memAfter);

    return true;
}

// === Test 6: Awareness does not write command ===

TEST(awareness_does_not_write_command)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(20, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto cmdBefore = world.commands.GetHistory().size();
    auto scheduler = CreateAwarenessOnlyScheduler(ctx);
    scheduler.Tick(world);
    auto cmdAfter = world.commands.GetHistory().size();

    ASSERT_EQ(cmdBefore, cmdAfter);

    return true;
}

// === Test 7: Awareness does not modify agent ===

TEST(awareness_does_not_modify_agent)
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

    auto scheduler = CreateAwarenessOnlyScheduler(ctx);
    scheduler.Tick(world);

    ASSERT_EQ(agent.position.x, pos.x);
    ASSERT_EQ(agent.position.y, pos.y);
    ASSERT_TRUE(agent.hunger == hunger);

    return true;
}

// === Test 8: Awareness does not modify group knowledge ===

TEST(awareness_does_not_modify_group_knowledge)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(20, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto gkBefore = world.GroupKnowledge().records.size();
    auto scheduler = CreateAwarenessOnlyScheduler(ctx);
    scheduler.Tick(world);
    auto gkAfter = world.GroupKnowledge().records.size();

    ASSERT_EQ(gkBefore, gkAfter);

    return true;
}

// === Test 9: Awareness cooldown prevents repeated stimulus ===

TEST(awareness_cooldown_prevents_repeated_stimulus)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(20, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto scheduler = CreateAwarenessOnlyScheduler(ctx);

    // First tick: stimulus should be emitted
    scheduler.Tick(world);
    u32 count1 = 0;
    for (const auto& stim : world.Cognitive().frameStimuli)
    {
        if (stim.observerId == a1 && stim.sense == SenseType::Social)
            count1++;
    }
    ASSERT_TRUE(count1 > 0);

    // Second tick: cooldown should prevent re-emission
    scheduler.Tick(world);
    u32 count2 = 0;
    for (const auto& stim : world.Cognitive().frameStimuli)
    {
        if (stim.observerId == a1 && stim.sense == SenseType::Social)
            count2++;
    }
    ASSERT_EQ(count2, 0u);

    return true;
}

// === Test 10: Awareness cooldown blocks future tick underflow ===

TEST(awareness_cooldown_blocks_future_tick_underflow)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    auto& cooldown = world.AwarenessCooldown();
    cooldown.MarkEmitted(1, 2, ctx.concepts.groupDangerEvidence, 100);

    ASSERT_TRUE(!cooldown.CanEmit(1, 2, ctx.concepts.groupDangerEvidence, 50, 20));
    ASSERT_TRUE(cooldown.CanEmit(1, 2, ctx.concepts.groupDangerEvidence, 120, 20));

    return true;
}

// === Test 11: Awareness cooldown allows stimulus after window ===

TEST(awareness_cooldown_allows_stimulus_after_window)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    EntityId a1 = world.SpawnAgent(20, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto scheduler = CreateAwarenessOnlyScheduler(ctx);

    // First tick: emit
    scheduler.Tick(world);

    // Run until cooldown expires, checking each tick
    bool reEmitted = false;
    for (int i = 1; i <= 30; i++)
    {
        scheduler.Tick(world);
        for (const auto& stim : world.Cognitive().frameStimuli)
        {
            if (stim.observerId == a1 && stim.sense == SenseType::Social)
            {
                reEmitted = true;
                break;
            }
        }
        if (reEmitted)
            break;
    }
    ASSERT_TRUE(reEmitted);

    return true;
}

// === Test 12: Awareness cooldown changes full hash ===

TEST(awareness_cooldown_changes_full_hash)
{
    HumanEvolutionRulePack rp;
    WorldState world(64, 64, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(20, 20);
    AddDangerZone(world.GroupKnowledge(), ctx.groupKnowledge.sharedDangerZone,
                  {20, 20}, 5.0f, 0.8f, 0);

    auto scheduler = CreateAwarenessOnlyScheduler(ctx);

    // Run one tick to populate cooldown
    scheduler.Tick(world);

    // Get hash with cooldown populated
    u64 hashWith = ComputeWorldHash(world, HashTier::Full);

    // Manually inject different cooldown state
    auto& cooldown = world.AwarenessCooldown();
    AwarenessCooldownRecord extra;
    extra.agentId = 999;
    extra.sourceRecordId = 888;
    extra.concept = ctx.concepts.groupDangerEvidence;
    extra.lastEmittedTick = 42;
    cooldown.records.push_back(extra);

    u64 hashWithExtra = ComputeWorldHash(world, HashTier::Full);

    ASSERT_TRUE(hashWith != hashWithExtra);

    return true;
}

// === Test 13: Group knowledge awareness system in pipeline ===

TEST(group_knowledge_awareness_system_in_pipeline)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);

    auto systems = rp.CreateSystems();
    bool found = false;
    for (const auto& reg : systems)
    {
        auto desc = reg.system->Descriptor();
        if (std::string(desc.name) == "GroupKnowledgeAwarenessSystem")
        {
            ASSERT_TRUE(reg.phase == SimPhase::Perception);
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);

    return true;
}

TEST(group_knowledge_awareness_descriptor_has_single_cooldown_access)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    GroupKnowledgeAwarenessSystem system(ctx.concepts.groupDangerEvidence,
                                         ctx.groupKnowledge.sharedDangerZone);
    auto desc = system.Descriptor();

    u32 cooldownReads = 0;
    u32 cooldownReadWrites = 0;

    for (size_t i = 0; i < desc.readCount; i++)
    {
        if (desc.reads[i].module == ModuleTag::AwarenessCooldown)
            cooldownReads++;
    }

    for (size_t i = 0; i < desc.writeCount; i++)
    {
        if (desc.writes[i].module == ModuleTag::AwarenessCooldown &&
            desc.writes[i].mode == AccessMode::ReadWrite)
            cooldownReadWrites++;
    }

    ASSERT_EQ(cooldownReads, 0u);
    ASSERT_EQ(cooldownReadWrites, 1u);

    return true;
}
