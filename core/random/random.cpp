#include "core/random/random.h"

Random::Random(u64 seed)
{
    // SplitMix64 to initialize state from seed
    auto SplitMix64 = [](u64& z) -> u64 {
        z += 0x9e3779b97f4a7c15;
        u64 result = z;
        result = (result ^ (result >> 30)) * 0xbf58476d1ce4e5b9;
        result = (result ^ (result >> 27)) * 0x94d049bb133111eb;
        return result ^ (result >> 31);
    };

    u64 z = seed;
    state[0] = SplitMix64(z);
    state[1] = SplitMix64(z);
}

u64 Random::Rotl(u64 x, i32 k)
{
    return (x << k) | (x >> (64 - k));
}

u64 Random::NextU64Internal()
{
    // Xoroshiro128+
    u64 s0 = state[0];
    u64 s1 = state[1];
    u64 result = s0 + s1;

    s1 ^= s0;
    state[0] = Rotl(s0, 24) ^ s1 ^ (s1 << 16);
    state[1] = Rotl(s1, 37);

    return result;
}

u32 Random::NextU32()
{
    return static_cast<u32>(NextU64Internal() >> 32);
}

u64 Random::NextU64()
{
    return NextU64Internal();
}

f32 Random::Next01()
{
    return static_cast<f32>(NextU64Internal() >> 40) / static_cast<f32>(1ULL << 24);
}

i32 Random::NextRange(i32 min, i32 max)
{
    if (min >= max) return min;
    u32 range = static_cast<u32>(max - min);
    return min + static_cast<i32>(NextU32() % range);
}
