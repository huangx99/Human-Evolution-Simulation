#pragma once

#include "sim/world/module_registry.h"
#include "sim/world/simulation_module.h"
#include "sim/field/field_module.h"
#include "sim/world/environment_module.h"
#include "sim/world/information_module.h"
#include "sim/world/agent_module.h"
#include "sim/world/ecology_module.h"
#include "sim/cognitive/cognitive_module.h"
#include "sim/event/event_bus.h"
#include "sim/command/command_buffer.h"
#include "sim/spatial/spatial_index.h"
#include "sim/runtime/rule_pack.h"

struct WorldState
{
    ModuleRegistry modules;
    EventBus events;
    CommandBuffer commands;
    SpatialIndex spatial;

    i32 width;
    i32 height;
    u64 lastTickHash = 0;

    // Primary constructor: engine modules + RulePack-driven fields/systems
    WorldState(i32 w, i32 h, u64 seed, IRulePack& rulePack)
        : width(w), height(h), spatial(w, h)
    {
        modules.Register<SimulationModule>(seed);
        modules.Register<FieldModule>(w, h);

        // RulePack registers its fields into FieldModule
        auto& fm = modules.Get<FieldModule>();
        rulePack.RegisterFields(fm);

        // RulePack provides field bindings — Engine constructs alias facades
        auto bindings = rulePack.BindFields();
        modules.Register<EnvironmentModule>(fm, bindings);
        modules.Register<InformationModule>(fm, bindings);

        modules.Register<AgentModule>();
        modules.Register<EcologyModule>();
        modules.Register<CognitiveModule>();
    }

    // Convenience constructor: no RulePack (for engine-only tests).
    // EnvironmentModule/InformationModule have invalid FieldRefs — use FieldModule directly.
    WorldState(i32 w, i32 h, u64 seed)
        : width(w), height(h), spatial(w, h)
    {
        modules.Register<SimulationModule>(seed);
        modules.Register<FieldModule>(w, h);

        FieldBindings empty;
        auto& fm = modules.Get<FieldModule>();
        modules.Register<EnvironmentModule>(fm, empty);
        modules.Register<InformationModule>(fm, empty);

        modules.Register<AgentModule>();
        modules.Register<EcologyModule>();
        modules.Register<CognitiveModule>();
    }

    // Deferred RulePack init: register fields + rebuild facades.
    // Use when WorldState was constructed without a RulePack.
    void Init(IRulePack& rulePack)
    {
        auto& fm = modules.Get<FieldModule>();
        rulePack.RegisterFields(fm);

        // Re-register alias facades with real bindings
        auto bindings = rulePack.BindFields();
        modules.Register<EnvironmentModule>(fm, bindings);
        modules.Register<InformationModule>(fm, bindings);
    }

    void RebuildSpatial()
    {
        spatial.Rebuild(Ecology().entities);
    }

    SimulationModule& Sim() { return modules.Get<SimulationModule>(); }
    FieldModule& Fields() { return modules.Get<FieldModule>(); }
    EnvironmentModule& Env() { return modules.Get<EnvironmentModule>(); }
    InformationModule& Info() { return modules.Get<InformationModule>(); }
    AgentModule& Agents() { return modules.Get<AgentModule>(); }
    EcologyModule& Ecology() { return modules.Get<EcologyModule>(); }
    CognitiveModule& Cognitive() { return modules.Get<CognitiveModule>(); }

    const SimulationModule& Sim() const { return modules.Get<SimulationModule>(); }
    const FieldModule& Fields() const { return modules.Get<FieldModule>(); }
    const EnvironmentModule& Env() const { return modules.Get<EnvironmentModule>(); }
    const InformationModule& Info() const { return modules.Get<InformationModule>(); }
    const AgentModule& Agents() const { return modules.Get<AgentModule>(); }
    const EcologyModule& Ecology() const { return modules.Get<EcologyModule>(); }
    const CognitiveModule& Cognitive() const { return modules.Get<CognitiveModule>(); }

    i32 Width() const { return width; }
    i32 Height() const { return height; }

    EntityId SpawnAgent(i32 x, i32 y)
    {
        return Agents().Spawn(x, y);
    }

};
