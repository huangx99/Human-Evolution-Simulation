#pragma once

#include "sim/field/field_id.h"
#include "sim/runtime/rule_context.h"
#include "sim/history/history_id.h"
#include "sim/social/social_signal_id.h"
#include "sim/social/observed_action_id.h"
#include "sim/cognitive/concept_id.h"
#include "sim/group_knowledge/group_knowledge_id.h"
#include "sim/pattern/pattern_id.h"
#include "sim/cultural_trace/cultural_trace_id.h"

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
    HistoryTypeId firstSharedDangerMemory;
    HistoryTypeId firstCollectiveAvoidance;
    HistoryTypeId firstDangerAvoidanceTrace;
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

    // Group knowledge awareness
    ConceptTypeId groupDangerEvidence;
};

// Group knowledge type ids (registered via RegisterGroupKnowledgeTypes)
struct GroupKnowledgeContext
{
    GroupKnowledgeTypeId sharedDangerZone;
    GroupKnowledgeTypeId sharedFireBenefit;
};

// Social pattern type ids (registered via RegisterPatternTypes)
struct SocialPatternContext
{
    PatternTypeId collectiveAvoidance;
};

// Cultural trace type ids (registered via RegisterCulturalTraceTypes)
struct CulturalTraceContext
{
    CulturalTraceTypeId dangerAvoidanceTrace;
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
    HumanEvolution::SocialPatternContext socialPatterns;
    HumanEvolution::CulturalTraceContext culturalTraces;
};
