#pragma once

#include "core/types/types.h"
#include "core/assert/he_assert.h"
#include <vector>

template<typename T>
class Grid
{
public:
    Grid() : width(0), height(0) {}
    Grid(i32 width, i32 height) : width(width), height(height), data(width * height) {}
    Grid(i32 width, i32 height, const T& defaultValue)
        : width(width), height(height), data(width * height, defaultValue) {}

    bool InBounds(i32 x, i32 y) const
    {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    T& At(i32 x, i32 y)
    {
        HE_ASSERT(InBounds(x, y));
        return data[y * width + x];
    }

    const T& At(i32 x, i32 y) const
    {
        HE_ASSERT(InBounds(x, y));
        return data[y * width + x];
    }

    void Fill(const T& value)
    {
        std::fill(data.begin(), data.end(), value);
    }

    i32 Width() const { return width; }
    i32 Height() const { return height; }

    const T* Data() const { return data.data(); }
    size_t DataSize() const { return data.size(); }

private:
    i32 width;
    i32 height;
    std::vector<T> data;
};
