#include "app/headless_console/detailed_logger.h"
#include "app/debug/chinese_debug_names.h"
#include "rules/human_evolution/commands.h"
#include "sim/command/command_buffer.h"
#include "sim/event/event_bus.h"
#include "sim/event/event_names.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

// ============================================================================
// Helpers: enums / bitfields → strings
// ============================================================================

static const char* MaterialToString(MaterialId m)
{
    switch (m)
    {
        case MaterialId::None:         return "None";
        case MaterialId::Dirt:         return "Dirt";
        case MaterialId::Sand:         return "Sand";
        case MaterialId::Stone:        return "Stone";
        case MaterialId::Clay:         return "Clay";
        case MaterialId::Grass:        return "Grass";
        case MaterialId::DryGrass:     return "DryGrass";
        case MaterialId::Bush:         return "Bush";
        case MaterialId::Wood:         return "Wood";
        case MaterialId::Tree:         return "Tree";
        case MaterialId::Water:        return "Water";
        case MaterialId::Ice:          return "Ice";
        case MaterialId::Mud:          return "Mud";
        case MaterialId::Flesh:        return "Flesh";
        case MaterialId::Bone:         return "Bone";
        case MaterialId::Leaf:         return "Leaf";
        case MaterialId::Fruit:        return "Fruit";
        case MaterialId::Charcoal:     return "Charcoal";
        case MaterialId::Ash:          return "Ash";
        case MaterialId::Coal:         return "Coal";
        case MaterialId::RottingPlant: return "RottingPlant";
        default:                       return "Unknown";
    }
}

static std::string StateFlagsToString(MaterialState s)
{
    std::vector<const char*> parts;
    if (HasState(s, MaterialState::Frozen))      parts.push_back("Frozen");
    if (HasState(s, MaterialState::Cold))        parts.push_back("Cold");
    if (HasState(s, MaterialState::Warm))        parts.push_back("Warm");
    if (HasState(s, MaterialState::Hot))         parts.push_back("Hot");
    if (HasState(s, MaterialState::Burning))     parts.push_back("Burning");
    if (HasState(s, MaterialState::Dry))         parts.push_back("Dry");
    if (HasState(s, MaterialState::Damp))        parts.push_back("Damp");
    if (HasState(s, MaterialState::Wet))         parts.push_back("Wet");
    if (HasState(s, MaterialState::Soaked))      parts.push_back("Soaked");
    if (HasState(s, MaterialState::Alive))       parts.push_back("Alive");
    if (HasState(s, MaterialState::Dead))        parts.push_back("Dead");
    if (HasState(s, MaterialState::Decomposing)) parts.push_back("Decomposing");
    if (HasState(s, MaterialState::Smoldering))  parts.push_back("Smoldering");
    if (HasState(s, MaterialState::Charred))     parts.push_back("Charred");
    if (HasState(s, MaterialState::AshCovered))  parts.push_back("AshCovered");
    if (parts.empty()) return "None";

    std::string r = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) { r += "|"; r += parts[i]; }
    return r;
}

static std::string CapFlagsToString(Capability c)
{
    std::vector<const char*> parts;
    if (HasCapability(c, Capability::Flammable))    parts.push_back("Flammable");
    if (HasCapability(c, Capability::Conducts))     parts.push_back("Conducts");
    if (HasCapability(c, Capability::Insulates))    parts.push_back("Insulates");
    if (HasCapability(c, Capability::Absorbent))    parts.push_back("Absorbent");
    if (HasCapability(c, Capability::RepelsWater))  parts.push_back("RepelsWater");
    if (HasCapability(c, Capability::Walkable))     parts.push_back("Walkable");
    if (HasCapability(c, Capability::Passable))     parts.push_back("Passable");
    if (HasCapability(c, Capability::BlocksWind))   parts.push_back("BlocksWind");
    if (HasCapability(c, Capability::Edible))       parts.push_back("Edible");
    if (HasCapability(c, Capability::Digestible))   parts.push_back("Digestible");
    if (HasCapability(c, Capability::Toxic))        parts.push_back("Toxic");
    if (HasCapability(c, Capability::Grows))        parts.push_back("Grows");
    if (HasCapability(c, Capability::Decays))       parts.push_back("Decays");
    if (HasCapability(c, Capability::Buildable))    parts.push_back("Buildable");
    if (HasCapability(c, Capability::Stackable))    parts.push_back("Stackable");
    if (HasCapability(c, Capability::LightEmission))parts.push_back("LightEmission");
    if (HasCapability(c, Capability::HeatEmission)) parts.push_back("HeatEmission");
    if (HasCapability(c, Capability::SmokeEmission))parts.push_back("SmokeEmission");
    if (HasCapability(c, Capability::Portable))     parts.push_back("Portable");
    if (HasCapability(c, Capability::SharpEdge))    parts.push_back("SharpEdge");
    if (HasCapability(c, Capability::LongReach))    parts.push_back("LongReach");
    if (HasCapability(c, Capability::Binding))      parts.push_back("Binding");
    if (HasCapability(c, Capability::Container))    parts.push_back("Container");
    if (HasCapability(c, Capability::Weapon))       parts.push_back("Weapon");
    if (HasCapability(c, Capability::Tool))         parts.push_back("Tool");
    if (parts.empty()) return "None";

    std::string r = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) { r += "|"; r += parts[i]; }
    return r;
}

static std::string AffFlagsToString(Affordance a)
{
    std::vector<const char*> parts;
    if (HasAffordance(a, Affordance::CanIgnite))       parts.push_back("CanIgnite");
    if (HasAffordance(a, Affordance::CanBurn))         parts.push_back("CanBurn");
    if (HasAffordance(a, Affordance::CanExtinguish))   parts.push_back("CanExtinguish");
    if (HasAffordance(a, Affordance::CanEat))          parts.push_back("CanEat");
    if (HasAffordance(a, Affordance::CanGather))       parts.push_back("CanGather");
    if (HasAffordance(a, Affordance::CanStore))        parts.push_back("CanStore");
    if (HasAffordance(a, Affordance::CanWalk))         parts.push_back("CanWalk");
    if (HasAffordance(a, Affordance::CanClimb))        parts.push_back("CanClimb");
    if (HasAffordance(a, Affordance::CanSwim))         parts.push_back("CanSwim");
    if (HasAffordance(a, Affordance::CanBuild))        parts.push_back("CanBuild");
    if (HasAffordance(a, Affordance::CanDig))          parts.push_back("CanDig");
    if (HasAffordance(a, Affordance::CanMine))         parts.push_back("CanMine");
    if (HasAffordance(a, Affordance::CanRest))         parts.push_back("CanRest");
    if (HasAffordance(a, Affordance::CanHide))         parts.push_back("CanHide");
    if (HasAffordance(a, Affordance::CanSignal))       parts.push_back("CanSignal");
    if (HasAffordance(a, Affordance::CanGrow))         parts.push_back("CanGrow");
    if (HasAffordance(a, Affordance::CanCut))          parts.push_back("CanCut");
    if (HasAffordance(a, Affordance::CanPoke))         parts.push_back("CanPoke");
    if (HasAffordance(a, Affordance::CanCarry))        parts.push_back("CanCarry");
    if (HasAffordance(a, Affordance::CanLight))        parts.push_back("CanLight");
    if (HasAffordance(a, Affordance::CanScare))        parts.push_back("CanScare");
    if (HasAffordance(a, Affordance::CanIgniteTarget)) parts.push_back("CanIgniteTarget");
    if (HasAffordance(a, Affordance::CanBind))         parts.push_back("CanBind");
    if (parts.empty()) return "None";

    std::string r = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) { r += "|"; r += parts[i]; }
    return r;
}


static std::string CommandToString(const CommandBase& cmd)
{
    switch (cmd.GetKind())
    {
        case CommandKind::MoveAgent: {
            auto* c = dynamic_cast<const MoveAgentCommand*>(&cmd);
            return std::string("MoveAgent agent=") + std::to_string(c->id)
                + " pos=(" + std::to_string(c->x) + "," + std::to_string(c->y) + ")";
        }
        case CommandKind::SetAgentAction: {
            auto* c = dynamic_cast<const SetAgentActionCommand*>(&cmd);
            return std::string("SetAgentAction agent=") + std::to_string(c->id)
                + " action=" + ChineseActionName(c->action);
        }
        case CommandKind::DamageAgent: {
            auto* c = dynamic_cast<const DamageAgentCommand*>(&cmd);
            return std::string("DamageAgent agent=") + std::to_string(c->id)
                + " amount=" + std::to_string(c->amount);
        }
        case CommandKind::FeedAgent: {
            auto* c = dynamic_cast<const FeedAgentCommand*>(&cmd);
            return std::string("FeedAgent agent=") + std::to_string(c->id)
                + " amount=" + std::to_string(c->amount);
        }
        case CommandKind::ModifyHunger: {
            auto* c = dynamic_cast<const ModifyHungerCommand*>(&cmd);
            return std::string("ModifyHunger agent=") + std::to_string(c->id)
                + " delta=" + std::to_string(c->delta);
        }
        case CommandKind::AddEntityState: {
            auto* c = dynamic_cast<const AddEntityStateCommand*>(&cmd);
            return std::string("AddEntityState entity=") + std::to_string(c->id)
                + " state=" + StateFlagsToString(static_cast<MaterialState>(c->state));
        }
        case CommandKind::RemoveEntityState: {
            auto* c = dynamic_cast<const RemoveEntityStateCommand*>(&cmd);
            return std::string("RemoveEntityState entity=") + std::to_string(c->id)
                + " state=" + StateFlagsToString(static_cast<MaterialState>(c->state));
        }
        case CommandKind::AddEntityCapability: {
            auto* c = dynamic_cast<const AddEntityCapabilityCommand*>(&cmd);
            return std::string("AddEntityCapability entity=") + std::to_string(c->id)
                + " cap=" + CapFlagsToString(static_cast<Capability>(c->capability));
        }
        case CommandKind::RemoveEntityCapability: {
            auto* c = dynamic_cast<const RemoveEntityCapabilityCommand*>(&cmd);
            return std::string("RemoveEntityCapability entity=") + std::to_string(c->id)
                + " cap=" + CapFlagsToString(static_cast<Capability>(c->capability));
        }
        case CommandKind::ModifyFieldValue: {
            auto* c = dynamic_cast<const ModifyFieldValueCommand*>(&cmd);
            return std::string("ModifyFieldValue field=") + std::to_string(c->field.value)
                + " pos=(" + std::to_string(c->x) + "," + std::to_string(c->y) + ")"
                + " mode=" + std::to_string(c->mode)
                + " value=" + std::to_string(c->value);
        }
        case HumanEvolution::CmdKind::IgniteFire: {
            auto* c = dynamic_cast<const IgniteFireCommand*>(&cmd);
            return std::string("IgniteFire pos=(") + std::to_string(c->x) + "," + std::to_string(c->y)
                + ") intensity=" + std::to_string(c->intensity);
        }
        case HumanEvolution::CmdKind::ExtinguishFire: {
            auto* c = dynamic_cast<const ExtinguishFireCommand*>(&cmd);
            return std::string("ExtinguishFire pos=(") + std::to_string(c->x) + "," + std::to_string(c->y) + ")";
        }
        case HumanEvolution::CmdKind::EmitSmell: {
            auto* c = dynamic_cast<const EmitSmellCommand*>(&cmd);
            return std::string("EmitSmell pos=(") + std::to_string(c->x) + "," + std::to_string(c->y)
                + ") amount=" + std::to_string(c->amount);
        }
        case HumanEvolution::CmdKind::SetDanger: {
            auto* c = dynamic_cast<const SetDangerCommand*>(&cmd);
            return std::string("SetDanger pos=(") + std::to_string(c->x) + "," + std::to_string(c->y)
                + ") value=" + std::to_string(c->value);
        }
        case HumanEvolution::CmdKind::EmitSmoke: {
            auto* c = dynamic_cast<const EmitSmokeCommand*>(&cmd);
            return std::string("EmitSmoke pos=(") + std::to_string(c->x) + "," + std::to_string(c->y)
                + ") amount=" + std::to_string(c->amount);
        }
        default:
            return std::string("Command(kind=") + std::to_string(static_cast<u16>(cmd.GetKind())) + ")";
    }
}

// ============================================================================
// Main logger
// ============================================================================

void PrintDetailedLog(const WorldState& world,
                      const HumanEvolution::EnvironmentContext& envCtx)
{
    std::ostringstream out;
    Tick tick = world.Sim().clock.currentTick;
    Tick submitTick = (tick > 0) ? tick - 1 : 0;
    const auto& regConcept = ConceptTypeRegistry::Instance();

    out << "\n========== Tick " << tick << " ==========\n";

    // ------------------------------------------------------------------------
    // Environment
    // ------------------------------------------------------------------------
    {
        i32 cx = world.Width() / 2;
        i32 cy = world.Height() / 2;
        f32 temp  = world.Fields().Read(envCtx.temperature, cx, cy);
        f32 humid = world.Fields().Read(envCtx.humidity, cx, cy);
        f32 windX = world.Fields().Read(envCtx.windX, 0, 0);
        f32 windY = world.Fields().Read(envCtx.windY, 0, 0);

        i32 fireCount = 0;
        f32 maxFire = 0.0f;
        for (i32 y = 0; y < world.Height(); ++y)
            for (i32 x = 0; x < world.Width(); ++x)
            {
                f32 f = world.Fields().Read(envCtx.fire, x, y);
                if (f > 0.0f) { ++fireCount; if (f > maxFire) maxFire = f; }
            }

        out << "[Environment]\n";
        out << "  Temperature(center): " << std::fixed << std::setprecision(1) << temp << "\n";
        out << "  Humidity(center):    " << std::fixed << std::setprecision(1) << humid << "\n";
        out << "  Wind:                (" << std::fixed << std::setprecision(2) << windX << ", " << windY << ")\n";
        out << "  FireCells:           " << fireCount << "\n";
        out << "  MaxFire:             " << std::fixed << std::setprecision(1) << maxFire << "\n";
    }

    // ------------------------------------------------------------------------
    // Agents (个体状态)
    // ------------------------------------------------------------------------
    {
        out << "[Agents] count=" << world.Agents().agents.size() << "\n";
        const auto& cog = world.Cognitive();
        const auto& baselines = world.InternalStateBaselines().baselines;

        for (const auto& agent : world.Agents().agents)
        {
            out << "  Agent_" << agent.id << ":\n";
            out << "    Position:      (" << agent.position.x << "," << agent.position.y << ")\n";
            out << "    Hunger:        " << std::fixed << std::setprecision(1) << agent.hunger << "\n";
            out << "    Health:        " << std::fixed << std::setprecision(1) << agent.health << "\n";
            out << "    Alive:         " << (agent.alive ? "true" : "false") << "\n";
            out << "    Action:        " << ChineseActionName(agent.currentAction) << "\n";
            out << "    Perception:    smell=" << agent.nearestSmell
                << " fire=" << agent.nearestFire
                << " temp=" << agent.localTemperature << "\n";

            // 当前关注
            bool hasFocus = false;
            for (const auto& f : cog.frameFocused)
            {
                if (f.stimulus.observerId == agent.id)
                {
                    if (!hasFocus)
                    {
                        out << "    CurrentFocus:\n";
                        hasFocus = true;
                    }
                    out << "      concept=" << ChineseConceptName(f.stimulus.concept)
                        << " score=" << std::fixed << std::setprecision(2) << f.attentionScore
                        << " sense=" << ChineseSenseName(f.stimulus.sense)
                        << " pos=(" << f.stimulus.location.x << "," << f.stimulus.location.y << ")\n";
                }
            }
            if (!hasFocus)
                out << "    CurrentFocus:  (none)\n";

            // 记忆
            const auto& mems = cog.GetAgentMemories(agent.id);
            out << "    Memories:      " << mems.size() << "\n";
            for (const auto& m : mems)
            {
                out << "      " << ChineseConceptName(m.subject)
                    << " kind=" << static_cast<int>(m.kind)
                    << " strength=" << std::fixed << std::setprecision(2) << m.strength
                    << " pos=(" << m.location.x << "," << m.location.y << ")\n";
            }

            // 知识统计
            size_t knowledgeNodes = 0;
            for (const auto& n : cog.knowledgeGraph.nodes)
                if (n.ownerAgentId == agent.id) ++knowledgeNodes;
            size_t knowledgeEdges = 0;
            for (const auto& e : cog.knowledgeGraph.edges)
            {
                const auto* fromNode = cog.knowledgeGraph.FindNodeById(e.fromNodeId);
                if (fromNode && fromNode->ownerAgentId == agent.id)
                    ++knowledgeEdges;
            }
            out << "    KnowledgeNodes:" << knowledgeNodes << "\n";
            out << "    KnowledgeEdges:" << knowledgeEdges << "\n";
        }
    }

    // ------------------------------------------------------------------------
    // CognitiveInput (当前认知输入)
    // ------------------------------------------------------------------------
    {
        const auto& cog = world.Cognitive();
        out << "[CognitiveInput]\n";

        // 感知刺激
        out << "  FrameStimuli:    " << cog.frameStimuli.size() << "\n";
        for (const auto& s : cog.frameStimuli)
        {
            out << "    observer=" << s.observerId
                << " concept=" << ChineseConceptName(s.concept)
                << " sense=" << ChineseSenseName(s.sense)
                << " pos=(" << s.location.x << "," << s.location.y << ")"
                << " intensity=" << std::fixed << std::setprecision(2) << s.intensity
                << " confidence=" << s.confidence << "\n";
        }

        // 关注刺激
        out << "  FrameFocused:    " << cog.frameFocused.size() << "\n";
        for (const auto& f : cog.frameFocused)
        {
            out << "    observer=" << f.stimulus.observerId
                << " concept=" << ChineseConceptName(f.stimulus.concept)
                << " score=" << std::fixed << std::setprecision(2) << f.attentionScore
                << " sense=" << ChineseSenseName(f.stimulus.sense)
                << " pos=(" << f.stimulus.location.x << "," << f.stimulus.location.y << ")\n";
        }

        // 当前 tick 形成的记忆
        out << "  FrameMemories:   " << cog.frameMemories.size() << "\n";
        for (const auto& m : cog.frameMemories)
        {
            out << "    owner=" << m.ownerId
                << " concept=" << ChineseConceptName(m.subject)
                << " strength=" << std::fixed << std::setprecision(2) << m.strength
                << " pos=(" << m.location.x << "," << m.location.y << ")\n";
        }

        // 当前 tick 的发现
        out << "  FrameDiscoveries:" << cog.frameDiscoveries.size() << "\n";
        for (const auto& d : cog.frameDiscoveries)
        {
            out << "    owner=" << d.ownerId
                << " " << ChineseConceptName(d.causeConcept)
                << " -> " << ChineseConceptName(d.effectConcept)
                << " confidence=" << std::fixed << std::setprecision(2) << d.confidence << "\n";
        }
    }

    // ------------------------------------------------------------------------
    // CognitiveHypotheses (认知假设)
    // ------------------------------------------------------------------------
    {
        const auto& cog = world.Cognitive();
        out << "[CognitiveHypotheses]\n";
        bool any = false;
        for (const auto& agent : world.Agents().agents)
        {
            auto it = cog.agentHypotheses.find(agent.id);
            if (it != cog.agentHypotheses.end() && !it->second.empty())
            {
                any = true;
                out << "  Agent_" << agent.id << ": count=" << it->second.size() << "\n";
                for (const auto& h : it->second)
                {
                    out << "    " << ChineseConceptName(h.causeConcept)
                        << " -> " << ChineseConceptName(h.effectConcept)
                        << " conf=" << std::fixed << std::setprecision(2) << h.confidence
                        << " support=" << h.supportingCount
                        << " contra=" << h.contradictingCount << "\n";
                }
            }
        }
        if (!any)
            out << "  (none)\n";
    }

    // ------------------------------------------------------------------------
    // Ecology
    // ------------------------------------------------------------------------
    {
        out << "[Ecology] count=" << world.Ecology().entities.Count() << "\n";
        for (const auto& entity : world.Ecology().entities.All())
        {
            out << "  " << entity.name << ":\n";
            out << "    Id:           " << entity.id << "\n";
            out << "    Material:     " << MaterialToString(entity.material) << "\n";
            out << "    Position:     (" << entity.x << "," << entity.y << ")\n";
            out << "    State:        " << StateFlagsToString(entity.state) << "\n";
            out << "    Capabilities: " << CapFlagsToString(entity.GetCapabilities()) << "\n";
            out << "    Affordances:  " << AffFlagsToString(entity.GetAffordances()) << "\n";
        }
    }

    // ------------------------------------------------------------------------
    // SocialSignals (社会信号)
    // ------------------------------------------------------------------------
    {
        const auto& sigMod = world.SocialSignals();
        Tick now = tick;
        out << "[SocialSignals] count=" << sigMod.Count() << "\n";
        if (sigMod.activeSignals.empty())
            out << "  (none)\n";
        else
        {
            for (const auto& s : sigMod.activeSignals)
            {
                u32 remaining = 0;
                if (now < s.createdTick + s.ttl)
                    remaining = static_cast<u32>(s.createdTick + s.ttl - now);
                out << "  " << ChineseSocialSignalName(s.typeId)
                    << " source=" << s.sourceEntityId
                    << " pos=(" << s.origin.x << "," << s.origin.y << ")"
                    << " intensity=" << std::fixed << std::setprecision(2) << s.intensity
                    << " ttl_remaining=" << remaining << "\n";
            }
        }
    }

    // ------------------------------------------------------------------------
    // GroupKnowledge (群体知识)
    // ------------------------------------------------------------------------
    {
        const auto& gkMod = world.GroupKnowledge();
        out << "[GroupKnowledge] count=" << gkMod.records.size() << "\n";
        if (gkMod.records.empty())
            out << "  (none)\n";
        else
        {
            for (const auto& r : gkMod.records)
            {
                out << "  " << ChineseGroupKnowledgeName(r.typeId)
                    << " id=" << r.id
                    << " pos=(" << r.origin.x << "," << r.origin.y << ")"
                    << " radius=" << std::fixed << std::setprecision(1) << r.radius
                    << " confidence=" << std::fixed << std::setprecision(2) << r.confidence
                    << " contributors=" << r.contributors << "\n";
            }
        }
    }

    // ------------------------------------------------------------------------
    // Patterns (群体行为模式)
    // ------------------------------------------------------------------------
    {
        const auto& patMod = world.Patterns();
        out << "[Patterns] count=" << patMod.Count() << "\n";
        if (patMod.All().empty())
            out << "  (none)\n";
        else
        {
            for (const auto& p : patMod.All())
            {
                out << "  " << ChinesePatternName(p.typeId)
                    << " id=" << p.id
                    << " pos=(" << p.x << "," << p.y << ")"
                    << " confidence=" << std::fixed << std::setprecision(2) << p.confidence
                    << " magnitude=" << p.magnitude
                    << " observations=" << p.observationCount << "\n";
            }
        }
    }

    // ------------------------------------------------------------------------
    // CulturalTraces (文化痕迹)
    // ------------------------------------------------------------------------
    {
        const auto& ctMod = world.CulturalTrace();
        out << "[CulturalTraces] count=" << ctMod.records.size() << "\n";
        if (ctMod.records.empty())
            out << "  (none)\n";
        else
        {
            for (const auto& t : ctMod.records)
            {
                out << "  " << ChineseCulturalTraceName(t.typeId)
                    << " id=" << t.id
                    << " confidence=" << std::fixed << std::setprecision(2) << t.confidence
                    << " reinforcements=" << t.reinforcementCount
                    << " sourcePatterns=[";
                for (size_t i = 0; i < t.sourcePatternIds.size(); ++i)
                {
                    if (i > 0) out << ",";
                    out << t.sourcePatternIds[i];
                }
                out << "] sourceGK=[";
                for (size_t i = 0; i < t.sourceGroupKnowledgeRecordIds.size(); ++i)
                {
                    if (i > 0) out << ",";
                    out << t.sourceGroupKnowledgeRecordIds[i];
                }
                out << "]\n";
            }
        }
    }

    // ------------------------------------------------------------------------
    // HistoryEvents (历史事件)
    // ------------------------------------------------------------------------
    {
        const auto& histMod = world.History();
        out << "[HistoryEvents] count=" << histMod.Count() << "\n";
        if (histMod.Events().empty())
            out << "  (none)\n";
        else
        {
            for (const auto& e : histMod.Events())
            {
                out << "  " << ChineseHistoryEventName(e.typeId)
                    << " id=" << e.id
                    << " tick=" << e.tick
                    << " pos=(" << e.x << "," << e.y << ")"
                    << " magnitude=" << std::fixed << std::setprecision(2) << e.magnitude
                    << " confidence=" << e.confidence;
                if (!e.involvedEntities.empty())
                {
                    out << " involved=[";
                    for (size_t i = 0; i < e.involvedEntities.size(); ++i)
                    {
                        if (i > 0) out << ",";
                        out << e.involvedEntities[i];
                    }
                    out << "]";
                }
                out << "\n";
            }
        }
    }

    // ------------------------------------------------------------------------
    // Commands (executed this tick)
    // ------------------------------------------------------------------------
    {
        std::vector<std::string> tickCmds;
        for (const auto& qc : world.commands.GetHistory())
        {
            if (qc.tick == submitTick && qc.command)
                tickCmds.push_back(CommandToString(*qc.command));
        }
        out << "[Commands] count=" << tickCmds.size() << "\n";
        if (tickCmds.empty())
            out << "  (none)\n";
        else
            for (const auto& s : tickCmds)
                out << "  " << s << "\n";
    }

    // ------------------------------------------------------------------------
    // Events (dispatched this tick)
    // ------------------------------------------------------------------------
    {
        std::vector<std::string> tickEvents;
        for (const auto& e : world.events.GetArchive())
        {
            if (e.tick == submitTick)
            {
                std::string s = EventTypeName(e.type);
                s += " entity=" + std::to_string(e.entityId);
                s += " pos=(" + std::to_string(e.x) + "," + std::to_string(e.y) + ")";
                s += " value=" + std::to_string(e.value);
                tickEvents.push_back(s);
            }
        }
        out << "[Events] count=" << tickEvents.size() << "\n";
        if (tickEvents.empty())
            out << "  (none)\n";
        else
            for (const auto& s : tickEvents)
                out << "  " << s << "\n";
    }

    out << "==================================\n";
    std::cout << out.str() << std::flush;
}
