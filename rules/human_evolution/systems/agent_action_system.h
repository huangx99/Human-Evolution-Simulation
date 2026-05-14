#pragma once

#include "sim/system/i_system.h"
#include "sim/system/system_context.h"
#include "rules/human_evolution/human_evolution_context.h"
#include "sim/cognitive/concept_registry.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>

// AgentActionSystem: executes agent behavior based on perception and action state.
// Moved from sim/system/ — depends on HumanEvolution field bindings.

class AgentActionSystem : public ISystem
{
public:
    explicit AgentActionSystem(const HumanEvolution::EnvironmentContext& envCtx)
        : envCtx_(envCtx) {}

    void Update(SystemContext& ctx) override
    {
        auto& fm = ctx.GetFieldModule();
        auto& commands = ctx.Commands();
        auto& cog = ctx.Cognitive();
        auto tick = ctx.CurrentTick();

        for (auto& agent : ctx.Agents().agents)
        {
            if (!agent.alive) continue;

            auto& exploreState = exploration_[agent.id];
            Vec2i previousPosition = exploreState.previousPosition;
            bool hasPreviousPosition = exploreState.hasPreviousPosition;

            // Hunger increases every tick
            commands.Submit(tick, ModifyHungerCommand{agent.id, 0.15f});
            MarkVisibleExplored(ctx.World(), agent);
            InvalidateVisibleMissingFoodMemories(ctx.World(), cog, agent);

            i32 dx = 0, dy = 0;

            switch (agent.currentAction)
            {
            case AgentAction::Flee:
            {
                i32 bestDx = 0, bestDy = 0;
                f32 bestHazard = TotalHazard(fm, cog, envCtx_, agent.id,
                                             agent.position.x, agent.position.y);
                static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                for (auto& off : offsets)
                {
                    i32 nx = agent.position.x + off[0];
                    i32 ny = agent.position.y + off[1];
                    f32 hazard = TotalHazard(fm, cog, envCtx_, agent.id, nx, ny);
                    if (hazard < bestHazard)
                    {
                        bestHazard = hazard;
                        bestDx = off[0];
                        bestDy = off[1];
                    }
                }
                dx = bestDx;
                dy = bestDy;
                break;
            }
            case AgentAction::MoveToFood:
            {
                i32 bestDx = 0, bestDy = 0;
                bool hasDirection = ChooseStepTowardLearnedFood(ctx.World(), fm, cog, envCtx_, agent,
                                                                 hasPreviousPosition, previousPosition,
                                                                 bestDx, bestDy);
                if (!hasDirection)
                    hasDirection = ChooseStepAlongFoodScent(fm, cog, envCtx_, agent,
                                                            hasPreviousPosition, previousPosition,
                                                            bestDx, bestDy);
                if (!hasDirection)
                    ChooseExplorationStep(ctx.World(), fm, cog, envCtx_, agent, bestDx, bestDy);
                dx = bestDx;
                dy = bestDy;
                break;
            }
            case AgentAction::Wander:
            {
                ChooseExplorationStep(ctx.World(), fm, cog, envCtx_, agent, dx, dy);
                break;
            }
            default:
                break;
            }

            // Submit move command
            i32 newX = agent.position.x + dx;
            i32 newY = agent.position.y + dy;
            if (fm.InBounds(envCtx_.temperature, newX, newY) &&
                (dx == 0 && dy == 0 || !IsHazardous(fm, cog, envCtx_, agent.id, newX, newY)))
            {
                commands.Submit(tick, MoveAgentCommand{agent.id, newX, newY});
            }

            exploreState.previousPosition = agent.position;
            exploreState.hasPreviousPosition = true;

            // Eating requires standing on a real food entity; the food field only guides navigation.
            if (HasFoodAt(ctx.World(), agent.position.x, agent.position.y))
            {
                commands.Submit(tick, FeedAgentCommand{agent.id, 5.0f});
            }

            // Heat damage: submit DamageAgent command
            if (agent.localTemperature > 50.0f)
            {
                f32 damage = (agent.localTemperature - 50.0f) * 0.1f;
                commands.Submit(tick, DamageAgentCommand{agent.id, damage});
            }
        }
    }

    SystemDescriptor Descriptor() const override
    {
        static constexpr ModuleAccess READS[] = {
            {ModuleTag::Agent, AccessMode::Read},
            {ModuleTag::Simulation, AccessMode::Read}
        };
        static constexpr ModuleAccess WRITES[] = {
            {ModuleTag::Command, AccessMode::Write}
        };
        static const char* const DEPS[] = {"AgentDecisionSystem"};
        return {"AgentActionSystem", SimPhase::Action, READS, 2, WRITES, 1, DEPS, 1, true, false};
    }

private:
    HumanEvolution::EnvironmentContext envCtx_;

    struct ExploreState
    {
        std::unordered_set<i32> explored;
        Vec2i target{0, 0};
        bool hasTarget = false;
        Vec2i previousPosition{0, 0};
        bool hasPreviousPosition = false;
    };

    std::unordered_map<EntityId, ExploreState> exploration_;

    static i32 CellKey(const WorldState& world, i32 x, i32 y)
    {
        return y * world.Width() + x;
    }

    static bool InWorld(const WorldState& world, i32 x, i32 y)
    {
        return x >= 0 && y >= 0 && x < world.Width() && y < world.Height();
    }

    bool IsVisibleToAgent(const WorldState& world, const Agent& agent, Vec2i location) const
    {
        if (!InWorld(world, location.x, location.y)) return false;
        return DistanceSquared(agent.position, location) <=
               visualExplorationRadius * visualExplorationRadius;
    }

    void MarkVisibleExplored(const WorldState& world, const Agent& agent)
    {
        auto& state = exploration_[agent.id];
        i32 radiusSq = visualExplorationRadius * visualExplorationRadius;
        for (i32 y = agent.position.y - visualExplorationRadius;
             y <= agent.position.y + visualExplorationRadius; ++y)
        {
            for (i32 x = agent.position.x - visualExplorationRadius;
                 x <= agent.position.x + visualExplorationRadius; ++x)
            {
                if (!InWorld(world, x, y)) continue;
                i32 dx = x - agent.position.x;
                i32 dy = y - agent.position.y;
                if (dx * dx + dy * dy > radiusSq) continue;
                state.explored.insert(CellKey(world, x, y));
            }
        }
    }
    void InvalidateVisibleMissingFoodMemories(const WorldState& world,
                                              CognitiveModule& cog,
                                              const Agent& agent) const
    {
        const auto& conceptRegistry = ConceptTypeRegistry::Instance();
        for (auto& memory : cog.GetAgentMemories(agent.id))
        {
            if (conceptRegistry.GetName(memory.subject) != "food") continue;
            if (memory.sense != SenseType::Vision) continue;
            if (!IsVisibleToAgent(world, agent, memory.location)) continue;
            if (HasFoodAt(world, memory.location.x, memory.location.y)) continue;

            memory.strength *= missingFoodMemoryStrengthScale;
            memory.confidence *= missingFoodMemoryConfidenceScale;
            memory.reinforcementCount = 1;
            memory.kind = MemoryKind::ShortTerm;
        }
    }


    i32 VisibleGainFrom(const WorldState& world, const ExploreState& state, Vec2i position) const
    {
        i32 gain = 0;
        i32 radiusSq = visualExplorationRadius * visualExplorationRadius;
        for (i32 y = position.y - visualExplorationRadius;
             y <= position.y + visualExplorationRadius; ++y)
        {
            for (i32 x = position.x - visualExplorationRadius;
                 x <= position.x + visualExplorationRadius; ++x)
            {
                if (!InWorld(world, x, y)) continue;
                i32 dx = x - position.x;
                i32 dy = y - position.y;
                if (dx * dx + dy * dy > radiusSq) continue;
                if (state.explored.find(CellKey(world, x, y)) == state.explored.end())
                    ++gain;
            }
        }
        return gain;
    }

    bool FindNearestUnexploredTarget(const WorldState& world,
                                     const FieldModule& fm,
                                     const CognitiveModule& cog,
                                     const HumanEvolution::EnvironmentContext& envCtx,
                                     const Agent& agent,
                                     const ExploreState& state,
                                     Vec2i& target) const
    {
        bool found = false;
        i32 bestScore = std::numeric_limits<i32>::max();
        for (i32 y = 0; y < world.Height(); ++y)
        {
            for (i32 x = 0; x < world.Width(); ++x)
            {
                if (state.explored.find(CellKey(world, x, y)) != state.explored.end()) continue;
                if (!fm.InBounds(envCtx.temperature, x, y)) continue;
                if (IsHazardous(fm, cog, envCtx, agent.id, x, y)) continue;

                i32 distSq = DistanceSquared(agent.position, {x, y});
                i32 score = distSq - VisibleGainFrom(world, state, {x, y}) * 4;
                if (score < bestScore)
                {
                    bestScore = score;
                    target = {x, y};
                    found = true;
                }
            }
        }
        return found;
    }

    void ChooseExplorationStep(const WorldState& world,
                               const FieldModule& fm,
                               const CognitiveModule& cog,
                               const HumanEvolution::EnvironmentContext& envCtx,
                               const Agent& agent,
                               i32& bestDx,
                               i32& bestDy)
    {
        auto& state = exploration_[agent.id];
        auto targetValid = [&]() {
            return state.hasTarget &&
                   InWorld(world, state.target.x, state.target.y) &&
                   state.explored.find(CellKey(world, state.target.x, state.target.y)) == state.explored.end() &&
                   !IsHazardous(fm, cog, envCtx, agent.id, state.target.x, state.target.y);
        };

        if (!targetValid())
            state.hasTarget = FindNearestUnexploredTarget(world, fm, cog, envCtx, agent, state, state.target);

        if (!state.hasTarget)
        {
            state.explored.clear();
            MarkVisibleExplored(world, agent);
            state.hasTarget = FindNearestUnexploredTarget(world, fm, cog, envCtx, agent, state, state.target);
        }

        static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        f32 bestScore = std::numeric_limits<f32>::lowest();
        i32 currentDist = state.hasTarget ? DistanceSquared(agent.position, state.target) : 0;
        for (auto& offset : offsets)
        {
            Vec2i candidate{agent.position.x + offset[0], agent.position.y + offset[1]};
            if (!fm.InBounds(envCtx.temperature, candidate.x, candidate.y)) continue;
            if (IsHazardous(fm, cog, envCtx, agent.id, candidate.x, candidate.y)) continue;
            i32 gain = VisibleGainFrom(world, state, candidate);
            i32 dist = state.hasTarget ? DistanceSquared(candidate, state.target) : currentDist;
            f32 hazard = TotalHazard(fm, cog, envCtx, agent.id, candidate.x, candidate.y);
            f32 score = static_cast<f32>(currentDist - dist) * 8.0f +
                        static_cast<f32>(gain) * 2.0f - hazard * 0.1f;

            if (score > bestScore)
            {
                bestScore = score;
                bestDx = offset[0];
                bestDy = offset[1];
            }
        }
    }

    static f32 CellHazard(const FieldModule& fm,
                          const HumanEvolution::EnvironmentContext& envCtx,
                          i32 x,
                          i32 y)
    {
        if (!fm.InBounds(envCtx.temperature, x, y))
            return std::numeric_limits<f32>::max();

        f32 fire = fm.InBounds(envCtx.fire, x, y) ? fm.Read(envCtx.fire, x, y) : 0.0f;
        f32 danger = fm.InBounds(envCtx.danger, x, y) ? fm.Read(envCtx.danger, x, y) : 0.0f;
        f32 temp = fm.Read(envCtx.temperature, x, y);
        f32 heat = std::max(0.0f, temp - 45.0f);
        return fire * 3.0f + danger * 1.5f + heat * 2.0f;
    }

    static f32 MemoryHazard(const CognitiveModule& cog, EntityId agentId, i32 x, i32 y)
    {
        const auto& conceptRegistry = ConceptTypeRegistry::Instance();
        f32 hazard = 0.0f;
        for (const auto& memory : cog.GetAgentMemories(agentId))
        {
            bool dangerousMemory = false;
            if (conceptRegistry.HasFlag(memory.subject, ConceptSemanticFlag::Danger) ||
                conceptRegistry.HasFlag(memory.subject, ConceptSemanticFlag::Thermal) ||
                conceptRegistry.HasFlag(memory.subject, ConceptSemanticFlag::TraumaRelevant))
            {
                dangerousMemory = true;
            }
            for (const auto& tag : memory.resultTags)
            {
                if (conceptRegistry.HasFlag(tag, ConceptSemanticFlag::Danger) ||
                    conceptRegistry.HasFlag(tag, ConceptSemanticFlag::TraumaRelevant) ||
                    conceptRegistry.GetName(tag) == "pain")
                {
                    dangerousMemory = true;
                    break;
                }
            }
            if (!dangerousMemory) continue;

            f32 dx = static_cast<f32>(x - memory.location.x);
            f32 dy = static_cast<f32>(y - memory.location.y);
            f32 distSq = dx * dx + dy * dy;
            if (distSq > learnedAvoidanceRadius * learnedAvoidanceRadius) continue;

            f32 distanceFalloff = 1.0f / (1.0f + distSq);
            f32 confidence = std::max(0.1f, memory.confidence);
            hazard += memory.strength * confidence * 80.0f * distanceFalloff;
        }
        return hazard;
    }

    static f32 TotalHazard(const FieldModule& fm,
                           const CognitiveModule& cog,
                           const HumanEvolution::EnvironmentContext& envCtx,
                           EntityId agentId,
                           i32 x,
                           i32 y)
    {
        return CellHazard(fm, envCtx, x, y) + MemoryHazard(cog, agentId, x, y);
    }

    static bool IsHazardous(const FieldModule& fm,
                            const CognitiveModule& cog,
                            const HumanEvolution::EnvironmentContext& envCtx,
                            EntityId agentId,
                            i32 x,
                            i32 y)
    {
        if (!fm.InBounds(envCtx.temperature, x, y))
            return true;

        f32 fire = fm.InBounds(envCtx.fire, x, y) ? fm.Read(envCtx.fire, x, y) : 0.0f;
        f32 danger = fm.InBounds(envCtx.danger, x, y) ? fm.Read(envCtx.danger, x, y) : 0.0f;
        f32 temp = fm.Read(envCtx.temperature, x, y);
        return fire > 0.1f || danger > 20.0f || temp > 55.0f ||
               MemoryHazard(cog, agentId, x, y) > 8.0f;
    }

    static bool HasFoodAt(const WorldState& world, i32 x, i32 y)
    {
        for (const auto& entity : world.Ecology().entities.All())
        {
            if (entity.x != x || entity.y != y) continue;
            if (IsActiveFoodEntity(entity))
            {
                return true;
            }
        }
        return false;
    }

    static bool IsActiveFoodEntity(const EcologyEntity& entity)
    {
        return entity.HasCapability(Capability::Edible) &&
               entity.HasCapability(Capability::Digestible) &&
               !entity.HasCapability(Capability::Toxic) &&
               !entity.HasState(MaterialState::Burning) &&
               !entity.HasState(MaterialState::Dead) &&
               !entity.HasState(MaterialState::Decomposing);
    }

    static bool FindLearnedFoodTarget(const CognitiveModule& cog,
                                      EntityId agentId,
                                      Vec2i& target)
    {
        const auto& conceptRegistry = ConceptTypeRegistry::Instance();
        const MemoryRecord* bestMemory = nullptr;
        f32 bestScore = std::numeric_limits<f32>::lowest();

        for (const auto& memory : cog.GetAgentMemories(agentId))
        {
            if (conceptRegistry.GetName(memory.subject) != "food") continue;
            if (memory.sense != SenseType::Vision) continue;
            if (memory.reinforcementCount < learnedFoodReinforcementThreshold) continue;

            f32 score = memory.strength * std::max(0.1f, memory.confidence) +
                        static_cast<f32>(memory.reinforcementCount) * 2.0f;
            if (score > bestScore)
            {
                bestScore = score;
                bestMemory = &memory;
            }
        }

        if (!bestMemory) return false;
        target = bestMemory->location;
        return true;
    }

    static bool ChooseStepTowardLearnedFood(const WorldState& world,
                                            const FieldModule& fm,
                                            const CognitiveModule& cog,
                                            const HumanEvolution::EnvironmentContext& envCtx,
                                            const Agent& agent,
                                            bool hasPreviousPosition,
                                            Vec2i previousPosition,
                                            i32& bestDx,
                                            i32& bestDy)
    {
        Vec2i target;
        if (!FindLearnedFoodTarget(cog, agent.id, target)) return false;
        if (DistanceSquared(agent.position, target) <= visualExplorationRadius * visualExplorationRadius &&
            !HasFoodAt(world, target.x, target.y))
            return false;

        i32 currentDist = DistanceSquared(agent.position, target);
        f32 bestScore = std::numeric_limits<f32>::lowest();
        bool found = false;
        static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        for (auto& offset : offsets)
        {
            Vec2i candidate{agent.position.x + offset[0], agent.position.y + offset[1]};
            if (!fm.InBounds(envCtx.temperature, candidate.x, candidate.y)) continue;
            if (IsHazardous(fm, cog, envCtx, agent.id, candidate.x, candidate.y)) continue;
            i32 candidateDist = DistanceSquared(candidate, target);
            if (candidateDist >= currentDist) continue;

            f32 foodSignal = fm.InBounds(envCtx.food, candidate.x, candidate.y)
                ? fm.Read(envCtx.food, candidate.x, candidate.y)
                : 0.0f;
            f32 hazard = TotalHazard(fm, cog, envCtx, agent.id, candidate.x, candidate.y);
            f32 score = static_cast<f32>(currentDist - candidateDist) * 10.0f + foodSignal - hazard * 0.1f;
            if (score > bestScore)
            {
                bestScore = score;
                bestDx = offset[0];
                bestDy = offset[1];
                found = true;
            }
        }
        return found;
    }

    static bool ChooseStepAlongFoodScent(const FieldModule& fm,
                                         const CognitiveModule& cog,
                                         const HumanEvolution::EnvironmentContext& envCtx,
                                         const Agent& agent,
                                         bool hasPreviousPosition,
                                         Vec2i previousPosition,
                                         i32& bestDx,
                                         i32& bestDy)
    {
        if (!fm.InBounds(envCtx.food, agent.position.x, agent.position.y)) return false;

        f32 currentFood = fm.Read(envCtx.food, agent.position.x, agent.position.y);
        f32 weightedDx = 0.0f;
        f32 weightedDy = 0.0f;
        f32 totalWeight = 0.0f;

        // Smell is fuzzy navigation: infer only a rough direction from the
        // nearby food field. This deliberately does not inspect food entities,
        // pick a global target, or run pathfinding; vision still has to confirm
        // the real food source before it can become stable memory.
        // Future sound cues, e.g. wolf howls, should use the same fuzzy-direction
        // shape: infer direction from local intensity, then confirm by vision.
        for (i32 dy = -scentNavigationRadius; dy <= scentNavigationRadius; ++dy)
        {
            for (i32 dx = -scentNavigationRadius; dx <= scentNavigationRadius; ++dx)
            {
                if (dx == 0 && dy == 0) continue;
                i32 x = agent.position.x + dx;
                i32 y = agent.position.y + dy;
                if (!fm.InBounds(envCtx.food, x, y)) continue;

                f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy));
                if (dist > scentNavigationRadius) continue;

                f32 signal = fm.Read(envCtx.food, x, y);
                if (signal <= foodGradientEpsilon) continue;

                f32 weight = signal / (1.0f + dist * 0.35f);
                weightedDx += static_cast<f32>(dx) * weight;
                weightedDy += static_cast<f32>(dy) * weight;
                totalWeight += weight;
            }
        }

        if (totalWeight <= foodGradientEpsilon) return false;

        f32 directionLen = std::sqrt(weightedDx * weightedDx + weightedDy * weightedDy);
        if (directionLen <= foodDirectionEpsilon) return false;

        f32 dirX = weightedDx / directionLen;
        f32 dirY = weightedDy / directionLen;
        f32 bestScore = std::numeric_limits<f32>::lowest();
        bool found = false;

        static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        for (auto& offset : offsets)
        {
            i32 nx = agent.position.x + offset[0];
            i32 ny = agent.position.y + offset[1];
            if (!fm.InBounds(envCtx.food, nx, ny)) continue;
            if (IsHazardous(fm, cog, envCtx, agent.id, nx, ny)) continue;

            f32 alignment = static_cast<f32>(offset[0]) * dirX +
                            static_cast<f32>(offset[1]) * dirY;
            if (alignment <= 0.0f) continue;

            f32 hazard = TotalHazard(fm, cog, envCtx, agent.id, nx, ny);
            f32 neighborFood = fm.Read(envCtx.food, nx, ny);
            f32 backtrackPenalty = 0.0f;
            if (hasPreviousPosition && nx == previousPosition.x && ny == previousPosition.y)
                backtrackPenalty = scentBacktrackPenalty;

            f32 score = alignment * 12.0f +
                        (neighborFood - currentFood) * 0.08f -
                        hazard * 0.1f - backtrackPenalty;
            if (score > bestScore)
            {
                bestScore = score;
                bestDx = offset[0];
                bestDy = offset[1];
                found = true;
            }
        }

        return found;
    }

    static i32 DistanceSquared(Vec2i a, Vec2i b)
    {
        i32 deltaX = a.x - b.x;
        i32 deltaY = a.y - b.y;
        return deltaX * deltaX + deltaY * deltaY;
    }

    static constexpr i32 visualExplorationRadius = 4;
    static constexpr f32 foodGradientEpsilon = 0.1f;
    static constexpr f32 foodDirectionEpsilon = 0.01f;
    static constexpr f32 scentBacktrackPenalty = 4.0f;
    static constexpr i32 scentNavigationRadius = 6;
    static constexpr f32 missingFoodMemoryStrengthScale = 0.2f;
    static constexpr f32 missingFoodMemoryConfidenceScale = 0.5f;
    static constexpr u32 learnedFoodReinforcementThreshold = 3;
    static constexpr f32 learnedAvoidanceRadius = 2.0f;
};
