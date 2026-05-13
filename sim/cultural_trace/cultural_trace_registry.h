#pragma once

#include "sim/cultural_trace/cultural_trace_id.h"
#include <string>
#include <vector>
#include <unordered_map>

class CulturalTraceRegistry
{
public:
    static CulturalTraceRegistry& Instance()
    {
        static CulturalTraceRegistry reg;
        return reg;
    }

    CulturalTraceTypeId Register(CulturalTraceKey key, const char* name)
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return CulturalTraceTypeId(it->second);

        u16 id = static_cast<u16>(entries_.size() + 1);
        entries_.push_back({key, name});
        keyToIndex_[key.value] = id;
        return CulturalTraceTypeId(id);
    }

    CulturalTraceTypeId FindByKey(CulturalTraceKey key) const
    {
        auto it = keyToIndex_.find(key.value);
        if (it != keyToIndex_.end())
            return CulturalTraceTypeId(it->second);
        return CulturalTraceTypeId(0);
    }

    const std::string& GetName(CulturalTraceTypeId id) const
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
    CulturalTraceRegistry() = default;

    struct Entry
    {
        CulturalTraceKey key;
        std::string name;
    };
    std::vector<Entry> entries_;
    std::unordered_map<u64, u16> keyToIndex_;
};
