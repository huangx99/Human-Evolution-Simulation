#pragma once

#include "sim/world/module_registry.h"
#include "sim/entity/agent.h"
#include "core/types/types.h"
#include <vector>

struct AgentModule : public IModule
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

    Agent* Find(EntityId id)
    {
        for (auto& a : agents)
        {
            if (a.id == id) return &a;
        }
        return nullptr;
    }
};
