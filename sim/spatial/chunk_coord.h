#pragma once

#include "core/types/types.h"

struct ChunkCoord
{
    i32 cx = 0;
    i32 cy = 0;

    bool operator==(const ChunkCoord& o) const { return cx == o.cx && cy == o.cy; }
    bool operator!=(const ChunkCoord& o) const { return !(*this == o); }
};
