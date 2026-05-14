#pragma once

#include "sim/world/world_state.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/scheduler/scheduler.h"
#include "sim/entity/agent_action.h"
#include "sim/ecology/material_id.h"
#include <string>
#include <deque>
#include <unordered_map>
#include <vector>

class AsciiViewer
{
public:
    void Run(WorldState& world,
             const HumanEvolution::EnvironmentContext& envCtx,
             Scheduler& scheduler,
             i32 totalTicks);

private:
    void Render(WorldState& world, const HumanEvolution::EnvironmentContext& envCtx);
    void GenerateLogs(WorldState& world, const HumanEvolution::EnvironmentContext& envCtx);
    void PushLog(const std::string& msg);

    static const char* ActionName(AgentAction action);
    static const char* MaterialName(MaterialId material);
    static std::string GetCellChar(i32 x, i32 y,
                                   const WorldState& world,
                                   const HumanEvolution::EnvironmentContext& envCtx);

    struct AgentSnapshot
    {
        Vec2i pos;
        AgentAction action;
        f32 health;
        bool alive;
    };

    struct EntitySnapshot
    {
        MaterialState state;
        i32 x;
        i32 y;
    };

    std::unordered_map<EntityId, AgentSnapshot> prevAgents_;
    std::unordered_map<EntityId, EntitySnapshot> prevEntities_;
    std::deque<std::string> logs_;
    static constexpr size_t MAX_LOGS = 10;
};
