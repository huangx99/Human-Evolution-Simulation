#pragma once

// IMemoryInferencePolicy: injects domain-specific result-tag inference
// into the engine's CognitiveMemorySystem.
//
// The engine decides WHEN to infer (on focused stimulus → memory).
// The RulePack decides WHAT to infer (which result tags to attach).
//
// If no policy is injected, InferResultTags is skipped entirely.

#include "sim/cognitive/concept_id.h"
#include "sim/cognitive/perceived_stimulus.h"
#include <vector>

class IMemoryInferencePolicy
{
public:
    virtual ~IMemoryInferencePolicy() = default;

    // Given a focused stimulus and the observer's current state,
    // append result tags to outTags. Called once per focused stimulus
    // during memory formation.
    virtual void InferResultTags(
        const PerceivedStimulus& stimulus,
        f32 observerHunger,
        f32 observerTemperature,
        std::vector<ConceptTypeId>& outTags) const = 0;
};
