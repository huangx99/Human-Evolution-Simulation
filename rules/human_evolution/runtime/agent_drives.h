#pragma once

// AgentDrives: physiological and psychological urgency assessment.
//
// Drives are independent of each other and normalized to 0..1.
// Hunger does NOT require smell to generate urgency.

#include "core/types/types.h"
#include "sim/entity/agent.h"
#include "rules/human_evolution/runtime/cell_risk.h"

struct AgentDrives
{
    f32 hungerUrgency   = 0.0f;
    f32 dangerUrgency   = 0.0f;
    f32 painUrgency     = 0.0f;
    f32 fatigueUrgency  = 0.0f;
    f32 curiosityUrgency = 0.0f;
};

struct DriveEvaluator
{
    // Smooth hermite interpolation: 0 below start, 1 above end, smooth ramp between.
    static f32 SmoothUrgency(f32 value, f32 start, f32 end)
    {
        if (value <= start)
            return 0.0f;
        if (value >= end)
            return 1.0f;

        const f32 x = (value - start) / (end - start);
        return x * x * (3.0f - 2.0f * x);
    }

    static AgentDrives Evaluate(const Agent& agent, const CellRisk& currentRisk)
    {
        AgentDrives d;

        d.hungerUrgency = SmoothUrgency(agent.hunger, 35.0f, 95.0f);
        d.dangerUrgency = currentRisk.total;

        if (agent.health < 100.0f)
            d.painUrgency = SmoothUrgency(100.0f - agent.health, 5.0f, 50.0f);

        d.curiosityUrgency = 0.15f;

        return d;
    }
};
