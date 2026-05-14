#pragma once

#include "sim/cognitive/attention_scoring_policy.h"
#include "sim/cognitive/concept_registry.h"
#include "sim/world/world_state.h"
#include "rules/human_evolution/human_evolution_context.h"
#include <algorithm>

struct HumanEvolutionAttentionConfig
{
    f32 dangerUrgency = 2.0f;
    f32 socialDangerUrgency = 1.6f;
    f32 thermalNegativeUrgency = 1.8f;
    f32 thermalPositiveUrgency = 1.5f;
    f32 hungerResourceUrgencyScale = 1.0f / 100.0f;
    f32 hungerFoodBiasScale = 1.0f / 200.0f;
    f32 healthDangerFearBias = 0.5f;
    f32 traumaFearBiasScale = 0.5f;
    f32 knownDangerBiasScale = 0.3f;
    f32 signalDangerBias = 0.4f;
    f32 positiveResourceBias = 0.3f;
    f32 maxRecentTrauma = 4.0f;
    f32 maxFearBias = 3.0f;
    f32 maxHungerBias = 2.0f;
    f32 maxKnowledgeBias = 3.0f;
};

class HumanEvolutionAttentionScoringPolicy final : public IAttentionScoringPolicy
{
public:
    explicit HumanEvolutionAttentionScoringPolicy(
        const HumanEvolutionContext& ctx,
        HumanEvolutionAttentionConfig config = {})
        : ctx_(ctx)
        , config_(config) {}

    f32 Score(const PerceivedStimulus& stimulus,
              const Agent& agent,
              const WorldState& world) const override
    {
        f32 strength = std::max(0.0f, stimulus.intensity);
        f32 urgency = ComputeUrgency(stimulus, agent);
        f32 novelty = ComputeNovelty(stimulus, agent, world);
        f32 fearBias = ComputeFearBias(stimulus, agent, world);
        f32 hungerBias = ComputeHungerBias(stimulus, agent);
        f32 knowledgeBias = ComputeKnowledgeBias(stimulus, agent, world);

        return strength * urgency * novelty *
            (1.0f + fearBias + hungerBias + knowledgeBias);
    }

private:
    const HumanEvolutionContext& ctx_;
    HumanEvolutionAttentionConfig config_;

    f32 ComputeUrgency(const PerceivedStimulus& stimulus, const Agent& agent) const
    {
        const auto& reg = ConceptTypeRegistry::Instance();

        if (stimulus.concept == ctx_.concepts.groupDangerEvidence ||
            stimulus.concept == ctx_.concepts.observedFlee)
            return config_.socialDangerUrgency;

        if (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Danger))
            return config_.dangerUrgency;

        if (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Resource) &&
            reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Organic))
            return 1.0f + std::max(0.0f, agent.hunger) * config_.hungerResourceUrgencyScale;

        if (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Thermal))
        {
            if (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Negative))
                return agent.localTemperature > 45.0f ? config_.thermalNegativeUrgency : 1.0f;
            if (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Positive))
                return agent.localTemperature < 5.0f ? config_.thermalPositiveUrgency : 1.0f;
        }

        return 1.0f;
    }

    f32 ComputeNovelty(const PerceivedStimulus& stimulus,
                       const Agent& agent,
                       const WorldState& world) const
    {
        const auto& memories = world.Cognitive().GetAgentMemories(agent.id);
        if (memories.empty()) return 1.5f;

        u32 matchCount = 0;
        for (const auto& mem : memories)
        {
            if (mem.subject == stimulus.concept)
                matchCount++;
        }

        return 1.0f + 0.5f / (1.0f + static_cast<f32>(matchCount));
    }

    f32 ComputeFearBias(const PerceivedStimulus& stimulus,
                        const Agent& agent,
                        const WorldState& world) const
    {
        const auto& reg = ConceptTypeRegistry::Instance();
        f32 bias = 0.0f;

        if (agent.health < 50.0f && reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Danger))
            bias += config_.healthDangerFearBias;

        f32 recentTrauma = 0.0f;
        Tick now = world.Sim().clock.currentTick;
        const auto& memories = world.Cognitive().GetAgentMemories(agent.id);
        for (const auto& mem : memories)
        {
            if (!reg.HasFlag(mem.subject, ConceptSemanticFlag::TraumaRelevant))
                continue;

            f32 age = 0.0f;
            if (mem.lastReinforcedTick <= now)
                age = static_cast<f32>(now - mem.lastReinforcedTick);
            f32 recency = 1.0f / (1.0f + age * 0.01f);
            recentTrauma += std::max(0.0f, mem.strength) * recency;
            if (recentTrauma >= config_.maxRecentTrauma)
            {
                recentTrauma = config_.maxRecentTrauma;
                break;
            }
        }

        if (recentTrauma > 0.1f &&
            (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Danger) ||
             reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Thermal)))
        {
            bias += recentTrauma * config_.traumaFearBiasScale;
        }

        return Clamp(bias, 0.0f, config_.maxFearBias);
    }

    f32 ComputeHungerBias(const PerceivedStimulus& stimulus, const Agent& agent) const
    {
        if (agent.hunger <= 50.0f)
            return 0.0f;

        const auto& reg = ConceptTypeRegistry::Instance();
        if (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Resource) &&
            reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Organic))
        {
            return Clamp(agent.hunger * config_.hungerFoodBiasScale,
                         0.0f, config_.maxHungerBias);
        }

        return 0.0f;
    }

    f32 ComputeKnowledgeBias(const PerceivedStimulus& stimulus,
                             const Agent& agent,
                             const WorldState& world) const
    {
        const auto& cog = world.Cognitive();
        const auto& reg = ConceptTypeRegistry::Instance();
        f32 bias = 0.0f;

        f32 knownDanger = cog.GetKnownDanger(agent.id, stimulus.concept);
        if (knownDanger > 0.0f &&
            (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Danger) ||
             reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Thermal)))
        {
            bias += knownDanger * config_.knownDangerBiasScale;
        }

        if (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Environmental) &&
            reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Danger))
        {
            auto edges = cog.knowledgeGraph.FindEdgesFrom(agent.id, 0, stimulus.concept);
            for (const auto* edge : edges)
            {
                if (edge->relation != KnowledgeRelation::Signals)
                    continue;
                const auto* toNode = cog.knowledgeGraph.FindNodeById(edge->toNodeId);
                if (toNode && reg.HasFlag(toNode->concept, ConceptSemanticFlag::Danger))
                {
                    bias += config_.signalDangerBias;
                    break;
                }
            }
        }

        if (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Resource))
        {
            auto edges = cog.knowledgeGraph.FindEdgesFrom(agent.id, 0, stimulus.concept);
            for (const auto* edge : edges)
            {
                if (edge->relation != KnowledgeRelation::Causes)
                    continue;
                const auto* toNode = cog.knowledgeGraph.FindNodeById(edge->toNodeId);
                if (toNode && reg.HasFlag(toNode->concept, ConceptSemanticFlag::Positive))
                {
                    bias += config_.positiveResourceBias;
                    break;
                }
            }
        }

        return Clamp(bias, 0.0f, config_.maxKnowledgeBias);
    }

    static f32 Clamp(f32 value, f32 minValue, f32 maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
};
