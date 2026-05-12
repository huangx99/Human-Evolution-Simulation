#pragma once

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include <string>

enum class AgentAction : u8
{
    Idle,
    MoveToFood,
    Flee,
    Wander
};

struct Agent
{
    EntityId id = 0;
    Vec2i position;
    f32 hunger = 50.0f;      // 0=full, 100=starving
    f32 health = 100.0f;     // 0=dead, 100=full
    AgentAction currentAction = AgentAction::Idle;

    // Perception results (set by AgentSystem)
    f32 nearestSmell = 0.0f;
    f32 nearestFire = 0.0f;
    f32 localTemperature = 20.0f;
};
