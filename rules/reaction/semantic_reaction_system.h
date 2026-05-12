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

                    SubmitEffects(world, x, y, rule);
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
            for (auto* e : ecology.At(x, y))
            {
                if (e->HasCapability(pred.capability)) return true;
            }
            return false;
        }
        case PredicateType::HasAffordance:
        {
            for (auto* e : ecology.At(x, y))
            {
                if (e->HasAffordance(pred.affordance)) return true;
            }
            return false;
        }
        case PredicateType::HasState:
        {
            for (auto* e : ecology.At(x, y))
            {
                if (e->HasState(pred.state)) return true;
            }
            return false;
        }
        case PredicateType::HasMaterial:
        {
            for (auto* e : ecology.At(x, y))
            {
                if (e->material == pred.material) return true;
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
            auto& env = world.Env();
            for (i32 dy = -pred.radius; dy <= pred.radius; dy++)
            {
                for (i32 dx = -pred.radius; dx <= pred.radius; dx++)
                {
                    if (dx == 0 && dy == 0) continue;
                    i32 nx = x + dx;
                    i32 ny = y + dy;
                    if (!env.temperature.InBounds(nx, ny)) continue;
                    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy));
                    if (dist > pred.radius) continue;

                    for (auto* e : ecology.At(nx, ny))
                    {
                        if (e->HasCapability(pred.capability)) return true;
                    }
                }
            }
            return false;
        }
        case PredicateType::NearbyState:
        {
            auto& env = world.Env();
            for (i32 dy = -pred.radius; dy <= pred.radius; dy++)
            {
                for (i32 dx = -pred.radius; dx <= pred.radius; dx++)
                {
                    if (dx == 0 && dy == 0) continue;
                    i32 nx = x + dx;
                    i32 ny = y + dy;
                    if (!env.temperature.InBounds(nx, ny)) continue;
                    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy));
                    if (dist > pred.radius) continue;

                    for (auto* e : ecology.At(nx, ny))
                    {
                        if (e->HasState(pred.state)) return true;
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
                cmd.value = effect.delta;
                if (effect.delta == 0.0f)
                {
                    cmd.targetY = 1; // flag: use setTo
                    cmd.value = effect.setTo;
                }
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
                evt.type = EventType::SmellEmitted;
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
