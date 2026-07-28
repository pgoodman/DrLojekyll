// Copyright 2026, Peter Goodman. All rights reserved.
//
// Shared fork/waitpid death-test harness for always-on validator negative
// space: the checks under test are fprintf+abort (they survive NDEBUG), so a
// death arm runs the check in a fork()ed child and classifies how the child
// terminated — the ONLY termination a death arm accepts is SIGABRT
// specifically. Extracted at D3.a.1 (Fable review [G]) when the third
// near-verbatim copy appeared; the EINTR/waitpid hardening had already
// drifted across copies once (the InstanceOrderTest retrofit), which is the
// drift class this header ends.
#pragma once

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

namespace drtest {

// Child-termination classes the arms assert on. kForkFailed makes a fork()
// failure loud in BOTH arms instead of masquerading as either outcome.
enum class ChildOutcome { kCleanExit, kSigAbrt, kOtherAbnormal, kForkFailed };

// Run `check()` in a forked child and classify the child's termination.
// stdout is flushed before fork so the child's abort-time flush cannot replay
// the parent's buffered DrTest progress lines into the ctest log. waitpid
// failure is LOUD: EINTR is retried; any other failure (e.g. ECHILD under
// SIG_IGN-SIGCHLD auto-reaping) classifies kOtherAbnormal — never a vacuous
// clean exit.
template <typename Check>
inline ChildOutcome RunInForkedChild(Check &&check) {
  std::fflush(nullptr);
  const pid_t pid = fork();
  if (pid < 0) {
    return ChildOutcome::kForkFailed;
  }
  if (pid == 0) {
    check();
    _exit(0);  // reached ONLY when the check does not abort
  }
  int status = 0;
  pid_t waited = -1;
  do {
    waited = waitpid(pid, &status, 0);
  } while (waited < 0 && errno == EINTR);
  if (waited != pid) {
    return ChildOutcome::kOtherAbnormal;
  }
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    return ChildOutcome::kCleanExit;
  }
  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) {
    return ChildOutcome::kSigAbrt;
  }
  return ChildOutcome::kOtherAbnormal;
}

}  // namespace drtest
