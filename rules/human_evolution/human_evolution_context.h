#pragma once

#include "sim/field/field_id.h"
#include "sim/runtime/rule_context.h"
#include "sim/history/history_id.h"
#include "sim/social/social_signal_id.h"
#include "sim/social/observed_action_id.h"
#include "sim/cognitive/concept_id.h"
#include "sim/group_knowledge/group_knowledge_id.h"

// HumanEvolutionContext: RulePack-specific context for the Human Evolution world.
//
// Organized by domain so systems only receive the sub-context they need.
// Adding new fields/config/tables only modifies the relevant sub-struct.

namespace HumanEvolution {

// Environment fields (temperature, fire, weather, information channels)
struct EnvironmentContext
{
    FieldIndex temperature;
    FieldIndex humidity;
    FieldIndex fire;
    FieldIndex windX;
    FieldIndex windY;
    FieldIndex smell;
    FieldIndex danger;
    FieldIndex smoke;
};

// History event type ids (registered via RegisterHistoryTypes)
struct HistoryContext
{
    HistoryTypeId firstStableFireUsage;
    HistoryTypeId massDeath;
};

// Social signal type ids (registered via RegisterSocialSignals)
struct SocialContext
{
    SocialSignalTypeId fear;
    SocialSignalTypeId deathWarning;
};

// Observed action type ids (registered via RegisterObservedActions)
struct ObservedActionContext
{
    ObservedActionTypeId observedFlee;
};

// Cognitive concept type ids (registered via RegisterConcepts)
// These are runtime ConceptTypeId values, not constexpr.
struct ConceptContext
{
    // Natural phenomena
    ConceptTypeId fire;
    ConceptTypeId water;
    ConceptTypeId wind;
    ConceptTypeId rain;
    ConceptTypeId lightning;
    ConceptTypeId darkness;
    ConceptTypeId light;
    ConceptTypeId smoke;
    ConceptTypeId ice;
    ConceptTypeId heat;
    ConceptTypeId cold;

    // Materials
    ConceptTypeId wood;
    ConceptTypeId stone;
    ConceptTypeId grass;
    ConceptTypeId meat;
    ConceptTypeId bone;
    ConceptTypeId ash;
    ConceptTypeId coal;
    ConceptTypeId fruit;

    // States / conditions
    ConceptTypeId warmth;
    ConceptTypeId wetness;
    ConceptTypeId dryness;
    ConceptTypeId hunger;
    ConceptTypeId pain;
    ConceptTypeId satiety;
    ConceptTypeId health;
    ConceptTypeId death;

    // Danger
    ConceptTypeId danger;
    ConceptTypeId beast;
    ConceptTypeId predator;
    ConceptTypeId fall;
    ConceptTypeId drowning;
    ConceptTypeId burning;

    // Opportunities
    ConceptTypeId food;
    ConceptTypeId shelter;
    ConceptTypeId tool;
    ConceptTypeId path;

    // Social
    ConceptTypeId companion;
    ConceptTypeId stranger;
    ConceptTypeId group;
    ConceptTypeId signal;
    ConceptTypeId voice;
    ConceptTypeId observedFlee;

    // Abstract
    ConceptTypeId safety;
    ConceptTypeId comfort;
    ConceptTypeId fear;
    ConceptTypeId curiosity;
    ConceptTypeId trust;
};

// Group knowledge type ids (registered via RegisterGroupKnowledgeTypes)
struct GroupKnowledgeContext
{
    GroupKnowledgeTypeId sharedDangerZone;
    GroupKnowledgeTypeId safePath;
    GroupKnowledgeTypeId resourceCluster;
};

// Future extensions (uncomment when needed):
// struct AgentContext { ... };
// struct CognitiveContext { ... };
// struct EcologyContext { ... };

} // namespace HumanEvolution

class HumanEvolutionContext : public IRuleContext
{
public:
    HumanEvolution::EnvironmentContext environment;
    HumanEvolution::HistoryContext history;
    HumanEvolution::SocialContext social;
    HumanEvolution::ObservedActionContext observedActions;
    HumanEvolution::ConceptContext concepts;
    HumanEvolution::GroupKnowledgeContext groupKnowledge;
};
