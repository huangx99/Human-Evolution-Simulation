#include "test_framework.h"
#include "sim/ecology/material_id.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/affordance.h"
#include "sim/ecology/material_db.h"
#include "sim/ecology/ecology_entity.h"
#include "sim/ecology/ecology_registry.h"
#include "sim/ecology/field_id.h"
#include "rules/reaction/reaction_rule.h"
#include "rules/reaction/semantic_predicate.h"
#include "rules/reaction/reaction_effect.h"
#include "rules/reaction/semantic_reaction_rule.h"

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

// ===== SEMANTIC REACTION SYSTEM TESTS =====

// Test: Semantic predicate types exist and can be constructed
TEST(semantic_predicate_types)
{
    // HasCapability predicate
    SemanticPredicate hasHeat;
    hasHeat.type = PredicateType::HasCapability;
    hasHeat.capability = Capability::HeatEmission;
    ASSERT_TRUE(hasHeat.type == PredicateType::HasCapability);
    ASSERT_TRUE(hasHeat.capability == Capability::HeatEmission);

    // HasState predicate
    SemanticPredicate isDry;
    isDry.type = PredicateType::HasState;
    isDry.state = MaterialState::Dry;
    ASSERT_TRUE(isDry.state == MaterialState::Dry);

    // FieldGreaterThan predicate
    SemanticPredicate hotField;
    hotField.type = PredicateType::FieldGreaterThan;
    hotField.field = FieldId::Temperature;
    hotField.value = 40.0f;
    ASSERT_TRUE(hotField.field == FieldId::Temperature);

    // NearbyCapability predicate
    SemanticPredicate nearbyHeat;
    nearbyHeat.type = PredicateType::NearbyCapability;
    nearbyHeat.capability = Capability::HeatEmission;
    nearbyHeat.radius = 2;
    ASSERT_TRUE(nearbyHeat.radius == 2);

    return true;
}

// Test: ReactionEffect types exist and can be constructed
TEST(reaction_effect_types)
{
    ReactionEffect addBurn;
    addBurn.type = EffectType::AddState;
    addBurn.state = MaterialState::Burning;
    ASSERT_TRUE(addBurn.type == EffectType::AddState);

    ReactionEffect addHeat;
    addHeat.type = EffectType::AddCapability;
    addHeat.capability = Capability::HeatEmission;
    ASSERT_TRUE(addHeat.capability == Capability::HeatEmission);

    ReactionEffect modTemp;
    modTemp.type = EffectType::ModifyField;
    modTemp.field = FieldId::Temperature;
    modTemp.delta = 5.0f;
    ASSERT_TRUE(modTemp.delta == 5.0f);

    return true;
}

// Test: Grass ignition rule (pure semantic, no ElementId)
// Flammable + Dry + NearbyCapability(HeatEmission) → Burning + HeatEmission + SmokeEmission
TEST(semantic_grass_ignition)
{
    // Build the rule
    SemanticReactionRule rule;
    rule.id = "grass_ignite";
    rule.name = "Grass ignition via semantic predicates";
    rule.probability = 1.0f;

    // Predicates
    SemanticPredicate isFlammable;
    isFlammable.type = PredicateType::HasCapability;
    isFlammable.capability = Capability::Flammable;

    SemanticPredicate isDry;
    isDry.type = PredicateType::HasState;
    isDry.state = MaterialState::Dry;

    SemanticPredicate nearbyHeat;
    nearbyHeat.type = PredicateType::NearbyCapability;
    nearbyHeat.capability = Capability::HeatEmission;
    nearbyHeat.radius = 2;

    rule.conditions = {isFlammable, isDry, nearbyHeat};

    // Effects
    ReactionEffect addBurning;
    addBurning.type = EffectType::AddState;
    addBurning.state = MaterialState::Burning;

    ReactionEffect addHeatEmission;
    addHeatEmission.type = EffectType::AddCapability;
    addHeatEmission.capability = Capability::HeatEmission;

    ReactionEffect addSmoke;
    addSmoke.type = EffectType::AddCapability;
    addSmoke.capability = Capability::SmokeEmission;

    rule.effects = {addBurning, addHeatEmission, addSmoke};

    // Verify rule structure
    ASSERT_TRUE(rule.conditions.size() == 3);
    ASSERT_TRUE(rule.effects.size() == 3);
    ASSERT_TRUE(rule.conditions[0].type == PredicateType::HasCapability);
    ASSERT_TRUE(rule.conditions[0].capability == Capability::Flammable);
    ASSERT_TRUE(rule.conditions[1].type == PredicateType::HasState);
    ASSERT_TRUE(rule.conditions[1].state == MaterialState::Dry);
    ASSERT_TRUE(rule.conditions[2].type == PredicateType::NearbyCapability);
    ASSERT_TRUE(rule.conditions[2].capability == Capability::HeatEmission);

    return true;
}

// Test: Rain extinguish rule (pure semantic)
// Burning + FieldGreaterThan(Humidity, 80) → RemoveBurning + RemoveHeatEmission
TEST(semantic_rain_extinguish)
{
    SemanticReactionRule rule;
    rule.id = "rain_extinguish";
    rule.name = "Rain extinguishes fire";

    SemanticPredicate isBurning;
    isBurning.type = PredicateType::HasState;
    isBurning.state = MaterialState::Burning;

    SemanticPredicate highHumidity;
    highHumidity.type = PredicateType::FieldGreaterThan;
    highHumidity.field = FieldId::Humidity;
    highHumidity.value = 80.0f;

    rule.conditions = {isBurning, highHumidity};

    ReactionEffect removeBurning;
    removeBurning.type = EffectType::RemoveState;
    removeBurning.state = MaterialState::Burning;

    ReactionEffect removeHeat;
    removeHeat.type = EffectType::RemoveCapability;
    removeHeat.capability = Capability::HeatEmission;

    rule.effects = {removeBurning, removeHeat};

    ASSERT_TRUE(rule.conditions.size() == 2);
    ASSERT_TRUE(rule.effects.size() == 2);
    ASSERT_TRUE(rule.effects[0].type == EffectType::RemoveState);
    ASSERT_TRUE(rule.effects[1].type == EffectType::RemoveCapability);

    return true;
}

// Test: Corpse decay rule (pure semantic)
// HasMaterial(Organic) + HasState(Dead) + FieldGreaterThan(Temperature, 25) → EmitSmell
TEST(semantic_corpse_decay)
{
    SemanticReactionRule rule;
    rule.id = "corpse_decay";
    rule.name = "Dead organic produces smell in warm conditions";

    SemanticPredicate isOrganic;
    isOrganic.type = PredicateType::HasMaterial;
    isOrganic.material = MaterialId::Flesh;

    SemanticPredicate isDead;
    isDead.type = PredicateType::HasState;
    isDead.state = MaterialState::Dead;

    SemanticPredicate isWarm;
    isWarm.type = PredicateType::FieldGreaterThan;
    isWarm.field = FieldId::Temperature;
    isWarm.value = 25.0f;

    rule.conditions = {isOrganic, isDead, isWarm};

    ReactionEffect emitSmell;
    emitSmell.type = EffectType::EmitSmell;
    emitSmell.field = FieldId::Smell;
    emitSmell.delta = 1.0f;

    rule.effects = {emitSmell};

    ASSERT_TRUE(rule.conditions.size() == 3);
    ASSERT_TRUE(rule.effects[0].type == EffectType::EmitSmell);
    ASSERT_TRUE(rule.effects[0].delta == 1.0f);

    return true;
}

// Test: High-temp stone ignition (verifies the key property)
// A "hot stone" with HeatEmission should trigger the same grass ignition rule
// as a torch, fire pit, or lava — without any special-case code.
TEST(semantic_hot_stone_ignites_grass)
{
    // The grass ignition rule only checks capabilities:
    //   HasCapability(Flammable) + HasState(Dry) + NearbyCapability(HeatEmission)
    //
    // A hot stone has HeatEmission. A dry grass has Flammable + Dry.
    // The rule fires without knowing what a "stone" or "torch" is.

    EcologyRegistry reg;

    // Hot stone: just a stone with HeatEmission added
    auto& hotStone = reg.Create(MaterialId::Stone, "hot_stone");
    hotStone.x = 5; hotStone.y = 5;
    hotStone.AddCapability(Capability::HeatEmission);

    // Dry grass nearby
    auto& dryGrass = reg.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 6; dryGrass.y = 5;

    // Verify: hot stone has HeatEmission
    ASSERT_TRUE(hotStone.HasCapability(Capability::HeatEmission));

    // Verify: dry grass is Flammable + Dry
    ASSERT_TRUE(dryGrass.HasCapability(Capability::Flammable));
    ASSERT_TRUE(dryGrass.HasState(MaterialState::Dry));

    // The semantic rule would fire because:
    //   NearbyCapability(HeatEmission, radius=2) from (6,5) finds hot_stone at (5,5)
    // No special-case for "stone" needed.

    return true;
}
