#pragma once

// state_manager.h: Manages the state stack and queue for a single agent.
//
// One StateManager instance per agent. Behaviors are shared across agents
// via pointers (owned by StateBehaviorRegistry).
//
// Two-phase execution:
//   Decide() — Decision phase: evaluate triggers, manage stack, write display action
//   Act()    — Action phase: execute top state, handle completion

#include "rules/human_evolution/runtime/agent_state.h"
#include <unordered_map>
#include <algorithm>

class StateManager
{
public:
    StateManager() = default;

    // Register a behavior (non-owning pointer, must outlive this manager)
    void RegisterBehavior(IStateBehavior* behavior)
    {
        if (behavior)
            behaviors_[behavior->GetStateId()] = behavior;
    }

    // === Decision phase (StateDecideContext, no WorldState) ===

    void Decide(StateDecideContext& ctx)
    {
        // 1. Apply pending external pushes
        ApplyPendingPushes(ctx.currentTick);

        // 2. Evaluate triggers
        EvaluateTriggers(ctx);

        // 3. Call OnEnter for newly pushed top state
        if (!stack_.empty())
        {
            auto& top = stack_.back();
            if (top.enteredTick == ctx.currentTick && !top.isPaused)
            {
                IStateBehavior* b = GetBehavior(top.id);
                if (b) b->OnEnter(top, ctx);
            }
        }

        // 4. Write display action to agent
        if (!stack_.empty())
        {
            IStateBehavior* b = GetBehavior(stack_.back().id);
            if (b)
            {
                ctx.agent.currentAction = b->GetDisplayAction(stack_.back());
                ctx.agent.currentIntent = b->GetIntentType();
            }
        }
        else
        {
            ctx.agent.currentAction = AgentAction::Idle;
            ctx.agent.currentIntent = AgentIntentType::None;
        }
    }

    // === Action phase (StateExecuteContext, has WorldState for system layer) ===

    void Act(StateExecuteContext& ctx)
    {
        if (stack_.empty()) return;

        // 1. Execute current top state
        ExecuteCurrentState(ctx);

        // 2. Handle completion
        HandleCompletion(ctx);
    }

    // === External trigger ===

    void PushExternal(StateId id, StateContext context, Tick tick)
    {
        pendingPushes_.emplace_back(id, std::move(context));
        pendingPushesTick_ = tick;
    }

    // === Accessors ===

    AgentAction GetCurrentDisplayAction() const
    {
        if (stack_.empty()) return AgentAction::Idle;
        IStateBehavior* b = GetBehavior(stack_.back().id);
        return b ? b->GetDisplayAction(stack_.back()) : AgentAction::Idle;
    }

    AgentIntentType GetCurrentIntentType() const
    {
        if (stack_.empty()) return AgentIntentType::None;
        IStateBehavior* b = GetBehavior(stack_.back().id);
        return b ? b->GetIntentType() : AgentIntentType::None;
    }

    bool HasState(StateId id) const
    {
        return HasStateOnStack(id) || HasStateInQueue(id);
    }

    StateId GetCurrentStateId() const
    {
        return stack_.empty() ? StateId::None : stack_.back().id;
    }

    AgentState* GetCurrentState()
    {
        return stack_.empty() ? nullptr : &stack_.back();
    }

    const AgentState* GetCurrentState() const
    {
        return stack_.empty() ? nullptr : &stack_.back();
    }

    const std::vector<AgentState>& GetStack() const { return stack_; }

private:
    // All registered behaviors (non-owning pointers)
    std::unordered_map<StateId, IStateBehavior*> behaviors_;

    // Per-agent state stack (bottom = lowest priority, top = current)
    std::vector<AgentState> stack_;

    // Per-priority-level FIFO queue (index = priority level 0..5)
    std::deque<AgentState> queue_[6];

    // Pending external pushes
    std::vector<std::pair<StateId, StateContext>> pendingPushes_;
    Tick pendingPushesTick_ = 0;

    // --- Core operations ---

    void ApplyPendingPushes(Tick tick)
    {
        for (auto& [id, ctx] : pendingPushes_)
        {
            AgentState state;
            state.id = id;
            state.priority = GetExternalPriority(id);
            state.enteredTick = tick;
            state.context = std::move(ctx);
            PushState(std::move(state));
        }
        pendingPushes_.clear();
    }

    void EvaluateTriggers(const StateDecideContext& ctx)
    {
        // Phase 1: Collect all triggered states (deterministic: sort by StateId)
        std::vector<std::pair<StateId, IStateBehavior*>> sorted(
            behaviors_.begin(), behaviors_.end());
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        std::vector<AgentState> triggered;
        for (auto& [id, behavior] : sorted)
        {
            if (HasStateOnStack(id) || HasStateInQueue(id)) continue;
            if (!behavior->CanTrigger(ctx)) continue;

            AgentState state;
            state.id = id;
            state.priority = behavior->GetPriority(ctx.agent);
            state.enteredTick = ctx.currentTick;
            triggered.push_back(std::move(state));
        }

        // Phase 2: Sort by priority descending (higher priority first) for deterministic push order
        std::sort(triggered.begin(), triggered.end(),
                  [](const auto& a, const auto& b) { return a.priority > b.priority; });

        for (auto& state : triggered)
            PushState(std::move(state));
    }

    void ExecuteCurrentState(const StateExecuteContext& ctx)
    {
        if (stack_.empty()) return;

        auto& top = stack_.back();
        IStateBehavior* b = GetBehavior(top.id);
        if (!b) return;

        // Call OnResume if this state was just unpaused
        if (top.isPaused)
        {
            top.isPaused = false;
            top.pausedTick = 0;
            b->OnResume(top, ctx);
        }

        top.context.ticksInState++;
        b->Execute(top, ctx);
    }

    void HandleCompletion(const StateExecuteContext& ctx)
    {
        if (stack_.empty()) return;

        auto& top = stack_.back();
        IStateBehavior* b = GetBehavior(top.id);
        if (!b) return;

        if (!b->IsComplete(top, ctx)) return;

        i32 completedPriority = top.priority;
        stack_.pop_back();

        // Resume previous state if any
        if (!stack_.empty())
        {
            auto& prev = stack_.back();
            prev.isPaused = false;
            prev.pausedTick = 0;
            IStateBehavior* prevB = GetBehavior(prev.id);
            if (prevB) prevB->OnResume(prev, ctx);
        }

        // Check queue for same-priority state
        if (completedPriority >= 0 && completedPriority < 6)
        {
            auto& q = queue_[completedPriority];
            if (!q.empty())
            {
                AgentState next = std::move(q.front());
                q.pop_front();
                PushState(std::move(next));
            }
        }
    }

    void PushState(AgentState state)
    {
        StateId id = state.id;

        // No duplicates
        if (HasStateOnStack(id)) return;
        if (HasStateInQueue(id)) return;

        if (stack_.empty())
        {
            stack_.push_back(std::move(state));
            return;
        }

        auto& currentTop = stack_.back();

        if (state.priority > currentTop.priority)
        {
            // Higher priority: interrupt current top
            currentTop.isPaused = true;
            currentTop.pausedTick = state.enteredTick;
            IStateBehavior* topB = GetBehavior(currentTop.id);
            if (topB) topB->OnPause(currentTop);

            stack_.push_back(std::move(state));
        }
        else
        {
            // Same or lower priority: queue it
            i32 p = state.priority;
            if (p >= 0 && p < 6)
                queue_[p].push_back(std::move(state));
        }
    }

    void PopState()
    {
        if (stack_.empty()) return;
        stack_.pop_back();
    }

    bool HasStateOnStack(StateId id) const
    {
        for (const auto& s : stack_)
            if (s.id == id) return true;
        return false;
    }

    bool HasStateInQueue(StateId id) const
    {
        for (i32 p = 0; p < 6; p++)
            for (const auto& s : queue_[p])
                if (s.id == id) return true;
        return false;
    }

    IStateBehavior* GetBehavior(StateId id) const
    {
        auto it = behaviors_.find(id);
        return (it != behaviors_.end()) ? it->second : nullptr;
    }

    static i32 GetExternalPriority(StateId id)
    {
        switch (id)
        {
        case StateId::Fear:    return StatePriority::Fear;
        case StateId::Pain:    return StatePriority::Pain;
        case StateId::Hunger:  return StatePriority::Hunger;
        case StateId::Sleep:   return StatePriority::Sleep;
        case StateId::Craft:   return StatePriority::Craft;
        case StateId::Hunt:    return StatePriority::Hunt;
        case StateId::Explore: return StatePriority::Explore;
        case StateId::Social:  return StatePriority::Social;
        case StateId::Rest:    return 3; // same as Hunger/Sleep
        default:               return 0;
        }
    }
};
