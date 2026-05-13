#pragma once

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/cognitive/concept_id.h"
#include <vector>

enum class MemoryKind : u8
{
    ShortTerm,      // recent, high detail, fast decay (seconds~minutes)
    Episodic,       // event + result, medium decay
    Pattern,        // repeated correlation, slow decay
    Social,         // learned from others
    Trauma          // emotionally charged, very slow decay
};

// MemoryRecord: a single memory of an experience.
//
// ARCHITECTURE NOTE: MemoryRecord is an ABSTRACT experience, not a world snapshot.
// It records what the agent subjectively experienced (subject, context, result),
// not what objectively happened. Two agents at the same event will form different
// memories based on what they attended to.
//
// Memories are subject to:
//   - Decay: strength decreases each tick (ShortTerm: 2%/tick, Episodic: 0.1%/tick)
//   - Forgetting: memories with strength < 0.01 are removed
//   - Promotion: strong ShortTerm memories become Episodic (slower decay)
//   - Emotional weight: traumatic memories decay slower, pleasant ones stick
//   - Reinforcement: re-observing the same concept resets the decay clock
//
// Uses concept tags instead of fixed categories.
// A memory of "ate red berry and got sick" would be:
//   subject: Fruit, contextTags: {Danger}, resultTags: {Pain, Health}
//
// RULE: MemorySystem only consumes FocusedStimulus, never raw WorldState.

struct MemoryRecord
{
    u64 id = 0;

    EntityId ownerId = 0;

    MemoryKind kind = MemoryKind::ShortTerm;

    // What was experienced
    ConceptTypeId subject;                      // primary concept
    std::vector<ConceptTypeId> contextTags;     // surrounding context
    std::vector<ConceptTypeId> resultTags;      // what happened as result

    Vec2i location;

    f32 strength = 0.0f;           // how strong this memory is (decays over time)
    f32 emotionalWeight = 0.0f;    // -1 (traumatic) to +1 (pleasant)
    f32 confidence = 0.0f;         // how certain the agent is about this memory

    Tick createdTick = 0;
    Tick lastReinforcedTick = 0;

    u64 sourceStimulusId = 0;      // which stimulus created this memory

    void Decay(f32 rate)
    {
        strength *= rate;
    }

    bool IsFaded() const
    {
        return strength < 0.01f;
    }
};
