#pragma once

#include "core/types/types.h"
#include <cstdint>

// Bitfield: actions that agents/agents can perform on a material
enum class Affordance : u32
{
    None        = 0,

    // Fire-related
    CanIgnite   = 1 << 0,   // can start fire here
    CanBurn     = 1 << 1,   // fire can spread here
    CanExtinguish = 1 << 2, // can put out fire

    // Food-related
    CanEat      = 1 << 3,   // agent can consume
    CanGather   = 1 << 4,   // agent can collect
    CanStore    = 1 << 5,   // agent can store items here

    // Movement
    CanWalk     = 1 << 6,   // agent can move through
    CanClimb    = 1 << 7,   // agent can climb
    CanSwim     = 1 << 8,   // agent can swim through

    // Construction (future)
    CanBuild    = 1 << 9,   // can place structures here
    CanDig      = 1 << 10,  // can excavate
    CanMine     = 1 << 11,  // can extract resources

    // Social (future)
    CanRest     = 1 << 12,  // agent can rest here
    CanHide     = 1 << 13,  // agent can take cover
    CanSignal   = 1 << 14,  // agent can signal from here

    // Growth
    CanGrow     = 1 << 15,  // vegetation can spread here

    // Tool-based actions
    CanCut      = 1 << 16,  // cutting/harvesting
    CanPoke     = 1 << 17,  // poking/prodding
    CanCarry    = 1 << 18,  // transporting items
    CanLight    = 1 << 19,  // providing light
    CanScare    = 1 << 20,  // frightening animals
    CanIgniteTarget = 1 << 21, // setting fire to target
    CanBind     = 1 << 22,  // combining/tying
};

constexpr Affordance operator|(Affordance a, Affordance b)
{
    return static_cast<Affordance>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline Affordance operator&(Affordance a, Affordance b)
{
    return static_cast<Affordance>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline bool HasAffordance(Affordance flags, Affordance aff)
{
    return (static_cast<u32>(flags) & static_cast<u32>(aff)) != 0;
}
