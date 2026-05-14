#pragma once

// state_behavior_registry.h: Singleton registry for all state behaviors.
//
// Behaviors are registered once at RulePack init time.
// StateManagers reference behaviors by pointer (non-owning).

#include "rules/human_evolution/runtime/agent_state.h"
#include <unordered_map>
#include <memory>

class StateBehaviorRegistry
{
public:
    static StateBehaviorRegistry& Instance()
    {
        static StateBehaviorRegistry instance;
        return instance;
    }

    void Register(std::unique_ptr<IStateBehavior> behavior)
    {
        StateId id = behavior->GetStateId();
        behaviors_[id] = std::move(behavior);
    }

    IStateBehavior* Get(StateId id) const
    {
        auto it = behaviors_.find(id);
        return (it != behaviors_.end()) ? it->second.get() : nullptr;
    }

    // Register all behaviors to a StateManager
    void RegisterAllTo(StateManager& manager) const
    {
        for (const auto& [id, behavior] : behaviors_)
            manager.RegisterBehavior(behavior.get());
    }

    const auto& GetAll() const { return behaviors_; }

private:
    StateBehaviorRegistry() = default;
    std::unordered_map<StateId, std::unique_ptr<IStateBehavior>> behaviors_;
};
