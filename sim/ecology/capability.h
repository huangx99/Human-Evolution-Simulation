#pragma once

#include "core/types/types.h"
#include <cstdint>

// Bitfield: inherent physical/chemical properties of a material
enum class Capability : u32
{
    None        = 0,

    // Thermal
    Flammable   = 1 << 0,   // can catch fire
    Conducts    = 1 << 1,   // conducts heat
    Insulates   = 1 << 2,   // resists heat transfer

    // Moisture
    Absorbent   = 1 << 3,   // absorbs water
    RepelsWater = 1 << 4,   // repels water

    // Structural
    Walkable    = 1 << 5,   // agents can walk on it
    Passable    = 1 << 6,   // wind/smell/smoke passes through
    BlocksWind  = 1 << 7,   // blocks wind

    // Biological
    Edible      = 1 << 8,   // agents can eat it
    Digestible  = 1 << 9,   // provides nutrition
    Toxic       = 1 << 10,  // harmful if eaten

    // Growth
    Grows       = 1 << 11,  // can spread/grow
    Decays      = 1 << 12,  // decomposes over time

    // Construction (future)
    Buildable   = 1 << 13,  // can be used in construction
    Stackable   = 1 << 14,  // can be stacked/piled

    // Emission (things an entity radiates/releases)
    LightEmission = 1 << 15, // emits visible light
    HeatEmission  = 1 << 16, // radiates heat
    SmokeEmission = 1 << 17, // produces smoke

    // Tool properties
    Portable    = 1 << 18,  // can be carried by agent
    SharpEdge   = 1 << 19,  // has cutting surface
    LongReach   = 1 << 20,  // extends reach distance
    Binding     = 1 << 21,  // can tie/combine items
    Container   = 1 << 22,  // can hold items/liquid

    // Composite tool capabilities
    Weapon      = 1 << 23,  // can be used as weapon
    Tool        = 1 << 24,  // can be used as tool
};

constexpr Capability operator|(Capability a, Capability b)
{
    return static_cast<Capability>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline Capability operator&(Capability a, Capability b)
{
    return static_cast<Capability>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline bool HasCapability(Capability flags, Capability cap)
{
    return (static_cast<u32>(flags) & static_cast<u32>(cap)) != 0;
}
