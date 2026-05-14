#pragma once

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/entity/agent_action.h"
#include "sim/cognitive/sensory_profile.h"
#include <string>

enum class Species : u8 { Human = 0 };

struct AgentFeedback
{
    Vec2i lastPosition{};
    AgentIntentType lastIntent = AgentIntentType::None;
    bool attemptedMove = false;
    bool moveSucceeded = false;
    bool actionSucceeded = false;
    i32 stuckTicks = 0;
    i32 failedEscapeTicks = 0;
    i32 failedForageTicks = 0;

    // Direction persistence: remember last movement direction
    i32 lastMoveDx = 0;
    i32 lastMoveDy = 0;

    // Intent commitment: lock intent for N ticks to prevent oscillation
    i32 intentCommitTicks = 0;
};

struct Agent
{
    EntityId id = 0;
    Vec2i position;
    f32 hunger = 50.0f;      // 0=full, 100=starving
    f32 health = 100.0f;     // 0=dead, 100=full
    bool alive = true;       // false once health reaches 0
    AgentAction currentAction = AgentAction::Idle;

    // Phase 2.7: intent and feedback
    AgentIntentType currentIntent = AgentIntentType::None;
    AgentFeedback feedback;

    // Perception results (set by AgentSystem)
    f32 nearestSmell = 0.0f;
    f32 nearestFire = 0.0f;
    f32 localTemperature = 20.0f;

    // Species and sensory bias
    Species species = Species::Human;
    SensoryProfile sensoryProfile = SensoryProfiles::Human();
};
