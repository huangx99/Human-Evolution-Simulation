#pragma once

#include "sim/world/environment_state.h"
#include "sim/world/information_state.h"
#include "sim/world/agent_state.h"
#include "sim/world/simulation_state.h"
#include "sim/event/event_bus.h"
#include "sim/command/command_buffer.h"

struct WorldState
{
    SimulationState sim;
    EnvironmentState env;
    InformationState info;
    AgentState agents;
    EventBus events;
    CommandBuffer commands;

    WorldState(i32 w, i32 h, u64 seed)
        : sim(seed)
        , env(w, h)
        , info(w, h)
    {
    }

    i32 Width() const { return env.width; }
    i32 Height() const { return env.height; }

    EntityId SpawnAgent(i32 x, i32 y)
    {
        return agents.Spawn(x, y);
    }
};
