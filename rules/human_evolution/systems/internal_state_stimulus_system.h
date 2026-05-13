#pragma once

// InternalStateStimulusSystem: converts agent internal state deltas
// into PerceivedStimulus (physiological perception).
//
// Health decrease → Pain stimulus
// Hunger decrease → Satiety stimulus
// Hunger increase above threshold → Hunger stimulus
//
// NOT a command system. NOT behavior attribution.
// Only writes CognitiveModule.frameStimuli and InternalStateBaselineModule.
//
// OWNERSHIP: RulePack (rules/human_evolution/systems/)
// READS: Agent, Simulation
// WRITES: Cognitive (frameStimuli), InternalStateBaseline
// PHASE: SimPhase::Perception (after ImitationObservation, before Attention)

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/concept_tag.h"
#include <algorithm>

class InternalStateStimulusSystem : public ISystem
{
public:
    void Update(SystemContext& sys) override
    {
        auto& agents = sys.Agents().agents;
        auto& cog = sys.Cognitive();
        auto& baselines = sys.InternalStateBaselines().baselines;
        Tick now = sys.CurrentTick();

        for (const auto& agent : agents)
        {
            if (!agent.alive)
                continue;

            auto it = baselines.find(agent.id);
            if (it == baselines.end())
            {
                // First tick: record baseline, no stimulus
                baselines[agent.id] = {agent.hunger, agent.health};
                continue;
            }

            auto& prev = it->second;
            f32 hungerDelta = agent.hunger - prev.hunger;
            f32 healthDelta = agent.health - prev.health;

            // Health decrease → Pain
            if (healthDelta < -kMinDelta)
            {
                EmitInternalStimulus(cog, agent, ConceptTag::Pain,
                    std::min(-healthDelta / 50.0f, 1.0f), now);
            }

            // Hunger decrease → Satiety
            if (hungerDelta < -kMinDelta)
            {
                EmitInternalStimulus(cog, agent, ConceptTag::Satiety,
                    std::min(-hungerDelta / 30.0f, 1.0f), now);
            }

            // Hunger increase above threshold → Hunger
            if (hungerDelta > kMinDelta && agent.hunger > kHungerThreshold)
            {
                EmitInternalStimulus(cog, agent, ConceptTag::Hunger,
                    std::min((agent.hunger - kHungerThreshold) / 50.0f, 1.0f), now);
            }

            // Update baseline
            prev = {agent.hunger, agent.health};
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::Write},
            {ModuleTag::InternalStateBaseline, AccessMode::ReadWrite}
        };
        return {"InternalStateStimulusSystem", SimPhase::Perception,
                READS, 2, WRITES, 2, nullptr, 0, true, false};
    }

private:
    static constexpr f32 kMinDelta = 0.01f;
    static constexpr f32 kHungerThreshold = 40.0f;

    static void EmitInternalStimulus(
        CognitiveModule& cog, const Agent& agent,
        ConceptTag concept, f32 intensity, Tick now)
    {
        PerceivedStimulus stim;
        stim.id = cog.nextStimulusId++;
        stim.observerId = agent.id;
        stim.sourceEntityId = agent.id;
        stim.sense = SenseType::Internal;
        stim.concept = concept;
        stim.location = agent.position;
        stim.intensity = intensity;
        stim.confidence = 0.9f;
        stim.distance = 0.0f;
        stim.tick = now;
        cog.frameStimuli.push_back(stim);
    }
};
