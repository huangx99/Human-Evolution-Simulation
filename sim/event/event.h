#pragma once

#include "core/types/types.h"
#include <cstdint>

enum class EventType : u16
{
    None = 0,

    // Environment events
    FireStarted,
    FireExtinguished,
    WindChanged,

    // Agent events
    AgentSpawned,
    AgentDied,
    AgentMoved,
    AgentAte,
    AgentFled,

    // Information events
    SmellEmitted,
    DangerDetected,

    // Ecology events
    ReactionTriggered,
    EcologyChanged,

    // Civilization events (future)
    Discovery,
    Cooperation,
    Migration,

    // Cognitive events (phase 2)
    CognitiveStimulusPerceived,
    CognitiveAttentionFocused,
    CognitiveMemoryFormed,
    CognitiveHypothesisFormed,
    CognitiveKnowledgeUpdated,
    SocialSignalEmitted,
    SocialSignalReceived,

    Count
};

struct Event
{
    EventType type = EventType::None;
    Tick tick = 0;
    EntityId entityId = 0;
    i32 x = 0;
    i32 y = 0;
    f32 value = 0.0f;
};
