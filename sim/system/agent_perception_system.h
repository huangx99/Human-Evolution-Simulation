#pragma once

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include <cmath>

// OWNERSHIP: Engine (sim/system/)
// READS: EnvironmentModule (fire, temperature), InformationModule (smell, danger), AgentModule (agents)
// WRITES: AgentModule (perception fields: nearestFire, localTemperature, nearestSmell, localDanger)
// PHASE: SimPhase::Perception

class AgentPerceptionSystem : public ISystem
{
public:
    void Update(SystemContext& ctx) override
    {
        auto& world = ctx.World();
        auto& env = world.Env();
        auto& info = world.Info();
        auto& agentMod = world.Agents();

        for (auto& agent : agentMod.agents)
        {
            i32 x = agent.position.x;
            i32 y = agent.position.y;

            if (!info.info0.InBounds(x, y)) continue;

            agent.localTemperature = env.env0.At(x, y);
            agent.nearestSmell = 0.0f;
            agent.nearestFire = 0.0f;

            i32 scanRadius = 3;
            for (i32 sy = -scanRadius; sy <= scanRadius; sy++)
            {
                for (i32 sx = -scanRadius; sx <= scanRadius; sx++)
                {
                    i32 nx = x + sx;
                    i32 ny = y + sy;

                    if (!info.info0.InBounds(nx, ny)) continue;

                    f32 dist = std::sqrt(static_cast<f32>(sx * sx + sy * sy));
                    if (dist > scanRadius) continue;

                    f32 smellVal = info.info0.At(nx, ny) / (1.0f + dist);
                    if (smellVal > agent.nearestSmell) agent.nearestSmell = smellVal;

                    f32 fireVal = env.env2.At(nx, ny) / (1.0f + dist);
                    if (fireVal > agent.nearestFire) agent.nearestFire = fireVal;
                }
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Environment, AccessMode::Read},
            {ModuleTag::Information, AccessMode::Read},
            {ModuleTag::Agent, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Agent, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"AgentPerceptionSystem", SimPhase::Perception, READS, 3, WRITES, 1, DEPS, 0, true, false};
    }
};
