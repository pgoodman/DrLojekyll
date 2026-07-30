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
#include <DeathHarness.h>

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
static hyde::GuardAnnotation MakeAnn(
    unsigned forcing_index, std::vector<unsigned> instance_key,
    hyde::GuardAnnotation::DemandSide side,
    hyde::GuardAnnotation::Role role = hyde::GuardAnnotation::kBody) {
  return hyde::GuardAnnotation{
      hyde::GuardAnnotation::kReadAtTuple,
      side,
      role,
      false /* is_instance_key: always false this slice */,
      std::move(instance_key),
      hyde::QueryView(nullptr) /* guarded_read: null handle, never deref'd */,
      hyde::QueryView(nullptr) /* demanded_view: null handle, never deref'd */,
      forcing_index};
}

// The shared fork/waitpid harness (DeathHarness.h, Fable review [G]); the
// death arm accepts ONLY SIGABRT.
using drtest::ChildOutcome;

static ChildOutcome RunCheckInChild(const hyde::GuardAnnotation &loser,
                                    const hyde::GuardAnnotation &survivor) {
  return drtest::RunInForkedChild(
      [&] { hyde::CheckGuardAnnotationFold(loser, survivor); });
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

// g1 SURVIVOR-RECORD POLICY (D3.a.3): CSE can make a kQueryProjection guard the
// survivor of a kBody loser (Optimize.cpp:365 orders by depth/det_seq, not
// role). The policy must promote the survivor back to kBody so
// ResolveLiveRecognition (Rel.cpp:976) still finds the body input. Directed
// witness (a): force the exact qp-survivor shape and assert the repair. The
// policy never aborts, so this is a plain ASSERT_EQ (no fork). Removing/
// regressing the policy fails this at ctest -- the fold-time teeth.
TEST(DataFlowValidators, SurvivorPolicyRepairsQueryProjectionSurvivor) {
  const hyde::GuardAnnotation loser =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kDReader,
              hyde::GuardAnnotation::kBody);
  hyde::GuardAnnotation surv =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kRawSeed,
              hyde::GuardAnnotation::kQueryProjection);
  hyde::PromoteSurvivorToBody(surv, loser);
  ASSERT_EQ(static_cast<int>(surv.role),
            static_cast<int>(hyde::GuardAnnotation::kBody));
  ASSERT_EQ(static_cast<int>(surv.demand_side),
            static_cast<int>(hyde::GuardAnnotation::kDReader));  // adopted stamp
  ASSERT_EQ(static_cast<int>(surv.kind), static_cast<int>(loser.kind));
}

// No-op arm: the policy must NOT disturb a survivor that already carries kBody.
TEST(DataFlowValidators, SurvivorPolicyNoOpWhenSurvivorAlreadyBody) {
  const hyde::GuardAnnotation loser =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kRawSeed,
              hyde::GuardAnnotation::kQueryProjection);
  hyde::GuardAnnotation surv =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kDReader,
              hyde::GuardAnnotation::kBody);
  hyde::PromoteSurvivorToBody(surv, loser);
  ASSERT_EQ(static_cast<int>(surv.role),
            static_cast<int>(hyde::GuardAnnotation::kBody));
}

// No-op arm: the policy must NOT invent a body where neither side had one.
TEST(DataFlowValidators, SurvivorPolicyNoOpWhenBothQueryProjection) {
  const hyde::GuardAnnotation loser =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kDReader,
              hyde::GuardAnnotation::kQueryProjection);
  hyde::GuardAnnotation surv =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kRawSeed,
              hyde::GuardAnnotation::kQueryProjection);
  hyde::PromoteSurvivorToBody(surv, loser);
  ASSERT_EQ(static_cast<int>(surv.role),
            static_cast<int>(hyde::GuardAnnotation::kQueryProjection));
}

// The instance_key belt is KEPT (D3.a.3): two guards of the SAME forcing but
// DIFFERENT instance_key are a mis-keyed instance and MUST abort (SIGABRT) --
// proves the KEPT `instance_key` conjunct stays armed under multi-adornment.
TEST(DataFlowValidators, GuardFoldTripsOnSameForcingDifferentKey) {
  const hyde::GuardAnnotation loser =
      MakeAnn(7u, {2u, 3u}, hyde::GuardAnnotation::kDReader);
  const hyde::GuardAnnotation survivor =
      MakeAnn(7u, {2u, 4u}, hyde::GuardAnnotation::kRawSeed);  // key differs
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(loser, survivor)));
}
