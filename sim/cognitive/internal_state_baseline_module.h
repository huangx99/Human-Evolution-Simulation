#pragma once

#include "sim/world/module_registry.h"
#include "core/types/types.h"
#include <unordered_map>

struct InternalStateBaseline
{
    f32 hunger = 0.0f;
    f32 health = 0.0f;
};

struct InternalStateBaselineModule : public IModule
{
    std::unordered_map<EntityId, InternalStateBaseline> baselines;
};
