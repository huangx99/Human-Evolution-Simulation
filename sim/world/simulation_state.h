#pragma once

#include "core/types/types.h"
#include "core/time/sim_clock.h"
#include "core/random/random.h"

struct SimulationState
{
    SimClock clock;
    Random random;

    explicit SimulationState(u64 seed) : random(seed) {}
};
