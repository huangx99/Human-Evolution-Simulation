#pragma once

#include "sim/ecology/material_id.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/affordance.h"
#include <array>

// Static database: maps MaterialId → default capabilities and affordances
struct MaterialProperties
{
    Capability capabilities = Capability::None;
    Affordance affordances = Affordance::None;
    MaterialState defaultState = MaterialState::None;
};

namespace MaterialDB
{
    constexpr std::array<MaterialProperties, static_cast<size_t>(MaterialId::Count)> Init()
    {
        std::array<MaterialProperties, static_cast<size_t>(MaterialId::Count)> db{};

        // Dirt
        db[static_cast<size_t>(MaterialId::Dirt)] = {
            Capability::Walkable | Capability::Passable | Capability::Absorbent | Capability::Buildable,
            Affordance::CanWalk | Affordance::CanDig | Affordance::CanBuild | Affordance::CanGrow,
            MaterialState::Warm
        };

        // Sand
        db[static_cast<size_t>(MaterialId::Sand)] = {
            Capability::Walkable | Capability::Passable | Capability::Insulates,
            Affordance::CanWalk | Affordance::CanDig,
            MaterialState::Warm | MaterialState::Dry
        };

        // Stone
        db[static_cast<size_t>(MaterialId::Stone)] = {
            Capability::Walkable | Capability::Conducts | Capability::BlocksWind | Capability::Stackable,
            Affordance::CanWalk | Affordance::CanBuild | Affordance::CanMine,
            MaterialState::Warm
        };

        // Clay
        db[static_cast<size_t>(MaterialId::Clay)] = {
            Capability::Walkable | Capability::Absorbent | Capability::Buildable,
            Affordance::CanWalk | Affordance::CanDig | Affordance::CanBuild,
            MaterialState::Damp
        };

        // Grass
        db[static_cast<size_t>(MaterialId::Grass)] = {
            Capability::Walkable | Capability::Passable | Capability::Flammable | Capability::Edible | Capability::Grows | Capability::Decays,
            Affordance::CanWalk | Affordance::CanEat | Affordance::CanGather | Affordance::CanIgnite | Affordance::CanGrow,
            MaterialState::Alive | MaterialState::Warm
        };

        // DryGrass
        db[static_cast<size_t>(MaterialId::DryGrass)] = {
            Capability::Walkable | Capability::Passable | Capability::Flammable | Capability::Edible | Capability::Decays,
            Affordance::CanWalk | Affordance::CanEat | Affordance::CanGather | Affordance::CanIgnite | Affordance::CanBurn,
            MaterialState::Dead | MaterialState::Dry
        };

        // Bush
        db[static_cast<size_t>(MaterialId::Bush)] = {
            Capability::Passable | Capability::Flammable | Capability::Edible | Capability::Grows | Capability::Decays,
            Affordance::CanEat | Affordance::CanGather | Affordance::CanIgnite | Affordance::CanHide | Affordance::CanGrow,
            MaterialState::Alive | MaterialState::Warm
        };

        // Wood (dead)
        db[static_cast<size_t>(MaterialId::Wood)] = {
            Capability::Flammable | Capability::Conducts | Capability::Buildable | Capability::Stackable | Capability::Decays,
            Affordance::CanBuild | Affordance::CanIgnite | Affordance::CanBurn | Affordance::CanGather,
            MaterialState::Dead | MaterialState::Dry
        };

        // Tree (living)
        db[static_cast<size_t>(MaterialId::Tree)] = {
            Capability::Flammable | Capability::Grows | Capability::BlocksWind | Capability::Decays,
            Affordance::CanHide | Affordance::CanIgnite | Affordance::CanSignal | Affordance::CanGrow,
            MaterialState::Alive | MaterialState::Warm
        };

        // Water
        db[static_cast<size_t>(MaterialId::Water)] = {
            Capability::Passable | Capability::Conducts | Capability::Absorbent,
            Affordance::CanSwim | Affordance::CanExtinguish,
            MaterialState::Wet
        };

        // Ice
        db[static_cast<size_t>(MaterialId::Ice)] = {
            Capability::Walkable | Capability::Conducts | Capability::RepelsWater,
            Affordance::CanWalk | Affordance::CanMine,
            MaterialState::Frozen
        };

        // Mud
        db[static_cast<size_t>(MaterialId::Mud)] = {
            Capability::Walkable | Capability::Absorbent,
            Affordance::CanWalk | Affordance::CanDig,
            MaterialState::Wet | MaterialState::Damp
        };

        // Flesh
        db[static_cast<size_t>(MaterialId::Flesh)] = {
            Capability::Edible | Capability::Digestible | Capability::Decays | Capability::Conducts,
            Affordance::CanEat | Affordance::CanGather,
            MaterialState::Warm
        };

        // Bone
        db[static_cast<size_t>(MaterialId::Bone)] = {
            Capability::Buildable | Capability::Stackable | Capability::Insulates,
            Affordance::CanGather | Affordance::CanBuild,
            MaterialState::Dead
        };

        // Leaf
        db[static_cast<size_t>(MaterialId::Leaf)] = {
            Capability::Flammable | Capability::Edible | Capability::Decays | Capability::Insulates,
            Affordance::CanEat | Affordance::CanGather | Affordance::CanIgnite,
            MaterialState::Alive | MaterialState::Warm
        };

        // Charcoal
        db[static_cast<size_t>(MaterialId::Charcoal)] = {
            Capability::Flammable | Capability::Conducts | Capability::Buildable,
            Affordance::CanIgnite | Affordance::CanBuild | Affordance::CanGather,
            MaterialState::Charred | MaterialState::Dry
        };

        // Ash
        db[static_cast<size_t>(MaterialId::Ash)] = {
            Capability::Insulates | Capability::Decays | Capability::Absorbent,
            Affordance::CanGather,
            MaterialState::AshCovered | MaterialState::Dead
        };

        // Coal — dense fuel, flammable when dry
        db[static_cast<size_t>(MaterialId::Coal)] = {
            Capability::Flammable | Capability::Conducts | Capability::Buildable | Capability::HeatEmission,
            Affordance::CanIgnite | Affordance::CanBuild | Affordance::CanGather,
            MaterialState::Dry
        };

        // RottingPlant — decomposing organic matter
        db[static_cast<size_t>(MaterialId::RottingPlant)] = {
            Capability::Edible | Capability::Decays | Capability::Flammable,
            Affordance::CanEat | Affordance::CanGather,
            MaterialState::Dead | MaterialState::Decomposing | MaterialState::Damp
        };

        return db;
    }

    constexpr auto properties = Init();

    inline const MaterialProperties& Get(MaterialId id)
    {
        return properties[static_cast<size_t>(id)];
    }

    inline Capability GetCapabilities(MaterialId id)
    {
        return Get(id).capabilities;
    }

    inline Affordance GetAffordances(MaterialId id)
    {
        return Get(id).affordances;
    }

    inline MaterialState GetDefaultState(MaterialId id)
    {
        return Get(id).defaultState;
    }
}
