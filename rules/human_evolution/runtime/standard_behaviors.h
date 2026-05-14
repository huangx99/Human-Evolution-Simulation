#pragma once

// StandardBehaviors: registers the default recipes for the Human Evolution world.
// Called from main.cpp and from tests that need recipe-based food/perception.
// Also provides RegisterLifecycleReactions for time-based state transitions.

#include "sim/ecology/behavior_table.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/entity/agent.h"
#include "rules/reaction/semantic_reaction_system.h"
#include "rules/reaction/semantic_predicate.h"
#include "rules/reaction/reaction_effect.h"

inline void RegisterStandardBehaviors(const HumanEvolution::ConceptContext& concepts)
{
    auto& behaviors = BehaviorTable::Instance();

    // DryGrass — not food, just grass
    behaviors.Register({
        .id = "dry_grass",
        .output = {MaterialId::DryGrass, MaterialState::None, MaterialState::None},
        .callbacks = {
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.grass);
            },
        }
    });

    // Grass — not food
    behaviors.Register({
        .id = "grass",
        .output = {MaterialId::Grass, MaterialState::Alive, MaterialState::None},
        .callbacks = {
            .emission = {.smell = {.neutral = 5.0f}},
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.grass);
            },
        }
    });

    // Fruit — fresh food
    behaviors.Register({
        .id = "fruit",
        .output = {MaterialId::Fruit, MaterialState::Alive, MaterialState::None},
        .callbacks = {
            .emission = {
                .smell = {.appetizing = 36.0f},
                .vision = {.attract = 20.0f},
            },
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.food);
            },
            .onEat = [](const Agent&) -> FoodEffect {
                return {FoodValidity::EdibleFresh, 1.0f, 0.05f, true};
            },
        }
    });

    // Fruit (no state specified) — default for Fruit without Alive state
    behaviors.Register({
        .id = "fruit_default",
        .output = {MaterialId::Fruit, MaterialState::None, MaterialState::None},
        .callbacks = {
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.food);
            },
            .onEat = [](const Agent&) -> FoodEffect {
                return {FoodValidity::EdibleFresh, 1.0f, 0.05f, true};
            },
        }
    });

    // Rotting corpse (Flesh + Dead + Decomposing) — not food, very repulsive
    // Registered BEFORE corpse so FindByOutput returns this for Decomposing entities.
    behaviors.Register({
        .id = "rotting_corpse",
        .output = {MaterialId::Flesh, MaterialState::Decomposing, MaterialState::None},
        .callbacks = {
            .emission = {
                .smell = {.repulsive = 60.0f},
            },
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.death);
            },
        }
    });

    // Cooked flesh (Flesh + Charred) — cooked meat, appetizing smell
    behaviors.Register({
        .id = "cooked_flesh",
        .output = {MaterialId::Flesh, MaterialState::Charred, MaterialState::None},
        .callbacks = {
            .emission = {
                .smell = {.appetizing = 30.0f},
                .vision = {.attract = 10.0f},
            },
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.food);
            },
            .onEat = [](const Agent&) -> FoodEffect {
                return {FoodValidity::EdibleRisky, 0.6f, 0.15f, true};
            },
        }
    });

    // Corpse (Dead Flesh) — edible but risky, repulsive smell
    behaviors.Register({
        .id = "corpse",
        .output = {MaterialId::Flesh, MaterialState::Dead, MaterialState::None},
        .callbacks = {
            .emission = {
                .smell = {.repulsive = 40.0f},
            },
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.food);
                out.push_back(concepts.death);
            },
            .onEat = [](const Agent&) -> FoodEffect {
                return {FoodValidity::EdibleRisky, 0.7f, 0.45f, true};
            },
        }
    });

    // Wood — material
    behaviors.Register({
        .id = "wood",
        .output = {MaterialId::Wood, MaterialState::None, MaterialState::None},
        .callbacks = {
            .emission = {.smell = {.neutral = 8.0f}},
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.wood);
            },
        }
    });

    // Tree — material (same concept as wood)
    behaviors.Register({
        .id = "tree",
        .output = {MaterialId::Tree, MaterialState::None, MaterialState::None},
        .callbacks = {
            .emission = {.smell = {.neutral = 10.0f}},
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.wood);
            },
        }
    });

    // Stone — material
    behaviors.Register({
        .id = "stone",
        .output = {MaterialId::Stone, MaterialState::None, MaterialState::None},
        .callbacks = {
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.stone);
            },
        }
    });

    // Water
    behaviors.Register({
        .id = "water",
        .output = {MaterialId::Water, MaterialState::None, MaterialState::None},
        .callbacks = {
            .emission = {.vision = {.attract = 15.0f}},
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.water);
            },
        }
    });

    // RottingPlant — not food
    behaviors.Register({
        .id = "rotting_plant",
        .output = {MaterialId::RottingPlant, MaterialState::Decomposing, MaterialState::None},
        .callbacks = {
            .emission = {.smell = {.repulsive = 15.0f}},
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.grass);
            },
        }
    });

    // RottingPlant (default state) — not food
    behaviors.Register({
        .id = "rotting_plant_default",
        .output = {MaterialId::RottingPlant, MaterialState::None, MaterialState::None},
        .callbacks = {
            .emission = {.smell = {.repulsive = 15.0f}},
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.grass);
            },
        }
    });

    // Leaf — not food
    behaviors.Register({
        .id = "leaf",
        .output = {MaterialId::Leaf, MaterialState::None, MaterialState::None},
        .callbacks = {
            .emission = {.smell = {.neutral = 3.0f}},
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.grass);
            },
        }
    });

    // Bush — not food
    behaviors.Register({
        .id = "bush",
        .output = {MaterialId::Bush, MaterialState::None, MaterialState::None},
        .callbacks = {
            .emission = {.smell = {.neutral = 4.0f}},
            .inferConcepts = [&concepts](const Agent&, std::vector<ConceptTypeId>& out) {
                out.push_back(concepts.grass);
            },
        }
    });
}

// Register time-based food lifecycle reactions into the SemanticReactionSystem.
// These handle cooking (burning flesh → charred) and decomposition (flesh → rotting).
inline void RegisterLifecycleReactions(SemanticReactionSystem& reactionSys)
{
    // Cook: Flesh + Dead + Burning for ≥1 hour → remove Burning, add Charred
    {
        SemanticReactionRule rule;
        rule.id = "cook_flesh";
        rule.name = "Cook flesh over fire";
        rule.targetMode = ReactionTargetMode::SameEntity;
        rule.probability = 1.0f;

        SemanticPredicate isFlesh;
        isFlesh.type = PredicateType::HasMaterial;
        isFlesh.material = MaterialId::Flesh;

        SemanticPredicate isDead;
        isDead.type = PredicateType::HasState;
        isDead.state = MaterialState::Dead;

        SemanticPredicate isBurning;
        isBurning.type = PredicateType::HasState;
        isBurning.state = MaterialState::Burning;

        SemanticPredicate duration;
        duration.type = PredicateType::StateDurationGreaterThan;
        duration.minDuration = 30;  // 30 ticks to cook

        rule.conditions = {isFlesh, isDead, isBurning, duration};

        ReactionEffect removeBurning;
        removeBurning.type = EffectType::RemoveState;
        removeBurning.state = MaterialState::Burning;

        ReactionEffect addCharred;
        addCharred.type = EffectType::AddState;
        addCharred.state = MaterialState::Charred;

        rule.effects = {removeBurning, addCharred};
        reactionSys.AddRule(rule);
    }

    // Decompose flesh: Flesh + Dead for ≥1 day → add Decomposing
    {
        SemanticReactionRule rule;
        rule.id = "decompose_flesh";
        rule.name = "Corpse decomposition";
        rule.targetMode = ReactionTargetMode::SameEntity;
        rule.probability = 1.0f;
        rule.repeatable = false;  // only fires once

        SemanticPredicate isFlesh;
        isFlesh.type = PredicateType::HasMaterial;
        isFlesh.material = MaterialId::Flesh;

        SemanticPredicate isDead;
        isDead.type = PredicateType::HasState;
        isDead.state = MaterialState::Dead;

        SemanticPredicate duration;
        duration.type = PredicateType::StateDurationGreaterThan;
        duration.minDuration = 200;  // 200 ticks to decompose

        rule.conditions = {isFlesh, isDead, duration};

        ReactionEffect addDecomposing;
        addDecomposing.type = EffectType::AddState;
        addDecomposing.state = MaterialState::Decomposing;

        rule.effects = {addDecomposing};
        reactionSys.AddRule(rule);
    }
}
