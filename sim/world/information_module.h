#pragma once

#include "sim/world/module_registry.h"
#include "core/container/field2d.h"
#include "core/types/types.h"

struct InformationModule : public IModule
{
    i32 width;
    i32 height;

    Field2D<f32> smell;
    Field2D<f32> danger;
    Field2D<f32> smoke;

    InformationModule(i32 w, i32 h)
        : width(w), height(h)
        , smell(w, h, 0.0f)
        , danger(w, h, 0.0f)
        , smoke(w, h, 0.0f)
    {
    }

    void SwapAll()
    {
        smell.Swap();
        danger.Swap();
        smoke.Swap();
    }
};
