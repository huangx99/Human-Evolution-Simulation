#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/system/cognitive_attention_system.h"
#include "sim/system/cognitive_memory_system.h"
#include "sim/runtime/simulation_hash.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/systems/internal_state_stimulus_system.h"
#include "test_framework.h"

// Test 1: Health decrease produces Pain stimulus
TEST(health_decrease_produces_pain_stimulus)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);

    // First tick: establish baseline
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
    scheduler.Tick(world);

    // Manually decrease health (simulating damage)
    auto& agents = world.Agents().agents;
    agents[0].health -= 20.0f;

    // Second tick: should produce Pain stimulus
    world.Cognitive().ClearFrame();
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    bool found = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 1 && stim.concept == rp.GetHumanEvolutionContext().concepts.pain)
        {
            found = true;
            ASSERT_TRUE(stim.intensity > 0.0f);
            ASSERT_EQ(stim.sense, SenseType::Internal);
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// Test 2: Hunger decrease produces Satiety stimulus
TEST(hunger_decrease_produces_satiety_stimulus)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);

    // First tick: establish baseline
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
    scheduler.Tick(world);

    // Manually decrease hunger (simulating eating)
    auto& agents = world.Agents().agents;
    agents[0].hunger -= 15.0f;

    // Second tick: should produce Satiety stimulus
    world.Cognitive().ClearFrame();
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    bool found = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 1 && stim.concept == rp.GetHumanEvolutionContext().concepts.satiety)
        {
            found = true;
            ASSERT_TRUE(stim.intensity > 0.0f);
            ASSERT_EQ(stim.sense, SenseType::Internal);
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// Test 3: Hunger increase above threshold produces Hunger stimulus
TEST(hunger_increase_above_threshold_produces_hunger_stimulus)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);

    // First tick: establish baseline (hunger starts at 0)
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
    scheduler.Tick(world);

    // Increase hunger above threshold (40)
    auto& agents = world.Agents().agents;
    agents[0].hunger = 55.0f;

    // Second tick: should produce Hunger stimulus
    world.Cognitive().ClearFrame();
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    bool found = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 1 && stim.concept == rp.GetHumanEvolutionContext().concepts.hunger)
        {
            found = true;
            ASSERT_TRUE(stim.intensity > 0.0f);
            ASSERT_EQ(stim.sense, SenseType::Internal);
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// Test 4: All internal state stimuli have SenseType::Internal
TEST(internal_state_stimulus_has_internal_sense)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);

    // First tick: establish baseline
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
    scheduler.Tick(world);

    // Change both health and hunger
    auto& agents = world.Agents().agents;
    agents[0].health -= 10.0f;
    agents[0].hunger -= 10.0f;

    // Second tick
    world.Cognitive().ClearFrame();
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 1)
            ASSERT_EQ(stim.sense, SenseType::Internal);
    }

    return true;
}

// Test 5: Internal state stimulus does not create memory directly
TEST(internal_state_stimulus_does_not_create_memory_directly)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);

    // First tick: establish baseline
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
    scheduler.Tick(world);

    // Decrease health
    auto& agents = world.Agents().agents;
    agents[0].health -= 20.0f;

    // Second tick: only InternalStateStimulusSystem, no attention/memory
    world.Cognitive().ClearFrame();
    scheduler.Tick(world);

    auto& cog = world.Cognitive();

    // Should have Pain stimulus
    bool foundStimulus = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 1 && stim.concept == rp.GetHumanEvolutionContext().concepts.pain)
            foundStimulus = true;
    }
    ASSERT_TRUE(foundStimulus);

    // But no memory (attention + memory systems didn't run)
    auto& mems = cog.GetAgentMemories(1);
    ASSERT_EQ(mems.size(), 0);

    return true;
}

// Test 6: Internal state stimulus can enter memory via attention
TEST(internal_state_stimulus_can_enter_memory_via_attention)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);

    // First tick: establish baseline
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveMemorySystem>());
    scheduler.Tick(world);

    // Decrease health
    auto& agents = world.Agents().agents;
    agents[0].health -= 30.0f;

    // Second tick: full pipeline
    world.Cognitive().ClearFrame();
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    auto& mems = cog.GetAgentMemories(1);

    // Must find a Pain memory
    bool foundMemory = false;
    for (const auto& mem : mems)
    {
        if (mem.subject == rp.GetHumanEvolutionContext().concepts.pain)
        {
            foundMemory = true;
            ASSERT_TRUE(mem.sourceStimulusId > 0);
            ASSERT_EQ(mem.ownerId, 1);
        }
    }
    ASSERT_TRUE(foundMemory);

    return true;
}

// Test 7: Internal state baseline is included in FullHash
TEST(internal_state_baseline_in_full_hash)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);

    // Establish baseline
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
    scheduler.Tick(world);

    u64 coreBefore = ComputeWorldHash(world, HashTier::Core);
    u64 fullBefore = ComputeWorldHash(world, HashTier::Full);

    // Modify agent state (changes baseline on next tick)
    auto& agents = world.Agents().agents;
    agents[0].health -= 20.0f;
    agents[0].hunger += 10.0f;

    // Tick to update baseline
    scheduler.Tick(world);

    u64 coreAfter = ComputeWorldHash(world, HashTier::Core);
    u64 fullAfter = ComputeWorldHash(world, HashTier::Full);

    // Core changes because agent state changed
    ASSERT_TRUE(coreAfter != coreBefore);

    // Full also changes (baseline is included)
    ASSERT_TRUE(fullAfter != fullBefore);

    return true;
}

// Test 7b: Baseline ONLY changes FullHash, not CoreHash
TEST(baseline_only_changes_full_hash_not_core_hash)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);

    // Establish baseline
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
    scheduler.Tick(world);

    u64 coreBefore = ComputeWorldHash(world, HashTier::Core);
    u64 fullBefore = ComputeWorldHash(world, HashTier::Full);

    // Directly modify baseline WITHOUT changing agent state
    world.InternalStateBaselines().baselines[1] = {12.0f, 88.0f};

    u64 coreAfter = ComputeWorldHash(world, HashTier::Core);
    u64 fullAfter = ComputeWorldHash(world, HashTier::Full);

    // Core must NOT change (baseline is not in Core tier)
    ASSERT_EQ(coreBefore, coreAfter);

    // Full MUST change (baseline is in Full tier)
    ASSERT_TRUE(fullBefore != fullAfter);

    return true;
}

// Test 8: Same baseline produces same FullHash (deterministic replay)
TEST(internal_state_baseline_replay_deterministic)
{
    auto computeFullHash = [](u64 seed, f32 healthDelta) -> u64 {
        HumanEvolutionRulePack rp;
        WorldState world(16, 16, seed, rp);
        const auto& ctx = rp.GetHumanEvolutionContext();
        world.SpawnAgent(5, 5);

        Scheduler scheduler;
        scheduler.AddSystem(SimPhase::Perception, std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
        scheduler.Tick(world);

        world.Agents().agents[0].health -= healthDelta;
        scheduler.Tick(world);

        return ComputeWorldHash(world, HashTier::Full);
    };

    u64 hash1 = computeFullHash(42, 15.0f);
    u64 hash2 = computeFullHash(42, 15.0f);
    ASSERT_EQ(hash1, hash2);

    return true;
}

// Test 9: CreateSystems() includes InternalStateStimulusSystem in correct order
TEST(internal_state_system_in_pipeline)
{
    HumanEvolutionRulePack rp;
    auto systems = rp.CreateSystems();

    bool hasInternalState = false;
    bool hasImitation = false;
    bool hasAttention = false;

    i32 internalStateIdx = -1;
    i32 imitationIdx = -1;
    i32 attentionIdx = -1;

    for (i32 i = 0; i < static_cast<i32>(systems.size()); i++)
    {
        const char* name = systems[i].system->Descriptor().name;
        std::string n(name);

        if (n == "InternalStateStimulusSystem")
        {
            hasInternalState = true;
            internalStateIdx = i;
            ASSERT_TRUE(systems[i].phase == SimPhase::Perception);
        }
        else if (n == "HumanEvolutionImitationObservationSystem")
        {
            hasImitation = true;
            imitationIdx = i;
        }
        else if (n == "CognitiveAttentionSystem")
        {
            hasAttention = true;
            attentionIdx = i;
        }
    }

    ASSERT_TRUE(hasInternalState);
    ASSERT_TRUE(hasImitation);
    ASSERT_TRUE(hasAttention);

    // Verify order: Imitation → InternalState → Attention
    ASSERT_TRUE(imitationIdx < internalStateIdx);
    ASSERT_TRUE(internalStateIdx < attentionIdx);

    return true;
}

// Test 10: Small delta does not produce stimulus
TEST(small_delta_does_not_produce_stimulus)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);

    // First tick: establish baseline
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
    scheduler.Tick(world);

    // Tiny change (below kMinDelta = 0.01)
    auto& agents = world.Agents().agents;
    agents[0].health -= 0.005f;
    agents[0].hunger += 0.005f;

    // Second tick: should NOT produce any stimulus
    world.Cognitive().ClearFrame();
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    i32 count = 0;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 1 &&
            (stim.concept == rp.GetHumanEvolutionContext().concepts.pain ||
             stim.concept == rp.GetHumanEvolutionContext().concepts.satiety ||
             stim.concept == rp.GetHumanEvolutionContext().concepts.hunger))
        {
            count++;
        }
    }
    ASSERT_EQ(count, 0);

    return true;
}
