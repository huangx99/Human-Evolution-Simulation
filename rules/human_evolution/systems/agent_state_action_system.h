#pragma once

// agent_state_action_system.h: Action-phase system using state stack.
//
// Replaces AgentActionSystem. Executes the top-of-stack state behavior,
// then handles system-level concerns: eating, heat damage, visibility,
// food memory invalidation.
//
// The state behavior handles movement. The system layer handles:
//   - Eating (checks WorldState for consumable food at agent position)
//   - Heat damage
//   - Hunger increase per tick
//   - MarkVisibleExplored
//   - InvalidateVisibleMissingFoodMemories
//
// PHASE: SimPhase::Action

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "rules/human_evolution/runtime/agent_state.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/food_assessment.h"
#include "sim/cognitive/concept_registry.h"
#include "sim/ecology/behavior_table.h"
#include <unordered_map>
#include <unordered_set>

class AgentStateActionSystem : public ISystem
{
public:
    explicit AgentStateActionSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : envCtx_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& fm = ctx.GetFieldModule();
        auto& cog = ctx.Cognitive();
        auto& commands = ctx.Commands();
        auto tick = ctx.CurrentTick();

        for (auto& agent : ctx.Agents().agents)
        {
            if (!agent.alive) continue;

            // Per-tick housekeeping (always runs, regardless of state)
            commands.Submit(tick, ModifyHungerCommand{agent.id, 0.15f});
            MarkVisibleExplored(ctx.World(), agent);
            InvalidateVisibleMissingFoodMemories(ctx.World(), cog, agent);

            StateManager* sm = ctx.Agents().FindStateManager(agent.id);
            if (!sm) continue;

            StateExecuteContext execCtx{
                agent, fm, cog, envCtx_, commands, tick, ctx.World()
            };

            // Execute current state behavior (handles movement)
            sm->Act(execCtx);

            // Handle memory modification flags set by state behaviors
            HandleMemoryFlags(agent, sm, cog, tick);

            // System-level: eating check (WorldState access here, not in state)
            if (HasConsumableFoodAt(ctx.World(), agent, agent.position.x, agent.position.y))
            {
                commands.Submit(tick, FeedAgentCommand{agent.id, 5.0f});

                // Sync to state context
                AgentState* currentState = sm->GetCurrentState();
                if (currentState)
                    currentState->context.actionSucceeded = true;
            }
            else
            {
                AgentState* currentState = sm->GetCurrentState();
                if (currentState)
                    currentState->context.actionSucceeded = false;
            }

            // System-level: heat damage
            if (agent.localTemperature > 50.0f)
            {
                f32 damage = (agent.localTemperature - 50.0f) * 0.1f;
                commands.Submit(tick, DamageAgentCommand{agent.id, damage});
            }

            // Sync state context to agent.feedback for backward compatibility
            SyncFeedback(agent, sm);

            // Transient memory lifecycle: track state transitions
            HandleTransientMemoryTransition(agent, sm, cog);

            // Periodically reconsider unreachable memories
            if (tick % 50 == 0)
                cog.ReconsiderUnreachable(agent.id, tick, 50);
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
        static const char* const DEPS[] = {"AgentStateDecisionSystem"};
        return {"AgentStateActionSystem", SimPhase::Action, READS, 2, WRITES, 1, DEPS, 1, true, false};
    }

private:
    HumanEvolution::EnvironmentContext envCtx_;
    std::unordered_map<EntityId, StateId> previousStateId_;

    // --- Helpers (migrated from AgentActionSystem) ---

    static bool HasConsumableFoodAt(const WorldState& world, const Agent& agent, i32 x, i32 y)
    {
        for (const auto& entity : world.Ecology().entities.All())
        {
            if (entity.x != x || entity.y != y) continue;
            if (FoodAssessor::IsConsumable(entity, agent))
                return true;
        }
        return false;
    }

    struct ExploreStateData
    {
        std::unordered_set<i32> explored;
    };
    std::unordered_map<EntityId, ExploreStateData> exploration_;

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

    // Sync state context to agent.feedback for backward compatibility
    static void SyncFeedback(Agent& agent, StateManager* sm)
    {
        AgentState* currentState = sm->GetCurrentState();
        if (!currentState)
        {
            agent.feedback = AgentFeedback{};
            return;
        }

        auto& fb = agent.feedback;
        auto& ctx = currentState->context;
        fb.lastMoveDx = ctx.lastMoveDx;
        fb.lastMoveDy = ctx.lastMoveDy;
        fb.stuckTicks = ctx.stuckTicks;
        fb.actionSucceeded = ctx.actionSucceeded;
        fb.lastPosition = agent.position;
    }

    void HandleMemoryFlags(const Agent& agent, StateManager* sm,
                           CognitiveModule& cog, Tick tick)
    {
        AgentState* currentState = sm->GetCurrentState();
        if (!currentState) return;

        auto& sctx = currentState->context;
        const auto& reg = ConceptTypeRegistry::Instance();

        // Handle failed approach count
        if (sctx.incrementFailedApproach > 0)
        {
            for (auto& mem : cog.GetAgentMemories(agent.id))
            {
                if (mem.location == sctx.targetPosition &&
                    reg.GetName(mem.subject) == "food")
                {
                    mem.failedApproachCount += sctx.incrementFailedApproach;
                    if (mem.failedApproachCount >= 3)
                    {
                        mem.markedUnreachable = true;
                        mem.unreachableSince = tick;
                        mem.annotation = "failed_approach";
                    }
                    break;
                }
            }
            sctx.incrementFailedApproach = 0;
        }

        // Handle unreachable marking (from pathfinder)
        if (sctx.markTargetUnreachable)
        {
            for (auto& mem : cog.GetAgentMemories(agent.id))
            {
                if (mem.location == sctx.targetPosition &&
                    reg.GetName(mem.subject) == "food")
                {
                    mem.markedUnreachable = true;
                    mem.unreachableSince = tick;
                    mem.annotation = "unreachable_path";
                    break;
                }
            }
            sctx.markTargetUnreachable = false;
        }
    }

    void HandleTransientMemoryTransition(const Agent& agent, StateManager* sm,
                                         CognitiveModule& cog)
    {
        AgentState* currentState = sm->GetCurrentState();
        StateId currentId = currentState ? currentState->id : StateId::None;
        StateId previousId = StateId::None;
        auto it = previousStateId_.find(agent.id);
        if (it != previousStateId_.end()) previousId = it->second;

        const auto& reg = ConceptTypeRegistry::Instance();

        if (previousId == StateId::Hunger && currentId != StateId::Hunger)
        {
            // Left hunger — make food memories transient (slower decay)
            for (auto& mem : cog.GetAgentMemories(agent.id))
            {
                if (reg.GetName(mem.subject) == "food" && mem.sense == SenseType::Vision)
                    mem.isTransient = true;
            }
        }
        else if (currentId == StateId::Hunger && previousId != StateId::Hunger)
        {
            // Entered hunger — food memories are actively pursued, normal decay
            for (auto& mem : cog.GetAgentMemories(agent.id))
            {
                if (reg.GetName(mem.subject) == "food" && mem.sense == SenseType::Vision)
                    mem.isTransient = false;
            }
        }

        previousStateId_[agent.id] = currentId;
    }

    static constexpr i32 visualExplorationRadius = 4;
    static constexpr f32 missingFoodMemoryStrengthScale = 0.2f;
    static constexpr f32 missingFoodMemoryConfidenceScale = 0.5f;
};
