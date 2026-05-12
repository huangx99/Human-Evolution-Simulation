#pragma once

#include "sim/cognitive/perceived_stimulus.h"

// FocusedStimulus: a stimulus that won the attention competition.
//
// ARCHITECTURE NOTE: Attention is the bottleneck that prevents agents
// from becoming omniscient. Not all PerceivedStimulus become FocusedStimulus.
// CognitiveAttentionSystem selects the top N stimuli based on:
//   strength × urgency × novelty × (1 + fearBias + hungerBias + knowledgeBias)
//
// This means: an agent with fire trauma will attend to fire more than one
// without. An agent who knows "smoke signals fire" will attend to smoke
// more than one who doesn't. Past experience shapes what enters memory.
//
// Only FocusedStimulus can become MemoryRecord. This is a hard rule.

struct FocusedStimulus
{
    PerceivedStimulus stimulus;

    f32 attentionScore = 0.0f;  // final attention weight (higher = more memorable)
};
