#pragma once

#include <cstdio>
#include <cstdlib>

#ifdef HE_DEBUG
    #define HE_ASSERT(expr) \
        do { \
            if (!(expr)) { \
                std::fprintf(stderr, "ASSERT FAILED: %s\n  at %s:%d\n", \
                    #expr, __FILE__, __LINE__); \
                std::abort(); \
            } \
        } while (0)
#else
    #define HE_ASSERT(expr) ((void)0)
#endif
