#pragma once

#include "sim/social/social_signal_id.h"
#include <string>
#include <vector>
#include <unordered_map>

// SocialSignalRegistry: maps SocialSignalKey -> SocialSignalTypeId + human-readable name.
//
// Singleton for now (consistent with PatternRegistry / HistoryRegistry).
// TRANSITIONAL DEBT: should become runtime-scoped when RuntimeRegistry arrives.

class SocialSignalRegistry
{
public:
    static SocialSignalRegistry& Instance()
    {
        static SocialSignalRegistry reg;
        return reg;
    }

    // Register a social signal type. Idempotent: returns existing id if already registered.
    SocialSignalTypeId Register(SocialSignalKey key, const char* name)
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return SocialSignalTypeId(it->second);

        u16 id = static_cast<u16>(entries_.size() + 1);  // 1-based
        entries_.push_back({key, name});
        keyToIndex_[key.value] = id;
        return SocialSignalTypeId(id);
    }

    // Lookup by key. Returns 0 if not found.
    SocialSignalTypeId FindByKey(SocialSignalKey key) const
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return SocialSignalTypeId(it->second);
        return SocialSignalTypeId(0);
    }

    const std::string& GetName(SocialSignalTypeId id) const
    {
        static const std::string empty;
        if (id.index == 0 || id.index > entries_.size()) return empty;
        return entries_[id.index - 1].name;
    }

    size_t TypeCount() const { return entries_.size(); }

private:
    SocialSignalRegistry() = default;

    struct Entry
    {
        SocialSignalKey key;
        std::string name;
    };
    std::vector<Entry> entries_;
    std::unordered_map<u64, u16> keyToIndex_;
};
