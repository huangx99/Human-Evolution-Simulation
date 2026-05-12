#pragma once

#include "rules/reaction/semantic_predicate.h"
#include "rules/reaction/reaction_effect.h"
#include <vector>
#include <string>

struct SemanticReactionRule
{
    std::string id;
    std::string name;

    // All predicates must be true for the rule to fire
    std::vector<SemanticPredicate> conditions;

    // Effects applied when the rule fires
    std::vector<ReactionEffect> effects;

    f32 probability = 1.0f;
    bool repeatable = false;  // can fire every tick
};
