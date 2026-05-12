#pragma once

#include "core/types/types.h"

struct Vec2i
{
    i32 x = 0;
    i32 y = 0;

    Vec2i() = default;
    Vec2i(i32 x, i32 y) : x(x), y(y) {}

    Vec2i operator+(const Vec2i& rhs) const { return {x + rhs.x, y + rhs.y}; }
    Vec2i operator-(const Vec2i& rhs) const { return {x - rhs.x, y - rhs.y}; }
    Vec2i operator*(i32 s) const { return {x * s, y * s}; }

    Vec2i& operator+=(const Vec2i& rhs) { x += rhs.x; y += rhs.y; return *this; }
    Vec2i& operator-=(const Vec2i& rhs) { x -= rhs.x; y -= rhs.y; return *this; }

    bool operator==(const Vec2i& rhs) const { return x == rhs.x && y == rhs.y; }
    bool operator!=(const Vec2i& rhs) const { return !(*this == rhs); }
};
