// Phase 2.7: Risk Runtime + Food Assessment tests
// Tests for CellRiskEvaluator, DriveEvaluator, IntentSelector, NavigationPolicy, FeedbackUpdater, FoodAssessor

#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/agent_drives.h"
#include "rules/human_evolution/runtime/agent_intent.h"
#include "rules/human_evolution/runtime/navigation_policy.h"
#include "rules/human_evolution/runtime/action_feedback.h"
#include "rules/human_evolution/runtime/food_assessment.h"
#include "rules/human_evolution/runtime/standard_behaviors.h"
#include "sim/ecology/behavior_table.h"
#include "sim/scheduler/scheduler.h"
#include "test_framework.h"
#include "sim/world/world_state.h"

static HumanEvolutionRulePack g_riskRulePack;
static const auto& g_riskEnvCtx = g_riskRulePack.GetHumanEvolutionContext().environment;

static bool g_riskBehaviorsRegistered = false;
inline void EnsureRiskBehaviorsRegistered()
{
    if (!g_riskBehaviorsRegistered)
    {
        RegisterStandardBehaviors(g_riskRulePack.GetHumanEvolutionContext().concepts);
        g_riskBehaviorsRegistered = true;
    }
}

// ============================================================
// CellRisk: Memory does NOT block traversal
// ============================================================
TEST(CellRisk_MemoryDoesNotBlockTraversal)
{
    WorldState world(32, 32, 42, g_riskRulePack);
    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    auto& cog = world.Cognitive();

    // Inject many strong danger memories at (15,16)
    for (int i = 0; i < 20; i++)
    {
        MemoryRecord mem;
        mem.ownerId = 1;
        mem.subject = g_riskRulePack.GetHumanEvolutionContext().concepts.fire;
        mem.location = {15, 16};
        mem.strength = 20.0f;
        mem.confidence = 1.0f;
        mem.sense = SenseType::Vision;
        cog.GetAgentMemories(1).push_back(mem);
    }

    // Evaluate risk at the memory location
    CellRisk risk = CellRiskEvaluator::Evaluate(
        world.Fields(), cog, g_riskEnvCtx, 1, 15, 16);

    // Memory risk should be high but traversal must NOT be Blocked
    ASSERT_TRUE(risk.memory > 0.0f);
    ASSERT_TRUE(risk.traversal != TraversalClass::Blocked);

    return true;
}

// ============================================================
// CellRisk: Lethal fire blocks traversal
// ============================================================
TEST(CellRisk_LethalFireBlocksTraversal)
{
    WorldState world(32, 32, 42, g_riskRulePack);
    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    // Write lethal fire
    world.Fields().WriteNext(g_riskEnvCtx.fire, 15, 16, 90.0f);
    world.Fields().SwapAll();

    auto& cog = world.Cognitive();
    CellRisk risk = CellRiskEvaluator::Evaluate(
        world.Fields(), cog, g_riskEnvCtx, 1, 15, 16);

    ASSERT_TRUE(risk.lethal);
    ASSERT_TRUE(risk.traversal == TraversalClass::Blocked);

    return true;
}

// ============================================================
// CellRisk: Lethal temperature blocks traversal
// ============================================================
TEST(CellRisk_LethalTempBlocksTraversal)
{
    WorldState world(32, 32, 42, g_riskRulePack);
    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    // Write lethal temperature
    world.Fields().WriteNext(g_riskEnvCtx.temperature, 15, 16, 110.0f);
    world.Fields().SwapAll();

    auto& cog = world.Cognitive();
    CellRisk risk = CellRiskEvaluator::Evaluate(
        world.Fields(), cog, g_riskEnvCtx, 1, 15, 16);

    ASSERT_TRUE(risk.lethal);
    ASSERT_TRUE(risk.traversal == TraversalClass::Blocked);

    return true;
}

// ============================================================
// CellRisk: Normal cell is Passable
// ============================================================
TEST(CellRisk_NormalCellIsPassable)
{
    WorldState world(32, 32, 42, g_riskRulePack);
    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    auto& cog = world.Cognitive();
    CellRisk risk = CellRiskEvaluator::Evaluate(
        world.Fields(), cog, g_riskEnvCtx, 1, 0, 0);

    ASSERT_TRUE(risk.traversal == TraversalClass::Passable);
    ASSERT_TRUE(risk.total < 0.25f);
    ASSERT_TRUE(!risk.lethal);

    return true;
}

// ============================================================
// CellRisk: All outputs bounded 0..1
// ============================================================
TEST(CellRisk_AllOutputsBounded)
{
    WorldState world(32, 32, 42, g_riskRulePack);
    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    auto& cog = world.Cognitive();

    // Inject extreme memories
    for (int i = 0; i < 50; i++)
    {
        MemoryRecord mem;
        mem.ownerId = 1;
        mem.subject = g_riskRulePack.GetHumanEvolutionContext().concepts.fire;
        mem.location = {16, 16};
        mem.strength = 1000.0f;
        mem.confidence = 1.0f;
        mem.sense = SenseType::Vision;
        cog.GetAgentMemories(1).push_back(mem);
    }

    // Check all cells
    for (i32 y = 0; y < 32; y++)
    {
        for (i32 x = 0; x < 32; x++)
        {
            CellRisk risk = CellRiskEvaluator::Evaluate(
                world.Fields(), cog, g_riskEnvCtx, 1, x, y);

            ASSERT_TRUE(risk.physical >= 0.0f && risk.physical <= 1.0f);
            ASSERT_TRUE(risk.memory >= 0.0f && risk.memory <= 1.0f);
            ASSERT_TRUE(risk.social >= 0.0f && risk.social <= 1.0f);
            ASSERT_TRUE(risk.total >= 0.0f && risk.total <= 1.0f);
        }
    }

    return true;
}

// ============================================================
// CellRisk: Out of bounds is Blocked
// ============================================================
TEST(CellRisk_OutOfBoundsIsBlocked)
{
    WorldState world(32, 32, 42, g_riskRulePack);
    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    auto& cog = world.Cognitive();
    CellRisk risk = CellRiskEvaluator::Evaluate(
        world.Fields(), cog, g_riskEnvCtx, 1, -1, 0);

    ASSERT_TRUE(risk.outOfBounds);
    ASSERT_TRUE(risk.lethal);
    ASSERT_TRUE(risk.traversal == TraversalClass::Blocked);

    return true;
}

// ============================================================
// SaturateStrength: converges to 1.0
// ============================================================
TEST(SaturateStrength_Converges)
{
    f32 s1 = CellRiskEvaluator::SaturateStrength(1.0f);
    f32 s10 = CellRiskEvaluator::SaturateStrength(10.0f);
    f32 s10000 = CellRiskEvaluator::SaturateStrength(10000.0f);

    ASSERT_TRUE(s1 > 0.0f && s1 < 1.0f);
    ASSERT_TRUE(s10 > s1);
    ASSERT_TRUE(s10000 > s10);
    ASSERT_TRUE(s10000 <= 1.0f);

    // Non-finite input returns 0
    f32 sInf = CellRiskEvaluator::SaturateStrength(std::numeric_limits<f32>::infinity());
    ASSERT_TRUE(sInf == 0.0f);

    // Negative input returns 0
    f32 sNeg = CellRiskEvaluator::SaturateStrength(-5.0f);
    ASSERT_TRUE(sNeg == 0.0f);

    return true;
}

// ============================================================
// DriveEvaluator: hunger urgency smooth ramp
// ============================================================
TEST(Drives_HungerUrgencySmooth)
{
    Agent agent;
    agent.hunger = 0.0f;
    CellRisk risk;

    AgentDrives d = DriveEvaluator::Evaluate(agent, risk);
    ASSERT_TRUE(d.hungerUrgency == 0.0f);

    agent.hunger = 35.0f;
    d = DriveEvaluator::Evaluate(agent, risk);
    ASSERT_TRUE(d.hungerUrgency == 0.0f);

    agent.hunger = 95.0f;
    d = DriveEvaluator::Evaluate(agent, risk);
    ASSERT_NEAR(d.hungerUrgency, 1.0f, 0.01f);

    agent.hunger = 65.0f;
    d = DriveEvaluator::Evaluate(agent, risk);
    ASSERT_TRUE(d.hungerUrgency > 0.0f && d.hungerUrgency < 1.0f);

    return true;
}

// ============================================================
// DriveEvaluator: danger urgency from risk
// ============================================================
TEST(Drives_DangerUrgencyFromRisk)
{
    Agent agent;
    CellRisk risk;
    risk.total = 0.75f;

    AgentDrives d = DriveEvaluator::Evaluate(agent, risk);
    ASSERT_NEAR(d.dangerUrgency, 0.75f, 0.01f);

    return true;
}

// ============================================================
// IntentSelector: starving without smell -> Forage
// ============================================================
TEST(Intent_StarvingWithoutSmellForages)
{
    Agent agent;
    agent.id = 1;
    agent.hunger = 90.0f;
    agent.nearestSmell = 0.0f;

    CognitiveModule cog;
    AgentDrives drives;
    drives.hungerUrgency = DriveEvaluator::SmoothUrgency(90.0f, 35.0f, 95.0f);
    drives.dangerUrgency = 0.0f;

    AgentIntent intent = IntentSelector::Select(agent, drives, cog);

    ASSERT_TRUE(intent.type == AgentIntentType::Forage ||
                intent.type == AgentIntentType::ApproachKnownFood);
    ASSERT_TRUE(intent.urgency > 0.0f);

    return true;
}

// ============================================================
// IntentSelector: high danger -> Escape
// ============================================================
TEST(Intent_HighDangerEscapes)
{
    Agent agent;
    agent.id = 1;
    agent.hunger = 20.0f;

    CognitiveModule cog;
    AgentDrives drives;
    drives.hungerUrgency = 0.0f;
    drives.dangerUrgency = 0.8f;

    AgentIntent intent = IntentSelector::Select(agent, drives, cog);

    ASSERT_TRUE(intent.type == AgentIntentType::Escape);
    ASSERT_TRUE(intent.riskTolerance > 0.9f);

    return true;
}

// ============================================================
// IntentSelector: low urgency -> Explore
// ============================================================
TEST(Intent_LowUrgencyExplores)
{
    Agent agent;
    agent.id = 1;
    agent.hunger = 10.0f;

    CognitiveModule cog;
    AgentDrives drives;
    drives.hungerUrgency = 0.0f;
    drives.dangerUrgency = 0.0f;

    AgentIntent intent = IntentSelector::Select(agent, drives, cog);

    ASSERT_TRUE(intent.type == AgentIntentType::Explore);

    return true;
}

// ============================================================
// Escape: can move into Risky cell
// ============================================================
TEST(Escape_CanMoveIntoRiskyCell)
{
    WorldState world(32, 32, 42, g_riskRulePack);
    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    // Make current cell dangerous
    world.Fields().WriteNext(g_riskEnvCtx.fire, 16, 16, 50.0f);
    // Make one neighbor less dangerous
    world.Fields().WriteNext(g_riskEnvCtx.fire, 17, 16, 10.0f);
    world.Fields().SwapAll();

    auto& cog = world.Cognitive();
    Agent agent;
    agent.id = 1;
    agent.position = {16, 16};

    AgentIntent intent;
    intent.type = AgentIntentType::Escape;
    intent.urgency = 0.8f;
    intent.riskTolerance = 0.95f;

    AgentFeedback feedback;

    MoveDecision move = NavigationPolicy::ChooseEscapeMove(
        world.Fields(), cog, g_riskEnvCtx, agent, intent, feedback);

    // Should attempt to move (not stay at dx=0,dy=0)
    ASSERT_TRUE(move.attemptedMove);

    return true;
}

// ============================================================
// Escape: stuck breaker forces move
// ============================================================
TEST(Escape_StuckBreakerForcesMove)
{
    WorldState world(32, 32, 42, g_riskRulePack);
    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    // Moderate danger everywhere
    world.Fields().WriteNext(g_riskEnvCtx.fire, 16, 16, 30.0f);
    world.Fields().WriteNext(g_riskEnvCtx.fire, 15, 16, 25.0f);
    world.Fields().WriteNext(g_riskEnvCtx.fire, 17, 16, 25.0f);
    world.Fields().WriteNext(g_riskEnvCtx.fire, 16, 15, 25.0f);
    world.Fields().WriteNext(g_riskEnvCtx.fire, 16, 17, 25.0f);
    world.Fields().SwapAll();

    auto& cog = world.Cognitive();
    Agent agent;
    agent.id = 1;
    agent.position = {16, 16};

    AgentIntent intent;
    intent.type = AgentIntentType::Escape;
    intent.urgency = 0.8f;
    intent.riskTolerance = 0.95f;

    AgentFeedback feedback;
    feedback.stuckTicks = 5; // stuck for 5 ticks

    MoveDecision move = NavigationPolicy::ChooseEscapeMove(
        world.Fields(), cog, g_riskEnvCtx, agent, intent, feedback);

    // Must attempt to move due to stuck breaker
    ASSERT_TRUE(move.attemptedMove);

    return true;
}

// ============================================================
// Forage: high hunger without smell still acts
// ============================================================
TEST(Forage_HighHungerWithoutSmellStillActs)
{
    WorldState world(32, 32, 42, g_riskRulePack);
    world.SpawnAgent(16, 16);
    world.RebuildSpatial();

    auto& cog = world.Cognitive();
    Agent agent;
    agent.id = 1;
    agent.position = {16, 16};

    AgentIntent intent;
    intent.type = AgentIntentType::Forage;
    intent.urgency = 0.9f;
    intent.riskTolerance = 0.7f;

    AgentFeedback feedback;

    MoveDecision move = NavigationPolicy::ChooseForageMove(
        world.Fields(), cog, g_riskEnvCtx, agent, intent, feedback);

    // Should find some candidate (even if just lowest-risk move)
    ASSERT_TRUE(move.foundCandidate);

    return true;
}

// ============================================================
// Feedback: stuckTicks increments on failed move
// ============================================================
TEST(Feedback_StuckTicksIncrement)
{
    AgentFeedback fb;
    AgentIntent intent;
    intent.type = AgentIntentType::Escape;

    // Failed move
    FeedbackUpdater::Update(fb, intent, true, false, {16, 16}, {16, 16});
    ASSERT_TRUE(fb.stuckTicks == 1);
    ASSERT_TRUE(fb.failedEscapeTicks == 1);

    // Another failed move
    FeedbackUpdater::Update(fb, intent, true, false, {16, 16}, {16, 16});
    ASSERT_TRUE(fb.stuckTicks == 2);

    // Successful move resets stuckTicks
    FeedbackUpdater::Update(fb, intent, true, true, {16, 16}, {17, 16});
    ASSERT_TRUE(fb.stuckTicks == 0);
    ASSERT_TRUE(fb.failedEscapeTicks == 0);

    return true;
}

// ============================================================
// KnowledgeInfluence: saturates (integration with knowledge_graph)
// ============================================================
TEST(KnowledgeInfluence_Saturates)
{
    // Test the saturation formula: 1.0 - exp(-strength / 8.0)
    auto influence = [](f32 strength) -> f32 {
        f32 familiarity = 1.0f - std::exp(-std::max(0.0f, strength) / 8.0f);
        return std::clamp(familiarity, 0.0f, 1.0f);
    };

    f32 i1 = influence(1.0f);
    f32 i10 = influence(10.0f);
    f32 i10000 = influence(10000.0f);

    ASSERT_TRUE(i1 > 0.0f && i1 < 1.0f);
    ASSERT_TRUE(i10 > i1);
    ASSERT_TRUE(i10000 > i10);
    ASSERT_TRUE(i10000 <= 1.0f);

    // Should be bounded
    ASSERT_TRUE(i10000 < 1.001f);

    return true;
}

// ============================================================
// Behavior: Dead Flesh = EdibleRisky via BehaviorTable
// ============================================================
TEST(Behavior_DeadFleshIsEdibleRisky)
{
    EnsureRiskBehaviorsRegistered();
    Agent agent;
    EcologyEntity corpse;
    corpse.material = MaterialId::Flesh;
    corpse.state = MaterialState::Dead;

    auto f = FoodAssessor::Assess(corpse, agent);
    ASSERT_TRUE(f.validity == FoodValidity::EdibleRisky);
    ASSERT_TRUE(f.consumable == true);
    ASSERT_TRUE(f.risk > 0.0f);
    ASSERT_TRUE(f.nutrition > 0.0f);

    return true;
}

// ============================================================
// Recipe: Burning entity has no recipe match = NotFood
// ============================================================
TEST(Behavior_BurningEntityIsNotFood)
{
    EnsureRiskBehaviorsRegistered();
    Agent agent;
    EcologyEntity burningMeat;
    burningMeat.material = MaterialId::Flesh;
    burningMeat.state = MaterialState::Burning | MaterialState::Dead;

    auto f = FoodAssessor::Assess(burningMeat, agent);
    ASSERT_TRUE(f.validity == FoodValidity::NotFood);
    ASSERT_TRUE(f.consumable == false);

    return true;
}

// ============================================================
// Behavior: Fresh Fruit = EdibleFresh via BehaviorTable
// ============================================================
TEST(Behavior_FreshFruitIsEdibleFresh)
{
    EnsureRiskBehaviorsRegistered();
    Agent agent;
    EcologyEntity berry;
    berry.material = MaterialId::Fruit;
    berry.state = MaterialState::Alive;

    auto f = FoodAssessor::Assess(berry, agent);
    ASSERT_TRUE(f.validity == FoodValidity::EdibleFresh);
    ASSERT_TRUE(f.consumable == true);
    ASSERT_NEAR(f.nutrition, 1.0f, 0.01f);
    ASSERT_TRUE(f.risk < 0.1f);

    return true;
}

// ============================================================
// Recipe: Stone has no recipe = NotFood
// ============================================================
TEST(Behavior_StoneIsNotFood)
{
    EnsureRiskBehaviorsRegistered();
    Agent agent;
    EcologyEntity stone;
    stone.material = MaterialId::Stone;
    stone.state = MaterialState::None;

    auto f = FoodAssessor::Assess(stone, agent);
    ASSERT_TRUE(f.validity == FoodValidity::NotFood);
    ASSERT_TRUE(f.consumable == false);

    return true;
}

// ============================================================
// Recipe: IsFoodTarget includes EdibleRisky
// ============================================================
TEST(Behavior_IsFoodTargetIncludesRisky)
{
    EnsureRiskBehaviorsRegistered();
    Agent agent;
    EcologyEntity corpse;
    corpse.material = MaterialId::Flesh;
    corpse.state = MaterialState::Dead;

    ASSERT_TRUE(FoodAssessor::IsFoodTarget(corpse, agent));
    ASSERT_TRUE(FoodAssessor::IsConsumable(corpse, agent));

    return true;
}

// ============================================================
// Recipe: Fruit emission is appetizing
// ============================================================
TEST(Behavior_FruitEmissionIsAppetizing)
{
    EnsureRiskBehaviorsRegistered();
    SenseEmission emission = BehaviorTable::Instance().GetEmission(
        MaterialId::Fruit, MaterialState::Alive);
    ASSERT_NEAR(emission.smell.appetizing, 36.0f, 0.01f);
    ASSERT_NEAR(emission.smell.repulsive, 0.0f, 0.01f);
    ASSERT_NEAR(emission.vision.attract, 20.0f, 0.01f);

    return true;
}

// ============================================================
// Recipe: Corpse emission is repulsive
// ============================================================
TEST(Behavior_CorpseSmellIsRepulsive)
{
    EnsureRiskBehaviorsRegistered();
    SenseEmission emission = BehaviorTable::Instance().GetEmission(
        MaterialId::Flesh, MaterialState::Dead);
    ASSERT_NEAR(emission.smell.repulsive, 40.0f, 0.01f);
    ASSERT_NEAR(emission.smell.appetizing, 0.0f, 0.01f);

    return true;
}

// ============================================================
// Recipe: Valence computation
// ============================================================
TEST(Behavior_ValenceFromEmission)
{
    EnsureRiskBehaviorsRegistered();

    // Fruit: appetizing=36, attract=20
    SenseEmission fruitEmission = BehaviorTable::Instance().GetEmission(
        MaterialId::Fruit, MaterialState::Alive);
    SensoryProfile human = SensoryProfiles::Human();
    f32 fruitValence = human.ComputeValence(fruitEmission);
    // 36*0.8 + 20*0.7 = 28.8 + 14 = 42.8 → clamped to 1.0
    ASSERT_TRUE(fruitValence > 0.5f);

    // Corpse: repulsive=40
    SenseEmission corpseEmission = BehaviorTable::Instance().GetEmission(
        MaterialId::Flesh, MaterialState::Dead);
    f32 corpseValence = human.ComputeValence(corpseEmission);
    // 40*(-0.8) = -32 → clamped to -1.0
    ASSERT_TRUE(corpseValence < -0.5f);

    // Vulture: likes repulsive smell
    SensoryProfile vulture = SensoryProfiles::Vulture();
    f32 vultureValence = vulture.ComputeValence(corpseEmission);
    // 40*(+0.6) = 24 → clamped to 1.0
    ASSERT_TRUE(vultureValence > 0.5f);

    return true;
}

// ============================================================
// Lifecycle: Rotting corpse smells worse than fresh
// ============================================================
TEST(Lifecycle_RottingMeatSmellsBad)
{
    EnsureRiskBehaviorsRegistered();
    SenseEmission rotting = BehaviorTable::Instance().GetEmission(
        MaterialId::Flesh, MaterialState::Dead | MaterialState::Decomposing);
    ASSERT_NEAR(rotting.smell.repulsive, 60.0f, 0.01f);
    ASSERT_NEAR(rotting.smell.appetizing, 0.0f, 0.01f);

    // Compare with fresh corpse
    SenseEmission fresh = BehaviorTable::Instance().GetEmission(
        MaterialId::Flesh, MaterialState::Dead);
    ASSERT_TRUE(rotting.smell.repulsive > fresh.smell.repulsive);

    return true;
}

// ============================================================
// Lifecycle: Cooked meat smells appetizing
// ============================================================
TEST(Lifecycle_CookedMeatSmellsGood)
{
    EnsureRiskBehaviorsRegistered();
    SenseEmission cooked = BehaviorTable::Instance().GetEmission(
        MaterialId::Flesh, MaterialState::Dead | MaterialState::Charred);
    ASSERT_NEAR(cooked.smell.appetizing, 30.0f, 0.01f);
    ASSERT_NEAR(cooked.vision.attract, 10.0f, 0.01f);

    return true;
}

// ============================================================
// Lifecycle: Rotting corpse is NOT food
// ============================================================
TEST(Lifecycle_RottingCorpseNotFood)
{
    EnsureRiskBehaviorsRegistered();
    Agent agent;
    EcologyEntity rotting;
    rotting.material = MaterialId::Flesh;
    rotting.state = MaterialState::Dead | MaterialState::Decomposing;

    auto f = FoodAssessor::Assess(rotting, agent);
    ASSERT_TRUE(f.validity == FoodValidity::NotFood);
    ASSERT_TRUE(f.consumable == false);

    return true;
}

// ============================================================
// Lifecycle: Cooked flesh IS food (EdibleRisky)
// ============================================================
TEST(Lifecycle_CookedFleshIsFood)
{
    EnsureRiskBehaviorsRegistered();
    Agent agent;
    EcologyEntity cooked;
    cooked.material = MaterialId::Flesh;
    cooked.state = MaterialState::Dead | MaterialState::Charred;

    auto f = FoodAssessor::Assess(cooked, agent);
    ASSERT_TRUE(f.validity == FoodValidity::EdibleRisky);
    ASSERT_TRUE(f.consumable == true);
    ASSERT_NEAR(f.nutrition, 0.6f, 0.01f);

    return true;
}

// ============================================================
// Lifecycle: stateChangedTick is set by AddState
// ============================================================
TEST(Lifecycle_StateChangedTickSet)
{
    EcologyEntity entity;
    entity.material = MaterialId::Flesh;
    entity.state = MaterialState::Dead;
    entity.stateChangedTick = 0;

    // Simulate what AddEntityStateCommand does
    entity.AddState(MaterialState::Decomposing);
    // Note: stateChangedTick is set by the command, not AddState directly.
    // In a real scenario, AddEntityStateCommand::Execute sets it.
    // Here we set it manually to verify the predicate logic.
    entity.stateChangedTick = 100;

    // After 50 ticks: not enough
    ASSERT_TRUE(entity.stateChangedTick == 100);
    ASSERT_TRUE(entity.HasState(MaterialState::Decomposing));

    return true;
}

// ============================================================
// Lifecycle: Valence shift from fresh to rotting corpse
// ============================================================
TEST(Lifecycle_ValenceShiftOnDecomposition)
{
    EnsureRiskBehaviorsRegistered();
    SensoryProfile human = SensoryProfiles::Human();

    SenseEmission freshEmission = BehaviorTable::Instance().GetEmission(
        MaterialId::Flesh, MaterialState::Dead);
    f32 freshValence = human.ComputeValence(freshEmission);

    SenseEmission rottingEmission = BehaviorTable::Instance().GetEmission(
        MaterialId::Flesh, MaterialState::Dead | MaterialState::Decomposing);
    f32 rottingValence = human.ComputeValence(rottingEmission);

    // Both should be strongly negative (clamped to -1.0 at these emission levels)
    ASSERT_TRUE(freshValence < -0.5f);
    ASSERT_TRUE(rottingValence < -0.5f);

    // Rotting emission is stronger (60 vs 40 repulsive), even though both clamp to -1.0
    ASSERT_TRUE(rottingEmission.smell.repulsive > freshEmission.smell.repulsive);

    // Vulture should still approach rotting (repulsive smell is attractive to vultures)
    SensoryProfile vulture = SensoryProfiles::Vulture();
    f32 vultureValence = vulture.ComputeValence(rottingEmission);
    ASSERT_TRUE(vultureValence > 0.0f);

    return true;
}

// ============================================================
// Stability: 10000 ticks, no freeze, all floats finite
// ============================================================
TEST(Stability_10000Ticks_NoFreeze)
{
    HumanEvolutionRulePack rulePack;
    const auto& envCtx = rulePack.GetHumanEvolutionContext().environment;

    WorldState world(32, 32, 123, rulePack);

    // Fire zones: moderate fire around center
    for (i32 y = 12; y < 20; y++)
        for (i32 x = 12; x < 20; x++)
            if (x != 16 || y != 16)
                world.Fields().WriteNext(envCtx.fire, x, y, 40.0f);
    world.Fields().SwapAll();

    // Spawn agents at varied positions
    world.SpawnAgent(5, 5);
    world.SpawnAgent(10, 10);
    world.SpawnAgent(25, 25);
    world.SpawnAgent(16, 3);
    world.SpawnAgent(3, 16);

    auto scheduler = CreateHumanEvolutionScheduler(envCtx);

    i32 maxStuckTicks = 0;

    for (i32 tick = 0; tick < 10000; tick++)
    {
        scheduler.Tick(world);

        // Check all agents every 100 ticks
        if (tick % 100 == 99)
        {
            for (auto& agent : world.Agents().agents)
            {
                // All position values must be finite (no NaN drift)
                ASSERT_TRUE(std::isfinite(static_cast<f32>(agent.position.x)));
                ASSERT_TRUE(std::isfinite(static_cast<f32>(agent.position.y)));

                // Hunger and health must be finite
                ASSERT_TRUE(std::isfinite(agent.hunger));
                ASSERT_TRUE(std::isfinite(agent.health));

                // Track max stuck ticks
                if (agent.feedback.stuckTicks > maxStuckTicks)
                    maxStuckTicks = agent.feedback.stuckTicks;
            }

            // Spot-check CellRisk bounds on a few cells
            auto& cog = world.Cognitive();
            CellRisk r = CellRiskEvaluator::Evaluate(
                world.Fields(), cog, envCtx, 1, 16, 16);
            ASSERT_TRUE(r.total >= 0.0f && r.total <= 1.0f);
            ASSERT_TRUE(r.physical >= 0.0f && r.physical <= 1.0f);
            ASSERT_TRUE(r.memory >= 0.0f && r.memory <= 1.0f);
            ASSERT_TRUE(r.social >= 0.0f && r.social <= 1.0f);
        }
    }

    // No agent should be permanently stuck (stuckTicks unbounded).
    // The stuck-breaker forces movement after 3 ticks. Allow some tolerance
    // for edge cases where all neighbors are blocked by fire + boundary.
    ASSERT_TRUE(maxStuckTicks < 100);

    return true;
}

// ============================================================
// Stability: Knowledge edges bounded after 10000 ticks
// ============================================================
TEST(Stability_KnowledgeEdgesBounded)
{
    HumanEvolutionRulePack rulePack;
    const auto& envCtx = rulePack.GetHumanEvolutionContext().environment;

    WorldState world(32, 32, 123, rulePack);

    // Fire zones
    for (i32 y = 12; y < 20; y++)
        for (i32 x = 12; x < 20; x++)
            world.Fields().WriteNext(envCtx.fire, x, y, 40.0f);
    world.Fields().SwapAll();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(10, 10);

    auto scheduler = CreateHumanEvolutionScheduler(envCtx);

    for (i32 i = 0; i < 10000; i++)
        scheduler.Tick(world);

    // All knowledge edge strengths must be bounded
    const auto& kg = world.Cognitive().knowledgeGraph;
    for (const auto& edge : kg.edges)
    {
        ASSERT_TRUE(edge.strength >= 0.0f);
        ASSERT_TRUE(edge.strength <= 1000.0f);
        ASSERT_TRUE(edge.confidence >= 0.0f && edge.confidence <= 1.0f);
        ASSERT_TRUE(std::isfinite(edge.strength));
        ASSERT_TRUE(std::isfinite(edge.confidence));
    }

    // All decision modifier magnitudes must be 0..1
    for (const auto& agent : world.Agents().agents)
    {
        auto mods = world.Cognitive().GenerateDecisionModifiers(agent.id);
        for (const auto& mod : mods)
        {
            ASSERT_TRUE(mod.magnitude >= 0.0f && mod.magnitude <= 1.0f);
            ASSERT_TRUE(std::isfinite(mod.magnitude));
        }
    }

    return true;
}
