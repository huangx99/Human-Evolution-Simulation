#pragma once

#include "core/types/types.h"
#include <cstdint>

enum class CommandType : u8
{
    None = 0,

    // Agent commands
    MoveAgent,
    SetAgentAction,
    DamageAgent,
    FeedAgent,

    // Environment commands
    IgniteFire,
    ExtinguishFire,
    EmitSmell,

    // Information commands
    SetDanger,

    Count
};

struct Command
{
    CommandType type = CommandType::None;
    Tick tick = 0;
    EntityId entityId = 0;
    i32 x = 0;
    i32 y = 0;
    i32 targetX = 0;
    i32 targetY = 0;
    f32 value = 0.0f;
};
