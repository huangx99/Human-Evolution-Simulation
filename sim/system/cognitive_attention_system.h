#pragma once

// CognitiveAttentionSystem: selects which stimuli get attended to.
//
// ARCHITECTURE NOTE: Attention is the key mechanism that prevents agents
// from becoming omniscient. Not all PerceivedStimulus enter Memory.
// Only the top N (by attention score) survive to become FocusedStimulus.
//
// The attention score is computed as:
//   strength × urgency × novelty × (1 + fearBias + hungerBias + knowledgeBias)
//
// Where knowledgeBias is driven by the agent's KnowledgeGraph:
//   - If agent knows "Fire → Causes → Pain", fire stimuli get boosted
//   - If agent knows "Smoke → Signals → Fire", smoke stimuli get boosted
//   - If agent knows "Food → Causes → Satiety", food stimuli get boosted
//
// This creates the feedback loop: Knowledge → Attention → Memory → Knowledge
//
// Pipeline position: runs AFTER CognitivePerceptionSystem (same SimPhase::Perception).
// Reads: CognitiveModule::frameStimuli, agent state, knowledgeGraph.
// Writes: CognitiveModule::frameFocused.
//
// OWNERSHIP: Engine (sim/system/)
// READS: CognitiveModule (frameStimuli)
// WRITES: CognitiveModule (frameFocused)
// PHASE: SimPhase::Perception

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/concept_registry.h"
#include <algorithm>
#include <cmath>

class CognitiveAttentionSystem : public ISystem
{
public:
    void Update(SystemContext& ctx) override
    {
        auto& world = ctx.World();
        auto& cog = world.Cognitive();

        // Group stimuli by observer
        std::unordered_map<EntityId, std::vector<PerceivedStimulus*>> byAgent;
        for (auto& s : cog.frameStimuli)
        {
            byAgent[s.observerId].push_back(&s);
        }

        for (auto& agent : world.Agents().agents)
        {
            auto it = byAgent.find(agent.id);
            if (it == byAgent.end()) continue;

            auto& stimuli = it->second;

            // Score each stimulus
            std::vector<FocusedStimulus> candidates;
            for (auto* s : stimuli)
            {
                f32 score = ComputeAttentionScore(*s, agent, world);
                if (score > attentionThreshold)
                {
                    FocusedStimulus fs;
                    fs.stimulus = *s;
                    fs.attentionScore = score;
                    candidates.push_back(fs);
                }
            }

            // Sort by score descending
            std::sort(candidates.begin(), candidates.end(),
                [](const FocusedStimulus& a, const FocusedStimulus& b)
                { return a.attentionScore > b.attentionScore; });

            // Keep top N (attention bottleneck)
            size_t maxFocus = std::min(candidates.size(), maxFocusedPerAgent);
            for (size_t i = 0; i < maxFocus; i++)
            {
                cog.frameFocused.push_back(candidates[i]);
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Cognitive, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::Write}
        };
        static const char* const DEPS[] = {"CognitivePerceptionSystem"};
        return {"CognitiveAttentionSystem", SimPhase::Perception, READS, 1, WRITES, 1, DEPS, 1, true, false};
    }

private:
    static constexpr f32 attentionThreshold = 0.1f;
    static constexpr size_t maxFocusedPerAgent = 4;

    // Attention score = signal strength × urgency × novelty × bias
    f32 ComputeAttentionScore(const PerceivedStimulus& s,
                               const Agent& agent,
                               const WorldState& world)
    {
        f32 strength = s.intensity;
        f32 urgency = ComputeUrgency(s, agent);
        f32 novelty = ComputeNovelty(s, agent, world);
        f32 fearBias = ComputeFearBias(s, agent, world);
        f32 hungerBias = ComputeHungerBias(s, agent);
        f32 knowledgeBias = ComputeKnowledgeBias(s, agent, world);

        return strength * urgency * novelty * (1.0f + fearBias + hungerBias + knowledgeBias);
    }

    // Urgency: how immediately important is this?
    // Uses semantic flags instead of domain-specific concept names.
    f32 ComputeUrgency(const PerceivedStimulus& s, const Agent& agent)
    {
        const auto& reg = ConceptTypeRegistry::Instance();

        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Danger))
            return 2.0f;

        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Resource) &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Organic))
            return 1.0f + agent.hunger / 100.0f;

        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Thermal))
        {
            if (reg.HasFlag(s.concept, ConceptSemanticFlag::Negative))
                return agent.localTemperature > 45.0f ? 1.8f : 1.0f;
            if (reg.HasFlag(s.concept, ConceptSemanticFlag::Positive))
                return agent.localTemperature < 5.0f ? 1.5f : 1.0f;
        }

        return 1.0f;
    }

    // Novelty: has the agent seen this before?
    f32 ComputeNovelty(const PerceivedStimulus& s,
                        const Agent& agent,
                        const WorldState& world)
    {
        const auto& memories = world.Cognitive().GetAgentMemories(agent.id);
        if (memories.empty()) return 1.5f;

        u32 matchCount = 0;
        for (const auto& mem : memories)
        {
            if (mem.subject == s.concept)
                matchCount++;
        }

        return 1.0f + 0.5f / (1.0f + static_cast<f32>(matchCount));
    }

    // Fear bias: dangerous stimuli get extra attention from fearful agents.
    // Dynamic: based on current health AND recent fear/pain memories.
    f32 ComputeFearBias(const PerceivedStimulus& s, const Agent& agent,
                         const WorldState& world)
    {
        const auto& reg = ConceptTypeRegistry::Instance();
        f32 bias = 0.0f;

        // Health-based fear: low health → more afraid of danger
        if (agent.health < 50.0f)
        {
            if (reg.HasFlag(s.concept, ConceptSemanticFlag::Danger))
                bias += 0.5f;
        }

        // Memory-based fear: recent pain/trauma memories amplify fear
        const auto& memories = world.Cognitive().GetAgentMemories(agent.id);
        f32 recentPain = 0.0f;
        Tick now = world.Sim().clock.currentTick;
        for (const auto& mem : memories)
        {
            if (reg.HasFlag(mem.subject, ConceptSemanticFlag::Internal) &&
                reg.HasFlag(mem.subject, ConceptSemanticFlag::Negative))
            {
                // Recent memories have more influence
                f32 recency = 1.0f / (1.0f + static_cast<f32>(now - mem.lastReinforcedTick) * 0.01f);
                recentPain += mem.strength * recency;
            }
        }
        if (recentPain > 0.1f)
        {
            // Pain memories amplify fear of danger stimuli
            if (reg.HasFlag(s.concept, ConceptSemanticFlag::Danger) ||
                reg.HasFlag(s.concept, ConceptSemanticFlag::Thermal))
            {
                bias += recentPain * 0.5f;
            }
        }

        return bias;
    }

    // Hunger bias: food gets extra attention when hungry
    f32 ComputeHungerBias(const PerceivedStimulus& s, const Agent& agent)
    {
        if (agent.hunger > 50.0f)
        {
            const auto& reg = ConceptTypeRegistry::Instance();
            if (reg.HasFlag(s.concept, ConceptSemanticFlag::Resource) &&
                reg.HasFlag(s.concept, ConceptSemanticFlag::Organic))
            {
                return agent.hunger / 200.0f;  // up to 50% bonus
            }
        }
        return 0.0f;
    }

    // Knowledge bias: agent's learned knowledge amplifies related stimuli.
    // This is the core of the Knowledge → Attention feedback loop.
    f32 ComputeKnowledgeBias(const PerceivedStimulus& s, const Agent& agent,
                              const WorldState& world)
    {
        const auto& cog = world.Cognitive();
        const auto& reg = ConceptTypeRegistry::Instance();
        f32 bias = 0.0f;

        // Check if agent knows this concept is dangerous
        f32 knownDanger = cog.GetKnownDanger(agent.id, s.concept);
        if (knownDanger > 0.0f)
        {
            // Knowledge of danger amplifies attention to danger-related stimuli
            if (reg.HasFlag(s.concept, ConceptSemanticFlag::Danger) ||
                reg.HasFlag(s.concept, ConceptSemanticFlag::Thermal))
            {
                bias += knownDanger * 0.3f;
            }
        }

        // Check knowledge links for specific concept pairs
        // These use the KnowledgeGraph which now stores ConceptTypeId
        // We need to find concept ids by checking what the agent knows

        // For smoke→fire knowledge boost: check if agent has any Signals edge
        // from a smoke-like concept to a fire-like concept
        // Since we can't hardcode concept names anymore, we check by semantic flags:
        // if the stimulus is environmental+danger and agent has Signals knowledge
        // to a danger concept, boost attention
        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Environmental) &&
            reg.HasFlag(s.concept, ConceptSemanticFlag::Danger))
        {
            // Check all outgoing knowledge edges from this concept
            auto edges = cog.knowledgeGraph.FindEdgesFrom(agent.id, 0, s.concept);
            for (const auto* e : edges)
            {
                if (e->relation == KnowledgeRelation::Signals)
                {
                    const auto* toNode = cog.knowledgeGraph.FindNodeById(e->toNodeId);
                    if (toNode && reg.HasFlag(toNode->concept, ConceptSemanticFlag::Danger))
                    {
                        bias += 0.4f;  // 40% bonus: "this signals danger"
                        break;
                    }
                }
            }
        }

        // For food→satiety knowledge boost
        if (reg.HasFlag(s.concept, ConceptSemanticFlag::Resource))
        {
            auto edges = cog.knowledgeGraph.FindEdgesFrom(agent.id, 0, s.concept);
            for (const auto* e : edges)
            {
                if (e->relation == KnowledgeRelation::Causes)
                {
                    const auto* toNode = cog.knowledgeGraph.FindNodeById(e->toNodeId);
                    if (toNode && reg.HasFlag(toNode->concept, ConceptSemanticFlag::Positive))
                    {
                        bias += 0.3f;  // 30% bonus: "this causes something good"
                        break;
                    }
                }
            }
        }

        return bias;
    }
};
