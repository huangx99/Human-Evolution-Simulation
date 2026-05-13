#pragma once

#include "sim/world/world_state.h"
#include "sim/system/field_writer.h"
#include "sim/scheduler/system_descriptor.h"
#include "sim/history/history_module.h"
#include "sim/social/social_signal_module.h"
#include "sim/cognitive/internal_state_baseline_module.h"

// SystemContext: controlled access wrapper passed to every ISystem::Update().
//
// Systems access fields via FieldModule (read) and FieldWriter (write).
// World-specific context (field indices, config) is injected via constructor.

class SystemContext
{
public:
    SystemContext(WorldState& world, const SystemDescriptor* desc = nullptr)
        : world_(world)
        , fields_(world.Fields())
        , descriptor_(desc)
    {}

    // Field read access (via FieldIndex)
    const ::FieldModule& GetFieldModule() const { return world_.Fields(); }

    // Field write access (via FieldIndex)
    FieldWriter& Fields() { return fields_; }

    // Engine module accessors
    const SimulationModule&  Sim()       const { return world_.Sim(); }
    const AgentModule&       Agents()    const { return world_.Agents(); }
          AgentModule&       Agents()          { return world_.Agents(); }
    const EcologyModule&     Ecology()   const { return world_.Ecology(); }
    const CognitiveModule&   Cognitive() const { return world_.Cognitive(); }
          CognitiveModule&   Cognitive()       { return world_.Cognitive(); }
    const PatternModule&     Patterns()  const { return world_.Patterns(); }
          PatternModule&     Patterns()        { return world_.Patterns(); }
    const HistoryModule&     History()   const { return world_.History(); }
          HistoryModule&     History()         { return world_.History(); }
    const SocialSignalModule& SocialSignals() const { return world_.SocialSignals(); }
          SocialSignalModule& SocialSignals()       { return world_.SocialSignals(); }
    const InternalStateBaselineModule& InternalStateBaselines() const { return world_.InternalStateBaselines(); }
          InternalStateBaselineModule& InternalStateBaselines()       { return world_.InternalStateBaselines(); }

    // Commands and Events
    CommandBuffer& Commands() { return world_.commands; }
    EventBus&      Events()   { return world_.events; }

    // Convenience
    Tick CurrentTick() const { return world_.Sim().clock.currentTick; }
    const SystemDescriptor* Descriptor() const { return descriptor_; }

    // Legacy escape hatch
    WorldState& World() { return world_; }

private:
    WorldState& world_;
    FieldWriter fields_;
    const SystemDescriptor* descriptor_;
};
