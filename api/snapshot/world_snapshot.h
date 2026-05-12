#pragma once

#include "core/types/types.h"
#include "sim/world/world_state.h"
#include "sim/cognitive/concept_tag.h"
#include "sim/cognitive/memory_record.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

struct AgentSnapshot
{
    EntityId id;
    i32 x, y;
    f32 hunger;
    f32 health;
    u8 action;
};

struct EntitySnapshot
{
    EntityId id;
    std::string name;
    i32 x, y;
    u32 material;
    u32 state;
    u32 capabilities;
    u32 affordances;
};

struct EventSnapshot
{
    u16 type;
    Tick tick;
    EntityId entityId;
    i32 x, y;
    f32 value;
};

struct SpatialStats
{
    i32 chunkCount;
    i32 chunkSize;
    i32 occupiedPositions;
    i32 totalEntities;
    bool initialized;
};

struct CommandStats
{
    size_t historyCount;
    bool spatialDirty;
};

struct CognitiveSnapshot
{
    // Per-agent memory counts
    struct AgentCognitiveState
    {
        EntityId id;
        size_t shortTermCount;
        size_t episodicCount;
        size_t patternCount;
        size_t socialCount;
        size_t traumaCount;
        size_t hypothesisCount;
        size_t knowledgeNodeCount;
    };

    std::vector<AgentCognitiveState> agentStates;

    // Knowledge graph summary
    size_t totalKnowledgeNodes;
    size_t totalKnowledgeEdges;

    // Frame activity
    size_t frameStimuliCount;
    size_t frameFocusedCount;
    size_t frameMemoriesCount;
    size_t frameDiscoveriesCount;
    size_t frameSocialSignalsCount;
};

struct WorldSnapshot
{
    Tick tick;
    u64 randomState[2];
    i32 width, height;
    f32 windX, windY;

    struct CellSample
    {
        i32 x, y;
        f32 temperature;
        f32 humidity;
        f32 fire;
        f32 smell;
        f32 danger;
    };

    std::vector<CellSample> hotCells;
    std::vector<AgentSnapshot> agents;

    // Ecology entities
    std::vector<EntitySnapshot> entities;

    // Event archive (last N events)
    std::vector<EventSnapshot> events;
    size_t eventPendingCount = 0;

    // Spatial stats
    SpatialStats spatial;

    // Command stats
    CommandStats commands;

    // Cognitive state
    CognitiveSnapshot cognitive;

    static WorldSnapshot Capture(const WorldState& world)
    {
        WorldSnapshot snap;
        snap.tick = world.Sim().clock.currentTick;
        snap.width = world.Env().width;
        snap.height = world.Env().height;
        snap.windX = world.Env().wind.x;
        snap.windY = world.Env().wind.y;

        const u64* rstate = world.Sim().random.GetState();
        snap.randomState[0] = rstate[0];
        snap.randomState[1] = rstate[1];

        // Hot cells
        for (i32 y = 0; y < snap.height; y++)
        {
            for (i32 x = 0; x < snap.width; x++)
            {
                f32 f = world.Env().fire.At(x, y);
                f32 s = world.Info().smell.At(x, y);
                f32 d = world.Info().danger.At(x, y);
                if (f > 0.0f || s > 1.0f || d > 0.0f)
                {
                    snap.hotCells.push_back({
                        x, y,
                        world.Env().temperature.At(x, y),
                        world.Env().humidity.At(x, y),
                        f, s, d
                    });
                }
            }
        }

        // Agents
        for (const auto& agent : world.Agents().agents)
        {
            snap.agents.push_back({
                agent.id,
                agent.position.x, agent.position.y,
                agent.hunger, agent.health,
                static_cast<u8>(agent.currentAction)
            });
        }

        // Ecology entities
        for (const auto& e : world.Ecology().entities.All())
        {
            snap.entities.push_back({
                e.id,
                e.name,
                e.x, e.y,
                static_cast<u32>(e.material),
                static_cast<u32>(e.state),
                static_cast<u32>(e.GetCapabilities()),
                static_cast<u32>(e.GetAffordances()),
            });
        }

        // Event archive (last 100 events to avoid huge snapshots)
        const auto& archive = world.events.GetArchive();
        size_t start = archive.size() > 100 ? archive.size() - 100 : 0;
        for (size_t i = start; i < archive.size(); i++)
        {
            const auto& e = archive[i];
            snap.events.push_back({
                static_cast<u16>(e.type),
                e.tick,
                e.entityId,
                e.x, e.y,
                e.value,
            });
        }
        snap.eventPendingCount = world.events.PendingCount();

        // Spatial stats
        snap.spatial.chunkCount = world.spatial.ChunkCount();
        snap.spatial.chunkSize = world.spatial.GetChunkSize();
        snap.spatial.occupiedPositions = static_cast<i32>(world.spatial.AllPositions().size());
        snap.spatial.totalEntities = static_cast<i32>(world.Ecology().entities.Count());
        snap.spatial.initialized = world.spatial.IsInitialized();

        // Command stats
        snap.commands.historyCount = world.commands.GetHistory().size();
        snap.commands.spatialDirty = world.commands.IsSpatialDirty();

        // Cognitive state
        {
            auto& cog = world.Cognitive();
            snap.cognitive.totalKnowledgeNodes = cog.knowledgeGraph.NodeCount();
            snap.cognitive.totalKnowledgeEdges = cog.knowledgeGraph.EdgeCount();
            snap.cognitive.frameStimuliCount = cog.frameStimuli.size();
            snap.cognitive.frameFocusedCount = cog.frameFocused.size();
            snap.cognitive.frameMemoriesCount = cog.frameMemories.size();
            snap.cognitive.frameDiscoveriesCount = cog.frameDiscoveries.size();
            snap.cognitive.frameSocialSignalsCount = cog.frameSocialSignals.size();

            for (const auto& agent : world.Agents().agents)
            {
                CognitiveSnapshot::AgentCognitiveState acs;
                acs.id = agent.id;

                const auto& mems = cog.GetAgentMemories(agent.id);
                acs.shortTermCount = 0;
                acs.episodicCount = 0;
                acs.patternCount = 0;
                acs.socialCount = 0;
                acs.traumaCount = 0;
                for (const auto& m : mems)
                {
                    switch (m.kind)
                    {
                    case MemoryKind::ShortTerm: acs.shortTermCount++; break;
                    case MemoryKind::Episodic:  acs.episodicCount++; break;
                    case MemoryKind::Pattern:   acs.patternCount++; break;
                    case MemoryKind::Social:    acs.socialCount++; break;
                    case MemoryKind::Trauma:    acs.traumaCount++; break;
                    }
                }

                auto hypIt = cog.agentHypotheses.find(agent.id);
                acs.hypothesisCount = (hypIt != cog.agentHypotheses.end())
                    ? hypIt->second.size() : 0;

                acs.knowledgeNodeCount = 0;
                for (const auto& n : cog.knowledgeGraph.nodes)
                {
                    if (n.ownerAgentId == agent.id)
                        acs.knowledgeNodeCount++;
                }

                snap.cognitive.agentStates.push_back(acs);
            }
        }

        return snap;
    }

    std::string Serialize() const
    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(2);

        os << "tick=" << tick << "\n";
        os << "random_state=" << randomState[0] << "," << randomState[1] << "\n";
        os << "wind=" << windX << "," << windY << "\n";

        // Hot cells
        os << "hot_cells=" << hotCells.size() << "\n";
        for (const auto& c : hotCells)
        {
            os << "  cell " << c.x << "," << c.y
               << " t=" << c.temperature
               << " h=" << c.humidity
               << " f=" << c.fire
               << " s=" << c.smell
               << " d=" << c.danger << "\n";
        }

        // Agents
        os << "agents=" << agents.size() << "\n";
        for (const auto& a : agents)
        {
            os << "  agent " << a.id
               << " pos=" << a.x << "," << a.y
               << " hunger=" << a.hunger
               << " health=" << a.health
               << " action=" << static_cast<i32>(a.action) << "\n";
        }

        // Ecology entities
        os << "entities=" << entities.size() << "\n";
        for (const auto& e : entities)
        {
            os << "  entity " << e.id
               << " [" << e.name << "]"
               << " pos=" << e.x << "," << e.y
               << " mat=" << e.material
               << " state=0x" << std::hex << e.state << std::dec
               << " cap=0x" << std::hex << e.capabilities << std::dec
               << " aff=0x" << std::hex << e.affordances << std::dec << "\n";
        }

        // Events (recent)
        os << "events=" << events.size()
           << " (pending=" << eventPendingCount << ")\n";
        for (const auto& e : events)
        {
            os << "  event type=" << e.type
               << " tick=" << e.tick
               << " pos=" << e.x << "," << e.y
               << " val=" << e.value << "\n";
        }

        // Spatial stats
        os << "spatial: chunks=" << spatial.chunkCount
           << " size=" << spatial.chunkSize
           << " occupied=" << spatial.occupiedPositions
           << " entities=" << spatial.totalEntities
           << " init=" << spatial.initialized << "\n";

        // Command stats
        os << "commands: history=" << commands.historyCount
           << " spatial_dirty=" << commands.spatialDirty << "\n";

        // Cognitive state
        os << "cognitive: nodes=" << cognitive.totalKnowledgeNodes
           << " edges=" << cognitive.totalKnowledgeEdges
           << " frame_stimuli=" << cognitive.frameStimuliCount
           << " frame_focused=" << cognitive.frameFocusedCount
           << " frame_memories=" << cognitive.frameMemoriesCount
           << " frame_discoveries=" << cognitive.frameDiscoveriesCount
           << " frame_signals=" << cognitive.frameSocialSignalsCount << "\n";

        for (const auto& acs : cognitive.agentStates)
        {
            os << "  agent " << acs.id
               << " stm=" << acs.shortTermCount
               << " epi=" << acs.episodicCount
               << " pat=" << acs.patternCount
               << " soc=" << acs.socialCount
               << " trm=" << acs.traumaCount
               << " hyp=" << acs.hypothesisCount
               << " kn=" << acs.knowledgeNodeCount << "\n";
        }

        // Knowledge edges (if any)
        if (cognitive.totalKnowledgeEdges > 0)
        {
            // Note: we'd need the full KnowledgeGraph to serialize edges,
            // which isn't available through the snapshot.
            // For now, just show the count.
        }

        return os.str();
    }
};
