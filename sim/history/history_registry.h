#pragma once

#include "sim/history/history_id.h"
#include <string>
#include <vector>
#include <cstring>

// HistoryRegistry: maps HistoryKey -> HistoryTypeId + human-readable name.
//
// Singleton for now (consistent with PatternRegistry).
// TRANSITIONAL DEBT: should become runtime-scoped when RuntimeRegistry arrives.

class HistoryRegistry
{
public:
    static HistoryRegistry& Instance()
    {
        static HistoryRegistry reg;
        return reg;
    }

    // Register a history event type. Idempotent: returns existing id if already registered.
    HistoryTypeId Register(HistoryKey key, const char* name)
    {
        // Check if already registered
        for (size_t i = 0; i < entries_.size(); i++)
        {
            if (entries_[i].key == key)
                return HistoryTypeId(static_cast<u16>(i + 1));
        }
        entries_.push_back({key, name});
        return HistoryTypeId(static_cast<u16>(entries_.size()));
    }

    // Lookup by key. Returns 0 if not found.
    HistoryTypeId FindByKey(HistoryKey key) const
    {
        for (size_t i = 0; i < entries_.size(); i++)
        {
            if (entries_[i].key == key)
                return HistoryTypeId(static_cast<u16>(i + 1));
        }
        return HistoryTypeId(0);
    }

    const std::string& GetName(HistoryTypeId id) const
    {
        static const std::string empty;
        if (id.index == 0 || id.index > entries_.size()) return empty;
        return entries_[id.index - 1].name;
    }

    size_t TypeCount() const { return entries_.size(); }

private:
    HistoryRegistry() = default;

    struct Entry
    {
        HistoryKey key;
        std::string name;
    };
    std::vector<Entry> entries_;
};
