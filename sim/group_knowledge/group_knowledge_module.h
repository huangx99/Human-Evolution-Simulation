#pragma once

// GroupKnowledgeModule: central storage for emergent group knowledge.
//
// === DATA LIFECYCLE ===
//
// Records accumulate over simulation lifetime:
//   - GroupKnowledgeAggregationSystem scans MemoryRecords each tick
//   - Creates new GroupKnowledgeRecords when ≥2 agents converge
//   - Reinforces existing records when new evidence arrives
//   - Records decay when lastReinforcedTick falls too far behind
//
// QUERY INTERFACE:
//   - QueryArea(center, radius): returns records whose zone overlaps the query area
//   - FindByType(typeId): returns all records of a given type
//   - GetAgentContributions(agentId): returns records this agent contributed to
//
// RULE: This module is READ-ONLY for downstream systems. Only the aggregation
// system writes to it.

#include "sim/world/module_registry.h"
#include "sim/group_knowledge/group_knowledge_record.h"
#include <vector>
#include <algorithm>

struct GroupKnowledgeModule : public IModule
{
    std::vector<GroupKnowledgeRecord> records;
    u64 nextId = 1;

    // === Query interface ===

    // Returns records whose zone overlaps the query circle.
    std::vector<const GroupKnowledgeRecord*> QueryArea(Vec2i center, f32 radius) const
    {
        std::vector<const GroupKnowledgeRecord*> result;
        f32 queryRadiusSq = radius * radius;
        for (const auto& rec : records)
        {
            f32 dx = static_cast<f32>(center.x - rec.origin.x);
            f32 dy = static_cast<f32>(center.y - rec.origin.y);
            f32 distSq = dx * dx + dy * dy;
            f32 combinedRadius = radius + rec.radius;
            if (distSq <= combinedRadius * combinedRadius)
                result.push_back(&rec);
        }
        return result;
    }

    // Returns all records of a given type.
    std::vector<const GroupKnowledgeRecord*> FindByType(GroupKnowledgeTypeId typeId) const
    {
        std::vector<const GroupKnowledgeRecord*> result;
        for (const auto& rec : records)
        {
            if (rec.typeId == typeId)
                result.push_back(&rec);
        }
        return result;
    }

    // === Write interface (used only by aggregation system) ===

    GroupKnowledgeRecord& CreateRecord(GroupKnowledgeTypeId typeId, Vec2i origin,
                                        f32 radius, f32 confidence, Tick tick)
    {
        GroupKnowledgeRecord rec;
        rec.id = nextId++;
        rec.typeId = typeId;
        rec.origin = origin;
        rec.radius = radius;
        rec.confidence = confidence;
        rec.contributors = 1;
        rec.firstObservedTick = tick;
        rec.lastReinforcedTick = tick;
        records.push_back(rec);
        return records.back();
    }

    void ReinforceRecord(u64 recordId, f32 confidenceBoost, Tick tick)
    {
        for (auto& rec : records)
        {
            if (rec.id == recordId)
            {
                rec.confidence = std::min(rec.confidence + confidenceBoost, 1.0f);
                rec.contributors++;
                rec.lastReinforcedTick = tick;
                return;
            }
        }
    }

    // Remove records that haven't been reinforced within the decay window.
    void DecayRecords(Tick currentTick, Tick decayWindow)
    {
        records.erase(
            std::remove_if(records.begin(), records.end(),
                [currentTick, decayWindow](const GroupKnowledgeRecord& r) {
                    return (currentTick - r.lastReinforcedTick) > decayWindow;
                }),
            records.end());
    }
};
