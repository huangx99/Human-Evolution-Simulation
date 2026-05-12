#include "test_framework.h"

int main()
{
    std::cout << "Human Evolution - Unit Tests" << std::endl;
    std::cout << "============================" << std::endl;
    return TestRunner::Instance().Summary();
}
