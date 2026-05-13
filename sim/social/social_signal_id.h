#pragma once

#include "core/types/types.h"

// SocialSignalKey: stable identity for a social signal type.
// Derived from a namespaced string (e.g. "human_evolution.fear") via FNV-1a.

struct SocialSignalKey
{
    u64 value = 0;

    SocialSignalKey() = default;

    explicit SocialSignalKey(const char* name)
        : value(FNV1a(name)) {}

    static SocialSignalKey FromRaw(u64 v) { SocialSignalKey k; k.value = v; return k; }

    bool operator==(const SocialSignalKey& o) const { return value == o.value; }
    bool operator!=(const SocialSignalKey& o) const { return value != o.value; }
    bool operator<(const SocialSignalKey& o) const { return value < o.value; }

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

// SocialSignalTypeId: runtime compact index into SocialSignalRegistry.
// Assigned sequentially during registration. 0 = invalid.

struct SocialSignalTypeId
{
    u16 index = 0;

    SocialSignalTypeId() = default;
    explicit SocialSignalTypeId(u16 idx) : index(idx) {}

    bool operator==(const SocialSignalTypeId& o) const { return index == o.index; }
    bool operator!=(const SocialSignalTypeId& o) const { return index != o.index; }
    explicit operator bool() const { return index != 0; }
};

inline SocialSignalKey MakeSocialSignalKey(const char* name)
{
    return SocialSignalKey(name);
}
