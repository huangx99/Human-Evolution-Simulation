#pragma once

// HistoryDetectionSystem: runs IHistoryDetector implementations each tick.
//
// This system:
//   - Calls each detector's Scan() method
//   - Detectors read PatternModule, AgentModule, EventBus, GroupKnowledge,
//     CulturalTrace, History, etc.
//   - Detectors write HistoryEvents into HistoryModule
//
// This system does NOT:
//   - Modify agents, fields, cognitive state, or commands
//   - Know about specific history types (fire, death, etc.)
//
// PHASE: SimPhase::History (after Analysis, before Snapshot)

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/history/i_history_detector.h"
#include "sim/history/history_module.h"
#include <vector>
#include <memory>

class HistoryDetectionSystem : public ISystem
{
public:
    void AddDetector(std::unique_ptr<IHistoryDetector> detector)
    {
        detectors_.push_back(std::move(detector));
    }

    void Update(SystemContext& ctx) override
    {
        auto& history = ctx.History();
        for (auto& det : detectors_)
            det->Scan(ctx, history);
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Pattern,        AccessMode::Read},
            {ModuleTag::Agent,          AccessMode::Read},
            {ModuleTag::Event,          AccessMode::Read},
            {ModuleTag::GroupKnowledge, AccessMode::Read},
            {ModuleTag::CulturalTrace,  AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::History, AccessMode::ReadWrite}
        };
        static const char* const DEPS[] = {};
        return {"HistoryDetectionSystem", SimPhase::History, READS, 5, WRITES, 1, DEPS, 0, true, true};
    }

private:
    std::vector<std::unique_ptr<IHistoryDetector>> detectors_;
};
