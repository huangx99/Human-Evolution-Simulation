#pragma once

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/cognitive/concept_id.h"

enum class SenseType : u8
{
    Vision,
    Smell,
    Touch,
    Sound,
    Thermal,
    Internal,
    Social   // social cognitive input (GroupKnowledge, CulturalTrace)
};

// PerceivedStimulus: what an agent subjectively perceives.
//
// ARCHITECTURE NOTE: This is a SUBJECTIVE perception, not WorldState truth.
// The world may have temperature=80 at a location, but if the agent didn't
// notice it (below threshold, too far, distracted), no stimulus is created.
//
// CognitivePerceptionSystem is the ONLY system that reads WorldState to
// produce PerceivedStimulus. All downstream systems (Attention, Memory,
// Discovery) consume PerceivedStimulus or its derivatives, NOT raw world data.
// This prevents agents from having direct access to objective truth.
//
// Pipeline: WorldState → CognitivePerceptionSystem → PerceivedStimulus
//           → CognitiveAttentionSystem → FocusedStimulus
//           → CognitiveMemorySystem → MemoryRecord

struct PerceivedStimulus
{
    u64 id = 0;

    EntityId observerId = 0;        // who perceived this
    EntityId sourceEntityId = 0;    // what caused it (0 = environment)

    SenseType sense = SenseType::Vision;
    ConceptTypeId concept;  // what was perceived

    Vec2i location;

    f32 intensity = 0.0f;   // how strong the signal is (0..1+)
    f32 confidence = 0.0f;  // how certain the perception is (0..1)
    f32 distance = 0.0f;    // how far from observer

    Tick tick = 0;
};
