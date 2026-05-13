#pragma once

#include "sim/runtime/rule_pack.h"
#include "sim/scheduler/scheduler.h"
#include "rules/human_evolution/environment/climate_system.h"
#include "rules/human_evolution/environment/fire_system.h"
#include "rules/human_evolution/environment/smell_system.h"
#include "sim/system/agent_perception_system.h"
#include "sim/system/agent_decision_system.h"
#include "sim/system/agent_action_system.h"

// HumanEvolutionRulePack: defines the Human Evolution world's environment rules.
//
// Fields: temperature, humidity, fire, wind_x, wind_y, smell, danger, smoke
// Systems: ClimateSystem, FireSystem, SmellSystem
//
// Agent/Cognitive systems remain in sim/system/ (engine-owned).
//
// Semantic role → FieldBindings mapping:
//   EnvironmentModule:
//     env0 = temperature (spatial 2D)
//     env1 = humidity    (spatial 2D)
//     env2 = fire        (spatial 2D)
//     env3 = wind_x      (scalar)
//     env4 = wind_y      (scalar)
//   InformationModule:
//     info0 = smell  (spatial 2D)
//     info1 = danger (spatial 2D)
//     info2 = smoke  (spatial 2D)

namespace HumanEvolution {

// Semantic role indices — used by systems and tests for readable access
namespace EnvRole {
    constexpr i32 Temperature = 0;
    constexpr i32 Humidity    = 1;
    constexpr i32 Fire        = 2;
    constexpr i32 WindX       = 3;
    constexpr i32 WindY       = 4;
}

namespace InfoRole {
    constexpr i32 Smell  = 0;
    constexpr i32 Danger = 1;
    constexpr i32 Smoke  = 2;
}

// Semantic accessors for EnvironmentModule
inline FieldRef&       EnvField(EnvironmentModule& env, i32 role)       { return (&env.env0)[role]; }
inline const FieldRef& EnvField(const EnvironmentModule& env, i32 role) { return (&env.env0)[role]; }

// Semantic accessors for InformationModule
inline FieldRef&       InfoField(InformationModule& info, i32 role)       { return (&info.info0)[role]; }
inline const FieldRef& InfoField(const InformationModule& info, i32 role) { return (&info.info0)[role]; }

} // namespace HumanEvolution

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

    FieldBindings BindFields() const override
    {
        FieldBindings b;
        // EnvironmentModule: env0=temperature, env1=humidity, env2=fire, env3=wind_x, env4=wind_y
        b.env0 = FieldKey("human_evolution.temperature");
        b.env1 = FieldKey("human_evolution.humidity");
        b.env2 = FieldKey("human_evolution.fire");
        b.env3 = FieldKey("human_evolution.wind_x");
        b.env4 = FieldKey("human_evolution.wind_y");
        // InformationModule: info0=smell, info1=danger, info2=smoke
        b.info0 = FieldKey("human_evolution.smell");
        b.info1 = FieldKey("human_evolution.danger");
        b.info2 = FieldKey("human_evolution.smoke");
        return b;
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

// === Test helpers ===

// Register all HumanEvolution systems (RulePack environment + engine agent).
// Use in tests instead of manual AddSystem calls.
inline void RegisterHumanEvolutionSystems(Scheduler& scheduler)
{
    // RulePack environment systems
    scheduler.AddSystem(SimPhase::Environment, std::make_unique<ClimateSystem>());
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<FireSystem>());
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SmellSystem>());

    // Engine agent systems
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>());
    scheduler.AddSystem(SimPhase::Decision,   std::make_unique<AgentDecisionSystem>());
    scheduler.AddSystem(SimPhase::Action,     std::make_unique<AgentActionSystem>());
}

// Create a fully configured HumanEvolution scheduler.
inline Scheduler CreateHumanEvolutionScheduler()
{
    Scheduler scheduler;
    RegisterHumanEvolutionSystems(scheduler);
    return scheduler;
}
