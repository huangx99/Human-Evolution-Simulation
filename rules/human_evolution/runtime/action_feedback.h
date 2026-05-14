#pragma once

// ActionFeedback: tracks movement success/failure for adaptive behavior.
//
// When an agent gets stuck, feedback drives escalation:
//   stuckTicks >= 3 → forced movement bias in Escape
//   failedForageTicks → risk tolerance relaxation
//
// AgentFeedback is defined in sim/entity/agent.h.
// This header provides FeedbackUpdater to update it.

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "rules/human_evolution/runtime/agent_intent.h"

struct FeedbackUpdater
{
    static void Update(AgentFeedback& fb,
                       const AgentIntent& intent,
                       bool attemptedMove,
                       bool moveSucceeded,
                       Vec2i oldPos,
                       Vec2i newPos)
    {
        // Direction persistence: track last movement direction
        if (moveSucceeded)
        {
            fb.lastMoveDx = newPos.x - oldPos.x;
            fb.lastMoveDy = newPos.y - oldPos.y;
        }

        // Intent commitment: set lock duration when intent changes.
        // No commitment when stuck — allow immediate re-evaluation.
        if (intent.type != fb.lastIntent)
        {
            fb.intentCommitTicks = (fb.stuckTicks > 0) ? 0 : CommitDuration(intent.type);
        }
        else if (fb.intentCommitTicks > 0)
        {
            fb.intentCommitTicks--;
        }

        fb.lastIntent = intent.type;
        fb.attemptedMove = attemptedMove;
        fb.moveSucceeded = oldPos.x != newPos.x || oldPos.y != newPos.y;

        if (fb.attemptedMove && !fb.moveSucceeded)
            fb.stuckTicks++;
        else if (fb.moveSucceeded)
            fb.stuckTicks = 0;

        if (intent.type == AgentIntentType::Escape && !fb.moveSucceeded)
            fb.failedEscapeTicks++;
        else if (intent.type != AgentIntentType::Escape || fb.moveSucceeded)
            fb.failedEscapeTicks = 0;

        if ((intent.type == AgentIntentType::Forage ||
             intent.type == AgentIntentType::ApproachKnownFood) &&
            !fb.actionSucceeded)
            fb.failedForageTicks++;
        else if (intent.type != AgentIntentType::Forage &&
                 intent.type != AgentIntentType::ApproachKnownFood)
            fb.failedForageTicks = 0;

        fb.lastPosition = newPos;
    }

private:
    static i32 CommitDuration(AgentIntentType type)
    {
        switch (type)
        {
        case AgentIntentType::Escape:           return 3;
        case AgentIntentType::Forage:           return 4;
        case AgentIntentType::ApproachKnownFood: return 4;
        case AgentIntentType::Explore:          return 5;
        default:                                return 2;
        }
    }
};
