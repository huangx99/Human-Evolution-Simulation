#pragma once

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/reaction/semantic_reaction_rule.h"
#include "sim/command/command.h"
#include "rules/human_evolution/commands.h"
#include "sim/event/event.h"
#include <vector>
#include <cmath>

// OWNERSHIP: Rule (rules/reaction/)
// READS: EcologyModule (entities), FieldModule (via FieldKey lookup), SimulationModule (random, clock)
// WRITES: EcologyModule (entity state, capabilities) via CommandBuffer, fields via CommandBuffer
// PHASE: SimPhase::Reaction

class SemanticReactionSystem : public ISystem
{
public:
    void AddRule(const SemanticReactionRule& rule)
    {
        rules.push_back(rule);
    }

    void Update(SystemContext& ctx) override
    {
        auto& world = ctx.World();
        auto& sim = world.Sim();
        auto& spatial = world.spatial;

        auto positions = spatial.AllPositions();

        for (const auto& rule : rules)
        {
            for (auto& [x, y] : positions)
            {
                if (sim.random.Next01() > rule.probability) continue;

                if (rule.targetMode == ReactionTargetMode::SameEntity)
                {
                    // Find a single entity that satisfies all entity-level predicates,
                    // then check non-entity predicates. Apply effects to that entity only.
                    auto candidates = world.Ecology().entities.At(x, y);
                    EcologyEntity* bound = FindBoundEntity(world, x, y, rule, spatial, candidates);
                    if (!bound) continue;
                    SubmitEffectsBound(world, x, y, rule, bound);
                }
                else
                {
                    // CellWide: predicates can be satisfied by different entities.
                    // Effects apply to ALL entities in the cell — use only for environment reactions.
                    if (!EvaluatePredicates(world, x, y, rule, spatial)) continue;
                    SubmitEffects(world, x, y, rule);
                }
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Ecology, AccessMode::Read},
            {ModuleTag::Environment, AccessMode::Read},
            {ModuleTag::Information, AccessMode::Read},
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Ecology, AccessMode::Write},
            {ModuleTag::Information, AccessMode::Write},
            {ModuleTag::Environment, AccessMode::Write},
            {ModuleTag::Event, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"SemanticReactionSystem", SimPhase::Reaction, READS, 4, WRITES, 4, DEPS, 0, true, false};
    }

private:
    std::vector<SemanticReactionRule> rules;

    // --- Entity-level predicate check against a single entity ---

    bool EntityMatchesPredicate(const EcologyEntity& e, const SemanticPredicate& pred)
    {
        switch (pred.type)
        {
        case PredicateType::HasCapability: return e.HasCapability(pred.capability);
        case PredicateType::HasAffordance: return e.HasAffordance(pred.affordance);
        case PredicateType::HasState:      return e.HasState(pred.state);
        case PredicateType::HasMaterial:   return e.material == pred.material;
        default: return true;  // non-entity predicates don't constrain the entity
        }
    }

    // Find a single entity at (x,y) that satisfies ALL entity-level predicates.
    // Non-entity predicates (Field, Nearby) are checked separately and must also pass.
    EcologyEntity* FindBoundEntity(WorldState& world, i32 x, i32 y,
                                   const SemanticReactionRule& rule, const SpatialIndex& spatial,
                                   std::vector<EcologyEntity*>& candidates)
    {
        for (auto* e : candidates)
        {
            if (e->x != x || e->y != y) continue;

            bool allMatch = true;
            bool hasEntityPred = false;

            for (const auto& pred : rule.conditions)
            {
                if (IsEntityPredicate(pred.type))
                {
                    hasEntityPred = true;
                    if (!EntityMatchesPredicate(*e, pred))
                    {
                        allMatch = false;
                        break;
                    }
                }
            }

            if (!allMatch) continue;
            if (!hasEntityPred) continue;  // SameEntity mode requires at least one entity predicate

            // All entity-level predicates pass for this entity.
            // Now check non-entity predicates (field, nearby) at cell level.
            if (EvaluateNonEntityPredicates(world, x, y, rule, spatial))
                return e;
        }
        return nullptr;
    }

    static bool IsEntityPredicate(PredicateType type)
    {
        return type == PredicateType::HasCapability
            || type == PredicateType::HasAffordance
            || type == PredicateType::HasState
            || type == PredicateType::HasMaterial;
    }

    // --- Cell-level predicate evaluation (SameCell mode) ---

    bool EvaluatePredicates(WorldState& world, i32 x, i32 y,
                            const SemanticReactionRule& rule, const SpatialIndex& spatial)
    {
        for (const auto& pred : rule.conditions)
        {
            if (!EvaluatePredicate(world, x, y, pred, spatial)) return false;
        }
        return true;
    }

    // Only non-entity predicates (Field, Nearby). Used by SameEntity after entity binding.
    bool EvaluateNonEntityPredicates(WorldState& world, i32 x, i32 y,
                                     const SemanticReactionRule& rule, const SpatialIndex& spatial)
    {
        for (const auto& pred : rule.conditions)
        {
            if (IsEntityPredicate(pred.type)) continue;
            if (!EvaluatePredicate(world, x, y, pred, spatial)) return false;
        }
        return true;
    }

    bool EvaluatePredicate(WorldState& world, i32 x, i32 y,
                           const SemanticPredicate& pred, const SpatialIndex& spatial)
    {
        switch (pred.type)
        {
        case PredicateType::HasCapability:
        {
            auto nearby = spatial.QueryArea(x, y, 0);
            for (auto* e : nearby)
            {
                if (e->x == x && e->y == y && e->HasCapability(pred.capability))
                    return true;
            }
            return false;
        }
        case PredicateType::HasAffordance:
        {
            auto nearby = spatial.QueryArea(x, y, 0);
            for (auto* e : nearby)
            {
                if (e->x == x && e->y == y && e->HasAffordance(pred.affordance))
                    return true;
            }
            return false;
        }
        case PredicateType::HasState:
        {
            auto nearby = spatial.QueryArea(x, y, 0);
            for (auto* e : nearby)
            {
                if (e->x == x && e->y == y && e->HasState(pred.state))
                    return true;
            }
            return false;
        }
        case PredicateType::HasMaterial:
        {
            auto nearby = spatial.QueryArea(x, y, 0);
            for (auto* e : nearby)
            {
                if (e->x == x && e->y == y && e->material == pred.material)
                    return true;
            }
            return false;
        }
        case PredicateType::FieldGreaterThan:
        {
            f32 val = GetFieldValue(world, x, y, pred.field);
            return val > pred.value;
        }
        case PredicateType::FieldLessThan:
        {
            f32 val = GetFieldValue(world, x, y, pred.field);
            return val < pred.value;
        }
        case PredicateType::FieldEquals:
        {
            f32 val = GetFieldValue(world, x, y, pred.field);
            return std::abs(val - pred.value) < 0.01f;
        }
        case PredicateType::NearbyCapability:
        {
            auto nearby = spatial.QueryAreaWithCapability(x, y, pred.radius, pred.capability);
            for (auto* e : nearby)
            {
                if (e->x == x && e->y == y) continue;
                f32 dx = static_cast<f32>(e->x - x);
                f32 dy = static_cast<f32>(e->y - y);
                f32 dist = std::sqrt(dx * dx + dy * dy);
                if (dist <= pred.radius) return true;
            }
            return false;
        }
        case PredicateType::NearbyState:
        {
            auto nearby = spatial.QueryAreaWithState(x, y, pred.radius, pred.state);
            for (auto* e : nearby)
            {
                if (e->x == x && e->y == y) continue;
                f32 dx = static_cast<f32>(e->x - x);
                f32 dy = static_cast<f32>(e->y - y);
                f32 dist = std::sqrt(dx * dx + dy * dy);
                if (dist <= pred.radius) return true;
            }
            return false;
        }
        default:
            return false;
        }
    }

    f32 GetFieldValue(WorldState& world, i32 x, i32 y, FieldKey field)
    {
        auto& fm = world.Fields();
        auto idx = fm.FindByKey(field);
        if (!idx) return 0.0f;
        if (!fm.InBounds(idx, x, y)) return 0.0f;
        return fm.Read(idx, x, y);
    }

    // --- Effect submission ---

    // SameEntity: apply entity-level effects to bound entity only
    void SubmitEffectsBound(WorldState& world, i32 x, i32 y,
                            const SemanticReactionRule& rule, EcologyEntity* bound)
    {
        Tick tick = world.Sim().clock.currentTick;
        for (const auto& effect : rule.effects)
        {
            switch (effect.type)
            {
            case EffectType::AddState:
                world.commands.Submit(tick,
                    AddEntityStateCommand{bound->id, static_cast<u32>(effect.state)});
                break;
            case EffectType::RemoveState:
                world.commands.Submit(tick,
                    RemoveEntityStateCommand{bound->id, static_cast<u32>(effect.state)});
                break;
            case EffectType::AddCapability:
                world.commands.Submit(tick,
                    AddEntityCapabilityCommand{bound->id, static_cast<u32>(effect.capability)});
                break;
            case EffectType::RemoveCapability:
                world.commands.Submit(tick,
                    RemoveEntityCapabilityCommand{bound->id, static_cast<u32>(effect.capability)});
                break;
            case EffectType::ModifyField:
                world.commands.Submit(tick,
                    ModifyFieldValueCommand{x, y, effect.field, static_cast<i32>(effect.mode), effect.value});
                break;
            case EffectType::EmitSmell:
                world.commands.Submit(tick, EmitSmellCommand{x, y, effect.delta});
                break;
            case EffectType::EmitSmoke:
                world.commands.Submit(tick, EmitSmokeCommand{x, y, effect.delta});
                break;
            case EffectType::EmitEvent:
            {
                Event evt;
                evt.type = EventType::ReactionTriggered;
                evt.tick = tick;
                evt.x = x;
                evt.y = y;
                evt.value = effect.delta;
                world.events.Emit(evt);
                break;
            }
            default:
                break;
            }
        }
    }

    // SameCell: apply entity-level effects to all entities in cell (legacy behavior)
    void SubmitEffects(WorldState& world, i32 x, i32 y, const SemanticReactionRule& rule)
    {
        auto& ecology = world.Ecology().entities;
        Tick tick = world.Sim().clock.currentTick;

        for (const auto& effect : rule.effects)
        {
            switch (effect.type)
            {
            case EffectType::AddState:
                for (auto* e : ecology.At(x, y))
                    world.commands.Submit(tick,
                        AddEntityStateCommand{e->id, static_cast<u32>(effect.state)});
                break;
            case EffectType::RemoveState:
                for (auto* e : ecology.At(x, y))
                    world.commands.Submit(tick,
                        RemoveEntityStateCommand{e->id, static_cast<u32>(effect.state)});
                break;
            case EffectType::AddCapability:
                for (auto* e : ecology.At(x, y))
                    world.commands.Submit(tick,
                        AddEntityCapabilityCommand{e->id, static_cast<u32>(effect.capability)});
                break;
            case EffectType::RemoveCapability:
                for (auto* e : ecology.At(x, y))
                    world.commands.Submit(tick,
                        RemoveEntityCapabilityCommand{e->id, static_cast<u32>(effect.capability)});
                break;
            case EffectType::ModifyField:
                world.commands.Submit(tick,
                    ModifyFieldValueCommand{x, y, effect.field, static_cast<i32>(effect.mode), effect.value});
                break;
            case EffectType::EmitSmell:
                world.commands.Submit(tick, EmitSmellCommand{x, y, effect.delta});
                break;
            case EffectType::EmitSmoke:
                world.commands.Submit(tick, EmitSmokeCommand{x, y, effect.delta});
                break;
            case EffectType::EmitEvent:
            {
                Event evt;
                evt.type = EventType::ReactionTriggered;
                evt.tick = tick;
                evt.x = x;
                evt.y = y;
                evt.value = effect.delta;
                world.events.Emit(evt);
                break;
            }
            default:
                break;
            }
        }
    }
};
