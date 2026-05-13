#pragma once

#include "sim/runtime/rule_pack.h"
#include "sim/scheduler/scheduler.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "rules/human_evolution/commands.h"
#include "rules/human_evolution/environment/climate_system.h"
#include "rules/human_evolution/environment/fire_system.h"
#include "rules/human_evolution/environment/smell_system.h"
#include "rules/human_evolution/systems/agent_perception_system.h"
#include "rules/human_evolution/systems/agent_action_system.h"
#include "rules/human_evolution/systems/cognitive_perception_system.h"
#include "sim/system/cognitive_attention_system.h"
#include "sim/system/cognitive_memory_system.h"
#include "sim/system/cognitive_discovery_system.h"
#include "sim/system/cognitive_knowledge_system.h"
#include "sim/system/social_signal_decay_system.h"
#include "rules/human_evolution/systems/agent_decision_system.h"
#include "sim/pattern/pattern_detection_system.h"
#include "sim/pattern/pattern_registry.h"
#include "sim/pattern/detectors/high_frequency_path_detector.h"
#include "sim/pattern/detectors/stable_field_zone_detector.h"
#include "sim/history/history_detection_system.h"
#include "rules/human_evolution/history/first_stable_fire_detector.h"
#include "rules/human_evolution/history/mass_death_detector.h"
#include "rules/human_evolution/social/human_evolution_social_signal_perception_system.h"
#include "rules/human_evolution/social/human_evolution_social_signal_emission_system.h"

// HumanEvolutionRulePack: defines the Human Evolution world.
//
// Fields: temperature, humidity, fire, wind_x, wind_y, smell, danger, smoke
// Commands: IgniteFire, ExtinguishFire, EmitSmell, SetDanger, EmitSmoke
// Systems: full cognitive pipeline (environment → perception → attention → memory
//          → discovery → knowledge → decision → action)

class HumanEvolutionRulePack : public IRulePack
{
public:
    const char* Name() const override { return "HumanEvolution"; }

    void RegisterFields(FieldModule& fields) override
    {
        ctx_.environment.temperature = fields.RegisterField(FieldKey("human_evolution.temperature"), "temperature", 20.0f);
        ctx_.environment.humidity    = fields.RegisterField(FieldKey("human_evolution.humidity"),    "humidity",    50.0f);
        ctx_.environment.fire        = fields.RegisterField(FieldKey("human_evolution.fire"),        "fire",         0.0f);
        ctx_.environment.windX       = fields.RegisterScalarField(FieldKey("human_evolution.wind_x"), "wind_x", 0.0f);
        ctx_.environment.windY       = fields.RegisterScalarField(FieldKey("human_evolution.wind_y"), "wind_y", 0.0f);
        ctx_.environment.smell       = fields.RegisterField(FieldKey("human_evolution.smell"),       "smell",        0.0f);
        ctx_.environment.danger      = fields.RegisterField(FieldKey("human_evolution.danger"),      "danger",       0.0f);
        ctx_.environment.smoke       = fields.RegisterField(FieldKey("human_evolution.smoke"),       "smoke",        0.0f);
    }

    void RegisterCommands() override
    {
        RegisterHumanEvolutionCommands();
    }

    void RegisterHistoryTypes(HistoryRegistry& registry) override
    {
        ctx_.history.firstStableFireUsage = registry.Register(
            HistoryKey("human_evolution.first_stable_fire_usage"), "first_stable_fire_usage");
        ctx_.history.massDeath = registry.Register(
            HistoryKey("human_evolution.mass_death"), "mass_death");
    }

    void RegisterSocialSignals(SocialSignalRegistry& registry) override
    {
        ctx_.social.fear = registry.Register(
            MakeSocialSignalKey("human_evolution.fear"), "fear");
        ctx_.social.deathWarning = registry.Register(
            MakeSocialSignalKey("human_evolution.death_warning"), "death_warning");
    }

    IRuleContext& GetContext() override { return ctx_; }

    std::vector<SystemRegistration> CreateSystems() override
    {
        std::vector<SystemRegistration> systems;

        // Environment (constructor-injected EnvironmentContext)
        systems.push_back({SimPhase::Environment, std::make_unique<ClimateSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Propagation, std::make_unique<FireSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Propagation, std::make_unique<SmellSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>()});

        // Perception pipeline
        systems.push_back({SimPhase::Perception,  std::make_unique<AgentPerceptionSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Perception,  std::make_unique<CognitivePerceptionSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Perception,  std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(ctx_)});
        systems.push_back({SimPhase::Perception,  std::make_unique<CognitiveAttentionSystem>()});
        systems.push_back({SimPhase::Perception,  std::make_unique<CognitiveMemorySystem>()});

        // Decision pipeline
        systems.push_back({SimPhase::Decision,    std::make_unique<CognitiveDiscoverySystem>()});
        systems.push_back({SimPhase::Decision,    std::make_unique<CognitiveKnowledgeSystem>()});
        systems.push_back({SimPhase::Decision,    std::make_unique<AgentDecisionSystem>()});

        // Action pipeline
        systems.push_back({SimPhase::Action,      std::make_unique<AgentActionSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Action,      std::make_unique<HumanEvolutionSocialSignalEmissionSystem>(ctx_)});

        // Pattern detection (read-only observer)
        {
            // Register Human Evolution pattern types
            PatternRegistry::Instance().Register(
                PatternKey("human_evolution.stable_fire_zone"), "stable_fire_zone");

            auto patternSys = std::make_unique<PatternDetectionSystem>();
            patternSys->AddDetector(std::make_unique<HighFrequencyPathDetector>(10));

            // Watch fire field: stable fire zones where fire > 0.5 for 50+ ticks
            FieldWatchSpec fireWatch;
            fireWatch.patternKey = PatternKey("human_evolution.stable_fire_zone");
            fireWatch.field = ctx_.environment.fire;
            fireWatch.threshold = 0.5f;
            fireWatch.minDuration = 50;
            patternSys->AddDetector(std::make_unique<StableFieldZoneDetector>(
                std::vector<FieldWatchSpec>{fireWatch}));

            systems.push_back({SimPhase::Analysis, std::move(patternSys)});
        }

        // History detection (read-only observer, emits history events)
        {
            auto historySys = std::make_unique<HistoryDetectionSystem>();

            // First stable fire usage: fires once when stable_fire_zone pattern appears
            historySys->AddDetector(std::make_unique<FirstStableFireUsageDetector>(
                HistoryKey("human_evolution.first_stable_fire_usage"),
                PatternKey("human_evolution.stable_fire_zone"),
                0.5f,   // minConfidence
                50      // minObservations
            ));

            // Mass death: cluster of deaths in short window
            MassDeathDetector::Config massDeathCfg;
            massDeathCfg.eventKey = HistoryKey("human_evolution.mass_death");
            massDeathCfg.windowTicks = 10;
            massDeathCfg.threshold = 3;
            massDeathCfg.cooldownTicks = 50;
            historySys->AddDetector(std::make_unique<MassDeathDetector>(std::move(massDeathCfg)));

            systems.push_back({SimPhase::History, std::move(historySys)});
        }

        return systems;
    }

    // Expose context for tests and helpers
    const HumanEvolutionContext& GetHumanEvolutionContext() const { return ctx_; }

private:
    HumanEvolutionContext ctx_;
};

// === Test helpers ===

// Register all HumanEvolution systems (RulePack environment + engine agent).
// Use in tests instead of manual AddSystem calls.
inline void RegisterHumanEvolutionSystems(Scheduler& scheduler, const HumanEvolution::EnvironmentContext& envCtx)
{
    // Environment systems (constructor-injected EnvironmentContext)
    scheduler.AddSystem(SimPhase::Environment, std::make_unique<ClimateSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<FireSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SmellSystem>(envCtx));

    // Agent/cognitive systems (receive EnvironmentContext via constructor)
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Decision,   std::make_unique<AgentDecisionSystem>());
    scheduler.AddSystem(SimPhase::Action,     std::make_unique<AgentActionSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(envCtx));
}

// Create a fully configured HumanEvolution scheduler.
inline Scheduler CreateHumanEvolutionScheduler(const HumanEvolution::EnvironmentContext& envCtx)
{
    Scheduler scheduler;
    RegisterHumanEvolutionSystems(scheduler, envCtx);
    return scheduler;
}

// Create a scheduler without PatternDetectionSystem.
// Used to prove Pattern is a pure observer (doesn't affect world hash).
inline Scheduler CreateHumanEvolutionSchedulerWithoutPattern(
    const HumanEvolution::EnvironmentContext& envCtx)
{
    Scheduler scheduler;
    scheduler.AddSystem(SimPhase::Environment, std::make_unique<ClimateSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<FireSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SmellSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Decision,   std::make_unique<AgentDecisionSystem>());
    scheduler.AddSystem(SimPhase::Action,     std::make_unique<AgentActionSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(envCtx));
    return scheduler;
}

// Create a scheduler with the full cognitive pipeline.
// Extends the base systems with attention, memory, discovery, and knowledge.
inline Scheduler CreateCognitiveScheduler(const HumanEvolution::EnvironmentContext& envCtx)
{
    Scheduler scheduler;

    // Environment systems (constructor-injected EnvironmentContext)
    scheduler.AddSystem(SimPhase::Environment, std::make_unique<ClimateSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<FireSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SmellSystem>(envCtx));

    // Perception pipeline
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<AgentPerceptionSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<CognitivePerceptionSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<CognitiveMemorySystem>());

    // Decision pipeline
    scheduler.AddSystem(SimPhase::Decision,    std::make_unique<CognitiveDiscoverySystem>());
    scheduler.AddSystem(SimPhase::Decision,    std::make_unique<CognitiveKnowledgeSystem>());
    scheduler.AddSystem(SimPhase::Decision,    std::make_unique<AgentDecisionSystem>());

    // Action pipeline
    scheduler.AddSystem(SimPhase::Action,      std::make_unique<AgentActionSystem>(envCtx));

    return scheduler;
}
