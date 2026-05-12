#pragma once

#include "sim/ecology/ecology_entity.h"
#include <vector>

struct SpatialChunk
{
    std::vector<EcologyEntity*> entities;
};
