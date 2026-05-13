#pragma once

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/cognitive/concept_tag.h"
#include "sim/social/social_signal_id.h"

// SocialSignal: externalized information transmitted between entities.
//
// Not all signals are intentional. A scent trail is passive.
// A vocal alert is active. Path formation is emergent.
//
// LAW 10: SocialSignal is observable, not imperative.
// It must not issue commands or directly create MemoryRecord.
// It must flow through: SocialSignal → PerceivedStimulus → FocusedStimulus → MemoryRecord.
//
// If read by perception/cognition, it is simulation-affecting state
// and must remain deterministic under replay.

struct SocialSignal
{
    u64 id = 0;
    SocialSignalTypeId typeId;

    EntityId sourceEntityId = 0;    // who/what created (agent, fire, corpse, etc.)
    EntityId targetEntityId = 0;    // 0 = broadcast (anyone nearby)

    ConceptTag concept = ConceptTag::None;  // what the signal is about

    Vec2i origin;                       // where the signal came from
    f32 intensity = 0.0f;               // signal strength (decays with distance)
    f32 confidence = 0.0f;              // how trustworthy (based on source)
    f32 effectiveRadius = 5.0f;         // how far it reaches

    Tick createdTick = 0;
    u32 ttl = 10;                       // time-to-live in ticks
};
