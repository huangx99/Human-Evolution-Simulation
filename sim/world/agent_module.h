#pragma once

#include "sim/world/module_registry.h"
#include "sim/entity/agent.h"
#include "rules/human_evolution/runtime/state_manager.h"
#include "rules/human_evolution/runtime/state_behavior_registry.h"
#include "core/types/types.h"
#include <vector>
#include <unordered_map>

struct AgentModule : public IModule
{
    std::vector<Agent> agents;
    EntityId nextEntityId = 1;

    // Per-agent state managers (keyed by agent id)
    std::unordered_map<EntityId, StateManager> stateManagers;

    EntityId Spawn(i32 x, i32 y)
    {
        Agent agent;
        agent.id = nextEntityId++;
        agent.position = {x, y};
        agents.push_back(agent);

        // Create state manager and register all behaviors
        auto& sm = stateManagers[agent.id];
        StateBehaviorRegistry::Instance().RegisterAllTo(sm);

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

    StateManager* FindStateManager(EntityId id)
    {
        auto it = stateManagers.find(id);
        return (it != stateManagers.end()) ? &it->second : nullptr;
    }
};
