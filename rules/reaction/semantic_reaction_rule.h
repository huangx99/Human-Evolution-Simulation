#pragma once

#include "rules/reaction/semantic_predicate.h"
#include "rules/reaction/reaction_effect.h"
#include <vector>
#include <string>

// Controls how entity-level predicates are bound to effect targets.
enum class ReactionTargetMode : u8
{
    SameEntity,  // all entity predicates must be satisfied by ONE entity; effects apply to it only
    SameCell,    // predicates can be satisfied by different entities in the cell; effects apply to all
};

struct SemanticReactionRule
{
    std::string id;
    std::string name;

    // All predicates must be true for the rule to fire
    std::vector<SemanticPredicate> conditions;

    // Effects applied when the rule fires
    std::vector<ReactionEffect> effects;

    // How entity-level predicates bind to effect targets
    ReactionTargetMode targetMode = ReactionTargetMode::SameEntity;

    f32 probability = 1.0f;
    bool repeatable = false;  // can fire every tick
};
