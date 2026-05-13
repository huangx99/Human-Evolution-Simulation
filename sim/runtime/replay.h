#pragma once

#include "sim/world/world_state.h"
#include "sim/command/command_buffer.h"
#include "sim/field/field_module.h"
#include <vector>
#include <deque>
#include <unordered_map>
#include <algorithm>

// SavedField: generic field snapshot, keyed by FieldKey for cross-version stability.

struct SavedField
{
    FieldKey key;
    FieldKind kind;
    std::vector<f32> data;  // row-major for Spatial, single value for Scalar
};

// SavedWorldState: full deep copy of all primary world state.
// Used as a restoration point for command replay.

struct SavedWorldState
{
    Tick tick;
    u64 randomState[2];
    i32 width, height;

    // Agents
    std::vector<Agent> agents;
    EntityId nextAgentId;

    // Ecology entities
    std::deque<EcologyEntity> ecologyEntities;
    EntityId nextEcologyId;

    // Field snapshots (generic, keyed by FieldKey) — includes wind scalars
    std::vector<SavedField> fields;

    // Cognitive persistent state
    std::unordered_map<EntityId, std::vector<MemoryRecord>> agentMemories;
    std::unordered_map<EntityId, std::vector<Hypothesis>> agentHypotheses;
    KnowledgeGraph knowledgeGraph;
    u64 nextStimulusId, nextMemoryId, nextHypothesisId, nextDiscoveryId, nextSocialSignalId;
};

// Save full world state into a SavedWorldState.

inline SavedWorldState SaveWorldState(const WorldState& world)
{
    SavedWorldState s;
    const auto& sim = world.Sim();
    const auto& agents = world.Agents();
    const auto& eco = world.Ecology().entities;
    const auto& cog = world.Cognitive();

    s.tick = sim.clock.currentTick;
    std::memcpy(s.randomState, sim.random.GetState(), 2 * sizeof(u64));
    s.width = world.width;
    s.height = world.height;

    // Agents
    s.agents = agents.agents;
    s.nextAgentId = agents.nextEntityId;

    // Ecology
    s.ecologyEntities = eco.All();
    s.nextEcologyId = eco.NextId();

    // Fields — generic copy from FieldModule
    const auto& fm = world.Fields();
    for (const auto& entry : fm.Entries())
    {
        SavedField sf;
        sf.key = entry.key;
        sf.kind = entry.kind;
        if (entry.kind == FieldKind::Spatial && entry.spatial)
            sf.data.assign(entry.spatial->current.Data(),
                           entry.spatial->current.Data() + entry.spatial->current.DataSize());
        else
            sf.data = {entry.scalarValue};
        s.fields.push_back(std::move(sf));
    }

    // Cognitive persistent state
    s.agentMemories = cog.agentMemories;
    s.agentHypotheses = cog.agentHypotheses;
    s.knowledgeGraph = cog.knowledgeGraph;
    s.nextStimulusId = cog.nextStimulusId;
    s.nextMemoryId = cog.nextMemoryId;
    s.nextHypothesisId = cog.nextHypothesisId;
    s.nextDiscoveryId = cog.nextDiscoveryId;
    s.nextSocialSignalId = cog.nextSocialSignalId;

    return s;
}

// Restore world state from a SavedWorldState.

inline void RestoreWorld(WorldState& world, const SavedWorldState& s)
{
    auto& sim = world.Sim();
    auto& agents = world.Agents();
    auto& eco = world.Ecology().entities;
    auto& cog = world.Cognitive();

    // Clock + RNG
    sim.clock.currentTick = s.tick;
    std::memcpy(const_cast<u64*>(sim.random.GetState()), s.randomState, 2 * sizeof(u64));

    // Agents
    agents.agents = s.agents;
    agents.nextEntityId = s.nextAgentId;

    // Ecology — clear and repopulate
    eco.Clear();
    for (const auto& e : s.ecologyEntities)
        eco.InsertCopy(e);
    eco.SetNextId(s.nextEcologyId);

    // Fields — generic restore via FieldModule
    auto& fm = world.Fields();
    for (const auto& sf : s.fields)
    {
        auto idx = fm.FindByKey(sf.key);
        if (!idx) continue;
        if (sf.kind == FieldKind::Spatial)
        {
            auto* f = fm.GetField2D(idx);
            if (f)
                for (i32 y = 0; y < f->Height(); y++)
                    for (i32 x = 0; x < f->Width(); x++)
                        f->current.At(x, y) = sf.data[y * f->Width() + x];
        }
        else
        {
            fm.WriteNext(idx, 0, 0, sf.data.empty() ? 0.0f : sf.data[0]);
        }
    }

    // Cognitive persistent state
    cog.agentMemories = s.agentMemories;
    cog.agentHypotheses = s.agentHypotheses;
    cog.knowledgeGraph = s.knowledgeGraph;
    cog.nextStimulusId = s.nextStimulusId;
    cog.nextMemoryId = s.nextMemoryId;
    cog.nextHypothesisId = s.nextHypothesisId;
    cog.nextDiscoveryId = s.nextDiscoveryId;
    cog.nextSocialSignalId = s.nextSocialSignalId;

    // Clear frame buffers (they're transient)
    cog.ClearFrame();

    // Rebuild spatial index
    world.RebuildSpatial();
}

// ReplayEngine: re-apply recorded commands onto a restored world.
//
// Replay is command re-apply, NOT re-simulation.
// Does NOT run systems. Does NOT call CommandBuffer::Submit().
// Groups commands by tick, applies each group, then does SwapAll + clock.Step.

struct ReplayEngine
{
    static void Replay(WorldState& world,
                       const std::vector<QueuedCommand>& commands,
                       Tick fromTick, Tick toTick)
    {
        // Group commands by tick
        // Commands are assumed to be in chronological order
        size_t cmdIdx = 0;

        for (Tick t = fromTick; t < toTick; t++)
        {
            // Apply all commands for this tick
            CommandBuffer tempBuffer;
            while (cmdIdx < commands.size() && commands[cmdIdx].tick <= t)
            {
                tempBuffer.Push(commands[cmdIdx].tick, commands[cmdIdx].command);
                cmdIdx++;
            }
            tempBuffer.Apply(world);

            // EndTick phase: swap buffers + advance clock
            world.Fields().SwapAll();
            world.Sim().clock.Step();
        }
    }
};
