#include "sim/world/world_state.h"
#include "sim/scheduler/scheduler.h"
#include "sim/scheduler/phase.h"
#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/reaction/semantic_reaction_system.h"
#include "rules/reaction/semantic_predicate.h"
#include "rules/reaction/reaction_effect.h"
#include "rules/reaction/semantic_reaction_rule.h"
#include "api/snapshot/world_snapshot.h"
#include "app/headless_console/ascii_viewer.h"
#include "app/headless_console/detailed_logger.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <string>
#include <sstream>

struct RunConfig
{
    u64 seed = 42;
    i32 ticks = 300;
    std::string dumpPath;
    bool quiet = false;
    bool ascii = false;
    bool log = false;
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
        else if (std::strcmp(argv[i], "--ascii") == 0)
            cfg.ascii = true;
        else if (std::strcmp(argv[i], "--log") == 0)
            cfg.log = true;
    }
    return cfg;
}

void PrintWorldState(const WorldState& world, const HumanEvolution::EnvironmentContext& envCtx, i32 interval)
{
    if (world.Sim().clock.currentTick % interval != 0) return;

    const auto& fm = world.Fields();
    auto& agentMod = world.Agents();
    auto& cog = world.Cognitive();

    std::cout << "=== Tick: " << world.Sim().clock.currentTick << " ===" << std::endl;

    i32 cx = world.Width() / 2;
    i32 cy = world.Height() / 2;
    std::cout << "Temperature (center): " << std::fixed << std::setprecision(1)
              << fm.Read(envCtx.temperature, cx, cy) << " C" << std::endl;
    std::cout << "Humidity (center): " << fm.Read(envCtx.humidity, cx, cy) << "%" << std::endl;
    std::cout << "Wind: (" << std::fixed << std::setprecision(2)
              << fm.Read(envCtx.windX, 0, 0) << ", " << fm.Read(envCtx.windY, 0, 0) << ")" << std::endl;

    i32 fireCount = 0;
    f32 maxFire = 0.0f;
    for (i32 y = 0; y < world.Height(); y++)
    {
        for (i32 x = 0; x < world.Width(); x++)
        {
            f32 f = fm.Read(envCtx.fire, x, y);
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

        // Cognitive state per agent
        const auto& mems = cog.GetAgentMemories(agent.id);
        if (!mems.empty())
        {
            std::cout << "  memories=" << mems.size();
            const MemoryRecord* strongest = &mems[0];
            for (const auto& m : mems)
            {
                if (m.strength > strongest->strength) strongest = &m;
            }
            std::cout << " strongest="
                      << ConceptTypeRegistry::Instance().GetName(strongest->subject)
                      << "(s=" << std::fixed << std::setprecision(2)
                      << strongest->strength << ")";
            std::cout << std::endl;
        }

        std::string knowledgeDump = cog.knowledgeGraph.Dump(agent.id);
        if (!knowledgeDump.empty())
        {
            std::cout << "  knowledge:\n";
            std::istringstream iss(knowledgeDump);
            std::string line;
            u32 shown = 0;
            while (std::getline(iss, line) && shown < 5)
            {
                std::cout << "    " << line << "\n";
                shown++;
            }
        }
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

    std::cout << "Human Evolution Simulation - Phase 2 (Cognitive)" << std::endl;
    std::cout << "Seed: " << cfg.seed << "  Ticks: " << cfg.ticks << std::endl;
    std::cout << "====================================" << std::endl << std::endl;

    HumanEvolutionRulePack rulePack;
    WorldState world(32, 32, cfg.seed, rulePack);
    const auto& envCtx = rulePack.GetHumanEvolutionContext().environment;
    auto& fm = world.Fields();

    fm.WriteNext(envCtx.fire, 16, 16, 80.0f);
    fm.WriteNext(envCtx.fire, 17, 16, 60.0f);
    fm.WriteNext(envCtx.fire, 16, 17, 60.0f);
    fm.WriteNext(envCtx.fire, 15, 16, 40.0f);
    fm.WriteNext(envCtx.fire, 8, 8, 60.0f);
    fm.WriteNext(envCtx.fire, 9, 8, 40.0f);
    fm.SwapAll();

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
        isWarm.field = FieldKey("human_evolution.temperature");
        isWarm.value = 25.0f;

        rule.conditions = {isFlesh, isDead, isWarm};

        ReactionEffect emitSmell;
        emitSmell.type = EffectType::EmitSmell;
        emitSmell.delta = 5.0f;

        rule.effects = {emitSmell};
        reactionSys->AddRule(rule);
    }

    Scheduler scheduler;

    // All systems from RulePack (environment + full cognitive pipeline)
    for (auto& reg : rulePack.CreateSystems())
        scheduler.AddSystem(reg.phase, std::move(reg.system));

    // World-specific reaction rules (not in RulePack — registered directly)
    scheduler.AddSystem(SimPhase::Reaction,     std::move(reactionSys));

    i32 printInterval = cfg.ticks / 12;
    if (printInterval < 1) printInterval = 1;

    if (cfg.ascii)
    {
        AsciiViewer viewer;
        viewer.Run(world, envCtx, scheduler, cfg.ticks);
    }
    else if (cfg.log)
    {
        for (i32 i = 0; i < cfg.ticks; i++)
        {
            scheduler.Tick(world);
            PrintDetailedLog(world, envCtx);
        }
    }
    else
    {
        for (i32 i = 0; i < cfg.ticks; i++)
        {
            scheduler.Tick(world);
            if (!cfg.quiet)
                PrintWorldState(world, envCtx, printInterval);
        }
    }

    if (!cfg.dumpPath.empty())
    {
        WorldSnapshot snap = WorldSnapshot::Capture(world, envCtx);
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
