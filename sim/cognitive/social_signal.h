#pragma once

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/cognitive/concept_tag.h"

enum class SocialSignalType : u8
{
    Imitation,      // observer copies what source did
    VocalAlert,     // sound-based warning
    ScentTrail,     // smell left behind
    BehaviorRepeat, // seen same behavior multiple times
    PathFormation   // worn path suggesting common route
};

// SocialSignal: information transmitted between agents.
//
// Not all signals are intentional. A scent trail is passive.
// A vocal alert is active. Path formation is emergent.
//
// Created by CognitiveSocialSystem when agents interact.
// Consumed by other agents' CognitivePerceptionSystem.

struct SocialSignal
{
    u64 id = 0;

    EntityId sourceAgentId = 0;     // who created the signal
    EntityId targetAgentId = 0;     // 0 = broadcast (anyone nearby)

    SocialSignalType type = SocialSignalType::Imitation;

    ConceptTag concept = ConceptTag::None;  // what the signal is about
    f32 intensity = 0.0f;                   // signal strength (decays with distance)
    f32 reliability = 0.0f;                 // how trustworthy (based on source)

    Vec2i origin;                           // where the signal came from
    f32 effectiveRadius = 5.0f;             // how far it reaches

    Tick tick = 0;
};
