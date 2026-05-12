#include "test_framework.h"
#include "core/random/random.h"

TEST(random_deterministic)
{
    Random a(12345);
    Random b(12345);

    for (int i = 0; i < 1000; i++)
    {
        ASSERT_EQ(a.NextU32(), b.NextU32());
    }
    return true;
}

TEST(random_range)
{
    Random rng(42);
    for (int i = 0; i < 100; i++)
    {
        i32 val = rng.NextRange(10, 20);
        ASSERT_TRUE(val >= 10 && val < 20);
    }
    return true;
}

TEST(random_01_range)
{
    Random rng(99);
    for (int i = 0; i < 1000; i++)
    {
        f32 val = rng.Next01();
        ASSERT_TRUE(val >= 0.0f && val <= 1.0f);
    }
    return true;
}
