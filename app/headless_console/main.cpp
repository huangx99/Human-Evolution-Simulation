#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/scheduler/phase.h"
#include "sim/system/climate_system.h"
#include "sim/system/fire_system.h"
#include "sim/system/smell_system.h"
#include "sim/system/agent_perception_system.h"
#include "sim/system/agent_decision_system.h"
#include "sim/system/agent_action_system.h"
#include "rules/reaction/semantic_reaction_system.h"
#include "rules/reaction/semantic_predicate.h"
#include "rules/reaction/reaction_effect.h"
#include "rules/reaction/semantic_reaction_rule.h"
#include "api/snapshot/world_snapshot.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <string>

struct RunConfig
{
    u64 seed = 42;
    i32 ticks = 300;
    std::string dumpPath;
    bool quiet = false;
};

RunConfig ParseArgs(int argc, char* argv[])
{
    RunConfig cfg;
    for (int i = 1; i < argc; i++)
    {
        if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
            cfg.seed = std::stoull(argv[++i]);
        else if (std::strcmp(argv[i], "--ticks") == 0 && i + 1 < argc)
            cfg.ticks = std::stoi(argv[++i]);
        else if (std::strcmp(argv[i], "--dump") == 0 && i + 1 < argc)
            cfg.dumpPath = argv[++i];
        else if (std::strcmp(argv[i], "--quiet") == 0)
            cfg.quiet = true;
    }
    return cfg;
}

void PrintWorldState(const WorldState& world, i32 interval)
{
    if (world.Sim().clock.currentTick % interval != 0) return;

    auto& env = world.Env();
    auto& agentMod = world.Agents();

    std::cout << "=== Tick: " << world.Sim().clock.currentTick << " ===" << std::endl;

    i32 cx = env.width / 2;
    i32 cy = env.height / 2;
    std::cout << "Temperature (center): " << std::fixed << std::setprecision(1)
              << env.temperature.At(cx, cy) << " C" << std::endl;
    std::cout << "Humidity (center): " << env.humidity.At(cx, cy) << "%" << std::endl;
    std::cout << "Wind: (" << std::fixed << std::setprecision(2)
              << env.wind.x << ", " << env.wind.y << ")" << std::endl;

    i32 fireCount = 0;
    f32 maxFire = 0.0f;
    for (i32 y = 0; y < env.height; y++)
    {
        for (i32 x = 0; x < env.width; x++)
        {
            f32 f = env.fire.At(x, y);
            if (f > 0.0f) { fireCount++; if (f > maxFire) maxFire = f; }
        }
    }
    std::cout << "Fire cells: " << fireCount << " (max: "
              << std::fixed << std::setprecision(1) << maxFire << ")" << std::endl;

    for (const auto& agent : agentMod.agents)
    {
        const char* actionStr = "idle";
        switch (agent.currentAction)
        {
        case AgentAction::MoveToFood: actionStr = "move_to_food"; break;
        case AgentAction::Flee:       actionStr = "flee"; break;
        case AgentAction::Wander:     actionStr = "wander"; break;
        case AgentAction::Idle:       actionStr = "idle"; break;
        }

        std::cout << "Agent_" << agent.id
                  << " pos=(" << agent.position.x << "," << agent.position.y << ")"
                  << " hunger=" << std::fixed << std::setprecision(0) << agent.hunger
                  << " health=" << agent.health
                  << " action=" << actionStr << std::endl;
    }

    // Ecology entities
    auto& ecology = world.Ecology().entities;
    for (const auto& entity : ecology.All())
    {
        std::cout << "  " << entity.name << " (" << entity.x << "," << entity.y << ")";
        if (HasState(entity.state, MaterialState::Burning))   std::cout << " [BURNING]";
        if (HasState(entity.state, MaterialState::Dead))      std::cout << " [DEAD]";
        if (HasState(entity.state, MaterialState::Wet))       std::cout << " [WET]";
        if (HasState(entity.state, MaterialState::Dry))       std::cout << " [DRY]";
        if (entity.HasCapability(Capability::HeatEmission))   std::cout << " [HEAT]";
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    RunConfig cfg = ParseArgs(argc, argv);

    std::cout << "Human Evolution Simulation - Phase 1" << std::endl;
    std::cout << "Seed: " << cfg.seed << "  Ticks: " << cfg.ticks << std::endl;
    std::cout << "====================================" << std::endl << std::endl;

    WorldState world(32, 32, cfg.seed);

    world.Env().fire.WriteNext(16, 16) = 80.0f;
    world.Env().fire.WriteNext(17, 16) = 60.0f;
    world.Env().fire.WriteNext(16, 17) = 60.0f;
    world.Env().fire.WriteNext(15, 16) = 40.0f;
    world.Env().fire.WriteNext(8, 8) = 60.0f;
    world.Env().fire.WriteNext(9, 8) = 40.0f;
    world.Env().fire.Swap();

    world.SpawnAgent(5, 5);
    world.SpawnAgent(10, 20);
    world.SpawnAgent(25, 10);

    // Ecology entities
    auto& ecology = world.Ecology().entities;

    auto& hotStone = ecology.Create(MaterialId::Stone, "hot_stone");
    hotStone.x = 16; hotStone.y = 16;
    hotStone.AddCapability(Capability::HeatEmission);

    auto& dryGrass = ecology.Create(MaterialId::DryGrass, "dry_grass");
    dryGrass.x = 17; dryGrass.y = 16;

    auto& dryGrass2 = ecology.Create(MaterialId::DryGrass, "dry_grass_2");
    dryGrass2.x = 15; dryGrass2.y = 16;

    auto& wetWood = ecology.Create(MaterialId::Wood, "wet_wood");
    wetWood.x = 18; wetWood.y = 16;
    wetWood.state = MaterialState::Wet;

    auto& corpse = ecology.Create(MaterialId::Flesh, "corpse");
    corpse.x = 8; corpse.y = 8;
    corpse.state = MaterialState::Dead;

    auto& coal = ecology.Create(MaterialId::Coal, "coal");
    coal.x = 20; coal.y = 20;

    world.RebuildSpatial();

    // Reaction rules
    auto reactionSys = std::make_unique<SemanticReactionSystem>();

    // Ignition: Flammable + Dry + NearbyHeat → Burning + HeatEmission
    {
        SemanticReactionRule rule;
        rule.id = "ignite";
        rule.name = "Flammable+Dry+Heat → Burning";
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
        reactionSys->AddRule(rule);
    }

    // Decay: Flesh + Dead + Temp>25 → EmitSmell
    {
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
        emitSmell.delta = 5.0f;

        rule.effects = {emitSmell};
        reactionSys->AddRule(rule);
    }

    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Environment,  std::make_unique<ClimateSystem>());
    scheduler.AddSystem(SimPhase::Reaction,     std::move(reactionSys));
    scheduler.AddSystem(SimPhase::Propagation,  std::make_unique<FireSystem>());
    scheduler.AddSystem(SimPhase::Propagation,  std::make_unique<SmellSystem>());
    scheduler.AddSystem(SimPhase::Perception,   std::make_unique<AgentPerceptionSystem>());
    scheduler.AddSystem(SimPhase::Decision,     std::make_unique<AgentDecisionSystem>());
    scheduler.AddSystem(SimPhase::Action,       std::make_unique<AgentActionSystem>());

    i32 printInterval = cfg.ticks / 12;
    if (printInterval < 1) printInterval = 1;

    for (i32 i = 0; i < cfg.ticks; i++)
    {
        scheduler.Tick(world);
        if (!cfg.quiet)
            PrintWorldState(world, printInterval);
    }

    if (!cfg.dumpPath.empty())
    {
        WorldSnapshot snap = WorldSnapshot::Capture(world);
        std::string data = snap.Serialize();
        std::ofstream ofs(cfg.dumpPath);
        if (ofs.is_open())
        {
            ofs << data;
            ofs.close();
            std::cout << "Snapshot dumped to: " << cfg.dumpPath << std::endl;
        }
        else
        {
            std::cerr << "Failed to open dump file: " << cfg.dumpPath << std::endl;
            return 1;
        }
    }

    std::cout << "Simulation complete." << std::endl;
    return 0;
}
