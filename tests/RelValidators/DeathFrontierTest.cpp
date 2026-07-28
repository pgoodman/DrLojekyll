// Copyright 2026, Peter Goodman. All rights reserved.
//
// Negative-space death test for the V-INST-DRAIN death clause (keyed-instances
// D3.a.1, G-4/G-5): a kInstanceDeath whose demand net-removals frontier was
// never provisioned — no DR vec of role kNetRemoval for its demand table, or
// no `-` kFrontierFilter producer — MUST trip `CheckInstanceDeathFrontier`
// (fprintf + abort). The check is factored PURE (pointer identity over
// `op.demand_table` only, no TABLE deref), so this hand-builds a minimal flow
// with a FAKE TABLE pointer that is never dereferenced. `abort()` would kill
// the whole test process, so each arm runs the check in a fork()ed child and
// inspects the child's exit via an EINTR-safe waitpid loop (the
// GuardAnnotationFoldTest mold):
//   - death arm: death op with NO vec entry and NO producer → SIGABRT;
//   - positive arm: kNetRemoval vec entry + `-` kFrontierFilter → EXIT 0.

#include <DrTest.h>
#include <DeathHarness.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

#include "Rel.h"

namespace {

// A never-dereferenced TABLE pointer: the pure core compares identities only.
static hyde::TABLE *FakeDemandTable(void) {
  static int anchor = 0;
  return reinterpret_cast<hyde::TABLE *>(&anchor);
}

// Hand-build a flow holding one kInstanceDeath over the fake demand table.
// When `provisioned`, add the demand table's kNetRemoval vec entry AND the
// matching `-` kFrontierFilter producer (both halves of the clause's
// requirement); otherwise leave both absent.
static hyde::DRFlowGraph MakeDeathFlow(bool provisioned) {
  hyde::DRFlowGraph flow;

  hyde::DROp death(hyde::DROpKind::kInstanceDeath);
  death.instance_store_id = 0u;
  death.table_op_sign = -1;
  death.demand_table = FakeDemandTable();
  flow.ops.push_back(std::move(death));

  if (provisioned) {
    flow.table_vecs[FakeDemandTable()][hyde::VecRole::kNetRemoval] = 0u;

    hyde::DROp filter(hyde::DROpKind::kFrontierFilter);
    filter.table_op_table = FakeDemandTable();
    filter.table_op_sign = -1;
    flow.ops.push_back(std::move(filter));
  }
  return flow;
}

// The shared fork/waitpid harness (DeathHarness.h, Fable review [G]); the
// death arm accepts ONLY SIGABRT.
using drtest::ChildOutcome;

static ChildOutcome RunCheckInChild(const hyde::DRFlowGraph &flow) {
  return drtest::RunInForkedChild([&] { hyde::CheckInstanceDeathFrontier(flow); });
}

}  // namespace

// The death arm: an unprovisioned death drain (no vec, no producer) MUST trip
// the clause — and the trip must be the ValidatorFail abort (SIGABRT).
TEST(RelValidators, DeathFrontierTripsOnMissingProvision) {
  const hyde::DRFlowGraph flow = MakeDeathFlow(/*provisioned=*/false);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(flow)));
}

// The positive control: vec entry + `-` frontier-filter producer present —
// the check must return cleanly (no abort).
TEST(RelValidators, DeathFrontierAcceptsProvisionedDrain) {
  const hyde::DRFlowGraph flow = MakeDeathFlow(/*provisioned=*/true);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kCleanExit),
            static_cast<int>(RunCheckInChild(flow)));
}
