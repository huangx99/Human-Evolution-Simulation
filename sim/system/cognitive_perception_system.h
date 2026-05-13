#pragma once

// CognitivePerceptionSystem: converts world state into subjective perception.
//
// ARCHITECTURE NOTE: This is the ONLY system that reads WorldState to produce
// cognitive data. All downstream systems (Attention, Memory, Discovery,
// Knowledge) consume PerceivedStimulus or its derivatives, NEVER raw WorldState.
// This is a hard architectural boundary that prevents agents from accessing
// objective truth.
//
// This system is DIFFERENT from AgentPerceptionSystem which fills raw runtime
// fields (nearestSmell, nearestFire, localTemperature). Those are deterministic
// sensor readings used by the action system. CognitivePerceptionSystem creates
// PerceivedStimulus objects that represent what the agent *subjectively notices*.
//
// Pipeline position: runs AFTER AgentPerceptionSystem (same SimPhase::Perception).
// Reads: Agent runtime fields, environment fields, ecology entities.
// Writes: CognitiveModule::frameStimuli.
//
// OWNERSHIP: Engine (sim/system/)
// READS: EnvironmentModule (fire, temperature), InformationModule (smell, danger, smoke), AgentModule (agents), EcologyModule (entities)
// WRITES: CognitiveModule (frameStimuli)
// PHASE: SimPhase::Perception

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/concept_tag.h"
#include <cmath>

class CognitivePerceptionSystem : public ISystem
{
public:
    void Update(SystemContext& ctx) override
    {
        auto& world = ctx.World();
        auto& env = world.Env();
        auto& info = world.Info();
        auto& cog = world.Cognitive();
        auto& sim = world.Sim();

        for (auto& agent : world.Agents().agents)
        {
            i32 ax = agent.position.x;
            i32 y = agent.position.y;

            // === Smell perception ===
            if (info.info0.InBounds(ax, y) && agent.nearestSmell > smellThreshold)
            {
                PerceivedStimulus s;
                s.id = cog.nextStimulusId++;
                s.observerId = agent.id;
                s.sense = SenseType::Smell;
                s.concept = ConceptTag::Food;  // default: smell = food nearby
                s.location = agent.position;
                s.intensity = agent.nearestSmell / 50.0f;  // normalize
                s.confidence = 0.7f;  // smell is imprecise
                s.distance = 0.0f;
                s.tick = sim.clock.currentTick;
                cog.frameStimuli.push_back(s);
            }

            // === Fire / danger perception ===
            if (agent.nearestFire > fireThreshold)
            {
                PerceivedStimulus s;
                s.id = cog.nextStimulusId++;
                s.observerId = agent.id;
                s.sense = SenseType::Vision;
                s.concept = ConceptTag::Fire;
                s.location = agent.position;
                s.intensity = agent.nearestFire / 80.0f;
                s.confidence = 0.9f;  // fire is very visible
                s.distance = 0.0f;
                s.tick = sim.clock.currentTick;
                cog.frameStimuli.push_back(s);
            }

            // === Temperature perception ===
            if (agent.localTemperature > heatThreshold)
            {
                PerceivedStimulus s;
                s.id = cog.nextStimulusId++;
                s.observerId = agent.id;
                s.sense = SenseType::Heat;
                s.concept = ConceptTag::Heat;
                s.location = agent.position;
                s.intensity = (agent.localTemperature - 30.0f) / 30.0f;
                s.confidence = 0.8f;
                s.distance = 0.0f;
                s.tick = sim.clock.currentTick;
                cog.frameStimuli.push_back(s);
            }
            else if (agent.localTemperature < coldThreshold)
            {
                PerceivedStimulus s;
                s.id = cog.nextStimulusId++;
                s.observerId = agent.id;
                s.sense = SenseType::Heat;
                s.concept = ConceptTag::Cold;
                s.location = agent.position;
                s.intensity = (10.0f - agent.localTemperature) / 20.0f;
                s.confidence = 0.8f;
                s.distance = 0.0f;
                s.tick = sim.clock.currentTick;
                cog.frameStimuli.push_back(s);
            }

            // === Danger field perception ===
            if (info.info1.InBounds(ax, y))
            {
                f32 dangerVal = info.info1.At(ax, y);
                if (dangerVal > dangerThreshold)
                {
                    PerceivedStimulus s;
                    s.id = cog.nextStimulusId++;
                    s.observerId = agent.id;
                    s.sense = SenseType::Danger;
                    s.concept = ConceptTag::Danger;
                    s.location = agent.position;
                    s.intensity = dangerVal / 50.0f;
                    s.confidence = 0.6f;  // danger is felt, not seen
                    s.distance = 0.0f;
                    s.tick = sim.clock.currentTick;
                    cog.frameStimuli.push_back(s);
                }
            }

            // === Smoke perception ===
            // Also populates agentPerceivedSmoke for DecisionSystem
            if (info.info2.InBounds(ax, y))
            {
                f32 smokeVal = info.info2.At(ax, y);
                cog.agentPerceivedSmoke[agent.id] = smokeVal;
                if (smokeVal > smokeThreshold)
                {
                    PerceivedStimulus s;
                    s.id = cog.nextStimulusId++;
                    s.observerId = agent.id;
                    s.sense = SenseType::Smoke;
                    s.concept = ConceptTag::Smoke;
                    s.location = agent.position;
                    s.intensity = smokeVal / 40.0f;
                    s.confidence = 0.75f;
                    s.distance = 0.0f;
                    s.tick = sim.clock.currentTick;
                    cog.frameStimuli.push_back(s);
                }
            }

            // === Nearby ecology entity perception ===
            auto nearbyEntities = world.spatial.QueryArea(ax, y, scanRadius);
            for (auto* entity : nearbyEntities)
            {
                if (entity->x == ax && entity->y == y) continue;  // skip self-cell

                f32 dx = static_cast<f32>(entity->x - ax);
                f32 dy = static_cast<f32>(entity->y - y);
                f32 dist = std::sqrt(dx * dx + dy * dy);
                if (dist > scanRadius) continue;

                // Determine concept from entity properties
                ConceptTag concept = InferConcept(*entity);
                if (concept == ConceptTag::None) continue;

                PerceivedStimulus s;
                s.id = cog.nextStimulusId++;
                s.observerId = agent.id;
                s.sourceEntityId = entity->id;
                s.sense = SenseType::Vision;
                s.concept = concept;
                s.location = {entity->x, entity->y};
                s.intensity = 1.0f / (1.0f + dist * 0.3f);  // distance falloff
                s.confidence = 0.85f / (1.0f + dist * 0.1f);
                s.distance = dist;
                s.tick = sim.clock.currentTick;
                cog.frameStimuli.push_back(s);
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Environment, AccessMode::Read},
            {ModuleTag::Information, AccessMode::Read},
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Ecology, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"CognitivePerceptionSystem", SimPhase::Perception, READS, 4, WRITES, 1, DEPS, 0, true, false};
    }

private:
    static constexpr f32 smellThreshold = 3.0f;
    static constexpr f32 fireThreshold = 5.0f;
    static constexpr f32 heatThreshold = 35.0f;
    static constexpr f32 coldThreshold = 5.0f;
    static constexpr f32 dangerThreshold = 3.0f;
    static constexpr f32 smokeThreshold = 2.0f;
    static constexpr i32 scanRadius = 4;

    // Infer what concept an ecology entity represents to an agent.
    static ConceptTag InferConcept(const EcologyEntity& entity)
    {
        // Burning entities = Fire
        if (entity.HasState(MaterialState::Burning))
            return ConceptTag::Fire;

        // Edible entities = Food
        if (entity.HasCapability(Capability::Edible))
            return ConceptTag::Food;

        // Water
        if (entity.material == MaterialId::Water)
            return ConceptTag::Water;

        // Wood
        if (entity.material == MaterialId::Wood || entity.material == MaterialId::Tree)
            return ConceptTag::Wood;

        // Stone
        if (entity.material == MaterialId::Stone)
            return ConceptTag::Stone;

        // Dead flesh = danger / death signal
        if (entity.HasState(MaterialState::Dead) && entity.material == MaterialId::Flesh)
            return ConceptTag::Death;

        return ConceptTag::None;
    }
};
