#pragma once

#include "sim/world/module_registry.h"
#include "sim/field/field_ref.h"
#include "sim/runtime/rule_pack.h"

// InformationModule: alias facade for information-related fields.
// FieldKeys are provided by the RulePack via FieldBindings — this module
// does NOT know any field names.

struct InformationModule : public IModule
{
    FieldRef smell;
    FieldRef danger;
    FieldRef smoke;

    InformationModule(FieldModule& fm, const FieldBindings& b)
        : smell(&fm, fm.FindByKey(b.smell))
        , danger(&fm, fm.FindByKey(b.danger))
        , smoke(&fm, fm.FindByKey(b.smoke))
    {
    }

    void SwapAll() {}  // no-op — FieldModule.SwapAll() handles it
};
