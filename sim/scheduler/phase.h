#pragma once

#include <cstdint>

enum class SimPhase : uint8_t
{
    Environment,    // Climate, weather
    Propagation,    // Fire spread, smell diffusion
    Perception,     // Agent sensing
    Decision,       // Agent decision making
    Action,         // Agent actions
    History,        // Recording, clock step

    Count
};
