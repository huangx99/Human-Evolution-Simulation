#pragma once

#include "sim/world/module_registry.h"
#include "core/container/field2d.h"
#include "core/types/types.h"

struct EnvironmentModule : public IModule
{
    i32 width;
    i32 height;

    Field2D<f32> temperature;
    Field2D<f32> humidity;
    Field2D<f32> fire;

    struct Wind { f32 x = 0.0f; f32 y = 0.0f; } wind;

    EnvironmentModule(i32 w, i32 h)
        : width(w), height(h)
        , temperature(w, h, 20.0f)
        , humidity(w, h, 50.0f)
        , fire(w, h, 0.0f)
    {
    }

    void SwapAll()
    {
        temperature.Swap();
        humidity.Swap();
        fire.Swap();
    }
};
