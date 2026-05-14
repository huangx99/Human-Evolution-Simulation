#pragma once

// hunger_state.h: Seek food when hungry. Priority 3.
//
// Trigger: hungerUrgency > 0.35 (via DriveEvaluator)
// Behavior: ApproachKnownFood (if memory) or Forage (smell gradient)
//   Uses cognitive pipeline: CognitiveModule::GetAgentMemories for food targets
//   Does NOT directly query WorldState for food positions
// Eating: handled by AgentStateActionSystem (system layer, not this state)
// Complete: agent.hunger < 20 (ate enough via system-layer FeedAgentCommand)
// Stuck: after 5 ticks, clear target and switch to pure Forage

#include "rules/human_evolution/runtime/agent_state.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/agent_drives.h"
#include "rules/human_evolution/runtime/navigation_policy.h"
#include "rules/human_evolution/runtime/agent_intent.h"
#include "sim/entity/agent.h"
#include "sim/field/field_module.h"
#include "sim/cognitive/cognitive_module.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/command/command_buffer.h"

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
        // Check memory for known food (cognitive pipeline)
        auto knownFood = IntentSelector::FindBestKnownFood(ctx.agent, ctx.cognitive);
        if (knownFood.has_value())
        {
            state.context.hasTarget = true;
            state.context.targetPosition = knownFood->position;
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
        // Re-check food memory on resume (may have changed while paused)
        auto knownFood = IntentSelector::FindBestKnownFood(ctx.agent, ctx.cognitive);
        if (knownFood.has_value())
        {
            state.context.hasTarget = true;
            state.context.targetPosition = knownFood->position;
        }
    }

    void OnPause(AgentState&) override {}

    void Execute(AgentState& state, const StateExecuteContext& ctx) override
    {
        // Stuck handling: after 5 ticks, clear target and switch to pure forage
        if (state.context.stuckTicks >= 5 && state.context.hasTarget)
        {
            state.context.hasTarget = false;
            state.context.stuckTicks = 0;
        }

        // Stuck handling: escalate risk tolerance
        if (state.context.stuckTicks >= 3)
            state.context.riskTolerance = std::min(1.0f, state.context.riskTolerance + 0.10f);

        // Build temporary feedback for NavigationPolicy compatibility
        AgentFeedback tempFeedback;
        tempFeedback.stuckTicks = state.context.stuckTicks;
        tempFeedback.lastMoveDx = state.context.lastMoveDx;
        tempFeedback.lastMoveDy = state.context.lastMoveDy;

        MoveDecision move;

        if (state.context.hasTarget)
        {
            // Approach known food target
            AgentIntent intent;
            intent.type = AgentIntentType::ApproachKnownFood;
            intent.hasTarget = true;
            intent.targetPosition = state.context.targetPosition;
            intent.riskTolerance = state.context.riskTolerance;

            move = NavigationPolicy::ChooseApproachTargetMove(
                ctx.fields, ctx.cognitive, ctx.envCtx, ctx.agent, intent, tempFeedback);
        }
        else
        {
            // Forage: search using smell gradient + memory
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
        // Complete when hunger is satisfied (system layer feeds the agent)
        return ctx.agent.hunger < 20.0f;
    }
};
