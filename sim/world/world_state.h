#pragma once

#include "sim/world/module_registry.h"
#include "sim/world/simulation_module.h"
#include "sim/field/field_module.h"
#include "sim/world/agent_module.h"
#include "sim/world/ecology_module.h"
#include "sim/cognitive/cognitive_module.h"
#include "sim/pattern/pattern_module.h"
#include "sim/pattern/pattern_registry.h"
#include "sim/history/history_module.h"
#include "sim/history/history_registry.h"
#include "sim/social/social_signal_module.h"
#include "sim/social/social_signal_registry.h"
#include "sim/event/event_bus.h"
#include "sim/command/command_buffer.h"
#include "sim/spatial/spatial_index.h"
#include "sim/runtime/rule_pack.h"

struct WorldState
{
    ModuleRegistry modules;
    EventBus events;
    CommandBuffer commands;
    SpatialIndex spatial;

    i32 width;
    i32 height;
    u64 lastTickHash = 0;

    // Primary constructor: engine modules + RulePack-driven fields/systems
    WorldState(i32 w, i32 h, u64 seed, IRulePack& rulePack)
        : width(w), height(h), spatial(w, h)
    {
        PatternTypes::RegisterCoreTypes();
        modules.Register<SimulationModule>(seed);
        modules.Register<FieldModule>(w, h);

        // RulePack registers its fields into FieldModule
        auto& fm = modules.Get<FieldModule>();
        rulePack.RegisterFields(fm);

        // RulePack registers its command factories
        rulePack.RegisterCommands();

        // RulePack registers its history event types
        rulePack.RegisterHistoryTypes(HistoryRegistry::Instance());

        // RulePack registers its social signal types
        rulePack.RegisterSocialSignals(SocialSignalRegistry::Instance());

        // Store RulePack context for snapshot/replay access
        ruleContext_ = &rulePack.GetContext();

        modules.Register<AgentModule>();
        modules.Register<EcologyModule>();
        modules.Register<CognitiveModule>();
        modules.Register<PatternModule>();
        modules.Register<HistoryModule>();
        modules.Register<SocialSignalModule>();
    }

    // Convenience constructor: no RulePack (for engine-only tests).
    WorldState(i32 w, i32 h, u64 seed)
        : width(w), height(h), spatial(w, h)
    {
        PatternTypes::RegisterCoreTypes();
        modules.Register<SimulationModule>(seed);
        modules.Register<FieldModule>(w, h);

        modules.Register<AgentModule>();
        modules.Register<EcologyModule>();
        modules.Register<CognitiveModule>();
        modules.Register<PatternModule>();
        modules.Register<HistoryModule>();
        modules.Register<SocialSignalModule>();
    }

    // Deferred RulePack init: register fields.
    // Use when WorldState was constructed without a RulePack.
    void Init(IRulePack& rulePack)
    {
        auto& fm = modules.Get<FieldModule>();
        rulePack.RegisterFields(fm);
        rulePack.RegisterCommands();
        rulePack.RegisterHistoryTypes(HistoryRegistry::Instance());
        rulePack.RegisterSocialSignals(SocialSignalRegistry::Instance());
        ruleContext_ = &rulePack.GetContext();
    }

    void RebuildSpatial()
    {
        spatial.Rebuild(Ecology().entities);
    }

    SimulationModule& Sim() { return modules.Get<SimulationModule>(); }
    FieldModule& Fields() { return modules.Get<FieldModule>(); }
    AgentModule& Agents() { return modules.Get<AgentModule>(); }
    EcologyModule& Ecology() { return modules.Get<EcologyModule>(); }
    CognitiveModule& Cognitive() { return modules.Get<CognitiveModule>(); }
    PatternModule& Patterns() { return modules.Get<PatternModule>(); }
    HistoryModule& History() { return modules.Get<HistoryModule>(); }
    SocialSignalModule& SocialSignals() { return modules.Get<SocialSignalModule>(); }

    const SimulationModule& Sim() const { return modules.Get<SimulationModule>(); }
    const FieldModule& Fields() const { return modules.Get<FieldModule>(); }
    const AgentModule& Agents() const { return modules.Get<AgentModule>(); }
    const EcologyModule& Ecology() const { return modules.Get<EcologyModule>(); }
    const CognitiveModule& Cognitive() const { return modules.Get<CognitiveModule>(); }
    const PatternModule& Patterns() const { return modules.Get<PatternModule>(); }
    const HistoryModule& History() const { return modules.Get<HistoryModule>(); }
    const SocialSignalModule& SocialSignals() const { return modules.Get<SocialSignalModule>(); }

    // RulePack context access (for snapshot/replay)
    IRuleContext* RuleContext() { return ruleContext_; }
    const IRuleContext* RuleContext() const { return ruleContext_; }

    i32 Width() const { return width; }
    i32 Height() const { return height; }

    EntityId SpawnAgent(i32 x, i32 y)
    {
        return Agents().Spawn(x, y);
    }

private:
    IRuleContext* ruleContext_ = nullptr;
};
