#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"

class AgentDecisionSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        for (auto& agent : world.agents.agents)
        {
            f32 fleeScore = 0.0f;
            f32 foodScore = 0.0f;
            f32 wanderScore = 0.1f;

            if (agent.nearestFire > 10.0f)
            {
                fleeScore = agent.nearestFire * 2.0f;
            }

            if (agent.hunger > 30.0f && agent.nearestSmell > 5.0f)
            {
                foodScore = agent.hunger * agent.nearestSmell * 0.01f;
            }

            if (agent.localTemperature > 40.0f)
            {
                wanderScore += (agent.localTemperature - 40.0f) * 0.1f;
            }

            if (fleeScore > foodScore && fleeScore > wanderScore)
                agent.currentAction = AgentAction::Flee;
            else if (foodScore > wanderScore)
                agent.currentAction = AgentAction::MoveToFood;
            else
                agent.currentAction = AgentAction::Wander;
        }
    }
};
