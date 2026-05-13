#pragma once

// SystemDescriptor: compile-time metadata declaring a system's runtime contract.
//
// Every ISystem must implement Descriptor() returning one of these.
// The Scheduler uses it for logging, phase validation, and future audit.
//
// Design: fixed-size, constexpr-friendly, zero heap allocation.
// Arrays are static constexpr in each system's Descriptor() method.

#include "sim/scheduler/phase.h"
#include <cstddef>
#include <cstdint>

enum class ModuleTag : uint8_t
{
    Simulation,     // SimClock, Random
    Environment,    // temperature, humidity, fire, wind
    Information,    // smell, danger, smoke
    Agent,          // agents (position, hunger, health, action)
    Ecology,        // entities (material, state, capabilities)
    Cognitive,      // memories, hypotheses, knowledge graph, stimuli
    Social,         // social signals between agents
    Command,        // CommandBuffer
    Event,          // EventBus
    Spatial,        // SpatialIndex
    Pattern,        // PatternModule (long-term structure detection)
    History,        // HistoryModule (significant historical events)
    Count
};

enum class AccessMode : uint8_t
{
    None,
    Read,
    Write,
    ReadWrite
};

struct ModuleAccess
{
    ModuleTag module;
    AccessMode mode;
};

struct SystemDescriptor
{
    const char* name;               // "CognitiveMemorySystem"
    SimPhase phase;                 // which phase this system runs in
    const ModuleAccess* reads;      // static constexpr array
    size_t readCount;
    const ModuleAccess* writes;     // static constexpr array
    size_t writeCount;
    const char* const* dependsOn;   // static array of dependency names
    size_t dependsOnCount;
    bool deterministic;             // does this system produce same output for same input?
    bool parallelSafe;              // future: can this system run in parallel?
};
