#pragma once

// AgentIntent: what the agent wants to do, separated from how it moves.
//
// Intent is selected based on drives, risk, and knowledge.
// NavigationPolicy translates intent into a concrete move.
// AgentIntentType is defined in sim/entity/agent_action.h.

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/entity/agent.h"
#include "sim/cognitive/cognitive_module.h"
#include "sim/cognitive/concept_registry.h"
#include "rules/human_evolution/runtime/agent_drives.h"
#include <optional>

struct AgentIntent
{
    AgentIntentType type = AgentIntentType::None;

    f32 urgency = 0.0f;
    bool hasTarget = false;
    Vec2i targetPosition{};
    ConceptTypeId targetConcept{};

    f32 riskTolerance = 0.3f;
};

struct KnownFoodTarget
{
    Vec2i position;
    ConceptTypeId concept;
};

struct IntentSelector
{
    static f32 ComputeRiskTolerance(AgentIntentType type,
                                    const Agent& agent,
                                    const AgentDrives& drives)
    {
        switch (type)
        {
        case AgentIntentType::Escape:
            return 0.95f;

        case AgentIntentType::Forage:
            return std::clamp(0.35f + drives.hungerUrgency * 0.45f, 0.35f, 0.80f);

        case AgentIntentType::ApproachKnownFood:
            return std::clamp(0.30f + drives.hungerUrgency * 0.35f, 0.30f, 0.65f);

        case AgentIntentType::Explore:
            return 0.25f;

        case AgentIntentType::Investigate:
            return 0.35f;

        case AgentIntentType::Rest:
            return 0.20f;

        default:
            return 0.30f;
        }
    }

    // Find the best known food target from memory.
    static std::optional<KnownFoodTarget> FindBestKnownFood(
        const Agent& agent,
        const CognitiveModule& cog)
    {
        const auto& reg = ConceptTypeRegistry::Instance();
        const MemoryRecord* bestMemory = nullptr;
        f32 bestScore = -1e9f;

        for (const auto& memory : cog.GetAgentMemories(agent.id))
        {
            if (reg.GetName(memory.subject) != "food") continue;
            if (memory.sense != SenseType::Vision) continue;
            if (memory.reinforcementCount < 3) continue;

            f32 score = memory.strength * std::max(0.1f, memory.confidence) +
                        static_cast<f32>(memory.reinforcementCount) * 2.0f;
            if (score > bestScore)
            {
                bestScore = score;
                bestMemory = &memory;
            }
        }

        if (!bestMemory) return std::nullopt;

        KnownFoodTarget target;
        target.position = bestMemory->location;
        target.concept = bestMemory->subject;
        return target;
    }

    static AgentIntent Select(const Agent& agent,
                              const AgentDrives& drives,
                              const CognitiveModule& cog)
    {
        AgentIntent intent;
        const auto& fb = agent.feedback;

        // === Intent commitment: if locked in, keep going ===
        // Break commitment on: emergency danger spike or stuck
        if (fb.intentCommitTicks > 0 &&
            fb.lastIntent != AgentIntentType::None &&
            fb.stuckTicks == 0 &&
            !(fb.lastIntent != AgentIntentType::Escape && drives.dangerUrgency > 0.85f))
        {
            intent.type = fb.lastIntent;
            intent.urgency = (fb.lastIntent == AgentIntentType::Escape)
                ? drives.dangerUrgency : drives.hungerUrgency;

            // For targeted intents, reconstruct target
            if (fb.lastIntent == AgentIntentType::ApproachKnownFood)
            {
                auto knownFood = FindBestKnownFood(agent, cog);
                if (knownFood.has_value())
                {
                    intent.hasTarget = true;
                    intent.targetPosition = knownFood->position;
                    intent.targetConcept = knownFood->concept;
                }
                else
                {
                    // Lost food target — break commitment
                    intent.type = AgentIntentType::Forage;
                }
            }

            intent.riskTolerance = ComputeRiskTolerance(intent.type, agent, drives);
            return intent;
        }

        // === Hysteresis: use lower exit thresholds when committed to an intent ===
        const bool isEscaping = (fb.lastIntent == AgentIntentType::Escape);
        const bool isForaging = (fb.lastIntent == AgentIntentType::Forage ||
                                 fb.lastIntent == AgentIntentType::ApproachKnownFood);

        // Entry thresholds
        const bool immediateDanger = drives.dangerUrgency > 0.65f;
        const bool starving = drives.hungerUrgency > 0.75f;
        // Exit thresholds (lower than entry to prevent flapping)
        const bool dangerLingering = drives.dangerUrgency > 0.30f;
        const bool hungerLingering = drives.hungerUrgency > 0.50f;

        if (immediateDanger && !starving)
        {
            intent.type = AgentIntentType::Escape;
            intent.urgency = drives.dangerUrgency;
        }
        else if (starving)
        {
            // Starving overrides lingering danger
            if (isEscaping && dangerLingering && drives.dangerUrgency > 0.45f)
            {
                // Danger still significant while starving — escape first
                intent.type = AgentIntentType::Escape;
                intent.urgency = drives.dangerUrgency;
            }
            else
            {
                auto knownFood = FindBestKnownFood(agent, cog);
                if (knownFood.has_value())
                {
                    intent.type = AgentIntentType::ApproachKnownFood;
                    intent.hasTarget = true;
                    intent.targetPosition = knownFood->position;
                    intent.targetConcept = knownFood->concept;
                    intent.urgency = drives.hungerUrgency;
                }
                else
                {
                    intent.type = AgentIntentType::Forage;
                    intent.urgency = drives.hungerUrgency;
                }
            }
        }
        else if (drives.dangerUrgency > 0.45f)
        {
            intent.type = AgentIntentType::Escape;
            intent.urgency = drives.dangerUrgency;
        }
        // Hysteresis: keep escaping if danger hasn't dropped enough
        else if (isEscaping && dangerLingering)
        {
            intent.type = AgentIntentType::Escape;
            intent.urgency = drives.dangerUrgency;
        }
        else if (drives.hungerUrgency > 0.35f)
        {
            auto knownFood = FindBestKnownFood(agent, cog);
            if (knownFood.has_value())
            {
                intent.type = AgentIntentType::ApproachKnownFood;
                intent.hasTarget = true;
                intent.targetPosition = knownFood->position;
                intent.targetConcept = knownFood->concept;
                intent.urgency = drives.hungerUrgency;
            }
            else
            {
                intent.type = AgentIntentType::Forage;
                intent.urgency = drives.hungerUrgency;
            }
        }
        // Hysteresis: keep foraging if hunger hasn't dropped enough
        else if (isForaging && hungerLingering)
        {
            auto knownFood = FindBestKnownFood(agent, cog);
            if (knownFood.has_value())
            {
                intent.type = AgentIntentType::ApproachKnownFood;
                intent.hasTarget = true;
                intent.targetPosition = knownFood->position;
                intent.targetConcept = knownFood->concept;
                intent.urgency = drives.hungerUrgency;
            }
            else
            {
                intent.type = AgentIntentType::Forage;
                intent.urgency = drives.hungerUrgency;
            }
        }
        else
        {
            intent.type = AgentIntentType::Explore;
            intent.urgency = 0.2f;
        }

        intent.riskTolerance = ComputeRiskTolerance(intent.type, agent, drives);
        return intent;
    }
};
