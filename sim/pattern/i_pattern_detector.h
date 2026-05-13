#pragma once

#include "sim/pattern/pattern_observation.h"
#include "sim/pattern/pattern_module.h"
#include "sim/pattern/pattern_config.h"

class FieldModule;

// IPatternDetector: abstract base for pattern detectors.
//
// Each detector observes a stream of PatternObservations and emits
// PatternRecords when sufficient evidence accumulates.
//
// Detectors are stateful — they maintain internal counters / windows.
// EndWindow() is called at the end of each observation window.
//
// Detectors that need direct field access (like StableFieldZoneDetector)
// override ScanWorld() to read fields directly.

class IPatternDetector
{
public:
    virtual ~IPatternDetector() = default;

    // Feed a single observation. Called each tick for each relevant observation.
    virtual void Observe(const PatternObservation& obs) = 0;

    // Optional: read world state directly (for field-watching detectors).
    // Default is no-op. Called before Observe each tick.
    virtual void ScanWorld(Tick /*tick*/, const FieldModule& /*fm*/, i32 /*w*/, i32 /*h*/) {}

    // Finalize the current window. Emit detected patterns to the module.
    // Called every observationWindowTicks.
    virtual void EndWindow(Tick currentTick, PatternModule& patterns, const PatternRuntimeConfig& config) = 0;

    // Reset internal state (for testing).
    virtual void Reset() = 0;
};
