#pragma once

#include <iostream>
#include <cmath>
#include <cstdlib>

static int g_test_failures = 0;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "CHECK FAILED: " #cond " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            ++g_test_failures; \
        } \
    } while (0)

#define CHECK_NEAR(a, b, tol) \
    do { \
        double diff = std::abs(static_cast<double>(a) - static_cast<double>(b)); \
        if (diff > static_cast<double>(tol)) { \
            std::cerr << "CHECK_NEAR FAILED: |(" #a ") - (" #b ")| = " << diff \
                      << " > " << (tol) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            ++g_test_failures; \
        } \
    } while (0)

