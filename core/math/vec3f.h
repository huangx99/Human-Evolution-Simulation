#pragma once

#include "core/types/types.h"
#include <cmath>

struct Vec3f
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;

    Vec3f() = default;
    Vec3f(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}

    Vec3f operator+(const Vec3f& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    Vec3f operator-(const Vec3f& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    Vec3f operator*(f32 s) const { return {x * s, y * s, z * s}; }

    f32 Length() const { return std::sqrt(x * x + y * y + z * z); }
    f32 Dot(const Vec3f& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
};
