#pragma once

// fear_state.h: Escape from danger. Highest priority (5).
//
// Trigger: CellRisk.total > 0.65 (uses cognitive pipeline: memory + fields)
// Behavior: NavigationPolicy::ChooseEscapeMove
// Complete: cachedRisk < 0.30 (danger subsided)
// Stuck: increase riskTolerance after 3 ticks

#include "rules/human_evolution/runtime/agent_state.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/navigation_policy.h"
#include "sim/entity/agent.h"
#include "sim/field/field_module.h"
#include "sim/cognitive/cognitive_module.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/command/command_buffer.h"

class FearState : public IStateBehavior
{
public:
    StateId GetStateId() const override { return StateId::Fear; }
    const char* GetName() const override { return "Fear"; }
    i32 GetPriority(const Agent&) const override { return StatePriority::Fear; }
    AgentIntentType GetIntentType() const override { return AgentIntentType::Escape; }
    AgentAction GetDisplayAction(const AgentState&) const override { return AgentAction::Flee; }

    bool CanTrigger(const StateDecideContext& ctx) const override
    {
        CellRisk risk = CellRiskEvaluator::Evaluate(
            ctx.fields, ctx.cognitive, ctx.envCtx,
            ctx.agent.id, ctx.agent.position.x, ctx.agent.position.y);
        return risk.total > 0.65f;
    }

    void OnEnter(AgentState& state, const StateDecideContext& ctx) override
    {
        CellRisk risk = CellRiskEvaluator::Evaluate(
            ctx.fields, ctx.cognitive, ctx.envCtx,
            ctx.agent.id, ctx.agent.position.x, ctx.agent.position.y);
        state.context.riskTolerance = 0.95f;
        state.context.urgency = risk.total;
        state.context.cachedRisk = risk.total;
    }

    void OnResume(AgentState& state, const StateExecuteContext& ctx) override
    {
        // Re-evaluate risk on resume
        CellRisk risk = CellRiskEvaluator::Evaluate(
            ctx.fields, ctx.cognitive, ctx.envCtx,
            ctx.agent.id, ctx.agent.position.x, ctx.agent.position.y);
        state.context.urgency = risk.total;
        state.context.cachedRisk = risk.total;
    }

    void OnPause(AgentState&) override {}

    void Execute(AgentState& state, const StateExecuteContext& ctx) override
    {
        // Stuck handling: escalate risk tolerance
        if (state.context.stuckTicks >= 3)
            state.context.riskTolerance = std::min(1.0f, state.context.riskTolerance + 0.15f);

        // Build temporary feedback for NavigationPolicy compatibility
        AgentFeedback tempFeedback;
        tempFeedback.stuckTicks = state.context.stuckTicks;
        tempFeedback.lastMoveDx = state.context.lastMoveDx;
        tempFeedback.lastMoveDy = state.context.lastMoveDy;

        AgentIntent intent;
        intent.type = AgentIntentType::Escape;
        intent.riskTolerance = state.context.riskTolerance;

        MoveDecision move = NavigationPolicy::ChooseEscapeMove(
            ctx.fields, ctx.cognitive, ctx.envCtx, ctx.agent, intent, tempFeedback);

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

        // Cache current risk for IsComplete
        CellRisk currentRisk = CellRiskEvaluator::Evaluate(
            ctx.fields, ctx.cognitive, ctx.envCtx,
            ctx.agent.id, ctx.agent.position.x, ctx.agent.position.y);
        state.context.cachedRisk = currentRisk.total;
    }

    bool IsComplete(const AgentState& state, const StateExecuteContext&) const override
    {
        return state.context.cachedRisk < 0.30f;
    }
};
