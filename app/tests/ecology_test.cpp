#include "test_framework.h"
#include "sim/ecology/material_id.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/affordance.h"
#include "sim/ecology/material_db.h"
#include "sim/ecology/ecology_entity.h"
#include "sim/ecology/ecology_registry.h"
#include "rules/reaction/reaction_rule.h"

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

// ===== DEMO: Semantic composition without concrete item names =====

// Test: Torch = Wood + Burning + LightEmission + HeatEmission + SmokeEmission
// No TorchSystem needed. Systems check capabilities, not item names.
TEST(ecology_demo_torch)
{
    EcologyRegistry reg;

    // Create a torch: base material is Wood, add extra capabilities
    auto& torch = reg.Create(MaterialId::Wood, "torch");
    torch.AddState(MaterialState::Burning);
    torch.AddCapability(Capability::LightEmission);
    torch.AddCapability(Capability::HeatEmission);
    torch.AddCapability(Capability::SmokeEmission);
    torch.AddAffordance(Affordance::CanLight);
    torch.AddAffordance(Affordance::CanScare);
    torch.AddAffordance(Affordance::CanIgniteTarget);

    // Verify: torch has wood's base capabilities + extras
    ASSERT_TRUE(torch.HasCapability(Capability::Flammable));      // from Wood
    ASSERT_TRUE(torch.HasCapability(Capability::LightEmission));   // extra
    ASSERT_TRUE(torch.HasCapability(Capability::HeatEmission));    // extra
    ASSERT_TRUE(torch.HasCapability(Capability::SmokeEmission));   // extra
    ASSERT_TRUE(torch.HasAffordance(Affordance::CanLight));        // extra
    ASSERT_TRUE(torch.HasAffordance(Affordance::CanIgniteTarget)); // extra
    ASSERT_TRUE(torch.HasState(MaterialState::Burning));

    // A system that needs light should check HasCapability(LightEmission),
    // NOT check if item == "Torch"

    return true;
}

// Test: SharpStone = Stone + SharpEdge + Weapon + Portable
// No KnifeSystem needed.
TEST(ecology_demo_sharp_stone)
{
    EcologyRegistry reg;

    auto& sharpStone = reg.Create(MaterialId::Stone, "sharp_stone");
    sharpStone.AddCapability(Capability::SharpEdge);
    sharpStone.AddCapability(Capability::Portable);
    sharpStone.AddCapability(Capability::Weapon);
    sharpStone.AddAffordance(Affordance::CanCut);

    ASSERT_TRUE(sharpStone.HasCapability(Capability::SharpEdge));
    ASSERT_TRUE(sharpStone.HasCapability(Capability::Portable));
    ASSERT_TRUE(sharpStone.HasCapability(Capability::Weapon));
    ASSERT_TRUE(sharpStone.HasAffordance(Affordance::CanCut));
    ASSERT_TRUE(sharpStone.HasCapability(Capability::BlocksWind));  // from Stone

    return true;
}

// Test: WoodenStick = Wood + LongReach + Portable
// No SpearSystem needed. Can be combined with SharpStone + Binding → Spear.
TEST(ecology_demo_wooden_stick)
{
    EcologyRegistry reg;

    auto& stick = reg.Create(MaterialId::Wood, "wooden_stick");
    stick.AddCapability(Capability::LongReach);
    stick.AddCapability(Capability::Portable);
    stick.AddAffordance(Affordance::CanPoke);

    ASSERT_TRUE(stick.HasCapability(Capability::LongReach));
    ASSERT_TRUE(stick.HasCapability(Capability::Portable));
    ASSERT_TRUE(stick.HasAffordance(Affordance::CanPoke));
    ASSERT_TRUE(stick.HasCapability(Capability::Flammable));  // from Wood

    return true;
}

// Test: Capability-based fire reaction
// "HeatEmission + Flammable + LowHumidity = Burning"
// This works for torches, fire pits, lava, lightning — not just "fire" string.
TEST(ecology_demo_capability_reaction)
{
    // Define a capability-based reaction rule
    ReactionRule rule;
    rule.id = "cap_ignite";
    rule.name = "Capability-based ignition";

    // No ElementId inputs needed — we check capabilities instead
    rule.inputs = {};

    // Capability condition: entity at cell must have HeatEmission
    CapabilityCondition heatSource;
    heatSource.requiredCapabilities = Capability::HeatEmission;
    rule.capabilityConditions.push_back(heatSource);

    // Field condition: humidity must be low
    FieldCondition dryAir;
    dryAir.field = FieldId::Humidity;
    dryAir.op = ConditionOp::Less;
    dryAir.value = 35.0f;
    rule.fieldConditions.push_back(dryAir);

    // Output: increase fire
    ReactionOutput ignite;
    ignite.type = OutputType::IncreaseFire;
    ignite.value = 30.0f;
    rule.outputs.push_back(ignite);

    // Verify the rule is correctly structured
    ASSERT_TRUE(rule.capabilityConditions.size() == 1);
    ASSERT_TRUE(rule.fieldConditions.size() == 1);
    ASSERT_TRUE(rule.outputs.size() == 1);

    // The rule triggers when:
    //   - an entity with HeatEmission exists at the cell (torch, fire pit, etc.)
    //   - AND humidity < 35
    // This is generic: no mention of "fire", "torch", or any specific item.

    return true;
}

// Test: EcologyRegistry queries
TEST(ecology_registry_query)
{
    EcologyRegistry reg;

    auto& torch = reg.Create(MaterialId::Wood, "torch");
    torch.x = 5; torch.y = 5;
    torch.AddCapability(Capability::LightEmission);
    torch.AddCapability(Capability::HeatEmission);

    auto& stone = reg.Create(MaterialId::Stone, "sharp_stone");
    stone.x = 5; stone.y = 5;
    stone.AddCapability(Capability::SharpEdge);

    auto& stick = reg.Create(MaterialId::Wood, "stick");
    stick.x = 10; stick.y = 10;
    stick.AddCapability(Capability::LongReach);

    // Query by position
    auto atCell = reg.At(5, 5);
    ASSERT_TRUE(atCell.size() == 2);

    // Query by capability at position
    auto lightSources = reg.AtWithCapability(5, 5, Capability::LightEmission);
    ASSERT_TRUE(lightSources.size() == 1);
    ASSERT_TRUE(lightSources[0]->name == "torch");

    // Query all with capability
    auto allPortable = reg.WithCapability(Capability::Portable);
    // (none have Portable in this test)

    auto allLongReach = reg.WithCapability(Capability::LongReach);
    ASSERT_TRUE(allLongReach.size() == 1);
    ASSERT_TRUE(allLongReach[0]->name == "stick");

    return true;
}
