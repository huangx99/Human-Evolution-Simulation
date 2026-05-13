#pragma once

#include "sim/scheduler/phase.h"
#include "sim/system/i_system.h"
#include "sim/field/field_module.h"
#include <vector>
#include <memory>

// SystemRegistration: a system created by a RulePack, tagged with its phase.
// The Scheduler uses this to register the system in the correct phase bucket.

struct SystemRegistration
{
    SimPhase phase;
    std::unique_ptr<ISystem> system;
};

// FieldBindings: generic indexed field role mapping.
// The Engine uses these bindings to construct EnvironmentModule / InformationModule
// without knowing any semantic role names (temperature, fire, smell, etc.).
// Each RulePack maps its own semantic roles to these generic indices.
//
// EnvironmentModule field slots:
//   env0, env1, env2 = spatial 2D fields (FieldRef)
//   env3, env4       = scalar fields (ScalarFieldRef)
//
// InformationModule field slots:
//   info0, info1, info2 = spatial 2D fields (FieldRef)

struct FieldBindings
{
    // EnvironmentModule: 3 spatial + 2 scalar
    FieldKey env0, env1, env2;   // spatial 2D
    FieldKey env3, env4;         // scalar

    // InformationModule: 3 spatial
    FieldKey info0, info1, info2;  // spatial 2D
};

// IRulePack: the boundary between Engine and world-specific rules.
//
// A RulePack defines WHAT the simulation contains:
//   - Fields (temperature, fire, smell, ...)
//   - Field bindings (which field plays which semantic role)
//   - Concepts (mental models agents form)
//   - Senses (perception channels)
//   - Events (domain-specific event types)
//   - Commands (domain-specific action types)
//   - Systems (climate, fire spread, agent behavior, ...)
//
// The Engine defines HOW the simulation runs:
//   - FieldModule (double-buffered 2D grids)
//   - Scheduler (phase-based tick loop)
//   - CommandBuffer (command pattern)
//   - EventBus (observer pattern)
//   - CognitiveModule (generic perception-attention-memory-knowledge pipeline)
//
// This separation allows different worlds (HumanEvolution, AlienWar, HiveCivilization)
// to share the same runtime by providing different RulePacks.

class IRulePack
{
public:
    virtual ~IRulePack() = default;

    // Human-readable name for logging
    virtual const char* Name() const = 0;

    // --- Registration (called once during WorldState construction) ---

    // Register simulation fields (temperature, fire, smell, wind, etc.)
    // Pure virtual: every RulePack must define its fields.
    virtual void RegisterFields(FieldModule& fields) = 0;

    // Return field bindings: which FieldKey plays which semantic role.
    // Used by the Engine to construct EnvironmentModule / InformationModule.
    // Pure virtual: every RulePack must define its bindings.
    virtual FieldBindings BindFields() const = 0;

    // Register concept tags (mental models agents can form)
    // Default: empty (no custom concepts)
    virtual void RegisterConcepts() {}

    // Register sense channels (perception modalities)
    // Default: empty (use engine defaults)
    virtual void RegisterSenses() {}

    // Register domain-specific event types
    // Default: empty (use engine defaults)
    virtual void RegisterEvents() {}

    // Register domain-specific command types
    // Default: empty (use engine defaults)
    virtual void RegisterCommands() {}

    // --- System creation (called once during WorldState construction) ---

    // Create simulation systems and return them tagged with their phase.
    // Pure virtual: every RulePack must define its systems.
    virtual std::vector<SystemRegistration> CreateSystems() = 0;
};
