#pragma once

#include "sim/system/i_system.h"
#include "sim/world/world_state.h"
#include "rules/reaction/reaction_rule.h"
#include <vector>
#include <cmath>

class ReactionSystem : public ISystem
{
public:
    void AddRule(const ReactionRule& rule)
    {
        rules.push_back(rule);
    }

    void Update(WorldState& world) override
    {
        auto& env = world.Env();
        auto& info = world.Info();
        auto& sim = world.Sim();

        for (const auto& rule : rules)
        {
            for (i32 y = 0; y < env.height; y++)
            {
                for (i32 x = 0; x < env.width; x++)
                {
                    if (!CheckInputs(world, x, y, rule)) continue;
                    if (!CheckConditions(world, x, y, rule)) continue;

                    // Probability check
                    if (sim.random.Next01() > rule.baseProbability) continue;

                    ApplyOutputs(world, x, y, rule);
                }
            }
        }
    }

private:
    std::vector<ReactionRule> rules;

    bool CheckInputs(WorldState& world, i32 x, i32 y, const ReactionRule& rule)
    {
        auto& env = world.Env();

        for (const auto& input : rule.inputs)
        {
            if (input == "fire")
            {
                if (env.fire.At(x, y) <= 0.0f) return false;
            }
            else if (input == "dry_grass")
            {
                if (env.humidity.At(x, y) >= 35.0f) return false;
            }
            else if (input == "water")
            {
                if (env.humidity.At(x, y) < 80.0f) return false;
            }
            else if (input == "smoke")
            {
                if (world.Info().smoke.At(x, y) <= 0.0f) return false;
            }
            // Add more input types as needed
        }
        return true;
    }

    bool CheckConditions(WorldState& world, i32 x, i32 y, const ReactionRule& rule)
    {
        auto& env = world.Env();
        auto& info = world.Info();

        for (const auto& cond : rule.conditions)
        {
            f32 fieldValue = 0.0f;

            if (cond.field == "temperature")
                fieldValue = env.temperature.At(x, y);
            else if (cond.field == "humidity")
                fieldValue = env.humidity.At(x, y);
            else if (cond.field == "fire")
                fieldValue = env.fire.At(x, y);
            else if (cond.field == "smell")
                fieldValue = info.smell.At(x, y);
            else if (cond.field == "danger")
                fieldValue = info.danger.At(x, y);
            else if (cond.field == "smoke")
                fieldValue = info.smoke.At(x, y);
            else if (cond.field == "wind_speed")
            {
                f32 wx = env.wind.x;
                f32 wy = env.wind.y;
                fieldValue = std::sqrt(wx * wx + wy * wy);
            }
            else
                continue;  // Unknown field, skip

            if (!EvaluateCondition(cond, fieldValue)) return false;
        }
        return true;
    }

    void ApplyOutputs(WorldState& world, i32 x, i32 y, const ReactionRule& rule)
    {
        auto& env = world.Env();
        auto& info = world.Info();

        for (const auto& output : rule.outputs)
        {
            switch (output.type)
            {
            case OutputType::IncreaseFire:
                env.fire.WriteNext(x, y) = env.fire.At(x, y) + output.value;
                break;
            case OutputType::DecreaseFire:
                env.fire.WriteNext(x, y) = std::max(0.0f, env.fire.At(x, y) - output.value);
                break;
            case OutputType::EmitSmell:
                info.smell.WriteNext(x, y) += output.value;
                break;
            case OutputType::EmitSmoke:
                info.smoke.WriteNext(x, y) += output.value;
                break;
            case OutputType::SetDanger:
                info.danger.WriteNext(x, y) = std::max(info.danger.At(x, y), output.value);
                break;
            default:
                break;
            }
        }
    }
};
