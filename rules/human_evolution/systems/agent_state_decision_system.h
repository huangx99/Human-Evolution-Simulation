#pragma once

// agent_state_decision_system.h: Decision-phase system using state stack.
//
// Replaces AgentDecisionSystem. Evaluates triggers, manages state stack,
// writes agent.currentAction / agent.currentIntent for display.
//
// Does NOT access WorldState — uses only cognitive pipeline (fields + memory).
//
// PHASE: SimPhase::Decision

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "rules/human_evolution/runtime/agent_state.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/agent_drives.h"

class AgentStateDecisionSystem : public ISystem
{
public:
    explicit AgentStateDecisionSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : envCtx_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& fm = ctx.GetFieldModule();
        auto& cog = ctx.Cognitive();
        auto tick = ctx.CurrentTick();

        for (auto& agent : ctx.Agents().agents)
        {
            if (!agent.alive) continue;

            StateManager* sm = ctx.Agents().FindStateManager(agent.id);
            if (!sm) continue;

            StateDecideContext decideCtx{
                agent, fm, cog, envCtx_, tick
            };

            sm->Decide(decideCtx);
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
        return {"AgentStateDecisionSystem", SimPhase::Decision, READS, 4, WRITES, 1, DEPS, 0, true, false};
    }

private:
    HumanEvolution::EnvironmentContext envCtx_;
};
