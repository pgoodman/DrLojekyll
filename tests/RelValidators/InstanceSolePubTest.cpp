// Copyright 2026, Peter Goodman. All rights reserved.
//
// Negative-space death test for the D3.a.3 O1 relaxation of V-INST-SOLE: the
// per-pub uniqueness tally is re-keyed on (pub_table, forcing_index) so N
// adornments of one query name (which SHARE the pub model table) are admitted,
// while a same-forcing DOUBLE-mint STILL aborts. The check is factored PURE
// (`CheckInstanceSolePub`, pointer identity + forcing_index only, no TABLE
// deref), so this hand-builds minimal flows with FAKE pub-table pointers that
// are never dereferenced. The two arms prove the relaxation kept its bite:
//   - death arm (masked-negative guard, L15): two instantiates over ONE pub
//     with the SAME forcing_index MUST still trip -> child ABORTS (SIGABRT);
//   - positive arm (the N-store shape the OLD keying wrongly rejected): two
//     instantiates over one pub with DISTINCT forcing_index -> child EXIT 0.
// `abort()` would kill the whole test process, so each arm runs in a fork()ed
// child inspected via the shared waitpid harness (DeathHarness.h).

#include <DrTest.h>
#include <DeathHarness.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

#include "Rel.h"

namespace {

// A never-dereferenced pub-table pointer shared by both instantiate ops.
static hyde::TABLE *FakePub(void) {
  static int anchor = 0;
  return reinterpret_cast<hyde::TABLE *>(&anchor);
}

// One kSubgraphInstantiate over the shared pub with the given forcing_index and
// store id.
static hyde::DROp Inst(unsigned forcing_index, unsigned store_id) {
  hyde::DROp inst(hyde::DROpKind::kSubgraphInstantiate);
  inst.instance_store_id = store_id;
  inst.table_op_sign = +1;
  inst.table_op_table = FakePub();
  inst.forcing_index = forcing_index;
  return inst;
}

using drtest::ChildOutcome;

static ChildOutcome RunCheckInChild(const hyde::DRFlowGraph &flow) {
  return drtest::RunInForkedChild([&] { hyde::CheckInstanceSolePub(flow); });
}

}  // namespace

// Death arm: TWO instantiates for ONE forcing over one pub is a real double-mint
// and MUST trip even under the relaxed (pub, forcing) key (SIGABRT).
TEST(RelValidators, InstSolePubTripsOnDoubleMintOneForcing) {
  hyde::DRFlowGraph flow;
  flow.ops.push_back(Inst(/*forcing=*/0u, /*store=*/0u));
  flow.ops.push_back(Inst(/*forcing=*/0u, /*store=*/1u));  // same forcing!
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(flow)));
}

// Positive arm: two instantiates over one pub with DISTINCT forcing_index is the
// N-store multi-adornment shape (each live forcing mints one deriver into the
// shared pub) — the OLD per-pub-pointer keying wrongly rejected this; the O1 key
// PASSES it.
TEST(RelValidators, InstSolePubAcceptsTwoForcingsOnePub) {
  hyde::DRFlowGraph flow;
  flow.ops.push_back(Inst(/*forcing=*/0u, /*store=*/0u));
  flow.ops.push_back(Inst(/*forcing=*/1u, /*store=*/1u));  // distinct forcing
  ASSERT_EQ(static_cast<int>(ChildOutcome::kCleanExit),
            static_cast<int>(RunCheckInChild(flow)));
}
