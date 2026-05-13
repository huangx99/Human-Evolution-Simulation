#pragma once

#include <cstdint>

enum class SimPhase : uint8_t
{
    BeginTick,          // Prepare for new tick

    Input,              // Process external input
    CommandPrepare,     // Prepare commands from input

    Environment,        // Climate, weather
    Reaction,           // Chemical/biological reactions
    Propagation,        // Fire spread, smell diffusion, disease spread

    Perception,         // Agent sensing
    Decision,           // Agent decision making
    Action,             // Agent actions (submit commands)

    CommandApply,       // Apply all pending commands
    EventResolve,       // Process and archive events
    FieldSwap,          // Swap field double buffers
    Analysis,           // Pattern detection (observes post-swap stable state)
    Snapshot,           // Capture world state, compute hash

    EndTick,            // Finalize tick, clock step

    Count
};
