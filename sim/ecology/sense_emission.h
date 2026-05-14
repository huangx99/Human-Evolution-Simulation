#pragma once

#include "core/types/types.h"

// SenseEmission: objective sensory signals emitted by an entity.
//
// ARCHITECTURE NOTE: SenseEmission is OBJECTIVE — the same entity emits
// the same signals to all observers. How an agent REACTS to these signals
// depends on their SensoryProfile (subjective, per-species).
//
// Each channel has fine-grained sub-types. Zero = no signal in that sub-type.

struct SmellEmission
{
    f32 appetizing = 0.0f;  // good smell (fruit, cooked meat)
    f32 repulsive = 0.0f;  // bad smell (rotting flesh, sulfur)
    f32 pungent = 0.0f;    // sharp smell (smoke, ammonia)
    f32 neutral = 0.0f;    // background smell (wet earth, grass)
};

struct VisionEmission
{
    f32 danger = 0.0f;     // danger signal (fire, predator eyes)
    f32 attract = 0.0f;    // attract signal (bright fruit, water shimmer)
    f32 novelty = 0.0f;    // novelty signal (unfamiliar object)
};

struct AudioEmission
{
    f32 alarm = 0.0f;      // alarm (crackling fire, animal cry)
    f32 comfort = 0.0f;    // comfort (rustling leaves, water)
    f32 social = 0.0f;     // social (other agent sounds)
};

struct TouchEmission
{
    f32 heat = 0.0f;       // heat (fire proximity)
    f32 cold = 0.0f;       // cold (ice, cold water)
    f32 pain = 0.0f;       // pain (sharp, thorns)
};

struct TasteEmission
{
    f32 sweet = 0.0f;
    f32 bitter = 0.0f;
    f32 umami = 0.0f;
};

struct ThermalEmission
{
    f32 heat = 0.0f;       // heat source (fire radiates)
    f32 cold = 0.0f;       // cold source
};

struct SenseEmission
{
    SmellEmission smell;
    VisionEmission vision;
    AudioEmission audio;
    TouchEmission touch;
    TasteEmission taste;
    ThermalEmission thermal;

    f32 TotalIntensity() const
    {
        f32 sum = 0.0f;
        sum += smell.appetizing + smell.repulsive + smell.pungent + smell.neutral;
        sum += vision.danger + vision.attract + vision.novelty;
        sum += audio.alarm + audio.comfort + audio.social;
        sum += touch.heat + touch.cold + touch.pain;
        sum += taste.sweet + taste.bitter + taste.umami;
        sum += thermal.heat + thermal.cold;
        return sum;
    }
};
