#pragma once

#include "core/types/types.h"
#include <cstdint>

// Identifies a type of material in the ecology
enum class MaterialId : u16
{
    None = 0,

    // Terrain
    Dirt,
    Sand,
    Stone,
    Clay,

    // Vegetation
    Grass,
    DryGrass,
    Bush,
    Wood,       // dead wood / logs
    Tree,       // living tree

    // Water
    Water,
    Ice,
    Mud,

    // Organic
    Flesh,
    Bone,
    Leaf,

    // Constructed (future)
    Charcoal,
    Ash,

    Count
};
