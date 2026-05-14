#pragma once

// NavigationPolicy: intent-aware movement selection.
//
// Each intent type has its own strategy. All strategies share:
//   - CellRisk evaluation (continuous, not boolean)
//   - Blocked = physical lethal only
//   - Risky cells are passable (with cost)
//   - Stuck-breaker: after 3+ failed moves, bias toward any movement

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "sim/field/field_module.h"
#include "sim/cognitive/cognitive_module.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/agent_intent.h"
#include "rules/human_evolution/runtime/action_feedback.h"
#include <algorithm>
#include <cmath>
#include <limits>

struct MoveCandidate
{
    i32 dx = 0;
    i32 dy = 0;
    Vec2i position{};
    CellRisk risk{};
    f32 score = -1e9f;
    bool valid = false;
};

struct MoveDecision
{
    i32 dx = 0;
    i32 dy = 0;
    bool attemptedMove = false;
    bool foundCandidate = false;
    f32 bestScore = 0.0f;
};

struct NavigationPolicy
{
    static constexpr i32 kMoveOffsets[5][2] =
    {
        {0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}
    };

    // Direction persistence bias: prefer continuing in the same direction
    static constexpr f32 kDirectionBias = 0.15f;

    static f32 DirectionPersistenceBonus(i32 dx, i32 dy, const AgentFeedback& fb)
    {
        // No direction bias when stuck — allow free exploration
        if (fb.stuckTicks >= 2) return 0.0f;
        if (fb.lastMoveDx == 0 && fb.lastMoveDy == 0) return 0.0f;
        if (dx == fb.lastMoveDx && dy == fb.lastMoveDy) return kDirectionBias;
        // Penalize reversing direction
        if (dx == -fb.lastMoveDx && dy == -fb.lastMoveDy) return -kDirectionBias;
        return 0.0f;
    }

    static f32 DistanceSquared(Vec2i a, Vec2i b)
    {
        i32 dx = a.x - b.x;
        i32 dy = a.y - b.y;
        return static_cast<f32>(dx * dx + dy * dy);
    }

    // === Escape: find the cell with the largest risk decrease ===
    static MoveDecision ChooseEscapeMove(
        const FieldModule& fm,
        const CognitiveModule& cog,
        const HumanEvolution::EnvironmentContext& envCtx,
        const Agent& agent,
        const AgentIntent& intent,
        const AgentFeedback& feedback)
    {
        MoveDecision result;

        const CellRisk currentRisk = CellRiskEvaluator::Evaluate(
            fm, cog, envCtx, agent.id, agent.position.x, agent.position.y);

        MoveCandidate best;
        best.position = agent.position;
        best.risk = currentRisk;
        best.score = -currentRisk.total;
        best.valid = true;

        for (const auto& off : kMoveOffsets)
        {
            const i32 nx = agent.position.x + off[0];
            const i32 ny = agent.position.y + off[1];

            const CellRisk r = CellRiskEvaluator::Evaluate(
                fm, cog, envCtx, agent.id, nx, ny);

            if (r.traversal == TraversalClass::Blocked)
                continue;

            const f32 escapeGain = currentRisk.total - r.total;
            const f32 moveBias = (off[0] == 0 && off[1] == 0) ? -0.15f : 0.10f;

            f32 score =
                escapeGain * 3.0f
                - r.total * 1.0f
                + moveBias
                + DirectionPersistenceBonus(off[0], off[1], feedback);

            // Stuck breaker: after 3+ failed moves, strongly prefer moving
            if (feedback.stuckTicks >= 3 && !(off[0] == 0 && off[1] == 0))
                score += 0.75f;

            if (score > best.score)
            {
                best.dx = off[0];
                best.dy = off[1];
                best.position = {nx, ny};
                best.risk = r;
                best.score = score;
                best.valid = true;
            }
        }

        result.dx = best.dx;
        result.dy = best.dy;
        result.attemptedMove = best.dx != 0 || best.dy != 0;
        result.foundCandidate = best.valid;
        result.bestScore = best.score;
        return result;
    }

    // === ApproachKnownFood: move toward target within risk tolerance ===
    static MoveDecision ChooseApproachTargetMove(
        const FieldModule& fm,
        const CognitiveModule& cog,
        const HumanEvolution::EnvironmentContext& envCtx,
        const Agent& agent,
        const AgentIntent& intent,
        const AgentFeedback& feedback)
    {
        MoveDecision result;
        const Vec2i target = intent.targetPosition;
        const f32 currentDist = DistanceSquared(agent.position, target);

        MoveCandidate best;
        best.score = -1e9f;

        for (const auto& off : kMoveOffsets)
        {
            const i32 nx = agent.position.x + off[0];
            const i32 ny = agent.position.y + off[1];

            const CellRisk r = CellRiskEvaluator::Evaluate(
                fm, cog, envCtx, agent.id, nx, ny);

            if (r.traversal == TraversalClass::Blocked)
                continue;

            if (r.total > intent.riskTolerance)
                continue;

            const f32 candidateDist = DistanceSquared({nx, ny}, target);
            const f32 distanceGain = currentDist - candidateDist;

            f32 foodSignal = 0.0f;
            if (fm.InBounds(envCtx.food, nx, ny))
                foodSignal = fm.Read(envCtx.food, nx, ny);

            f32 score =
                distanceGain * 2.0f
                - r.total * 1.5f
                + foodSignal * 0.5f
                + DirectionPersistenceBonus(off[0], off[1], feedback);

            if (feedback.stuckTicks >= 3 && !(off[0] == 0 && off[1] == 0))
                score += 0.75f;

            if (score > best.score)
            {
                best.dx = off[0];
                best.dy = off[1];
                best.position = {nx, ny};
                best.risk = r;
                best.score = score;
                best.valid = true;
            }
        }

        // Fallback: if no candidate within tolerance, try with relaxed tolerance
        if (!best.valid)
        {
            f32 relaxedTolerance = std::min(1.0f, intent.riskTolerance + 0.25f);
            for (const auto& off : kMoveOffsets)
            {
                if (off[0] == 0 && off[1] == 0) continue;
                const i32 nx = agent.position.x + off[0];
                const i32 ny = agent.position.y + off[1];

                const CellRisk r = CellRiskEvaluator::Evaluate(
                    fm, cog, envCtx, agent.id, nx, ny);

                if (r.traversal == TraversalClass::Blocked) continue;
                if (r.total > relaxedTolerance) continue;

                f32 score = -r.total;
                if (score > best.score)
                {
                    best.dx = off[0];
                    best.dy = off[1];
                    best.position = {nx, ny};
                    best.risk = r;
                    best.score = score;
                    best.valid = true;
                }
            }
        }

        result.dx = best.dx;
        result.dy = best.dy;
        result.attemptedMove = best.dx != 0 || best.dy != 0;
        result.foundCandidate = best.valid;
        result.bestScore = best.score;
        return result;
    }

    // === Forage: search for food using smell gradient + memory + novelty ===
    static MoveDecision ChooseForageMove(
        const FieldModule& fm,
        const CognitiveModule& cog,
        const HumanEvolution::EnvironmentContext& envCtx,
        const Agent& agent,
        const AgentIntent& intent,
        const AgentFeedback& feedback,
        f32 deterministicNoise = 0.0f)
    {
        MoveDecision result;

        f32 currentSmell = 0.0f;
        if (fm.InBounds(envCtx.food, agent.position.x, agent.position.y))
            currentSmell = fm.Read(envCtx.food, agent.position.x, agent.position.y);

        MoveCandidate best;
        best.score = -1e9f;

        for (const auto& off : kMoveOffsets)
        {
            const i32 nx = agent.position.x + off[0];
            const i32 ny = agent.position.y + off[1];

            const CellRisk r = CellRiskEvaluator::Evaluate(
                fm, cog, envCtx, agent.id, nx, ny);

            if (r.traversal == TraversalClass::Blocked)
                continue;

            if (r.total > intent.riskTolerance)
                continue;

            f32 foodSignal = 0.0f;
            if (fm.InBounds(envCtx.food, nx, ny))
                foodSignal = fm.Read(envCtx.food, nx, ny);

            f32 smellGradient = foodSignal - currentSmell;

            // Memory pull: is there a food memory near this cell?
            f32 foodMemoryPull = 0.0f;
            const auto& reg = ConceptTypeRegistry::Instance();
            for (const auto& mem : cog.GetAgentMemories(agent.id))
            {
                if (reg.GetName(mem.subject) != "food") continue;
                f32 ddx = static_cast<f32>(nx - mem.location.x);
                f32 ddy = static_cast<f32>(ny - mem.location.y);
                f32 distSq = ddx * ddx + ddy * ddy;
                if (distSq < 25.0f) // within 5 tiles
                    foodMemoryPull += mem.strength * mem.confidence / (1.0f + distSq);
            }
            foodMemoryPull = std::clamp(foodMemoryPull, 0.0f, 1.0f);

            f32 score =
                foodSignal * 2.0f
                + smellGradient * 1.2f
                + foodMemoryPull * 1.5f
                - r.total * 1.0f
                + deterministicNoise * 0.01f
                + DirectionPersistenceBonus(off[0], off[1], feedback);

            if (feedback.stuckTicks >= 3 && !(off[0] == 0 && off[1] == 0))
                score += 0.75f;

            if (score > best.score)
            {
                best.dx = off[0];
                best.dy = off[1];
                best.position = {nx, ny};
                best.risk = r;
                best.score = score;
                best.valid = true;
            }
        }

        // Fallback: if nothing in tolerance, pick lowest-risk non-blocked move
        if (!best.valid)
        {
            for (const auto& off : kMoveOffsets)
            {
                if (off[0] == 0 && off[1] == 0) continue;
                const i32 nx = agent.position.x + off[0];
                const i32 ny = agent.position.y + off[1];

                const CellRisk r = CellRiskEvaluator::Evaluate(
                    fm, cog, envCtx, agent.id, nx, ny);

                if (r.traversal == TraversalClass::Blocked) continue;

                f32 score = -r.total;
                if (score > best.score)
                {
                    best.dx = off[0];
                    best.dy = off[1];
                    best.position = {nx, ny};
                    best.risk = r;
                    best.score = score;
                    best.valid = true;
                }
            }
        }

        result.dx = best.dx;
        result.dy = best.dy;
        result.attemptedMove = best.dx != 0 || best.dy != 0;
        result.foundCandidate = best.valid;
        result.bestScore = best.score;
        return result;
    }

    // === Explore: low-risk novelty-seeking ===
    static MoveDecision ChooseExploreMove(
        const FieldModule& fm,
        const CognitiveModule& cog,
        const HumanEvolution::EnvironmentContext& envCtx,
        const Agent& agent,
        const AgentIntent& intent,
        const AgentFeedback& feedback,
        f32 deterministicNoise = 0.0f)
    {
        MoveDecision result;

        MoveCandidate best;
        best.score = -1e9f;

        for (const auto& off : kMoveOffsets)
        {
            const i32 nx = agent.position.x + off[0];
            const i32 ny = agent.position.y + off[1];

            const CellRisk r = CellRiskEvaluator::Evaluate(
                fm, cog, envCtx, agent.id, nx, ny);

            if (r.traversal == TraversalClass::Blocked)
                continue;

            f32 score =
                - r.total * 1.5f
                + deterministicNoise * 0.01f
                + DirectionPersistenceBonus(off[0], off[1], feedback);

            if (off[0] != 0 || off[1] != 0)
                score += 0.10f; // mild movement bias

            if (feedback.stuckTicks >= 3 && !(off[0] == 0 && off[1] == 0))
                score += 0.75f;

            if (score > best.score)
            {
                best.dx = off[0];
                best.dy = off[1];
                best.position = {nx, ny};
                best.risk = r;
                best.score = score;
                best.valid = true;
            }
        }

        result.dx = best.dx;
        result.dy = best.dy;
        result.attemptedMove = best.dx != 0 || best.dy != 0;
        result.foundCandidate = best.valid;
        result.bestScore = best.score;
        return result;
    }
};
