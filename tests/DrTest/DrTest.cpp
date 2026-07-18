// Copyright 2026, Peter Goodman. All rights reserved.
//
// Entry point for the `drtest` harness: runs every registered test case and
// returns a non-zero exit code if any of them fail.

#include "DrTest.h"

#include <cstdio>
#include <exception>

int main(int, char **) {
  auto &tests = ::drtest::Registry();
  std::printf("[==========] Running %zu test(s).\n", tests.size());

  int failed = 0;
  for (const auto &test : tests) {
    std::printf("[ RUN      ] %s.%s\n", test.suite, test.name);

    bool ok = true;
    try {
      test.fn();
    } catch (const ::drtest::AssertionFailure &) {
      ok = false;
    } catch (const std::exception &e) {
      std::fprintf(stderr, "  Unexpected exception: %s\n", e.what());
      ok = false;
    }

    if (ok) {
      std::printf("[       OK ] %s.%s\n", test.suite, test.name);
    } else {
      std::printf("[  FAILED  ] %s.%s\n", test.suite, test.name);
      ++failed;
    }
  }

  std::printf("[==========] %zu test(s) ran, %d failed.\n", tests.size(),
              failed);
  return failed ? 1 : 0;
}
