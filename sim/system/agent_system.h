#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class AgentSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        for (auto& agent : world.agents.agents)
        {
            Perceive(world, agent);
            Decide(world, agent);
            Act(world, agent);
        }
    }

private:
    void Perceive(WorldState& world, Agent& agent)
    {
        i32 x = agent.position.x;
        i32 y = agent.position.y;

        if (!world.info.smell.InBounds(x, y)) return;

        agent.localTemperature = world.env.temperature.At(x, y);

        agent.nearestSmell = 0.0f;
        agent.nearestFire = 0.0f;

        i32 scanRadius = 3;
        for (i32 sy = -scanRadius; sy <= scanRadius; sy++)
        {
            for (i32 sx = -scanRadius; sx <= scanRadius; sx++)
            {
                i32 nx = x + sx;
                i32 ny = y + sy;

                if (!world.info.smell.InBounds(nx, ny)) continue;

                f32 dist = std::sqrt(static_cast<f32>(sx * sx + sy * sy));
                if (dist > scanRadius) continue;

                f32 smellVal = world.info.smell.At(nx, ny) / (1.0f + dist);
                if (smellVal > agent.nearestSmell) agent.nearestSmell = smellVal;

                f32 fireVal = world.env.fire.At(nx, ny) / (1.0f + dist);
                if (fireVal > agent.nearestFire) agent.nearestFire = fireVal;
            }
        }
    }

    void Decide(WorldState& /*world*/, Agent& agent)
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
        {
            agent.currentAction = AgentAction::Flee;
        }
        else if (foodScore > wanderScore)
        {
            agent.currentAction = AgentAction::MoveToFood;
        }
        else
        {
            agent.currentAction = AgentAction::Wander;
        }
    }

    void Act(WorldState& world, Agent& agent)
    {
        agent.hunger = std::min(100.0f, agent.hunger + 0.5f);

        i32 dx = 0, dy = 0;

        switch (agent.currentAction)
        {
        case AgentAction::Flee:
        {
            i32 bestDx = 0, bestDy = 0;
            f32 bestFire = agent.nearestFire;

            static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
            for (auto& off : offsets)
            {
                i32 nx = agent.position.x + off[0];
                i32 ny = agent.position.y + off[1];
                if (!world.env.fire.InBounds(nx, ny)) continue;
                if (world.env.fire.At(nx, ny) < bestFire)
                {
                    bestFire = world.env.fire.At(nx, ny);
                    bestDx = off[0];
                    bestDy = off[1];
                }
            }
            dx = bestDx;
            dy = bestDy;
            break;
        }
        case AgentAction::MoveToFood:
        {
            i32 bestDx = 0, bestDy = 0;
            f32 bestSmell = 0.0f;

            static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
            for (auto& off : offsets)
            {
                i32 nx = agent.position.x + off[0];
                i32 ny = agent.position.y + off[1];
                if (!world.info.smell.InBounds(nx, ny)) continue;
                if (world.info.smell.At(nx, ny) > bestSmell)
                {
                    bestSmell = world.info.smell.At(nx, ny);
                    bestDx = off[0];
                    bestDy = off[1];
                }
            }
            dx = bestDx;
            dy = bestDy;
            break;
        }
        case AgentAction::Wander:
        {
            i32 dir = world.sim.random.NextRange(0, 4);
            static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1},{0,0}};
            dx = offsets[dir][0];
            dy = offsets[dir][1];
            break;
        }
        case AgentAction::Idle:
        default:
            break;
        }

        i32 newX = agent.position.x + dx;
        i32 newY = agent.position.y + dy;

        if (world.env.temperature.InBounds(newX, newY))
        {
            agent.position.x = newX;
            agent.position.y = newY;
        }

        if (world.info.smell.InBounds(agent.position.x, agent.position.y))
        {
            if (world.info.smell.At(agent.position.x, agent.position.y) > 20.0f)
            {
                agent.hunger = std::max(0.0f, agent.hunger - 5.0f);
            }
        }

        if (agent.localTemperature > 50.0f)
        {
            agent.health -= (agent.localTemperature - 50.0f) * 0.1f;
        }
        agent.health = std::max(0.0f, std::min(100.0f, agent.health));
    }
};
