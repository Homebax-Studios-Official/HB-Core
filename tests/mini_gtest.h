// Minimal GoogleTest-compatible subset used only when GoogleTest is unavailable (offline builds).
// Supports TEST, EXPECT_/ASSERT_ {TRUE,FALSE,EQ,NE,LT,LE,GT,GE,NEAR}.
#pragma once
#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace hbmini {
struct TestCase { const char* suite; const char* name; std::function<void()> fn; };
inline std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
inline int& failures() { static int f = 0; return f; }
inline bool& currentFailed() { static bool f = false; return f; }
struct Registrar { Registrar(const char* s, const char* n, std::function<void()> f) { registry().push_back({s, n, std::move(f)}); } };
inline void fail(const char* file, int line, const std::string& msg) {
    std::cerr << file << ":" << line << ": Failure\n  " << msg << "\n";
    currentFailed() = true;
}
template <class A, class B> std::string pair(const char* ea, const char* eb, const A& a, const B& b, const char* op) {
    std::ostringstream s; s << "Expected: " << ea << " " << op << " " << eb << "\n  Actual: " << a << " vs " << b; return s.str();
}
}

#define TEST(suite, name) \
    static void hbmini_##suite##_##name(); \
    static hbmini::Registrar hbmini_reg_##suite##_##name(#suite, #name, hbmini_##suite##_##name); \
    static void hbmini_##suite##_##name()

#define HBMINI_CHECK(cond, msg, ret) do { if (!(cond)) { hbmini::fail(__FILE__, __LINE__, msg); ret; } } while (0)
#define EXPECT_TRUE(c) HBMINI_CHECK((c), "Expected true: " #c, (void)0)
#define EXPECT_FALSE(c) HBMINI_CHECK(!(c), "Expected false: " #c, (void)0)
#define ASSERT_TRUE(c) HBMINI_CHECK((c), "Expected true: " #c, return)
#define ASSERT_FALSE(c) HBMINI_CHECK(!(c), "Expected false: " #c, return)
#define HBMINI_BIN(a, b, op, ret) do { const auto hb_a = (a); const auto hb_b = (b); \
    if (!(hb_a op hb_b)) { hbmini::fail(__FILE__, __LINE__, hbmini::pair(#a, #b, hb_a, hb_b, #op)); ret; } } while (0)
#define EXPECT_EQ(a, b) HBMINI_BIN(a, b, ==, (void)0)
#define EXPECT_NE(a, b) HBMINI_BIN(a, b, !=, (void)0)
#define EXPECT_LT(a, b) HBMINI_BIN(a, b, <, (void)0)
#define EXPECT_LE(a, b) HBMINI_BIN(a, b, <=, (void)0)
#define EXPECT_GT(a, b) HBMINI_BIN(a, b, >, (void)0)
#define EXPECT_GE(a, b) HBMINI_BIN(a, b, >=, (void)0)
#define ASSERT_EQ(a, b) HBMINI_BIN(a, b, ==, return)
#define ASSERT_NE(a, b) HBMINI_BIN(a, b, !=, return)
#define ASSERT_GT(a, b) HBMINI_BIN(a, b, >, return)
#define ASSERT_GE(a, b) HBMINI_BIN(a, b, >=, return)
#define ASSERT_LT(a, b) HBMINI_BIN(a, b, <, return)
#define EXPECT_NEAR(a, b, tol) HBMINI_CHECK(std::fabs((double)(a) - (double)(b)) <= (tol), "Expected near: " #a " ~ " #b, (void)0)
