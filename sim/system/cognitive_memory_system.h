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
// Also: emits CognitiveMemoryFormed or CognitiveMemoryReinforced events.
//
// OWNERSHIP: Engine (sim/system/)
// READS: CognitiveModule (frameFocused), AgentModule (agents)
// WRITES: CognitiveModule (agentMemories, frameMemories)
// PHASE: SimPhase::Perception

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/concept_registry.h"
#include "sim/cognitive/memory_inference_policy.h"
#include "sim/cognitive/attention_numerics.h"

class CognitiveMemorySystem : public ISystem
{
public:
    CognitiveMemorySystem() = default;

    explicit CognitiveMemorySystem(const IMemoryInferencePolicy* policy)
        : policy_(policy) {}

    CognitiveMemorySystem(const IMemoryInferencePolicy* policy,
                          MemoryNumericConfig numericConfig)
        : policy_(policy)
        , numericConfig_(numericConfig) {}

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

            MemoryRecord mem;
            mem.ownerId = s.observerId;
            mem.kind = MemoryKind::ShortTerm;
            mem.subject = s.concept;
            mem.location = s.location;
            mem.strength = MemoryNumerics::Sanitize(
                focused.attentionScore, numericConfig_);
            mem.emotionalWeight = ComputeEmotionalWeight(s, world);
            mem.confidence = s.confidence;
            mem.createdTick = sim.clock.currentTick;
            mem.lastReinforcedTick = sim.clock.currentTick;
            mem.sourceStimulusId = s.id;

            // Infer result tags via injected policy (domain-specific)
            if (policy_)
            {
                auto* agent = world.Agents().Find(s.observerId);
                if (agent)
                    policy_->InferResultTags(s, agent->hunger,
                                             agent->localTemperature,
                                             mem.resultTags);
            }

            auto& memories = cog.GetAgentMemories(s.observerId);
            MemoryRecord* stored = TryMergeInternalMemory(memories, mem, s, sim.clock.currentTick);
            const bool reinforced = stored != nullptr;

            if (!reinforced)
            {
                mem.id = cog.nextMemoryId++;
                memories.push_back(mem);
                stored = &memories.back();
            }

            // Track in frame buffer
            cog.frameMemories.push_back(*stored);

            // Emit event
            world.events.Emit({
                reinforced ? EventType::CognitiveMemoryReinforced
                           : EventType::CognitiveMemoryFormed,
                sim.clock.currentTick,
                s.observerId,
                s.location.x, s.location.y,
                stored->strength
            });
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::ReadWrite}
        };
        static const char* const DEPS[] = {"CognitiveAttentionSystem"};
        return {"CognitiveMemorySystem", SimPhase::Perception, READS, 1, WRITES, 1, DEPS, 1, true, false};
    }

private:
    static constexpr f32 shortTermDecayRate = 0.98f;  // 2% per tick
    static constexpr f32 longTermDecayRate = 0.999f;   // 0.1% per tick
    static constexpr f32 promotionThreshold = 0.6f;    // strength to promote to episodic

    MemoryNumericConfig numericConfig_;

    static constexpr Tick internalMergeWindowTicks = 10;

    MemoryRecord* TryMergeInternalMemory(std::vector<MemoryRecord>& memories,
                                         const MemoryRecord& incoming,
                                         const PerceivedStimulus& stimulus,
                                         Tick now)
    {
        if (stimulus.sense != SenseType::Internal)
            return nullptr;

        for (auto it = memories.rbegin(); it != memories.rend(); ++it)
        {
            auto& existing = *it;
            if (existing.ownerId != incoming.ownerId || existing.subject != incoming.subject)
                continue;
            if (existing.lastReinforcedTick > now)
                continue;
            if ((now - existing.lastReinforcedTick) > internalMergeWindowTicks)
                continue;

            existing.strength = std::max(existing.strength, incoming.strength);
            existing.emotionalWeight = incoming.emotionalWeight;
            existing.confidence = std::max(existing.confidence, incoming.confidence);
            existing.location = incoming.location;
            existing.lastReinforcedTick = now;
            existing.sourceStimulusId = incoming.sourceStimulusId;
            existing.reinforcementCount++;
            existing.resultTags = incoming.resultTags;
            return &existing;
        }

        return nullptr;
    }

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

    const IMemoryInferencePolicy* policy_ = nullptr;
};
