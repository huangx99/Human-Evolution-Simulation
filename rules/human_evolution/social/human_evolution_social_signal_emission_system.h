#pragma once

// HumanEvolutionSocialSignalEmissionSystem: emits social signals when an agent
// perceives danger. Converts focused danger stimuli into broadcast fear signals.
//
// This is the bridge from individual perception to social communication.
// An agent that perceives danger warns nearby agents via a fear signal.
//
// OWNERSHIP: RulePack (rules/human_evolution/social/)
// READS: Cognitive (frameFocused), Agent, Simulation
// WRITES: Social (Emit)
// PHASE: SimPhase::Action (after AgentAction, signals available next tick)

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/cognitive/concept_tag.h"
#include <unordered_set>

class HumanEvolutionSocialSignalEmissionSystem : public ISystem
{
public:
    explicit HumanEvolutionSocialSignalEmissionSystem(
        const HumanEvolutionContext& ctx)
        : ctx_(ctx) {}

    void Update(SystemContext& sys) override
    {
        auto& agents = sys.Agents().agents;
        auto& cog = sys.Cognitive();
        auto& social = sys.SocialSignals();
        Tick now = sys.CurrentTick();

        // One fear signal per agent per tick
        std::unordered_set<EntityId> emittedThisTick;

        for (const auto& focused : cog.frameFocused)
        {
            const auto& stim = focused.stimulus;

            if (!IsDangerConcept(stim.concept))
                continue;

            // Find the agent who perceived this danger
            const Agent* sourceAgent = nullptr;
            for (const auto& agent : agents)
            {
                if (agent.id == stim.observerId && agent.alive)
                {
                    sourceAgent = &agent;
                    break;
                }
            }
            if (!sourceAgent)
                continue;

            // Skip if this agent already emitted fear this tick
            if (emittedThisTick.count(sourceAgent->id))
                continue;

            // Emit broadcast fear signal at agent's position
            SocialSignal sig;
            sig.typeId = ctx_.social.fear;
            sig.sourceEntityId = sourceAgent->id;
            sig.targetEntityId = 0;  // broadcast
            sig.origin = sourceAgent->position;
            sig.intensity = stim.intensity;
            sig.confidence = stim.confidence;
            sig.effectiveRadius = kFearRadius;
            sig.createdTick = now;
            sig.ttl = kFearTtl;

            social.Emit(sig);
            emittedThisTick.insert(sourceAgent->id);
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Cognitive, AccessMode::Read},
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Social, AccessMode::Write}
        };
        return {"HumanEvolutionSocialSignalEmissionSystem", SimPhase::Action,
                READS, 3, WRITES, 1, nullptr, 0, true, false};
    }

private:
    static constexpr f32 kFearRadius = 8.0f;
    static constexpr u32 kFearTtl = 10;

    const HumanEvolutionContext& ctx_;

    static bool IsDangerConcept(ConceptTag tag)
    {
        return tag == ConceptTag::Danger
            || tag == ConceptTag::Beast
            || tag == ConceptTag::Predator
            || tag == ConceptTag::Fire
            || tag == ConceptTag::Burning
            || tag == ConceptTag::Pain
            || tag == ConceptTag::Death
            || tag == ConceptTag::Drowning;
    }
};
