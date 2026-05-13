#pragma once

// GroupKnowledgeAwarenessSystem: lets GroupKnowledge zones generate
// PerceivedStimuli for nearby agents, closing the feedback loop:
//   GroupKnowledge → PerceivedStimulus → Attention → Memory
//
// First version: only shared_danger_zone awareness.
// Uses AwarenessCooldownModule for cross-tick cooldown (not system-private state).
//
// READS: Agent, Simulation, GroupKnowledge, AwarenessCooldown
// WRITES: Cognitive (frameStimuli), AwarenessCooldown (ReadWrite)
// PHASE: SimPhase::Perception (before CognitiveAttentionSystem)
// DOES NOT WRITE: Command, Memory, Agent, GroupKnowledge, Pattern, CulturalTrace, History

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include <cmath>

class GroupKnowledgeAwarenessSystem : public ISystem
{
public:
    GroupKnowledgeAwarenessSystem(ConceptTypeId groupDangerConcept,
                                  GroupKnowledgeTypeId dangerZoneType)
        : groupDangerConcept_(groupDangerConcept)
        , dangerZoneType_(dangerZoneType) {}

    void Update(SystemContext& sys) override
    {
        auto& agents = sys.Agents().agents;
        auto& cog = sys.Cognitive();
        auto& gk = sys.GroupKnowledge();
        auto& cooldown = sys.AwarenessCooldown();
        Tick now = sys.CurrentTick();

        auto zones = gk.FindByType(dangerZoneType_);

        for (const auto& agent : agents)
        {
            if (!agent.alive)
                continue;

            for (const auto* zone : zones)
            {
                if (zone->confidence < kMinConfidence)
                    continue;

                f32 dx = static_cast<f32>(agent.position.x - zone->origin.x);
                f32 dy = static_cast<f32>(agent.position.y - zone->origin.y);
                f32 distance = std::sqrt(dx * dx + dy * dy);
                f32 awarenessRadius = zone->radius * kAwarenessRadiusMultiplier;

                if (distance > awarenessRadius)
                    continue;

                if (!cooldown.CanEmit(agent.id, zone->id, groupDangerConcept_, now, kCooldownTicks))
                    continue;

                PerceivedStimulus stim;
                stim.id = cog.nextStimulusId++;
                stim.observerId = agent.id;
                stim.sourceEntityId = 0;  // group knowledge is not an entity
                stim.sense = SenseType::Social;
                stim.concept = groupDangerConcept_;
                stim.location = zone->origin;
                stim.intensity = zone->confidence * (1.0f - distance / awarenessRadius);
                stim.confidence = zone->confidence;
                stim.distance = distance;
                stim.tick = now;
                cog.frameStimuli.push_back(stim);

                cooldown.MarkEmitted(agent.id, zone->id, groupDangerConcept_, now);
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Simulation, AccessMode::Read},
            {ModuleTag::GroupKnowledge, AccessMode::Read},
            {ModuleTag::AwarenessCooldown, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::Write},
            {ModuleTag::AwarenessCooldown, AccessMode::ReadWrite}
        };
        return {"GroupKnowledgeAwarenessSystem", SimPhase::Perception,
                READS, 4, WRITES, 2, nullptr, 0, true, false};
    }

private:
    ConceptTypeId groupDangerConcept_;
    GroupKnowledgeTypeId dangerZoneType_;

    static constexpr f32 kAwarenessRadiusMultiplier = 1.5f;
    static constexpr Tick kCooldownTicks = 20;
    static constexpr f32 kMinConfidence = 0.35f;
};
