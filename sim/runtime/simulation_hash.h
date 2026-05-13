#pragma once

#include "sim/world/world_state.h"
#include "sim/command/command.h"
#include <cstring>
#include <algorithm>
#include <vector>
#include <utility>

// FNV-1a 64-bit hash — zero dependencies, deterministic, well-known.

struct SimHash
{
    u64 value = 0xcbf29ce484222325ULL;

    void Feed(const void* data, size_t size)
    {
        const auto* bytes = static_cast<const u8*>(data);
        for (size_t i = 0; i < size; i++)
        {
            value ^= bytes[i];
            value *= 0x100000001b3ULL;
        }
    }

    void FeedU64(u64 v) { Feed(&v, sizeof(v)); }
    void FeedI32(i32 v) { Feed(&v, sizeof(v)); }
    void FeedU32(u32 v) { Feed(&v, sizeof(v)); }
    void FeedU16(u16 v) { Feed(&v, sizeof(v)); }
    void FeedU8(u8 v)   { Feed(&v, sizeof(v)); }

    void FeedF32(f32 v)
    {
        u32 bits;
        std::memcpy(&bits, &v, sizeof(f32));
        FeedU32(bits);
    }
};

// Hash tiers for graduated determinism audit.

enum class HashTier : u8
{
    Core,   // RNG + tick + agents + ecology IDs
    Full,   // Core + all field buffers + wind + cognitive persistent state
    Audit   // Full + command history + event archive
};

// --- Hash helpers for individual data types ---

inline void HashAgent(SimHash& h, const Agent& a)
{
    h.FeedU64(a.id);
    h.FeedI32(a.position.x);
    h.FeedI32(a.position.y);
    h.FeedF32(a.hunger);
    h.FeedF32(a.health);
    h.FeedU8(static_cast<u8>(a.currentAction));
    h.FeedF32(a.nearestSmell);
    h.FeedF32(a.nearestFire);
    h.FeedF32(a.localTemperature);
}

inline void HashEcologyEntity(SimHash& h, const EcologyEntity& e)
{
    h.FeedU64(e.id);
    h.FeedU16(static_cast<u16>(e.material));
    h.FeedU32(static_cast<u32>(e.state));
    h.FeedU32(static_cast<u32>(e.extraCapabilities));
    h.FeedU32(static_cast<u32>(e.extraAffordances));
    h.FeedI32(e.x);
    h.FeedI32(e.y);
}

inline void HashField2D(SimHash& h, const Field2D<f32>& field)
{
    h.Feed(field.current.Data(), field.current.DataSize() * sizeof(f32));
}

inline void HashMemoryRecord(SimHash& h, const MemoryRecord& m)
{
    h.FeedU64(m.id);
    h.FeedU64(m.ownerId);
    h.FeedU8(static_cast<u8>(m.kind));
    h.FeedU32(static_cast<u32>(m.subject));
    for (auto tag : m.contextTags) h.FeedU32(static_cast<u32>(tag));
    for (auto tag : m.resultTags)  h.FeedU32(static_cast<u32>(tag));
    h.FeedI32(m.location.x);
    h.FeedI32(m.location.y);
    h.FeedF32(m.strength);
    h.FeedF32(m.emotionalWeight);
    h.FeedF32(m.confidence);
    h.FeedU64(m.createdTick);
    h.FeedU64(m.lastReinforcedTick);
    h.FeedU64(m.sourceStimulusId);
}

inline void HashHypothesis(SimHash& h, const Hypothesis& hyp)
{
    h.FeedU64(hyp.id);
    h.FeedU64(hyp.ownerId);
    h.FeedU64(hyp.groupId);
    h.FeedU32(static_cast<u32>(hyp.causeConcept));
    h.FeedU32(static_cast<u32>(hyp.effectConcept));
    h.FeedU8(static_cast<u8>(hyp.proposedRelation));
    h.FeedF32(hyp.confidence);
    h.FeedU32(hyp.supportingCount);
    h.FeedU32(hyp.contradictingCount);
    h.FeedU64(hyp.firstObservedTick);
    h.FeedU64(hyp.lastObservedTick);
    h.FeedU8(static_cast<u8>(hyp.status));
}

inline void HashKnowledgeNode(SimHash& h, const KnowledgeNode& n)
{
    h.FeedU64(n.id);
    h.FeedU64(n.ownerAgentId);
    h.FeedU64(n.groupId);
    h.FeedU32(static_cast<u32>(n.concept));
    h.FeedF32(n.strength);
    h.FeedU64(n.firstKnownTick);
    h.FeedU64(n.lastUpdatedTick);
}

inline void HashKnowledgeEdge(SimHash& h, const KnowledgeEdge& e)
{
    h.FeedU64(e.id);
    h.FeedU64(e.fromNodeId);
    h.FeedU64(e.toNodeId);
    h.FeedU8(static_cast<u8>(e.relation));
    h.FeedF32(e.confidence);
    h.FeedF32(e.strength);
    h.FeedU32(e.evidenceCount);
    h.FeedU64(e.firstObservedTick);
    h.FeedU64(e.lastObservedTick);
}

// Hash a single typed command struct via visitor
inline void HashCommandByKind(SimHash& h, const Command& cmd)
{
    h.FeedU16(static_cast<u16>(GetCommandKind(cmd)));

    std::visit([&](const auto& inner) {
        std::visit([&](const auto& typed) {
            using T = std::decay_t<decltype(typed)>;
            if constexpr (std::is_same_v<T, MoveAgentCommand>) {
                h.FeedU64(typed.id); h.FeedI32(typed.x); h.FeedI32(typed.y);
            } else if constexpr (std::is_same_v<T, SetAgentActionCommand>) {
                h.FeedU64(typed.id); h.FeedU8(static_cast<u8>(typed.action));
            } else if constexpr (std::is_same_v<T, DamageAgentCommand>) {
                h.FeedU64(typed.id); h.FeedF32(typed.amount);
            } else if constexpr (std::is_same_v<T, FeedAgentCommand>) {
                h.FeedU64(typed.id); h.FeedF32(typed.amount);
            } else if constexpr (std::is_same_v<T, ModifyHungerCommand>) {
                h.FeedU64(typed.id); h.FeedF32(typed.delta);
            } else if constexpr (std::is_same_v<T, IgniteFireCommand>) {
                h.FeedI32(typed.x); h.FeedI32(typed.y); h.FeedF32(typed.intensity);
            } else if constexpr (std::is_same_v<T, ExtinguishFireCommand>) {
                h.FeedI32(typed.x); h.FeedI32(typed.y);
            } else if constexpr (std::is_same_v<T, EmitSmellCommand>) {
                h.FeedI32(typed.x); h.FeedI32(typed.y); h.FeedF32(typed.amount);
            } else if constexpr (std::is_same_v<T, SetDangerCommand>) {
                h.FeedI32(typed.x); h.FeedI32(typed.y); h.FeedF32(typed.value);
            } else if constexpr (std::is_same_v<T, AddEntityStateCommand>) {
                h.FeedU64(typed.id); h.FeedU32(typed.state);
            } else if constexpr (std::is_same_v<T, RemoveEntityStateCommand>) {
                h.FeedU64(typed.id); h.FeedU32(typed.state);
            } else if constexpr (std::is_same_v<T, AddEntityCapabilityCommand>) {
                h.FeedU64(typed.id); h.FeedU32(typed.capability);
            } else if constexpr (std::is_same_v<T, RemoveEntityCapabilityCommand>) {
                h.FeedU64(typed.id); h.FeedU32(typed.capability);
            } else if constexpr (std::is_same_v<T, ModifyFieldValueCommand>) {
                h.FeedI32(typed.x); h.FeedI32(typed.y);
                h.FeedU64(typed.field.value);
                h.FeedI32(typed.mode); h.FeedF32(typed.value);
            } else if constexpr (std::is_same_v<T, EmitSmokeCommand>) {
                h.FeedI32(typed.x); h.FeedI32(typed.y); h.FeedF32(typed.amount);
            }
        }, inner);
    }, cmd);
}

inline void HashEvent(SimHash& h, const Event& e)
{
    h.FeedU16(static_cast<u16>(e.type));
    h.FeedU64(e.tick);
    h.FeedU64(e.entityId);
    h.FeedI32(e.x);
    h.FeedI32(e.y);
    h.FeedF32(e.value);
}

// --- Main hash function ---

inline u64 ComputeWorldHash(const WorldState& world, HashTier tier)
{
    SimHash h;

    // === Core: RNG + tick + agents + ecology ===

    const auto& sim = world.Sim();
    h.Feed(sim.random.GetState(), 2 * sizeof(u64));
    h.FeedU64(sim.clock.currentTick);
    h.FeedF32(sim.clock.tickDeltaSeconds);

    const auto& agents = world.Agents();
    h.FeedU64(agents.nextEntityId);
    for (const auto& a : agents.agents)
        HashAgent(h, a);

    const auto& eco = world.Ecology().entities;
    h.FeedU64(eco.NextId());
    for (const auto& e : eco.All())
        HashEcologyEntity(h, e);

    if (tier == HashTier::Core)
        return h.value;

    // === Full: Core + fields + wind + cognitive persistent state ===

    // Hash fields via FieldModule (single source of truth)
    const auto& fm = world.Fields();
    for (const auto& entry : fm.Entries())
    {
        h.FeedU64(entry.key.value);
        if (entry.kind == FieldKind::Spatial && entry.spatial)
            HashField2D(h, *entry.spatial);
        else
            h.FeedF32(entry.scalarValue);
    }

    const auto& cog = world.Cognitive();
    h.FeedU64(cog.nextStimulusId);
    h.FeedU64(cog.nextMemoryId);
    h.FeedU64(cog.nextHypothesisId);
    h.FeedU64(cog.nextDiscoveryId);
    h.FeedU64(cog.nextSocialSignalId);

    // Sort unordered_map by EntityId for deterministic iteration
    {
        std::vector<std::pair<EntityId, const std::vector<MemoryRecord>*>> sorted;
        for (const auto& [id, vec] : cog.agentMemories)
            sorted.emplace_back(id, &vec);
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& [id, vec] : sorted)
        {
            h.FeedU64(id);
            for (const auto& m : *vec)
                HashMemoryRecord(h, m);
        }
    }
    {
        std::vector<std::pair<EntityId, const std::vector<Hypothesis>*>> sorted;
        for (const auto& [id, vec] : cog.agentHypotheses)
            sorted.emplace_back(id, &vec);
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& [id, vec] : sorted)
        {
            h.FeedU64(id);
            for (const auto& hyp : *vec)
                HashHypothesis(h, hyp);
        }
    }

    const auto& kg = cog.knowledgeGraph;
    h.FeedU64(kg.nextNodeId);
    h.FeedU64(kg.nextEdgeId);
    for (const auto& n : kg.nodes) HashKnowledgeNode(h, n);
    for (const auto& e : kg.edges) HashKnowledgeEdge(h, e);

    if (tier == HashTier::Full)
        return h.value;

    // === Audit: Full + command history + event archive ===

    for (const auto& qc : world.commands.GetHistory())
    {
        h.FeedU64(qc.tick);
        HashCommandByKind(h, qc.command);
    }

    for (const auto& e : world.events.GetArchive())
        HashEvent(h, e);

    return h.value;
}
