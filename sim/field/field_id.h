#pragma once

#include "core/types/types.h"

// FieldKey: stable identity for a simulation field.
// Derived from a namespaced string (e.g. "human_evolution.fire") via FNV-1a.
// Used for persistence, replay, snapshot, cross-version compatibility.

struct FieldKey
{
    u64 value = 0;

    FieldKey() = default;

    explicit FieldKey(const char* name)
        : value(FNV1a(name)) {}

    bool operator==(const FieldKey& o) const { return value == o.value; }
    bool operator!=(const FieldKey& o) const { return value != o.value; }
    bool operator<(const FieldKey& o) const { return value < o.value; }

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

// FieldIndex: runtime compact index into FieldModule.
// Assigned sequentially during registration. 0 = invalid.
// Used for hot-path read/write. NOT stable across different RulePacks.
// Named FieldIndex to avoid collision with legacy FieldId enum (sim/ecology/field_id.h).

struct FieldIndex
{
    u16 index = 0;

    FieldIndex() = default;
    explicit FieldIndex(u16 idx) : index(idx) {}

    bool operator==(const FieldIndex& o) const { return index == o.index; }
    bool operator!=(const FieldIndex& o) const { return index != o.index; }
    bool operator<(const FieldIndex& o) const { return index < o.index; }
    explicit operator bool() const { return index != 0; }
};
