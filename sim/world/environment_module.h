#pragma once

#include "sim/world/module_registry.h"
#include "sim/field/field_ref.h"
#include "sim/field/scalar_field_ref.h"
#include "sim/runtime/rule_pack.h"

// EnvironmentModule: alias facade for environment-related fields.
// Generic field names (env0-env4) — the RulePack defines which field
// plays which semantic role. The Engine does NOT know any role names.

struct EnvironmentModule : public IModule
{
    FieldRef env0;       // spatial 2D
    FieldRef env1;       // spatial 2D
    FieldRef env2;       // spatial 2D
    ScalarFieldRef env3; // scalar
    ScalarFieldRef env4; // scalar

    EnvironmentModule(FieldModule& fm, const FieldBindings& b)
        : env0(&fm, fm.FindByKey(b.env0))
        , env1(&fm, fm.FindByKey(b.env1))
        , env2(&fm, fm.FindByKey(b.env2))
        , env3(&fm, fm.FindByKey(b.env3))
        , env4(&fm, fm.FindByKey(b.env4))
    {
    }

    void SwapAll() {}  // no-op — FieldModule.SwapAll() handles it
};
