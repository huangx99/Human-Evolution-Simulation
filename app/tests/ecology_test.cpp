#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "test_framework.h"
#include "sim/system/system_context.h"
#include "sim/ecology/material_id.h"
#include "sim/ecology/material_state.h"
#include "sim/ecology/capability.h"
#include "sim/ecology/affordance.h"
#include "sim/ecology/material_db.h"
#include "sim/ecology/ecology_entity.h"
#include "sim/ecology/ecology_registry.h"
#include "sim/field/field_id.h"
#include "rules/reaction/semantic_predicate.h"
#include "rules/reaction/reaction_effect.h"
#include "rules/reaction/semantic_reaction_rule.h"
#include "rules/reaction/semantic_reaction_system.h"

static HumanEvolutionRulePack g_rulePack;
static const auto& g_envCtx = g_rulePack.GetHumanEvolutionContext().environment;

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
TEST(ecology_demo_torch)
{
    EcologyRegistry reg;

    auto& torch = reg.Create(MaterialId::Wood, "torch");
    torch.AddState(MaterialState::Burning);
    torch.AddCapability(Capability::LightEmission);
    torch.AddCapability(Capability::HeatEmission);
    torch.AddCapability(Capability::SmokeEmission);
    torch.AddAffordance(Affordance::CanLight);
    torch.AddAffordance(Affordance::CanScare);
    torch.AddAffordance(Affordance::CanIgniteTarget);

    ASSERT_TRUE(torch.HasCapability(Capability::Flammable));
    ASSERT_TRUE(torch.HasCapability(Capability::LightEmission));
    ASSERT_TRUE(torch.HasCapability(Capability::HeatEmission));
    ASSERT_TRUE(torch.HasCapability(Capability::SmokeEmission));
    ASSERT_TRUE(torch.HasAffordance(Affordance::CanLight));
    ASSERT_TRUE(torch.HasAffordance(Affordance::CanIgniteTarget));
    ASSERT_TRUE(torch.HasState(MaterialState::Burning));

    return true;
}

// Test: SharpStone = Stone + SharpEdge + Weapon + Portable
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
    ASSERT_TRUE(sharpStone.HasCapability(Capability::BlocksWind));

    return true;
}

// Test: WoodenStick = Wood + LongReach + Portable
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
    ASSERT_TRUE(stick.HasCapability(Capability::Flammable));

    return true;
}

// Test: Capability-based fire reaction (semantic version)
TEST(ecology_demo_capability_reaction)
{
    SemanticReactionRule rule;
    rule.id = "cap_ignite";
    rule.name = "Capability-based ignition";

    SemanticPredicate heatSource;
    heatSource.type = PredicateType::HasCapability;
    heatSource.capability = Capability::HeatEmission;

    SemanticPredicate dryAir;
    dryAir.type = PredicateType::FieldLessThan;
    dryAir.field = FieldKey("human_evolution.humidity");
    dryAir.value = 35.0f;

    rule.conditions = {heatSource, dryAir};

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

    auto allLongReach = reg.WithCapability(Capability::LongReach);
    ASSERT_TRUE(allLongReach.size() == 1);
    ASSERT_TRUE(allLongReach[0]->name == "stick");

    return true;
}

// ===== SEMANTIC REACTION SYSTEM TESTS =====

TEST(semantic_predicate_types)
{
    SemanticPredicate hasHeat;
    hasHeat.type = PredicateType::HasCapability;
    hasHeat.capability = Capability::HeatEmission;
    ASSERT_TRUE(hasHeat.type == PredicateType::HasCapability);
    ASSERT_TRUE(hasHeat.capability == Capability::HeatEmission);

    SemanticPredicate isDry;
    isDry.type = PredicateType::HasState;
    isDry.state = MaterialState::Dry;
    ASSERT_TRUE(isDry.state == MaterialState::Dry);

    SemanticPredicate hotField;
    hotField.type = PredicateType::FieldGreaterThan;
    hotField.field = FieldKey("human_evolution.temperature");
    hotField.value = 40.0f;
    ASSERT_TRUE(hotField.field == FieldKey("human_evolution.temperature"));

    SemanticPredicate nearbyHeat;
    nearbyHeat.type = PredicateType::NearbyCapability;
    nearbyHeat.capability = Capability::HeatEmission;
    nearbyHeat.radius = 2;
    ASSERT_TRUE(nearbyHeat.radius == 2);

    return true;
}

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
    modTemp.field = FieldKey("human_evolution.temperature");
    modTemp.mode = FieldModifyMode::Add;
    modTemp.value = 5.0f;
    ASSERT_TRUE(modTemp.value == 5.0f);

    return true;
}

TEST(semantic_grass_ignition)
{
    SemanticReactionRule rule;
    rule.id = "grass_ignite";
    rule.name = "Grass ignition via semantic predicates";
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

    ReactionEffect addHeatEmission;
    addHeatEmission.type = EffectType::AddCapability;
    addHeatEmission.capability = Capability::HeatEmission;

    ReactionEffect addSmoke;
    addSmoke.type = EffectType::AddCapability;
    addSmoke.capability = Capability::SmokeEmission;

    rule.effects = {addBurning, addHeatEmission, addSmoke};

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
    highHumidity.field = FieldKey("human_evolution.humidity");
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
    isWarm.field = FieldKey("human_evolution.temperature");
    isWarm.value = 25.0f;

    rule.conditions = {isOrganic, isDead, isWarm};

    ReactionEffect emitSmell;
    emitSmell.type = EffectType::EmitSmell;
    emitSmell.field = FieldKey("human_evolution.smell");
    emitSmell.delta = 1.0f;

    rule.effects = {emitSmell};

    ASSERT_TRUE(rule.conditions.size() == 3);
    ASSERT_TRUE(rule.effects[0].type == EffectType::EmitSmell);
    ASSERT_TRUE(rule.effects[0].delta == 1.0f);

    return true;
}

TEST(semantic_hot_stone_ignites_grass)
{
    EcologyRegistry reg;

    auto& hotStone = reg.Create(MaterialId::Stone, "hot_stone");
    hotStone.x = 5; hotStone.y = 5;
    hotStone.AddCapability(Capability::HeatEmission);

    auto& dryGrass = reg.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 6; dryGrass.y = 5;

    ASSERT_TRUE(hotStone.HasCapability(Capability::HeatEmission));
    ASSERT_TRUE(dryGrass.HasCapability(Capability::Flammable));
    ASSERT_TRUE(dryGrass.HasState(MaterialState::Dry));

    return true;
}

// ===== EXECUTION TESTS: SemanticReactionSystem against real WorldState =====

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

TEST(semantic_exec_hot_stone)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& hotStone = ecology.Create(MaterialId::Stone, "hot_stone");
    hotStone.x = 5; hotStone.y = 5;
    hotStone.AddCapability(Capability::HeatEmission);

    auto& dryGrass = ecology.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 6; dryGrass.y = 5;

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);

    ASSERT_TRUE(dryGrass.HasState(MaterialState::Burning));
    ASSERT_TRUE(dryGrass.HasCapability(Capability::HeatEmission));

    return true;
}

TEST(semantic_exec_torch_ignites)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& torch = ecology.Create(MaterialId::Wood, "torch");
    torch.x = 5; torch.y = 5;
    torch.AddState(MaterialState::Burning);
    torch.AddCapability(Capability::HeatEmission);
    torch.AddCapability(Capability::LightEmission);

    auto& dryGrass = ecology.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 5; dryGrass.y = 6;

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);

    ASSERT_TRUE(dryGrass.HasState(MaterialState::Burning));

    return true;
}

TEST(semantic_exec_wet_wood_no_burn)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& heatSource = ecology.Create(MaterialId::Stone, "hot_stone");
    heatSource.x = 5; heatSource.y = 5;
    heatSource.AddCapability(Capability::HeatEmission);

    auto& wetWood = ecology.Create(MaterialId::Wood, "wet_wood");
    wetWood.x = 6; wetWood.y = 5;
    wetWood.state = MaterialState::Wet;

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);

    ASSERT_TRUE(!wetWood.HasState(MaterialState::Burning));
    ASSERT_TRUE(!wetWood.HasCapability(Capability::HeatEmission));

    return true;
}

TEST(semantic_exec_rotten_meat)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& corpse = ecology.Create(MaterialId::Flesh, "corpse");
    corpse.x = 5; corpse.y = 5;
    corpse.state = MaterialState::Dead;

    // Set temperature > 25
    world.Fields().WriteNext(g_envCtx.temperature, 5, 5, 30.0f);
    world.Fields().SwapAll();

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
    isWarm.field = FieldKey("human_evolution.temperature");
    isWarm.value = 25.0f;

    rule.conditions = {isFlesh, isDead, isWarm};

    ReactionEffect emitSmell;
    emitSmell.type = EffectType::EmitSmell;
    emitSmell.field = FieldKey("human_evolution.smell");
    emitSmell.delta = 5.0f;

    rule.effects = {emitSmell};

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(rule);
    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);
    world.Fields().SwapAll();

    f32 smell = world.Fields().Read(g_envCtx.smell, 5, 5);
    ASSERT_TRUE(smell > 0.0f);

    return true;
}

TEST(semantic_exec_wet_grass_no_burn)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& heatSource = ecology.Create(MaterialId::Stone, "hot_stone");
    heatSource.x = 5; heatSource.y = 5;
    heatSource.AddCapability(Capability::HeatEmission);

    auto& wetGrass = ecology.Create(MaterialId::Grass, "wet_grass");
    wetGrass.x = 6; wetGrass.y = 5;
    wetGrass.state = MaterialState::Wet;

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);

    ASSERT_TRUE(!wetGrass.HasState(MaterialState::Burning));

    return true;
}

// ===== EXTENSION TESTS =====

TEST(extension_coal_ignites_grass)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& coal = ecology.Create(MaterialId::Coal, "coal");
    coal.x = 5; coal.y = 5;

    auto& dryGrass = ecology.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 6; dryGrass.y = 5;

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);

    ASSERT_TRUE(dryGrass.HasState(MaterialState::Burning));

    return true;
}

TEST(extension_rotting_plant_smell)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& plant = ecology.Create(MaterialId::RottingPlant, "rotting_plant");
    plant.x = 5; plant.y = 5;

    // Set temperature > 20
    world.Fields().WriteNext(g_envCtx.temperature, 5, 5, 25.0f);
    world.Fields().SwapAll();

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
    isWarm.field = FieldKey("human_evolution.temperature");
    isWarm.value = 20.0f;

    rule.conditions = {isDead, isDecomposing, isWarm};

    ReactionEffect emitSmell;
    emitSmell.type = EffectType::EmitSmell;
    emitSmell.delta = 3.0f;

    rule.effects = {emitSmell};

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(rule);
    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);
    world.Fields().SwapAll();

    f32 smell = world.Fields().Read(g_envCtx.smell, 5, 5);
    ASSERT_TRUE(smell > 0.0f);

    return true;
}

TEST(extension_coal_no_ignite_wet)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& coal = ecology.Create(MaterialId::Coal, "coal");
    coal.x = 5; coal.y = 5;

    auto& wetGrass = ecology.Create(MaterialId::Grass, "wet_grass");
    wetGrass.x = 6; wetGrass.y = 5;
    wetGrass.state = MaterialState::Wet;

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(MakeIgnitionRule());
    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);

    ASSERT_TRUE(!wetGrass.HasState(MaterialState::Burning));

    return true;
}

TEST(reaction_same_entity_no_cross_contamination)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& heatSource = ecology.Create(MaterialId::Stone, "hot_stone");
    heatSource.x = 3; heatSource.y = 3;
    heatSource.AddCapability(Capability::HeatEmission);

    auto& dryGrass = ecology.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 4; dryGrass.y = 3;

    auto& wetWood = ecology.Create(MaterialId::Wood, "wet_wood");
    wetWood.x = 4; wetWood.y = 3;
    wetWood.state = MaterialState::Wet;

    world.RebuildSpatial();
    SemanticReactionSystem sys;

    auto rule = MakeIgnitionRule();
    ASSERT_TRUE(rule.targetMode == ReactionTargetMode::SameEntity);
    sys.AddRule(rule);

    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);

    ASSERT_TRUE(dryGrass.HasState(MaterialState::Burning));
    ASSERT_TRUE(dryGrass.HasCapability(Capability::HeatEmission));
    ASSERT_TRUE(!wetWood.HasState(MaterialState::Burning));
    ASSERT_TRUE(!wetWood.HasCapability(Capability::HeatEmission));

    return true;
}

TEST(reaction_same_cell_applies_to_all)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& heatSource = ecology.Create(MaterialId::Stone, "hot_stone");
    heatSource.x = 3; heatSource.y = 3;
    heatSource.AddCapability(Capability::HeatEmission);

    auto& grass1 = ecology.Create(MaterialId::DryGrass, "grass1");
    grass1.x = 4; grass1.y = 3;

    auto& grass2 = ecology.Create(MaterialId::DryGrass, "grass2");
    grass2.x = 4; grass2.y = 3;

    world.RebuildSpatial();
    SemanticReactionSystem sys;

    auto rule = MakeIgnitionRule();
    rule.targetMode = ReactionTargetMode::CellWide;
    sys.AddRule(rule);

    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);

    ASSERT_TRUE(grass1.HasState(MaterialState::Burning));
    ASSERT_TRUE(grass2.HasState(MaterialState::Burning));

    return true;
}

TEST(reaction_same_entity_corpse_decay)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& corpse = ecology.Create(MaterialId::Flesh, "corpse");
    corpse.x = 5; corpse.y = 5;
    corpse.state = MaterialState::Dead;

    auto& deadPlant = ecology.Create(MaterialId::Grass, "dead_plant");
    deadPlant.x = 5; deadPlant.y = 5;
    deadPlant.state = MaterialState::Dead;

    // Set temperature > 25
    world.Fields().WriteNext(g_envCtx.temperature, 5, 5, 30.0f);
    world.Fields().SwapAll();

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
    isWarm.field = FieldKey("human_evolution.temperature");
    isWarm.value = 25.0f;

    rule.conditions = {isFlesh, isDead, isWarm};

    ReactionEffect emitSmell;
    emitSmell.type = EffectType::EmitSmell;
    emitSmell.delta = 5.0f;
    rule.effects = {emitSmell};

    world.RebuildSpatial();
    SemanticReactionSystem sys;
    sys.AddRule(rule);
    { SystemContext ctx(world); sys.Update(ctx); }
    world.commands.Apply(world);
    world.Fields().SwapAll();

    f32 smell = world.Fields().Read(g_envCtx.smell, 5, 5);
    ASSERT_TRUE(smell > 0.0f);

    return true;
}

TEST(command_payload_u32_precision)
{
    WorldState world(16, 16, 42);
    world.Init(g_rulePack);
    auto& ecology = world.Ecology().entities;

    auto& entity = ecology.Create(MaterialId::Stone, "test_entity");
    entity.x = 5; entity.y = 5;

    Capability largeCap = Capability::Tool;

    world.commands.Push(0, AddEntityCapabilityCommand{entity.id, static_cast<u32>(largeCap)});
    world.commands.Apply(world);

    ASSERT_TRUE(entity.HasCapability(Capability::Tool));

    Capability combined = Capability::Flammable | Capability::Tool | Capability::Weapon;
    world.commands.Push(0, AddEntityCapabilityCommand{entity.id, static_cast<u32>(combined)});
    world.commands.Apply(world);

    ASSERT_TRUE(entity.HasCapability(Capability::Flammable));
    ASSERT_TRUE(entity.HasCapability(Capability::Weapon));
    ASSERT_TRUE(entity.HasCapability(Capability::Tool));

    return true;
}

TEST(command_modify_field_bounds_check)
{
    WorldState world(8, 8, 42);
    world.Init(g_rulePack);

    world.commands.Push(0,
        ModifyFieldValueCommand{-1, -1, FieldKey("human_evolution.temperature"), 0, 999.0f});
    world.commands.Apply(world);

    ASSERT_TRUE(true);

    return true;
}
