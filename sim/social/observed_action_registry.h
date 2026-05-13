#pragma once

#include "sim/social/observed_action_id.h"
#include <string>
#include <vector>
#include <unordered_map>

// ObservedActionRegistry: maps ObservedActionKey -> ObservedActionTypeId + human-readable name.
//
// Singleton for now (consistent with SocialSignalRegistry / PatternRegistry).
// TRANSITIONAL DEBT: should become runtime-scoped when RuntimeRegistry arrives.

class ObservedActionRegistry
{
public:
    static ObservedActionRegistry& Instance()
    {
        static ObservedActionRegistry reg;
        return reg;
    }

    // Register an observed action type. Idempotent: returns existing id if already registered.
    ObservedActionTypeId Register(ObservedActionKey key, const char* name)
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return ObservedActionTypeId(it->second);

        u16 id = static_cast<u16>(entries_.size() + 1);  // 1-based
        entries_.push_back({key, name ? name : ""});
        keyToIndex_[key.value] = id;
        return ObservedActionTypeId(id);
    }

    // Lookup by key. Returns 0 if not found.
    ObservedActionTypeId FindByKey(ObservedActionKey key) const
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return ObservedActionTypeId(it->second);
        return ObservedActionTypeId(0);
    }

    const std::string& GetName(ObservedActionTypeId id) const
    {
        static const std::string empty;
        if (id.index == 0 || id.index > entries_.size()) return empty;
        return entries_[id.index - 1].name;
    }

    size_t TypeCount() const { return entries_.size(); }

private:
    ObservedActionRegistry() = default;

    struct Entry
    {
        ObservedActionKey key;
        std::string name;
    };
    std::vector<Entry> entries_;
    std::unordered_map<u64, u16> keyToIndex_;
};
