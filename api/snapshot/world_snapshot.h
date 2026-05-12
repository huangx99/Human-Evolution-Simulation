#pragma once

#include "core/types/types.h"
#include "sim/world/world_state.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

struct AgentSnapshot
{
    EntityId id;
    i32 x, y;
    f32 hunger;
    f32 health;
    u8 action;
};

struct WorldSnapshot
{
    Tick tick;
    u64 randomState[2];
    i32 width, height;
    f32 windX, windY;

    // Flattened field data (only sampled points to keep size manageable)
    struct CellSample
    {
        i32 x, y;
        f32 temperature;
        f32 humidity;
        f32 fire;
        f32 smell;
    };

    std::vector<CellSample> hotCells;   // cells with fire > 0 or smell > 1
    std::vector<AgentSnapshot> agents;

    static WorldSnapshot Capture(const WorldState& world, f32 fireThreshold = 0.0f, f32 smellThreshold = 0.0f)
    {
        WorldSnapshot snap;
        snap.tick = world.clock.currentTick;
        snap.width = world.width;
        snap.height = world.height;
        snap.windX = world.wind.x;
        snap.windY = world.wind.y;

        const u64* rstate = world.random.GetState();
        snap.randomState[0] = rstate[0];
        snap.randomState[1] = rstate[1];

        for (i32 y = 0; y < world.height; y++)
        {
            for (i32 x = 0; x < world.width; x++)
            {
                f32 f = world.fire.At(x, y);
                f32 s = world.smell.At(x, y);
                if (f > fireThreshold || s > smellThreshold)
                {
                    snap.hotCells.push_back({
                        x, y,
                        world.temperature.At(x, y),
                        world.humidity.At(x, y),
                        f, s
                    });
                }
            }
        }

        for (const auto& agent : world.agents)
        {
            snap.agents.push_back({
                agent.id,
                agent.position.x, agent.position.y,
                agent.hunger, agent.health,
                static_cast<u8>(agent.currentAction)
            });
        }

        return snap;
    }

    // Serialize to deterministic text format
    std::string Serialize() const
    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(2);

        os << "tick=" << tick << "\n";
        os << "wind=" << windX << "," << windY << "\n";
        os << "hot_cells=" << hotCells.size() << "\n";

        for (const auto& c : hotCells)
        {
            os << "  cell " << c.x << "," << c.y
               << " t=" << c.temperature
               << " h=" << c.humidity
               << " f=" << c.fire
               << " s=" << c.smell << "\n";
        }

        os << "agents=" << agents.size() << "\n";
        for (const auto& a : agents)
        {
            os << "  agent " << a.id
               << " pos=" << a.x << "," << a.y
               << " hunger=" << a.hunger
               << " health=" << a.health
               << " action=" << static_cast<i32>(a.action) << "\n";
        }

        return os.str();
    }
};
