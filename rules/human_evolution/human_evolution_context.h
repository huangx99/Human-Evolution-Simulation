#pragma once

#include "sim/field/field_id.h"
#include "sim/runtime/rule_context.h"
#include "sim/history/history_id.h"

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
    // HumanEvolution::AgentContext agent;
    // HumanEvolution::CognitiveContext cognitive;
};
