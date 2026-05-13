#pragma once

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/cognitive/concept_tag.h"
#include <cmath>

// CognitivePerceptionSystem: converts world state into subjective perception.
//
// ARCHITECTURE NOTE: This is the ONLY system that reads WorldState to produce
// cognitive data. All downstream systems (Attention, Memory, Discovery,
// Knowledge) consume PerceivedStimulus or its derivatives, NEVER raw WorldState.
//
// Moved from sim/system/ — depends on HumanEvolution field bindings.
// Uses EnvironmentContext for field access via FieldModule.

class CognitivePerceptionSystem : public ISystem
{
public:
    explicit CognitivePerceptionSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : envCtx_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& fm = ctx.GetFieldModule();
        auto& cog = ctx.Cognitive();
        auto& sim = ctx.Sim();
        auto& world = ctx.World();

        for (auto& agent : ctx.Agents().agents)
        {
            i32 ax = agent.position.x;
            i32 y = agent.position.y;

            // === Smell perception ===
            if (fm.InBounds(envCtx_.smell, ax, y) && agent.nearestSmell > smellThreshold)
            {
                PerceivedStimulus s;
                s.id = cog.nextStimulusId++;
                s.observerId = agent.id;
                s.sense = SenseType::Smell;
                s.concept = ConceptTag::Food;
                s.location = agent.position;
                s.intensity = agent.nearestSmell / 50.0f;
                s.confidence = 0.7f;
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
                s.confidence = 0.9f;
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
            if (fm.InBounds(envCtx_.danger, ax, y))
            {
                f32 dangerVal = fm.Read(envCtx_.danger, ax, y);
                if (dangerVal > dangerThreshold)
                {
                    PerceivedStimulus s;
                    s.id = cog.nextStimulusId++;
                    s.observerId = agent.id;
                    s.sense = SenseType::Danger;
                    s.concept = ConceptTag::Danger;
                    s.location = agent.position;
                    s.intensity = dangerVal / 50.0f;
                    s.confidence = 0.6f;
                    s.distance = 0.0f;
                    s.tick = sim.clock.currentTick;
                    cog.frameStimuli.push_back(s);
                }
            }

            // === Smoke perception ===
            if (fm.InBounds(envCtx_.smoke, ax, y))
            {
                f32 smokeVal = fm.Read(envCtx_.smoke, ax, y);
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
                if (entity->x == ax && entity->y == y) continue;

                f32 dx = static_cast<f32>(entity->x - ax);
                f32 dy = static_cast<f32>(entity->y - y);
                f32 dist = std::sqrt(dx * dx + dy * dy);
                if (dist > scanRadius) continue;

                ConceptTag concept = InferConcept(*entity);
                if (concept == ConceptTag::None) continue;

                PerceivedStimulus s;
                s.id = cog.nextStimulusId++;
                s.observerId = agent.id;
                s.sourceEntityId = entity->id;
                s.sense = SenseType::Vision;
                s.concept = concept;
                s.location = {entity->x, entity->y};
                s.intensity = 1.0f / (1.0f + dist * 0.3f);
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
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Ecology, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"CognitivePerceptionSystem", SimPhase::Perception, READS, 2, WRITES, 1, DEPS, 0, true, false};
    }

private:
    HumanEvolution::EnvironmentContext envCtx_;

    static constexpr f32 smellThreshold = 3.0f;
    static constexpr f32 fireThreshold = 5.0f;
    static constexpr f32 heatThreshold = 35.0f;
    static constexpr f32 coldThreshold = 5.0f;
    static constexpr f32 dangerThreshold = 3.0f;
    static constexpr f32 smokeThreshold = 2.0f;
    static constexpr i32 scanRadius = 4;

    static ConceptTag InferConcept(const EcologyEntity& entity)
    {
        if (entity.HasState(MaterialState::Burning))
            return ConceptTag::Fire;
        if (entity.HasCapability(Capability::Edible))
            return ConceptTag::Food;
        if (entity.material == MaterialId::Water)
            return ConceptTag::Water;
        if (entity.material == MaterialId::Wood || entity.material == MaterialId::Tree)
            return ConceptTag::Wood;
        if (entity.material == MaterialId::Stone)
            return ConceptTag::Stone;
        if (entity.HasState(MaterialState::Dead) && entity.material == MaterialId::Flesh)
            return ConceptTag::Death;
        return ConceptTag::None;
    }
};
