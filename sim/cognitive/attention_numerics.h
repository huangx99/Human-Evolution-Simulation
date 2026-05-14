#pragma once

#include "core/types/types.h"
#include <algorithm>
#include <cmath>

struct AttentionNumericConfig
{
    f32 minScore = 0.0f;
    f32 maxScore = 20.0f;
    f32 invalidScore = 0.0f;
};

struct AttentionNumerics
{
    static f32 Sanitize(f32 score, const AttentionNumericConfig& cfg)
    {
        if (!std::isfinite(score))
            return cfg.invalidScore;
        return std::max(cfg.minScore, std::min(score, cfg.maxScore));
    }
};

struct MemoryNumericConfig
{
    f32 minStrength = 0.0f;
    f32 maxStrength = 20.0f;
    f32 invalidStrength = 0.0f;
};

struct MemoryNumerics
{
    static f32 Sanitize(f32 strength, const MemoryNumericConfig& cfg)
    {
        if (!std::isfinite(strength))
            return cfg.invalidStrength;
        return std::max(cfg.minStrength, std::min(strength, cfg.maxStrength));
    }
};
