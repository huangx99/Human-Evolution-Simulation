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
        auto& sim = world.Sim();

        for (const auto& rule : rules)
        {
            for (i32 y = 0; y < env.height; y++)
            {
                for (i32 x = 0; x < env.width; x++)
                {
                    if (!CheckInputs(world, x, y, rule)) continue;
                    if (!CheckFieldConditions(world, x, y, rule)) continue;
                    if (!CheckCapabilityConditions(world, x, y, rule)) continue;

                    // Probability check
                    if (sim.random.Next01() > rule.baseProbability) continue;

                    ApplyOutputs(world, x, y, rule);
                }
            }
        }
    }

private:
    std::vector<ReactionRule> rules;

    // Check ElementId inputs using capability-based logic from EcologyRegistry
    bool CheckInputs(WorldState& world, i32 x, i32 y, const ReactionRule& rule)
    {
        auto& env = world.Env();
        auto& info = world.Info();
        auto& ecology = world.Ecology().entities;

        for (const auto& input : rule.inputs)
        {
            switch (input)
            {
            case ElementId::Fire:
            {
                // Fire = any entity with HeatEmission at this cell, OR fire field > 0
                bool hasHeatSource = false;
                for (auto& e : ecology.At(x, y))
                {
                    if (e.HasCapability(Capability::HeatEmission))
                    {
                        hasHeatSource = true;
                        break;
                    }
                }
                if (!hasHeatSource && env.fire.At(x, y) <= 0.0f) return false;
                break;
            }
            case ElementId::DryGrass:
            {
                // DryGrass = entity with Flammable + (Dead|Dry) at this cell, OR low humidity
                bool hasFlammableDry = false;
                for (auto& e : ecology.At(x, y))
                {
                    if (e.HasCapability(Capability::Flammable) &&
                        (e.HasState(MaterialState::Dead) || e.HasState(MaterialState::Dry)))
                    {
                        hasFlammableDry = true;
                        break;
                    }
                }
                if (!hasFlammableDry && env.humidity.At(x, y) >= 35.0f) return false;
                break;
            }
            case ElementId::Water:
            {
                // Water = entity that is Wet/Soaked at this cell, OR high humidity
                bool hasWater = false;
                for (auto& e : ecology.At(x, y))
                {
                    if (e.HasState(MaterialState::Wet) || e.HasState(MaterialState::Soaked))
                    {
                        hasWater = true;
                        break;
                    }
                }
                if (!hasWater && env.humidity.At(x, y) < 80.0f) return false;
                break;
            }
            case ElementId::Smoke:
            {
                // Smoke = entity with SmokeEmission at this cell, OR smoke field > 0
                bool hasSmokeSource = false;
                for (auto& e : ecology.At(x, y))
                {
                    if (e.HasCapability(Capability::SmokeEmission))
                    {
                        hasSmokeSource = true;
                        break;
                    }
                }
                if (!hasSmokeSource && info.smoke.At(x, y) <= 0.0f) return false;
                break;
            }
            default:
                break;
            }
        }
        return true;
    }

    bool CheckFieldConditions(WorldState& world, i32 x, i32 y, const ReactionRule& rule)
    {
        auto& env = world.Env();
        auto& info = world.Info();

        for (const auto& cond : rule.fieldConditions)
        {
            f32 fieldValue = 0.0f;

            switch (cond.field)
            {
            case FieldId::Temperature:
                fieldValue = env.temperature.At(x, y);
                break;
            case FieldId::Humidity:
                fieldValue = env.humidity.At(x, y);
                break;
            case FieldId::Fire:
                fieldValue = env.fire.At(x, y);
                break;
            case FieldId::Smell:
                fieldValue = info.smell.At(x, y);
                break;
            case FieldId::Danger:
                fieldValue = info.danger.At(x, y);
                break;
            case FieldId::Smoke:
                fieldValue = info.smoke.At(x, y);
                break;
            case FieldId::WindSpeed:
            {
                f32 wx = env.wind.x;
                f32 wy = env.wind.y;
                fieldValue = std::sqrt(wx * wx + wy * wy);
                break;
            }
            default:
                continue;
            }

            if (!EvaluateCondition(cond, fieldValue)) return false;
        }
        return true;
    }

    bool CheckCapabilityConditions(WorldState& world, i32 x, i32 y, const ReactionRule& rule)
    {
        auto& ecology = world.Ecology().entities;

        for (const auto& cond : rule.capabilityConditions)
        {
            bool found = false;

            for (auto& entity : ecology.At(x, y))
            {
                bool match = true;

                if (cond.requiredCapabilities != Capability::None)
                {
                    Capability entityCaps = entity.GetCapabilities();
                    if ((static_cast<u32>(entityCaps) & static_cast<u32>(cond.requiredCapabilities))
                        != static_cast<u32>(cond.requiredCapabilities))
                    {
                        match = false;
                    }
                }

                if (cond.requiredAffordances != Affordance::None)
                {
                    Affordance entityAffs = entity.GetAffordances();
                    if ((static_cast<u32>(entityAffs) & static_cast<u32>(cond.requiredAffordances))
                        != static_cast<u32>(cond.requiredAffordances))
                    {
                        match = false;
                    }
                }

                if (cond.requiredStates != MaterialState::None)
                {
                    if (!entity.HasState(cond.requiredStates))
                    {
                        match = false;
                    }
                }

                if (match)
                {
                    found = true;
                    break;
                }
            }

            if (!found) return false;
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
