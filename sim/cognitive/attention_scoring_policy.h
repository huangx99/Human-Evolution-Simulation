#pragma once

#include "core/types/types.h"
#include "sim/cognitive/perceived_stimulus.h"
#include "sim/world/agent_module.h"

struct WorldState;

class IAttentionScoringPolicy
{
public:
    virtual ~IAttentionScoringPolicy() = default;

    virtual f32 Score(const PerceivedStimulus& stimulus,
                      const Agent& agent,
                      const WorldState& world) const = 0;
};

class DefaultAttentionScoringPolicy final : public IAttentionScoringPolicy
{
public:
    f32 Score(const PerceivedStimulus& stimulus,
              const Agent&,
              const WorldState&) const override
    {
        return stimulus.intensity;
    }
};
