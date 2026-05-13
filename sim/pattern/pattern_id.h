#pragma once

#include "core/types/types.h"

// PatternKey: stable identity for a pattern type.
// Derived from a namespaced string (e.g. "core.high_frequency_path") via FNV-1a.
// Used for persistence, replay, snapshot, cross-version compatibility.

struct PatternKey
{
    u64 value = 0;

    PatternKey() = default;

    explicit PatternKey(const char* name)
        : value(FNV1a(name)) {}

    static PatternKey FromRaw(u64 v) { PatternKey k; k.value = v; return k; }

    bool operator==(const PatternKey& o) const { return value == o.value; }
    bool operator!=(const PatternKey& o) const { return value != o.value; }
    bool operator<(const PatternKey& o) const { return value < o.value; }

private:
    static u64 FNV1a(const char* s)
    {
        u64 h = 0xcbf29ce484222325ULL;
        while (*s)
        {
            h ^= static_cast<u8>(*s++);
            h *= 0x100000001b3ULL;
        }
        return h;
    }
};

// PatternTypeId: runtime compact index into PatternRegistry.
// Assigned sequentially during registration. 0 = invalid.

struct PatternTypeId
{
    u16 index = 0;

    PatternTypeId() = default;
    explicit PatternTypeId(u16 idx) : index(idx) {}

    bool operator==(const PatternTypeId& o) const { return index == o.index; }
    bool operator!=(const PatternTypeId& o) const { return index != o.index; }
    explicit operator bool() const { return index != 0; }
};
