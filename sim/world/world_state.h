#pragma once

#include "core/types/types.h"
#include "core/time/sim_clock.h"
#include "core/random/random.h"
#include "core/container/grid.h"
#include "sim/entity/agent.h"
#include <vector>

struct WindDirection
{
    f32 x = 0.0f;  // East-West: positive = East
    f32 y = 0.0f;  // North-South: positive = North
};

struct WorldState
{
    SimClock clock;
    Random random;

    i32 width;
    i32 height;

    // Environment fields
    Grid<f32> temperature;   // Celsius
    Grid<f32> humidity;      // 0-100
    Grid<f32> fire;          // 0-100 (burn intensity)
    Grid<f32> smell;         // smell strength

    // Global
    WindDirection wind;

    // Entities
    std::vector<Agent> agents;
    EntityId nextEntityId = 1;

    WorldState(i32 w, i32 h, u64 seed)
        : random(seed)
        , width(w), height(h)
        , temperature(w, h, 20.0f)
        , humidity(w, h, 50.0f)
        , fire(w, h, 0.0f)
        , smell(w, h, 0.0f)
    {
    }

    EntityId SpawnAgent(i32 x, i32 y)
    {
        Agent agent;
        agent.id = nextEntityId++;
        agent.position = {x, y};
        agents.push_back(agent);
        return agent.id;
    }
};
