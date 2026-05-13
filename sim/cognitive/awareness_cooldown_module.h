#pragma once

// AwarenessCooldownModule: per-agent cooldown for cognitive awareness stimuli.
//
// Prevents repeated stimulus emission when an agent remains near a source
// (e.g. GroupKnowledge zone, CulturalTrace) across multiple ticks.
//
// Generic design: keyed by (agentId, sourceRecordId, concept).
// Reusable for GroupKnowledge awareness, CulturalTrace awareness, etc.
//
// IN FULLHASH: cooldown state affects stimulus generation, which feeds
// Attention → Memory → Decision. This is simulation-affecting state.

#include "sim/world/module_registry.h"
#include "sim/cognitive/concept_id.h"
#include "core/types/types.h"
#include <vector>

struct AwarenessCooldownRecord
{
    EntityId agentId = 0;
    u64 sourceRecordId = 0;       // GroupKnowledgeRecord.id or CulturalTraceRecord.id
    ConceptTypeId concept;        // which concept was emitted
    Tick lastEmittedTick = 0;
};

struct AwarenessCooldownModule : public IModule
{
    std::vector<AwarenessCooldownRecord> records;

    AwarenessCooldownRecord* Find(EntityId agentId, u64 sourceRecordId, ConceptTypeId concept)
    {
        for (auto& r : records)
        {
            if (r.agentId == agentId && r.sourceRecordId == sourceRecordId && r.concept == concept)
                return &r;
        }
        return nullptr;
    }

    bool CanEmit(EntityId agentId, u64 sourceRecordId, ConceptTypeId concept,
                 Tick now, Tick cooldownTicks) const
    {
        for (const auto& r : records)
        {
            if (r.agentId == agentId && r.sourceRecordId == sourceRecordId && r.concept == concept)
                return (now - r.lastEmittedTick) >= cooldownTicks;
        }
        return true;  // no record = can emit
    }

    void MarkEmitted(EntityId agentId, u64 sourceRecordId, ConceptTypeId concept, Tick now)
    {
        for (auto& r : records)
        {
            if (r.agentId == agentId && r.sourceRecordId == sourceRecordId && r.concept == concept)
            {
                r.lastEmittedTick = now;
                return;
            }
        }
        AwarenessCooldownRecord rec;
        rec.agentId = agentId;
        rec.sourceRecordId = sourceRecordId;
        rec.concept = concept;
        rec.lastEmittedTick = now;
        records.push_back(rec);
    }
};
