#pragma once

#include "core/types/types.h"
#include "sim/cognitive/concept_id.h"
#include "sim/cognitive/knowledge_relation.h"
#include <vector>

enum class HypothesisStatus : u8
{
    Weak,           // few observations, low confidence
    Stable,         // enough evidence, confidence threshold met
    Contradicted    // evidence against it found
};

// Hypothesis: an agent's guess about how concepts relate.
//
// ARCHITECTURE NOTE: A Hypothesis is a "possible pattern", NOT a fact.
// It represents an agent's belief that two concepts are related.
// Hypotheses can be WRONG — this is by design. Agents don't discover
// truth; they form beliefs from limited experience.
//
// === LIFECYCLE (exact rules) ===
//
// 1. FORMATION (CognitiveDiscoverySystem):
//    Two memories with related concepts observed within maxTickGap ticks.
//    Initial: status=Weak, confidence=0.2, supportingCount=1
//
// 2. REINFORCEMENT (CognitiveDiscoverySystem):
//    Each time the same co-occurrence is observed:
//      supportingCount += 1
//      confidence = min(1.0, confidence + 0.1)
//      lastObservedTick = now
//
// 3. PROMOTION Weak → Stable (CognitiveDiscoverySystem):
//    Condition: confidence >= 0.6 AND supportingCount >= 3
//    Effect: CognitiveKnowledgeSystem creates a KnowledgeEdge
//
// 4. CONTRADICTION (CognitiveDiscoverySystem):
//    When causeConcept is observed but effectConcept is NOT observed
//    (within maxTickGap/2 ticks of last observation):
//      contradictingCount += 1
//      confidence = max(0.0, confidence - 0.1)
//
// 5. DEMOTION Stable → Contradicted:
//    Condition: contradictingCount > supportingCount
//    Effect: status changes, but KnowledgeEdge PERSISTS
//    (wrong knowledge sticks — this models real beliefs)
//
// 6. WRONG KNOWLEDGE PERSISTS:
//    Contradicted hypotheses keep their KnowledgeEdge.
//    The edge's confidence may be low, but it doesn't disappear.
//    This models how superstitions, taboos, and misconceptions
//    survive in real cultures even when contradicted.

struct Hypothesis
{
    u64 id = 0;

    EntityId ownerId = 0;
    u64 groupId = 0;

    ConceptTypeId causeConcept = ConceptTypeId{};
    ConceptTypeId effectConcept = ConceptTypeId{};
    KnowledgeRelation proposedRelation = KnowledgeRelation::AssociatedWith;

    f32 confidence = 0.0f;      // 0..1
    u32 supportingCount = 0;    // observations that support this
    u32 contradictingCount = 0; // observations that contradict this

    Tick firstObservedTick = 0;
    Tick lastObservedTick = 0;

    HypothesisStatus status = HypothesisStatus::Weak;

    void UpdateStatus(f32 stableThreshold, u32 minEvidence)
    {
        if (contradictingCount > supportingCount)
        {
            status = HypothesisStatus::Contradicted;
        }
        else if (confidence >= stableThreshold &&
                 supportingCount >= minEvidence)
        {
            status = HypothesisStatus::Stable;
        }
        else
        {
            status = HypothesisStatus::Weak;
        }
    }
};

// DiscoveryRecord: a log entry when a hypothesis is first formed or promoted.
struct DiscoveryRecord
{
    u64 id = 0;

    EntityId ownerId = 0;
    u64 hypothesisId = 0;

    ConceptTypeId causeConcept = ConceptTypeId{};
    ConceptTypeId effectConcept = ConceptTypeId{};
    KnowledgeRelation relation = KnowledgeRelation::AssociatedWith;

    f32 confidence = 0.0f;
    HypothesisStatus newStatus = HypothesisStatus::Weak;

    Tick tick = 0;
};
