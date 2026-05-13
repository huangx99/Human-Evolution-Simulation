#pragma once

#include "sim/runtime/rule_pack.h"
#include "rules/human_evolution/environment/climate_system.h"
#include "rules/human_evolution/environment/fire_system.h"
#include "rules/human_evolution/environment/smell_system.h"

// HumanEvolutionRulePack: defines the Human Evolution world's environment rules.
//
// Fields: temperature, humidity, fire, wind_x, wind_y, smell, danger, smoke
// Systems: ClimateSystem, FireSystem, SmellSystem
//
// Agent/Cognitive systems remain in sim/system/ (engine-owned).

class HumanEvolutionRulePack : public IRulePack
{
public:
    const char* Name() const override { return "HumanEvolution"; }

    void RegisterFields(FieldModule& fields) override
    {
        fields.RegisterField(FieldKey("human_evolution.temperature"), "temperature", 20.0f);
        fields.RegisterField(FieldKey("human_evolution.humidity"),    "humidity",    50.0f);
        fields.RegisterField(FieldKey("human_evolution.fire"),        "fire",         0.0f);
        fields.RegisterScalarField(FieldKey("human_evolution.wind_x"), "wind_x", 0.0f);
        fields.RegisterScalarField(FieldKey("human_evolution.wind_y"), "wind_y", 0.0f);
        fields.RegisterField(FieldKey("human_evolution.smell"),       "smell",        0.0f);
        fields.RegisterField(FieldKey("human_evolution.danger"),      "danger",       0.0f);
        fields.RegisterField(FieldKey("human_evolution.smoke"),       "smoke",        0.0f);
    }

    std::vector<SystemRegistration> CreateSystems() override
    {
        std::vector<SystemRegistration> systems;
        systems.push_back({SimPhase::Environment, std::make_unique<ClimateSystem>()});
        systems.push_back({SimPhase::Propagation, std::make_unique<FireSystem>()});
        systems.push_back({SimPhase::Propagation, std::make_unique<SmellSystem>()});
        return systems;
    }
};
