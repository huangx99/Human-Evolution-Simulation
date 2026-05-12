#pragma once

#include "sim/spatial/chunk_coord.h"
#include "sim/spatial/spatial_chunk.h"
#include "sim/ecology/ecology_registry.h"
#include <vector>
#include <utility>

class SpatialIndex
{
public:
    SpatialIndex() = default;

    SpatialIndex(i32 worldW, i32 worldH, i32 chunkSize = 16)
        : chunkSize(chunkSize)
        , chunksX((worldW + chunkSize - 1) / chunkSize)
        , chunksY((worldH + chunkSize - 1) / chunkSize)
        , chunks(chunksX * chunksY)
    {
    }

    void Rebuild(EcologyRegistry& ecology)
    {
        for (auto& chunk : chunks)
            chunk.entities.clear();

        for (auto& entity : ecology.All())
        {
            if (entity.x >= 0 && entity.y >= 0)
            {
                auto& chunk = ChunkAt(entity.x, entity.y);
                chunk.entities.push_back(&entity);
            }
        }
    }

    void Insert(EcologyEntity& entity)
    {
        if (entity.x >= 0 && entity.y >= 0)
            ChunkAt(entity.x, entity.y).entities.push_back(&entity);
    }

    void Remove(EcologyEntity& entity)
    {
        if (entity.x < 0 || entity.y < 0) return;
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

    // All positions that have at least one entity
    std::vector<std::pair<i32, i32>> AllPositions() const
    {
        std::vector<std::pair<i32, i32>> positions;
        for (const auto& chunk : chunks)
        {
            for (const auto* e : chunk.entities)
            {
                positions.push_back({e->x, e->y});
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

    i32 ChunkCount() const { return chunksX * chunksY; }
    i32 GetChunkSize() const { return chunkSize; }

private:
    i32 chunkSize = 16;
    i32 chunksX = 0;
    i32 chunksY = 0;
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
