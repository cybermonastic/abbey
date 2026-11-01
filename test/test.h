#pragma once

#include <iostream>
#include <string>

#define BEGIN_SUITE(name) \
    if (argc > 1 && std::string(name) != argv[1]) return 0;

#define BEGIN_TEST(name) \
    if (argc > 2 && std::string(name) != argv[2]) return 0;

#define EXPECT(actual, expected) \
    if (actual != expected) { \
        std::cerr << "FAILURE: (" << actual << ") != (" << expected << ")\n"; \
        return 1; \
    }