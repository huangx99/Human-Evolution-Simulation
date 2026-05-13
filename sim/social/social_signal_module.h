#pragma once

// SocialSignalModule: independent module for social signals between entities.
//
// Replaces the old frameSocialSignals buffer in CognitiveModule.
// SocialSignals have TTL-based lifecycle (not frame-level clear).
//
// OWNERSHIP: Engine (sim/social/)
// PHASE: Any system can emit; perception systems read in later phases.

#include "sim/world/module_registry.h"
#include "sim/social/social_signal.h"
#include <vector>
#include <algorithm>
#include <cmath>

struct SocialSignalModule : public IModule
{
    std::vector<SocialSignal> activeSignals;
    u64 nextSignalId = 1;

    // Emit a signal. Returns assigned signal id.
    u64 Emit(const SocialSignal& signal)
    {
        SocialSignal copy = signal;
        copy.id = nextSignalId++;
        activeSignals.push_back(copy);
        return copy.id;
    }

    // Remove signals whose TTL has expired.
    void ClearExpired(Tick now)
    {
        activeSignals.erase(
            std::remove_if(activeSignals.begin(), activeSignals.end(),
                [now](const SocialSignal& s) {
                    return now >= s.createdTick + s.ttl;
                }),
            activeSignals.end());
    }

    // Query signals within radius of a position.
    std::vector<const SocialSignal*> QueryNear(Vec2i pos, f32 radius) const
    {
        std::vector<const SocialSignal*> result;
        f32 r2 = radius * radius;
        for (const auto& s : activeSignals)
        {
            f32 dx = static_cast<f32>(s.origin.x - pos.x);
            f32 dy = static_cast<f32>(s.origin.y - pos.y);
            if (dx * dx + dy * dy <= r2)
                result.push_back(&s);
        }
        return result;
    }

    size_t Count() const { return activeSignals.size(); }

    void Clear()
    {
        activeSignals.clear();
        nextSignalId = 1;
    }
};
