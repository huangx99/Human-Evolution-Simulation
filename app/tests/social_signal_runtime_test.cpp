#include "sim/world/world_state.h"
#include "sim/social/social_signal_module.h"
#include "sim/system/social_signal_decay_system.h"
#include "sim/scheduler/scheduler.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/social/human_evolution_social_signal_perception_system.h"
#include "rules/human_evolution/social/human_evolution_social_signal_emission_system.h"
#include "test_framework.h"

static HumanEvolutionRulePack g_rulePack2;

// Test helper: injects a pre-built FocusedStimulus into CognitiveModule during Decision phase.
// Runs before Action phase so emission system can see it.
class InjectFocusedStimulusSystem : public ISystem
{
public:
    InjectFocusedStimulusSystem(FocusedStimulus stim, EntityId agentId)
        : stim_(std::move(stim)), agentId_(agentId) {}

    void Update(SystemContext& sys) override
    {
        stim_.stimulus.observerId = agentId_;
        stim_.stimulus.tick = sys.CurrentTick();
        sys.Cognitive().frameFocused.push_back(stim_);
    }

    SystemDescriptor Descriptor() const override
    {
        return {"InjectFocusedStimulusSystem", SimPhase::Decision, nullptr, 0, nullptr, 0, nullptr, 0, true, false};
    }

private:
    FocusedStimulus stim_;
    EntityId agentId_;
};

// Test 1: SocialSignalDecaySystem expires old signals
TEST(social_signal_decay_system_expires_signals)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    auto& social = world.SocialSignals();

    // Emit signal with ttl=5 at tick 0
    SocialSignal sig;
    sig.typeId = SocialSignalTypeId(1);
    sig.origin = {5, 5};
    sig.createdTick = 0;
    sig.ttl = 5;
    social.Emit(sig);
    ASSERT_EQ(social.Count(), 1);

    // Run decay system for 4 ticks — signal should survive
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    for (i32 i = 0; i < 4; i++)
        scheduler.Tick(world);
    ASSERT_EQ(social.Count(), 1);

    // Tick 5: ClearExpired runs at tick 4 (before EndTick advances).
    // Tick 6: ClearExpired runs at tick 5 → 5 >= 0+5 → expires.
    scheduler.Tick(world);
    scheduler.Tick(world);
    ASSERT_EQ(social.Count(), 0);

    return true;
}

// Test 2: SocialSignalDecaySystem keeps unexpired signals
TEST(social_signal_decay_system_keeps_unexpired_signals)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    auto& social = world.SocialSignals();

    SocialSignal sig;
    sig.typeId = SocialSignalTypeId(1);
    sig.origin = {5, 5};
    sig.createdTick = 0;
    sig.ttl = 100;
    social.Emit(sig);

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    for (i32 i = 0; i < 50; i++)
        scheduler.Tick(world);

    ASSERT_EQ(social.Count(), 1);

    return true;
}

// Test 3: SocialSignalDecaySystem does not write CommandBuffer
TEST(social_signal_decay_system_does_not_write_command)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    auto& social = world.SocialSignals();

    SocialSignal sig;
    sig.typeId = SocialSignalTypeId(1);
    sig.origin = {5, 5};
    sig.createdTick = 0;
    sig.ttl = 1;
    social.Emit(sig);

    auto historyBefore = world.commands.GetHistory().size();

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    scheduler.Tick(world);

    ASSERT_EQ(world.commands.GetHistory().size(), historyBefore);

    return true;
}

// === Perception tests ===

// Test 4: Nearby agent perceives fear signal
TEST(nearby_agent_perceives_fear_signal)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    // Spawn two agents close together
    world.SpawnAgent(5, 5);  // Agent A (source)
    world.SpawnAgent(6, 6);  // Agent B (observer, distance ~1.4)

    // Emit fear signal at Agent A's position
    SocialSignal sig;
    sig.typeId = ctx.social.fear;
    sig.sourceEntityId = 1;
    sig.origin = {5, 5};
    sig.intensity = 1.0f;
    sig.confidence = 0.8f;
    sig.effectiveRadius = 8.0f;
    sig.createdTick = 0;
    sig.ttl = 10;
    social.Emit(sig);

    // Build scheduler with perception system
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(rp.GetHumanEvolutionContext()));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveMemorySystem>());

    scheduler.Tick(world);

    // Check that Agent B perceived a social stimulus
    auto& cog = world.Cognitive();
    bool found = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 2 && stim.concept == rp.GetHumanEvolutionContext().concepts.fear)
        {
            found = true;
            ASSERT_TRUE(stim.sourceEntityId == 1);
            ASSERT_TRUE(stim.intensity > 0.0f);
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// Test 5: Far agent does not perceive fear signal
TEST(far_agent_does_not_perceive_fear_signal)
{
    HumanEvolutionRulePack rp;
    WorldState world(32, 32, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    // Spawn agents far apart
    world.SpawnAgent(2, 2);   // Agent A (source)
    world.SpawnAgent(28, 28); // Agent B (observer, distance ~37)

    SocialSignal sig;
    sig.typeId = ctx.social.fear;
    sig.sourceEntityId = 1;
    sig.origin = {2, 2};
    sig.intensity = 1.0f;
    sig.confidence = 0.8f;
    sig.effectiveRadius = 8.0f;
    sig.createdTick = 0;
    sig.ttl = 10;
    social.Emit(sig);

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(rp.GetHumanEvolutionContext()));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveMemorySystem>());

    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    bool found = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 2 && stim.concept == rp.GetHumanEvolutionContext().concepts.fear)
            found = true;
    }
    ASSERT_TRUE(!found);

    return true;
}

// Test 6: Agent does not perceive its own signal
TEST(agent_does_not_perceive_own_signal)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    world.SpawnAgent(5, 5);

    SocialSignal sig;
    sig.typeId = ctx.social.fear;
    sig.sourceEntityId = 1;  // Same as observer
    sig.origin = {5, 5};
    sig.intensity = 1.0f;
    sig.confidence = 0.8f;
    sig.effectiveRadius = 8.0f;
    sig.createdTick = 0;
    sig.ttl = 10;
    social.Emit(sig);

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(rp.GetHumanEvolutionContext()));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveMemorySystem>());

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

// Test 7: Death warning signal becomes PerceivedStimulus
TEST(death_warning_signal_becomes_perceived_stimulus)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(6, 6);

    SocialSignal sig;
    sig.typeId = ctx.social.deathWarning;
    sig.sourceEntityId = 99;  // Dead agent
    sig.origin = {5, 5};
    sig.intensity = 1.0f;
    sig.confidence = 0.9f;
    sig.effectiveRadius = 8.0f;
    sig.createdTick = 0;
    sig.ttl = 10;
    social.Emit(sig);

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(rp.GetHumanEvolutionContext()));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveMemorySystem>());

    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    bool found = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 2 && stim.concept == rp.GetHumanEvolutionContext().concepts.death)
        {
            found = true;
            ASSERT_TRUE(stim.sourceEntityId == 99);
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// Test 8: Social signal perception alone does not create memory
TEST(social_signal_perception_does_not_create_memory)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(6, 6);

    SocialSignal sig;
    sig.typeId = ctx.social.fear;
    sig.sourceEntityId = 1;
    sig.origin = {5, 5};
    sig.intensity = 1.0f;
    sig.confidence = 0.8f;
    sig.effectiveRadius = 8.0f;
    sig.createdTick = 0;
    sig.ttl = 10;
    social.Emit(sig);

    // Only run decay + perception (no attention, no memory)
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(rp.GetHumanEvolutionContext()));
    scheduler.Tick(world);

    auto& cog = world.Cognitive();

    // Agent 2 should have perceived the fear stimulus
    bool foundStimulus = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 2 && stim.concept == rp.GetHumanEvolutionContext().concepts.fear)
        {
            foundStimulus = true;
            ASSERT_TRUE(stim.id > 0);  // Fix 2: unique id assigned
        }
    }
    ASSERT_TRUE(foundStimulus);

    // But no focused stimuli (attention didn't run)
    ASSERT_EQ(cog.frameFocused.size(), 0);

    // And no memories formed (memory system didn't run)
    auto& mems = cog.GetAgentMemories(2);
    ASSERT_EQ(mems.size(), 0);

    return true;
}

// === Emission tests ===

// Test 9: Agent with focused danger stimulus emits fear signal
TEST(emission_focused_danger_emits_fear)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    world.SpawnAgent(5, 5);

    // Build focused danger stimulus to inject during Decision phase
    FocusedStimulus focused;
    focused.stimulus.sense = SenseType::Internal;
    focused.stimulus.concept = rp.GetHumanEvolutionContext().concepts.danger;
    focused.stimulus.location = {5, 5};
    focused.stimulus.intensity = 0.8f;
    focused.stimulus.confidence = 0.6f;
    focused.stimulus.distance = 0.0f;
    focused.attentionScore = 1.0f;

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Decision, std::make_unique<InjectFocusedStimulusSystem>(focused, 1));
    scheduler.AddSystem(SimPhase::Action, std::make_unique<HumanEvolutionSocialSignalEmissionSystem>(ctx));
    scheduler.Tick(world);

    ASSERT_EQ(social.Count(), 1);
    if (social.Count() > 0)
    {
        const auto& sig = social.activeSignals[0];
        ASSERT_TRUE(sig.typeId.index == ctx.social.fear.index);
        ASSERT_TRUE(sig.sourceEntityId == 1);
        ASSERT_TRUE(sig.origin.x == 5 && sig.origin.y == 5);
    }

    return true;
}

// Test 10: Agent without focused danger does not emit
TEST(emission_no_danger_no_signal)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    world.SpawnAgent(5, 5);

    // Non-danger focused stimulus (food) should not trigger emission
    FocusedStimulus focused;
    focused.stimulus.sense = SenseType::Smell;
    focused.stimulus.concept = rp.GetHumanEvolutionContext().concepts.food;
    focused.stimulus.location = {5, 5};
    focused.stimulus.intensity = 0.5f;
    focused.stimulus.confidence = 0.7f;
    focused.stimulus.distance = 1.0f;
    focused.attentionScore = 0.8f;

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Decision, std::make_unique<InjectFocusedStimulusSystem>(focused, 1));
    scheduler.AddSystem(SimPhase::Action, std::make_unique<HumanEvolutionSocialSignalEmissionSystem>(ctx));
    scheduler.Tick(world);

    ASSERT_EQ(social.Count(), 0);

    return true;
}

// Test 11: Emitted fear signal is perceivable by nearby agent (full chain)
TEST(emission_fear_signal_perceivable_by_nearby)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    // Agent A at (5,5) will emit fear; Agent B at (6,6) should perceive it
    world.SpawnAgent(5, 5);
    world.SpawnAgent(6, 6);

    // Build focused danger stimulus for Agent A
    FocusedStimulus focused;
    focused.stimulus.sense = SenseType::Internal;
    focused.stimulus.concept = rp.GetHumanEvolutionContext().concepts.danger;
    focused.stimulus.location = {5, 5};
    focused.stimulus.intensity = 0.9f;
    focused.stimulus.confidence = 0.7f;
    focused.stimulus.distance = 0.0f;
    focused.attentionScore = 1.0f;

    // Build full scheduler: inject danger → emit fear → (next tick) decay + perceive
    Scheduler scheduler;
    // Tick 1: inject danger + emit fear signal
    scheduler.AddSystem(SimPhase::Decision, std::make_unique<InjectFocusedStimulusSystem>(focused, 1));
    scheduler.AddSystem(SimPhase::Action, std::make_unique<HumanEvolutionSocialSignalEmissionSystem>(ctx));
    scheduler.Tick(world);

    // Signal should exist now
    ASSERT_EQ(social.Count(), 1);

    // Tick 2: signal decay + perception (one-tick delay)
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(ctx));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveMemorySystem>());
    scheduler.Tick(world);

    // Agent B should have perceived a Fear stimulus from Agent A
    auto& cog = world.Cognitive();
    bool found = false;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 2 && stim.concept == rp.GetHumanEvolutionContext().concepts.fear)
        {
            found = true;
            ASSERT_TRUE(stim.sourceEntityId == 1);
            ASSERT_TRUE(stim.intensity > 0.0f);
        }
    }
    ASSERT_TRUE(found);

    return true;
}

// Test 12: Full RulePack pipeline wiring verification
// Verifies that HumanEvolutionRulePack::CreateSystems() includes all social systems
// and they execute in the correct phase order.
TEST(full_pipeline_social_systems_present)
{
    HumanEvolutionRulePack rp;
    auto systems = rp.CreateSystems();

    bool hasDecay = false;
    bool hasPerception = false;
    bool hasEmission = false;

    for (const auto& reg : systems)
    {
        const char* name = reg.system->Descriptor().name;
        if (std::string(name) == "SocialSignalDecaySystem")
        {
            hasDecay = true;
            ASSERT_TRUE(reg.phase == SimPhase::Propagation);
        }
        else if (std::string(name) == "HumanEvolutionSocialSignalPerceptionSystem")
        {
            hasPerception = true;
            ASSERT_TRUE(reg.phase == SimPhase::Perception);
        }
        else if (std::string(name) == "HumanEvolutionSocialSignalEmissionSystem")
        {
            hasEmission = true;
            ASSERT_TRUE(reg.phase == SimPhase::Action);
        }
    }

    ASSERT_TRUE(hasDecay);
    ASSERT_TRUE(hasPerception);
    ASSERT_TRUE(hasEmission);

    return true;
}

// Test 13: Social signal perception assigns unique stimulus ids
TEST(social_signal_stimulus_gets_unique_id)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(6, 6);
    world.SpawnAgent(7, 7);

    // Emit two signals at different positions
    SocialSignal sig1;
    sig1.typeId = ctx.social.fear;
    sig1.sourceEntityId = 1;
    sig1.origin = {5, 5};
    sig1.intensity = 0.9f;
    sig1.confidence = 0.8f;
    sig1.effectiveRadius = 8.0f;
    sig1.createdTick = 0;
    sig1.ttl = 10;
    social.Emit(sig1);

    SocialSignal sig2;
    sig2.typeId = ctx.social.fear;
    sig2.sourceEntityId = 2;
    sig2.origin = {6, 6};
    sig2.intensity = 0.7f;
    sig2.confidence = 0.6f;
    sig2.effectiveRadius = 8.0f;
    sig2.createdTick = 0;
    sig2.ttl = 10;
    social.Emit(sig2);

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(ctx));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveMemorySystem>());
    scheduler.Tick(world);

    auto& cog = world.Cognitive();

    // Find social stimuli for agent 3 (perceives both signals)
    std::vector<u64> socialStimIds;
    for (const auto& stim : cog.frameStimuli)
    {
        if (stim.observerId == 3 && stim.concept == rp.GetHumanEvolutionContext().concepts.fear)
            socialStimIds.push_back(stim.id);
    }

    // Should have perceived 2 signals with unique ids
    ASSERT_EQ(socialStimIds.size(), 2);
    ASSERT_TRUE(socialStimIds[0] != socialStimIds[1]);
    ASSERT_TRUE(socialStimIds[0] > 0);
    ASSERT_TRUE(socialStimIds[1] > 0);

    return true;
}

// Test 14: Social signal memory records non-zero sourceStimulusId
TEST(social_memory_records_source_stimulus_id)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(6, 6);

    // Emit a strong fear signal
    SocialSignal sig;
    sig.typeId = ctx.social.fear;
    sig.sourceEntityId = 1;
    sig.origin = {5, 5};
    sig.intensity = 1.0f;
    sig.confidence = 0.9f;
    sig.effectiveRadius = 8.0f;
    sig.createdTick = 0;
    sig.ttl = 10;
    social.Emit(sig);

    // Full cognitive pipeline: perception → attention → memory
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(ctx));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitiveMemorySystem>());
    scheduler.Tick(world);

    auto& cog = world.Cognitive();
    auto& mems = cog.GetAgentMemories(2);

    // If a memory formed from the social stimulus, its sourceStimulusId should be non-zero
    for (const auto& mem : mems)
    {
        if (mem.subject == rp.GetHumanEvolutionContext().concepts.fear || mem.subject == rp.GetHumanEvolutionContext().concepts.danger)
        {
            ASSERT_TRUE(mem.sourceStimulusId > 0);
        }
    }

    return true;
}

// Test 15: Same agent emits at most one fear signal per tick
TEST(same_agent_emits_one_fear_signal_per_tick)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    world.SpawnAgent(5, 5);

    // Inject multiple danger focused stimuli for the same agent
    FocusedStimulus danger1;
    danger1.stimulus.sense = SenseType::Internal;
    danger1.stimulus.concept = rp.GetHumanEvolutionContext().concepts.danger;
    danger1.stimulus.location = {5, 5};
    danger1.stimulus.intensity = 0.8f;
    danger1.stimulus.confidence = 0.6f;
    danger1.stimulus.distance = 0.0f;
    danger1.attentionScore = 1.0f;

    FocusedStimulus fire;
    fire.stimulus.sense = SenseType::Thermal;
    fire.stimulus.concept = rp.GetHumanEvolutionContext().concepts.fire;
    fire.stimulus.location = {5, 5};
    fire.stimulus.intensity = 0.9f;
    fire.stimulus.confidence = 0.7f;
    fire.stimulus.distance = 0.0f;
    fire.attentionScore = 0.9f;

    FocusedStimulus pain;
    pain.stimulus.sense = SenseType::Touch;
    pain.stimulus.concept = rp.GetHumanEvolutionContext().concepts.pain;
    pain.stimulus.location = {5, 5};
    pain.stimulus.intensity = 0.7f;
    pain.stimulus.confidence = 0.5f;
    pain.stimulus.distance = 0.0f;
    pain.attentionScore = 0.8f;

    // Inject all three in Decision phase, then emit in Action phase
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Decision, std::make_unique<InjectFocusedStimulusSystem>(danger1, 1));
    scheduler.AddSystem(SimPhase::Decision, std::make_unique<InjectFocusedStimulusSystem>(fire, 1));
    scheduler.AddSystem(SimPhase::Decision, std::make_unique<InjectFocusedStimulusSystem>(pain, 1));
    scheduler.AddSystem(SimPhase::Action, std::make_unique<HumanEvolutionSocialSignalEmissionSystem>(ctx));
    scheduler.Tick(world);

    // Should only emit 1 fear signal despite 3 danger concepts
    ASSERT_EQ(social.Count(), 1);

    return true;
}

TEST(fear_signal_emission_requires_trauma_relevant_concept)
{
    HumanEvolutionRulePack rp;
    WorldState world(16, 16, 42, rp);
    const auto& ctx = rp.GetHumanEvolutionContext();
    auto& social = world.SocialSignals();

    world.SpawnAgent(5, 5);

    FocusedStimulus cold;
    cold.stimulus.sense = SenseType::Thermal;
    cold.stimulus.concept = ctx.concepts.cold;
    cold.stimulus.location = {5, 5};
    cold.stimulus.intensity = 0.9f;
    cold.stimulus.confidence = 0.7f;
    cold.attentionScore = 1.0f;

    FocusedStimulus pain;
    pain.stimulus.sense = SenseType::Touch;
    pain.stimulus.concept = ctx.concepts.pain;
    pain.stimulus.location = {5, 5};
    pain.stimulus.intensity = 0.9f;
    pain.stimulus.confidence = 0.7f;
    pain.attentionScore = 1.0f;

    Scheduler coldScheduler;
    coldScheduler.AddSystem(SimPhase::Decision, std::make_unique<InjectFocusedStimulusSystem>(cold, 1));
    coldScheduler.AddSystem(SimPhase::Action, std::make_unique<HumanEvolutionSocialSignalEmissionSystem>(ctx));
    coldScheduler.Tick(world);

    ASSERT_EQ(social.Count(), 0u);

    Scheduler painScheduler;
    painScheduler.AddSystem(SimPhase::Decision, std::make_unique<InjectFocusedStimulusSystem>(pain, 1));
    painScheduler.AddSystem(SimPhase::Action, std::make_unique<HumanEvolutionSocialSignalEmissionSystem>(ctx));
    painScheduler.Tick(world);

    ASSERT_EQ(social.Count(), 1u);

    return true;
}
