#pragma once

#include "core/types/types.h"

// ObservedActionKey: stable identity for an observed action type.
// Derived from a namespaced string (e.g. "human_evolution.observed_flee") via FNV-1a.

struct ObservedActionKey
{
    u64 value = 0;

    ObservedActionKey() = default;

    explicit ObservedActionKey(const char* name)
        : value(FNV1a(name)) {}

    static ObservedActionKey FromRaw(u64 v) { ObservedActionKey k; k.value = v; return k; }

    bool operator==(const ObservedActionKey& o) const { return value == o.value; }
    bool operator!=(const ObservedActionKey& o) const { return value != o.value; }

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

// ObservedActionTypeId: runtime compact index into ObservedActionRegistry.
// Assigned sequentially during registration. 0 = invalid.

struct ObservedActionTypeId
{
    u16 index = 0;

    ObservedActionTypeId() = default;
    explicit ObservedActionTypeId(u16 idx) : index(idx) {}

    bool operator==(const ObservedActionTypeId& o) const { return index == o.index; }
    bool operator!=(const ObservedActionTypeId& o) const { return index != o.index; }
    explicit operator bool() const { return index != 0; }
};

inline ObservedActionKey MakeObservedActionKey(const char* name)
{
    return ObservedActionKey(name);
}
