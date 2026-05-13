#pragma once

// ObservedAction: lightweight struct for agent-to-agent behavior observation.
//
// Used by rule-pack systems (e.g. ImitationObservationSystem) to represent
// "observer saw actor do X". NOT persisted, NOT hashed — only a transient
// input to PerceivedStimulus generation.
//
// The action type is identified by ObservedActionTypeId, which is registered
// by the RulePack via IRulePack::RegisterObservedActions(). The engine layer
// does NOT contain any domain-specific action kinds (no Flee, GatherNearFire, etc.).

#include "core/types/types.h"
#include "sim/social/observed_action_id.h"

struct ObservedAction
{
    EntityId observerEntityId = 0;
    EntityId actorEntityId = 0;

    ObservedActionTypeId typeId;

    Tick observedTick = 0;
    Vec2i origin;

    f32 visibility = 0.0f;
    f32 distance = 0.0f;
    f32 confidence = 0.0f;
};
