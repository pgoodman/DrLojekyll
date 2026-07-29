// Copyright 2026, Peter Goodman. All rights reserved.
//
// Negative-space death tests for the D3.a.2 input-arm belt (A1.8): a keyed
// instance over a @differential (deletable) summarized input must
//   (V-INST-DRAIN input arm, E1e) provision BOTH signs' ± frontiers — the DR
//     vec of role kNetAddition/kNetRemoval AND the matching signed
//     kFrontierFilter producer; a missing `-` producer MUST trip; and
//   (V-INST-EFFECT role admit, E1d) drain its input table ONLY through a
//     net-additions or net-removals rebuild drain; any other role MUST trip.
// The belt is factored PURE over the flow (`CheckInstanceInputArm`, pointer
// identity only, no TABLE deref — the CheckInstanceDeathFrontier mold), so this
// hand-builds a minimal flow with FAKE TABLE pointers that are never
// dereferenced. A DIFFERENTIAL input is detected STRUCTURALLY (a kNetRemoval
// role in `table_vecs`, F-b1-2). `abort()` would kill the whole test process,
// so each arm runs the check in a fork()ed child and inspects the child's exit
// via the shared EINTR-safe waitpid harness (DeathHarness.h); the death arms
// accept ONLY SIGABRT.

#include <DrTest.h>
#include <DeathHarness.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

#include "Rel.h"

namespace {

// Never-dereferenced TABLE pointers: the pure belt compares identities only.
static hyde::TABLE *FakeInputTable(void) {
  static int anchor = 0;
  return reinterpret_cast<hyde::TABLE *>(&anchor);
}
static hyde::TABLE *FakeDemandTable(void) {
  static int anchor = 0;
  return reinterpret_cast<hyde::TABLE *>(&anchor);
}
static hyde::TABLE *FakePubTable(void) {
  static int anchor = 0;
  return reinterpret_cast<hyde::TABLE *>(&anchor);
}

// A signed kFrontierFilter producer for `table` (the both-sign provisioning
// half `dr_ok` looks for).
static hyde::DROp Filter(hyde::TABLE *table, int sign) {
  hyde::DROp filter(hyde::DROpKind::kFrontierFilter);
  filter.table_op_table = table;
  filter.table_op_sign = sign;
  return filter;
}

// Base flow: one kSubgraphInstantiate over a DIFFERENTIAL input (both ± roles
// present in `table_vecs`, so the belt classifies it differential) plus its
// `+` frontier-filter producer. Callers add the `-` producer / wrong-role
// effects to shape the arm under test.
static hyde::DRFlowGraph MakeDiffInputFlow(void) {
  hyde::DRFlowGraph flow;

  hyde::DROp inst(hyde::DROpKind::kSubgraphInstantiate);
  inst.instance_store_id = 0u;
  inst.table_op_sign = +1;
  inst.table_op_table = FakePubTable();
  inst.demand_table = FakeDemandTable();
  inst.input_table = FakeInputTable();
  flow.ops.push_back(std::move(inst));

  // Differential input: both ± roles minted in the DR inventory (F-b1-2).
  flow.table_vecs[FakeInputTable()][hyde::VecRole::kNetAddition] = 0u;
  flow.table_vecs[FakeInputTable()][hyde::VecRole::kNetRemoval] = 0u;

  // The `+` producer is always present; the `-` producer is the variable.
  flow.ops.push_back(Filter(FakeInputTable(), +1));
  return flow;
}

// A valid input drain effect (net-additions or net-removals) on the instantiate.
static void AddInputDrain(hyde::DROp &inst, hyde::VecRole role) {
  hyde::DREffect fx;
  fx.kind = hyde::EffKind::kVecDrain;
  fx.value_table = inst.input_table;
  fx.vec_role = role;
  inst.effects.push_back(fx);
}

using drtest::ChildOutcome;

static ChildOutcome RunCheckInChild(const hyde::DRFlowGraph &flow) {
  return drtest::RunInForkedChild([&] { hyde::CheckInstanceInputArm(flow); });
}

}  // namespace

// ---- V-INST-DRAIN input arm (E1e): a missing `-` kFrontierFilter producer ----

// Death arm: a differential input whose net-REMOVALS frontier filter is absent
// (only the `+` producer exists) MUST trip the input arm's both-signs check.
TEST(RelValidators, InputArmRejectsMissingRemovalProducer) {
  hyde::DRFlowGraph flow = MakeDiffInputFlow();
  // Valid input drains, so the effect-role belt (a) passes and (b) is the sole
  // failure surface. NO `-` frontier-filter producer added.
  AddInputDrain(flow.ops[0], hyde::VecRole::kNetAddition);
  AddInputDrain(flow.ops[0], hyde::VecRole::kNetRemoval);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(flow)));
}

// Positive twin: BOTH signed producers present — the input arm is satisfied.
TEST(RelValidators, InputArmAcceptsBothSignProducers) {
  hyde::DRFlowGraph flow = MakeDiffInputFlow();
  AddInputDrain(flow.ops[0], hyde::VecRole::kNetAddition);
  AddInputDrain(flow.ops[0], hyde::VecRole::kNetRemoval);
  flow.ops.push_back(Filter(FakeInputTable(), -1));  // the missing half
  ASSERT_EQ(static_cast<int>(ChildOutcome::kCleanExit),
            static_cast<int>(RunCheckInChild(flow)));
}

// ---- V-INST-EFFECT input drain role admit (E1d): a wrong-role input drain ----

// Death arm: a differential input, fully ± provisioned, but drained through a
// role that is neither kNetAddition nor kNetRemoval — a never-minted
// kProductInput-class role — MUST trip the effect-role admit.
TEST(RelValidators, EffectCountRejectsWrongInputDrainRole) {
  hyde::DRFlowGraph flow = MakeDiffInputFlow();
  flow.ops.push_back(Filter(FakeInputTable(), -1));  // fully provisioned (b) ok
  // WRONG role on the input drain: kProductInput is never a demand-slice input
  // drain role (the L-b1-4 never-minted-role class).
  AddInputDrain(flow.ops[0], hyde::VecRole::kProductInput);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(flow)));
}

// Positive twin: input_drains == 2 (net-additions + net-removals) — the roles
// are admitted and the provisioning is complete.
TEST(RelValidators, EffectCountAcceptsNetAddAndNetRemovalDrains) {
  hyde::DRFlowGraph flow = MakeDiffInputFlow();
  flow.ops.push_back(Filter(FakeInputTable(), -1));
  AddInputDrain(flow.ops[0], hyde::VecRole::kNetAddition);
  AddInputDrain(flow.ops[0], hyde::VecRole::kNetRemoval);
  ASSERT_EQ(static_cast<int>(ChildOutcome::kCleanExit),
            static_cast<int>(RunCheckInChild(flow)));
}
