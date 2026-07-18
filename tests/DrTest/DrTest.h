// Copyright 2026, Peter Goodman. All rights reserved.
//
// A tiny, dependency-free unit-test harness. It provides just enough of the
// GoogleTest surface used by the Dr. Lojekyll tests: a `TEST(suite, name)`
// definition macro and `ASSERT_*` checks. Link a test executable against the
// `drtest` library to pick up `main`.

#pragma once

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace drtest {

// A single registered test case.
struct TestCase {
  const char *suite;
  const char *name;
  void (*fn)(void);
};

// Global registry of test cases. A function-local static gives us a well-
// defined initialization order across translation units.
inline std::vector<TestCase> &Registry(void) {
  static std::vector<TestCase> tests;
  return tests;
}

// Registers a test case at static-initialization time.
struct Registrar {
  Registrar(const char *suite, const char *name, void (*fn)(void)) {
    Registry().push_back({suite, name, fn});
  }
};

// Thrown by a failing `ASSERT_*` to abort the current test case without
// aborting the whole process.
struct AssertionFailure {};

// Stringifies a value for diagnostic output. Types without a stream
// operator still compare fine — their failure diagnostic just cannot
// print the value (so ASSERT_EQ stays usable on row structs).
template <typename T>
inline std::string Show(const T &value) {
  if constexpr (requires(std::ostringstream &os) { os << value; }) {
    std::ostringstream os;
    os << value;
    return os.str();
  } else {
    return "<unprintable>";
  }
}

// Reports a failed binary comparison and aborts the current test.
template <typename A, typename B>
[[noreturn]] inline void FailBinary(const char *file, int line, const char *op,
                                    const char *a_expr, const char *b_expr,
                                    const A &a, const B &b) {
  std::fprintf(stderr,
               "%s:%d: Failure\n"
               "  Expected: (%s) %s (%s)\n"
               "    Actual: %s vs %s\n",
               file, line, a_expr, op, b_expr, Show(a).c_str(),
               Show(b).c_str());
  throw AssertionFailure{};
}

// Reports a failed boolean check and aborts the current test.
[[noreturn]] inline void FailBool(const char *file, int line, const char *expr,
                                  bool expected) {
  std::fprintf(stderr, "%s:%d: Failure\n  Expected: (%s) is %s\n", file, line,
               expr, expected ? "true" : "false");
  throw AssertionFailure{};
}

}  // namespace drtest

// Defines and registers a test case. The body follows the macro invocation.
#define TEST(suite, name)                                                    \
  static void drtest_body_##suite##_##name(void);                            \
  static ::drtest::Registrar drtest_reg_##suite##_##name(                    \
      #suite, #name, &drtest_body_##suite##_##name);                         \
  static void drtest_body_##suite##_##name(void)

#define ASSERT_EQ(a, b)                                                      \
  do {                                                                       \
    auto &&drtest_a = (a);                                                   \
    auto &&drtest_b = (b);                                                   \
    if (!(drtest_a == drtest_b)) {                                           \
      ::drtest::FailBinary(__FILE__, __LINE__, "==", #a, #b, drtest_a,       \
                           drtest_b);                                        \
    }                                                                        \
  } while (0)

#define ASSERT_NE(a, b)                                                      \
  do {                                                                       \
    auto &&drtest_a = (a);                                                   \
    auto &&drtest_b = (b);                                                   \
    if (!(drtest_a != drtest_b)) {                                           \
      ::drtest::FailBinary(__FILE__, __LINE__, "!=", #a, #b, drtest_a,       \
                           drtest_b);                                        \
    }                                                                        \
  } while (0)

// Ordered comparisons. Prefer these over ASSERT_TRUE(a < b): the assertion
// states its intent and the failure prints both operands — the property
// being tested survives into the diagnostic (and into any future
// property-based harness built on these).

#define ASSERT_LT(a, b)                                                      \
  do {                                                                       \
    auto &&drtest_a = (a);                                                   \
    auto &&drtest_b = (b);                                                   \
    if (!(drtest_a < drtest_b)) {                                            \
      ::drtest::FailBinary(__FILE__, __LINE__, "<", #a, #b, drtest_a,        \
                           drtest_b);                                        \
    }                                                                        \
  } while (0)

#define ASSERT_LE(a, b)                                                      \
  do {                                                                       \
    auto &&drtest_a = (a);                                                   \
    auto &&drtest_b = (b);                                                   \
    if (!(drtest_a <= drtest_b)) {                                           \
      ::drtest::FailBinary(__FILE__, __LINE__, "<=", #a, #b, drtest_a,       \
                           drtest_b);                                        \
    }                                                                        \
  } while (0)

#define ASSERT_GT(a, b)                                                      \
  do {                                                                       \
    auto &&drtest_a = (a);                                                   \
    auto &&drtest_b = (b);                                                   \
    if (!(drtest_a > drtest_b)) {                                            \
      ::drtest::FailBinary(__FILE__, __LINE__, ">", #a, #b, drtest_a,        \
                           drtest_b);                                        \
    }                                                                        \
  } while (0)

#define ASSERT_GE(a, b)                                                      \
  do {                                                                       \
    auto &&drtest_a = (a);                                                   \
    auto &&drtest_b = (b);                                                   \
    if (!(drtest_a >= drtest_b)) {                                           \
      ::drtest::FailBinary(__FILE__, __LINE__, ">=", #a, #b, drtest_a,       \
                           drtest_b);                                        \
    }                                                                        \
  } while (0)

#define ASSERT_TRUE(cond)                                                    \
  do {                                                                       \
    if (!(cond)) {                                                           \
      ::drtest::FailBool(__FILE__, __LINE__, #cond, true);                   \
    }                                                                        \
  } while (0)

#define ASSERT_FALSE(cond)                                                   \
  do {                                                                       \
    if (cond) {                                                              \
      ::drtest::FailBool(__FILE__, __LINE__, #cond, false);                  \
    }                                                                        \
  } while (0)
