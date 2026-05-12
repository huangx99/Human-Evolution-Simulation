#pragma once

// DirtyRegion: tracks which spatial chunks have been modified.
//
// STATUS: Placeholder for step 2. Currently unused.
//
// Future use: SpatialIndex will skip rebuilding/querying unchanged chunks.
// Dirty sources: entity creation/removal, state changes, capability changes,
// entity movement. Environment field changes (temperature, humidity, etc.)
// are NOT tracked here — they use the double-buffer pattern independently.
//
// This is part of the spatial abstraction layer, not a standalone feature.

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
