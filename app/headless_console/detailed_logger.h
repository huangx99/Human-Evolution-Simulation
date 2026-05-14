#pragma once

#include "sim/world/world_state.h"
#include "rules/human_evolution/human_evolution_context.h"

void PrintDetailedLog(const WorldState& world,
                      const HumanEvolution::EnvironmentContext& envCtx);
