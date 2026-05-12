#pragma once

#include "sim/world/module_registry.h"
#include "core/time/sim_clock.h"
#include "core/random/random.h"

struct SimulationModule : public IModule
{
    SimClock clock;
    Random random;

    explicit SimulationModule(u64 seed) : random(seed) {}
};
