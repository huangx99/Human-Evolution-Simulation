#pragma once

#include "core/types/types.h"

// ConceptKey: stable identity for a cognitive concept.
// Derived from a namespaced string (e.g. "human_evolution.fire") via FNV-1a.
// Same pattern as SocialSignalKey / ObservedActionKey.

struct ConceptKey
{
    u64 value = 0;

    ConceptKey() = default;

    explicit ConceptKey(const char* name)
        : value(FNV1a(name)) {}

    static ConceptKey FromRaw(u64 v) { ConceptKey k; k.value = v; return k; }

    bool operator==(const ConceptKey& o) const { return value == o.value; }
    bool operator!=(const ConceptKey& o) const { return value != o.value; }
    bool operator<(const ConceptKey& o) const { return value < o.value; }

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

// ConceptTypeId: runtime compact index into ConceptRegistry.
// Assigned sequentially during registration. 0 = invalid.
// Same pattern as SocialSignalTypeId / ObservedActionTypeId.

struct ConceptTypeId
{
    u16 index = 0;

    ConceptTypeId() = default;
    explicit ConceptTypeId(u16 idx) : index(idx) {}

    bool operator==(const ConceptTypeId& o) const { return index == o.index; }
    bool operator!=(const ConceptTypeId& o) const { return index != o.index; }
    explicit operator bool() const { return index != 0; }
};

inline ConceptKey MakeConceptKey(const char* name)
{
    return ConceptKey(name);
}

// ConceptSemanticFlag: bitfield flags for generic concept categorization.
// Engine systems use these instead of referencing domain-specific concept names.
// Example: attention system checks HasFlag(Danger) rather than switch(Fire/Beast/Predator).

enum class ConceptSemanticFlag : u32
{
    None        = 0,
    Danger      = 1 << 0,
    Resource    = 1 << 1,
    Internal    = 1 << 2,   // physiological (hunger, pain, health)
    Negative    = 1 << 3,
    Positive    = 1 << 4,
    Thermal     = 1 << 5,
    Organic     = 1 << 6,
    Social      = 1 << 7,
    Abstract    = 1 << 8,
    Threat      = 1 << 9,
    Edible      = 1 << 10,
    Environmental = 1 << 11,
    Mineral     = 1 << 12,
    Sheltering  = 1 << 13,
    ToolLike    = 1 << 14,
};

constexpr ConceptSemanticFlag operator|(ConceptSemanticFlag a, ConceptSemanticFlag b)
{
    return static_cast<ConceptSemanticFlag>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline bool HasConceptFlag(u32 flags, ConceptSemanticFlag f)
{
    return (flags & static_cast<u32>(f)) != 0;
}
