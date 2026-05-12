#pragma once

#include "core/types/types.h"
#include <cstdint>

// Bitfield: conditions a material cell can be in
enum class MaterialState : u32
{
    None        = 0,

    // Temperature states
    Frozen      = 1 << 0,
    Cold        = 1 << 1,
    Warm        = 1 << 2,
    Hot         = 1 << 3,
    Burning     = 1 << 4,

    // Moisture states
    Dry         = 1 << 5,
    Damp        = 1 << 6,
    Wet         = 1 << 7,
    Soaked      = 1 << 8,

    // Life states
    Alive       = 1 << 9,
    Dead        = 1 << 10,
    Decomposing = 1 << 11,

    // Fire states
    Smoldering  = 1 << 12,
    Charred     = 1 << 13,
    AshCovered  = 1 << 14,
};

constexpr MaterialState operator|(MaterialState a, MaterialState b)
{
    return static_cast<MaterialState>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline MaterialState operator&(MaterialState a, MaterialState b)
{
    return static_cast<MaterialState>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline bool HasState(MaterialState flags, MaterialState flag)
{
    return (static_cast<u32>(flags) & static_cast<u32>(flag)) != 0;
}
