#pragma once

#include "sim/world/module_registry.h"
#include "sim/field/field_ref.h"
#include "sim/field/scalar_field_ref.h"
#include "sim/runtime/rule_pack.h"

// EnvironmentModule: alias facade for environment-related fields.
// FieldKeys are provided by the RulePack via FieldBindings — this module
// does NOT know any field names. Wind is stored as ScalarFieldRef (no
// separate f32 struct, no Scheduler sync needed).

struct EnvironmentModule : public IModule
{
    FieldRef temperature;
    FieldRef humidity;
    FieldRef fire;
    ScalarFieldRef windX;
    ScalarFieldRef windY;

    EnvironmentModule(FieldModule& fm, const FieldBindings& b)
        : temperature(&fm, fm.FindByKey(b.temperature))
        , humidity(&fm, fm.FindByKey(b.humidity))
        , fire(&fm, fm.FindByKey(b.fire))
        , windX(&fm, fm.FindByKey(b.windX))
        , windY(&fm, fm.FindByKey(b.windY))
    {
    }

    void SwapAll() {}  // no-op — FieldModule.SwapAll() handles it
};
