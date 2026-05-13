#pragma once

// HumanEvolutionSocialSignalPerceptionSystem: converts SocialSignals into
// PerceivedStimulus so they can enter the cognitive pipeline.
//
// This is the bridge between SocialSignalModule and CognitiveModule.
// SocialSignals are observable information, not commands.
// They flow through: SocialSignal → PerceivedStimulus → Attention → Memory.
//
// OWNERSHIP: RulePack (rules/human_evolution/social/)
// READS: Agent, Social, Simulation
// WRITES: Cognitive (frameStimuli)
// PHASE: SimPhase::Perception (after AgentPerceptionSystem)

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/cognitive/concept_tag.h"
#include <cmath>

class HumanEvolutionSocialSignalPerceptionSystem : public ISystem
{
public:
    explicit HumanEvolutionSocialSignalPerceptionSystem(
        const HumanEvolutionContext& ctx)
        : ctx_(ctx) {}

    void Update(SystemContext& sys) override
    {
        auto& agents = sys.Agents().agents;
        const auto& social = sys.SocialSignals();
        auto& cog = sys.Cognitive();
        Tick now = sys.CurrentTick();

        for (const auto& agent : agents)
        {
            if (!agent.alive)
                continue;

            auto nearbySignals = social.QueryNear(agent.position, kSocialSenseRadius);

            for (const SocialSignal* signal : nearbySignals)
            {
                if (!signal)
                    continue;

                // Don't perceive your own signals
                if (signal->sourceEntityId == agent.id)
                    continue;

                PerceivedStimulus stim;
                if (TryMapSignalToStimulus(agent, *signal, now, stim))
                {
                    stim.id = cog.nextStimulusId++;
                    cog.frameStimuli.push_back(stim);
                }
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Social, AccessMode::Read},
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::Write}
        };
        return {"HumanEvolutionSocialSignalPerceptionSystem", SimPhase::Perception,
                READS, 3, WRITES, 1, nullptr, 0, true, false};
    }

private:
    static constexpr f32 kSocialSenseRadius = 8.0f;
    static constexpr f32 kMinIntensity = 0.05f;

    const HumanEvolutionContext& ctx_;

    bool TryMapSignalToStimulus(
        const Agent& observer,
        const SocialSignal& signal,
        Tick now,
        PerceivedStimulus& out)
    {
        if (signal.effectiveRadius <= 0.0f)
            return false;

        f32 dx = static_cast<f32>(signal.origin.x - observer.position.x);
        f32 dy = static_cast<f32>(signal.origin.y - observer.position.y);
        f32 distance = std::sqrt(dx * dx + dy * dy);

        // Distance attenuation: actual range = min(observer cap, signal cap)
        f32 distanceFactor = 1.0f - std::min(distance / signal.effectiveRadius, 1.0f);
        f32 intensity = signal.intensity * distanceFactor;
        f32 confidence = signal.confidence * distanceFactor;

        if (intensity < kMinIntensity)
            return false;

        ConceptTag concept = MapSignalToConcept(signal.typeId);
        if (concept == ConceptTag::None)
            return false;

        out.observerId = observer.id;
        out.sourceEntityId = signal.sourceEntityId;
        out.sense = MapSignalToSense(signal.typeId);
        out.concept = concept;
        out.location = signal.origin;
        out.intensity = intensity;
        out.confidence = confidence;
        out.distance = distance;
        out.tick = now;

        return true;
    }

    ConceptTag MapSignalToConcept(SocialSignalTypeId typeId) const
    {
        if (typeId == ctx_.social.fear)
            return ConceptTag::Fear;
        if (typeId == ctx_.social.deathWarning)
            return ConceptTag::Death;
        return ConceptTag::None;
    }

    SenseType MapSignalToSense(SocialSignalTypeId typeId) const
    {
        // Centralized signal-to-sense mapping.
        // fear: "heard" as vocalization/behavioral cue
        // death_warning: "heard" as alarm/distress
        // Future signals (blood_trace, scent_trail) will map to Vision/Smell.
        if (typeId == ctx_.social.fear)
            return SenseType::Sound;
        if (typeId == ctx_.social.deathWarning)
            return SenseType::Sound;
        return SenseType::Sound;  // default fallback
    }
};
