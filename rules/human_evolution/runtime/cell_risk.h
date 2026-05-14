#pragma once

// CellRiskEvaluator: unified continuous risk assessment for a grid cell.
//
// ARCHITECTURE NOTE: This replaces the boolean IsHazardous() gate.
// Risk is always 0..1 per component, never a hard wall.
//
// Core rules:
//   - Blocked ONLY comes from physical lethal conditions (fire>85, temp>105) or out-of-bounds.
//   - MemoryRisk and SocialRisk can NEVER produce Blocked on their own.
//   - All risk components are normalized to 0..1 before weighting.

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/field/field_module.h"
#include "sim/cognitive/cognitive_module.h"
#include "sim/cognitive/concept_registry.h"
#include "rules/human_evolution/human_evolution_context.h"
#include <algorithm>
#include <cmath>

enum class TraversalClass : u8
{
    Passable,
    Risky,
    Blocked
};

struct CellRisk
{
    f32 physical = 0.0f;   // fire, temperature, objective danger field
    f32 memory   = 0.0f;   // individual danger memories
    f32 social   = 0.0f;   // group danger, fear signals
    f32 terrain  = 0.0f;   // terrain traversal cost (future extension)
    f32 total    = 0.0f;

    bool outOfBounds = false;
    bool lethal = false;
    TraversalClass traversal = TraversalClass::Passable;
};

struct CellRiskEvaluator
{
    // Normalize fire/temp/danger into 0..1 physical risk.
    static f32 NormalizePhysicalRisk(f32 fire, f32 temp, f32 danger)
    {
        f32 fireRisk = std::clamp(fire / 80.0f, 0.0f, 1.0f);

        f32 tempRisk = 0.0f;
        if (temp > 35.0f)
            tempRisk = std::clamp((temp - 35.0f) / 65.0f, 0.0f, 1.0f);

        f32 dangerRisk = std::clamp(danger / 100.0f, 0.0f, 1.0f);

        return std::clamp(std::max({fireRisk, tempRisk, dangerRisk}), 0.0f, 1.0f);
    }

    // Saturating function for memory strength: maps unbounded strength to 0..1.
    // Prevents accumulated strength from dominating risk assessment.
    static f32 SaturateStrength(f32 strength)
    {
        if (!std::isfinite(strength))
            return 0.0f;

        strength = std::max(0.0f, strength);
        return 1.0f - std::exp(-strength / 6.0f);
    }

    // Check if a concept has danger-related semantic flags.
    static bool IsDangerConcept(ConceptTypeId concept)
    {
        const auto& reg = ConceptTypeRegistry::Instance();
        return reg.HasFlag(concept, ConceptSemanticFlag::Danger) ||
               reg.HasFlag(concept, ConceptSemanticFlag::Thermal) ||
               reg.HasFlag(concept, ConceptSemanticFlag::TraumaRelevant);
    }

    // Evaluate memory-based subjective risk for a cell. Output 0..1.
    static f32 EvaluateMemoryRisk(const CognitiveModule& cog,
                                  EntityId agentId,
                                  i32 x, i32 y)
    {
        f32 risk = 0.0f;

        for (const auto& memory : cog.GetAgentMemories(agentId))
        {
            if (!IsDangerConcept(memory.subject))
            {
                // Also check result tags for danger
                bool dangerous = false;
                for (const auto& tag : memory.resultTags)
                {
                    if (IsDangerConcept(tag) ||
                        ConceptTypeRegistry::Instance().GetName(tag) == "pain")
                    {
                        dangerous = true;
                        break;
                    }
                }
                if (!dangerous) continue;
            }

            f32 dx = static_cast<f32>(x - memory.location.x);
            f32 dy = static_cast<f32>(y - memory.location.y);
            f32 distSq = dx * dx + dy * dy;

            f32 strength = SaturateStrength(memory.strength);
            f32 confidence = std::clamp(memory.confidence, 0.0f, 1.0f);
            f32 falloff = 1.0f / (1.0f + distSq * 0.35f);

            risk += strength * confidence * falloff;
        }

        return std::clamp(risk, 0.0f, 1.0f);
    }

    // Evaluate social/collective risk for a cell. Output 0..1.
    // Placeholder: currently returns 0. Will integrate GroupKnowledge
    // and SocialSignals when those modules are wired in.
    static f32 EvaluateSocialRisk(EntityId agentId, i32 x, i32 y)
    {
        // TODO: integrate GroupKnowledge records and SocialSignal fear signals
        return 0.0f;
    }

    // Unified cell risk evaluation. This is the single entry point.
    static CellRisk Evaluate(const FieldModule& fm,
                             const CognitiveModule& cog,
                             const HumanEvolution::EnvironmentContext& envCtx,
                             EntityId agentId,
                             i32 x, i32 y)
    {
        CellRisk r;

        if (!fm.InBounds(envCtx.temperature, x, y))
        {
            r.outOfBounds = true;
            r.lethal = true;
            r.traversal = TraversalClass::Blocked;
            r.total = 1.0f;
            return r;
        }

        const f32 fire   = fm.InBounds(envCtx.fire, x, y) ? fm.Read(envCtx.fire, x, y) : 0.0f;
        const f32 temp   = fm.Read(envCtx.temperature, x, y);
        const f32 danger = fm.InBounds(envCtx.danger, x, y) ? fm.Read(envCtx.danger, x, y) : 0.0f;

        r.physical = NormalizePhysicalRisk(fire, temp, danger);
        r.memory   = EvaluateMemoryRisk(cog, agentId, x, y);
        r.social   = EvaluateSocialRisk(agentId, x, y);

        r.total = std::clamp(
            r.physical * 0.60f +
            r.memory   * 0.25f +
            r.social   * 0.15f,
            0.0f, 1.0f
        );

        // Only physical lethality can block traversal
        const bool lethalFire = fire > 85.0f;
        const bool lethalTemp = temp > 105.0f;

        if (lethalFire || lethalTemp)
        {
            r.lethal = true;
            r.traversal = TraversalClass::Blocked;
        }
        else if (r.total > 0.25f)
        {
            r.traversal = TraversalClass::Risky;
        }
        else
        {
            r.traversal = TraversalClass::Passable;
        }

        return r;
    }
};
