# Next Session: CostModel Adjudication and One Grounded Slice

Continue work in the Dr. Lojekyll repository. This is experimental compiler
work, but the governing discipline is reality grounding through independent
tests, differential execution, reviewed goldens, explicit negative witnesses,
and reproducible measurements. A numeric result is not evidence merely because
it is numeric.

Do not assume the previous CostModel proposal is implementation-ready. The next
session must first adjudicate its product boundary and the unfinished Rel
migration, then land at most one falsifiable vertical slice if its prerequisites
can be made honest.

## Repository and Operating Rules

1. Work from the current checked-out branch. At the start, record `git status`,
   the branch, and `git log -1`. The audit began on branch `keyed-instances` at
   `d6904ae3`, but that is historical context, not a checkout instruction.
2. Treat the repository as greenfield. Do not add compatibility shims, parallel
   old/new CostModel modes, optional fallback paths, warning-and-continue
   branches, or test-only production behavior.
3. Do not make injected writers, collectors, callbacks, or policies nullable.
   Use a required protocol and a no-op implementation, or return an explicit
   value to the caller.
4. Use named domain types for view/table/column identities, row and event
   counts, counter values, signs, strata, and prediction classifications. Do
   not export a raw `unsigned`, `float`, pointer, or untyped counter map as a new
   architectural boundary.
5. Never bless a golden merely to make a test pass. Derive the expected result
   by hand or from an independent implementation first, inspect the diff, then
   bless deliberately.
6. Build and test a given build tree sequentially. Never run tests against a
   tree while it is being rebuilt.
7. Do not commit or push unless the owner explicitly asks. If asked, use
   repository-local identity `Peter Goodman <peter.goodman@gmail.com>`, inspect
   the staged diff, and keep tool/model/provider details out of commits and
   repository artifacts.

## Mandatory Reading

Read the persistent audit first, end to end:

1. [`audit/README.md`](audit/README.md)
2. [`audit/architecture-code.md`](audit/architecture-code.md)
3. [`audit/rel-migration-state.md`](audit/rel-migration-state.md)
4. [`audit/cost-model-findings.md`](audit/cost-model-findings.md)
5. [`audit/quantity-audit.md`](audit/quantity-audit.md)
6. [`audit/testing-grounding.md`](audit/testing-grounding.md)
7. [`audit/test-coverage-gaps.md`](audit/test-coverage-gaps.md)
8. [`audit/agent-harness-reality.md`](audit/agent-harness-reality.md)
9. [`audit/codebase-risk-register.md`](audit/codebase-risk-register.md)
10. [`audit/comment-code-drift.md`](audit/comment-code-drift.md)
11. [`audit/recommended-sequence.md`](audit/recommended-sequence.md)

This audit is a living log, not an authority by assertion. Re-check its anchors
against current executable code. Update it as facts are confirmed, refuted, or
made stale by implementation. Add a separate finding file when one issue needs
substantial evidence or examples, and link it from `audit/README.md`.

After the audit, read these as untrusted design evidence:

- `cost-simulator-seed.md`
- `measured-calibration-1.md`
- `grounding-double-join.md`
- `prov-recognizer-impl.md`
- `../CostModel.md`

The design documents contain useful observations, but code and reproducible
tests outrank their claims.

## Facts That Must Not Be Re-Litigated Without New Evidence

- The historical pipeline was effectively `DataFlow -> ControlFlow`. Rel was
  introduced as a shadow inventory and has progressively taken over important
  differential inventory, scheduling, and lowering responsibilities.
- Current Rel is not yet a clean standalone `DataFlow -> Rel -> ControlFlow`
  stage. It is built inside `Program::Build`, depends on private ControlFlow
  objects, observes eager work created before it exists, lowers through
  ControlFlow helpers, and dies with the build context.
- `Program` does not retain `DRFlowGraph`. A post-build borrowed accessor is not
  viable. The diagnostic `.rel` dump is not automatically a stable API.
- `Prov` proves conservative value-containment facts. It does not compute slice
  cardinality, fanout, key-frequency distributions, or recursive closure size.
- `gBenchCounters` lives in the generated process. A compiler-side analysis
  cannot measure it without a separate execution and report protocol.
- Bench counters span different quantity families. Concrete values and event
  order are required for probe steps, duplicates, transitions, rehashing,
  compaction, and similar stateful fields.
- The checked `mono.demand.df` inspected at the audit baseline matched the
  `df.ident_join`-disabled counterfactual, not the default recognizer-on output.
  Reverify and rename/configure artifacts before using them as evidence.
- The calibration driver referenced by prior prose was not tracked at the audit
  baseline. A measured table without a tracked reproducer is not a gate.
- `Query::Build` contains a catch-all that silently continues under `NDEBUG`.
  Release calibration is not trustworthy until that path is removed.
- Query canonicalization can stop at a silent iteration cap while still
  changing. Instrument its exit reason before treating canonical Query shape as
  a stable semantic identity.
- OptDiff is broad but not exhaustive: it has 181 small directed programs and
  four coarse optimization modes, not systematic individual-pass or feature-
  composition coverage. The full suite is not a normal CTest/CI gate.
- Current fuzzing does not establish its advertised round-trip property. The
  parser target formats nothing and reparses the original bytes; fuzz CMake
  also declares a missing backend source.
- Real-world agent effects cannot be a direct callback consequence of a
  derived fact. The eventual harness needs a durable intent/result protocol,
  stable idempotency and attempt identities, at-least-once delivery semantics,
  cancellation/late-result rules, replay, and capability enforcement.

## Product Distinction

Keep these as separate products and types:

```text
static estimator:
  program facts + workload summaries -> estimated logical work + assumptions

concrete-trace cost interpreter:
  program facts + initial state + ordered events -> exact modeled counters

runtime measurement runner:
  generated executable + identical trace -> observed BenchCounters
```

Do not call a static estimate an exact simulation. Do not return observed
`BenchCounters` as the prediction type. Do not fill unsupported predictions
with zero. A suggested report classification is:

```text
PredictionKind = Exact | Estimated | Unsupported

CounterPrediction = {
  counter_name,
  kind,
  value_or_interval,
  assumptions,
  derivation_rule,
  evidence_case
}
```

Exact integral predictions compare by equality. Estimated predictions need a
declared error metric and a calibration/held-out split. The first slice should
not introduce estimated fields merely to populate a complete report.

## Work Order

### 0. Re-establish a trustworthy baseline

Run sequentially:

1. configure/build Debug;
2. Debug CTest;
3. the relevant OptDiff witness in all four modes;
4. the full OptDiff suite if time permits;
5. configure/build/test Release after the catch-all issue is fixed;
6. sanitizer tests for touched runtime/compiler paths if a sanitizer tree is
   available and correctly configured.

Record exact commands and results in the audit log. Do not report an unrun gate
as passing. If an initial failure occurs during a concurrent or stale build,
discard it as invalid evidence and rerun cleanly before drawing conclusions.

### 1. Repair the minimum trust prerequisites

Before implementing CostModel behavior:

1. delete the `catch (...)` continuation in `Query::Build`; let failures reach
   the owning boundary;
2. instrument Query canonicalization termination and record whether the corpus
   reaches a true no-change result, the hash heuristic, or the iteration cap;
3. make the mono counter measurement reproducible from tracked source, trace,
   build flags, and a dedicated counter-enabled generated executable;
4. relabel the mono artifacts with explicit recognizer configuration and add the
   missing on/off counterpart or a deterministic generator;
5. verify the identity-join on/off structural delta in all four optimization
   modes.

Keep these changes narrowly scoped and independently testable. Do not hide a
failure behind a broader tolerance or a fallback scenario.

### 2. Adjudicate Rel's intended end state

Produce a short binding decision, grounded in code, choosing one:

```text
A. Rel becomes a complete value-semantic physical plan with one-way lowering.
B. Rel remains an internal differential scheduler; CostModel snapshots multiple
   representations and does not claim Rel completeness.
C. Rel's algorithms move into a different physical-planning owner and Rel is
   retired.
```

For each viable option, state deletion work, dependency direction, ownership,
lifetime, test strategy, and CostModel consequence. Do not choose A merely
because proposal prose calls Rel the sole authority. Do not choose B merely
because it is the current shape. If the owner must decide between viable
directions, stop implementation after presenting the concrete tradeoff.

At minimum, audit every Rel op family into this table:

| Family | Inventory author | Payload author | Schedule author | CF emitter | Independent checker | Complete? |
|---|---|---|---|---|---|---|

Use perturbation tests where practical: changing an alleged retired authority
must either have no effect or fail to compile; changing the live authority must
trip a targeted structural or runtime check.

### 3. Adjudicate the CostModel contract

Answer explicitly:

1. Is slice 1 a static estimator or a concrete-trace interpreter?
2. What represents initial state, ordered message batches, queries, demand
   calls, retractions, duplicates, and external functor behavior?
3. Which counter fields are exact, estimated, observed-only, or unsupported?
4. How does the generated process emit a machine-readable observation report?
5. Is analysis hosted as a library, compiler mode, or standalone executable?
6. What immutable, pointer-free value crosses Query/Rel/ControlFlow ownership?
7. How are semantic identities kept stable across optimizer renumbering?
8. What evidence would cause the general simulator direction to be abandoned?

Recommended slice-1 answer, subject to code verification:

```text
analysis:
  library over CostProgramSnapshot + ConcreteScenarioTrace

supported program class:
  monotone, non-recursive identity-join witness only

prediction:
  exact delta for one explicitly proven counter field
  every other field Unsupported

measurement:
  separate generated calibration executable runs the same concrete trace

comparison:
  exact integer equality on held-out traces
```

Do not reshape all of `CostModel.md` or commit to a general `.cost` grammar
until this decision is written and the first slice demonstrates that its inputs
actually determine its output.

### 4. Define the immutable experiment boundary

If the Rel adjudication does not first create a stable complete plan, prototype
a value-semantic snapshot built while Query, Rel, and ControlFlow are alive:

```text
CostProgramSnapshot:
  semantic view identities and edges
  physical tables and index key definitions
  normalized cost-relevant operations
  access plans for each supported operation
  explicit mappings among Query, Rel, and Program facts
  source/provenance for each fact
```

Requirements:

- no raw pointers, process addresses, presentation-only node numbers, or
  borrowed handles;
- deterministic serialization for review/testing;
- contradictions across representations remain reportable;
- every supported cost-bearing operation has the access-plan information its
  rule consumes;
- collection is explicit and required, with a no-op implementation for normal
  compilation if needed;
- no global dump sink and no nullable callback.

Do not parse the current `.df`/`.rel` diagnostic text as an accidental internal
API. A deliberately versionless, tested public trace/snapshot grammar is a
different decision and must be named as such.

### 5. Implement at most one exact vertical slice

The candidate is the monotone non-recursive identity-join delta. First derive
the exact runtime law from counter increment sites and generated code. The old
shorthand `delta idx_adds = F * K` is only valid if duplicate behavior and index
multiplicity make it valid for that concrete witness.

Required cases:

1. hand-counted positive case;
2. recognizer on/off in all four optimization modes;
3. duplicate-row negative case that tests distinct-row/set semantics;
4. empty input and repeated demand;
5. different index multiplicity;
6. fanout sweep with concrete generated values;
7. held-out keys/sizes not used to derive the rule;
8. an unsupported counter assertion proving no implicit zero/default exists.

The predictor and measurement runner must consume the same trace. Snapshot
counters at a named boundary and compare the on/off delta, not unrelated
process totals. Keep counter-enabled binaries separate from timed benchmark
binaries.

If exact `idx_adds` is not determined by the proposed snapshot and trace, add
the missing typed state or narrow the field. Do not switch the classification
to Exact by adding an analyst-supplied fanout/selectivity number.

### 6. Reassess before generalization

After the slice, write a decision record comparing:

- extending an exact trace interpreter operation family by operation family;
- building a separate statistical estimator with explicit assumptions and
  error bars;
- relying on direct measured A/B counter tests without static prediction;
- stopping because the interpreter duplicates runtime semantics or because the
  Rel migration makes the boundary premature.

Continuing to a full simulator is not the default outcome. Deletion or removal
of overclaimed proposal scope is a successful result when the experiment
falsifies it.

Only after this reassessment should `CostModel.md` be rewritten as a current
contract. Remove or demote unsupported symbolic/numeric architectures rather
than maintaining both as compatibility modes. Link every retained empirical
law to a tracked reproduction command.

### 7. Preserve the broader reality gates

CostModel is not the whole product. Keep these follow-on directions explicit
without expanding slice 1 into them:

1. Repair the inert parser round-trip fuzz target and delete or implement the
   missing backend target before claiming fuzz coverage.
2. Add pass-selection truth-table tests and a pairwise covering array over live
   passes; retain the four coarse OptDiff modes as the readable baseline.
3. Strengthen PointsTo from a nonempty smoke test to an exact or independently
   checked semantic test.
4. Use the deterministic reverse-engineering investigation coordinator in
   `audit/agent-harness-reality.md` as the flagship feature-composition test.
5. Keep external effects outside Datalog evaluation. The first coordinator
   records typed intents and scripted results; real adapters wait for a binding
   durability, idempotency, cancellation, replay, and capability decision.

Do not make the CostModel slice wait for the whole flagship application. Do not
let the narrow slice be cited as evidence that these language and product risks
are resolved.

## Stop Conditions

Stop and write an adjudication finding instead of improvising when any of these
occurs:

- the scenario summary cannot drive the actual generated program;
- a supposedly exact field depends on concrete values, order, prior state,
  hash behavior, or runtime capacity not represented by the trace;
- the snapshot would need to retain Query/Rel/ControlFlow pointers after build;
- a Rel fact has two production authors and no clear authority;
- a finite scenario family is being used to claim universal or asymptotic
  behavior;
- the only way to pass is a default, nullable collaborator, warning-and-
  continue branch, broad exception handler, or unexplained tolerance;
- a golden expectation comes solely from the implementation under test;
- the Rel end-state options remain materially different and require owner
  judgment.

## Required Deliverables

At the end of the session, leave:

1. updated persistent audit files with evidence, dispositions, and current
   status;
2. a Rel end-state decision or a concise owner-adjudication brief;
3. a CostModel product/input/output decision;
4. tracked reproduction for every measured claim retained;
5. the exact vertical slice and its tests, only if all prerequisites were met;
6. a short verification record listing commands actually run and gates not run;
7. updated test/flagship findings when implementation changes their status;
8. an updated version of this continuation prompt pointing to the audit log and
   the next unresolved decision.

The success criterion is not lines of CostModel code. It is a smaller set of
claims with stronger independent evidence, plus one exact supported behavior if
the architecture can honestly provide it.
