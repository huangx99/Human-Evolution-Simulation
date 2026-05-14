#pragma once

// AgentDecisionSystem: runtime intent selection based on drives + risk + knowledge.
//
// Phase 2.7 refactor: replaces score-based action selection with
// DriveEvaluator → IntentSelector pipeline.
// Sets agent.currentIntent (primary) and agent.currentAction (display compat).
//
// PHASE: SimPhase::Decision

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/concept_registry.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/agent_drives.h"
#include "rules/human_evolution/runtime/agent_intent.h"

class AgentDecisionSystem : public ISystem
{
public:
    AgentDecisionSystem() = default;

    explicit AgentDecisionSystem(const HumanEvolution::ConceptContext& concepts)
        : concepts_(concepts) {}

    explicit AgentDecisionSystem(const HumanEvolution::EnvironmentContext& envCtx,
                                 const HumanEvolution::ConceptContext& concepts)
        : envCtx_(envCtx), concepts_(concepts) {}

    void Update(SystemContext& ctx) override
    {
        auto& fm = ctx.GetFieldModule();
        auto& cog = ctx.Cognitive();

        for (auto& agent : ctx.Agents().agents)
        {
            if (!agent.alive) continue;

            // Step 1: assess risk at current position
            CellRisk currentRisk = CellRiskEvaluator::Evaluate(
                fm, cog, envCtx_, agent.id, agent.position.x, agent.position.y);

            // Step 2: evaluate drives
            AgentDrives drives = DriveEvaluator::Evaluate(agent, currentRisk);

            // Step 3: select intent
            AgentIntent intent = IntentSelector::Select(agent, drives, cog);

            // Step 4: write to agent
            agent.currentIntent = intent.type;
            agent.currentAction = ToDisplayAction(intent.type);
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
    HumanEvolution::EnvironmentContext envCtx_{};
    HumanEvolution::ConceptContext concepts_{};

    static AgentAction ToDisplayAction(AgentIntentType intent)
    {
        switch (intent)
        {
        case AgentIntentType::Escape:
            return AgentAction::Flee;
        case AgentIntentType::ApproachKnownFood:
        case AgentIntentType::Forage:
            return AgentAction::SeekFood;
        case AgentIntentType::Explore:
        case AgentIntentType::Investigate:
            return AgentAction::Wander;
        case AgentIntentType::Rest:
            return AgentAction::Rest;
        default:
            return AgentAction::Wander;
        }
    }
};
