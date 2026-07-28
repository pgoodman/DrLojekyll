// Copyright 2026, Peter Goodman. All rights reserved.
//
// Negative-space death test for the OWN-3 promoted guard-annotation fold
// diagnostic (keyed-instances D3.a.0): two annotated guards folding into one
// survivor with DIFFERENT fold-invariant identity (forcing_index or
// instance_key) MUST trip `CheckGuardAnnotationFold` (fprintf + abort). The
// check is factored PURE (it reads only the two GuardAnnotation records), so
// this hand-builds records with no QueryImpl/view graph -- the two QueryView
// handle slots are filled with NULL handles (never dereferenced; the
// diagnostic prints them as raw %p). `CheckGuardAnnotationFold` calls
// `abort()`, which would kill the whole test process, so each arm runs the
// check in a fork()ed child and inspects the child's exit via waitpid:
//   - death arm: distinct forcings collapse -> child must ABORT (SIGABRT);
//   - positive arm: same forcing + key, only site metadata (demand_side)
//     differs -> child must EXIT 0.

#include <DrTest.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "Query.h"

namespace {

// Hand-build one GuardAnnotation record. `QueryView` is not
// default-constructible (its handle ctor takes an impl pointer), so an
// aggregate-`{}` skeleton does not compile; every field is set explicitly
// here -- which also avoids the value-init trap where `loser{}` and
// `survivor{}` would both zero `forcing_index` and compare COMPATIBLE,
// false-greening the death arm.
static hyde::GuardAnnotation MakeAnn(unsigned forcing_index,
                                     std::vector<unsigned> instance_key,
                                     hyde::GuardAnnotation::DemandSide side) {
  return hyde::GuardAnnotation{
      hyde::GuardAnnotation::kReadAtTuple,
      side,
      hyde::GuardAnnotation::kBody,
      false /* is_instance_key: always false this slice */,
      std::move(instance_key),
      hyde::QueryView(nullptr) /* guarded_read: null handle, never deref'd */,
      hyde::QueryView(nullptr) /* demanded_view: null handle, never deref'd */,
      forcing_index};
}

// Child-termination classes the arms assert on. kForkFailed makes a fork()
// failure loud in BOTH arms instead of masquerading as either outcome.
enum class ChildOutcome { kCleanExit, kSigAbrt, kOtherAbnormal, kForkFailed };

// Run `CheckGuardAnnotationFold(loser, survivor)` in a forked child and
// classify how the child terminated. The check is fprintf + abort(), so the
// ONLY termination the death arm accepts is SIGABRT specifically -- any other
// crash (e.g. a future deref regression on the trip path) must turn the test
// red, not pass as a green death. stdout is flushed before fork so the
// child's abort-time flush cannot replay the parent's buffered DrTest
// progress lines into the ctest log.
static ChildOutcome RunCheckInChild(const hyde::GuardAnnotation &loser,
                                    const hyde::GuardAnnotation &survivor) {
  std::fflush(nullptr);
  const pid_t pid = fork();
  if (pid < 0) {
    return ChildOutcome::kForkFailed;
  }
  if (pid == 0) {
    hyde::CheckGuardAnnotationFold(loser, survivor);
    _exit(0);  // reached ONLY when the check does not abort
  }
  // waitpid failure must be LOUD (the fork() treatment extended): with an
  // unchecked -1 return, `status` stays 0 and WIFEXITED would classify the
  // arm as a clean exit -- a vacuous green. Retry EINTR; any other failure
  // (e.g. ECHILD under SIG_IGN-SIGCHLD auto-reaping) classifies as abnormal.
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

}  // namespace

// The OWN-3 death test: two guards of DISTINCT forcings collapsing into one
// survivor is a mis-keyed instance -- the fold check MUST abort (SIGABRT
// specifically, the fprintf + abort idiom, not just any abnormal exit).
TEST(DataFlowValidators, GuardFoldTripsOnDistinctForcings) {
  const hyde::GuardAnnotation loser =
      MakeAnn(0u, {}, hyde::GuardAnnotation::kDReader);
  const hyde::GuardAnnotation survivor =
      MakeAnn(1u, {}, hyde::GuardAnnotation::kRawSeed);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(loser, survivor)));
}

// The positive control: same forcing + same instance key, only the PRE-CSE
// site stamp (demand_side) differs -- the legitimate raw-seed-into-d-reader
// fold shape. The check must return cleanly (no abort).
TEST(DataFlowValidators, GuardFoldAcceptsSameForcingSameKey) {
  const hyde::GuardAnnotation a =
      MakeAnn(7u, {2u, 3u}, hyde::GuardAnnotation::kDReader);
  const hyde::GuardAnnotation b =
      MakeAnn(7u, {2u, 3u}, hyde::GuardAnnotation::kRawSeed);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kCleanExit),
            static_cast<int>(RunCheckInChild(a, b)));
}
