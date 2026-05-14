#pragma once

#include "sim/event/event.h"

inline const char* EventTypeName(EventType type)
{
    switch (type)
    {
    case EventType::None:                       return "None";
    case EventType::FireStarted:                return "FireStarted";
    case EventType::FireExtinguished:           return "FireExtinguished";
    case EventType::WindChanged:                return "WindChanged";
    case EventType::AgentSpawned:               return "AgentSpawned";
    case EventType::AgentDied:                  return "AgentDied";
    case EventType::AgentMoved:                 return "AgentMoved";
    case EventType::AgentAte:                   return "AgentAte";
    case EventType::AgentFled:                  return "AgentFled";
    case EventType::SmellEmitted:               return "SmellEmitted";
    case EventType::DangerDetected:             return "DangerDetected";
    case EventType::ReactionTriggered:          return "ReactionTriggered";
    case EventType::EcologyChanged:             return "EcologyChanged";
    case EventType::Discovery:                  return "Discovery";
    case EventType::Cooperation:                return "Cooperation";
    case EventType::Migration:                  return "Migration";
    case EventType::CognitiveStimulusPerceived: return "CognitiveStimulusPerceived";
    case EventType::CognitiveAttentionFocused:  return "CognitiveAttentionFocused";
    case EventType::CognitiveMemoryFormed:      return "CognitiveMemoryFormed";
    case EventType::CognitiveHypothesisFormed:  return "CognitiveHypothesisFormed";
    case EventType::CognitiveKnowledgeUpdated:  return "CognitiveKnowledgeUpdated";
    case EventType::SocialSignalEmitted:        return "SocialSignalEmitted";
    case EventType::SocialSignalReceived:       return "SocialSignalReceived";
    case EventType::CognitiveMemoryReinforced:  return "CognitiveMemoryReinforced";
    case EventType::CognitiveMemoryStabilized:  return "CognitiveMemoryStabilized";
    case EventType::Count:                      return "Count";
    }
    return "Unknown";
}
