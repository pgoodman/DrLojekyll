// Copyright 2026, Peter Goodman. All rights reserved.
//
// Negative-space death tests for the V-INST-EFFECT effect-multiset totality of a
// kSubgraphInstantiate (D3.a.3 design-1). Before this extraction the totality
// lived ONLY inline in ValidateDROps and had NO teeth: a regression weakening the
// giant regime-split `ok` condition (a dropped counter, a flipped sign, a missing
// crossing/append, a wrong drain role) changes NO program's emitted output, so
// the golden suite is blind to it. `CheckInstantiateEffects(op, diff, input_diff)`
// is factored PURE — `diff`/`input_diff` are booleans (no TABLE deref) — so this
// hand-builds a minimal instantiate op with FAKE, never-dereferenced table
// pointers and drives the check directly. `ValidatorFail` calls `abort()`, which
// would kill the whole test process, so each death arm runs in a fork()ed child
// inspected via the shared EINTR-safe waitpid harness (DeathHarness.h); death
// arms accept ONLY SIGABRT, positive twins ONLY a clean exit.
//
// The expected effect signatures (from InstantiateEffects, Rel.cpp:778, the single
// mint authority this belt fingerprints):
//   R-MONO  (diff=0,input_diff=0): 2 drains (demand+ / input+), 1 demand,
//     1 leaf, 1 rebuild(+1), 1 emit, 1 old, 1 counter(+1); 0 crossing/append.
//   R-DIFF  (diff=1,input_diff=0): as R-MONO but 2 counters(sum 0),
//     2 crossings, 2 appends.
//   input_diff (diff=1,input_diff=1): R-DIFF + a second (net-removal) input
//     drain -> 3 drains, 2 input drains.

#include <DrTest.h>
#include <DeathHarness.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

#include "Rel.h"

namespace {

// Never-dereferenced TABLE pointers: CheckInstantiateEffects compares identities
// only (op.demand_table / op.input_table), and `diff`/`input_diff` arrive as
// booleans, so no TABLE is ever read.
static hyde::TABLE *FakeDemandTable(void) {
  static int anchor = 0;
  return reinterpret_cast<hyde::TABLE *>(&anchor);
}
static hyde::TABLE *FakeInputTable(void) {
  static int anchor = 0;
  return reinterpret_cast<hyde::TABLE *>(&anchor);
}
static hyde::TABLE *FakePubTable(void) {
  static int anchor = 0;
  return reinterpret_cast<hyde::TABLE *>(&anchor);
}

static hyde::DREffect Drain(hyde::TABLE *value_table, hyde::VecRole role) {
  hyde::DREffect fx;
  fx.kind = hyde::EffKind::kVecDrain;
  fx.value_table = value_table;
  fx.vec_role = role;
  return fx;
}
static hyde::DREffect Bare(hyde::EffKind kind) {
  hyde::DREffect fx;
  fx.kind = kind;
  return fx;
}
static hyde::DREffect Rebuild(int sign) {
  hyde::DREffect fx;
  fx.kind = hyde::EffKind::kInstanceRebuild;
  fx.sign = sign;
  return fx;
}
static hyde::DREffect Counter(int sign) {
  hyde::DREffect fx;
  fx.kind = hyde::EffKind::kCounter;
  fx.sign = sign;
  fx.klass = hyde::DerivClass::kNonRecursive;
  return fx;
}

// The op skeleton (tables only); callers fill `effects` for the regime.
static hyde::DROp MakeInstantiate(void) {
  hyde::DROp inst(hyde::DROpKind::kSubgraphInstantiate);
  inst.instance_store_id = 0u;
  inst.table_op_sign = +1;
  inst.table_op_table = FakePubTable();
  inst.demand_table = FakeDemandTable();
  inst.input_table = FakeInputTable();
  return inst;
}

// A VALID R-MONO effect set (diff=0, input_diff=0): the exact §3.3 signature.
static void FillMono(hyde::DROp &op) {
  op.effects.push_back(Drain(FakeDemandTable(), hyde::VecRole::kNetAddition));
  op.effects.push_back(Drain(FakeInputTable(), hyde::VecRole::kNetAddition));
  op.effects.push_back(Bare(hyde::EffKind::kInstanceDemand));
  op.effects.push_back(Bare(hyde::EffKind::kFlagRead));
  op.effects.push_back(Rebuild(+1));
  op.effects.push_back(Bare(hyde::EffKind::kStateEmit));
  op.effects.push_back(Bare(hyde::EffKind::kStateOld));
  op.effects.push_back(Counter(+1));
}

// A VALID R-DIFF effect set (diff=1, input_diff=0): 2 counters(sum 0), 2
// crossings, 2 appends replace the single +1 counter.
static void FillDiff(hyde::DROp &op) {
  op.effects.push_back(Drain(FakeDemandTable(), hyde::VecRole::kNetAddition));
  op.effects.push_back(Drain(FakeInputTable(), hyde::VecRole::kNetAddition));
  op.effects.push_back(Bare(hyde::EffKind::kInstanceDemand));
  op.effects.push_back(Bare(hyde::EffKind::kFlagRead));
  op.effects.push_back(Rebuild(+1));
  op.effects.push_back(Bare(hyde::EffKind::kStateEmit));
  op.effects.push_back(Bare(hyde::EffKind::kStateOld));
  op.effects.push_back(Counter(+1));
  op.effects.push_back(Counter(-1));
  op.effects.push_back(Bare(hyde::EffKind::kInIReadFrozen));
  op.effects.push_back(Bare(hyde::EffKind::kInIReadFrozen));
  op.effects.push_back(Bare(hyde::EffKind::kVecAppend));
  op.effects.push_back(Bare(hyde::EffKind::kVecAppend));
}

// A VALID input_diff effect set (diff=1, input_diff=1): R-DIFF + the net-removals
// input rebuild drain (3 drains total, 2 input drains).
static void FillInputDiff(hyde::DROp &op) {
  FillDiff(op);
  op.effects.push_back(Drain(FakeInputTable(), hyde::VecRole::kNetRemoval));
}

using drtest::ChildOutcome;

static ChildOutcome RunCheckInChild(const hyde::DROp &op, bool diff,
                                    bool input_diff) {
  return drtest::RunInForkedChild(
      [&] { hyde::CheckInstantiateEffects(op, diff, input_diff); });
}

}  // namespace

// ---- Positive controls: each regime's exact signature is accepted ----

TEST(RelValidators, InstEffectsAcceptsMonoSignature) {
  hyde::DROp op = MakeInstantiate();
  FillMono(op);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kCleanExit),
            static_cast<int>(RunCheckInChild(op, /*diff=*/false,
                                             /*input_diff=*/false)));
}

TEST(RelValidators, InstEffectsAcceptsDiffSignature) {
  hyde::DROp op = MakeInstantiate();
  FillDiff(op);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kCleanExit),
            static_cast<int>(RunCheckInChild(op, /*diff=*/true,
                                             /*input_diff=*/false)));
}

TEST(RelValidators, InstEffectsAcceptsInputDiffSignature) {
  hyde::DROp op = MakeInstantiate();
  FillInputDiff(op);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kCleanExit),
            static_cast<int>(RunCheckInChild(op, /*diff=*/true,
                                             /*input_diff=*/true)));
}

// ---- E2 (dropped counter): a diff mint with ONE counter, not two ----

// Death arm: under diff the totality requires counters==2 (sum 0). A single +1
// counter (the R-MONO count) MUST trip — the mutation that a lax `counters==1u`
// under diff would silently admit.
TEST(RelValidators, InstEffectsRejectsDiffWithSingleCounter) {
  hyde::DROp op = MakeInstantiate();
  FillDiff(op);
  // Remove the -1 counter -> counters==1, counter_signs==1 (the mono shape).
  for (auto it = op.effects.begin(); it != op.effects.end(); ++it) {
    if (it->kind == hyde::EffKind::kCounter && it->sign == -1) {
      op.effects.erase(it);
      break;
    }
  }
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(op, /*diff=*/true,
                                             /*input_diff=*/false)));
}

// ---- E1 (role flip): the demand rebuild's sign inverted ----

// Death arm: rebuild_sign must be +1 (birth). A -1 rebuild (the DEATH sign) on
// an instantiate MUST trip the totality.
TEST(RelValidators, InstEffectsRejectsNegativeRebuildSign) {
  hyde::DROp op = MakeInstantiate();
  FillMono(op);
  for (auto &fx : op.effects) {
    if (fx.kind == hyde::EffKind::kInstanceRebuild) {
      fx.sign = -1;
      break;
    }
  }
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(op, /*diff=*/false,
                                             /*input_diff=*/false)));
}

// Death arm: a wrong-role drain (kProductInput — a never-minted demand-slice
// role, the L-b1-4 class) MUST trip the per-drain role admit.
TEST(RelValidators, InstEffectsRejectsWrongDrainRole) {
  hyde::DROp op = MakeInstantiate();
  FillMono(op);
  for (auto &fx : op.effects) {
    if (fx.kind == hyde::EffKind::kVecDrain &&
        fx.value_table == FakeInputTable()) {
      fx.vec_role = hyde::VecRole::kProductInput;
      break;
    }
  }
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(op, /*diff=*/false,
                                             /*input_diff=*/false)));
}

// ---- D1 (erased kNetRemoval): an input_diff mint missing its removals drain ----

// Death arm: under input_diff the totality requires 3 drains / 2 input drains
// (the net-removals rebuild drain present). Dropping it (the R-DIFF-without-input
// shape) MUST trip — the mutation a lax `drains==2u` would admit.
TEST(RelValidators, InstEffectsRejectsInputDiffMissingRemovalDrain) {
  hyde::DROp op = MakeInstantiate();
  FillDiff(op);  // R-DIFF shape: only the + input drain, NO net-removals drain.
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(op, /*diff=*/true,
                                             /*input_diff=*/true)));
}

// ---- O-1 closure belt: a differential input over a monotone pub ----

// Death arm: input_diff && !diff is the broken O-1 closure (a deletable input
// feeding a monotone pub) and MUST trip regardless of the effect set.
TEST(RelValidators, InstEffectsRejectsInputDiffOverMonotonePub) {
  hyde::DROp op = MakeInstantiate();
  FillInputDiff(op);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(op, /*diff=*/false,
                                             /*input_diff=*/true)));
}
