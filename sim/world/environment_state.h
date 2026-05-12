#pragma once

#include "core/types/types.h"
#include "core/container/grid.h"

struct WindDirection
{
    f32 x = 0.0f;
    f32 y = 0.0f;
};

struct EnvironmentState
{
    i32 width;
    i32 height;

    Grid<f32> temperature;
    Grid<f32> humidity;
    Grid<f32> fire;
    WindDirection wind;

    EnvironmentState(i32 w, i32 h)
        : width(w), height(h)
        , temperature(w, h, 20.0f)
        , humidity(w, h, 50.0f)
        , fire(w, h, 0.0f)
    {
    }
};
