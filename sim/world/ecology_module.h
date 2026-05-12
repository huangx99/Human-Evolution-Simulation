#pragma once

#include "sim/world/module_registry.h"
#include "sim/ecology/ecology_registry.h"

struct EcologyModule : public IModule
{
    EcologyRegistry entities;
};
