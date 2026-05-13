#pragma once

// CognitiveKnowledgeSystem: promotes stable hypotheses into knowledge graph edges.
//
// When a hypothesis reaches Stable status (enough evidence, high confidence),
// it becomes a KnowledgeEdge in the agent's knowledge graph. This is NOT
// "discovering truth" — it's "forming a belief that guides behavior."
//
// Knowledge edges can be contradicted later. If an agent believes
// "fire repels beast" but then sees a beast near fire, the edge's
// confidence drops. This is how knowledge evolves.
//
// Pipeline position: runs AFTER CognitiveDiscoverySystem (same SimPhase::Decision).
// Reads: CognitiveModule::agentHypotheses, frameDiscoveries.
// Writes: CognitiveModule::knowledgeGraph.
//
// OWNERSHIP: Engine (sim/system/)
// READS: CognitiveModule (agentHypotheses, frameDiscoveries), AgentModule (agents), SimulationModule (clock)
// WRITES: CognitiveModule (knowledgeGraph)
// PHASE: SimPhase::Decision

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"

class CognitiveKnowledgeSystem : public ISystem
{
public:
    void Update(SystemContext& ctx) override
    {
        auto& world = ctx.World();
        auto& cog = world.Cognitive();
        auto& sim = world.Sim();
        Tick now = sim.clock.currentTick;

        // Process recent discoveries: check if any hypothesis just became Stable
        for (auto& disc : cog.frameDiscoveries)
        {
            if (disc.newStatus != HypothesisStatus::Stable) continue;

            // Find the hypothesis
            auto& hypotheses = cog.GetAgentHypotheses(disc.ownerId);
            Hypothesis* hyp = nullptr;
            for (auto& h : hypotheses)
            {
                if (h.id == disc.hypothesisId)
                {
                    hyp = &h;
                    break;
                }
            }
            if (!hyp || hyp->status != HypothesisStatus::Stable) continue;

            // Create or reinforce knowledge edge
            PromoteToKnowledge(cog, *hyp, now);

            world.events.Emit({
                EventType::CognitiveKnowledgeUpdated,
                now, disc.ownerId,
                0, 0,
                hyp->confidence
            });
        }

        // Also scan all hypotheses for ones that transitioned to Stable
        // (in case discovery system updated them but didn't emit a discovery)
        for (auto& agent : world.Agents().agents)
        {
            auto& hypotheses = cog.GetAgentHypotheses(agent.id);
            for (auto& hyp : hypotheses)
            {
                if (hyp.status == HypothesisStatus::Stable &&
                    !HasKnowledgeEdge(cog, hyp))
                {
                    PromoteToKnowledge(cog, hyp, now);
                }
            }
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
            {ModuleTag::Cognitive, AccessMode::Write}
        };
        static const char* const DEPS[] = {"CognitiveDiscoverySystem"};
        return {"CognitiveKnowledgeSystem", SimPhase::Decision, READS, 3, WRITES, 1, DEPS, 1, true, false};
    }

private:
    void PromoteToKnowledge(CognitiveModule& cog, Hypothesis& hyp, Tick now)
    {
        auto& kg = cog.knowledgeGraph;

        // Get or create the cause node (capture ID by value — push_back invalidates refs)
        u64 causeId = kg.GetOrCreateNode(
            hyp.ownerId, hyp.groupId, hyp.causeConcept, now).id;
        auto* causeNode = kg.FindNodeById(causeId);
        causeNode->strength = std::min(1.0f, causeNode->strength + 0.1f);
        causeNode->lastUpdatedTick = now;

        // Get or create the effect node
        u64 effectId = kg.GetOrCreateNode(
            hyp.ownerId, hyp.groupId, hyp.effectConcept, now).id;
        auto* effectNode = kg.FindNodeById(effectId);
        effectNode->strength = std::min(1.0f, effectNode->strength + 0.1f);
        effectNode->lastUpdatedTick = now;

        // Get or create the edge
        auto& edge = kg.GetOrCreateEdge(
            causeId, effectId, hyp.proposedRelation, now);

        // Reinforce edge based on hypothesis confidence
        f32 delta = hyp.confidence * 0.2f;
        kg.ReinforceEdge(edge, delta, now);
    }

    // Check if this hypothesis already has a corresponding knowledge edge
    bool HasKnowledgeEdge(CognitiveModule& cog, const Hypothesis& hyp)
    {
        auto& kg = cog.knowledgeGraph;
        auto* causeNode = kg.FindNode(hyp.ownerId, hyp.groupId, hyp.causeConcept);
        auto* effectNode = kg.FindNode(hyp.ownerId, hyp.groupId, hyp.effectConcept);
        if (!causeNode || !effectNode) return false;

        return kg.FindEdge(causeNode->id, effectNode->id, hyp.proposedRelation) != nullptr;
    }
};
