# CostModel and Architecture Audit

> Status: code-derived audit at branch `keyed-instances`, inspected at
> `d6904ae3` on 2026-07-31. This is a living record. Re-check anchors against
> the current tip before relying on line numbers or status claims.

This directory records the audit that precedes the next CostModel session. It
exists because the current proposal, its seed, and its continuation prompt mix
verified code facts, stale artifacts, intended designs, and unimplemented
assumptions. Those categories must remain distinct if testing is to ground the
experimental work in reality.

## Read Order

1. [`architecture-code.md`](architecture-code.md): executable architecture and
   algorithm labels derived from entry points, types, calls, loops, and state.
2. [`rel-migration-state.md`](rel-migration-state.md): what the Rel cutover has
   actually replaced, what remains ControlFlow-authored, and the plausible end
   states that still require an owner decision.
3. [`cost-model-findings.md`](cost-model-findings.md): split-brain findings,
   ordered by severity, with repair classification.
4. [`quantity-audit.md`](quantity-audit.md): the quantities the proposed model
   combines, what can be predicted, and where primitive types collapse meaning.
5. [`testing-grounding.md`](testing-grounding.md): which tests are independent
   evidence, which are self-consistency checks, and which gates are not
   automated.
6. [`test-coverage-gaps.md`](test-coverage-gaps.md): measured OptDiff coverage,
   sparse feature intersections, pass-matrix gaps, application-test weakness,
   and the currently inert fuzzing claims.
7. [`agent-harness-reality.md`](agent-harness-reality.md): a useful
   reverse-engineering investigation coordinator and the durability,
   idempotency, cancellation, replay, and capability decisions required before
   derived facts can safely cause real-world effects.
8. [`codebase-risk-register.md`](codebase-risk-register.md): broader release,
   termination, configurability, optionality, and ownership risks exposed by
   the CostModel audit.
9. [`comment-code-drift.md`](comment-code-drift.md): independent code/comment
   architecture diff and high-value cleanup targets.
10. [`recommended-sequence.md`](recommended-sequence.md): a staged path that can
   fail honestly before a large simulator is built.

The session handoff is
[`../next-session-prompt.md`](../next-session-prompt.md). It must continue to
reference this index.

## Severity and Disposition

Each finding uses one of these dispositions:

- **Fix now**: local, well-understood defect with a clear contract.
- **Design decision**: multiple coherent designs exist; implementation should
  wait for an explicit choice.
- **Research question**: the requested guarantee cannot yet be stated or
  tested honestly. Build an experiment before choosing an architecture.
- **Remove claim/scope**: the proposal asserts something current evidence does
  not support. Delete or narrow it rather than adding a fallback.

## Executive Result

The compiler is a real incremental Datalog engine with unusually strong manual
differential and golden testing. The CostModel direction is useful, especially
as a way to expose regressions in "do less" transformations, but the current
numeric-simulator plan is not implementation-ready.

The central issue is not symbolic versus numeric representation. It is whether
the tool is:

```text
static estimator:
  IR + summarized workload statistics -> estimated logical work

trace cost interpreter:
  IR + concrete event trace -> exact modeled operation counts

runtime measurement harness:
  generated executable + concrete event trace -> measured BenchCounters
```

The proposal currently claims all three while specifying none of their
boundaries completely. A number is not automatically more grounded than a
symbol. A numeric value derived from analyst-supplied selectivity, closure size,
and retraction fanout remains an estimate.

The recommended first deliverable is therefore not the full `.cost` grammar or
a complete `bin/Cost`. It is one reproducible, checked-in calibration case that
predicts a deliberately small exact counter subset and compares it with a
separate measured run. The first target can remain the identity-join delta, but
the missing measurement driver and mislabeled artifacts must be repaired first.

There is a prior architectural decision to make as well. Rel is a live,
partially completed replacement of the direct DataFlow-to-ControlFlow builder,
not a settled standalone layer. A CostModel must not accidentally freeze the
current migration scaffolding into a public contract. The Rel end state and the
CostModel observation boundary need to be adjudicated together.

The broader test surface is strong in small directed witnesses but sparse in
feature composition and individual-pass combinations. The eventual agent and
reverse-engineering use case also adds a separate reality boundary: derived
facts can express effect intent, but cannot safely stand in for durable,
idempotent execution in the outside world. The proposed investigation
coordinator makes that boundary testable without pretending that deterministic
fake adapters provide production durability.

## Verification Performed During This Audit

- Clean tracked worktree at the initial inspection.
- Current debug tree rebuilt successfully after an accidental concurrent build
  was allowed to settle.
- `ctest --test-dir build/debug --output-on-failure`: 6/6 passed.
- Current release tree built successfully; Release CTest passed 6/6 with
  `ctest --test-dir build/release --output-on-failure`.
- Full OptDiff passed 181/181 with `DR=build/debug/bin/drlojekyll
  tests/OptDiff/runall.sh /tmp/drlojekyll-costmodel-audit-optdiff-20260731 4`.
- Current mono witness regenerated with `df.ident_join` on and off. The checked
  in `mono.demand.df` is byte-identical to the current **off** form, not the
  default current **on** form.
- Exception scan found one production catch-all in `Query::Build`.
- Configuration scan found a registered but empty `df.sink` pass.
- Fuzz audit found an inert parser round-trip property, a missing
  `BackendFuzzer.cpp` named by CMake, and a disabled workflow using obsolete
  option names.
- Sanitizer tests and a libFuzzer build were not run. The latter is expected to
  be unconfigurable until the missing backend target is deleted or implemented;
  this expectation was code-inspected, not counted as an executed failure.
- Comment-history scan found 516 matching source-comment lines across 80 files;
  this is a candidate count, not a claim that every line is wrong.

An earlier test attempt ran while the same build tree was being rebuilt and
produced transient failures. Those failures are invalid evidence and are not
used by this audit.
