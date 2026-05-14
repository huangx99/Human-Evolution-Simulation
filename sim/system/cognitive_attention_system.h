#pragma once

// CognitiveAttentionSystem: selects which stimuli get attended to.
//
// Engine owns the attention pipeline and numeric invariants:
//   PerceivedStimulus -> IAttentionScoringPolicy -> sanitized FocusedStimulus
//
// Rule packs own scoring semantics by providing IAttentionScoringPolicy.
// The default policy is intentionally minimal and domain-neutral.

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "sim/cognitive/attention_numerics.h"
#include "sim/cognitive/attention_scoring_policy.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

class CognitiveAttentionSystem : public ISystem
{
public:
    explicit CognitiveAttentionSystem(
        const IAttentionScoringPolicy* scoringPolicy = nullptr,
        AttentionNumericConfig numericConfig = {})
        : scoringPolicy_(scoringPolicy ? scoringPolicy : &DefaultPolicy())
        , numericConfig_(numericConfig) {}

    void Update(SystemContext& ctx) override
    {
        auto& world = ctx.World();
        auto& cog = world.Cognitive();

        std::unordered_map<EntityId, std::vector<PerceivedStimulus*>> byAgent;
        for (auto& stimulus : cog.frameStimuli)
            byAgent[stimulus.observerId].push_back(&stimulus);

        for (auto& agent : world.Agents().agents)
        {
            if (!agent.alive) continue;

            auto it = byAgent.find(agent.id);
            if (it == byAgent.end()) continue;

            std::vector<FocusedStimulus> candidates;
            for (auto* stimulus : it->second)
            {
                f32 rawScore = scoringPolicy_->Score(*stimulus, agent, world);
                f32 score = AttentionNumerics::Sanitize(rawScore, numericConfig_);
                if (score > attentionThreshold)
                {
                    FocusedStimulus focused;
                    focused.stimulus = *stimulus;
                    focused.attentionScore = score;
                    candidates.push_back(focused);
                }
            }

            std::sort(candidates.begin(), candidates.end(),
                [](const FocusedStimulus& a, const FocusedStimulus& b) {
                    if (a.attentionScore != b.attentionScore)
                        return a.attentionScore > b.attentionScore;
                    return a.stimulus.id < b.stimulus.id;
                });

            size_t maxFocus = std::min(candidates.size(), maxFocusedPerAgent);
            for (size_t i = 0; i < maxFocus; i++)
                cog.frameFocused.push_back(candidates[i]);
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Cognitive, AccessMode::Read},
            {ModuleTag::Agent, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Cognitive, AccessMode::Write}
        };
        static const char* const DEPS[] = {"CognitivePerceptionSystem"};
        return {"CognitiveAttentionSystem", SimPhase::Perception,
                READS, 2, WRITES, 1, DEPS, 1, true, false};
    }

private:
    static constexpr f32 attentionThreshold = 0.1f;
    static constexpr size_t maxFocusedPerAgent = 4;

    static const IAttentionScoringPolicy& DefaultPolicy()
    {
        static const DefaultAttentionScoringPolicy policy;
        return policy;
    }

    const IAttentionScoringPolicy* scoringPolicy_ = nullptr;
    AttentionNumericConfig numericConfig_;
};
