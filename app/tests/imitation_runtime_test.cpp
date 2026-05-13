#include "sim/world/world_state.h"
#include "sim/system/social_signal_decay_system.h"
#include "sim/scheduler/scheduler.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/social/human_evolution_imitation_observation_system.h"
#include "sim/entity/agent_action.h"
#include "test_framework.h"

// Test 1: Nearby agent observes flee → gets social danger stimulus
TEST(observed_flee_creates_social_danger_stimulus)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);   // Actor (will flee)
    world.SpawnAgent(6, 6);   // Observer (nearby)

    // Set actor to flee action
    auto& agents = world.Agents().agents;
    agents[0].currentAction = AgentAction::Flee;

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionImitationObservationSystem>(ctx));
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    bool found = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 2 && stim.sourceEntityId == 1)
        {
            found = true;
            ASSERT_TRUE(stim.id > 0);
            ASSERT_TRUE(stim.intensity > 0.0f);
            ASSERT_TRUE(stim.confidence > 0.0f);
            ASSERT_TRUE(stim.sense == SenseType::Vision);
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// Test 2: Far agent does not observe flee
TEST(far_agent_does_not_observe_flee)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(2, 2);    // Actor
    world.SpawnAgent(28, 28);  // Observer (far away)

    auto& agents = world.Agents().agents;
    agents[0].currentAction = AgentAction::Flee;

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionImitationObservationSystem>(ctx));
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    bool found = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 2 && stim.sourceEntityId == 1)
            found = true;
    }
    ASSERT_TRUE(!found);

    return true;
}

// Test 3: Agent does not observe self fleeing
TEST(agent_does_not_observe_self_flee)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);

    auto& agents = world.Agents().agents;
    agents[0].currentAction = AgentAction::Flee;

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionImitationObservationSystem>(ctx));
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    bool found = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 1 && stim.sourceEntityId == 1)
            found = true;
    }
    ASSERT_TRUE(!found);

    return true;
}

// Test 4: Imitation observation does not create command
TEST(observed_flee_does_not_create_command)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(6, 6);

    auto& agents = world.Agents().agents;
    agents[0].currentAction = AgentAction::Flee;

    auto historyBefore = world.commands.GetHistory().size();

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionImitationObservationSystem>(ctx));
    scheduler.Tick(world);

    ASSERT_EQ(world.commands.GetHistory().size(), historyBefore);

    return true;
}

// Test 5: Observed flee perception alone does not create memory
TEST(observed_flee_does_not_create_memory_directly)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(6, 6);

    auto& agents = world.Agents().agents;
    agents[0].currentAction = AgentAction::Flee;

    // Only run imitation observation (no attention, no memory)
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionImitationObservationSystem>(ctx));
    scheduler.Tick(world);

    auto& cog = world.Cognitive();

    // Should have perceived stimulus
    bool foundStimulus = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 2 && stim.sourceEntityId == 1)
        {
            foundStimulus = true;
            ASSERT_TRUE(stim.id > 0);
        }
    }
    ASSERT_TRUE(foundStimulus);

    // But no focused stimuli (attention didn't run)
    ASSERT_EQ(cog.frameFocused.size(), 0);

    // And no memories formed
    auto& mems = cog.GetAgentMemories(2);
    ASSERT_EQ(mems.size(), 0);

    return true;
}

// Test 6: Observed flee can enter attention
TEST(observed_flee_can_enter_attention)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(6, 6);

    auto& agents = world.Agents().agents;
    agents[0].currentAction = AgentAction::Flee;

    // Run imitation + attention (no other stimuli competing)
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionImitationObservationSystem>(ctx));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.Tick(world);

    auto& cog = world.Cognitive();

    // With no competing stimuli, observed_flee should win attention
    bool foundFocused = false;
    for (const auto& focused : cog.frameFocused)
    {
        if (focused.stimulus.observerId == 2 && focused.stimulus.concept == ConceptTag::ObservedFlee)
        {
            foundFocused = true;
            ASSERT_TRUE(focused.attentionScore > 0.0f);
        }
    }
    ASSERT_TRUE(foundFocused);

    return true;
}

// Test 7: Observed flee memory requires attention and has correct sourceStimulusId
TEST(observed_flee_memory_requires_attention)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(6, 6);

    auto& agents = world.Agents().agents;
    agents[0].currentAction = AgentAction::Flee;

    // Full pipeline: imitation → attention → memory
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionImitationObservationSystem>(ctx));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveMemorySystem>());
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    auto& mems = cog.GetAgentMemories(2);

    // Must find an ObservedFlee memory with valid sourceStimulusId
    bool foundMemory = false;
    for (const auto& mem : mems)
    {
        if (mem.subject == ConceptTag::ObservedFlee)
        {
            foundMemory = true;
            ASSERT_TRUE(mem.sourceStimulusId > 0);
            ASSERT_EQ(mem.ownerId, 2);
        }
    }
    ASSERT_TRUE(foundMemory);

    return true;
}

// Test 8: Each observer gets at most one observed_flee stimulus per tick
TEST(observer_gets_at_most_one_observed_flee_per_tick)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();

    world.SpawnAgent(5, 5);   // Actor A
    world.SpawnAgent(5, 6);   // Actor B (same row, close to observer)
    world.SpawnAgent(6, 7);   // Observer

    auto& agents = world.Agents().agents;
    agents[0].currentAction = AgentAction::Flee;  // A flees
    agents[1].currentAction = AgentAction::Flee;  // B flees

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionImitationObservationSystem>(ctx));
    scheduler.Tick(world);

    auto& cog = world.Cognitive();

    // Count observed_flee stimuli for observer (id=3)
    i32 count = 0;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 3 && stim.concept == ConceptTag::ObservedFlee)
            count++;
    }

    // Must be exactly 1: nearest/strongest wins, not zero, not both
    ASSERT_EQ(count, 1);

    return true;
}
