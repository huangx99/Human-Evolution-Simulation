#pragma once

// AgentActionSystem: executes agent behavior based on intent + navigation policy.
//
// Phase 2.7 refactor: replaces IsHazardous/TotalHazard/MemoryHazard with
// CellRiskEvaluator + NavigationPolicy. Movement is intent-driven, not action-driven.
//
// Deleted: IsHazardous, TotalHazard, MemoryHazard, CellHazard
// These are replaced by CellRiskEvaluator::Evaluate which outputs continuous risk.

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/cognitive/concept_registry.h"
#include "sim/ecology/behavior_table.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/navigation_policy.h"
#include "rules/human_evolution/runtime/food_assessment.h"
#include "rules/human_evolution/runtime/action_feedback.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>

class AgentActionSystem : public ISystem
{
public:
    explicit AgentActionSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : envCtx_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& fm = ctx.GetFieldModule();
        auto& commands = ctx.Commands();
        auto& cog = ctx.Cognitive();
        auto tick = ctx.CurrentTick();

        for (auto& agent : ctx.Agents().agents)
        {
            if (!agent.alive) continue;

            // Hunger increases every tick
            commands.Submit(tick, ModifyHungerCommand{agent.id, 0.15f});
            MarkVisibleExplored(ctx.World(), agent);
            InvalidateVisibleMissingFoodMemories(ctx.World(), cog, agent);

            // Build intent from agent's current intent (set by DecisionSystem)
            AgentIntent intent;
            intent.type = agent.currentIntent;
            intent.riskTolerance = IntentSelector::ComputeRiskTolerance(
                intent.type, agent, DriveEvaluator::Evaluate(
                    agent, CellRiskEvaluator::Evaluate(
                        fm, cog, envCtx_, agent.id,
                        agent.position.x, agent.position.y)));

            // Select move via NavigationPolicy
            MoveDecision move;
            switch (intent.type)
            {
            case AgentIntentType::Escape:
                move = NavigationPolicy::ChooseEscapeMove(
                    fm, cog, envCtx_, agent, intent, agent.feedback);
                break;

            case AgentIntentType::ApproachKnownFood:
            {
                // Reconstruct target from intent selector
                auto knownFood = IntentSelector::FindBestKnownFood(agent, cog);
                if (knownFood.has_value())
                {
                    intent.hasTarget = true;
                    intent.targetPosition = knownFood->position;
                }
                move = NavigationPolicy::ChooseApproachTargetMove(
                    fm, cog, envCtx_, agent, intent, agent.feedback);
                break;
            }

            case AgentIntentType::Forage:
                move = NavigationPolicy::ChooseForageMove(
                    fm, cog, envCtx_, agent, intent, agent.feedback,
                    static_cast<f32>(tick % 7));
                break;

            case AgentIntentType::Explore:
            case AgentIntentType::Investigate:
            default:
                move = NavigationPolicy::ChooseExploreMove(
                    fm, cog, envCtx_, agent, intent, agent.feedback,
                    static_cast<f32>(tick % 7));
                break;
            }

            // Execute move if valid
            Vec2i oldPos = agent.position;
            Vec2i intendedPos = oldPos;
            bool moveExecuted = false;

            if (move.foundCandidate)
            {
                i32 newX = agent.position.x + move.dx;
                i32 newY = agent.position.y + move.dy;

                if (fm.InBounds(envCtx_.temperature, newX, newY))
                {
                    // Only block on physical lethality, not subjective risk
                    CellRisk targetRisk = CellRiskEvaluator::Evaluate(
                        fm, cog, envCtx_, agent.id, newX, newY);

                    if (targetRisk.traversal != TraversalClass::Blocked)
                    {
                        commands.Submit(tick, MoveAgentCommand{agent.id, newX, newY});
                        intendedPos = {newX, newY};
                        moveExecuted = true;
                    }
                }
            }

            // Update feedback: use intended position since commands execute later
            FeedbackUpdater::Update(
                agent.feedback, intent,
                move.attemptedMove, moveExecuted,
                oldPos, intendedPos);

            // Eating: use unified food assessment
            if (HasConsumableFoodAt(ctx.World(), agent, agent.position.x, agent.position.y))
            {
                commands.Submit(tick, FeedAgentCommand{agent.id, 5.0f});
                agent.feedback.actionSucceeded = true;
            }
            else
            {
                agent.feedback.actionSucceeded = false;
            }

            // Heat damage
            if (agent.localTemperature > 50.0f)
            {
                f32 damage = (agent.localTemperature - 50.0f) * 0.1f;
                commands.Submit(tick, DamageAgentCommand{agent.id, damage});
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Command, AccessMode::Write}
        };
        static const char* const DEPS[] = {"AgentDecisionSystem"};
        return {"AgentActionSystem", SimPhase::Action, READS, 2, WRITES, 1, DEPS, 1, true, false};
    }

private:
    HumanEvolution::EnvironmentContext envCtx_;

    // --- Exploration state (retained from old system) ---

    struct ExploreState
    {
        std::unordered_set<i32> explored;
        Vec2i target{0, 0};
        bool hasTarget = false;
        Vec2i previousPosition{0, 0};
        bool hasPreviousPosition = false;
    };

    std::unordered_map<EntityId, ExploreState> exploration_;

    static i32 CellKey(const WorldState& world, i32 x, i32 y)
    {
        return y * world.Width() + x;
    }

    static bool InWorld(const WorldState& world, i32 x, i32 y)
    {
        return x >= 0 && y >= 0 && x < world.Width() && y < world.Height();
    }

    bool IsVisibleToAgent(const WorldState& world, const Agent& agent, Vec2i location) const
    {
        if (!InWorld(world, location.x, location.y)) return false;
        i32 dx = agent.position.x - location.x;
        i32 dy = agent.position.y - location.y;
        return dx * dx + dy * dy <= visualExplorationRadius * visualExplorationRadius;
    }

    void MarkVisibleExplored(const WorldState& world, const Agent& agent)
    {
        auto& state = exploration_[agent.id];
        i32 radiusSq = visualExplorationRadius * visualExplorationRadius;
        for (i32 y = agent.position.y - visualExplorationRadius;
             y <= agent.position.y + visualExplorationRadius; ++y)
        {
            for (i32 x = agent.position.x - visualExplorationRadius;
                 x <= agent.position.x + visualExplorationRadius; ++x)
            {
                if (!InWorld(world, x, y)) continue;
                i32 dx = x - agent.position.x;
                i32 dy = y - agent.position.y;
                if (dx * dx + dy * dy > radiusSq) continue;
                state.explored.insert(CellKey(world, x, y));
            }
        }
    }

    void InvalidateVisibleMissingFoodMemories(const WorldState& world,
                                              CognitiveModule& cog,
                                              const Agent& agent) const
    {
        const auto& conceptRegistry = ConceptTypeRegistry::Instance();
        for (auto& memory : cog.GetAgentMemories(agent.id))
        {
            if (conceptRegistry.GetName(memory.subject) != "food") continue;
            if (memory.sense != SenseType::Vision) continue;
            if (!IsVisibleToAgent(world, agent, memory.location)) continue;
            if (HasConsumableFoodAt(world, agent, memory.location.x, memory.location.y)) continue;

            memory.strength *= missingFoodMemoryStrengthScale;
            memory.confidence *= missingFoodMemoryConfidenceScale;
            memory.reinforcementCount = 1;
            memory.kind = MemoryKind::ShortTerm;
        }
    }

    // Unified food check using FoodAssessor
    static bool HasConsumableFoodAt(const WorldState& world, const Agent& agent,
                                    i32 x, i32 y)
    {
        for (const auto& entity : world.Ecology().entities.All())
        {
            if (entity.x != x || entity.y != y) continue;
            if (FoodAssessor::IsConsumable(entity, agent))
                return true;
        }
        return false;
    }

    static constexpr i32 visualExplorationRadius = 4;
    static constexpr f32 missingFoodMemoryStrengthScale = 0.2f;
    static constexpr f32 missingFoodMemoryConfidenceScale = 0.5f;
};
