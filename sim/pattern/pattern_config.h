#pragma once

#include "core/types/types.h"

// PatternRuntimeConfig: tuning knobs for pattern detection.
//
// Keep conservative defaults. RulePacks can override via their context.

struct PatternRuntimeConfig
{
    u32 observationWindowTicks = 100;   // sliding window size
    u32 maxObservationsPerTick = 1024;  // cap per-tick observation count
    u32 maxPatterns = 4096;             // global pattern limit
    f32 minConfidence = 0.6f;           // minimum confidence to emit
};
