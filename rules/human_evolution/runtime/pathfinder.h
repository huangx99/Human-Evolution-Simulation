#pragma once

// pathfinder.h: Incremental A* pathfinder on local perception map.
//
// Game-style pathfinding: agent knows start+destination, discovers obstacles
// incrementally through its vision radius. Plans on the LocalPerceptionMap
// (fog of war), not on the full FieldModule.
//
// This is a stateless utility. Path state lives in StateContext.

#include "core/types/types.h"
#include "core/math/vec2i.h"
#include "rules/human_evolution/runtime/local_perception_map.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>

struct PathfinderResult
{
    i32 dx = 0;
    i32 dy = 0;
    bool success = false;
    bool targetReached = false;
    bool unreachable = false;
    bool replanned = false;
};

struct PathfinderConfig
{
    f32 riskTolerance = 0.65f;
    i32 maxIterations = 200;
    i32 replanInterval = 10;
};

struct Pathfinder
{
    static PathfinderResult GetNextMove(
        Vec2i currentPos,
        Vec2i targetPos,
        const LocalPerceptionMap& map,
        std::vector<Vec2i>& cachedPath,
        i32& pathIndex,
        i32& pathAge,
        const PathfinderConfig& config)
    {
        PathfinderResult result;
        pathAge++;

        if (currentPos == targetPos)
        {
            result.targetReached = true;
            return result;
        }

        bool needReplan = false;

        if (cachedPath.empty() || pathIndex >= static_cast<i32>(cachedPath.size()))
            needReplan = true;
        else if (pathAge >= config.replanInterval)
            needReplan = true;
        else
        {
            Vec2i next = cachedPath[pathIndex];
            auto cell = map.Get(next);
            if (cell.knowledge == CellKnowledge::Blocked ||
                (cell.knowledge == CellKnowledge::Walkable && cell.risk > config.riskTolerance))
                needReplan = true;
        }

        if (needReplan)
        {
            cachedPath = PlanPath(currentPos, targetPos, map, config);
            pathIndex = 0;
            pathAge = 0;
            result.replanned = true;

            if (cachedPath.empty())
            {
                if (currentPos == targetPos)
                {
                    result.targetReached = true;
                    return result;
                }
                result.unreachable = true;
                return result;
            }
        }

        if (pathIndex < static_cast<i32>(cachedPath.size()))
        {
            Vec2i next = cachedPath[pathIndex];
            result.dx = next.x - currentPos.x;
            result.dy = next.y - currentPos.y;
            result.success = true;
            if (std::abs(result.dx) <= 1 && std::abs(result.dy) <= 1)
                pathIndex++;
        }

        return result;
    }

    static std::vector<Vec2i> PlanPath(
        Vec2i start,
        Vec2i goal,
        const LocalPerceptionMap& map,
        const PathfinderConfig& config)
    {
        if (start == goal) return {};

        auto cellKey = [&](Vec2i p) { return p.y * map.width + p.x; };
        auto heuristic = [](Vec2i a, Vec2i b) {
            return static_cast<f32>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
        };

        std::priority_queue<ANode, std::vector<ANode>, ANodeCmp> open;
        std::unordered_map<i32, ANode> closed;
        std::unordered_map<i32, f32> bestG;

        ANode startNode{start, 0, heuristic(start, goal), {-1, -1}};
        open.push(startNode);
        bestG[cellKey(start)] = 0;

        static constexpr i32 dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        i32 iterations = 0;

        while (!open.empty() && iterations < config.maxIterations)
        {
            iterations++;
            ANode cur = open.top();
            open.pop();

            i32 curKey = cellKey(cur.pos);
            if (closed.count(curKey)) continue;
            closed[curKey] = cur;

            if (cur.pos == goal)
                return ReconstructPath(closed, start, goal, map.width);

            for (auto& d : dirs)
            {
                Vec2i nb{cur.pos.x + d[0], cur.pos.y + d[1]};
                if (!map.InBounds(nb)) continue;

                i32 nbKey = cellKey(nb);
                if (closed.count(nbKey)) continue;

                auto cell = map.Get(nb);
                if (cell.knowledge == CellKnowledge::Blocked) continue;
                if (cell.knowledge == CellKnowledge::Walkable && cell.risk > config.riskTolerance) continue;

                f32 moveCost = 1.0f;
                if (cell.knowledge == CellKnowledge::Walkable)
                    moveCost = 1.0f + cell.risk * 2.0f;
                else if (cell.knowledge == CellKnowledge::Unknown)
                    moveCost = 2.0f;

                f32 tentativeG = cur.g + moveCost;
                auto it = bestG.find(nbKey);
                if (it != bestG.end() && tentativeG >= it->second) continue;
                bestG[nbKey] = tentativeG;

                ANode nbNode{nb, tentativeG, tentativeG + heuristic(nb, goal), cur.pos};
                open.push(nbNode);
            }
        }

        return {};
    }

private:
    struct ANode
    {
        Vec2i pos;
        f32 g = 0;
        f32 f = 0;
        Vec2i parent{-1, -1};
    };

    struct ANodeCmp
    {
        bool operator()(const ANode& a, const ANode& b) const { return a.f > b.f; }
    };

    static std::vector<Vec2i> ReconstructPath(
        const std::unordered_map<i32, ANode>& closed,
        Vec2i start, Vec2i goal, i32 gridWidth)
    {
        std::vector<Vec2i> path;
        Vec2i cur = goal;
        i32 curKey = cur.y * gridWidth + cur.x;

        while (cur != start)
        {
            auto it = closed.find(curKey);
            if (it == closed.end()) break;
            path.push_back(cur);
            cur = it->second.parent;
            curKey = cur.y * gridWidth + cur.x;
        }

        std::reverse(path.begin(), path.end());
        return path;
    }
};
