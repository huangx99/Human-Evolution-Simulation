#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include "rules/reaction/semantic_reaction_rule.h"
#include <vector>
#include <cmath>

class SemanticReactionSystem : public ISystem
{
public:
    void AddRule(const SemanticReactionRule& rule)
    {
        rules.push_back(rule);
    }

    void Update(WorldState& world) override
    {
        auto& env = world.Env();
        auto& sim = world.Sim();

        for (const auto& rule : rules)
        {
            for (i32 y = 0; y < env.height; y++)
            {
                for (i32 x = 0; x < env.width; x++)
                {
                    if (!EvaluatePredicates(world, x, y, rule)) continue;
                    if (sim.random.Next01() > rule.probability) continue;

                    ApplyEffects(world, x, y, rule);
                }
            }
        }
    }

private:
    std::vector<SemanticReactionRule> rules;

    bool EvaluatePredicates(WorldState& world, i32 x, i32 y, const SemanticReactionRule& rule)
    {
        for (const auto& pred : rule.conditions)
        {
            if (!EvaluatePredicate(world, x, y, pred)) return false;
        }
        return true;
    }

    bool EvaluatePredicate(WorldState& world, i32 x, i32 y, const SemanticPredicate& pred)
    {
        auto& env = world.Env();
        auto& info = world.Info();
        auto& ecology = world.Ecology().entities;

        switch (pred.type)
        {
        case PredicateType::HasCapability:
        {
            for (auto& e : ecology.At(x, y))
            {
                if (e.HasCapability(pred.capability)) return true;
            }
            return false;
        }
        case PredicateType::HasAffordance:
        {
            for (auto& e : ecology.At(x, y))
            {
                if (e.HasAffordance(pred.affordance)) return true;
            }
            return false;
        }
        case PredicateType::HasState:
        {
            for (auto& e : ecology.At(x, y))
            {
                if (e.HasState(pred.state)) return true;
            }
            return false;
        }
        case PredicateType::HasMaterial:
        {
            for (auto& e : ecology.At(x, y))
            {
                if (e.material == pred.material) return true;
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
            for (i32 dy = -pred.radius; dy <= pred.radius; dy++)
            {
                for (i32 dx = -pred.radius; dx <= pred.radius; dx++)
                {
                    if (dx == 0 && dy == 0) continue;
                    i32 nx = x + dx;
                    i32 ny = y + dy;
                    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy));
                    if (dist > pred.radius) continue;

                    for (auto& e : ecology.At(nx, ny))
                    {
                        if (e.HasCapability(pred.capability)) return true;
                    }
                }
            }
            return false;
        }
        case PredicateType::NearbyState:
        {
            for (i32 dy = -pred.radius; dy <= pred.radius; dy++)
            {
                for (i32 dx = -pred.radius; dx <= pred.radius; dx++)
                {
                    if (dx == 0 && dy == 0) continue;
                    i32 nx = x + dx;
                    i32 ny = y + dy;
                    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy));
                    if (dist > pred.radius) continue;

                    for (auto& e : ecology.At(nx, ny))
                    {
                        if (e.HasState(pred.state)) return true;
                    }
                }
            }
            return false;
        }
        default:
            return false;
        }
    }

    f32 GetFieldValue(WorldState& world, i32 x, i32 y, FieldId field)
    {
        auto& env = world.Env();
        auto& info = world.Info();

        switch (field)
        {
        case FieldId::Temperature: return env.temperature.At(x, y);
        case FieldId::Humidity:    return env.humidity.At(x, y);
        case FieldId::Fire:        return env.fire.At(x, y);
        case FieldId::Smell:       return info.smell.At(x, y);
        case FieldId::Danger:      return info.danger.At(x, y);
        case FieldId::Smoke:       return info.smoke.At(x, y);
        case FieldId::WindSpeed:
        {
            f32 wx = env.wind.x;
            f32 wy = env.wind.y;
            return std::sqrt(wx * wx + wy * wy);
        }
        default: return 0.0f;
        }
    }

    void ApplyEffects(WorldState& world, i32 x, i32 y, const SemanticReactionRule& rule)
    {
        auto& env = world.Env();
        auto& info = world.Info();
        auto& ecology = world.Ecology().entities;

        for (const auto& effect : rule.effects)
        {
            switch (effect.type)
            {
            case EffectType::AddState:
            {
                for (auto& e : ecology.At(x, y))
                    e.AddState(effect.state);
                break;
            }
            case EffectType::RemoveState:
            {
                for (auto& e : ecology.At(x, y))
                    e.state = e.state & ~effect.state;
                break;
            }
            case EffectType::AddCapability:
            {
                for (auto& e : ecology.At(x, y))
                    e.AddCapability(effect.capability);
                break;
            }
            case EffectType::RemoveCapability:
            {
                for (auto& e : ecology.At(x, y))
                    e.extraCapabilities = static_cast<Capability>(
                        static_cast<u32>(e.extraCapabilities) & ~static_cast<u32>(effect.capability));
                break;
            }
            case EffectType::ModifyField:
            {
                f32 current = GetFieldValue(world, x, y, effect.field);
                f32 newVal = (effect.delta != 0.0f) ? current + effect.delta : effect.setTo;
                SetFieldValue(world, x, y, effect.field, newVal);
                break;
            }
            case EffectType::EmitSmell:
                info.smell.WriteNext(x, y) += effect.delta;
                break;
            case EffectType::EmitSmoke:
                info.smoke.WriteNext(x, y) += effect.delta;
                break;
            case EffectType::EmitEvent:
                // Events are emitted via the EventBus; this is a placeholder
                // for future event emission from semantic reactions
                break;
            default:
                break;
            }
        }
    }

    void SetFieldValue(WorldState& world, i32 x, i32 y, FieldId field, f32 value)
    {
        auto& env = world.Env();
        auto& info = world.Info();

        switch (field)
        {
        case FieldId::Temperature: env.temperature.WriteNext(x, y) = value; break;
        case FieldId::Humidity:    env.humidity.WriteNext(x, y) = value; break;
        case FieldId::Fire:        env.fire.WriteNext(x, y) = value; break;
        case FieldId::Smell:       info.smell.WriteNext(x, y) = value; break;
        case FieldId::Danger:      info.danger.WriteNext(x, y) = value; break;
        case FieldId::Smoke:       info.smoke.WriteNext(x, y) = value; break;
        default: break;
        }
    }
};
