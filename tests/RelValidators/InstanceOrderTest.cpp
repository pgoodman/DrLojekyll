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
#include <DeathHarness.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

#include "Rel.h"

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

// The shared fork/waitpid harness (DeathHarness.h, Fable review [G]); the
// death arm accepts ONLY SIGABRT.
using drtest::ChildOutcome;

static ChildOutcome RunCheckInChild(const hyde::DRFlowGraph &flow) {
  return drtest::RunInForkedChild([&] { hyde::CheckInstanceOrder(flow); });
}

}  // namespace

// The HP-3 death test: a plus-before-minus (instantiate pinned before death)
// MUST trip V-INST-ORDER — and the trip must be the ValidatorFail abort
// (SIGABRT), not just any abnormal termination.
TEST(RelValidators, InstOrderTripsOnPlusBeforeMinus) {
  const hyde::DRFlowGraph flow = MakeTwoOpFlow(/*instantiate_first=*/true);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(flow)));
}

// The positive control: death pinned before instantiate is well-formed — the
// check must return cleanly (no abort).
TEST(RelValidators, InstOrderAcceptsMinusBeforePlus) {
  const hyde::DRFlowGraph flow = MakeTwoOpFlow(/*instantiate_first=*/false);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kCleanExit),
            static_cast<int>(RunCheckInChild(flow)));
}
