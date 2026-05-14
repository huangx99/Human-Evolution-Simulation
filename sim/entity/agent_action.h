#pragma once

#include "core/types/types.h"

enum class AgentAction : u8
{
    Idle,
    MoveToFood,
    Flee,
    Wander,
    SeekFood,   // Forage / ApproachKnownFood (display alias)
    Rest
};

enum class AgentIntentType : u8
{
    None,
    Escape,
    Forage,
    ApproachKnownFood,
    Investigate,
    Explore,
    Rest
};
