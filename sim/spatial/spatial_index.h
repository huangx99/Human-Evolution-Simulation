#pragma once

// SpatialIndex: chunk-based spatial query layer for ecology entities.
//
// Current state (step 1):
//   - Divides world into fixed-size chunks (default 16x16)
//   - Provides area queries: QueryArea, QueryAreaWithCapability, QueryAreaWithState
//   - Rebuilt automatically by CommandBuffer::Apply() when ecology entities change
//   - AllPositions() replaces full grid scan in reaction system
//
// NOT yet implemented (step 2):
//   - DirtyRegion tracking (skip unchanged chunks)
//   - Incremental updates (only rebuild modified chunks)
//   - Spatial partitioning optimizations (quadtree, etc.)
//
// This is a spatial abstraction layer, not a complete performance optimization.

#include "sim/spatial/chunk_coord.h"
#include "sim/spatial/spatial_chunk.h"
#include "sim/ecology/ecology_registry.h"
#include <vector>
#include <utility>
#include <algorithm>

class SpatialIndex
{
public:
    SpatialIndex() = default;

    SpatialIndex(i32 worldW, i32 worldH, i32 chunkSize = 16)
        : worldW(worldW), worldH(worldH)
        , chunkSize(chunkSize)
        , chunksX((worldW + chunkSize - 1) / chunkSize)
        , chunksY((worldH + chunkSize - 1) / chunkSize)
        , chunks(chunksX * chunksY)
    {
    }

    bool InBounds(i32 x, i32 y) const
    {
        return x >= 0 && x < worldW && y >= 0 && y < worldH;
    }

    void Rebuild(EcologyRegistry& ecology)
    {
        for (auto& chunk : chunks)
            chunk.entities.clear();

        for (auto& entity : ecology.All())
        {
            if (InBounds(entity.x, entity.y))
            {
                ChunkAt(entity.x, entity.y).entities.push_back(&entity);
            }
        }
        initialized = true;
    }

    void Insert(EcologyEntity& entity)
    {
        if (InBounds(entity.x, entity.y))
            ChunkAt(entity.x, entity.y).entities.push_back(&entity);
    }

    void Remove(EcologyEntity& entity)
    {
        if (!InBounds(entity.x, entity.y)) return;
        auto& chunk = ChunkAt(entity.x, entity.y);
        for (auto it = chunk.entities.begin(); it != chunk.entities.end(); ++it)
        {
            if ((*it)->id == entity.id)
            {
                chunk.entities.erase(it);
                return;
            }
        }
    }

    // Unique positions that have at least one entity
    std::vector<std::pair<i32, i32>> AllPositions() const
    {
        std::vector<std::pair<i32, i32>> positions;
        for (const auto& chunk : chunks)
        {
            for (const auto* e : chunk.entities)
            {
                std::pair<i32, i32> pos = {e->x, e->y};
                if (std::find(positions.begin(), positions.end(), pos) == positions.end())
                    positions.push_back(pos);
            }
        }
        return positions;
    }

    // Entities in chunks overlapping the circle (x, y, radius)
    std::vector<EcologyEntity*> QueryArea(i32 x, i32 y, i32 radius) const
    {
        std::vector<EcologyEntity*> result;
        i32 minCX = (x - radius) / chunkSize;
        i32 maxCX = (x + radius) / chunkSize;
        i32 minCY = (y - radius) / chunkSize;
        i32 maxCY = (y + radius) / chunkSize;

        for (i32 cy = minCY; cy <= maxCY; cy++)
        {
            for (i32 cx = minCX; cx <= maxCX; cx++)
            {
                if (!ChunkInBounds(cx, cy)) continue;
                for (auto* e : chunks[cy * chunksX + cx].entities)
                    result.push_back(e);
            }
        }
        return result;
    }

    std::vector<EcologyEntity*> QueryAreaWithCapability(i32 x, i32 y, i32 radius, Capability cap) const
    {
        std::vector<EcologyEntity*> result;
        i32 minCX = (x - radius) / chunkSize;
        i32 maxCX = (x + radius) / chunkSize;
        i32 minCY = (y - radius) / chunkSize;
        i32 maxCY = (y + radius) / chunkSize;

        for (i32 cy = minCY; cy <= maxCY; cy++)
        {
            for (i32 cx = minCX; cx <= maxCX; cx++)
            {
                if (!ChunkInBounds(cx, cy)) continue;
                for (auto* e : chunks[cy * chunksX + cx].entities)
                    if (e->HasCapability(cap)) result.push_back(e);
            }
        }
        return result;
    }

    std::vector<EcologyEntity*> QueryAreaWithState(i32 x, i32 y, i32 radius, MaterialState s) const
    {
        std::vector<EcologyEntity*> result;
        i32 minCX = (x - radius) / chunkSize;
        i32 maxCX = (x + radius) / chunkSize;
        i32 minCY = (y - radius) / chunkSize;
        i32 maxCY = (y + radius) / chunkSize;

        for (i32 cy = minCY; cy <= maxCY; cy++)
        {
            for (i32 cx = minCX; cx <= maxCX; cx++)
            {
                if (!ChunkInBounds(cx, cy)) continue;
                for (auto* e : chunks[cy * chunksX + cx].entities)
                    if (e->HasState(s)) result.push_back(e);
            }
        }
        return result;
    }

    bool IsInitialized() const { return initialized; }
    i32 ChunkCount() const { return chunksX * chunksY; }
    i32 GetChunkSize() const { return chunkSize; }

private:
    i32 worldW = 0;
    i32 worldH = 0;
    i32 chunkSize = 16;
    i32 chunksX = 0;
    i32 chunksY = 0;
    bool initialized = false;
    std::vector<SpatialChunk> chunks;

    SpatialChunk& ChunkAt(i32 x, i32 y)
    {
        return chunks[(y / chunkSize) * chunksX + (x / chunkSize)];
    }

    const SpatialChunk& ChunkAt(i32 x, i32 y) const
    {
        return chunks[(y / chunkSize) * chunksX + (x / chunkSize)];
    }

    bool ChunkInBounds(i32 cx, i32 cy) const
    {
        return cx >= 0 && cx < chunksX && cy >= 0 && cy < chunksY;
    }
};
