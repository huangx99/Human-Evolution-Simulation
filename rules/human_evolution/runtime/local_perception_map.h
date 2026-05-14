#pragma once

// local_perception_map.h: Per-agent fog-of-war grid for pathfinding.
//
// The agent only "sees" what's within its vision radius. Unknown cells
// exist outside the explored area. The pathfinder uses this map, not
// the full FieldModule, to plan paths.

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include <vector>
#include <limits>

enum class CellKnowledge : u8
{
    Unknown,    // never perceived — pathfinder treats as passable with higher cost
    Walkable,   // perceived and safe
    Blocked     // perceived and impassable (fire, lethal temp, out-of-bounds)
};

struct PerceivedCell
{
    CellKnowledge knowledge = CellKnowledge::Unknown;
    f32 risk = 0.0f;  // 0..1, meaningful only for Walkable cells
};

struct LocalPerceptionMap
{
    i32 width = 0;
    i32 height = 0;
    std::vector<PerceivedCell> cells;

    void Init(i32 w, i32 h)
    {
        width = w;
        height = h;
        cells.assign(static_cast<size_t>(w) * h, PerceivedCell{});
    }

    bool InBounds(Vec2i pos) const
    {
        return pos.x >= 0 && pos.y >= 0 && pos.x < width && pos.y < height;
    }

    i32 CellIndex(Vec2i pos) const
    {
        return pos.y * width + pos.x;
    }

    PerceivedCell Get(Vec2i pos) const
    {
        if (!InBounds(pos)) return {CellKnowledge::Blocked, 1.0f};
        return cells[CellIndex(pos)];
    }

    void UpdateCell(Vec2i pos, CellKnowledge k, f32 risk)
    {
        if (!InBounds(pos)) return;
        auto& cell = cells[CellIndex(pos)];
        cell.knowledge = k;
        cell.risk = risk;
    }

    bool IsWalkable(Vec2i pos) const
    {
        return Get(pos).knowledge == CellKnowledge::Walkable;
    }

    bool IsBlocked(Vec2i pos) const
    {
        return Get(pos).knowledge == CellKnowledge::Blocked;
    }

    bool IsUnknown(Vec2i pos) const
    {
        return Get(pos).knowledge == CellKnowledge::Unknown;
    }
};
