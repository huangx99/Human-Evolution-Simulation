#pragma once

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/ecology/behavior_table.h"
#include <cmath>
#include <vector>

// CognitivePerceptionSystem: converts world state into subjective perception.
//
// ARCHITECTURE NOTE: This is the ONLY system that reads WorldState to produce
// cognitive data. All downstream systems (Attention, Memory, Discovery,
// Knowledge) consume PerceivedStimulus or its derivatives, NEVER raw WorldState.
//
// Moved from sim/system/ — depends on HumanEvolution field bindings.
// Uses EnvironmentContext for field access, ConceptContext for concept ids.

class CognitivePerceptionSystem : public ISystem
{
public:
    explicit CognitivePerceptionSystem(const HumanEvolutionContext& ctx)
        : envCtx_(ctx.environment), concepts_(ctx.concepts) {}

    // Convenience constructor for test helpers that only have EnvironmentContext.
    // Looks up concepts from global ConceptTypeRegistry by key.
    explicit CognitivePerceptionSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : envCtx_(envCtx)
    {
        const auto& reg = ConceptTypeRegistry::Instance();
        concepts_.fire      = reg.FindByKey(MakeConceptKey("human_evolution.fire"));
        concepts_.food      = reg.FindByKey(MakeConceptKey("human_evolution.food"));
        concepts_.water     = reg.FindByKey(MakeConceptKey("human_evolution.water"));
        concepts_.wood      = reg.FindByKey(MakeConceptKey("human_evolution.wood"));
        concepts_.stone     = reg.FindByKey(MakeConceptKey("human_evolution.stone"));
        concepts_.death     = reg.FindByKey(MakeConceptKey("human_evolution.death"));
        concepts_.heat      = reg.FindByKey(MakeConceptKey("human_evolution.heat"));
        concepts_.cold      = reg.FindByKey(MakeConceptKey("human_evolution.cold"));
        concepts_.danger    = reg.FindByKey(MakeConceptKey("human_evolution.danger"));
        concepts_.smoke     = reg.FindByKey(MakeConceptKey("human_evolution.smoke"));
    }

    void Update(SystemContext& ctx) override
    {
        auto& fm = ctx.GetFieldModule();
        auto& cog = ctx.Cognitive();
        auto& sim = ctx.Sim();
        auto& world = ctx.World();

        for (auto& agent : ctx.Agents().agents)
        {
            if (!agent.alive) continue;

            i32 ax = agent.position.x;
            i32 y = agent.position.y;

            // === Smell perception ===
            if (fm.InBounds(envCtx_.food, ax, y) && agent.nearestSmell > smellThreshold)
            {
                PerceivedStimulus s;
                s.id = cog.nextStimulusId++;
                s.observerId = agent.id;
                s.sense = SenseType::Smell;
                s.concept = concepts_.food;
                s.location = FindStrongestFieldLocation(fm, envCtx_.food, agent.position, foodSourceScanRadius);
                s.intensity = agent.nearestSmell / 50.0f;
                s.confidence = 0.7f;
                s.distance = Distance(agent.position, s.location);
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
                s.concept = concepts_.fire;
                s.location = FindStrongestFieldLocation(fm, envCtx_.fire, agent.position, scanRadius);
                s.intensity = agent.nearestFire / 80.0f;
                s.confidence = 0.9f;
                s.distance = Distance(agent.position, s.location);
                s.tick = sim.clock.currentTick;
                cog.frameStimuli.push_back(s);
            }

            // === Temperature perception ===
            if (agent.localTemperature > heatThreshold)
            {
                PerceivedStimulus s;
                s.id = cog.nextStimulusId++;
                s.observerId = agent.id;
                s.sense = SenseType::Thermal;
                s.concept = concepts_.heat;
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
                s.sense = SenseType::Thermal;
                s.concept = concepts_.cold;
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
                    s.sense = SenseType::Internal;
                    s.concept = concepts_.danger;
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
                if (smokeVal > smokeThreshold)
                {
                    PerceivedStimulus s;
                    s.id = cog.nextStimulusId++;
                s.observerId = agent.id;
                s.sense = SenseType::Smell;
                s.concept = concepts_.smoke;
                s.location = FindStrongestFieldLocation(fm, envCtx_.smoke, agent.position, scanRadius);
                s.intensity = smokeVal / 40.0f;
                s.confidence = 0.75f;
                s.distance = Distance(agent.position, s.location);
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

                // Get sense emission from recipe table
                SenseEmission emission = BehaviorTable::Instance().GetEmission(
                    entity->material, entity->state);

                // Infer concepts (multi-concept: e.g. corpse = food + death)
                std::vector<ConceptTypeId> concepts;
                InferConcepts(*entity, agent, concepts);

                for (auto concept : concepts)
                {
                    if (!concept) continue;

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

                    // Sense emission and subjective valence
                    s.rawEmission = emission;
                    s.valence = agent.sensoryProfile.ComputeValence(emission);
                    s.arousal = std::abs(s.valence);

                    cog.frameStimuli.push_back(s);
                }
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
    HumanEvolution::ConceptContext concepts_;

    static constexpr f32 smellThreshold = 3.0f;
    static constexpr f32 fireThreshold = 5.0f;
    static constexpr f32 heatThreshold = 35.0f;
    static constexpr f32 coldThreshold = 5.0f;
    static constexpr f32 dangerThreshold = 3.0f;
    static constexpr f32 smokeThreshold = 2.0f;
    static constexpr i32 scanRadius = 4;
    static constexpr i32 foodSourceScanRadius = 8;

    static f32 Distance(Vec2i a, Vec2i b)
    {
        f32 dx = static_cast<f32>(a.x - b.x);
        f32 dy = static_cast<f32>(a.y - b.y);
        return std::sqrt(dx * dx + dy * dy);
    }

    static Vec2i FindStrongestFieldLocation(const FieldModule& fm,
                                            FieldIndex field,
                                            Vec2i origin,
                                            i32 radius)
    {
        Vec2i best = origin;
        f32 bestScore = -1.0f;
        for (i32 dy = -radius; dy <= radius; ++dy)
        {
            for (i32 dx = -radius; dx <= radius; ++dx)
            {
                i32 x = origin.x + dx;
                i32 y = origin.y + dy;
                if (!fm.InBounds(field, x, y)) continue;

                f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy));
                if (dist > radius) continue;

                f32 score = fm.Read(field, x, y) / (1.0f + dist * 0.25f);
                if (score > bestScore)
                {
                    bestScore = score;
                    best = {x, y};
                }
            }
        }
        return best;
    }

    void InferConcepts(const EcologyEntity& entity, const Agent& agent,
                       std::vector<ConceptTypeId>& out) const
    {
        // Burning is a global priority (not per-recipe)
        if (entity.HasState(MaterialState::Burning))
        {
            out.push_back(concepts_.fire);
            return;
        }

        // Query recipe table
        BehaviorTable::Instance().GetConcepts(entity.material, entity.state, agent, out);

        // Fallback if recipe returned nothing
        if (out.empty())
        {
            if (entity.material == MaterialId::Water)
                out.push_back(concepts_.water);
            else if (entity.material == MaterialId::Wood ||
                     entity.material == MaterialId::Tree)
                out.push_back(concepts_.wood);
            else if (entity.material == MaterialId::Stone)
                out.push_back(concepts_.stone);
        }
    }
};
