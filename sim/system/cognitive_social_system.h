#pragma once

// CognitiveSocialSystem: emits social signals between nearby agents.
//
// ARCHITECTURE NOTE: This system is a STUB. It only emits SocialSignal
// objects into frameSocialSignals. It does NOT create memories directly.
//
// Reason: "Memory only comes from FocusedStimulus" is a core invariant.
// If social signals should enter memory, they must go through the full
// pipeline: SocialSignal → SocialAttention → FocusedSocialStimulus → Memory.
// That pipeline does not exist yet in Phase 2.
//
// Future: when social cognition is implemented, social signals should be
// perceived as stimuli, compete for attention, and only then form memories.
// This preserves the attention bottleneck.
//
// Pipeline position: runs in SimPhase::Action.
// Reads: agent positions, agent actions.
// Writes: CognitiveModule::frameSocialSignals only.
//
// OWNERSHIP: Engine (sim/system/)
// READS: AgentModule (agents), CognitiveModule (knowledgeGraph)
// WRITES: CognitiveModule (frameSocialSignals) via EventBus
// PHASE: SimPhase::Action

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include "sim/cognitive/concept_tag.h"
#include <cmath>

class CognitiveSocialSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
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
