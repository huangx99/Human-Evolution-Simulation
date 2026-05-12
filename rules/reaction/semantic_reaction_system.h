#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include "rules/reaction/semantic_reaction_rule.h"
#include "sim/command/command.h"
#include "sim/event/event.h"
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
        auto& sim = world.Sim();
        auto& spatial = world.spatial;

        auto positions = spatial.AllPositions();

        for (const auto& rule : rules)
        {
            for (auto& [x, y] : positions)
            {
                if (!EvaluatePredicates(world, x, y, rule, spatial)) continue;
                if (sim.random.Next01() > rule.probability) continue;

                SubmitEffects(world, x, y, rule);
            }
        }
    }

private:
    std::vector<SemanticReactionRule> rules;

    bool EvaluatePredicates(WorldState& world, i32 x, i32 y,
                            const SemanticReactionRule& rule, const SpatialIndex& spatial)
    {
        for (const auto& pred : rule.conditions)
        {
            if (!EvaluatePredicate(world, x, y, pred, spatial)) return false;
        }
        return true;
    }

    bool EvaluatePredicate(WorldState& world, i32 x, i32 y,
                           const SemanticPredicate& pred, const SpatialIndex& spatial)
    {
        auto& env = world.Env();
        auto& info = world.Info();

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

    void SubmitEffects(WorldState& world, i32 x, i32 y, const SemanticReactionRule& rule)
    {
        auto& ecology = world.Ecology().entities;

        for (const auto& effect : rule.effects)
        {
            Command cmd;
            cmd.x = x;
            cmd.y = y;

            switch (effect.type)
            {
            case EffectType::AddState:
            {
                cmd.type = CommandType::AddEntityState;
                cmd.value = static_cast<f32>(static_cast<u32>(effect.state));
                for (auto* e : ecology.At(x, y))
                {
                    Command entityCmd = cmd;
                    entityCmd.entityId = e->id;
                    world.commands.Submit(entityCmd);
                }
                break;
            }
            case EffectType::RemoveState:
            {
                cmd.type = CommandType::RemoveEntityState;
                cmd.value = static_cast<f32>(static_cast<u32>(effect.state));
                for (auto* e : ecology.At(x, y))
                {
                    Command entityCmd = cmd;
                    entityCmd.entityId = e->id;
                    world.commands.Submit(entityCmd);
                }
                break;
            }
            case EffectType::AddCapability:
            {
                cmd.type = CommandType::AddEntityCapability;
                cmd.value = static_cast<f32>(static_cast<u32>(effect.capability));
                for (auto* e : ecology.At(x, y))
                {
                    Command entityCmd = cmd;
                    entityCmd.entityId = e->id;
                    world.commands.Submit(entityCmd);
                }
                break;
            }
            case EffectType::RemoveCapability:
            {
                cmd.type = CommandType::RemoveEntityCapability;
                cmd.value = static_cast<f32>(static_cast<u32>(effect.capability));
                for (auto* e : ecology.At(x, y))
                {
                    Command entityCmd = cmd;
                    entityCmd.entityId = e->id;
                    world.commands.Submit(entityCmd);
                }
                break;
            }
            case EffectType::ModifyField:
            {
                cmd.type = CommandType::ModifyFieldValue;
                cmd.targetX = static_cast<i32>(effect.field);
                cmd.targetY = static_cast<i32>(effect.mode);
                cmd.value = effect.value;
                world.commands.Submit(cmd);
                break;
            }
            case EffectType::EmitSmell:
            {
                cmd.type = CommandType::EmitSmell;
                cmd.value = effect.delta;
                world.commands.Submit(cmd);
                break;
            }
            case EffectType::EmitSmoke:
            {
                cmd.type = CommandType::EmitSmoke;
                cmd.value = effect.delta;
                world.commands.Submit(cmd);
                break;
            }
            case EffectType::EmitEvent:
            {
                Event evt;
                evt.type = EventType::ReactionTriggered;
                evt.tick = world.Sim().clock.currentTick;
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
