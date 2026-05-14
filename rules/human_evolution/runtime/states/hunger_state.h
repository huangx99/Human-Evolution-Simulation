#pragma once

// hunger_state.h: Seek food when hungry. Priority 3.
//
// Trigger: hungerUrgency > 0.35 (via DriveEvaluator)
// Behavior: A* pathfinding to known food (if memory) or Forage (smell gradient)
//   Uses LocalPerceptionMap for fog-of-war pathfinding
//   Scans vision radius each tick to update perception
// Eating: handled by AgentStateActionSystem (system layer, not this state)
// Complete: agent.hunger < 20 (ate enough via system-layer FeedAgentCommand)
// Stuck: after 5 ticks, mark memory as failed, clear target

#include "rules/human_evolution/runtime/agent_state.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/agent_drives.h"
#include "rules/human_evolution/runtime/navigation_policy.h"
#include "rules/human_evolution/runtime/agent_intent.h"
#include "rules/human_evolution/runtime/pathfinder.h"
#include "sim/entity/agent.h"
#include "sim/field/field_module.h"
#include "sim/cognitive/cognitive_module.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/command/command_buffer.h"
#include <cmath>

class HungerState : public IStateBehavior
{
public:
    StateId GetStateId() const override { return StateId::Hunger; }
    const char* GetName() const override { return "Hunger"; }
    i32 GetPriority(const Agent&) const override { return StatePriority::Hunger; }
    AgentIntentType GetIntentType() const override { return AgentIntentType::Forage; }
    AgentAction GetDisplayAction(const AgentState& state) const override
    {
        return state.context.hasTarget ? AgentAction::SeekFood : AgentAction::Wander;
    }

    bool CanTrigger(const StateDecideContext& ctx) const override
    {
        CellRisk currentRisk = CellRiskEvaluator::Evaluate(
            ctx.fields, ctx.cognitive, ctx.envCtx,
            ctx.agent.id, ctx.agent.position.x, ctx.agent.position.y);
        AgentDrives drives = DriveEvaluator::Evaluate(ctx.agent, currentRisk);
        return drives.hungerUrgency > 0.35f;
    }

    void OnEnter(AgentState& state, const StateDecideContext& ctx) override
    {
        // Init perception map on first entry
        if (state.context.perceptionMap.width == 0)
            state.context.perceptionMap.Init(ctx.fields.Width(), ctx.fields.Height());

        // Scan current vision area
        ScanVisibleArea(state.context, ctx.agent.position, ctx.fields, ctx.cognitive, ctx.envCtx, ctx.agent.id);

        // Check memory for known food (cognitive pipeline)
        auto knownFood = IntentSelector::FindBestKnownFood(ctx.agent, ctx.cognitive);
        if (knownFood.has_value())
        {
            state.context.hasTarget = true;
            state.context.targetPosition = knownFood->position;
            state.context.pathfinderActive = true;
            state.context.cachedPath.clear();
            state.context.pathIndex = 0;
            state.context.pathAge = 0;
        }

        CellRisk currentRisk = CellRiskEvaluator::Evaluate(
            ctx.fields, ctx.cognitive, ctx.envCtx,
            ctx.agent.id, ctx.agent.position.x, ctx.agent.position.y);
        AgentDrives drives = DriveEvaluator::Evaluate(ctx.agent, currentRisk);
        state.context.urgency = drives.hungerUrgency;
        state.context.riskTolerance = std::clamp(
            0.35f + drives.hungerUrgency * 0.45f, 0.35f, 0.80f);
    }

    void OnResume(AgentState& state, const StateExecuteContext& ctx) override
    {
        // Re-scan vision area (conditions may have changed while paused)
        ScanVisibleArea(state.context, ctx.agent.position, ctx.fields, ctx.cognitive, ctx.envCtx, ctx.agent.id);

        // Re-check food memory on resume
        auto knownFood = IntentSelector::FindBestKnownFood(ctx.agent, ctx.cognitive);
        if (knownFood.has_value())
        {
            state.context.hasTarget = true;
            state.context.targetPosition = knownFood->position;
            state.context.pathfinderActive = true;
            // Invalidate cached path (conditions may have changed while paused)
            state.context.cachedPath.clear();
            state.context.pathIndex = 0;
            state.context.pathAge = 0;
        }
    }

    void OnPause(AgentState&) override {}

    void Execute(AgentState& state, const StateExecuteContext& ctx) override
    {
        // Scan vision area each tick (fog of war updates)
        ScanVisibleArea(state.context, ctx.agent.position, ctx.fields, ctx.cognitive, ctx.envCtx, ctx.agent.id);

        // Stuck handling: after 5 ticks, mark memory as failed and clear target
        if (state.context.stuckTicks >= 5 && state.context.hasTarget)
        {
            state.context.incrementFailedApproach++;
            state.context.hasTarget = false;
            state.context.pathfinderActive = false;
            state.context.stuckTicks = 0;
        }

        // Stuck handling: escalate risk tolerance
        if (state.context.stuckTicks >= 3)
            state.context.riskTolerance = std::min(1.0f, state.context.riskTolerance + 0.10f);

        MoveDecision move;

        if (state.context.hasTarget)
        {
            // Use A* pathfinder on local perception map
            PathfinderConfig pathCfg;
            pathCfg.riskTolerance = state.context.riskTolerance;

            PathfinderResult pathResult = Pathfinder::GetNextMove(
                ctx.agent.position,
                state.context.targetPosition,
                state.context.perceptionMap,
                state.context.cachedPath,
                state.context.pathIndex,
                state.context.pathAge,
                pathCfg);

            if (pathResult.unreachable)
            {
                // Target unreachable — signal system layer to mark memory
                state.context.markTargetUnreachable = true;
                state.context.hasTarget = false;
                state.context.pathfinderActive = false;
            }
            else if (pathResult.targetReached)
            {
                move.dx = 0;
                move.dy = 0;
                move.foundCandidate = true;
                move.attemptedMove = false;
            }
            else if (pathResult.success)
            {
                move.dx = pathResult.dx;
                move.dy = pathResult.dy;
                move.foundCandidate = true;
                move.attemptedMove = true;
            }
            else
            {
                move.dx = 0;
                move.dy = 0;
                move.foundCandidate = true;
                move.attemptedMove = false;
            }
        }

        // If no target (or target became unreachable), fall back to forage
        if (!state.context.hasTarget)
        {
            AgentFeedback tempFeedback;
            tempFeedback.stuckTicks = state.context.stuckTicks;
            tempFeedback.lastMoveDx = state.context.lastMoveDx;
            tempFeedback.lastMoveDy = state.context.lastMoveDy;

            AgentIntent intent;
            intent.type = AgentIntentType::Forage;
            intent.riskTolerance = state.context.riskTolerance;

            move = NavigationPolicy::ChooseForageMove(
                ctx.fields, ctx.cognitive, ctx.envCtx, ctx.agent, intent, tempFeedback,
                static_cast<f32>(ctx.currentTick % 7));
        }

        // Execute move
        bool moveExecuted = false;
        if (move.foundCandidate)
        {
            i32 nx = ctx.agent.position.x + move.dx;
            i32 ny = ctx.agent.position.y + move.dy;
            if (ctx.fields.InBounds(ctx.envCtx.temperature, nx, ny))
            {
                CellRisk targetRisk = CellRiskEvaluator::Evaluate(
                    ctx.fields, ctx.cognitive, ctx.envCtx, ctx.agent.id, nx, ny);
                if (targetRisk.traversal != TraversalClass::Blocked)
                {
                    ctx.commands.Submit(ctx.currentTick, MoveAgentCommand{ctx.agent.id, nx, ny});
                    moveExecuted = true;
                }
            }
        }

        // Update stuck tracking
        if (move.attemptedMove && !moveExecuted)
            state.context.stuckTicks++;
        else if (moveExecuted)
        {
            state.context.stuckTicks = 0;
            state.context.lastMoveDx = move.dx;
            state.context.lastMoveDy = move.dy;
        }

        // Update urgency
        CellRisk currentRisk = CellRiskEvaluator::Evaluate(
            ctx.fields, ctx.cognitive, ctx.envCtx,
            ctx.agent.id, ctx.agent.position.x, ctx.agent.position.y);
        AgentDrives drives = DriveEvaluator::Evaluate(ctx.agent, currentRisk);
        state.context.urgency = drives.hungerUrgency;
    }

    bool IsComplete(const AgentState& state, const StateExecuteContext& ctx) const override
    {
        return ctx.agent.hunger < 20.0f;
    }

private:
    // Scan vision radius and update LocalPerceptionMap
    static void ScanVisibleArea(
        StateContext& sctx,
        Vec2i center,
        const FieldModule& fm,
        const CognitiveModule& cog,
        const HumanEvolution::EnvironmentContext& envCtx,
        EntityId agentId)
    {
        auto& map = sctx.perceptionMap;
        i32 radius = sctx.searchRadius; // vision radius = 5
        i32 radiusSq = radius * radius;

        for (i32 y = center.y - radius; y <= center.y + radius; ++y)
        {
            for (i32 x = center.x - radius; x <= center.x + radius; ++x)
            {
                Vec2i pos{x, y};
                if (!map.InBounds(pos)) continue;
                i32 dx = x - center.x;
                i32 dy = y - center.y;
                if (dx * dx + dy * dy > radiusSq) continue;

                if (!fm.InBounds(envCtx.temperature, x, y))
                {
                    map.UpdateCell(pos, CellKnowledge::Blocked, 1.0f);
                    continue;
                }

                CellRisk risk = CellRiskEvaluator::Evaluate(
                    fm, cog, envCtx, agentId, x, y);

                if (risk.traversal == TraversalClass::Blocked)
                    map.UpdateCell(pos, CellKnowledge::Blocked, risk.total);
                else
                    map.UpdateCell(pos, CellKnowledge::Walkable, risk.total);
            }
        }
    }
};
