#pragma once

#include <iostream>
#include <string>

#define EXPECT_LINE_AS_STRING(n) #n
#define EXPECT_PREFACE(file, line) std::string(file ":" EXPECT_LINE_AS_STRING(line) ": ")

#define BEGIN_SUITE(name) \
    if (argc > 1 && std::string(name) != argv[1]) return 0;

#define BEGIN_TEST(name) \
    if (argc > 2 && std::string(name) != argv[2]) return 0;

#define EXPECT(actual, expected) \
    { \
        auto a = actual; \
        auto b = expected; \
        if (a != b) { \
            std::cerr << EXPECT_PREFACE(__FILE__, __LINE__) << "FAILURE: (" << a << ") != (" << b << ")\n"; \
            return 1; \
        } \
    }

#define EXPECT_WITHIN(actual, expected, error) \
    { \
        auto a = actual; \
        auto b = expected; \
        auto e = error; \
        if (a > 0 && (a > (1.0 + e) * b || a < (1.0 - e) * b) || a < 0 && (a < (1.0 + e) * b || a > (1.0 - e) * b)) { \
            std::cerr << EXPECT_PREFACE(__FILE__, __LINE__) << "FAILURE: (" << a << ") != (" << b << "+/-" << (100 * e) << "%" << ")\n"; \
            return 1; \
        } \
    }