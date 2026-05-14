#pragma once

// agent_state.h: Core types for the priority-based state stack architecture.
//
// Replaces the flat intent system (AgentIntentType) with a LIFO state stack
// where higher-priority states interrupt lower ones, and same-priority states
// queue in FIFO order.
//
// Each state owns its behavior (navigation, eating, crafting, etc.) via
// IStateBehavior. States are injected per species/role through StateBehaviorRegistry.

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/entity/agent_action.h"
#include "rules/human_evolution/runtime/local_perception_map.h"
#include <vector>
#include <deque>
#include <memory>
#include <limits>

// Forward declarations
struct Agent;
class FieldModule;
class CognitiveModule;
class CommandBuffer;
class WorldState;

namespace HumanEvolution { struct EnvironmentContext; }

// --- StateId: extensible via u16 ---

enum class StateId : u16
{
    None = 0,
    Idle,
    Explore,
    Forage,
    ApproachKnownFood,
    Hunger,
    Rest,
    Fear,
    Pain,
    Sleep,
    Craft,
    Hunt,
    Social,
    // User-defined states start at 100
    UserDefined = 100,
};

// --- Priority constants ---

namespace StatePriority
{
    constexpr i32 Idle    = 0;
    constexpr i32 Explore = 1;
    constexpr i32 Social  = 1;
    constexpr i32 Craft   = 2;
    constexpr i32 Hunt    = 2;
    constexpr i32 Hunger  = 3;
    constexpr i32 Sleep   = 3;
    constexpr i32 Pain    = 4;
    constexpr i32 Fear    = 5;
}

// --- StateContext: per-state mutable data, lives on the stack element ---

struct StateContext
{
    Vec2i targetPosition{};     // navigation target (if any)
    bool hasTarget = false;
    f32 urgency = 0.0f;        // how urgent this state is (0..1)
    f32 riskTolerance = 0.3f;  // how much risk this state accepts
    i32 ticksInState = 0;      // how long this state has been active
    i32 stuckTicks = 0;        // consecutive failed moves
    i32 lastMoveDx = 0;        // direction persistence
    i32 lastMoveDy = 0;
    bool actionSucceeded = false; // did the last action succeed (e.g., ate food)
    i32 searchRadius = 5;      // for search-type states / vision radius
    f32 cachedRisk = 0.0f;     // Execute writes, IsComplete reads

    // Pathfinder state (A* on local perception map)
    LocalPerceptionMap perceptionMap;
    Vec2i pathTarget{};
    bool pathfinderActive = false;
    std::vector<Vec2i> cachedPath;
    i32 pathIndex = 0;
    i32 pathAge = 0;

    // Memory modification flags (system layer handles actual mutation)
    bool markTargetUnreachable = false;
    i32 incrementFailedApproach = 0;
};

// --- AgentState: the stack/queue element ---

struct AgentState
{
    StateId id = StateId::None;
    i32 priority = 0;
    StateContext context;
    Tick enteredTick = 0;       // when this state was pushed
    Tick pausedTick = 0;        // when this state was paused (0 = active)
    bool isPaused = false;      // true if a higher-priority state interrupted

    bool IsActive() const { return !isPaused; }
};

// --- Contexts: split by phase to enforce cognitive pipeline constraint ---

// Decision phase: no WorldState, agents must use cognitive pipeline
struct StateDecideContext
{
    Agent& agent;
    const FieldModule& fields;
    const CognitiveModule& cognitive;
    const HumanEvolution::EnvironmentContext& envCtx;
    Tick currentTick;
};

// Action phase: has WorldState for system-level checks only (eating, damage)
struct StateExecuteContext
{
    Agent& agent;
    const FieldModule& fields;
    const CognitiveModule& cognitive;
    const HumanEvolution::EnvironmentContext& envCtx;
    CommandBuffer& commands;
    Tick currentTick;
    const WorldState& world;  // system layer only, state behaviors must not access
};

// --- IStateBehavior: interface for state logic ---

class IStateBehavior
{
public:
    virtual ~IStateBehavior() = default;

    // Identity
    virtual StateId GetStateId() const = 0;
    virtual const char* GetName() const = 0;
    virtual i32 GetPriority(const Agent& agent) const = 0;

    // Display mapping (backward compatibility)
    virtual AgentAction GetDisplayAction(const AgentState& state) const = 0;
    virtual AgentIntentType GetIntentType() const = 0;

    // Decision phase: trigger evaluation and entry (no WorldState access)
    virtual bool CanTrigger(const StateDecideContext& ctx) const = 0;
    virtual void OnEnter(AgentState& state, const StateDecideContext& ctx) = 0;

    // Action phase: execute and lifecycle (no direct WorldState access)
    virtual void OnResume(AgentState& state, const StateExecuteContext& ctx) = 0;
    virtual void OnPause(AgentState& state) = 0;
    virtual void Execute(AgentState& state, const StateExecuteContext& ctx) = 0;
    virtual bool IsComplete(const AgentState& state, const StateExecuteContext& ctx) const = 0;
};
