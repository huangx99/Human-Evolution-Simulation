#pragma once

#include "core/types/types.h"
#include "sim/cognitive/concept_tag.h"
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
// === LIFECYCLE ===
//
// Formation: CognitiveDiscoverySystem detects co-occurrence in memories.
//   Initial state: Weak, confidence = 0.2, supportingCount = 1
//
// Reinforcement: each time the co-occurrence is observed again:
//   supportingCount++, confidence += 0.1 (capped at 1.0)
//
// Promotion to Stable: when confidence >= 0.6 AND supportingCount >= 3
//   → CognitiveKnowledgeSystem creates a KnowledgeEdge
//
// Contradiction: when cause is observed but effect is NOT observed:
//   contradictingCount++, confidence -= 0.1
//   If contradictingCount > supportingCount → status = Contradicted
//
// Contradicted hypotheses keep their KnowledgeEdge (wrong knowledge persists).
// This models how real beliefs survive even when contradicted.

struct Hypothesis
{
    u64 id = 0;

    EntityId ownerId = 0;
    u64 groupId = 0;

    ConceptTag causeConcept = ConceptTag::None;
    ConceptTag effectConcept = ConceptTag::None;
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

    ConceptTag causeConcept = ConceptTag::None;
    ConceptTag effectConcept = ConceptTag::None;
    KnowledgeRelation relation = KnowledgeRelation::AssociatedWith;

    f32 confidence = 0.0f;
    HypothesisStatus newStatus = HypothesisStatus::Weak;

    Tick tick = 0;
};
