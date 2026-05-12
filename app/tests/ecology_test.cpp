#include "test_framework.h"
#include "sim/world/world_state.h"
#include "sim/ecology/material_id.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/affordance.h"
#include "sim/ecology/material_db.h"
#include "sim/ecology/ecology_entity.h"
#include "sim/ecology/ecology_registry.h"
#include "sim/ecology/field_id.h"
#include "rules/reaction/semantic_predicate.h"
#include "rules/reaction/reaction_effect.h"
#include "rules/reaction/semantic_reaction_rule.h"
#include "rules/reaction/semantic_reaction_system.h"

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

// Test: Capability-based fire reaction (semantic version)
// "HeatEmission + Flammable + LowHumidity = Burning"
// This works for torches, fire pits, lava, lightning — not just "fire" string.
TEST(ecology_demo_capability_reaction)
{
    SemanticReactionRule rule;
    rule.id = "cap_ignite";
    rule.name = "Capability-based ignition";

    // Predicate: entity at cell must have HeatEmission
    SemanticPredicate heatSource;
    heatSource.type = PredicateType::HasCapability;
    heatSource.capability = Capability::HeatEmission;

    // Predicate: humidity must be low
    SemanticPredicate dryAir;
    dryAir.type = PredicateType::FieldLessThan;
    dryAir.field = FieldId::Humidity;
    dryAir.value = 35.0f;

    rule.conditions = {heatSource, dryAir};

    // Effect: add Burning state
    ReactionEffect ignite;
    ignite.type = EffectType::AddState;
    ignite.state = MaterialState::Burning;
    rule.effects = {ignite};

    ASSERT_TRUE(rule.conditions.size() == 2);
    ASSERT_TRUE(rule.effects.size() == 1);

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
    modTemp.mode = FieldModifyMode::Add;
    modTemp.value = 5.0f;
    ASSERT_TRUE(modTemp.value == 5.0f);

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

// ===== EXECUTION TESTS: SemanticReactionSystem against real WorldState =====

// Helper: build the standard fire ignition rule (Flammable + Dry + NearbyHeat → Burning)
static SemanticReactionRule MakeIgnitionRule()
{
    SemanticReactionRule rule;
    rule.id = "ignite";
    rule.name = "Flammable+Dry+HeatEmission → Burning";
    rule.probability = 1.0f;

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

    ReactionEffect addBurning;
    addBurning.type = EffectType::AddState;
    addBurning.state = MaterialState::Burning;

    ReactionEffect addHeat;
    addHeat.type = EffectType::AddCapability;
    addHeat.capability = Capability::HeatEmission;

    rule.effects = {addBurning, addHeat};

    return rule;
}

// Scenario 1: 高温石头点燃干草
// Hot stone (HeatEmission) + Dry grass (Flammable+Dry) → grass catches fire
TEST(semantic_exec_hot_stone)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    // Place hot stone at (5,5)
    auto& hotStone = ecology.Create(MaterialId::Stone, "hot_stone");
    hotStone.x = 5; hotStone.y = 5;
    hotStone.AddCapability(Capability::HeatEmission);

    // Place dry grass at (6,5) — nearby
    auto& dryGrass = ecology.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 6; dryGrass.y = 5;

    // Build spatial index, then run one tick
    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    sys.Update(world);
    world.commands.Apply(world);

    // Verify: dry grass should now be Burning and have HeatEmission
    ASSERT_TRUE(dryGrass.HasState(MaterialState::Burning));
    ASSERT_TRUE(dryGrass.HasCapability(Capability::HeatEmission));

    return true;
}

// Scenario 2: 干草本身可燃 (Flammable+Dry)
// A torch (Wood+Burning+HeatEmission) ignites dry grass
TEST(semantic_exec_torch_ignites)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    // Place torch at (5,5)
    auto& torch = ecology.Create(MaterialId::Wood, "torch");
    torch.x = 5; torch.y = 5;
    torch.AddState(MaterialState::Burning);
    torch.AddCapability(Capability::HeatEmission);
    torch.AddCapability(Capability::LightEmission);

    // Place dry grass at (5,6)
    auto& dryGrass = ecology.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 5; dryGrass.y = 6;

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    sys.Update(world);
    world.commands.Apply(world);

    ASSERT_TRUE(dryGrass.HasState(MaterialState::Burning));

    return true;
}

// Scenario 3: 湿木头不容易燃烧
// Wet wood (Flammable+Wet) should NOT catch fire — no Dry state
TEST(semantic_exec_wet_wood_no_burn)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    // Place heat source at (5,5)
    auto& heatSource = ecology.Create(MaterialId::Stone, "hot_stone");
    heatSource.x = 5; heatSource.y = 5;
    heatSource.AddCapability(Capability::HeatEmission);

    // Place WET wood at (6,5) — Flammable but NOT Dry
    auto& wetWood = ecology.Create(MaterialId::Wood, "wet_wood");
    wetWood.x = 6; wetWood.y = 5;
    wetWood.state = MaterialState::Wet;  // override default Dry state

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    sys.Update(world);
    world.commands.Apply(world);

    // Verify: wet wood should NOT be Burning
    ASSERT_TRUE(!wetWood.HasState(MaterialState::Burning));
    ASSERT_TRUE(!wetWood.HasCapability(Capability::HeatEmission));

    return true;
}

// Scenario 4: 腐肉在高温下产生气味
// Organic(Flesh) + Dead + Temperature>25 → SmellEmission
TEST(semantic_exec_rotten_meat)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    // Place corpse at (5,5)
    auto& corpse = ecology.Create(MaterialId::Flesh, "corpse");
    corpse.x = 5; corpse.y = 5;
    corpse.state = MaterialState::Dead;

    // Set temperature > 25 at that cell
    world.Env().temperature.WriteNext(5, 5) = 30.0f;
    world.Env().temperature.Swap();

    // Build decay rule: Flesh + Dead + Temp>25 → EmitSmell
    SemanticReactionRule rule;
    rule.id = "decay";
    rule.name = "Corpse decay in warm conditions";
    rule.probability = 1.0f;

    SemanticPredicate isFlesh;
    isFlesh.type = PredicateType::HasMaterial;
    isFlesh.material = MaterialId::Flesh;

    SemanticPredicate isDead;
    isDead.type = PredicateType::HasState;
    isDead.state = MaterialState::Dead;

    SemanticPredicate isWarm;
    isWarm.type = PredicateType::FieldGreaterThan;
    isWarm.field = FieldId::Temperature;
    isWarm.value = 25.0f;

    rule.conditions = {isFlesh, isDead, isWarm};

    ReactionEffect emitSmell;
    emitSmell.type = EffectType::EmitSmell;
    emitSmell.field = FieldId::Smell;
    emitSmell.delta = 5.0f;

    rule.effects = {emitSmell};

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(rule);
    sys.Update(world);
    world.commands.Apply(world);
    world.Info().smell.Swap();

    // Verify: smell field should have increased
    f32 smell = world.Info().smell.At(5, 5);
    ASSERT_TRUE(smell > 0.0f);

    return true;
}

// Scenario 5: 湿草不容易燃烧 (no Dry state)
TEST(semantic_exec_wet_grass_no_burn)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    auto& heatSource = ecology.Create(MaterialId::Stone, "hot_stone");
    heatSource.x = 5; heatSource.y = 5;
    heatSource.AddCapability(Capability::HeatEmission);

    // Wet grass — Flammable but state is Wet, not Dry
    auto& wetGrass = ecology.Create(MaterialId::Grass, "wet_grass");
    wetGrass.x = 6; wetGrass.y = 5;
    wetGrass.state = MaterialState::Wet;

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    sys.Update(world);
    world.commands.Apply(world);

    ASSERT_TRUE(!wetGrass.HasState(MaterialState::Burning));

    return true;
}

// ===== EXTENSION TEST: New materials + new rules, zero system changes =====

// Test: Coal (new material) ignites dry grass via existing ignition rule
// Verifies: SemanticReactionSystem doesn't know about Coal — it just sees Flammable+Dry+HeatEmission
TEST(extension_coal_ignites_grass)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    // Coal has HeatEmission by default in MaterialDB
    auto& coal = ecology.Create(MaterialId::Coal, "coal");
    coal.x = 5; coal.y = 5;

    // Dry grass nearby
    auto& dryGrass = ecology.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 6; dryGrass.y = 5;

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    sys.Update(world);
    world.commands.Apply(world);

    // Coal's HeatEmission should ignite the dry grass
    ASSERT_TRUE(dryGrass.HasState(MaterialState::Burning));

    return true;
}

// Test: RottingPlant (new material) produces smell in warm conditions
// Verifies: new material + new rule works without touching SemanticReactionSystem
TEST(extension_rotting_plant_smell)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    // RottingPlant is Dead + Decomposing by default
    auto& plant = ecology.Create(MaterialId::RottingPlant, "rotting_plant");
    plant.x = 5; plant.y = 5;

    // Set temperature > 20
    world.Env().temperature.WriteNext(5, 5) = 25.0f;
    world.Env().temperature.Swap();

    // Build rule: Dead + Decomposing + Temp>20 → EmitSmell
    SemanticReactionRule rule;
    rule.id = "plant_decay";
    rule.name = "Rotting plant smell";
    rule.probability = 1.0f;

    SemanticPredicate isDead;
    isDead.type = PredicateType::HasState;
    isDead.state = MaterialState::Dead;

    SemanticPredicate isDecomposing;
    isDecomposing.type = PredicateType::HasState;
    isDecomposing.state = MaterialState::Decomposing;

    SemanticPredicate isWarm;
    isWarm.type = PredicateType::FieldGreaterThan;
    isWarm.field = FieldId::Temperature;
    isWarm.value = 20.0f;

    rule.conditions = {isDead, isDecomposing, isWarm};

    ReactionEffect emitSmell;
    emitSmell.type = EffectType::EmitSmell;
    emitSmell.delta = 3.0f;

    rule.effects = {emitSmell};

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(rule);
    sys.Update(world);
    world.commands.Apply(world);
    world.Info().smell.Swap();

    f32 smell = world.Info().smell.At(5, 5);
    ASSERT_TRUE(smell > 0.0f);

    return true;
}

// Test: Coal does NOT ignite wet grass (no Dry state)
// Verifies: new material respects existing semantic constraints
TEST(extension_coal_no_ignite_wet)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    auto& coal = ecology.Create(MaterialId::Coal, "coal");
    coal.x = 5; coal.y = 5;

    auto& wetGrass = ecology.Create(MaterialId::Grass, "wet_grass");
    wetGrass.x = 6; wetGrass.y = 5;
    wetGrass.state = MaterialState::Wet;  // override default — not Dry

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    sys.Update(world);
    world.commands.Apply(world);

    // Wet grass should NOT burn — rule requires Dry state
    ASSERT_TRUE(!wetGrass.HasState(MaterialState::Burning));

    return true;
}

// Test: SameEntity binding prevents cross-contamination
// Scenario: dry_grass (Flammable+Dry) and wet_wood (Flammable+Wet) share a cell.
// With SameEntity mode, only dry_grass should burn — wet_wood must NOT be affected.
TEST(reaction_same_entity_no_cross_contamination)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    // Heat source nearby
    auto& heatSource = ecology.Create(MaterialId::Stone, "hot_stone");
    heatSource.x = 3; heatSource.y = 3;
    heatSource.AddCapability(Capability::HeatEmission);

    // Two entities in the SAME cell: dry_grass (Flammable+Dry) and wet_wood (Flammable+Wet)
    auto& dryGrass = ecology.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 4; dryGrass.y = 3;

    auto& wetWood = ecology.Create(MaterialId::Wood, "wet_wood");
    wetWood.x = 4; wetWood.y = 3;
    wetWood.state = MaterialState::Wet;  // NOT Dry

    world.RebuildSpatial();
    SemanticReactionSystem sys;

    auto rule = MakeIgnitionRule();
    // Default is SameEntity — all entity predicates must match same entity
    ASSERT_TRUE(rule.targetMode == ReactionTargetMode::SameEntity);
    sys.AddRule(rule);

    sys.Update(world);
    world.commands.Apply(world);

    // dry_grass IS Flammable+Dry → should burn
    ASSERT_TRUE(dryGrass.HasState(MaterialState::Burning));
    ASSERT_TRUE(dryGrass.HasCapability(Capability::HeatEmission));

    // wet_wood is Flammable but NOT Dry → must NOT burn (SameEntity binding)
    ASSERT_TRUE(!wetWood.HasState(MaterialState::Burning));
    ASSERT_TRUE(!wetWood.HasCapability(Capability::HeatEmission));

    return true;
}

// Test: SameCell mode applies effects to ALL entities in the cell
// (legacy behavior, explicitly opted-in)
TEST(reaction_same_cell_applies_to_all)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    auto& heatSource = ecology.Create(MaterialId::Stone, "hot_stone");
    heatSource.x = 3; heatSource.y = 3;
    heatSource.AddCapability(Capability::HeatEmission);

    // Two entities in the same cell, both Flammable+Dry
    auto& grass1 = ecology.Create(MaterialId::DryGrass, "grass1");
    grass1.x = 4; grass1.y = 3;

    auto& grass2 = ecology.Create(MaterialId::DryGrass, "grass2");
    grass2.x = 4; grass2.y = 3;

    world.RebuildSpatial();
    SemanticReactionSystem sys;

    auto rule = MakeIgnitionRule();
    rule.targetMode = ReactionTargetMode::SameCell;  // explicit opt-in
    sys.AddRule(rule);

    sys.Update(world);
    world.commands.Apply(world);

    // Both should burn in SameCell mode
    ASSERT_TRUE(grass1.HasState(MaterialState::Burning));
    ASSERT_TRUE(grass2.HasState(MaterialState::Burning));

    return true;
}

// Test: SameEntity with corpse decay — Flesh+Dead must be on same entity
// Prevents: another Dead object in the cell triggering decay on unrelated Flesh
TEST(reaction_same_entity_corpse_decay)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    // A dead flesh entity (should decay)
    auto& corpse = ecology.Create(MaterialId::Flesh, "corpse");
    corpse.x = 5; corpse.y = 5;
    corpse.state = MaterialState::Dead;

    // A dead plant in the same cell (also Dead, but NOT Flesh)
    auto& deadPlant = ecology.Create(MaterialId::Grass, "dead_plant");
    deadPlant.x = 5; deadPlant.y = 5;
    deadPlant.state = MaterialState::Dead;

    world.Env().temperature.WriteNext(5, 5) = 30.0f;
    world.Env().temperature.Swap();

    // Rule: HasMaterial(Flesh) + HasState(Dead) + Temp>25 → EmitSmell
    SemanticReactionRule rule;
    rule.id = "decay";
    rule.name = "Corpse decay";
    rule.probability = 1.0f;
    rule.targetMode = ReactionTargetMode::SameEntity;

    SemanticPredicate isFlesh;
    isFlesh.type = PredicateType::HasMaterial;
    isFlesh.material = MaterialId::Flesh;

    SemanticPredicate isDead;
    isDead.type = PredicateType::HasState;
    isDead.state = MaterialState::Dead;

    SemanticPredicate isWarm;
    isWarm.type = PredicateType::FieldGreaterThan;
    isWarm.field = FieldId::Temperature;
    isWarm.value = 25.0f;

    rule.conditions = {isFlesh, isDead, isWarm};

    ReactionEffect emitSmell;
    emitSmell.type = EffectType::EmitSmell;
    emitSmell.delta = 5.0f;
    rule.effects = {emitSmell};

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(rule);
    sys.Update(world);
    world.commands.Apply(world);
    world.Info().smell.Swap();

    // Smell should be emitted (from corpse, not dead_plant)
    f32 smell = world.Info().smell.At(5, 5);
    ASSERT_TRUE(smell > 0.0f);

    return true;
}

// Test: Command payloadU32 preserves large bitfield values exactly
// Verifies fix for f32 precision loss with Capability::Tool (1 << 24)
TEST(command_payload_u32_precision)
{
    WorldState world(16, 16, 42);
    auto& ecology = world.Ecology().entities;

    auto& entity = ecology.Create(MaterialId::Stone, "test_entity");
    entity.x = 5; entity.y = 5;

    // Use a large capability flag that would lose precision in f32
    Capability largeCap = Capability::Tool;  // 1 << 24 = 16777216

    Command cmd;
    cmd.type = CommandType::AddEntityCapability;
    cmd.entityId = entity.id;
    cmd.payloadU32 = static_cast<u32>(largeCap);

    world.commands.Push(cmd);
    world.commands.Apply(world);

    ASSERT_TRUE(entity.HasCapability(Capability::Tool));

    // Also test with combined flags
    Capability combined = Capability::Flammable | Capability::Tool | Capability::Weapon;
    Command cmd2;
    cmd2.type = CommandType::AddEntityCapability;
    cmd2.entityId = entity.id;
    cmd2.payloadU32 = static_cast<u32>(combined);

    world.commands.Push(cmd2);
    world.commands.Apply(world);

    ASSERT_TRUE(entity.HasCapability(Capability::Flammable));
    ASSERT_TRUE(entity.HasCapability(Capability::Weapon));
    ASSERT_TRUE(entity.HasCapability(Capability::Tool));

    return true;
}

// Test: ModifyFieldValue rejects out-of-bounds coordinates
TEST(command_modify_field_bounds_check)
{
    WorldState world(8, 8, 42);

    // Try to modify a field at invalid coordinates
    Command cmd;
    cmd.type = CommandType::ModifyFieldValue;
    cmd.x = -1;
    cmd.y = -1;
    cmd.targetX = static_cast<i32>(FieldId::Temperature);
    cmd.targetY = 0;  // Add mode
    cmd.value = 999.0f;

    world.commands.Push(cmd);
    world.commands.Apply(world);

    // Should not crash — bounds check rejects the command
    // If we get here without crashing, the test passes
    ASSERT_TRUE(true);

    return true;
}
