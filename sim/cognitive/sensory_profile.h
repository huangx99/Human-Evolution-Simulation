#pragma once

#include "core/types/types.h"
#include "sim/ecology/sense_emission.h"
#include <algorithm>
#include <cmath>

// SensoryProfile: per-species/agent bias coefficients.
//
// Determines how a specific agent SUBJECTIVELY reacts to objective SenseEmission.
// valence = Σ(emission.subType × profile.subTypeWeight)
// Positive valence = approach. Negative valence = avoid.
//
// Example:
//   Human: smell.repulsive weight = -0.8 → avoids rotting meat
//   Vulture: smell.repulsive weight = +0.6 → approaches rotting meat

struct SmellProfile
{
    f32 appetizingWeight = +0.8f;
    f32 repulsiveWeight = -0.8f;
    f32 pungentWeight = -0.5f;
    f32 neutralWeight = 0.0f;
};

struct VisionProfile
{
    f32 dangerWeight = -0.9f;
    f32 attractWeight = +0.7f;
    f32 noveltyWeight = +0.3f;
};

struct AudioProfile
{
    f32 alarmWeight = -0.6f;
    f32 comfortWeight = +0.4f;
    f32 socialWeight = +0.5f;
};

struct TouchProfile
{
    f32 heatWeight = -0.7f;
    f32 coldWeight = -0.3f;
    f32 painWeight = -1.0f;
};

struct TasteProfile
{
    f32 sweetWeight = +0.6f;
    f32 bitterWeight = -0.5f;
    f32 umamiWeight = +0.5f;
};

struct ThermalProfile
{
    f32 heatWeight = -0.4f;
    f32 coldWeight = -0.2f;
};

struct SensoryProfile
{
    SmellProfile smell;
    VisionProfile vision;
    AudioProfile audio;
    TouchProfile touch;
    TasteProfile taste;
    ThermalProfile thermal;

    f32 ComputeValence(const SenseEmission& emission) const
    {
        f32 v = 0.0f;
        v += emission.smell.appetizing * smell.appetizingWeight;
        v += emission.smell.repulsive * smell.repulsiveWeight;
        v += emission.smell.pungent * smell.pungentWeight;
        v += emission.smell.neutral * smell.neutralWeight;
        v += emission.vision.danger * vision.dangerWeight;
        v += emission.vision.attract * vision.attractWeight;
        v += emission.vision.novelty * vision.noveltyWeight;
        v += emission.audio.alarm * audio.alarmWeight;
        v += emission.audio.comfort * audio.comfortWeight;
        v += emission.audio.social * audio.socialWeight;
        v += emission.touch.heat * touch.heatWeight;
        v += emission.touch.cold * touch.coldWeight;
        v += emission.touch.pain * touch.painWeight;
        v += emission.taste.sweet * taste.sweetWeight;
        v += emission.taste.bitter * taste.bitterWeight;
        v += emission.taste.umami * taste.umamiWeight;
        v += emission.thermal.heat * thermal.heatWeight;
        v += emission.thermal.cold * thermal.coldWeight;
        return std::clamp(v, -1.0f, 1.0f);
    }
};

namespace SensoryProfiles
{
    inline SensoryProfile Human()
    {
        return {};
    }

    inline SensoryProfile Vulture()
    {
        SensoryProfile p;
        p.smell.appetizingWeight = +0.3f;
        p.smell.repulsiveWeight = +0.6f;
        p.smell.pungentWeight = -0.2f;
        p.vision.dangerWeight = -0.4f;
        p.vision.attractWeight = +0.5f;
        return p;
    }
}
