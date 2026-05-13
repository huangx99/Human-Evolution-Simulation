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
    f32 ComputeUrgency(const PerceivedStimulus& s, const Agent& agent)
    {
        switch (s.concept)
        {
        case ConceptTag::Fire:
        case ConceptTag::Danger:
        case ConceptTag::Burning:
            return 2.0f;

        case ConceptTag::Food:
            return 1.0f + agent.hunger / 100.0f;

        case ConceptTag::Heat:
            return agent.localTemperature > 45.0f ? 1.8f : 1.0f;

        case ConceptTag::Cold:
            return agent.localTemperature < 5.0f ? 1.5f : 1.0f;

        default:
            return 1.0f;
        }
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
        f32 bias = 0.0f;

        // Health-based fear: low health → more afraid of danger
        if (agent.health < 50.0f)
        {
            switch (s.concept)
            {
            case ConceptTag::Fire:
            case ConceptTag::Danger:
            case ConceptTag::Beast:
            case ConceptTag::Predator:
            case ConceptTag::Burning:
                bias += 0.5f;
            default:
                break;
            }
        }

        // Memory-based fear: recent pain/trauma memories amplify fear
        const auto& memories = world.Cognitive().GetAgentMemories(agent.id);
        f32 recentPain = 0.0f;
        Tick now = world.Sim().clock.currentTick;
        for (const auto& mem : memories)
        {
            if (mem.subject == ConceptTag::Pain || mem.subject == ConceptTag::Burning)
            {
                // Recent memories have more influence
                f32 recency = 1.0f / (1.0f + static_cast<f32>(now - mem.lastReinforcedTick) * 0.01f);
                recentPain += mem.strength * recency;
            }
        }
        if (recentPain > 0.1f)
        {
            // Pain memories amplify fear of danger stimuli
            switch (s.concept)
            {
            case ConceptTag::Fire:
            case ConceptTag::Heat:
            case ConceptTag::Burning:
            case ConceptTag::Danger:
                bias += recentPain * 0.5f;
            default:
                break;
            }
        }

        return bias;
    }

    // Hunger bias: food gets extra attention when hungry
    f32 ComputeHungerBias(const PerceivedStimulus& s, const Agent& agent)
    {
        if (agent.hunger > 50.0f)
        {
            switch (s.concept)
            {
            case ConceptTag::Food:
            case ConceptTag::Meat:
            case ConceptTag::Fruit:
                return agent.hunger / 200.0f;  // up to 50% bonus
            default:
                break;
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
        f32 bias = 0.0f;

        // Check if agent knows this concept is dangerous
        f32 knownDanger = cog.GetKnownDanger(agent.id, s.concept);
        if (knownDanger > 0.0f)
        {
            // Knowledge of danger amplifies attention to danger-related stimuli
            switch (s.concept)
            {
            case ConceptTag::Fire:
            case ConceptTag::Heat:
            case ConceptTag::Smoke:
            case ConceptTag::Burning:
            case ConceptTag::Danger:
            case ConceptTag::Beast:
            case ConceptTag::Predator:
                bias += knownDanger * 0.3f;  // up to 30% bonus per knowledge unit
            default:
                break;
            }
        }

        // Check if agent knows Smoke → Signals → Fire (smoke means fire nearby)
        if (s.concept == ConceptTag::Smoke)
        {
            if (cog.HasKnowledgeLink(agent.id, ConceptTag::Smoke,
                                     ConceptTag::Fire, KnowledgeRelation::Signals))
            {
                bias += 0.4f;  // 40% bonus: smoke is more interesting if you know it means fire
            }
        }

        // Check if agent knows Food → Causes → Satiety (food is valuable)
        if (s.concept == ConceptTag::Food || s.concept == ConceptTag::Meat ||
            s.concept == ConceptTag::Fruit)
        {
            if (cog.HasKnowledgeLink(agent.id, ConceptTag::Food,
                                     ConceptTag::Satiety, KnowledgeRelation::Causes))
            {
                bias += 0.3f;  // 30% bonus: food is more interesting if you know it satisfies hunger
            }
        }

        return bias;
    }
};
