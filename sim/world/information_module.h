#pragma once

#include "sim/world/module_registry.h"
#include "sim/field/field_ref.h"

// DEPRECATED alias facade — fields delegate to FieldModule.
// Use FieldModule via world.Fields() in new code.

struct InformationModule : public IModule
{
    FieldRef smell;
    FieldRef danger;
    FieldRef smoke;

    InformationModule(FieldModule& fm)
        : smell(&fm, fm.FindByKey(FieldKey("human_evolution.smell")))
        , danger(&fm, fm.FindByKey(FieldKey("human_evolution.danger")))
        , smoke(&fm, fm.FindByKey(FieldKey("human_evolution.smoke")))
    {
    }

    void SwapAll() {}  // no-op — FieldModule.SwapAll() handles it
};
