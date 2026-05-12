#pragma once

#include "sim/world/module_registry.h"
#include "sim/world/simulation_module.h"
#include "sim/world/environment_module.h"
#include "sim/world/information_module.h"
#include "sim/world/agent_module.h"
#include "sim/world/ecology_module.h"
#include "sim/event/event_bus.h"
#include "sim/command/command_buffer.h"

struct WorldState
{
    ModuleRegistry modules;
    EventBus events;
    CommandBuffer commands;

    i32 width;
    i32 height;

    WorldState(i32 w, i32 h, u64 seed)
        : width(w), height(h)
    {
        modules.Register<SimulationModule>(seed);
        modules.Register<EnvironmentModule>(w, h);
        modules.Register<InformationModule>(w, h);
        modules.Register<AgentModule>();
        modules.Register<EcologyModule>();
    }

    SimulationModule& Sim() { return modules.Get<SimulationModule>(); }
    EnvironmentModule& Env() { return modules.Get<EnvironmentModule>(); }
    InformationModule& Info() { return modules.Get<InformationModule>(); }
    AgentModule& Agents() { return modules.Get<AgentModule>(); }
    EcologyModule& Ecology() { return modules.Get<EcologyModule>(); }

    const SimulationModule& Sim() const { return modules.Get<SimulationModule>(); }
    const EnvironmentModule& Env() const { return modules.Get<EnvironmentModule>(); }
    const InformationModule& Info() const { return modules.Get<InformationModule>(); }
    const AgentModule& Agents() const { return modules.Get<AgentModule>(); }
    const EcologyModule& Ecology() const { return modules.Get<EcologyModule>(); }

    i32 Width() const { return width; }
    i32 Height() const { return height; }

    EntityId SpawnAgent(i32 x, i32 y)
    {
        return Agents().Spawn(x, y);
    }
};
