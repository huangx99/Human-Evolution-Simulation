#pragma once

#include "core/types/types.h"

// KnowledgeRelation: the type of relationship between two concepts.
//
// Knowledge is NOT "discovered facts" — it's a web of associations
// that can be wrong, incomplete, or contradictory.
//
// Example: fire causes warmth (high confidence)
//          fire repels beast (medium confidence, could be wrong)
//          fire sacred_as ritual (low confidence, cultural)

enum class KnowledgeRelation : u8
{
    // Causal
    Causes,         // A leads to B
    Prevents,       // A stops B

    // Spatial / physical
    Attracts,       // A draws B closer
    Repels,         // A pushes B away
    Transforms,     // A changes into B
    Emits,          // A releases B
    Contains,       // A holds B

    // Functional
    Requires,       // A needs B to work
    Replaces,       // A can substitute for B
    Signals,        // A indicates presence of B

    // Evaluative (subjective, can differ per agent)
    FearedAs,       // A is feared because of B
    ValuedAs,       // A is valued because of B
    SacredAs,       // A is sacred because of B

    // Associative
    AssociatedWith, // A and B often co-occur
};
