// Copyright 2026, Peter Goodman. All rights reserved.
//
// Negative-space death test for V-INST-ORDER (keyed-instances D1.b, HP-3): a
// deliberately mis-minted plus-before-minus — a kSubgraphInstantiate pinned
// BEFORE its paired kInstanceDeath — MUST trip `CheckInstanceOrder`
// (fprintf + abort). The check is factored PURE (it reads only op.kind /
// instance_store_id / pinned_order), so this hand-builds a minimal 2-op
// DRFlowGraph with no real TABLE/Context. `CheckInstanceOrder` calls
// `abort()`, which would kill the whole test process, so each arm runs the
// check in a fork()ed child and inspects the child's exit via waitpid:
//   - death arm: instantiate pinned before death → child must ABORT (SIGABRT);
//   - positive arm: death pinned before instantiate → child must EXIT 0.

#include <DrTest.h>

#include <csignal>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

#include "DeltaRel.h"

namespace {

// A minimal 2-op flow: a kSubgraphInstantiate + a kInstanceDeath sharing one
// `instance_store_id`, with an explicit `pinned_order` encoding the emission
// order under test. ops[0] is always the instantiate, ops[1] the death; only
// the pinned order flips.
static hyde::DRFlowGraph MakeTwoOpFlow(bool instantiate_first) {
  hyde::DRFlowGraph flow;

  hyde::DROp inst(hyde::DROpKind::kSubgraphInstantiate);
  inst.instance_store_id = 0u;
  inst.table_op_sign = +1;

  hyde::DROp death(hyde::DROpKind::kInstanceDeath);
  death.instance_store_id = 0u;
  death.table_op_sign = -1;

  flow.ops.push_back(std::move(inst));   // op index 0
  flow.ops.push_back(std::move(death));  // op index 1

  if (instantiate_first) {
    flow.pinned_order = {0u, 1u};  // WRONG: instantiate precedes death
  } else {
    flow.pinned_order = {1u, 0u};  // RIGHT: death precedes instantiate
  }
  return flow;
}

// Child-termination classes the arms assert on. kForkFailed makes a fork()
// failure loud in BOTH arms instead of masquerading as either outcome.
enum class ChildOutcome { kCleanExit, kSigAbrt, kOtherAbnormal, kForkFailed };

// Run `CheckInstanceOrder(flow)` in a forked child and classify how the child
// terminated. ValidatorFail is fprintf + abort(), so the ONLY termination the
// death arm accepts is SIGABRT specifically — any other crash (e.g. a future
// deref regression on the trip path) must turn the test red, not pass as a
// green death. stdout is flushed before fork so the child's abort-time flush
// cannot replay the parent's buffered DrTest progress lines into the ctest log.
static ChildOutcome RunCheckInChild(const hyde::DRFlowGraph &flow) {
  std::fflush(nullptr);
  const pid_t pid = fork();
  if (pid < 0) {
    return ChildOutcome::kForkFailed;
  }
  if (pid == 0) {
    hyde::CheckInstanceOrder(flow);
    _exit(0);  // reached ONLY when the check does not abort
  }
  int status = 0;
  (void) waitpid(pid, &status, 0);
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    return ChildOutcome::kCleanExit;
  }
  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) {
    return ChildOutcome::kSigAbrt;
  }
  return ChildOutcome::kOtherAbnormal;
}

}  // namespace

// The HP-3 death test: a plus-before-minus (instantiate pinned before death)
// MUST trip V-INST-ORDER — and the trip must be the ValidatorFail abort
// (SIGABRT), not just any abnormal termination.
TEST(DeltaRelValidators, InstOrderTripsOnPlusBeforeMinus) {
  const hyde::DRFlowGraph flow = MakeTwoOpFlow(/*instantiate_first=*/true);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(flow)));
}

// The positive control: death pinned before instantiate is well-formed — the
// check must return cleanly (no abort).
TEST(DeltaRelValidators, InstOrderAcceptsMinusBeforePlus) {
  const hyde::DRFlowGraph flow = MakeTwoOpFlow(/*instantiate_first=*/false);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kCleanExit),
            static_cast<int>(RunCheckInChild(flow)));
}
