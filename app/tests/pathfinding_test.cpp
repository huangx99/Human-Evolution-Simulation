// Pathfinding and LocalPerceptionMap tests

#include "rules/human_evolution/human_evolution_rule_pack.h"
#include "rules/human_evolution/runtime/local_perception_map.h"
#include "rules/human_evolution/runtime/pathfinder.h"
#include "rules/human_evolution/runtime/cell_risk.h"
#include "rules/human_evolution/runtime/states/hunger_state.h"
#include "sim/world/world_state.h"
#include "test_framework.h"

static HumanEvolutionRulePack g_pfRulePack;
static const auto& g_pfEnvCtx = g_pfRulePack.GetHumanEvolutionContext().environment;

// ============================================================
// LocalPerceptionMap basics
// ============================================================

TEST(PerceptionMap_InitAndBounds)
{
    LocalPerceptionMap map;
    map.Init(20, 20);
    ASSERT_EQ(map.width, 20);
    ASSERT_EQ(map.height, 20);
    ASSERT_TRUE(map.InBounds({0, 0}));
    ASSERT_TRUE(map.InBounds({19, 19}));
    ASSERT_TRUE(!map.InBounds({-1, 0}));
    ASSERT_TRUE(!map.InBounds({20, 0}));
    return true;
}

TEST(PerceptionMap_AllUnknownByDefault)
{
    LocalPerceptionMap map;
    map.Init(10, 10);
    ASSERT_TRUE(map.IsUnknown({5, 5}));
    ASSERT_TRUE(!map.IsBlocked({5, 5}));
    ASSERT_TRUE(!map.IsWalkable({5, 5}));
    return true;
}

TEST(PerceptionMap_UpdateAndRead)
{
    LocalPerceptionMap map;
    map.Init(10, 10);
    map.UpdateCell({3, 4}, CellKnowledge::Walkable, 0.2f);
    ASSERT_TRUE(map.IsWalkable({3, 4}));
    ASSERT_NEAR(map.Get({3, 4}).risk, 0.2f, 0.01f);

    map.UpdateCell({5, 5}, CellKnowledge::Blocked, 1.0f);
    ASSERT_TRUE(map.IsBlocked({5, 5}));
    return true;
}

TEST(PerceptionMap_OutOfBoundsIsBlocked)
{
    LocalPerceptionMap map;
    map.Init(10, 10);
    auto cell = map.Get({-1, -1});
    ASSERT_TRUE(cell.knowledge == CellKnowledge::Blocked);
    return true;
}

// ============================================================
// Pathfinder: basic path planning
// ============================================================

TEST(Pathfinder_StraightLine)
{
    LocalPerceptionMap map;
    map.Init(20, 20);
    // All walkable
    for (i32 y = 0; y < 20; y++)
        for (i32 x = 0; x < 20; x++)
            map.UpdateCell({x, y}, CellKnowledge::Walkable, 0.0f);

    PathfinderConfig cfg;
    auto path = Pathfinder::PlanPath({2, 5}, {7, 5}, map, cfg);
    ASSERT_TRUE(!path.empty());
    ASSERT_EQ(path.back().x, 7);
    ASSERT_EQ(path.back().y, 5);
    // Should be 5 steps
    ASSERT_EQ(static_cast<i32>(path.size()), 5);
    return true;
}

TEST(Pathfinder_AvoidsBlocked)
{
    LocalPerceptionMap map;
    map.Init(20, 20);
    for (i32 y = 0; y < 20; y++)
        for (i32 x = 0; x < 20; x++)
            map.UpdateCell({x, y}, CellKnowledge::Walkable, 0.0f);

    // Wall between (5,3) and (5,7)
    for (i32 y = 3; y <= 7; y++)
        map.UpdateCell({5, y}, CellKnowledge::Blocked, 1.0f);

    PathfinderConfig cfg;
    auto path = Pathfinder::PlanPath({3, 5}, {8, 5}, map, cfg);
    ASSERT_TRUE(!path.empty());
    // Path should not go through any blocked cell
    for (auto& p : path)
        ASSERT_TRUE(!map.IsBlocked(p));
    // Should reach destination
    ASSERT_EQ(path.back().x, 8);
    ASSERT_EQ(path.back().y, 5);
    return true;
}

TEST(Pathfinder_Unreachable)
{
    LocalPerceptionMap map;
    map.Init(20, 20);
    for (i32 y = 0; y < 20; y++)
        for (i32 x = 0; x < 20; x++)
            map.UpdateCell({x, y}, CellKnowledge::Walkable, 0.0f);

    // Surround target (10,10) with blocked cells
    map.UpdateCell({9, 10}, CellKnowledge::Blocked, 1.0f);
    map.UpdateCell({11, 10}, CellKnowledge::Blocked, 1.0f);
    map.UpdateCell({10, 9}, CellKnowledge::Blocked, 1.0f);
    map.UpdateCell({10, 11}, CellKnowledge::Blocked, 1.0f);

    PathfinderConfig cfg;
    auto path = Pathfinder::PlanPath({5, 5}, {10, 10}, map, cfg);
    ASSERT_TRUE(path.empty()); // unreachable
    return true;
}

TEST(Pathfinder_UnknownCellsPassable)
{
    LocalPerceptionMap map;
    map.Init(20, 20);
    // Only mark start area as walkable, rest is unknown
    map.UpdateCell({2, 5}, CellKnowledge::Walkable, 0.0f);
    map.UpdateCell({3, 5}, CellKnowledge::Walkable, 0.0f);
    // 4,5 and beyond are Unknown (default)

    PathfinderConfig cfg;
    auto path = Pathfinder::PlanPath({2, 5}, {7, 5}, map, cfg);
    // Should still find a path through unknown cells
    ASSERT_TRUE(!path.empty());
    ASSERT_EQ(path.back().x, 7);
    ASSERT_EQ(path.back().y, 5);
    return true;
}

TEST(Pathfinder_AlreadyAtTarget)
{
    LocalPerceptionMap map;
    map.Init(10, 10);
    for (i32 y = 0; y < 10; y++)
        for (i32 x = 0; x < 10; x++)
            map.UpdateCell({x, y}, CellKnowledge::Walkable, 0.0f);

    PathfinderConfig cfg;
    std::vector<Vec2i> cachedPath;
    i32 pathIndex = 0, pathAge = 0;

    auto result = Pathfinder::GetNextMove({5, 5}, {5, 5}, map, cachedPath, pathIndex, pathAge, cfg);
    ASSERT_TRUE(result.targetReached);
    return true;
}

TEST(Pathfinder_GetNextMove_FollowsPath)
{
    LocalPerceptionMap map;
    map.Init(20, 20);
    for (i32 y = 0; y < 20; y++)
        for (i32 x = 0; x < 20; x++)
            map.UpdateCell({x, y}, CellKnowledge::Walkable, 0.0f);

    PathfinderConfig cfg;
    std::vector<Vec2i> cachedPath;
    i32 pathIndex = 0, pathAge = 0;

    auto result = Pathfinder::GetNextMove({2, 5}, {5, 5}, map, cachedPath, pathIndex, pathAge, cfg);
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.replanned); // first call plans
    ASSERT_EQ(result.dx, 1);      // should move right
    ASSERT_EQ(result.dy, 0);
    return true;
}

TEST(Pathfinder_ReplansWhenBlocked)
{
    LocalPerceptionMap map;
    map.Init(20, 20);
    for (i32 y = 0; y < 20; y++)
        for (i32 x = 0; x < 20; x++)
            map.UpdateCell({x, y}, CellKnowledge::Walkable, 0.0f);

    PathfinderConfig cfg;
    std::vector<Vec2i> cachedPath;
    i32 pathIndex = 0, pathAge = 0;

    // Plan initial path
    Pathfinder::GetNextMove({2, 5}, {6, 5}, map, cachedPath, pathIndex, pathAge, cfg);

    // Now block the next waypoint
    if (!cachedPath.empty())
        map.UpdateCell(cachedPath[pathIndex], CellKnowledge::Blocked, 1.0f);

    auto result = Pathfinder::GetNextMove({2, 5}, {6, 5}, map, cachedPath, pathIndex, pathAge, cfg);
    ASSERT_TRUE(result.replanned); // should have replanned
    return true;
}

TEST(Pathfinder_RiskWeightedCost)
{
    LocalPerceptionMap map;
    map.Init(20, 20);
    for (i32 y = 0; y < 20; y++)
        for (i32 x = 0; x < 20; x++)
            map.UpdateCell({x, y}, CellKnowledge::Walkable, 0.0f);

    // Make a risky direct path
    for (i32 x = 3; x <= 7; x++)
        map.UpdateCell({x, 5}, CellKnowledge::Walkable, 0.8f);

    // Make a safe detour (y=3 or y=7)
    for (i32 x = 3; x <= 7; x++)
    {
        map.UpdateCell({x, 3}, CellKnowledge::Walkable, 0.0f);
        map.UpdateCell({x, 7}, CellKnowledge::Walkable, 0.0f);
    }

    PathfinderConfig cfg;
    cfg.riskTolerance = 1.0f; // allow risky cells
    auto path = Pathfinder::PlanPath({2, 5}, {8, 5}, map, cfg);
    ASSERT_TRUE(!path.empty());

    // Check that path avoids the risky middle row if possible
    // (the safe detour should be preferred if cost is lower)
    bool usesRiskyRow = false;
    for (auto& p : path)
    {
        if (p.y == 5 && p.x >= 3 && p.x <= 7)
            usesRiskyRow = true;
    }
    // With risk*2 cost, risky cells cost 1+0.8*2=2.6, safe cells cost 1.0
    // Detour adds 4 extra steps but each is cheaper. A* should prefer detour.
    ASSERT_TRUE(!usesRiskyRow);
    return true;
}

