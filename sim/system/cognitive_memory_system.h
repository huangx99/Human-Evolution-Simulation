#pragma once

// CognitiveMemorySystem: converts focused stimuli into memory records.
//
// ARCHITECTURE NOTE: This system ONLY consumes FocusedStimulus, never raw
// WorldState. If a stimulus didn't pass through the attention bottleneck,
// it cannot become a memory. This is the hard filter that prevents
// omniscient agents.
//
// Pipeline position: runs AFTER CognitiveAttentionSystem (same SimPhase::Perception).
// Reads: CognitiveModule::frameFocused.
// Writes: CognitiveModule::frameMemories, agent persistent memories.
// Also: emits CognitiveMemoryFormed events.
//
// OWNERSHIP: Engine (sim/system/)
// READS: CognitiveModule (frameFocused), AgentModule (agents)
// WRITES: CognitiveModule (agentMemories, frameMemories)
// PHASE: SimPhase::Perception

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/concept_tag.h"

class CognitiveMemorySystem : public ISystem
{
public:
    void Update(SystemContext& ctx) override
    {
        auto& world = ctx.World();
        auto& cog = world.Cognitive();
        auto& sim = world.Sim();

        // Decay existing memories each tick
        cog.DecayAllMemories(shortTermDecayRate, longTermDecayRate);

        // Consolidate strong short-term memories to episodic
        cog.ConsolidateMemories(promotionThreshold);

        // Process focused stimuli into new memories
        for (auto& focused : cog.frameFocused)
        {
            auto& s = focused.stimulus;
            auto& agent = world.Agents();  // used for result inference

            MemoryRecord mem;
            mem.id = cog.nextMemoryId++;
            mem.ownerId = s.observerId;
            mem.kind = MemoryKind::ShortTerm;
            mem.subject = s.concept;
            mem.location = s.location;
            mem.strength = focused.attentionScore;  // attention determines memory strength
            mem.emotionalWeight = ComputeEmotionalWeight(s, world);
            mem.confidence = s.confidence;
            mem.createdTick = sim.clock.currentTick;
            mem.lastReinforcedTick = sim.clock.currentTick;
            mem.sourceStimulusId = s.id;

            // Infer result tags based on current agent state
            InferResultTags(mem, s, world);

            // Store in persistent memory
            cog.GetAgentMemories(s.observerId).push_back(mem);

            // Track in frame buffer
            cog.frameMemories.push_back(mem);

            // Emit event
            world.events.Emit({
                EventType::CognitiveMemoryFormed,
                sim.clock.currentTick,
                s.observerId,
                s.location.x, s.location.y,
                mem.strength
            });
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Cognitive, AccessMode::Read},
            {ModuleTag::Agent, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::Write}
        };
        static const char* const DEPS[] = {"CognitiveAttentionSystem"};
        return {"CognitiveMemorySystem", SimPhase::Perception, READS, 2, WRITES, 1, DEPS, 1, true, false};
    }

private:
    static constexpr f32 shortTermDecayRate = 0.98f;  // 2% per tick
    static constexpr f32 longTermDecayRate = 0.999f;   // 0.1% per tick
    static constexpr f32 promotionThreshold = 0.6f;    // strength to promote to episodic

    // Emotional weight: strong emotions make memories stickier.
    f32 ComputeEmotionalWeight(const PerceivedStimulus& s, const WorldState& world)
    {
        switch (s.concept)
        {
        case ConceptTag::Fire:
        case ConceptTag::Burning:
        case ConceptTag::Danger:
        case ConceptTag::Beast:
        case ConceptTag::Predator:
            return -0.5f * s.intensity;  // negative emotion, proportional to intensity

        case ConceptTag::Food:
        case ConceptTag::Satiety:
        case ConceptTag::Warmth:
            return 0.3f * s.intensity;   // positive emotion

        case ConceptTag::Pain:
        case ConceptTag::Death:
            return -0.8f;  // strongly negative

        case ConceptTag::Water:
            return 0.2f;   // mildly positive

        default:
            return 0.0f;
        }
    }

    // Infer what happened as a result of this perception.
    // Example: agent perceived fire → agent has Pain → result includes Pain.
    void InferResultTags(MemoryRecord& mem, const PerceivedStimulus& s,
                          WorldState& world)
    {
        // Find the agent
        auto* agent = world.Agents().Find(s.observerId);
        if (!agent) return;

        // If agent is hungry and perceived food, result is positive
        if (agent->hunger > 50.0f &&
            (s.concept == ConceptTag::Food || s.concept == ConceptTag::Meat))
        {
            mem.resultTags.push_back(ConceptTag::Satiety);
        }

        // If agent was near fire and took damage, result includes pain
        if (s.concept == ConceptTag::Fire && agent->localTemperature > 50.0f)
        {
            mem.resultTags.push_back(ConceptTag::Pain);
            mem.resultTags.push_back(ConceptTag::Burning);
        }

        // If agent is cold and found warmth, result is comfort
        if (agent->localTemperature < 10.0f && s.concept == ConceptTag::Warmth)
        {
            mem.resultTags.push_back(ConceptTag::Comfort);
        }
    }
};
