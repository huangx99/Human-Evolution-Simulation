#pragma once

#include "core/types/types.h"

struct SimClock
{
    Tick currentTick = 0;
    f32 tickDeltaSeconds = 1.0f;

    void Step()
    {
        currentTick++;
    }
};
