#pragma once

#include "core/types/types.h"
#include "core/container/grid.h"

template<typename T>
class Field2D
{
public:
    Grid<T> current;
    Grid<T> next;

    Field2D() = default;

    Field2D(i32 width, i32 height)
        : current(width, height)
        , next(width, height)
    {
    }

    Field2D(i32 width, i32 height, const T& defaultValue)
        : current(width, height, defaultValue)
        , next(width, height, defaultValue)
    {
    }

    i32 Width() const { return current.Width(); }
    i32 Height() const { return current.Height(); }

    bool InBounds(i32 x, i32 y) const { return current.InBounds(x, y); }

    // Read from current buffer
    const T& At(i32 x, i32 y) const { return current.At(x, y); }

    // Write to next buffer (double buffering)
    T& WriteNext(i32 x, i32 y) { return next.At(x, y); }

    // Swap buffers at end of tick
    void Swap()
    {
        std::swap(current, next);
    }

    // Clear next buffer
    void ClearNext(const T& value = T{})
    {
        next.Fill(value);
    }

    // Copy current to next (for systems that need to read-modify-write)
    void CopyCurrentToNext()
    {
        // Grid doesn't have a copy assignment that copies data, so do it manually
        for (i32 y = 0; y < current.Height(); y++)
        {
            for (i32 x = 0; x < current.Width(); x++)
            {
                next.At(x, y) = current.At(x, y);
            }
        }
    }
};
