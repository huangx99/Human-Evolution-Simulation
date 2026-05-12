#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include <cmath>

class AgentSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        for (auto& agent : world.agents)
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

        if (!world.smell.InBounds(x, y)) return;

        agent.localTemperature = world.temperature.At(x, y);

        // Scan nearby for strongest smell and fire
        agent.nearestSmell = 0.0f;
        agent.nearestFire = 0.0f;

        i32 scanRadius = 3;
        for (i32 sy = -scanRadius; sy <= scanRadius; sy++)
        {
            for (i32 sx = -scanRadius; sx <= scanRadius; sx++)
            {
                i32 nx = x + sx;
                i32 ny = y + sy;

                if (!world.smell.InBounds(nx, ny)) continue;

                f32 dist = std::sqrt(static_cast<f32>(sx * sx + sy * sy));
                if (dist > scanRadius) continue;

                f32 smellVal = world.smell.At(nx, ny) / (1.0f + dist);
                if (smellVal > agent.nearestSmell) agent.nearestSmell = smellVal;

                f32 fireVal = world.fire.At(nx, ny) / (1.0f + dist);
                if (fireVal > agent.nearestFire) agent.nearestFire = fireVal;
            }
        }
    }

    void Decide(WorldState& /*world*/, Agent& agent)
    {
        // Score each action based on perception
        f32 fleeScore = 0.0f;
        f32 foodScore = 0.0f;
        f32 wanderScore = 0.1f;  // Small base wander desire

        // High fire → flee
        if (agent.nearestFire > 10.0f)
        {
            fleeScore = agent.nearestFire * 2.0f;
        }

        // Hunger + smell → seek food
        if (agent.hunger > 30.0f && agent.nearestSmell > 5.0f)
        {
            foodScore = agent.hunger * agent.nearestSmell * 0.01f;
        }

        // High temperature → discomfort, wander away
        if (agent.localTemperature > 40.0f)
        {
            wanderScore += (agent.localTemperature - 40.0f) * 0.1f;
        }

        // Pick highest score
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
        // Hunger increases each tick
        agent.hunger = std::min(100.0f, agent.hunger + 0.5f);

        i32 dx = 0, dy = 0;

        switch (agent.currentAction)
        {
        case AgentAction::Flee:
        {
            // Move away from fire
            i32 bestDx = 0, bestDy = 0;
            f32 bestFire = agent.nearestFire;

            static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
            for (auto& off : offsets)
            {
                i32 nx = agent.position.x + off[0];
                i32 ny = agent.position.y + off[1];
                if (!world.fire.InBounds(nx, ny)) continue;
                if (world.fire.At(nx, ny) < bestFire)
                {
                    bestFire = world.fire.At(nx, ny);
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
            // Move toward strongest smell
            i32 bestDx = 0, bestDy = 0;
            f32 bestSmell = 0.0f;

            static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
            for (auto& off : offsets)
            {
                i32 nx = agent.position.x + off[0];
                i32 ny = agent.position.y + off[1];
                if (!world.smell.InBounds(nx, ny)) continue;
                if (world.smell.At(nx, ny) > bestSmell)
                {
                    bestSmell = world.smell.At(nx, ny);
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
            // Random direction
            i32 dir = world.random.NextRange(0, 4);
            static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1},{0,0}};
            dx = offsets[dir][0];
            dy = offsets[dir][1];
            break;
        }
        case AgentAction::Idle:
        default:
            break;
        }

        // Apply movement
        i32 newX = agent.position.x + dx;
        i32 newY = agent.position.y + dy;

        if (world.temperature.InBounds(newX, newY))
        {
            agent.position.x = newX;
            agent.position.y = newY;
        }

        // Eating: if on a cell with strong smell, reduce hunger
        if (world.smell.InBounds(agent.position.x, agent.position.y))
        {
            if (world.smell.At(agent.position.x, agent.position.y) > 20.0f)
            {
                agent.hunger = std::max(0.0f, agent.hunger - 5.0f);
            }
        }

        // Heat damage
        if (agent.localTemperature > 50.0f)
        {
            agent.health -= (agent.localTemperature - 50.0f) * 0.1f;
        }
        agent.health = std::max(0.0f, std::min(100.0f, agent.health));
    }
};
