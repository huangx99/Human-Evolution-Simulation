#pragma once

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include <cmath>

// AgentPerceptionSystem: reads environment/information fields to update agent perception.
// Moved from sim/system/ — depends on HumanEvolution field bindings.

class AgentPerceptionSystem : public ISystem
{
public:
    explicit AgentPerceptionSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : envCtx_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& fm = ctx.GetFieldModule();
        auto& agentMod = ctx.Agents();

        for (auto& agent : agentMod.agents)
        {
            i32 x = agent.position.x;
            i32 y = agent.position.y;

            if (!fm.InBounds(envCtx_.smell, x, y)) continue;

            agent.localTemperature = fm.Read(envCtx_.temperature, x, y);
            agent.nearestSmell = 0.0f;
            agent.nearestFire = 0.0f;

            i32 scanRadius = 3;
            for (i32 sy = -scanRadius; sy <= scanRadius; sy++)
            {
                for (i32 sx = -scanRadius; sx <= scanRadius; sx++)
                {
                    i32 nx = x + sx;
                    i32 ny = y + sy;

                    if (!fm.InBounds(envCtx_.smell, nx, ny)) continue;

                    f32 dist = std::sqrt(static_cast<f32>(sx * sx + sy * sy));
                    if (dist > scanRadius) continue;

                    f32 smellVal = fm.Read(envCtx_.smell, nx, ny) / (1.0f + dist);
                    if (smellVal > agent.nearestSmell) agent.nearestSmell = smellVal;

                    f32 fireVal = fm.Read(envCtx_.fire, nx, ny) / (1.0f + dist);
                    if (fireVal > agent.nearestFire) agent.nearestFire = fireVal;
                }
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Agent, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"AgentPerceptionSystem", SimPhase::Perception, READS, 1, WRITES, 1, DEPS, 0, true, false};
    }

private:
    HumanEvolution::EnvironmentContext envCtx_;
};
