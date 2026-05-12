#pragma once

#include "api/snapshot/world_snapshot.h"

// SnapshotProvider: the ONLY interface frontend code should use.
// Frontend MUST NOT include world_state.h or any sim/ headers directly.
//
// Contract:
//   - Frontend reads Snapshot (const ref)
//   - Frontend sends Command (future)
//   - Frontend NEVER modifies WorldState
//
// This boundary ensures:
//   - Simulation Core can run without any frontend
//   - Frontend can be replaced (Console, Unity, Web) without changing sim
//   - Simulation remains deterministic regardless of frontend behavior

class SnapshotProvider
{
public:
    virtual ~SnapshotProvider() = default;

    // Get current world snapshot (read-only)
    virtual const WorldSnapshot& GetSnapshot() const = 0;

    // Check if snapshot has changed since last call
    virtual bool IsDirty() const = 0;

    // Mark snapshot as read
    virtual void ClearDirty() = 0;
};

// Concrete implementation that wraps WorldState
class WorldSnapshotProvider : public SnapshotProvider
{
public:
    explicit WorldSnapshotProvider(const WorldState& world)
        : world(world), dirty(true)
    {
    }

    const WorldSnapshot& GetSnapshot() const override
    {
        if (dirty)
        {
            cachedSnapshot = WorldSnapshot::Capture(world);
            dirty = false;
        }
        return cachedSnapshot;
    }

    bool IsDirty() const override { return dirty; }
    void ClearDirty() override { dirty = false; }

    void MarkDirty() { dirty = true; }

private:
    const WorldState& world;
    mutable WorldSnapshot cachedSnapshot;
    mutable bool dirty;
};
