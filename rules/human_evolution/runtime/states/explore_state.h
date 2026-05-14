#pragma once

// explore_state.h: Default idle behavior — low-risk novelty seeking. Priority 1.
//
// Trigger: always (lowest priority, always available as fallback)
// Behavior: NavigationPolicy::ChooseExploreMove with direction persistence
// Complete: never (re-enters itself; interrupted by higher priority states)
// Stuck: reset direction after 3 ticks

#include "rules/human_evolution/runtime/agent_state.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/navigation_policy.h"
#include "sim/entity/agent.h"
#include "sim/field/field_module.h"
#include "sim/cognitive/cognitive_module.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/command/command_buffer.h"

class ExploreState : public IStateBehavior
{
public:
    StateId GetStateId() const override { return StateId::Explore; }
    const char* GetName() const override { return "Explore"; }
    i32 GetPriority(const Agent&) const override { return StatePriority::Explore; }
    AgentIntentType GetIntentType() const override { return AgentIntentType::Explore; }
    AgentAction GetDisplayAction(const AgentState&) const override { return AgentAction::Wander; }

    bool CanTrigger(const StateDecideContext&) const override
    {
        // Always available as the default idle behavior
        return true;
    }

    void OnEnter(AgentState& state, const StateDecideContext&) override
    {
        state.context.urgency = 0.2f;
        state.context.riskTolerance = 0.25f;
    }

    void OnResume(AgentState& state, const StateExecuteContext&) override
    {
        state.context.urgency = 0.2f;
    }

    void OnPause(AgentState&) override {}

    void Execute(AgentState& state, const StateExecuteContext& ctx) override
    {
        // Stuck handling: reset direction to allow free exploration
        if (state.context.stuckTicks >= 3)
        {
            state.context.lastMoveDx = 0;
            state.context.lastMoveDy = 0;
        }

        // Build temporary feedback for NavigationPolicy compatibility
        AgentFeedback tempFeedback;
        tempFeedback.stuckTicks = state.context.stuckTicks;
        tempFeedback.lastMoveDx = state.context.lastMoveDx;
        tempFeedback.lastMoveDy = state.context.lastMoveDy;

        AgentIntent intent;
        intent.type = AgentIntentType::Explore;
        intent.riskTolerance = state.context.riskTolerance;

        MoveDecision move = NavigationPolicy::ChooseExploreMove(
            ctx.fields, ctx.cognitive, ctx.envCtx, ctx.agent, intent, tempFeedback,
            static_cast<f32>(ctx.currentTick % 7));

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
    }

    bool IsComplete(const AgentState&, const StateExecuteContext&) const override
    {
        // Explore never completes — it's the default idle loop
        // Only interrupted by higher priority states
        return false;
    }
};
