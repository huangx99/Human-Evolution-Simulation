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

struct WorldState
{
    ModuleRegistry modules;
    EventBus events;
    CommandBuffer commands;
    SpatialIndex spatial;

    i32 width;
    i32 height;
    u64 lastTickHash = 0;

    WorldState(i32 w, i32 h, u64 seed)
        : width(w), height(h), spatial(w, h)
    {
        modules.Register<SimulationModule>(seed);
        modules.Register<FieldModule>(w, h);

        // Register fields BEFORE alias facades (they look up FieldIndex at construction)
        auto& fm = modules.Get<FieldModule>();
        fm.RegisterField(FieldKey("human_evolution.temperature"), "temperature", 20.0f);
        fm.RegisterField(FieldKey("human_evolution.humidity"),    "humidity",    50.0f);
        fm.RegisterField(FieldKey("human_evolution.fire"),        "fire",         0.0f);
        fm.RegisterScalarField(FieldKey("human_evolution.wind_x"), "wind_x", 0.0f);
        fm.RegisterScalarField(FieldKey("human_evolution.wind_y"), "wind_y", 0.0f);
        fm.RegisterField(FieldKey("human_evolution.smell"),       "smell",        0.0f);
        fm.RegisterField(FieldKey("human_evolution.danger"),      "danger",       0.0f);
        fm.RegisterField(FieldKey("human_evolution.smoke"),       "smoke",        0.0f);

        // Alias facades — delegate to FieldModule, no own Field2D
        modules.Register<EnvironmentModule>(fm);
        modules.Register<InformationModule>(fm);
        modules.Register<AgentModule>();
        modules.Register<EcologyModule>();
        modules.Register<CognitiveModule>();
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
