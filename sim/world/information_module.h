#pragma once

#include "sim/world/module_registry.h"
#include "sim/field/field_ref.h"
#include "sim/runtime/rule_pack.h"

// InformationModule: alias facade for information-related fields.
// Generic field names (info0-info2) — the RulePack defines which field
// plays which semantic role. The Engine does NOT know any role names.

struct InformationModule : public IModule
{
    FieldRef info0;  // spatial 2D
    FieldRef info1;  // spatial 2D
    FieldRef info2;  // spatial 2D

    InformationModule(FieldModule& fm, const FieldBindings& b)
        : info0(&fm, fm.FindByKey(b.info0))
        , info1(&fm, fm.FindByKey(b.info1))
        , info2(&fm, fm.FindByKey(b.info2))
    {
    }

    void SwapAll() {}  // no-op — FieldModule.SwapAll() handles it
};
