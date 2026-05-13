#pragma once

// GroupKnowledgeAggregationSystem: scans agent memories and creates/reinforces
// GroupKnowledgeRecords when ≥2 agents have convergent danger-related observations.
//
// Aggregation rules:
//   1. Only danger-related memories participate:
//      - Concept has ConceptSemanticFlag::Danger, OR
//      - Concept == fear, OR concept == observedFlee
//   2. Memories must be within a spatial cluster radius
//   3. ≥2 DIFFERENT agents must contribute to create/merge a record
//   4. Old memories (outside time window) don't participate
//   5. Nearby existing records get reinforced; distant ones don't merge
//   6. Same evidence batch does NOT reinforce every tick (evidenceTick guard)
//
// PHASE: SimPhase::Analysis (after memory consolidation, before pattern detection)
//
// READS: Cognitive (memories), Simulation (clock)
// WRITES: GroupKnowledge
// Does NOT write to Command, Memory, or Agent modules.

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/concept_registry.h"
#include "rules/human_evolution/human_evolution_context.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <set>

class GroupKnowledgeAggregationSystem : public ISystem
{
public:
    GroupKnowledgeAggregationSystem(const HumanEvolution::ConceptContext& concepts,
                                     GroupKnowledgeTypeId dangerZoneType)
        : concepts_(concepts), dangerZoneType_(dangerZoneType) {}

    void Update(SystemContext& ctx) override
    {
        auto& cog = ctx.Cognitive();
        auto& gk = ctx.GroupKnowledge();
        Tick currentTick = ctx.CurrentTick();

        // Collect all qualifying danger memories from all agents
        struct QualifyingMemory
        {
            EntityId agentId;
            Vec2i location;
            f32 strength;
            f32 confidence;
            Tick createdTick;
        };
        std::vector<QualifyingMemory> qualifying;

        // Sort agent memory entries by EntityId for deterministic iteration
        std::vector<std::pair<EntityId, const std::vector<MemoryRecord>*>> sortedAgents;
        for (const auto& [agentId, memories] : cog.agentMemories)
            sortedAgents.emplace_back(agentId, &memories);
        std::sort(sortedAgents.begin(), sortedAgents.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        for (const auto& [agentId, memories] : sortedAgents)
        {
            for (const auto& mem : *memories)
            {
                if (!IsDangerRelated(mem.subject)) continue;
                if (currentTick - mem.createdTick > timeWindow) continue;
                if (mem.strength < minStrength) continue;

                qualifying.push_back({
                    agentId,
                    mem.location,
                    mem.strength,
                    mem.confidence,
                    mem.createdTick
                });
            }
        }

        if (qualifying.size() < 2) return;

        // Sort qualifying memories for deterministic clustering
        std::sort(qualifying.begin(), qualifying.end(),
            [](const auto& a, const auto& b)
            {
                if (a.location.x != b.location.x) return a.location.x < b.location.x;
                if (a.location.y != b.location.y) return a.location.y < b.location.y;
                if (a.agentId != b.agentId) return a.agentId < b.agentId;
                return a.createdTick < b.createdTick;
            });

        // Cluster qualifying memories by spatial proximity
        // Simple O(n^2) clustering — adequate for small agent counts
        std::vector<bool> used(qualifying.size(), false);

        for (size_t i = 0; i < qualifying.size(); ++i)
        {
            if (used[i]) continue;

            // Start a new cluster centered on memory i
            std::vector<size_t> cluster = {i};
            used[i] = true;

            for (size_t j = i + 1; j < qualifying.size(); ++j)
            {
                if (used[j]) continue;

                f32 dx = static_cast<f32>(qualifying[j].location.x - qualifying[i].location.x);
                f32 dy = static_cast<f32>(qualifying[j].location.y - qualifying[i].location.y);
                f32 dist = std::sqrt(dx * dx + dy * dy);

                if (dist <= clusterRadius)
                {
                    cluster.push_back(j);
                    used[j] = true;
                }
            }

            // Check if cluster has ≥2 different agents
            std::set<EntityId> uniqueAgents;
            for (size_t idx : cluster)
                uniqueAgents.insert(qualifying[idx].agentId);

            if (uniqueAgents.size() < 2) continue;

            // Compute cluster centroid, aggregate confidence, newest evidence tick
            f32 sumX = 0, sumY = 0, sumConf = 0;
            Tick newestEvidenceTick = 0;
            for (size_t idx : cluster)
            {
                sumX += static_cast<f32>(qualifying[idx].location.x);
                sumY += static_cast<f32>(qualifying[idx].location.y);
                sumConf += qualifying[idx].confidence;
                if (qualifying[idx].createdTick > newestEvidenceTick)
                    newestEvidenceTick = qualifying[idx].createdTick;
            }
            Vec2i centroid = {
                static_cast<i32>(sumX / cluster.size()),
                static_cast<i32>(sumY / cluster.size())
            };
            f32 avgConfidence = sumConf / static_cast<f32>(cluster.size());
            u32 contributorCount = static_cast<u32>(uniqueAgents.size());

            // Check if an existing record is nearby for reinforcement
            bool reinforced = false;
            for (auto& rec : gk.records)
            {
                if (rec.typeId != dangerZoneType_) continue;

                f32 dx = static_cast<f32>(centroid.x - rec.origin.x);
                f32 dy = static_cast<f32>(centroid.y - rec.origin.y);
                f32 dist = std::sqrt(dx * dx + dy * dy);

                if (dist <= mergeRadius)
                {
                    gk.ReinforceRecord(rec.id, avgConfidence * reinforceFactor,
                                       contributorCount, newestEvidenceTick, currentTick);
                    reinforced = true;
                    break;
                }
            }

            if (!reinforced)
            {
                auto& rec = gk.CreateRecord(dangerZoneType_, centroid, clusterRadius,
                                             avgConfidence, currentTick);
                rec.contributors = contributorCount;
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Simulation, AccessMode::Read},
            {ModuleTag::Cognitive, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::GroupKnowledge, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"GroupKnowledgeAggregationSystem", SimPhase::Analysis, READS, 2, WRITES, 1, DEPS, 0, true, false};
    }

private:
    HumanEvolution::ConceptContext concepts_;
    GroupKnowledgeTypeId dangerZoneType_;

    static constexpr f32 clusterRadius = 8.0f;
    static constexpr f32 mergeRadius = 5.0f;
    static constexpr f32 minStrength = 0.1f;
    static constexpr f32 reinforceFactor = 0.3f;
    static constexpr Tick timeWindow = 50;

    bool IsDangerRelated(ConceptTypeId concept) const
    {
        if (concept == concepts_.fear) return true;
        if (concept == concepts_.observedFlee) return true;

        const auto& reg = ConceptTypeRegistry::Instance();
        return reg.HasFlag(concept, ConceptSemanticFlag::Danger);
    }
};
