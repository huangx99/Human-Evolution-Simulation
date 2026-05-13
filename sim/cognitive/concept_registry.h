#pragma once

#include "sim/cognitive/concept_id.h"
#include <string>
#include <vector>
#include <unordered_map>

// ConceptTypeRegistry: maps ConceptKey -> ConceptTypeId + name + semantic flags.
//
// Singleton for now (consistent with SocialSignalRegistry / ObservedActionRegistry).
// TRANSITIONAL DEBT: should become runtime-scoped when RuntimeRegistry arrives.
//
// RulePacks register their concepts during WorldState construction.
// Engine systems query semantic flags instead of referencing domain-specific names.

class ConceptTypeRegistry
{
public:
    static ConceptTypeRegistry& Instance()
    {
        static ConceptTypeRegistry reg;
        return reg;
    }

    // Register a concept. Idempotent: returns existing id if already registered.
    ConceptTypeId Register(ConceptKey key, const char* name, u32 semanticFlags)
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return ConceptTypeId(it->second);

        u16 id = static_cast<u16>(entries_.size() + 1);  // 1-based
        entries_.push_back({key, name ? name : "", semanticFlags});
        keyToIndex_[key.value] = id;
        return ConceptTypeId(id);
    }

    // Lookup by key. Returns 0 if not found.
    ConceptTypeId FindByKey(ConceptKey key) const
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return ConceptTypeId(it->second);
        return ConceptTypeId(0);
    }

    const std::string& GetName(ConceptTypeId id) const
    {
        static const std::string empty;
        if (id.index == 0 || id.index > entries_.size()) return empty;
        return entries_[id.index - 1].name;
    }

    bool HasFlag(ConceptTypeId id, ConceptSemanticFlag flag) const
    {
        if (id.index == 0 || id.index > entries_.size()) return false;
        return (entries_[id.index - 1].flags & static_cast<u32>(flag)) != 0;
    }

    u32 GetFlags(ConceptTypeId id) const
    {
        if (id.index == 0 || id.index > entries_.size()) return 0;
        return entries_[id.index - 1].flags;
    }

    size_t Count() const { return entries_.size(); }

private:
    ConceptTypeRegistry() = default;

    struct Entry
    {
        ConceptKey key;
        std::string name;
        u32 flags = 0;
    };
    std::vector<Entry> entries_;
    std::unordered_map<u64, u16> keyToIndex_;
};
