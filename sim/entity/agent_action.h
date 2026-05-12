#pragma once

#include "core/types/types.h"

enum class AgentAction : u8
{
    Idle,
    MoveToFood,
    Flee,
    Wander
};
