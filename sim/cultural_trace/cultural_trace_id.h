#pragma once

#include "core/types/types.h"
#include <cstring>

// CulturalTraceKey: FNV-1a hash of a domain-qualified string.
// CulturalTraceTypeId: 1-based u16 handle into CulturalTraceRegistry.
//
// Follows the same pattern as GroupKnowledgeKey / SocialSignalKey.

struct CulturalTraceKey
{
    u64 value = 0;
};

inline CulturalTraceKey MakeCulturalTraceKey(const char* str)
{
    u64 hash = 0xcbf29ce484222325ULL;
    for (const char* p = str; *p; ++p)
    {
        hash ^= static_cast<u64>(static_cast<u8>(*p));
        hash *= 0x100000001b3ULL;
    }
    return {hash};
}

struct CulturalTraceTypeId
{
    u16 index = 0;  // 1-based; 0 = invalid

    CulturalTraceTypeId() = default;
    explicit CulturalTraceTypeId(u16 idx) : index(idx) {}

    explicit operator bool() const { return index != 0; }
};

inline bool operator==(CulturalTraceTypeId a, CulturalTraceTypeId b) { return a.index == b.index; }
inline bool operator!=(CulturalTraceTypeId a, CulturalTraceTypeId b) { return a.index != b.index; }
