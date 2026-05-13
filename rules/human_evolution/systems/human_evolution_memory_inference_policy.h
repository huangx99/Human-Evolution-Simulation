#pragma once

// HumanEvolutionMemoryInferencePolicy: domain-specific result-tag inference.
//
// Maps semantic flag combinations to domain concepts:
//   Hungry + Resource + Organic → Satiety
//   Danger + Thermal + hot agent → Pain, Burning
//   Positive + Thermal + cold agent → Comfort
//
// OWNERSHIP: RulePack (rules/human_evolution/systems/)

#include "sim/cognitive/memory_inference_policy.h"
#include "rules/human_evolution/human_evolution_context.h"

class HumanEvolutionMemoryInferencePolicy final : public IMemoryInferencePolicy
{
public:
    explicit HumanEvolutionMemoryInferencePolicy(
        const HumanEvolution::ConceptContext& concepts)
        : concepts_(concepts) {}

    void InferResultTags(
        const PerceivedStimulus& stimulus,
        f32 observerHunger,
        f32 observerTemperature,
        std::vector<ConceptTypeId>& outTags) const override
    {
        const auto& reg = ConceptTypeRegistry::Instance();

        // Hungry agent perceives edible resource → satiety
        if (observerHunger > 50.0f &&
            reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Resource) &&
            reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Organic))
        {
            outTags.push_back(concepts_.satiety);
        }

        // Near fire-like danger and hot → pain, burning
        if (reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Danger) &&
            reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Thermal) &&
            observerTemperature > 50.0f)
        {
            outTags.push_back(concepts_.pain);
            outTags.push_back(concepts_.burning);
        }

        // Cold agent finds warmth → comfort
        if (observerTemperature < 10.0f &&
            reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Positive) &&
            reg.HasFlag(stimulus.concept, ConceptSemanticFlag::Thermal))
        {
            outTags.push_back(concepts_.comfort);
        }
    }

private:
    const HumanEvolution::ConceptContext& concepts_;
};
