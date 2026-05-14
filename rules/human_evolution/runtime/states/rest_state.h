#pragma once

// rest_state.h: Stationary recovery. Priority 3.
//
// Trigger: fatigue > 0.70 (when fatigue system is implemented)
// Behavior: no movement, just rest
// Complete: after 20 ticks or fatigue < 0.30
//
// NOTE: fatigue field not yet on Agent. This state is a placeholder
// for when the fatigue system is added. Currently never triggers.

#include "rules/human_evolution/runtime/agent_state.h"
#include "sim/entity/agent.h"

class RestState : public IStateBehavior
{
public:
    StateId GetStateId() const override { return StateId::Rest; }
    const char* GetName() const override { return "Rest"; }
    i32 GetPriority(const Agent&) const override { return 3; } // same as Hunger/Sleep
    AgentIntentType GetIntentType() const override { return AgentIntentType::Rest; }
    AgentAction GetDisplayAction(const AgentState&) const override { return AgentAction::Rest; }

    bool CanTrigger(const StateDecideContext& ctx) const override
    {
        // Placeholder: trigger when fatigue is high
        // return ctx.agent.fatigue > 0.70f;
        return false; // disabled until fatigue system exists
    }

    void OnEnter(AgentState& state, const StateDecideContext&) override
    {
        state.context.urgency = 0.5f;
        state.context.riskTolerance = 0.20f;
    }

    void OnResume(AgentState&, const StateExecuteContext&) override {}
    void OnPause(AgentState&) override {}

    void Execute(AgentState& state, const StateExecuteContext&) override
    {
        // No movement — just rest
        state.context.ticksInState++;
    }

    bool IsComplete(const AgentState& state, const StateExecuteContext&) const override
    {
        // Rest for 20 ticks
        return state.context.ticksInState >= 20;
    }
};
