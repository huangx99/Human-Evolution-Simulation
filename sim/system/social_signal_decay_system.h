#pragma once

// SocialSignalDecaySystem: removes expired social signals each tick.
//
// Runs in Propagation phase (before Perception) so that expired signals
// are cleaned up before any system tries to perceive them.
//
// OWNERSHIP: Engine (sim/system/)
// READS: Simulation (tick)
// WRITES: Social (ClearExpired)
// PHASE: SimPhase::Propagation

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"

class SocialSignalDecaySystem : public ISystem
{
public:
    void Update(SystemContext& ctx) override
    {
        ctx.SocialSignals().ClearExpired(ctx.CurrentTick());
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Social, AccessMode::Write}
        };
        return {"SocialSignalDecaySystem", SimPhase::Propagation, READS, 1, WRITES, 1, nullptr, 0, true, false};
    }
};
