# Calibration Slice: the identity-join `idx_adds` law, reproducible

> Status: landed this session (2026-07-31, branch `keyed-instances`, tip after
> `61b17a15`). This is the MEASUREMENT half of the monotone non-recursive
> identity-join vertical slice. It closes cost-model-findings.md #7 (untracked
> driver) and #8 (mislabelled mono artifact), and supplies the checked negative
> witnesses cost-model-findings.md #5 asked for. It does NOT build the static
> predictor; see `costmodel-contract-decision.md` for why that half is gated.

## What this is, precisely

A **runtime measurement runner** (the third product in the Product Distinction),
not a static estimator and not a trace interpreter. It compiles the mono demand
witness two ways, runs the *same* concrete trace against both, and compares the
`idx_adds` counter delta over the probe epoch. It reports observed
`gBenchCounters`; it never predicts them from a plan.

- Driver: `../calib/calib_driver.cpp` — parametric (`N F K DUP [REPEAT]`),
  snapshots `hyde::rt::gBenchCounters` around a K-probe demand epoch only
  (ingest excluded), asserts each drained answer is exactly `neighborhood(s)`
  (HP-5), prints every counter's delta as `COUNTER <field> <delta>`.
- Runner: `../calib/runcalib.sh` — regenerates the recognizer-ON codegen
  (default `-demand`) and recognizer-OFF codegen (`-opt-disable=df.ident_join`),
  compiles the driver against each `-O2 -DNDEBUG -DDRLOJEKYLL_BENCH_COUNTERS`
  (a COUNTS binary, never a timed one), runs a scenario sweep, and checks the
  law by exact integer equality.
- Reproduce from a clean tree: `cmake --build --preset debug` then
  `docs/proposals/CostModel.artifacts/calib/runcalib.sh`. Saved run:
  `../calib/last-run.txt`.

## The law, corrected and checked

The old shorthand was `Δidx_adds = F·K` (measured-calibration-1.md). Measured
across the sweep, the exact runtime law is

```text
idx_adds(OFF) − idx_adds(ON)  ==  F · min(N, K)     over the probe epoch
```

`min(N, K)`, not `K`: a demanded key with no out-edges (`s ≥ N`) stands up an
empty instance and materialises nothing. Raw `F·K` MISPREDICTS the empty-tail
case (would say 1024, runtime does 256); the corrected law is checked there as a
refutation, not hand-waved.

The recognizer-OFF build re-materialises the demanded F neighbours into the
redundant projection-guard's intermediate table once per populated probe; the ON
build drops that guard (`join.7`), saving exactly F index inserts per populated
demanded key. This reproduces measured-calibration-1.md's headline row exactly
(`idx_adds` ON=1024 / OFF=2048 at N=256,F=4,K=256).

## Cases (all green; `CALIB: PASS`)

| Case | Purpose | Result |
|---|---|---|
| positive/base `256 4 256` | reproduce the measured table | Δ=1024=F·min(N,K) |
| fanout F∈{1,2,8,16} | scaling law, not one point | Δ=F·K each |
| held-out K=100 | K not used to derive the rule | Δ=400 |
| held-out N=1000, N=10000 | N-independence (demand prunes) | Δ=1024 both |
| dup ×4, ×16 | distinct-row semantics (#5 witness) | Δ unchanged (DUP-independent) |
| empty tail `64 4 256` | refute raw F·K | Δ=256=F·min(N,K), not 1024 |
| repeat ×4 | demand idempotence | Δ unchanged (not ×REPEAT) |

The duplicate-row and empty-tail cases are the load-bearing negatives: they prove
`idx_adds` counts distinct STORED rows over POPULATED probes, not ingest events
and not raw probe count. This is exactly the false-in-general mapping
cost-model-findings.md #5 flagged.

## Structural on/off delta, all four optimization modes (Work Order 1.5)

| mode | df opt | recognizer | df joins | kEagerJoin / kJoinEmit |
|---|---|---|---|---|
| opt  | on  | fires   | 1 | 2 / 1 |
| nocf | on  | fires   | 1 | 2 / 1 |
| nodf | off | skipped | 2 | 4 / 2 |
| none | off | skipped | 2 | 4 / 2 |

`-disable-controlflow-opt` correctly leaves the dataflow recognizer untouched
(opt ≡ nocf for the `.df`/`.rel`); `-disable-dataflow-opt` gates `df.ident_join`
off (nodf ≡ none). This is the "mode-split" the CLAUDE.md notes.

## What this slice deliberately does NOT claim

- It is not a validated static prediction. F and K are the *trace's own inputs*;
  the runner checks that the runtime obeys `F·min(N,K)`, which is a
  runtime-behaviour law confirmed by measurement, not a plan-derived estimate.
- Index multiplicity (Stage-3 case 5) is NOT exercised: the mono witness has one
  relevant index, so `idx_adds` multiplicity is 1. A multi-index witness is a
  separate case and is listed as unfinished in `costmodel-contract-decision.md`.
- The "unsupported-counter, no implicit zero" assertion (Stage-3 case 8) belongs
  to the *prediction report type*, which this measurement-only slice does not
  build. It is deferred with the predictor.
