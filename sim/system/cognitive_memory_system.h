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
#include "sim/cognitive/concept_registry.h"

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
    // Uses semantic flags instead of domain-specific concept names.
    f32 ComputeEmotionalWeight(const PerceivedStimulus& s, const WorldState& world)
    {
        const auto& reg = ConceptTypeRegistry::Instance();

        // Danger/Threat → negative emotion
        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Danger) ||
            reg.HasFlag(s.concept, ConceptSemanticFlag::Threat))
        {
            return -0.5f * s.intensity;
        }

        // Internal + Negative (pain) → strongly negative
        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Internal) &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Negative))
        {
            return -0.8f;
        }

        // Positive resource (food, warmth) → positive emotion
        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Positive) &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Resource))
        {
            return 0.3f * s.intensity;
        }

        // Positive internal (satiety) → positive emotion
        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Internal) &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Positive))
        {
            return 0.3f * s.intensity;
        }

        // Environmental + Organic (water) → mildly positive
        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Environmental) &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Organic))
        {
            return 0.2f;
        }

        return 0.0f;
    }

    // Infer what happened as a result of this perception.
    // Uses semantic flags instead of domain-specific concept names.
    void InferResultTags(MemoryRecord& mem, const PerceivedStimulus& s,
                          WorldState& world)
    {
        const auto& reg = ConceptTypeRegistry::Instance();

        // Find the agent
        auto* agent = world.Agents().Find(s.observerId);
        if (!agent) return;

        // If agent is hungry and perceived edible resource, result is satiety
        if (agent->hunger > 50.0f &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Resource) &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Organic))
        {
            // Look up satiety concept from the registry
            auto satietyId = reg.FindByKey(MakeConceptKey("human_evolution.satiety"));
            if (satietyId) mem.resultTags.push_back(satietyId);
        }

        // If agent was near fire-like danger and took damage, result includes pain
        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Danger) &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Thermal) &&
            agent->localTemperature > 50.0f)
        {
            auto painId = reg.FindByKey(MakeConceptKey("human_evolution.pain"));
            auto burningId = reg.FindByKey(MakeConceptKey("human_evolution.burning"));
            if (painId) mem.resultTags.push_back(painId);
            if (burningId) mem.resultTags.push_back(burningId);
        }

        // If agent is cold and found warmth, result is comfort
        if (agent->localTemperature < 10.0f &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Positive) &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Thermal))
        {
            auto comfortId = reg.FindByKey(MakeConceptKey("human_evolution.comfort"));
            if (comfortId) mem.resultTags.push_back(comfortId);
        }
    }
};
