#pragma once

#include "sim/pattern/pattern_id.h"
#include <vector>
#include <string>
#include <unordered_map>

// PatternEntry: one registered pattern type.

struct PatternEntry
{
    PatternKey key;
    std::string name;
};

// PatternRegistry: global registry of pattern types.
//
// Pattern types are engine-level definitions (like "core.high_frequency_path").
// RulePacks register additional types (like "human_evolution.stable_fire_zone").
// This is a global singleton — pattern types are not per-world.

class PatternRegistry
{
public:
    static PatternRegistry& Instance()
    {
        static PatternRegistry instance;
        return instance;
    }

    // Register a pattern type. Returns the assigned PatternTypeId.
    PatternTypeId Register(PatternKey key, const char* name)
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return PatternTypeId(it->second);

        u16 id = static_cast<u16>(entries_.size() + 1);  // 1-based
        entries_.push_back({key, name});
        keyToIndex_[key.value] = id;
        return PatternTypeId(id);
    }

    // Lookup
    PatternTypeId FindByKey(PatternKey key) const
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return PatternTypeId(it->second);
        return PatternTypeId(0);
    }

    PatternKey GetKey(PatternTypeId id) const { return entries_[id.index - 1].key; }
    const std::string& GetName(PatternTypeId id) const { return entries_[id.index - 1].name; }
    u16 TypeCount() const { return static_cast<u16>(entries_.size()); }
    const std::vector<PatternEntry>& Entries() const { return entries_; }

private:
    PatternRegistry() = default;

    std::vector<PatternEntry> entries_;
    std::unordered_map<u64, u16> keyToIndex_;
};

// Core pattern type keys — engine-level, always available.

namespace PatternTypes
{
    inline PatternKey HighFrequencyPath() { return PatternKey("core.high_frequency_path"); }
    inline PatternKey AggregationZone()   { return PatternKey("core.aggregation_zone"); }
    inline PatternKey StableFieldZone()   { return PatternKey("core.stable_field_zone"); }
    inline PatternKey RepeatedBehavior()  { return PatternKey("core.repeated_behavior"); }
    inline PatternKey PeriodicMigration() { return PatternKey("core.periodic_migration"); }

    // Register all core pattern types. Call once at startup.
    inline void RegisterCoreTypes()
    {
        auto& reg = PatternRegistry::Instance();
        reg.Register(HighFrequencyPath(), "high_frequency_path");
        reg.Register(AggregationZone(),   "aggregation_zone");
        reg.Register(StableFieldZone(),   "stable_field_zone");
        reg.Register(RepeatedBehavior(),  "repeated_behavior");
        reg.Register(PeriodicMigration(), "periodic_migration");
    }
}
