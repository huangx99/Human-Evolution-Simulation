#pragma once

#include "core/types/types.h"
#include <cstring>

// HistoryKey: stable identity derived from a namespaced string (e.g. "human_evolution.mass_death").
// Used for serialization, snapshot, debug. Survives across runs.
struct HistoryKey
{
    u64 value = 0;

    HistoryKey() = default;

    explicit HistoryKey(const char* name)
    {
        // FNV-1a 64-bit hash
        value = 14695981039346656037ULL;
        for (const char* p = name; *p; ++p)
        {
            value ^= static_cast<u8>(*p);
            value *= 1099511628211ULL;
        }
        if (value == 0) value = 1; // 0 = invalid
    }

    static HistoryKey FromRaw(u64 v) { HistoryKey k; k.value = v; return k; }

    bool operator==(const HistoryKey& o) const { return value == o.value; }
    bool operator!=(const HistoryKey& o) const { return value != o.value; }
    bool operator<(const HistoryKey& o) const { return value < o.value; }
};

// HistoryTypeId: compact runtime index into HistoryRegistry.
// 0 = invalid. Assigned sequentially during registration (1-based).
struct HistoryTypeId
{
    u16 index = 0;

    HistoryTypeId() = default;
    explicit HistoryTypeId(u16 i) : index(i) {}

    explicit operator bool() const { return index != 0; }
};
