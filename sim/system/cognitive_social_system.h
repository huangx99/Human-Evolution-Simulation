#pragma once

// Phase 2.1 stub.
// Temporarily excluded from all default pipelines during Phase 1.4
// Social Runtime Boundary Cleanup.
//
// Future:
// CognitiveSocialSystem -> ImitationObservationSystem
// It will read Agent actions/outcomes and write SocialSignalModule.
//
// Current pipeline (CreateSystems / CreateCognitiveScheduler) does NOT
// include this system. It will be re-enabled in Phase 2.1.

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"

class CognitiveSocialSystem : public ISystem
{
public:
    void Update(SystemContext&) override
    {
        // Disabled until Phase 2.1.
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Social, AccessMode::Write}
        };
        return {"CognitiveSocialSystem", SimPhase::Action, READS, 1, WRITES, 1, nullptr, 0, true, false};
    }
};
