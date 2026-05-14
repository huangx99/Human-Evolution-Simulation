#pragma once

#include "sim/cognitive/concept_registry.h"
#include "sim/social/social_signal_registry.h"
#include "sim/group_knowledge/group_knowledge_registry.h"
#include "sim/pattern/pattern_registry.h"
#include "sim/cultural_trace/cultural_trace_registry.h"
#include "sim/history/history_registry.h"
#include "sim/cognitive/perceived_stimulus.h"
#include "sim/entity/agent_action.h"
#include <string>

// ============================================================================
// 概念名（认知层）
// ============================================================================
inline std::string ChineseConceptName(ConceptTypeId id)
{
    const auto& name = ConceptTypeRegistry::Instance().GetName(id);
    if (name == "fear")                  return "恐惧";
    if (name == "danger")                return "危险";
    if (name == "observed_flee")         return "观察到他者逃离";
    if (name == "group_danger_evidence") return "群体危险证据";
    if (name == "hunger")                return "饥饿";
    if (name == "pain")                  return "疼痛";
    if (name == "satiety")               return "饱足";
    if (name == "health")                return "健康";
    if (name == "death")                 return "死亡";
    if (name == "fire")                  return "火焰";
    if (name == "water")                 return "水";
    if (name == "wind")                  return "风";
    if (name == "rain")                  return "雨";
    if (name == "smoke")                 return "烟雾";
    if (name == "heat")                  return "热";
    if (name == "cold")                  return "冷";
    if (name == "wood")                  return "木头";
    if (name == "stone")                 return "石头";
    if (name == "grass")                 return "草";
    if (name == "meat")                  return "肉";
    if (name == "coal")                  return "煤炭";
    if (name == "warmth")                return "温暖";
    if (name == "wetness")               return "潮湿";
    if (name == "dryness")               return "干燥";
    if (name == "burning")               return "燃烧";
    if (name == "beast")                 return "野兽";
    if (name == "predator")              return "捕食者";
    if (name == "food")                  return "食物";
    if (name == "shelter")               return "庇护所";
    if (name == "tool")                  return "工具";
    if (name == "companion")             return "同伴";
    if (name == "stranger")              return "陌生人";
    if (name == "group")                 return "群体";
    if (name == "safety")                return "安全";
    if (name == "comfort")               return "舒适";
    if (name == "curiosity")             return "好奇";
    if (name == "trust")                 return "信任";
    return name.empty() ? "未知概念" : name;
}

// ============================================================================
// 社会信号名
// ============================================================================
inline std::string ChineseSocialSignalName(SocialSignalTypeId id)
{
    const auto& name = SocialSignalRegistry::Instance().GetName(id);
    if (name == "fear")           return "恐惧";
    if (name == "death_warning")  return "死亡警告";
    return name.empty() ? "未知信号" : name;
}

// ============================================================================
// 群体知识名
// ============================================================================
inline std::string ChineseGroupKnowledgeName(GroupKnowledgeTypeId id)
{
    const auto& name = GroupKnowledgeRegistry::Instance().GetName(id);
    if (name == "shared_danger_zone")  return "共享危险区域";
    if (name == "shared_fire_benefit") return "共享火焰收益";
    return name.empty() ? "未知群体知识" : name;
}

// ============================================================================
// 行为模式名
// ============================================================================
inline std::string ChinesePatternName(PatternTypeId id)
{
    const auto& name = PatternRegistry::Instance().GetName(id);
    if (name == "collective_avoidance") return "集体避让";
    if (name == "stable_fire_zone")     return "稳定火区";
    if (name == "high_frequency_path")  return "高频路径";
    if (name == "aggregation_zone")     return "聚集区域";
    if (name == "stable_field_zone")    return "稳定场区域";
    return name.empty() ? "未知模式" : name;
}

// ============================================================================
// 文化痕迹名
// ============================================================================
inline std::string ChineseCulturalTraceName(CulturalTraceTypeId id)
{
    const auto& name = CulturalTraceRegistry::Instance().GetName(id);
    if (name == "danger_avoidance_trace") return "危险避让痕迹";
    return name.empty() ? "未知文化痕迹" : name;
}

// ============================================================================
// 历史事件名
// ============================================================================
inline std::string ChineseHistoryEventName(HistoryTypeId id)
{
    const auto& name = HistoryRegistry::Instance().GetName(id);
    if (name == "first_shared_danger_memory")   return "第一次共享危险记忆";
    if (name == "first_collective_avoidance")   return "第一次集体避让";
    if (name == "first_danger_avoidance_trace") return "第一次危险避让痕迹";
    if (name == "first_stable_fire_usage")      return "第一次稳定用火";
    if (name == "mass_death")                   return "群体死亡事件";
    return name.empty() ? "未知历史事件" : name;
}

// ============================================================================
// 感知通道名
// ============================================================================
inline std::string ChineseSenseName(SenseType sense)
{
    switch (sense)
    {
        case SenseType::Vision:   return "视觉";
        case SenseType::Smell:    return "嗅觉";
        case SenseType::Touch:    return "触觉";
        case SenseType::Sound:    return "声音";
        case SenseType::Thermal:  return "热感";
        case SenseType::Internal: return "内部";
        case SenseType::Social:   return "社会";
    }
    return "未知感知";
}

// ============================================================================
// 行为名
// ============================================================================
inline std::string ChineseActionName(AgentAction action)
{
    switch (action)
    {
        case AgentAction::Idle:       return "停留";
        case AgentAction::MoveToFood: return "觅食";
        case AgentAction::Flee:       return "逃离";
        case AgentAction::Wander:     return "游走";
    }
    return "未知行为";
}
