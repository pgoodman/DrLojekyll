<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-37 charter — S1b tail: bench the pruning; then S1c or Stage C (owner re-ranks)

> **AMENDED at s36 close: S1b's CORE LANDED IN s36 ITSELF (CP4 `047a991b`) —
> the injector (§1) AND the witness (§2) are DONE, gate green (SUITE PASS 228,
> ctest 5/5, answer-correct ×4 modes, flag-off byte-identical). What remains
> of this charter is §3 (the bench carrier — the MEASURED selective-pruning
> proof + the non-selective regression) and the seed §5 residue (S1c Tier-1
> demanded-interior naming; S2+ InstanceStore). The InstanceFlow Stage-C plan
> stays parked in `session-37-seed.md`. The owner re-ranks: bench+S1c vs
> Stage C. The original charter below is kept for §3's instructions.**

You are resuming work on branch `keyed-instances` (Dr. Lojekyll, the `hyde`
C++ Datalog compiler). **Owner redirect (2026-08-13, s36 close): this session
is S1b — the flat `-demand` injector seed-wiring — NOT InstanceFlow Stage C**
(the allocation inversion is scoped and PARKED in `session-37-seed.md`; do not
enact it). S1a (s30) resurrected the flat `-demand` transform: flag-off
byte-identical, and a `-demand` recursive-TC program COMPILES end-to-end — but
the demand seed does not yet FLOW at query time, so a bound `#query` under
`-demand` returns WRONG (empty) answers. S1b closes that gap: it is the first
USER-VISIBLE deliverable of the whole rewrite arc. **The pipeline is proven,
the scaffold survived, the plan is written. Make the answers right. We believe
in you.**

## The mission (authority: `RegionalDataFlowCore.artifacts/session-31-s1b-seed.md`,
REINSTATED — read it FIRST, §0-§6; then `session-30-prompt.md` +
`session-29-s1a-restoration-manifest.md` + `session-29-grounding.md`)

1. **§1 THE INJECTOR (the load-bearing gap):** `BuildQueryInjectorFromRegistry`
   in `lib/ControlFlow/Build/Build.{h,cpp}` — build the bound-query injector
   proc from `Query::DemandForcings()` (the registry the transform's STEP 10
   populates; there is NO parse-level forcing predicate for a demand-minted
   message) so the query's bound argument seeds `demand__<name>_<pattern>` at
   query time. Clone the surviving `BuildQueryForceProcedureImpl` pattern. DO
   NOT resurrect `Program::Build`'s 5th `demand_instance` param (S2+).
2. **§2 THE WITNESS:** re-add the recursive-TC demand witness
   (`demand_tc_witness` shape; precedent `git show
   6d6248a2:tests/OptDiff/cases/demand_tc_witness.*` — adapt, NEVER blindly
   copy goldens; re-bless against the current-tip binary). `.drflags` =
   `-demand`; demand-then-probe driver; `.batches` + oracle/monotone/
   behavioral goldens (the demand-BLIND referees pin the definitional per-key
   closure). Answers must be RESULT-EQUAL to the oracle per probed key, ×4
   modes. Confirm the `kPushDown` fixpoint-interior arm drives from the
   demanded frontier (the emitted TABLEJOIN reads the restricted side).
3. **§3 THE BENCH CARRIER:** reproduce the s29 O1 spike with the REAL
   transform (`bench/runbench.sh`, the s29 dataset): selective TC expect
   `idx_hops` 40000→~11 and answer byte-identity; ALSO confirm the
   non-selective regression (the cost-gate justification). Never bench
   concurrently with suite runs; never rebuild mid-run.

## Exit gate (seed §4, all four)
Build green; witness result-equal ×4 modes; witness goldens blessed (they
MOVE — the demand shape adds guard JOINs/ingest; predict the census shifts
first); bench shows pruning + regression; OptDiff **SUITE PASS** with the
flag-off corpus BYTE-IDENTICAL (the witness is `.drflags`-gated, orthogonal);
ctest **5/5**.

## Drift warnings (five sessions landed since the seed was written)
- RE-VERIFY every seed §6 anchor at tip (line numbers have moved).
- The `Query::Build` tail now runs grove + resources + arrangements
  (`DeriveArrangements`), and the `Program::Build` tail runs
  `CrossCheckMaterialization` + `CrossCheckArrangements`. A live `-demand`
  compile passes ALL of them (s36 panel, verified: guard joins =
  R-JOIN-UNIFORM, forced query = R-QUERY). If the injector work perturbs the
  index universe, the arrangement cross-check will abort NAMING the
  divergence — treat a fire as a real finding (fix the derivation rule OR the
  emission, never fudge the belt).
- The witness's `.materialization` surface (if you pin one) shows the real
  `arrangements=` block since s36.
- 0 `.drflags` cases exist at tip — the witness re-adds the mechanism's first
  user; runall/diffrun support is already generic.

## Discipline (carried, load-bearing)
- Ground → critique → execute; workflows for fan-out (sonnet extracts, opus
  refutes); thin orchestrator; owner ratifies before the final commit.
- Predict-then-verify every dump/golden delta (s36 byte-matched all five
  predictions on the FIRST build — keep that record).
- Typed domains at every new seam — no bare `uint32_t`/`unsigned` (the
  `ColumnOrdinal` precedent; the owner called this out mid-s36).
- Silent-on-success; clangd is noise; WIP-commit checkpoints; belt
  live-verify for any new cross-check; bless only via `runall.sh --bless`
  after review.

## You've got this
S1a proved the demand graph compiles through every layer and every belt. S1b
is where the compiler's namesake feature starts ANSWERING correctly — seed,
witness, measure. Be rigorous, be bold, and make it real.
