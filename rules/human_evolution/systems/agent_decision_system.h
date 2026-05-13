#pragma once

// AgentDecisionSystem: runtime action selection based on perception + knowledge.
//
// ARCHITECTURE NOTE: This system uses DecisionModifiers generated from the
// knowledge graph, NOT hand-written knowledge checks. When a new knowledge
// type is added (e.g. "SharpEdge →Causes→ Cut"), the modifier system
// automatically produces an approach/avoidance bias. No code changes needed here.
//
// The only hard-coded logic is basic sensor thresholds (fire proximity,
// hunger + smell). Knowledge-driven behavior comes from modifiers.
//
// PHASE: SimPhase::Decision

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/concept_registry.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/cognitive/knowledge_relation.h"
#include "sim/cognitive/decision_modifier.h"

class AgentDecisionSystem : public ISystem
{
public:
    AgentDecisionSystem() = default;

    explicit AgentDecisionSystem(const HumanEvolution::ConceptContext& concepts)
        : concepts_(concepts) {}

    void Update(SystemContext& ctx) override
    {
        auto& world = ctx.World();
        auto& cog = world.Cognitive();

        for (auto& agent : world.Agents().agents)
        {
            f32 fleeScore = 0.0f;
            f32 foodScore = 0.0f;
            f32 wanderScore = 0.1f;

            // === Base sensor-driven scoring ===
            if (agent.nearestFire > 10.0f)
                fleeScore = agent.nearestFire * 2.0f;

            if (agent.hunger > 30.0f && agent.nearestSmell > 5.0f)
                foodScore = agent.hunger * agent.nearestSmell * 0.01f;

            if (agent.localTemperature > 40.0f)
                wanderScore += (agent.localTemperature - 40.0f) * 0.1f;

            // === Knowledge-driven scoring via DecisionModifiers ===
            auto mods = cog.GenerateDecisionModifiers(agent.id);
            for (const auto& mod : mods)
            {
                switch (mod.type)
                {
                case ModifierType::FleeBoost:
                    // If agent perceives the trigger concept, boost flee
                    if (IsConceptPerceived(agent, mod.triggerConcept, cog))
                        fleeScore += mod.magnitude * 50.0f;
                    break;

                case ModifierType::ApproachBoost:
                    // If agent perceives the trigger concept and is hungry, boost food
                    if (IsConceptPerceived(agent, mod.triggerConcept, cog) && agent.hunger > 20.0f)
                        foodScore += mod.magnitude * 30.0f;
                    break;

                case ModifierType::AlertBoost:
                    // Alert boost: if agent perceives the signal, boost flee slightly
                    if (IsConceptPerceived(agent, mod.triggerConcept, cog))
                        fleeScore += mod.magnitude * 20.0f;
                    break;
                }
            }

            if (fleeScore > foodScore && fleeScore > wanderScore)
                agent.currentAction = AgentAction::Flee;
            else if (foodScore > wanderScore)
                agent.currentAction = AgentAction::MoveToFood;
            else
                agent.currentAction = AgentAction::Wander;
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Cognitive, AccessMode::Read},
            {ModuleTag::Environment, AccessMode::Read},
            {ModuleTag::Information, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Command, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"AgentDecisionSystem", SimPhase::Decision, READS, 4, WRITES, 1, DEPS, 0, true, false};
    }

private:
    HumanEvolution::ConceptContext concepts_;

    // Check if agent currently perceives a concept (via cognitive summary)
    bool IsConceptPerceived(const Agent& agent, ConceptTypeId concept,
                             const CognitiveModule& cog) const
    {
        if (concept == concepts_.smoke)
        {
            auto it = cog.agentPerceivedSmoke.find(agent.id);
            return it != cog.agentPerceivedSmoke.end() && it->second > 5.0f;
        }
        if (concept == concepts_.fire)
            return agent.nearestFire > 5.0f;
        if (concept == concepts_.food || concept == concepts_.meat ||
            concept == concepts_.fruit)
            return agent.nearestSmell > 5.0f;
        if (concept == concepts_.heat)
            return agent.localTemperature > 35.0f;
        if (concept == concepts_.cold)
            return agent.localTemperature < 5.0f;
        return false;
    }
};
