#pragma once

#include "core/types/types.h"
#include "core/container/grid.h"

struct InformationState
{
    i32 width;
    i32 height;

    Grid<f32> smell;
    Grid<f32> danger;

    InformationState(i32 w, i32 h)
        : width(w), height(h)
        , smell(w, h, 0.0f)
        , danger(w, h, 0.0f)
    {
    }
};
