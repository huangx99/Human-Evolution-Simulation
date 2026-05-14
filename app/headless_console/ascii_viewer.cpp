#include "app/headless_console/ascii_viewer.h"
#include "sim/pattern/pattern_registry.h"
#include "sim/cognitive/concept_registry.h"
#include "sim/social/social_signal_registry.h"
#include "sim/group_knowledge/group_knowledge_registry.h"
#include "sim/cultural_trace/cultural_trace_registry.h"
#include "sim/history/history_registry.h"
#include "sim/ecology/material_db.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <algorithm>

namespace
{
f32 DistanceSquared(Vec2i a, Vec2i b)
{
    f32 dx = static_cast<f32>(a.x - b.x);
    f32 dy = static_cast<f32>(a.y - b.y);
    return dx * dx + dy * dy;
}

bool ContainsId(const std::vector<u64>& ids, u64 id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

std::string ConceptName(ConceptTypeId id)
{
    const auto& name = ConceptTypeRegistry::Instance().GetName(id);
    if (name == "fear") return "恐惧";
    if (name == "danger") return "危险";
    if (name == "observed_flee") return "观察逃离";
    if (name == "group_danger_evidence") return "群体危险";
    if (name == "hunger") return "饥饿";
    if (name == "pain") return "疼痛";
    if (name == "death") return "死亡";
    if (name == "fire") return "火焰";
    if (name == "heat") return "热";
    if (name == "smoke") return "烟雾";
    if (name == "food") return "食物";
    if (name == "water") return "水";
    if (name == "wood") return "木头";
    if (name == "stone") return "石头";
    if (name == "grass") return "草";
    return name.empty() ? "未知概念" : name;
}

std::string GroupKnowledgeName(GroupKnowledgeTypeId id)
{
    const auto& name = GroupKnowledgeRegistry::Instance().GetName(id);
    if (name == "shared_danger_zone") return "共享危险区域";
    if (name == "shared_fire_benefit") return "共享火焰收益";
    return name.empty() ? "未知群体知识" : name;
}

std::string PatternName(PatternTypeId id)
{
    const auto& name = PatternRegistry::Instance().GetName(id);
    if (name == "collective_avoidance") return "集体避让";
    if (name == "stable_fire_zone") return "稳定火区";
    if (name == "high_frequency_path") return "高频路径";
    if (name == "aggregation_zone") return "聚集区域";
    if (name == "stable_field_zone") return "稳定场区域";
    return name.empty() ? "未知模式" : name;
}

std::string CulturalTraceName(CulturalTraceTypeId id)
{
    const auto& name = CulturalTraceRegistry::Instance().GetName(id);
    if (name == "danger_avoidance_trace") return "危险避让痕迹";
    return name.empty() ? "未知文化痕迹" : name;
}

std::string HistoryEventName(HistoryTypeId id)
{
    const auto& name = HistoryRegistry::Instance().GetName(id);
    if (name == "first_shared_danger_memory") return "第一次共享危险记忆";
    if (name == "first_collective_avoidance") return "第一次集体避让";
    if (name == "first_danger_avoidance_trace") return "第一次危险避让痕迹";
    if (name == "first_stable_fire_usage") return "第一次稳定用火";
    if (name == "mass_death") return "群体死亡事件";
    return name.empty() ? "未知历史事件" : name;
}

bool TraceTouchesCell(const WorldState& world, const CulturalTraceRecord& trace, i32 x, i32 y)
{
    for (const auto& pattern : world.Patterns().All())
    {
        if (ContainsId(trace.sourcePatternIds, pattern.id) && pattern.x == x && pattern.y == y)
            return true;
    }

    Vec2i cell{x, y};
    for (const auto& knowledge : world.GroupKnowledge().records)
    {
        if (ContainsId(trace.sourceGroupKnowledgeRecordIds, knowledge.id) &&
            DistanceSquared(cell, knowledge.origin) <= knowledge.radius * knowledge.radius)
            return true;
    }

    return false;
}

const char* PatternChar(const PatternRecord& pattern)
{
    const auto& name = PatternRegistry::Instance().GetName(pattern.typeId);
    if (name == "collective_avoidance") return "避";
    if (name == "high_frequency_path") return "路";
    if (name == "aggregation_zone") return "聚";
    if (name == "stable_fire_zone") return "稳";
    if (name == "stable_field_zone") return "场";
    return "型";
}

const char* SocialSignalChar(const SocialSignal& signal)
{
    const auto& name = SocialSignalRegistry::Instance().GetName(signal.typeId);
    if (name == "fear") return "惧";
    if (name == "death_warning") return "警";
    return "讯";
}

std::string FormatTopFocus(const WorldState& world, EntityId agentId)
{
    std::vector<const FocusedStimulus*> focused;
    for (const auto& focus : world.Cognitive().frameFocused)
    {
        if (focus.stimulus.observerId == agentId)
            focused.push_back(&focus);
    }

    std::sort(focused.begin(), focused.end(),
        [](const FocusedStimulus* lhs, const FocusedStimulus* rhs) {
            return lhs->attentionScore > rhs->attentionScore;
        });

    if (focused.empty()) return "无";

    std::ostringstream out;
    size_t shown = std::min<size_t>(focused.size(), 3);
    for (size_t index = 0; index < shown; ++index)
    {
        const auto& focus = *focused[index];
        if (index > 0) out << "  ";
        out << ConceptName(focus.stimulus.concept)
            << "@" << focus.stimulus.location.x << "," << focus.stimulus.location.y
            << " s=" << std::fixed << std::setprecision(1) << focus.attentionScore;
    }
    return out.str();
}

std::string FormatTopMemories(const WorldState& world, EntityId agentId)
{
    std::vector<const MemoryRecord*> memories;
    for (const auto& memory : world.Cognitive().GetAgentMemories(agentId))
        memories.push_back(&memory);

    std::sort(memories.begin(), memories.end(),
        [](const MemoryRecord* lhs, const MemoryRecord* rhs) {
            return lhs->strength > rhs->strength;
        });

    if (memories.empty()) return "无";

    std::ostringstream out;
    size_t shown = std::min<size_t>(memories.size(), 3);
    for (size_t index = 0; index < shown; ++index)
    {
        const auto& memory = *memories[index];
        if (index > 0) out << "  ";
        out << ConceptName(memory.subject)
            << "=" << std::fixed << std::setprecision(1) << memory.strength;
    }
    return out.str();
}

std::string FormatKnowledgeSummary(const WorldState& world, EntityId agentId)
{
    std::istringstream input(world.Cognitive().knowledgeGraph.Dump(agentId));
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(input, line))
    {
        if (!line.empty())
            lines.push_back(line);
    }

    if (lines.empty()) return "无";

    std::ostringstream out;
    size_t shown = std::min<size_t>(lines.size(), 2);
    for (size_t index = 0; index < shown; ++index)
    {
        if (index > 0) out << " | ";
        out << lines[index];
    }
    return out.str();
}
}

void AsciiViewer::Run(WorldState& world,
                      const HumanEvolution::EnvironmentContext& envCtx,
                      Scheduler& scheduler,
                      i32 totalTicks,
                      i32 sleepMs,
                      PermanentFireConfig permanentFire,
                      PermanentFoodConfig permanentFood)
{
    MaintainPermanentFood(world, permanentFood);
    for (i32 i = 0; i < totalTicks; ++i)
    {
        scheduler.Tick(world);
        MaintainPermanentFire(world, envCtx, permanentFire);
        MaintainPermanentFood(world, permanentFood);
        GenerateLogs(world, envCtx);
        Render(world, envCtx);
        if (sleepMs > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }
}

void AsciiViewer::Render(WorldState& world, const HumanEvolution::EnvironmentContext& envCtx)
{
    std::ostringstream out;

    // ANSI clear screen + scrollback, then move cursor to home
    out << "\033[2J\033[3J\033[H";

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

    size_t aliveAgents = 0;
    for (const auto& agent : world.Agents().agents)
        if (agent.alive) ++aliveAgents;

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
            out << "  Agent_" << agent.id << " [已死亡] 位置("
                << agent.position.x << "," << agent.position.y << ")\n";
            continue;
        }
        out << "  Agent_" << agent.id;
        out << " 位置(" << agent.position.x << "," << agent.position.y << ")";
        out << " 饥饿" << static_cast<int>(agent.hunger);
        out << " 生命" << static_cast<int>(agent.health);
        out << " 行为:" << ActionName(agent.currentAction);
        out << "\n";
        out << "    Focus: " << FormatTopFocus(world, agent.id) << "\n";
        out << "    Memory: " << FormatTopMemories(world, agent.id) << "\n";
        out << "    Knowledge: " << FormatKnowledgeSummary(world, agent.id) << "\n";
    }

    out << "\n";

    // Legend
    out << "【图例】";
    out << "人=活体 ";
    out << "亡=死亡 ";
    out << "迹=文化痕迹 ";
    out << "避=集体避让 ";
    out << "路=路径 ";
    out << "聚=聚集 ";
    out << "稳=稳定火区 ";
    out << "场=稳定场 ";
    out << "型=其他模式 ";
    out << "惧=恐惧信号 ";
    out << "炎=大火 ";
    out << "火=中火 ";
    out << "焰=小火 ";
    out << "石=热石 ";
    out << "食=食物 ";
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
    out << "  智能体:" << aliveAgents << "/" << world.Agents().agents.size();
    out << "  生态实体:" << world.Ecology().entities.Count();
    out << "  社会信号:" << world.SocialSignals().Count();
    out << "  群体知识:" << world.GroupKnowledge().records.size();
    out << "  模式:" << world.Patterns().Count();
    out << "  文化痕迹:" << world.CulturalTrace().records.size();
    out << "  历史事件:" << world.History().Count();
    out << "\n\n";

    // Logs
    out << "【日志】\n";
    for (const auto& log : logs_)
    {
        out << "  " << log << "\n";
    }

    std::cout << out.str() << std::flush;
}

void AsciiViewer::MaintainPermanentFire(WorldState& world,
                                        const HumanEvolution::EnvironmentContext& envCtx,
                                        const PermanentFireConfig& permanentFire)
{
    if (!permanentFire.enabled) return;

    auto& fields = world.Fields();
    auto* fire = fields.GetField2D(envCtx.fire);
    auto* temperature = fields.GetField2D(envCtx.temperature);
    auto* humidity = fields.GetField2D(envCtx.humidity);
    auto* danger = fields.GetField2D(envCtx.danger);
    if (!fire) return;

    i32 radius = std::max(0, permanentFire.radius);
    i32 radiusSq = radius * radius;
    f32 intensity = std::max(0.0f, permanentFire.intensity);

    for (i32 y = permanentFire.y - radius; y <= permanentFire.y + radius; ++y)
    {
        for (i32 x = permanentFire.x - radius; x <= permanentFire.x + radius; ++x)
        {
            if (!fields.InBounds(envCtx.fire, x, y)) continue;

            i32 dx = x - permanentFire.x;
            i32 dy = y - permanentFire.y;
            i32 distSq = dx * dx + dy * dy;
            if (distSq > radiusSq) continue;

            f32 falloff = radius > 0
                ? 1.0f - (static_cast<f32>(distSq) / static_cast<f32>(radiusSq + 1)) * 0.35f
                : 1.0f;
            f32 cellIntensity = intensity * falloff;

            fire->current.At(x, y) = std::max(fire->current.At(x, y), cellIntensity);
            fire->next.At(x, y) = std::max(fire->next.At(x, y), cellIntensity);

            if (temperature)
            {
                temperature->current.At(x, y) = std::max(temperature->current.At(x, y), 80.0f);
                temperature->next.At(x, y) = std::max(temperature->next.At(x, y), 80.0f);
            }
            if (humidity)
            {
                humidity->current.At(x, y) = std::min(humidity->current.At(x, y), 10.0f);
                humidity->next.At(x, y) = std::min(humidity->next.At(x, y), 10.0f);
            }
            if (danger)
            {
                danger->current.At(x, y) = std::max(danger->current.At(x, y), cellIntensity);
                danger->next.At(x, y) = std::max(danger->next.At(x, y), cellIntensity);
            }
        }
    }

    if (!loggedPermanentFire_)
    {
        loggedPermanentFire_ = true;
        std::ostringstream msg;
        msg << "[第" << world.Sim().clock.currentTick << "刻] 永久火源区域 pos=("
            << permanentFire.x << "," << permanentFire.y << ")"
            << " r=" << radius
            << " intensity=" << std::fixed << std::setprecision(1) << intensity;
        PushLog(msg.str());
    }
}

void AsciiViewer::MaintainPermanentFood(WorldState& world,
                                        const PermanentFoodConfig& permanentFood)
{
    if (!permanentFood.enabled) return;

    i32 radius = std::max(0, permanentFood.radius);
    i32 radiusSq = radius * radius;
    bool changed = false;

    for (i32 y = permanentFood.y - radius; y <= permanentFood.y + radius; ++y)
    {
        for (i32 x = permanentFood.x - radius; x <= permanentFood.x + radius; ++x)
        {
            if (x < 0 || y < 0 || x >= world.Width() || y >= world.Height()) continue;
            i32 dx = x - permanentFood.x;
            i32 dy = y - permanentFood.y;
            if (dx * dx + dy * dy > radiusSq) continue;

            bool found = false;
            for (auto& entity : world.Ecology().entities.All())
            {
                if (entity.x == x && entity.y == y && entity.name == "permanent_food")
                {
                    entity.material = MaterialId::Fruit;
                    entity.state = MaterialDB::GetDefaultState(MaterialId::Fruit);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                auto& food = world.Ecology().entities.Create(MaterialId::Fruit, "permanent_food");
                food.x = x;
                food.y = y;
                changed = true;
            }
        }
    }

    if (changed)
        world.RebuildSpatial();

    if (!loggedPermanentFood_)
    {
        loggedPermanentFood_ = true;
        std::ostringstream msg;
        msg << "[第" << world.Sim().clock.currentTick << "刻] 永久食物区域 pos=("
            << permanentFood.x << "," << permanentFood.y << ")"
            << " r=" << radius;
        PushLog(msg.str());
    }
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

    for (const auto& knowledge : world.GroupKnowledge().records)
    {
        if (seenGroupKnowledge_.insert(knowledge.id).second)
        {
            std::ostringstream msg;
            msg << prefix << "形成" << GroupKnowledgeName(knowledge.typeId)
                << " pos=(" << knowledge.origin.x << "," << knowledge.origin.y << ")"
                << " confidence=" << std::fixed << std::setprecision(2) << knowledge.confidence;
            PushLog(msg.str());
        }
    }

    for (const auto& pattern : world.Patterns().All())
    {
        if (seenPatterns_.insert(pattern.id).second)
        {
            std::ostringstream msg;
            msg << prefix << "出现" << PatternName(pattern.typeId)
                << "模式 pos=(" << pattern.x << "," << pattern.y << ")"
                << " confidence=" << std::fixed << std::setprecision(2) << pattern.confidence;
            PushLog(msg.str());
        }
    }

    for (const auto& trace : world.CulturalTrace().records)
    {
        if (seenCulturalTraces_.insert(trace.id).second)
        {
            std::ostringstream msg;
            msg << prefix << "形成" << CulturalTraceName(trace.typeId)
                << " confidence=" << std::fixed << std::setprecision(2) << trace.confidence;
            PushLog(msg.str());
        }
    }

    for (const auto& event : world.History().Events())
    {
        if (seenHistoryEvents_.insert(event.id).second)
        {
            std::ostringstream msg;
            msg << prefix << "历史事件: " << HistoryEventName(event.typeId)
                << " pos=(" << event.x << "," << event.y << ")"
                << " confidence=" << std::fixed << std::setprecision(2) << event.confidence;
            PushLog(msg.str());
        }
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
        case MaterialId::Fruit:        return "果";
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
    // 1. Dead agent (highest priority)
    for (const auto& agent : world.Agents().agents)
    {
        if (!agent.alive && agent.position.x == x && agent.position.y == y)
            return "亡";
    }

    // 2. Alive agent
    for (const auto& agent : world.Agents().agents)
    {
        if (agent.alive && agent.position.x == x && agent.position.y == y)
            return "人";
    }

    // 3. Pattern
    for (const auto& pattern : world.Patterns().All())
    {
        if (pattern.x == x && pattern.y == y)
            return PatternChar(pattern);
    }

    // 4. Social signal origin
    for (const auto& signal : world.SocialSignals().activeSignals)
    {
        if (signal.origin.x == x && signal.origin.y == y)
            return SocialSignalChar(signal);
    }

    // 5. Fire field
    f32 fire = world.Fields().Read(envCtx.fire, x, y);
    if (fire > 50.0f) return "炎";
    if (fire > 20.0f) return "火";
    if (fire > 0.0f)  return "焰";

    // 6. Ecology entity
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
                case MaterialId::Fruit:        return "食";
                case MaterialId::Charcoal:     return "炭";
                case MaterialId::Ash:          return "灰";
                case MaterialId::RottingPlant: return "腐";
                default:                       return "物";
            }
        }
    }

    // 7. Cultural trace background overlay
    for (const auto& trace : world.CulturalTrace().records)
    {
        if (TraceTouchesCell(world, trace, x, y))
            return "迹";
    }

    // 8. Empty
    return "　";
}
