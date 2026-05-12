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

    struct CellSample
    {
        i32 x, y;
        f32 temperature;
        f32 humidity;
        f32 fire;
        f32 smell;
        f32 danger;
    };

    std::vector<CellSample> hotCells;
    std::vector<AgentSnapshot> agents;

    static WorldSnapshot Capture(const WorldState& world)
    {
        WorldSnapshot snap;
        snap.tick = world.Sim().clock.currentTick;
        snap.width = world.Env().width;
        snap.height = world.Env().height;
        snap.windX = world.Env().wind.x;
        snap.windY = world.Env().wind.y;

        const u64* rstate = world.Sim().random.GetState();
        snap.randomState[0] = rstate[0];
        snap.randomState[1] = rstate[1];

        for (i32 y = 0; y < snap.height; y++)
        {
            for (i32 x = 0; x < snap.width; x++)
            {
                f32 f = world.Env().fire.At(x, y);
                f32 s = world.Info().smell.At(x, y);
                f32 d = world.Info().danger.At(x, y);
                if (f > 0.0f || s > 1.0f || d > 0.0f)
                {
                    snap.hotCells.push_back({
                        x, y,
                        world.Env().temperature.At(x, y),
                        world.Env().humidity.At(x, y),
                        f, s, d
                    });
                }
            }
        }

        for (const auto& agent : world.Agents().agents)
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

    std::string Serialize() const
    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(2);

        os << "tick=" << tick << "\n";
        os << "random_state=" << randomState[0] << "," << randomState[1] << "\n";
        os << "wind=" << windX << "," << windY << "\n";
        os << "hot_cells=" << hotCells.size() << "\n";

        for (const auto& c : hotCells)
        {
            os << "  cell " << c.x << "," << c.y
               << " t=" << c.temperature
               << " h=" << c.humidity
               << " f=" << c.fire
               << " s=" << c.smell
               << " d=" << c.danger << "\n";
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
