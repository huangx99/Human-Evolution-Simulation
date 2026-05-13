#pragma once

#include "sim/scheduler/phase.h"
#include "sim/system/i_system.h"
#include "sim/field/field_module.h"
#include "sim/runtime/rule_context.h"
#include <vector>
#include <memory>

// SystemRegistration: a system created by a RulePack, tagged with its phase.
// The Scheduler uses this to register the system in the correct phase bucket.

struct SystemRegistration
{
    SimPhase phase;
    std::unique_ptr<ISystem> system;
};

// IRulePack: the boundary between Engine and world-specific rules.
//
// A RulePack defines WHAT the simulation contains:
//   - Fields (temperature, fire, smell, ...)
//   - Context (field indices, config, tables — via IRuleContext subclass)
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

    // Return the RulePack-specific context (field indices, config, tables).
    // The Engine stores this as IRuleContext* for snapshot/replay access.
    // Pure virtual: every RulePack must provide a context.
    virtual IRuleContext& GetContext() = 0;

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
