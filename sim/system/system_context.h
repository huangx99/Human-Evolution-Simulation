#pragma once

#include "sim/world/world_state.h"
#include "sim/system/field_writer.h"
#include "sim/scheduler/system_descriptor.h"

// SystemContext: controlled access wrapper passed to every ISystem::Update().
//
// Phase 1.1-b: Fields() returns FieldWriter (write via FieldIndex).
//              FieldModule() returns const FieldModule& (read via FieldIndex).
//              Env()/Info() kept for non-field data (width, height, wind).

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

    // Legacy module accessors (for non-field data: width, height, wind, etc.)
    const EnvironmentModule& Env()       const { return world_.Env(); }
    const InformationModule& Info()      const { return world_.Info(); }
    const SimulationModule&  Sim()       const { return world_.Sim(); }
    const AgentModule&       Agents()    const { return world_.Agents(); }
    const EcologyModule&     Ecology()   const { return world_.Ecology(); }
    const CognitiveModule&   Cognitive() const { return world_.Cognitive(); }

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
