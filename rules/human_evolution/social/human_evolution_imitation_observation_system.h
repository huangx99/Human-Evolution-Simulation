#pragma once

// HumanEvolutionImitationObservationSystem: observes other agents' flee behavior
// and converts it into PerceivedStimulus (social danger evidence).
//
// This is the bridge from direct behavior observation to cognitive evidence.
// Agent B sees Agent A fleeing → B receives observed_flee stimulus.
//
// NOT a command system. NOT behavior copying.
// Only writes CognitiveModule.frameStimuli.
//
// Uses ctx_.observedActions.observedFlee (registered via RegisterObservedActions)
// instead of hardcoded ObservedActionKind enum.
//
// OWNERSHIP: RulePack (rules/human_evolution/social/)
// READS: Agent, Simulation
// WRITES: Cognitive (frameStimuli)
// PHASE: SimPhase::Perception (after SocialSignalPerceptionSystem)

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/social/observed_action.h"
#include "sim/entity/agent_action.h"
#include "sim/cognitive/concept_tag.h"
#include <cmath>
#include <algorithm>

class HumanEvolutionImitationObservationSystem : public ISystem
{
public:
    explicit HumanEvolutionImitationObservationSystem(
        const HumanEvolutionContext& ctx)
        : ctx_(ctx) {}

    void Update(SystemContext& sys) override
    {
        auto& agents = sys.Agents().agents;
        auto& cog = sys.Cognitive();
        Tick now = sys.CurrentTick();

        for (const auto& observer : agents)
        {
            if (!observer.alive)
                continue;

            f32 bestScore = 0.0f;
            ObservedAction bestObserved{};
            bool found = false;

            for (const auto& actor : agents)
            {
                if (!actor.alive)
                    continue;
                if (observer.id == actor.id)
                    continue;
                if (!IsFleeLikeAction(actor))
                    continue;

                f32 dx = static_cast<f32>(actor.position.x - observer.position.x);
                f32 dy = static_cast<f32>(actor.position.y - observer.position.y);
                f32 distance = std::sqrt(dx * dx + dy * dy);

                if (distance > kObserveRadius)
                    continue;

                f32 visibility = 1.0f - std::min(distance / kObserveRadius, 1.0f);
                if (visibility <= kMinVisibility)
                    continue;

                f32 intensity = kObservedFleeBaseIntensity * visibility;
                f32 confidence = kObservedFleeBaseConfidence * visibility;
                f32 score = intensity * confidence;

                if (score > bestScore)
                {
                    bestScore = score;
                    bestObserved.observerEntityId = observer.id;
                    bestObserved.actorEntityId = actor.id;
                    bestObserved.typeId = ctx_.observedActions.observedFlee;
                    bestObserved.observedTick = now;
                    bestObserved.origin = actor.position;
                    bestObserved.visibility = visibility;
                    bestObserved.distance = distance;
                    bestObserved.confidence = confidence;
                    found = true;
                }
            }

            if (found)
            {
                PerceivedStimulus stim;
                MapObservedToStimulus(bestObserved, ctx_.concepts.observedFlee, stim);
                stim.id = cog.nextStimulusId++;
                cog.frameStimuli.push_back(stim);
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::Write}
        };
        return {"HumanEvolutionImitationObservationSystem", SimPhase::Perception,
                READS, 2, WRITES, 1, nullptr, 0, true, false};
    }

private:
    static constexpr f32 kObserveRadius = 6.0f;
    static constexpr f32 kMinVisibility = 0.05f;
    static constexpr f32 kObservedFleeBaseIntensity = 0.65f;
    static constexpr f32 kObservedFleeBaseConfidence = 0.45f;

    const HumanEvolutionContext& ctx_;

    static bool IsFleeLikeAction(const Agent& agent)
    {
        return agent.currentAction == AgentAction::Flee;
    }

    static void MapObservedToStimulus(
        const ObservedAction& observed,
        ConceptTypeId concept,
        PerceivedStimulus& out)
    {
        out.observerId = observed.observerEntityId;
        out.sourceEntityId = observed.actorEntityId;
        out.sense = SenseType::Vision;
        out.concept = concept;
        out.location = observed.origin;
        out.intensity = kObservedFleeBaseIntensity * observed.visibility;
        out.confidence = kObservedFleeBaseConfidence * observed.visibility;
        out.distance = observed.distance;
        out.tick = observed.observedTick;
    }
};
