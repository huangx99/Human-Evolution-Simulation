#pragma once

// CognitiveDiscoverySystem: detects patterns in memories and forms hypotheses.
//
// ARCHITECTURE NOTE: This system is RULE-DRIVEN hypothesis formation,
// NOT statistical pattern mining. It uses:
//   1. DiscoveryRule table: concept-pair → relation mapping
//   2. Result-tag matching: generic cause-effect from memory results
//   3. Co-occurrence: two concepts observed within maxTickGap ticks
//
// Phase 2 simplification: co-occurrence + rules is sufficient for
// basic hypothesis formation. Future phases should add:
//   - Statistical significance testing (chi-squared, mutual information)
//   - Temporal ordering (A before B vs B before A)
//   - Confound detection (C causes both A and B)
//   - Sample size requirements (don't form hypothesis from 2 observations)
//
// Hypotheses can be WRONG — this is by design. The system also detects
// contradictions: if a hypothesis predicts outcome X but the agent
// observes not-X, the hypothesis's contradiction count increases.
//
// Pipeline position: runs in SimPhase::Decision.
// Reads: CognitiveModule::agentMemories (recent episodic memories).
// Writes: CognitiveModule::agentHypotheses, frameDiscoveries.
//
// OWNERSHIP: Engine (sim/system/)
// READS: CognitiveModule (agentMemories), AgentModule (agents), SimulationModule (clock)
// WRITES: CognitiveModule (agentHypotheses, frameDiscoveries)
// PHASE: SimPhase::Decision

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/concept_id.h"
#include "sim/cognitive/knowledge_relation.h"
#include <vector>
#include <algorithm>
#include <cmath>

// DiscoveryRule: a configurable pattern for hypothesis formation.
//
// Each rule defines a pair of concepts and the relation between them.
// When two memories match (causeConcept, effectConcept), the system
// creates a hypothesis with the specified relation.
//
// To add a new cognitive pattern: add a DiscoveryRule. No system changes needed.
struct DiscoveryRule
{
    ConceptTypeId causeConcept;
    ConceptTypeId effectConcept;
    KnowledgeRelation relation;
};

class CognitiveDiscoverySystem : public ISystem
{
public:
    CognitiveDiscoverySystem() = default;

    explicit CognitiveDiscoverySystem(std::vector<DiscoveryRule> rules)
        : rules(std::move(rules)) {}

    // Allow RulePack to inject domain-specific discovery rules.
    // Called after concepts are registered (runtime ConceptTypeIds available).
    void SetRules(std::vector<DiscoveryRule> newRules)
    {
        rules = std::move(newRules);
    }

    void Update(SystemContext& ctx) override
    {
        auto& world = ctx.World();
        auto& cog = world.Cognitive();
        auto& sim = world.Sim();
        Tick now = sim.clock.currentTick;

        for (auto& agent : world.Agents().agents)
        {
            auto& memories = cog.GetAgentMemories(agent.id);
            if (memories.size() < 2) continue;

            // Look for co-occurrence patterns in recent memories
            for (size_t i = 0; i < memories.size(); i++)
            {
                auto& memA = memories[i];

                // Only look at episodic or strong short-term memories
                if (memA.kind == MemoryKind::ShortTerm && memA.strength < 0.3f)
                    continue;

                for (size_t j = i + 1; j < memories.size(); j++)
                {
                    auto& memB = memories[j];

                    // Skip if too far apart in time
                    // Consolidated memories preserve repeated evidence by updating
                    // lastReinforcedTick, so use evidence time rather than creation time.
                    if (std::abs(static_cast<i64>(EvidenceTick(memA)) -
                                 static_cast<i64>(EvidenceTick(memB))) > maxTickGap)
                        continue;

                    // Skip if same concept (no self-relation)
                    if (memA.subject == memB.subject) continue;

                    // Try to infer a relation between these two memories
                    KnowledgeRelation relation = InferRelation(memA, memB);
                    if (relation == KnowledgeRelation::AssociatedWith &&
                        !HasStrongAssociation(memA, memB))
                        continue;  // skip weak associations

                    // Check if a hypothesis already exists for this pair
                    auto& hypotheses = cog.GetAgentHypotheses(agent.id);
                    Hypothesis* existing = FindHypothesis(
                        hypotheses, memA.subject, memB.subject, relation);

                    const Tick pairEvidenceTick = PairEvidenceTick(memA, memB);
                    const u32 supportWeight = EvidenceSupport(memA, memB);
                    if (existing)
                    {
                        existing->lastObservedTick = now;
                        if (pairEvidenceTick <= existing->lastEvidenceTick)
                            continue;

                        // Consolidated memories represent repeated observations.
                        // Count their preserved evidence weight only when the pair has
                        // a newer evidence watermark, otherwise stale pairs would be
                        // re-counted every tick by the discovery scan.
                        existing->supportingCount += supportWeight;
                        existing->confidence = std::min(1.0f,
                            existing->confidence + confidenceIncrement * static_cast<f32>(supportWeight));
                        existing->lastEvidenceTick = pairEvidenceTick;
                        existing->UpdateStatus(stableThreshold, minEvidence);
                    }
                    else
                    {
                        // Create new hypothesis
                        Hypothesis hyp;
                        hyp.id = cog.nextHypothesisId++;
                        hyp.ownerId = agent.id;
                        hyp.causeConcept = memA.subject;
                        hyp.effectConcept = memB.subject;
                        hyp.proposedRelation = relation;
                        hyp.confidence = std::min(1.0f,
                            initialConfidence + confidenceIncrement * static_cast<f32>(supportWeight));
                        hyp.supportingCount = supportWeight;
                        hyp.firstObservedTick = now;
                        hyp.lastObservedTick = now;
                        hyp.lastEvidenceTick = pairEvidenceTick;
                        hyp.status = HypothesisStatus::Weak;
                        hyp.UpdateStatus(stableThreshold, minEvidence);

                        hypotheses.push_back(hyp);

                        // Record discovery
                        DiscoveryRecord disc;
                        disc.id = cog.nextDiscoveryId++;
                        disc.ownerId = agent.id;
                        disc.hypothesisId = hyp.id;
                        disc.causeConcept = hyp.causeConcept;
                        disc.effectConcept = hyp.effectConcept;
                        disc.relation = relation;
                        disc.confidence = hyp.confidence;
                        disc.newStatus = hyp.status;
                        disc.tick = now;

                        cog.frameDiscoveries.push_back(disc);

                        world.events.Emit({
                            EventType::CognitiveHypothesisFormed,
                            now, agent.id,
                            memA.location.x, memA.location.y,
                            hyp.confidence
                        });
                    }
                }
            }

            // Contradiction detection: check if any memory contradicts existing hypotheses
            DetectContradictions(agent.id, memories, cog, now, world);
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
        static const char* const DEPS[] = {"CognitiveMemorySystem"};
        return {"CognitiveDiscoverySystem", SimPhase::Decision, READS, 3, WRITES, 1, DEPS, 1, true, false};
    }

private:
    static Tick EvidenceTick(const MemoryRecord& memory)
    {
        return memory.lastReinforcedTick;
    }

    static Tick PairEvidenceTick(const MemoryRecord& a, const MemoryRecord& b)
    {
        return std::max(EvidenceTick(a), EvidenceTick(b));
    }

    static u32 EvidenceSupport(const MemoryRecord& a, const MemoryRecord& b)
    {
        // A candidate relation is itself evidence in addition to the
        // strongest consolidated memory backing either side of the pair.
        return std::max(a.reinforcementCount, b.reinforcementCount) + 2u;
    }

    static constexpr i64 maxTickGap = 50;
    static constexpr f32 initialConfidence = 0.2f;
    static constexpr f32 confidenceIncrement = 0.1f;
    static constexpr f32 stableThreshold = 0.6f;
    static constexpr u32 minEvidence = 3;
    static constexpr f32 contradictionConfidencePenalty = 0.1f;

    std::vector<DiscoveryRule> rules;

    // Infer relation using rules first, then result-tag matching as fallback.
    // Returns AssociatedWith if no specific relation is found — callers should
    // filter these out unless strong co-occurrence evidence exists.
    KnowledgeRelation InferRelation(const MemoryRecord& a, const MemoryRecord& b)
    {
        // Check specific rules first (A→B direction)
        for (const auto& rule : rules)
        {
            if (a.subject == rule.causeConcept && b.subject == rule.effectConcept)
                return rule.relation;
        }

        // Check rules in B→A direction
        for (const auto& rule : rules)
        {
            if (b.subject == rule.causeConcept && a.subject == rule.effectConcept)
                return rule.relation;
        }

        // Fallback: result-tag matching (generic cause-effect)
        for (auto tag : a.resultTags)
        {
            if (tag == b.subject)
                return KnowledgeRelation::Causes;
        }
        for (auto tag : b.resultTags)
        {
            if (tag == a.subject)
                return KnowledgeRelation::Causes;
        }

        // No specific relation found — return AssociatedWith.
        // The caller requires HasStrongAssociation() for these, which
        // filters out weak co-location noise.
        return KnowledgeRelation::AssociatedWith;
    }

    // Check if two memories are strongly associated (co-located, close in time)
    bool HasStrongAssociation(const MemoryRecord& a, const MemoryRecord& b)
    {
        if (a.location == b.location &&
            std::abs(static_cast<i64>(EvidenceTick(a)) - static_cast<i64>(EvidenceTick(b))) < 10)
            return true;

        for (auto tag : a.resultTags)
        {
            if (tag == b.subject) return true;
        }
        for (auto tag : b.resultTags)
        {
            if (tag == a.subject) return true;
        }

        return false;
    }

    // Contradiction detection: if a hypothesis predicts "A causes B" but
    // the agent observes A without B (or B without A), increment contradiction count.
    void DetectContradictions(EntityId agentId,
                               const std::vector<MemoryRecord>& memories,
                               CognitiveModule& cog, Tick now,
                               WorldState& world)
    {
        auto& hypotheses = cog.GetAgentHypotheses(agentId);
        if (hypotheses.empty()) return;

        // Check each hypothesis against recent memories
        for (auto& hyp : hypotheses)
        {
            // Only check hypotheses that predict a cause-effect relationship
            if (hyp.proposedRelation != KnowledgeRelation::Causes &&
                hyp.proposedRelation != KnowledgeRelation::Signals)
                continue;

            // Look for the cause concept in recent memories
            bool foundCause = false;
            bool foundEffect = false;
            for (const auto& mem : memories)
            {
                if (std::abs(static_cast<i64>(EvidenceTick(mem)) - static_cast<i64>(now)) > maxTickGap)
                    continue;

                if (mem.subject == hyp.causeConcept) foundCause = true;
                if (mem.subject == hyp.effectConcept) foundEffect = true;
            }

            // Contradiction: cause observed but effect NOT observed
            // (only if enough time has passed since hypothesis was last observed)
            if (foundCause && !foundEffect &&
                (now - hyp.lastObservedTick) > maxTickGap / 2)
            {
                hyp.contradictingCount++;
                hyp.confidence = std::max(0.0f,
                    hyp.confidence - contradictionConfidencePenalty);
                hyp.UpdateStatus(stableThreshold, minEvidence);
            }
        }
    }

    Hypothesis* FindHypothesis(std::vector<Hypothesis>& hypotheses,
                                ConceptTypeId cause, ConceptTypeId effect,
                                KnowledgeRelation relation)
    {
        for (auto& h : hypotheses)
        {
            if (h.causeConcept == cause &&
                h.effectConcept == effect &&
                h.proposedRelation == relation)
                return &h;
        }
        return nullptr;
    }
};
