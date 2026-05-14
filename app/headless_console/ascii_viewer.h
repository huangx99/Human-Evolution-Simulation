#pragma once

#include "sim/world/world_state.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/scheduler/scheduler.h"
#include "sim/entity/agent_action.h"
#include "sim/ecology/material_id.h"
#include <string>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class AsciiViewer
{
public:
    struct PermanentFireConfig
    {
        bool enabled = true;
        i32 x = 16;
        i32 y = 16;
        i32 radius = 2;
        f32 intensity = 85.0f;
    };

    struct PermanentFoodConfig
    {
        bool enabled = true;
        i32 x = 27;
        i32 y = 27;
        i32 radius = 1;
    };

    void Run(WorldState& world,
             const HumanEvolution::EnvironmentContext& envCtx,
             Scheduler& scheduler,
             i32 totalTicks,
             i32 sleepMs,
             PermanentFireConfig permanentFire,
             PermanentFoodConfig permanentFood);

private:
    void Render(WorldState& world, const HumanEvolution::EnvironmentContext& envCtx);
    void GenerateLogs(WorldState& world, const HumanEvolution::EnvironmentContext& envCtx);
    void MaintainPermanentFire(WorldState& world,
                               const HumanEvolution::EnvironmentContext& envCtx,
                               const PermanentFireConfig& permanentFire);
    void MaintainPermanentFood(WorldState& world,
                               const PermanentFoodConfig& permanentFood);
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
    std::unordered_set<u64> seenGroupKnowledge_;
    std::unordered_set<u64> seenPatterns_;
    std::unordered_set<u64> seenCulturalTraces_;
    std::unordered_set<u64> seenHistoryEvents_;
    bool loggedPermanentFire_ = false;
    bool loggedPermanentFood_ = false;
    std::deque<std::string> logs_;
    static constexpr size_t MAX_LOGS = 10;
};
