#include "test_framework.h"
#include "sim/ecology/material_id.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/affordance.h"
#include "sim/ecology/material_db.h"

// Test: MaterialDB has entries for all defined materials
TEST(ecology_material_db_init)
{
    // Grass should be flammable and edible
    auto grassCaps = MaterialDB::GetCapabilities(MaterialId::Grass);
    ASSERT_TRUE(HasCapability(grassCaps, Capability::Flammable));
    ASSERT_TRUE(HasCapability(grassCaps, Capability::Edible));
    ASSERT_TRUE(HasCapability(grassCaps, Capability::Grows));

    // Water should be passable and allow swimming
    auto waterAffs = MaterialDB::GetAffordances(MaterialId::Water);
    ASSERT_TRUE(HasAffordance(waterAffs, Affordance::CanSwim));
    ASSERT_TRUE(HasAffordance(waterAffs, Affordance::CanExtinguish));

    // Stone should block wind and be walkable
    auto stoneCaps = MaterialDB::GetCapabilities(MaterialId::Stone);
    ASSERT_TRUE(HasCapability(stoneCaps, Capability::BlocksWind));
    ASSERT_TRUE(HasCapability(stoneCaps, Capability::Walkable));

    return true;
}

// Test: MaterialState bitfield operations
TEST(ecology_state_bitfield)
{
    MaterialState state = MaterialState::Warm | MaterialState::Dry;
    ASSERT_TRUE(HasState(state, MaterialState::Warm));
    ASSERT_TRUE(HasState(state, MaterialState::Dry));
    ASSERT_TRUE(!HasState(state, MaterialState::Wet));
    ASSERT_TRUE(!HasState(state, MaterialState::Burning));

    // Can combine with burning
    state = state | MaterialState::Burning;
    ASSERT_TRUE(HasState(state, MaterialState::Burning));
    ASSERT_TRUE(HasState(state, MaterialState::Warm));  // still warm

    return true;
}

// Test: Capability bitfield operations
TEST(ecology_capability_bitfield)
{
    Capability caps = Capability::Flammable | Capability::Edible;
    ASSERT_TRUE(HasCapability(caps, Capability::Flammable));
    ASSERT_TRUE(HasCapability(caps, Capability::Edible));
    ASSERT_TRUE(!HasCapability(caps, Capability::Walkable));

    return true;
}

// Test: Affordance bitfield operations
TEST(ecology_affordance_bitfield)
{
    Affordance affs = Affordance::CanIgnite | Affordance::CanEat;
    ASSERT_TRUE(HasAffordance(affs, Affordance::CanIgnite));
    ASSERT_TRUE(HasAffordance(affs, Affordance::CanEat));
    ASSERT_TRUE(!HasAffordance(affs, Affordance::CanBuild));

    return true;
}

// Test: DryGrass is flammable but Grass is also flammable
TEST(ecology_fire_spread_semantics)
{
    auto grassCaps = MaterialDB::GetCapabilities(MaterialId::Grass);
    auto dryGrassCaps = MaterialDB::GetCapabilities(MaterialId::DryGrass);

    // Both flammable
    ASSERT_TRUE(HasCapability(grassCaps, Capability::Flammable));
    ASSERT_TRUE(HasCapability(dryGrassCaps, Capability::Flammable));

    // DryGrass has CanBurn affordance (easier to ignite)
    auto dryGrassAffs = MaterialDB::GetAffordances(MaterialId::DryGrass);
    ASSERT_TRUE(HasAffordance(dryGrassAffs, Affordance::CanBurn));

    // Water can extinguish
    auto waterAffs = MaterialDB::GetAffordances(MaterialId::Water);
    ASSERT_TRUE(HasAffordance(waterAffs, Affordance::CanExtinguish));

    return true;
}
