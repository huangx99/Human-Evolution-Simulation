#pragma once

#include "core/types/types.h"

class Random
{
public:
    explicit Random(u64 seed);

    u32 NextU32() const;
    u64 NextU64() const;
    f32 Next01() const;
    i32 NextRange(i32 min, i32 max) const;

    const u64* GetState() const { return state; }

private:
    mutable u64 state[2];

    u64 Rotl(u64 x, i32 k) const;
    u64 NextU64Internal() const;
};
