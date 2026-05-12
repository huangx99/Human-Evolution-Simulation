#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

struct TestResult
{
    std::string name;
    bool passed;
    std::string message;
};

class TestRunner
{
public:
    static TestRunner& Instance()
    {
        static TestRunner instance;
        return instance;
    }

    void Run(const std::string& name, bool (*func)())
    {
        bool passed = func();
        results.push_back({name, passed, passed ? "OK" : "FAILED"});
        std::cout << (passed ? "  PASS: " : "  FAIL: ") << name << std::endl;
    }

    int Summary()
    {
        int passed = 0, failed = 0;
        for (const auto& r : results)
        {
            if (r.passed) passed++;
            else failed++;
        }
        std::cout << "\n=== Results: " << passed << " passed, "
                  << failed << " failed ===" << std::endl;
        return failed;
    }

private:
    std::vector<TestResult> results;
};

#define TEST(name) bool test_##name(); \
    static bool _reg_##name = (TestRunner::Instance().Run(#name, test_##name), true); \
    bool test_##name()

#define ASSERT_TRUE(expr) do { if (!(expr)) return false; } while(0)
#define ASSERT_EQ(a, b) do { if ((a) != (b)) return false; } while(0)
#define ASSERT_NEAR(a, b, eps) do { if (std::abs((a) - (b)) > (eps)) return false; } while(0)
