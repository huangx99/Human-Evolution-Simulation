#pragma once

// AgentDecisionSystem: runtime action selection based on perception + knowledge.
//
// This system scores three action candidates (flee, food, wander) using:
// 1. Runtime sensor data (nearestFire, nearestSmell, localTemperature)
// 2. Knowledge graph (learned danger/safety associations)
//
// Knowledge feedback: if an agent has learned that Fire→Causes→Danger,
// the flee score increases even if the fire is small. If an agent has
// learned that Food→Causes→Satiety, the food approach score increases.

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include "sim/cognitive/concept_tag.h"
#include "sim/cognitive/knowledge_relation.h"

class AgentDecisionSystem : public ISystem
{
public:
    void Update(WorldState& world) override
    {
        auto& cog = world.Cognitive();

        for (auto& agent : world.Agents().agents)
        {
            f32 fleeScore = 0.0f;
            f32 foodScore = 0.0f;
            f32 wanderScore = 0.1f;

            // Base flee score from fire proximity
            if (agent.nearestFire > 10.0f)
                fleeScore = agent.nearestFire * 2.0f;

            // Knowledge feedback: known danger amplifies flee
            // If agent knows Fire→Danger, even small fire triggers stronger flee
            if (agent.nearestFire > 3.0f)
            {
                f32 fireDanger = cog.GetKnownDanger(agent.id, ConceptTag::Fire);
                if (fireDanger > 0.0f)
                    fleeScore += fireDanger * 50.0f;
            }

            // Knowledge feedback: smoke means fire nearby (if agent knows this)
            // Uses cognitive summary from CognitivePerceptionSystem, NOT raw world field
            if (cog.HasKnowledgeLink(agent.id, ConceptTag::Smoke,
                                     ConceptTag::Fire, KnowledgeRelation::Signals))
            {
                auto smokeIt = cog.agentPerceivedSmoke.find(agent.id);
                if (smokeIt != cog.agentPerceivedSmoke.end() && smokeIt->second > 5.0f)
                    fleeScore += smokeIt->second * 3.0f;
            }

            // Base food score from hunger + smell
            if (agent.hunger > 30.0f && agent.nearestSmell > 5.0f)
                foodScore = agent.hunger * agent.nearestSmell * 0.01f;

            // Knowledge feedback: known food safety amplifies approach
            if (agent.hunger > 20.0f)
            {
                if (cog.HasKnowledgeLink(agent.id, ConceptTag::Food,
                                         ConceptTag::Satiety, KnowledgeRelation::Causes))
                {
                    foodScore *= 1.5f;  // 50% bonus: agent knows food is good
                }
            }

            // Temperature wander
            if (agent.localTemperature > 40.0f)
                wanderScore += (agent.localTemperature - 40.0f) * 0.1f;

            if (fleeScore > foodScore && fleeScore > wanderScore)
                agent.currentAction = AgentAction::Flee;
            else if (foodScore > wanderScore)
                agent.currentAction = AgentAction::MoveToFood;
            else
                agent.currentAction = AgentAction::Wander;
        }
    }
};
