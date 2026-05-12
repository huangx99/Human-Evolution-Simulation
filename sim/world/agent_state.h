#pragma once

#include "core/types/types.h"
#include "sim/entity/agent.h"
#include <vector>

struct AgentState
{
    std::vector<Agent> agents;
    EntityId nextEntityId = 1;

    EntityId Spawn(i32 x, i32 y)
    {
        Agent agent;
        agent.id = nextEntityId++;
        agent.position = {x, y};
        agents.push_back(agent);
        return agent.id;
    }
};
