#include "test_framework.h"
#include "core/container/grid.h"

TEST(grid_create)
{
    Grid<f32> g(10, 8, 0.0f);
    ASSERT_EQ(g.Width(), 10);
    ASSERT_EQ(g.Height(), 8);
    ASSERT_TRUE(g.InBounds(0, 0));
    ASSERT_TRUE(g.InBounds(9, 7));
    ASSERT_TRUE(!g.InBounds(10, 7));
    ASSERT_TRUE(!g.InBounds(9, 8));
    ASSERT_TRUE(!g.InBounds(-1, 0));
    return true;
}

TEST(grid_access)
{
    Grid<i32> g(5, 5, 0);
    g.At(2, 3) = 42;
    ASSERT_EQ(g.At(2, 3), 42);
    ASSERT_EQ(g.At(0, 0), 0);
    return true;
}

TEST(grid_fill)
{
    Grid<f32> g(4, 4, 1.0f);
    g.Fill(99.0f);
    ASSERT_EQ(g.At(0, 0), 99.0f);
    ASSERT_EQ(g.At(3, 3), 99.0f);
    return true;
}
