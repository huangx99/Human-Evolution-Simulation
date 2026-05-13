#pragma once

#include "sim/world/module_registry.h"
#include "sim/field/field_ref.h"

// DEPRECATED alias facade — spatial fields delegate to FieldModule.
// Use FieldModule via world.Fields() in new code.
// Wind is scalar (not 2D), so kept as plain f32 and synced separately.

struct EnvironmentModule : public IModule
{
    FieldRef temperature;
    FieldRef humidity;
    FieldRef fire;

    struct Wind { f32 x = 0.0f; f32 y = 0.0f; } wind;

    EnvironmentModule(FieldModule& fm)
        : temperature(&fm, fm.FindByKey(FieldKey("human_evolution.temperature")))
        , humidity(&fm, fm.FindByKey(FieldKey("human_evolution.humidity")))
        , fire(&fm, fm.FindByKey(FieldKey("human_evolution.fire")))
    {
    }

    void SwapAll() {}  // no-op — FieldModule.SwapAll() handles spatial fields
};
