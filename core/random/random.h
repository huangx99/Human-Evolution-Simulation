#pragma once

#include "core/types/types.h"

class Random
{
public:
    explicit Random(u64 seed);

    u32 NextU32();
    u64 NextU64();
    f32 Next01();
    i32 NextRange(i32 min, i32 max);

private:
    u64 state[2];

    u64 Rotl(u64 x, i32 k);
    u64 NextU64Internal();
};
