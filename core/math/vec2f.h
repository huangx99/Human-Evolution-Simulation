#pragma once

#include "core/types/types.h"
#include <cmath>

struct Vec2f
{
    f32 x = 0.0f;
    f32 y = 0.0f;

    Vec2f() = default;
    Vec2f(f32 x, f32 y) : x(x), y(y) {}

    Vec2f operator+(const Vec2f& rhs) const { return {x + rhs.x, y + rhs.y}; }
    Vec2f operator-(const Vec2f& rhs) const { return {x - rhs.x, y - rhs.y}; }
    Vec2f operator*(f32 s) const { return {x * s, y * s}; }

    Vec2f& operator+=(const Vec2f& rhs) { x += rhs.x; y += rhs.y; return *this; }

    f32 Length() const { return std::sqrt(x * x + y * y); }
    f32 Dot(const Vec2f& rhs) const { return x * rhs.x + y * rhs.y; }
};
