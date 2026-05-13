#pragma once

#include "core/types/types.h"
#include <cstring>

// GroupKnowledgeKey: FNV-1a hash of a domain-qualified string (e.g. "human_evolution.shared_danger_zone").
// GroupKnowledgeTypeId: 1-based u16 handle into GroupKnowledgeRegistry.
//
// Follows the same pattern as SocialSignalKey / ConceptKey.

struct GroupKnowledgeKey
{
    u64 value = 0;
};

inline GroupKnowledgeKey MakeGroupKnowledgeKey(const char* str)
{
    // FNV-1a 64-bit
    u64 hash = 0xcbf29ce484222325ULL;
    for (const char* p = str; *p; ++p)
    {
        hash ^= static_cast<u64>(static_cast<u8>(*p));
        hash *= 0x100000001b3ULL;
    }
    return {hash};
}

struct GroupKnowledgeTypeId
{
    u16 index = 0;  // 1-based; 0 = invalid

    GroupKnowledgeTypeId() = default;
    explicit GroupKnowledgeTypeId(u16 idx) : index(idx) {}

    explicit operator bool() const { return index != 0; }
};

inline bool operator==(GroupKnowledgeTypeId a, GroupKnowledgeTypeId b) { return a.index == b.index; }
inline bool operator!=(GroupKnowledgeTypeId a, GroupKnowledgeTypeId b) { return a.index != b.index; }
