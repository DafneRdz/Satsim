#pragma once

// A minimal, dependency-free unit test framework. Good enough to get started
// without needing network access to fetch Catch2/GoogleTest; swap in a real
// framework later (see docs/roadmap.md) once the project grows.

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <functional>

namespace testfw {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, fn});
    }
};

inline int failures = 0;
inline int assertions = 0;

inline void checkNear(double actual, double expected, double tolerance,
                       const char* exprStr, const char* file, int line) {
    ++assertions;
    if (std::fabs(actual - expected) > tolerance) {
        ++failures;
        std::cerr << "  [FAIL] " << file << ":" << line << "  " << exprStr
                  << "\n         expected ~" << expected << ", got " << actual
                  << " (tolerance " << tolerance << ")\n";
    }
}

inline void checkTrue(bool condition, const char* exprStr, const char* file, int line) {
    ++assertions;
    if (!condition) {
        ++failures;
        std::cerr << "  [FAIL] " << file << ":" << line << "  " << exprStr << " was false\n";
    }
}

inline int runAll() {
    std::cout << "Running " << registry().size() << " test case(s)...\n";
    for (auto& t : registry()) {
        std::cout << "  - " << t.name << "\n";
        t.fn();
    }
    std::cout << "\n" << assertions << " assertion(s), " << failures << " failure(s)\n";
    return failures == 0 ? 0 : 1;
}

} // namespace testfw

#define TEST_CASE(name) \
    static void name(); \
    static testfw::Registrar registrar_##name(#name, name); \
    static void name()

#define CHECK_NEAR(actual, expected, tolerance) \
    testfw::checkNear((actual), (expected), (tolerance), #actual " ~= " #expected, __FILE__, __LINE__)

#define CHECK_TRUE(cond) \
    testfw::checkTrue((cond), #cond, __FILE__, __LINE__)
