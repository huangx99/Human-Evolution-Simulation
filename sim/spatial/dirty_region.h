#pragma once

#include "sim/spatial/chunk_coord.h"
#include <vector>
#include <algorithm>

// Placeholder for step 2: dirty region tracking.
// Currently unused — spatial index does full scan of occupied chunks.
// Future: track which chunks have entity changes to skip unchanged regions.

class DirtyRegion
{
public:
    void Mark(ChunkCoord coord)
    {
        auto it = std::find(dirty.begin(), dirty.end(), coord);
        if (it == dirty.end())
            dirty.push_back(coord);
    }

    bool IsDirty(ChunkCoord coord) const
    {
        return std::find(dirty.begin(), dirty.end(), coord) != dirty.end();
    }

    void Clear() { dirty.clear(); }

    const std::vector<ChunkCoord>& GetDirty() const { return dirty; }

private:
    std::vector<ChunkCoord> dirty;
};
