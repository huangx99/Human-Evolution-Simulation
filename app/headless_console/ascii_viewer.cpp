#include "app/headless_console/ascii_viewer.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

void AsciiViewer::Run(WorldState& world,
                      const HumanEvolution::EnvironmentContext& envCtx,
                      Scheduler& scheduler,
                      i32 totalTicks)
{
    for (i32 i = 0; i < totalTicks; ++i)
    {
        scheduler.Tick(world);
        GenerateLogs(world, envCtx);
        Render(world, envCtx);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void AsciiViewer::Render(WorldState& world, const HumanEvolution::EnvironmentContext& envCtx)
{
    std::ostringstream out;

    // ANSI clear screen + move cursor to home
    out << "\033[2J\033[H";

    // Header stats
    i32 cx = world.Width() / 2;
    i32 cy = world.Height() / 2;
    f32 temp  = world.Fields().Read(envCtx.temperature, cx, cy);
    f32 humid = world.Fields().Read(envCtx.humidity, cx, cy);
    f32 windX = world.Fields().Read(envCtx.windX, 0, 0);
    f32 windY = world.Fields().Read(envCtx.windY, 0, 0);

    i32 fireCount = 0;
    for (i32 y = 0; y < world.Height(); ++y)
        for (i32 x = 0; x < world.Width(); ++x)
            if (world.Fields().Read(envCtx.fire, x, y) > 0.0f)
                ++fireCount;

    out << "═══ 第 " << world.Sim().clock.currentTick << " 刻 ═══";
    out << "  温度:" << std::fixed << std::setprecision(1) << temp << "°C";
    out << "  湿度:" << std::fixed << std::setprecision(1) << humid << "%";
    out << "  风:(" << std::fixed << std::setprecision(2) << windX << "," << windY << ")";
    out << "\n\n";

    // Map: each cell is one full-width CJK character
    for (i32 y = 0; y < world.Height(); ++y)
    {
        for (i32 x = 0; x < world.Width(); ++x)
        {
            out << GetCellChar(x, y, world, envCtx);
        }
        out << "\n";
    }

    out << "\n";

    // Agents detail
    out << "【智能体】\n";
    for (const auto& agent : world.Agents().agents)
    {
        if (!agent.alive)
        {
            out << "  " << agent.id << " [已死亡]\n";
            continue;
        }
        out << "  " << agent.id;
        out << " 位置(" << agent.position.x << "," << agent.position.y << ")";
        out << " 饥饿" << static_cast<int>(agent.hunger);
        out << " 生命" << static_cast<int>(agent.health);
        out << " 行为:" << ActionName(agent.currentAction);
        out << "\n";
    }

    out << "\n";

    // Legend
    out << "【图例】";
    out << "人=智能体 ";
    out << "炎=大火 ";
    out << "火=中火 ";
    out << "焰=小火 ";
    out << "石=热石 ";
    out << "草=干草 ";
    out << "木=湿木 ";
    out << "尸=尸体 ";
    out << "煤=煤炭 ";
    out << "水=水 ";
    out << "土=土地 ";
    out << "　=空地";
    out << "\n\n";

    // Environment summary
    out << "【环境】火焰格:" << fireCount;
    out << "  智能体:" << world.Agents().agents.size();
    out << "  生态实体:" << world.Ecology().entities.Count();
    out << "\n\n";

    // Logs
    out << "【日志】\n";
    for (const auto& log : logs_)
    {
        out << "  " << log << "\n";
    }

    std::cout << out.str() << std::flush;
}

void AsciiViewer::GenerateLogs(WorldState& world, const HumanEvolution::EnvironmentContext& envCtx)
{
    (void)envCtx;
    bool isFirstTick = prevAgents_.empty();
    i32 tick = static_cast<i32>(world.Sim().clock.currentTick);
    std::string prefix = "[第" + std::to_string(tick) + "刻] ";

    if (isFirstTick)
    {
        PushLog(prefix + "模拟开始，世界 " +
                std::to_string(world.Width()) + "x" + std::to_string(world.Height()) +
                "，智能体 " + std::to_string(world.Agents().agents.size()) + " 个");
    }

    // Agent changes
    for (const auto& agent : world.Agents().agents)
    {
        auto it = prevAgents_.find(agent.id);
        if (it != prevAgents_.end())
        {
            const auto& prev = it->second;
            if (prev.alive && !agent.alive)
            {
                PushLog(prefix + "智能体" + std::to_string(agent.id) + " 死亡");
            }
            else if (agent.alive && prev.action != agent.currentAction)
            {
                std::string msg = "智能体" + std::to_string(agent.id);
                msg += " 从" + std::string(ActionName(prev.action));
                msg += " 变为" + std::string(ActionName(agent.currentAction));
                PushLog(prefix + msg);
            }
        }
        prevAgents_[agent.id] = {agent.position, agent.currentAction, agent.health, agent.alive};
    }

    // Ecology entity changes (burning state)
    for (const auto& entity : world.Ecology().entities.All())
    {
        auto it = prevEntities_.find(entity.id);
        if (it != prevEntities_.end())
        {
            const auto& prev = it->second;
            bool wasBurning = HasState(prev.state, MaterialState::Burning);
            bool isBurning  = entity.HasState(MaterialState::Burning);
            if (!wasBurning && isBurning)
            {
                std::string name = entity.name.empty() ? MaterialName(entity.material) : entity.name;
                PushLog(prefix + name + "(" + std::to_string(entity.x) + "," +
                        std::to_string(entity.y) + ") 开始燃烧");
            }
            else if (wasBurning && !isBurning)
            {
                std::string name = entity.name.empty() ? MaterialName(entity.material) : entity.name;
                PushLog(prefix + name + "(" + std::to_string(entity.x) + "," +
                        std::to_string(entity.y) + ") 熄灭");
            }
        }
        prevEntities_[entity.id] = {entity.state, entity.x, entity.y};
    }
}

void AsciiViewer::PushLog(const std::string& msg)
{
    logs_.push_back(msg);
    if (logs_.size() > MAX_LOGS)
        logs_.pop_front();
}

const char* AsciiViewer::ActionName(AgentAction action)
{
    switch (action)
    {
        case AgentAction::Idle:       return "待机";
        case AgentAction::MoveToFood: return "觅食";
        case AgentAction::Flee:       return "逃离";
        case AgentAction::Wander:     return "游荡";
    }
    return "未知";
}

const char* AsciiViewer::MaterialName(MaterialId material)
{
    switch (material)
    {
        case MaterialId::Stone:        return "石头";
        case MaterialId::DryGrass:     return "干草";
        case MaterialId::Wood:         return "木头";
        case MaterialId::Flesh:        return "肉";
        case MaterialId::Coal:         return "煤炭";
        case MaterialId::Water:        return "水";
        case MaterialId::Grass:        return "草";
        case MaterialId::Tree:         return "树";
        case MaterialId::Bush:         return "灌木";
        case MaterialId::Dirt:         return "土";
        case MaterialId::Sand:         return "沙";
        case MaterialId::Clay:         return "黏土";
        case MaterialId::Ice:          return "冰";
        case MaterialId::Mud:          return "泥";
        case MaterialId::Bone:         return "骨";
        case MaterialId::Leaf:         return "叶";
        case MaterialId::Charcoal:     return "木炭";
        case MaterialId::Ash:          return "灰";
        case MaterialId::RottingPlant: return "腐植";
        default:                       return "物体";
    }
}

std::string AsciiViewer::GetCellChar(i32 x, i32 y,
                                     const WorldState& world,
                                     const HumanEvolution::EnvironmentContext& envCtx)
{
    // 1. Agent (highest priority)
    for (const auto& agent : world.Agents().agents)
    {
        if (agent.alive && agent.position.x == x && agent.position.y == y)
            return "人";
    }

    // 2. Fire field
    f32 fire = world.Fields().Read(envCtx.fire, x, y);
    if (fire > 50.0f) return "炎";
    if (fire > 20.0f) return "火";
    if (fire > 0.0f)  return "焰";

    // 3. Ecology entity
    for (const auto& entity : world.Ecology().entities.All())
    {
        if (entity.x == x && entity.y == y)
        {
            switch (entity.material)
            {
                case MaterialId::Stone:        return "石";
                case MaterialId::DryGrass:     return "草";
                case MaterialId::Wood:         return "木";
                case MaterialId::Flesh:        return "尸";
                case MaterialId::Coal:         return "煤";
                case MaterialId::Water:        return "水";
                case MaterialId::Grass:        return "丛";
                case MaterialId::Tree:         return "树";
                case MaterialId::Bush:         return "灌";
                case MaterialId::Dirt:         return "土";
                case MaterialId::Sand:         return "沙";
                case MaterialId::Clay:         return "泥";
                case MaterialId::Ice:          return "冰";
                case MaterialId::Mud:          return "泞";
                case MaterialId::Bone:         return "骨";
                case MaterialId::Leaf:         return "叶";
                case MaterialId::Charcoal:     return "炭";
                case MaterialId::Ash:          return "灰";
                case MaterialId::RottingPlant: return "腐";
                default:                       return "物";
            }
        }
    }

    // 4. Empty
    return "　";
}
