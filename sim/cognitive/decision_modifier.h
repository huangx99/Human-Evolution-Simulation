#pragma once

// DecisionModifier: knowledge-driven behavior bias.
//
// ARCHITECTURE NOTE: This replaces hand-written knowledge checks in
// AgentDecisionSystem. Instead of "if Fire→Danger then flee += X",
// the system queries the knowledge graph and produces generic modifiers.
//
// Each KnowledgeEdge can produce a DecisionModifier that biases the
// agent's action scoring. This scales with knowledge count — adding
// new knowledge types doesn't require touching DecisionSystem.
//
// Example: if agent knows "Fire →Causes→ Pain" with confidence=0.7,
// the system automatically produces: {flee_concept=Fire, magnitude=0.7}
// which boosts flee score when fire is perceived.

#include "core/types/types.h"
#include "sim/cognitive/concept_id.h"
#include "sim/cognitive/knowledge_relation.h"

enum class ModifierType : u8
{
    FleeBoost,      // knowledge of danger → boost flee for this concept
    ApproachBoost,  // knowledge of value → boost approach for this concept
    AlertBoost,     // knowledge of signal → boost alertness for this concept
    ValenceBias,    // direct valence override from knowledge
    ArousalBias,    // arousal modifier from knowledge
};

struct DecisionModifier
{
    ModifierType type = ModifierType::FleeBoost;
    ConceptTypeId triggerConcept = ConceptTypeId{};  // what to watch for
    f32 magnitude = 0.0f;                          // how strong the bias is
    f32 confidence = 0.0f;                         // based on knowledge edge confidence

    // Directional bias from knowledge
    f32 valenceBias = 0.0f;   // -1 to +1, added to computed valence
    f32 arousalBias = 0.0f;   // 0 to 1, added to computed arousal
};
