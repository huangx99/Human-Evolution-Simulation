#pragma once

#include "sim/group_knowledge/group_knowledge_id.h"
#include <string>
#include <vector>
#include <unordered_map>

// GroupKnowledgeRegistry: maps GroupKnowledgeKey -> GroupKnowledgeTypeId + human-readable name.
//
// Singleton for now (consistent with SocialSignalRegistry / ConceptTypeRegistry).
// TRANSITIONAL DEBT: should become runtime-scoped when RuntimeRegistry arrives.

class GroupKnowledgeRegistry
{
public:
    static GroupKnowledgeRegistry& Instance()
    {
        static GroupKnowledgeRegistry reg;
        return reg;
    }

    // Register a group knowledge type. Idempotent: returns existing id if already registered.
    GroupKnowledgeTypeId Register(GroupKnowledgeKey key, const char* name)
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return GroupKnowledgeTypeId(it->second);

        u16 id = static_cast<u16>(entries_.size() + 1);  // 1-based
        entries_.push_back({key, name});
        keyToIndex_[key.value] = id;
        return GroupKnowledgeTypeId(id);
    }

    // Lookup by key. Returns 0 if not found.
    GroupKnowledgeTypeId FindByKey(GroupKnowledgeKey key) const
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return GroupKnowledgeTypeId(it->second);
        return GroupKnowledgeTypeId(0);
    }

    const std::string& GetName(GroupKnowledgeTypeId id) const
    {
        static const std::string empty;
        if (id.index == 0 || id.index > entries_.size()) return empty;
        return entries_[id.index - 1].name;
    }

    size_t TypeCount() const { return entries_.size(); }

    void ClearForTests()
    {
        entries_.clear();
        keyToIndex_.clear();
    }

private:
    GroupKnowledgeRegistry() = default;

    struct Entry
    {
        GroupKnowledgeKey key;
        std::string name;
    };
    std::vector<Entry> entries_;
    std::unordered_map<u64, u16> keyToIndex_;
};
