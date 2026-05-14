#pragma once

// CulturalTraceModule: stores emergent cultural traces.
//
// Traces are derived data — they describe stable group behavior patterns
// that have crystallized into cultural seeds. Not included in FullHash.
//
// Lifecycle:
//   - CulturalTraceDetectionSystem scans GroupKnowledge + Pattern each tick
//   - Creates new traces when sources align (spatial + threshold)
//   - Reinforces existing traces when new evidence arrives
//   - Dedup: same typeId + same sourceGroupKnowledgeRecordId → reinforce

#include "sim/world/module_registry.h"
#include "sim/cultural_trace/cultural_trace_record.h"
#include <vector>
#include <algorithm>

struct CulturalTraceModule : public IModule
{
    std::vector<CulturalTraceRecord> records;
    u64 nextId = 1;

    // Query: all traces of a given type
    std::vector<const CulturalTraceRecord*> FindByType(CulturalTraceTypeId typeId) const
    {
        std::vector<const CulturalTraceRecord*> result;
        for (const auto& rec : records)
        {
            if (rec.typeId == typeId)
                result.push_back(&rec);
        }
        return result;
    }

    // Create or reinforce a trace.
    // Dedup: if a trace of the same typeId exists with overlapping sourceGKRecordIds,
    // reinforce it instead of creating a new one.
    CulturalTraceRecord& AddOrReinforce(CulturalTraceTypeId typeId,
                                         const std::vector<u64>& sourcePatternIds,
                                         const std::vector<u64>& sourceGKRecordIds,
                                         f32 confidence, Tick evidenceTick,
                                         Tick currentTick)
    {
        // Try to find existing trace with same typeId and overlapping GK source
        for (auto& rec : records)
        {
            if (rec.typeId != typeId) continue;

            for (u64 gkId : sourceGKRecordIds)
            {
                auto it = std::find(rec.sourceGroupKnowledgeRecordIds.begin(),
                                     rec.sourceGroupKnowledgeRecordIds.end(), gkId);
                if (it != rec.sourceGroupKnowledgeRecordIds.end())
                {
                    if (evidenceTick <= rec.lastEvidenceTick)
                        return rec;

                    // Reinforce existing trace
                    rec.confidence = std::min(rec.confidence + confidence * 0.3f, 1.0f);
                    rec.reinforcementCount++;
                    rec.lastEvidenceTick = evidenceTick;
                    rec.lastReinforcedTick = currentTick;

                    // Merge new pattern sources (avoid duplicates)
                    for (u64 pid : sourcePatternIds)
                    {
                        auto pit = std::find(rec.sourcePatternIds.begin(),
                                              rec.sourcePatternIds.end(), pid);
                        if (pit == rec.sourcePatternIds.end())
                            rec.sourcePatternIds.push_back(pid);
                    }
                    return rec;
                }
            }
        }

        // No match — create new trace
        CulturalTraceRecord rec;
        rec.id = nextId++;
        rec.typeId = typeId;
        rec.confidence = confidence;
        rec.reinforcementCount = 1;
        rec.sourcePatternIds = sourcePatternIds;
        rec.sourceGroupKnowledgeRecordIds = sourceGKRecordIds;
        rec.firstObservedTick = currentTick;
        rec.lastEvidenceTick = evidenceTick;
        rec.lastReinforcedTick = currentTick;
        records.push_back(rec);
        return records.back();
    }

    // Decay traces that haven't been reinforced within the window
    void DecayRecords(Tick currentTick, Tick decayWindow)
    {
        records.erase(
            std::remove_if(records.begin(), records.end(),
                [currentTick, decayWindow](const CulturalTraceRecord& r) {
                    if (r.lastReinforcedTick > currentTick)
                        return false;
                    return (currentTick - r.lastReinforcedTick) > decayWindow;
                }),
            records.end());
    }
};
