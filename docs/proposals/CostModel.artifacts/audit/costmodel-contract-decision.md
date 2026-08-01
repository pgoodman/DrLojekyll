# CostModel Contract Decision (Work Order 3) + Slice Reassessment (Work Order 6)

> Status: 2026-07-31, branch `keyed-instances`, tip after `61b17a15`. Binding for
> the next slice; supersedes the seed's single `cost/calibrate/judge` API and the
> symbolic framing of `CostModel.md` §1–§3. Grounded in the measurement slice
> that landed this session (`calibration-slice.md`) and the Rel adjudication
> (`rel-endstate-decision.md`).

## The eight contract questions, answered

1. **Static estimator or concrete-trace interpreter?** Neither, for slice 1. The
   only product that landed is the **runtime measurement runner** (Product
   Distinction, third box). It is the honest floor: it consumes a concrete trace
   and reports observed counters. The static estimator and the trace interpreter
   are DEFERRED (see reassessment below) because both need the value-semantic
   snapshot, which is gated on the Rel boundary decision.
2. **What represents initial state, batches, queries, demand, retractions,
   duplicates, functors?** For the measurement runner: a parametric C++ driver
   (`calib/calib_driver.cpp`) that expresses the trace directly — ingest epochs,
   `DUP` duplicate multiplicity, `K` demand probes, `REPEAT` re-probes. There is
   NO `.cost` summary grammar; a scenario is executable trace code, which is the
   only thing that can drive the generated program (cost-model-findings.md #10).
   Retractions/negation/aggregates/external functors are out of the mono slice's
   program class and are not represented yet.
3. **Which counter fields are exact/estimated/observed-only/unsupported?** In the
   measurement product every field is **observed** (a measured `gBenchCounters`
   delta). The one field with a *checked law* is `idx_adds`:
   `idx_adds(OFF) − idx_adds(ON) == F·min(N,K)`, verified by integer equality
   over a sweep. No field is estimated (no analyst-supplied selectivity/fanout
   enters — F and K are trace inputs). No field is silently zeroed.
4. **How does the generated process emit a machine-readable report?** The driver
   prints `PARAMS …` then one `COUNTER <field> <delta>` line per
   `HYDE_RT_BENCH_COUNTER_FIELDS` entry, snapshotting `hyde::rt::gBenchCounters`
   around the measured epoch. The runner parses these lines. The COUNTS binary is
   `-DDRLOJEKYLL_BENCH_COUNTERS` and is never a timed binary.
5. **Library, compiler mode, or standalone executable?** For slice 1: a **shell
   runner + a compiled driver**, no new compiler mode and no `bin/Cost`. This is
   deliberate — a `bin/Cost` is only justified once a static predictor exists,
   and that is gated. The measurement product does not need in-process compiler
   linkage.
6. **What immutable, pointer-free value crosses Query/Rel/ControlFlow
   ownership?** Nothing yet — the measurement runner reads only the generated
   process's counters, so it never touches compiler internals. This is the
   property that makes it honest and unblocked. The moment a static predictor is
   added, the answer becomes `CostProgramSnapshot` (Work Order 4), and that is
   where the Rel decision bites.
7. **How are semantic identities kept stable across renumbering?** The
   measurement product addresses facts by *runtime observable* (counter field
   name + the trace's own F/K/N), never by dump node ids like `join.7`
   (cost-model-findings.md #18). The mono artifacts are now named by
   configuration (`mono.demand.ident-join-{on,off}.*`), not by node number.
8. **What evidence would abandon the general-simulator direction?** Stated in the
   reassessment below.

## Recommended slice-1 answer — as landed

```text
product:      runtime measurement runner (shell runner + parametric driver)
domain:       monotone, non-recursive, single-adornment identity-join witness
law checked:  idx_adds(OFF) − idx_adds(ON) == F·min(N,K)   [exact integer eq]
other fields: observed-only, reported, not asserted
measurement:  dedicated -DDRLOJEKYLL_BENCH_COUNTERS build, separate from timing
comparison:   exact integer equality over held-out (N,K,F) and negatives
```

This differs from the recommended-sequence Stage 1 answer in one honest way: it
does **not** yet pair a static prediction with the measurement. The prediction
half is where `CostProgramSnapshot` + the Rel decision are required, and building
it now would either (a) freeze the migration scaffolding into an API, or (b)
smuggle in analyst-supplied numbers to look "exact". Both are stop conditions.

## What is NOT built, and why (no silent gaps)

- **Static predictor / `CostProgramSnapshot`** (Work Order 4): gated on the Rel
  end-state recommendation (B) being ratified. Until then the predictor would
  need either a post-build `DRFlowGraph` accessor (does not exist; unsafe) or
  `.rel` text parsing (accidental API). Deferred deliberately, not forgotten.
- **Index-multiplicity case** (Stage-3 #5): the mono witness has one relevant
  index, so `idx_adds` multiplicity is 1. Proving the multiplicity term needs a
  witness whose queried relation carries ≥2 indexes. Listed as the next
  measurement case, not claimed.
- **Unsupported-field / no-implicit-zero assertion** (Stage-3 #8): this is a
  property of the *prediction report type* (`PredictionKind = Exact | Estimated
  | Unsupported`). With no predictor, there is nothing to zero. Deferred with the
  predictor; when it lands, the report type must make an unsupported field a
  distinct value, never `0`.

## Reassessment: is a general simulator still the goal? (Work Order 6)

Comparing the four continuations the prompt names:

- **Extend an exact trace interpreter op-family by op-family.** Viable but
  premature: it requires the snapshot + a chosen Rel boundary first. Each family
  (ingest, probe, eager join, differential round, aggregate, keyed instance)
  needs its own hand-derived law + measured positive + negative + unsupported.
  The mono slice shows one family costs one law; extrapolating to ~29 op kinds is
  a large program.
- **Separate statistical estimator with error bars.** Only honest for fields
  driven by selectivity/fanout/closure — which are *estimates*, never exact. Keep
  strictly separate from the exact measurement product; never let an estimated
  field wear an equality assertion.
- **Direct measured A/B counter tests without a static predictor.** This is what
  landed, and it is the strongest evidence-per-effort. It already catches the
  regression the whole exercise cares about ("a 'do less' feature that does
  more") — the recognizer's `idx_adds` win is measured, not asserted.
- **Stop because the interpreter duplicates runtime semantics.** A real risk: an
  exact trace interpreter that reproduces `gBenchCounters` field-by-field IS a
  second copy of the runtime. Every field it models exactly is a field the
  measurement runner already gives for free by running the real code.

**Verdict.** The default outcome is **NOT** a general simulator. The measurement
runner is the durable core. A static predictor is worth building only where it
answers a question measurement cannot — namely *predicting* a regression before
paying to run it, on a program too large or too external to run cheaply. Until a
concrete caller needs that, extend the measurement product (index multiplicity,
then a second op family) and keep the symbolic/numeric `CostModel.md`
architecture DEMOTED, not maintained as a parallel mode.

**Abandon-the-simulator evidence, made concrete:** abandon the general static
simulator if any of these hold when the predictor is first attempted — (a) a
supposedly-exact field turns out to depend on hash occupancy / insertion order /
prior state not in the trace (probe_steps already does — quantity-audit.md); (b)
the snapshot cannot derive the field without an analyst-supplied
selectivity/fanout number; (c) reproducing the field exactly requires
re-implementing a runtime data-structure invariant (i.e. it duplicates the
runtime). In all three, the measured A/B test is the correct instrument and the
static prediction is deleted, not tolerance-fudged.

## Documentation cutover (deferred, not done)

`CostModel.md` §1–§3 (symbolic semiring / polynomial normalizer) and the seed's
unified `cost/calibrate/judge` API remain DEMOTED design evidence. Rewriting
`CostModel.md` as a current contract should wait until either the predictor lands
or the owner ratifies Rel direction B — rewriting it now would document a
contract the code does not yet keep. This decision file is the current contract
until then.
