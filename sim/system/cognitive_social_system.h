#pragma once

// Phase 2.1 stub.
// Temporarily excluded from all default pipelines during Phase 1.4
// Social Runtime Boundary Cleanup.
//
// Future: CognitiveSocialSystem -> ImitationObservationSystem
//
// This system emits social signals between nearby agents.
// It does NOT create memories directly — that requires the full
// SocialSignal → SocialAttention → FocusedSocialStimulus → Memory pipeline.
//
// NOTE: This code still references CognitiveModule::frameSocialSignals
// which was removed in Phase 1.4. It will need updating when re-enabled
// in Phase 2.1 to use SocialSignalModule instead.

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/concept_tag.h"
#include <cmath>

class CognitiveSocialSystem : public ISystem
{
public:
    void Update(SystemContext& ctx) override
    {
        auto& world = ctx.World();
        auto& cog = world.Cognitive();
        auto& sim = world.Sim();
        auto& agents = world.Agents().agents;
        Tick now = sim.clock.currentTick;

        for (size_t i = 0; i < agents.size(); i++)
        {
            for (size_t j = i + 1; j < agents.size(); j++)
            {
                auto& a = agents[i];
                auto& b = agents[j];

                f32 dx = static_cast<f32>(a.position.x - b.position.x);
                f32 dy = static_cast<f32>(a.position.y - b.position.y);
                f32 dist = std::sqrt(dx * dx + dy * dy);

                if (dist > socialRadius) continue;

                // Agent A observes Agent B
                EmitSignal(cog, a, b, dist, now);
                // Agent B observes Agent A
                EmitSignal(cog, b, a, dist, now);
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Cognitive, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::Write},
            {ModuleTag::Event, AccessMode::Write}
        };
        static const char* const DEPS[] = {"CognitiveKnowledgeSystem"};
        return {"CognitiveSocialSystem", SimPhase::Action, READS, 2, WRITES, 2, DEPS, 1, true, false};
    }

private:
    static constexpr f32 socialRadius = 5.0f;

    // Emit a social signal. Does NOT create memory — that requires
    // the full SocialSignal → Attention → Memory pipeline (not yet built).
    void EmitSignal(CognitiveModule& cog,
                    const Agent& observer,
                    const Agent& source,
                    f32 distance,
                    Tick now)
    {
        ConceptTag observedConcept = InferActionConcept(source.currentAction);
        if (observedConcept == ConceptTag::None) return;

        SocialSignal signal;
        signal.id = cog.nextSocialSignalId++;
        signal.sourceAgentId = source.id;
        signal.targetAgentId = observer.id;
        signal.type = SocialSignalType::Imitation;
        signal.concept = observedConcept;
        signal.intensity = 1.0f / (1.0f + distance * 0.2f);
        signal.reliability = 0.5f;
        signal.origin = source.position;
        signal.effectiveRadius = socialRadius;
        signal.tick = now;

        cog.frameSocialSignals.push_back(signal);
    }

    static ConceptTag InferActionConcept(AgentAction action)
    {
        switch (action)
        {
        case AgentAction::MoveToFood: return ConceptTag::Food;
        case AgentAction::Flee:       return ConceptTag::Danger;
        case AgentAction::Wander:     return ConceptTag::Path;
        default: return ConceptTag::None;
        }
    }
};
