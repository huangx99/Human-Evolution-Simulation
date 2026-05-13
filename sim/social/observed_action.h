#pragma once

// ObservedAction: lightweight struct for agent-to-agent behavior observation.
//
// Used by ImitationObservationSystem to represent "observer saw actor do X".
// NOT persisted, NOT hashed — only a transient input to PerceivedStimulus generation.
//
// Uses ObservedActionKind instead of AgentAction to decouple from
// agent state changes. Phase 2.1 only needs Flee.

#include "core/types/types.h"

enum class ObservedActionKind : u8
{
    Unknown = 0,
    Flee,
    GatherNearFire
};

struct ObservedAction
{
    EntityId observerEntityId = 0;
    EntityId actorEntityId = 0;

    ObservedActionKind kind = ObservedActionKind::Unknown;

    Tick observedTick = 0;
    Vec2i origin;

    f32 visibility = 0.0f;
    f32 distance = 0.0f;
    f32 confidence = 0.0f;
};
