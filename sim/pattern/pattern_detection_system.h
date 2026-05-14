#pragma once

// PatternDetectionSystem: read-only observer that detects long-term structures.
//
// This system:
//   - Collects observations from world state (agent positions, field values)
//   - Feeds observations to registered detectors
//   - Triggers EndWindow at configured intervals
//
// This system does NOT:
//   - Modify agents, fields, cognitive state, or commands
//   - Influence behavior or decisions
//   - Know about specific world semantics (fire, food, etc.)
//
// PHASE: SimPhase::Analysis

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/pattern/i_pattern_detector.h"
#include "sim/pattern/pattern_config.h"
#include <vector>
#include <memory>

class PatternDetectionSystem : public ISystem
{
public:
    PatternDetectionSystem() = default;

    explicit PatternDetectionSystem(PatternRuntimeConfig config)
        : config_(config) {}

    void AddDetector(std::unique_ptr<IPatternDetector> detector)
    {
        detectors_.push_back(std::move(detector));
    }

    void Update(SystemContext& ctx) override
    {
        Tick tick = ctx.CurrentTick();
        auto& agents = ctx.Agents();
        auto& fm = ctx.GetFieldModule();
        i32 w = ctx.World().Width();
        i32 h = ctx.World().Height();

        // Let detectors read world state directly (e.g. field watchers)
        for (auto& det : detectors_)
            det->ScanWorld(tick, fm, w, h);

        // Collect agent position observations
        u32 obsCount = 0;
        for (const auto& agent : agents.agents)
        {
            if (!agent.alive) continue;
            if (obsCount >= config_.maxObservationsPerTick) break;

            PatternObservation obs;
            obs.tick = tick;
            obs.type = PatternSignalType::AgentPosition;
            obs.entityId = agent.id;
            obs.x = agent.position.x;
            obs.y = agent.position.y;
            obs.value = 1.0f;

            for (auto& det : detectors_)
                det->Observe(obs);

            obsCount++;
        }

        // EndWindow at configured interval
        if (tick > 0 && tick % config_.observationWindowTicks == 0)
        {
            auto& patterns = ctx.Patterns();
            for (auto& det : detectors_)
                det->EndWindow(tick, patterns, config_);

            patterns.Prune(config_.maxPatterns);
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Environment, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Pattern, AccessMode::Write}
        };
        static const char* const DEPS[] = {};
        return {"PatternDetectionSystem", SimPhase::Analysis, READS, 2, WRITES, 1, DEPS, 0, true, true};
    }

private:
    PatternRuntimeConfig config_;
    std::vector<std::unique_ptr<IPatternDetector>> detectors_;
};
