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
#include "rules/human_evolution/systems/human_evolution_memory_inference_policy.h"
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
#include "rules/human_evolution/history/social_history_detector.h"
#include "rules/human_evolution/social/human_evolution_social_signal_perception_system.h"
#include "rules/human_evolution/social/human_evolution_social_signal_emission_system.h"
#include "rules/human_evolution/social/human_evolution_imitation_observation_system.h"
#include "rules/human_evolution/systems/internal_state_stimulus_system.h"
#include "rules/human_evolution/systems/group_knowledge_aggregation_system.h"
#include "rules/human_evolution/systems/collective_avoidance_system.h"
#include "rules/human_evolution/systems/cultural_trace_detection_system.h"
#include "rules/human_evolution/systems/group_knowledge_awareness_system.h"

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
        ctx_.history.firstSharedDangerMemory = registry.Register(
            HistoryKey("human_evolution.first_shared_danger_memory"), "first_shared_danger_memory");
        ctx_.history.firstCollectiveAvoidance = registry.Register(
            HistoryKey("human_evolution.first_collective_avoidance"), "first_collective_avoidance");
        ctx_.history.firstDangerAvoidanceTrace = registry.Register(
            HistoryKey("human_evolution.first_danger_avoidance_trace"), "first_danger_avoidance_trace");
    }

    void RegisterSocialSignals(SocialSignalRegistry& registry) override
    {
        ctx_.social.fear = registry.Register(
            MakeSocialSignalKey("human_evolution.fear"), "fear");
        ctx_.social.deathWarning = registry.Register(
            MakeSocialSignalKey("human_evolution.death_warning"), "death_warning");
    }

    void RegisterObservedActions(ObservedActionRegistry& registry) override
    {
        ctx_.observedActions.observedFlee = registry.Register(
            MakeObservedActionKey("human_evolution.observed_flee"), "observed_flee");
    }

    void RegisterGroupKnowledgeTypes(GroupKnowledgeRegistry& registry) override
    {
        ctx_.groupKnowledge.sharedDangerZone = registry.Register(
            MakeGroupKnowledgeKey("human_evolution.shared_danger_zone"), "shared_danger_zone");
        ctx_.groupKnowledge.sharedFireBenefit = registry.Register(
            MakeGroupKnowledgeKey("human_evolution.shared_fire_benefit"), "shared_fire_benefit");
    }

    void RegisterConcepts(ConceptTypeRegistry& registry) override
    {
        using F = ConceptSemanticFlag;
        auto f = [](ConceptSemanticFlag a, ConceptSemanticFlag b, ConceptSemanticFlag c) {
            return static_cast<u32>(a | b | c);
        };
        auto f2 = [](ConceptSemanticFlag a, ConceptSemanticFlag b) {
            return static_cast<u32>(a | b);
        };
        constexpr u32 none = 0;

        // Natural phenomena
        ctx_.concepts.fire      = registry.Register(MakeConceptKey("human_evolution.fire"),      "fire",      f(F::Danger, F::Thermal, F::Environmental));
        ctx_.concepts.water     = registry.Register(MakeConceptKey("human_evolution.water"),     "water",     f(F::Environmental, F::Organic, F::Resource));
        ctx_.concepts.wind      = registry.Register(MakeConceptKey("human_evolution.wind"),      "wind",      static_cast<u32>(F::Environmental));
        ctx_.concepts.rain      = registry.Register(MakeConceptKey("human_evolution.rain"),      "rain",      f(F::Environmental, F::Negative, F::Thermal));
        ctx_.concepts.lightning = registry.Register(MakeConceptKey("human_evolution.lightning"), "lightning", f(F::Environmental, F::Danger, F::Thermal));
        ctx_.concepts.darkness  = registry.Register(MakeConceptKey("human_evolution.darkness"),  "darkness",  f2(F::Environmental, F::Danger));
        ctx_.concepts.light     = registry.Register(MakeConceptKey("human_evolution.light"),     "light",     f2(F::Environmental, F::Positive));
        ctx_.concepts.smoke     = registry.Register(MakeConceptKey("human_evolution.smoke"),     "smoke",     f(F::Environmental, F::Danger, F::Thermal));
        ctx_.concepts.ice       = registry.Register(MakeConceptKey("human_evolution.ice"),       "ice",       f(F::Environmental, F::Thermal, F::Negative));
        ctx_.concepts.heat      = registry.Register(MakeConceptKey("human_evolution.heat"),      "heat",      f(F::Environmental, F::Thermal, F::Danger));
        ctx_.concepts.cold      = registry.Register(MakeConceptKey("human_evolution.cold"),      "cold",      f(F::Environmental, F::Thermal, F::Danger));

        // Materials
        ctx_.concepts.wood  = registry.Register(MakeConceptKey("human_evolution.wood"),  "wood",  f2(F::Organic, F::Resource));
        ctx_.concepts.stone = registry.Register(MakeConceptKey("human_evolution.stone"), "stone", static_cast<u32>(F::Resource));
        ctx_.concepts.grass = registry.Register(MakeConceptKey("human_evolution.grass"), "grass", f2(F::Organic, F::Resource));
        ctx_.concepts.meat  = registry.Register(MakeConceptKey("human_evolution.meat"),  "meat",  f(F::Organic, F::Resource, F::Internal));
        ctx_.concepts.bone  = registry.Register(MakeConceptKey("human_evolution.bone"),  "bone",  f2(F::Organic, F::Resource));
        ctx_.concepts.ash   = registry.Register(MakeConceptKey("human_evolution.ash"),   "ash",   none);
        ctx_.concepts.coal  = registry.Register(MakeConceptKey("human_evolution.coal"),  "coal",  f2(F::Thermal, F::Resource));
        ctx_.concepts.fruit = registry.Register(MakeConceptKey("human_evolution.fruit"), "fruit", f(F::Organic, F::Resource, F::Positive));

        // States / conditions
        ctx_.concepts.warmth  = registry.Register(MakeConceptKey("human_evolution.warmth"),  "warmth",  f2(F::Positive, F::Thermal));
        ctx_.concepts.wetness = registry.Register(MakeConceptKey("human_evolution.wetness"), "wetness", static_cast<u32>(F::Environmental));
        ctx_.concepts.dryness = registry.Register(MakeConceptKey("human_evolution.dryness"), "dryness", static_cast<u32>(F::Environmental));
        ctx_.concepts.hunger  = registry.Register(MakeConceptKey("human_evolution.hunger"),  "hunger",  f(F::Internal, F::Negative, F::Danger));
        ctx_.concepts.pain    = registry.Register(MakeConceptKey("human_evolution.pain"),    "pain",    f(F::Internal, F::Negative, F::Danger));
        ctx_.concepts.satiety = registry.Register(MakeConceptKey("human_evolution.satiety"), "satiety", f2(F::Internal, F::Positive));
        ctx_.concepts.health  = registry.Register(MakeConceptKey("human_evolution.health"),  "health",  f2(F::Internal, F::Positive));
        ctx_.concepts.death   = registry.Register(MakeConceptKey("human_evolution.death"),   "death",   f(F::Danger, F::Negative, F::Abstract));

        // Danger
        ctx_.concepts.danger    = registry.Register(MakeConceptKey("human_evolution.danger"),    "danger",    f2(F::Danger, F::Abstract));
        ctx_.concepts.beast     = registry.Register(MakeConceptKey("human_evolution.beast"),     "beast",     f(F::Danger, F::Threat, F::Organic));
        ctx_.concepts.predator  = registry.Register(MakeConceptKey("human_evolution.predator"),  "predator",  f(F::Danger, F::Threat, F::Organic));
        ctx_.concepts.fall      = registry.Register(MakeConceptKey("human_evolution.fall"),      "fall",      f2(F::Danger, F::Threat));
        ctx_.concepts.drowning  = registry.Register(MakeConceptKey("human_evolution.drowning"),  "drowning",  f(F::Danger, F::Threat, F::Negative));
        ctx_.concepts.burning   = registry.Register(MakeConceptKey("human_evolution.burning"),   "burning",   f(F::Danger, F::Thermal, F::Negative));

        // Opportunities
        ctx_.concepts.food    = registry.Register(MakeConceptKey("human_evolution.food"),    "food",    f(F::Resource, F::Organic, F::Positive));
        ctx_.concepts.shelter = registry.Register(MakeConceptKey("human_evolution.shelter"), "shelter", f2(F::Resource, F::Abstract));
        ctx_.concepts.tool    = registry.Register(MakeConceptKey("human_evolution.tool"),    "tool",    f2(F::Resource, F::Abstract));
        ctx_.concepts.path    = registry.Register(MakeConceptKey("human_evolution.path"),    "path",    f2(F::Environmental, F::Abstract));

        // Social
        ctx_.concepts.companion    = registry.Register(MakeConceptKey("human_evolution.companion"),    "companion",    f2(F::Social, F::Positive));
        ctx_.concepts.stranger     = registry.Register(MakeConceptKey("human_evolution.stranger"),     "stranger",     f(F::Social, F::Danger, F::Abstract));
        ctx_.concepts.group        = registry.Register(MakeConceptKey("human_evolution.group"),        "group",        f2(F::Social, F::Abstract));
        ctx_.concepts.signal       = registry.Register(MakeConceptKey("human_evolution.signal"),       "signal",       f2(F::Social, F::Abstract));
        ctx_.concepts.voice        = registry.Register(MakeConceptKey("human_evolution.voice"),        "voice",        f2(F::Social, F::Abstract));
        ctx_.concepts.observedFlee = registry.Register(MakeConceptKey("human_evolution.observed_flee"), "observed_flee", f(F::Social, F::Danger, F::Abstract));

        // Abstract
        ctx_.concepts.safety    = registry.Register(MakeConceptKey("human_evolution.safety"),    "safety",    f2(F::Abstract, F::Positive));
        ctx_.concepts.comfort   = registry.Register(MakeConceptKey("human_evolution.comfort"),   "comfort",   f(F::Abstract, F::Positive, F::Thermal));
        ctx_.concepts.fear      = registry.Register(MakeConceptKey("human_evolution.fear"),      "fear",      f(F::Abstract, F::Danger, F::Negative));
        ctx_.concepts.curiosity = registry.Register(MakeConceptKey("human_evolution.curiosity"), "curiosity", f2(F::Abstract, F::Positive));
        ctx_.concepts.trust     = registry.Register(MakeConceptKey("human_evolution.trust"),     "trust",     f(F::Abstract, F::Social, F::Positive));

        // Group knowledge awareness
        ctx_.concepts.groupDangerEvidence = registry.Register(
            MakeConceptKey("human_evolution.group_danger_evidence"),
            "group_danger_evidence",
            static_cast<u32>(F::Danger | F::Social | F::Abstract));
    }

    void RegisterPatternTypes(PatternRegistry& registry) override
    {
        ctx_.socialPatterns.collectiveAvoidance = registry.Register(
            PatternKey("human_evolution.collective_avoidance"), "collective_avoidance");
        registry.Register(
            PatternKey("human_evolution.stable_fire_zone"), "stable_fire_zone");
    }

    void RegisterCulturalTraceTypes(CulturalTraceRegistry& registry) override
    {
        ctx_.culturalTraces.dangerAvoidanceTrace = registry.Register(
            MakeCulturalTraceKey("human_evolution.danger_avoidance_trace"), "danger_avoidance_trace");
    }

    IRuleContext& GetContext() override { return ctx_; }

    std::vector<SystemRegistration> CreateSystems() override
    {
        memoryPolicy_ = std::make_unique<HumanEvolutionMemoryInferencePolicy>(ctx_.concepts);
        std::vector<SystemRegistration> systems;

        // Environment (constructor-injected EnvironmentContext)
        systems.push_back({SimPhase::Environment, std::make_unique<ClimateSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Propagation, std::make_unique<FireSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Propagation, std::make_unique<SmellSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>()});

        // Perception pipeline
        systems.push_back({SimPhase::Perception,  std::make_unique<AgentPerceptionSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Perception,  std::make_unique<CognitivePerceptionSystem>(ctx_)});
        systems.push_back({SimPhase::Perception,  std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(ctx_)});
        systems.push_back({SimPhase::Perception,  std::make_unique<HumanEvolutionImitationObservationSystem>(ctx_)});
        systems.push_back({SimPhase::Perception,  std::make_unique<InternalStateStimulusSystem>(ctx_.concepts)});
        systems.push_back({SimPhase::Perception,  std::make_unique<GroupKnowledgeAwarenessSystem>(
            ctx_.concepts.groupDangerEvidence, ctx_.groupKnowledge.sharedDangerZone)});
        systems.push_back({SimPhase::Perception,  std::make_unique<CognitiveAttentionSystem>()});
        systems.push_back({SimPhase::Perception,  std::make_unique<CognitiveMemorySystem>(memoryPolicy_.get())});

        // Decision pipeline
        systems.push_back({SimPhase::Decision,    std::make_unique<CognitiveDiscoverySystem>(BuildDiscoveryRules())});
        systems.push_back({SimPhase::Decision,    std::make_unique<CognitiveKnowledgeSystem>()});
        systems.push_back({SimPhase::Decision,    std::make_unique<AgentDecisionSystem>(ctx_.concepts)});

        // Action pipeline
        systems.push_back({SimPhase::Action,      std::make_unique<AgentActionSystem>(ctx_.environment)});
        systems.push_back({SimPhase::Action,      std::make_unique<HumanEvolutionSocialSignalEmissionSystem>(ctx_)});

        // Group knowledge aggregation (after memory, before pattern)
        systems.push_back({SimPhase::Analysis, std::make_unique<GroupKnowledgeAggregationSystem>(
            ctx_.concepts, ctx_.groupKnowledge.sharedDangerZone)});

        // Collective avoidance detection (reads GroupKnowledge, writes Pattern)
        systems.push_back({SimPhase::Analysis, std::make_unique<CollectiveAvoidanceSystem>(
            ctx_.socialPatterns.collectiveAvoidance,
            PatternKey("human_evolution.collective_avoidance"),
            ctx_.groupKnowledge.sharedDangerZone)});

        // Cultural trace detection (reads GroupKnowledge + Pattern, writes CulturalTrace)
        systems.push_back({SimPhase::Analysis, std::make_unique<CulturalTraceDetectionSystem>(
            ctx_.culturalTraces.dangerAvoidanceTrace,
            PatternKey("human_evolution.collective_avoidance"),
            ctx_.groupKnowledge.sharedDangerZone)});

        // Pattern detection (read-only observer)
        {
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

            // Social emergence milestones (fires once per type)
            historySys->AddDetector(std::make_unique<SocialHistoryDetector>(
                HistoryKey("human_evolution.first_shared_danger_memory"),
                HistoryKey("human_evolution.first_collective_avoidance"),
                HistoryKey("human_evolution.first_danger_avoidance_trace"),
                ctx_.groupKnowledge.sharedDangerZone,
                PatternKey("human_evolution.collective_avoidance"),
                ctx_.culturalTraces.dangerAvoidanceTrace));

            systems.push_back({SimPhase::History, std::move(historySys)});
        }

        return systems;
    }

    // Expose context for tests and helpers
    const HumanEvolutionContext& GetHumanEvolutionContext() const { return ctx_; }

private:
    HumanEvolutionContext ctx_;
    std::unique_ptr<HumanEvolutionMemoryInferencePolicy> memoryPolicy_;

    std::vector<DiscoveryRule> BuildDiscoveryRules() const
    {
        const auto& c = ctx_.concepts;
        return {
            // Smoke → Signals → Fire
            {c.smoke,     c.fire,      KnowledgeRelation::Signals},
            // Fire → Causes → Pain
            {c.fire,      c.pain,      KnowledgeRelation::Causes},
            // Fire → Causes → Warmth
            {c.fire,      c.warmth,    KnowledgeRelation::Causes},
            // Heat → Causes → Comfort
            {c.heat,      c.comfort,   KnowledgeRelation::Causes},
            // Cold → Causes → Pain
            {c.cold,      c.pain,      KnowledgeRelation::Causes},
            // Meat → Causes → Satiety
            {c.meat,      c.satiety,   KnowledgeRelation::Causes},
            // Fruit → Causes → Satiety
            {c.fruit,     c.satiety,   KnowledgeRelation::Causes},
            // Food → Causes → Satiety
            {c.food,      c.satiety,   KnowledgeRelation::Causes},
            // Danger → Signals → Fear
            {c.danger,    c.fear,      KnowledgeRelation::Signals},
            // Beast → Signals → Danger
            {c.beast,     c.danger,    KnowledgeRelation::Signals},
            // Predator → Signals → Danger
            {c.predator,  c.danger,    KnowledgeRelation::Signals},
            // Burning → Causes → Pain
            {c.burning,   c.pain,      KnowledgeRelation::Causes},
            // Drowning → Causes → Death
            {c.drowning,  c.death,     KnowledgeRelation::Causes},
        };
    }
};

// === Test helpers ===

// Register base HumanEvolution systems (environment + agent + cognitive perception).
// Does NOT include Social, Attention, Memory, Discovery, Knowledge, Pattern, History.
// Use for tests that only need the minimal pipeline.
inline void RegisterHumanEvolutionSystems(Scheduler& scheduler, const HumanEvolution::EnvironmentContext& envCtx)
{
    scheduler.AddSystem(SimPhase::Environment, std::make_unique<ClimateSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<FireSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SmellSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<AgentPerceptionSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Decision,   std::make_unique<AgentDecisionSystem>());
    scheduler.AddSystem(SimPhase::Action,     std::make_unique<AgentActionSystem>(envCtx));
    scheduler.AddSystem(SimPhase::Perception, std::make_unique<CognitivePerceptionSystem>(envCtx));
}

// Create a base scheduler (environment + agent + cognitive perception).
// Does NOT include Social, Attention, Memory, Pattern, History.
inline Scheduler CreateHumanEvolutionScheduler(const HumanEvolution::EnvironmentContext& envCtx)
{
    Scheduler scheduler;
    RegisterHumanEvolutionSystems(scheduler, envCtx);
    return scheduler;
}

// Create a base scheduler without PatternDetectionSystem.
// Used to prove Pattern is a pure observer (doesn't affect world hash).
inline Scheduler CreateHumanEvolutionSchedulerWithoutPattern(
    const HumanEvolution::EnvironmentContext& envCtx)
{
    return CreateHumanEvolutionScheduler(envCtx);
}

// Create a scheduler with the full cognitive pipeline (no Social).
// Includes attention, memory, discovery, knowledge.
// Use for tests that need the cognitive loop but not social signals.
inline Scheduler CreateCognitiveScheduler(const HumanEvolutionContext& ctx)
{
    auto memoryPolicy = std::make_shared<HumanEvolutionMemoryInferencePolicy>(ctx.concepts);
    auto* memoryPolicyPtr = memoryPolicy.get();

    const auto& c = ctx.concepts;
    std::vector<DiscoveryRule> discoveryRules = {
        {c.smoke, c.fire, KnowledgeRelation::Signals},
        {c.fire, c.pain, KnowledgeRelation::Causes},
        {c.fire, c.warmth, KnowledgeRelation::Causes},
        {c.heat, c.comfort, KnowledgeRelation::Causes},
        {c.cold, c.pain, KnowledgeRelation::Causes},
        {c.meat, c.satiety, KnowledgeRelation::Causes},
        {c.fruit, c.satiety, KnowledgeRelation::Causes},
        {c.food, c.satiety, KnowledgeRelation::Causes},
        {c.danger, c.fear, KnowledgeRelation::Signals},
        {c.beast, c.danger, KnowledgeRelation::Signals},
        {c.predator, c.danger, KnowledgeRelation::Signals},
        {c.burning, c.pain, KnowledgeRelation::Causes},
        {c.drowning, c.death, KnowledgeRelation::Causes},
    };

    Scheduler scheduler;
    scheduler.SetUserData(std::static_pointer_cast<void>(memoryPolicy));

    scheduler.AddSystem(SimPhase::Environment, std::make_unique<ClimateSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<FireSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SmellSystem>(ctx.environment));

    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<AgentPerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<CognitivePerceptionSystem>(ctx));
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<CognitiveMemorySystem>(memoryPolicyPtr));

    scheduler.AddSystem(SimPhase::Decision,    std::make_unique<CognitiveDiscoverySystem>(std::move(discoveryRules)));
    scheduler.AddSystem(SimPhase::Decision,    std::make_unique<CognitiveKnowledgeSystem>());
    scheduler.AddSystem(SimPhase::Decision,    std::make_unique<AgentDecisionSystem>(ctx.concepts));

    scheduler.AddSystem(SimPhase::Action,      std::make_unique<AgentActionSystem>(ctx.environment));

    return scheduler;
}

// Create a scheduler with the full pipeline including Social signals.
// Includes everything: environment, cognitive, social decay/perception/emission.
// Use for tests that need the complete Phase 2.0 pipeline.
inline Scheduler CreateFullSocialScheduler(const HumanEvolutionContext& ctx)
{
    auto memoryPolicy = std::make_shared<HumanEvolutionMemoryInferencePolicy>(ctx.concepts);
    auto* memoryPolicyPtr = memoryPolicy.get();

    const auto& c = ctx.concepts;
    std::vector<DiscoveryRule> discoveryRules = {
        {c.smoke, c.fire, KnowledgeRelation::Signals},
        {c.fire, c.pain, KnowledgeRelation::Causes},
        {c.fire, c.warmth, KnowledgeRelation::Causes},
        {c.heat, c.comfort, KnowledgeRelation::Causes},
        {c.cold, c.pain, KnowledgeRelation::Causes},
        {c.meat, c.satiety, KnowledgeRelation::Causes},
        {c.fruit, c.satiety, KnowledgeRelation::Causes},
        {c.food, c.satiety, KnowledgeRelation::Causes},
        {c.danger, c.fear, KnowledgeRelation::Signals},
        {c.beast, c.danger, KnowledgeRelation::Signals},
        {c.predator, c.danger, KnowledgeRelation::Signals},
        {c.burning, c.pain, KnowledgeRelation::Causes},
        {c.drowning, c.death, KnowledgeRelation::Causes},
    };

    Scheduler scheduler;
    scheduler.SetUserData(std::static_pointer_cast<void>(memoryPolicy));

    scheduler.AddSystem(SimPhase::Environment, std::make_unique<ClimateSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<FireSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SmellSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Propagation, std::make_unique<SocialSignalDecaySystem>());

    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<AgentPerceptionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<CognitivePerceptionSystem>(ctx));
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<HumanEvolutionSocialSignalPerceptionSystem>(ctx));
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<HumanEvolutionImitationObservationSystem>(ctx));
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<InternalStateStimulusSystem>(ctx.concepts));
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<GroupKnowledgeAwarenessSystem>(
        ctx.concepts.groupDangerEvidence, ctx.groupKnowledge.sharedDangerZone));
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<CognitiveAttentionSystem>());
    scheduler.AddSystem(SimPhase::Perception,  std::make_unique<CognitiveMemorySystem>(memoryPolicyPtr));

    scheduler.AddSystem(SimPhase::Decision,    std::make_unique<CognitiveDiscoverySystem>(std::move(discoveryRules)));
    scheduler.AddSystem(SimPhase::Decision,    std::make_unique<CognitiveKnowledgeSystem>());
    scheduler.AddSystem(SimPhase::Decision,    std::make_unique<AgentDecisionSystem>(ctx.concepts));

    scheduler.AddSystem(SimPhase::Action,      std::make_unique<AgentActionSystem>(ctx.environment));
    scheduler.AddSystem(SimPhase::Action,      std::make_unique<HumanEvolutionSocialSignalEmissionSystem>(ctx));

    scheduler.AddSystem(SimPhase::Analysis,    std::make_unique<GroupKnowledgeAggregationSystem>(
        ctx.concepts, ctx.groupKnowledge.sharedDangerZone));

    scheduler.AddSystem(SimPhase::Analysis,    std::make_unique<CollectiveAvoidanceSystem>(
        ctx.socialPatterns.collectiveAvoidance,
        PatternKey("human_evolution.collective_avoidance"),
        ctx.groupKnowledge.sharedDangerZone));

    scheduler.AddSystem(SimPhase::Analysis,    std::make_unique<CulturalTraceDetectionSystem>(
        ctx.culturalTraces.dangerAvoidanceTrace,
        PatternKey("human_evolution.collective_avoidance"),
        ctx.groupKnowledge.sharedDangerZone));

    {
        auto historySys = std::make_unique<HistoryDetectionSystem>();
        historySys->AddDetector(std::make_unique<SocialHistoryDetector>(
            HistoryKey("human_evolution.first_shared_danger_memory"),
            HistoryKey("human_evolution.first_collective_avoidance"),
            HistoryKey("human_evolution.first_danger_avoidance_trace"),
            ctx.groupKnowledge.sharedDangerZone,
            PatternKey("human_evolution.collective_avoidance"),
            ctx.culturalTraces.dangerAvoidanceTrace));
        scheduler.AddSystem(SimPhase::History, std::move(historySys));
    }

    return scheduler;
}
