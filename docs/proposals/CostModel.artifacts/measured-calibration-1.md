# CALIBRATION 1 — the recognizer's cost, MEASURED (not symbolic)

> Tip: keyed-instances, post-recognizer. Answers the owner's challenge (2026-07-31):
> "how are you actually evaluating these? or is it just pure make-believe magic
> document?" — by MEASURING actual runtime operation counts via the bench-counter
> seam (`include/drlojekyll/Runtime/BenchCounters.h`). This is the cost model's
> FIRST predicted-vs-measured calibration point and the template for the rest
> ([[predict-then-verify-ir]] applied to cost). Grounds the numeric/simulation
> reshape of CostModel.md.

## SETUP

`demand_neighborhood_mono_witness` (`neighborhood(bound Start, free Node) :
edge(Start, Node)`), compiled under `-demand` TWO ways:
- **OFF**: `-opt-disable=df.ident_join` — the redundant projection guard join.7 present.
- **ON**: default — the recognizer drops join.7 (`kEagerJoin` 4→2, `kJoinEmit` 2→1).

Both compiled `-O2 -DNDEBUG -DDRLOJEKYLL_BENCH_COUNTERS`; a driver
(`scratchpad/benchmeasure/measure.cpp`) loads N source nodes × F=4 out-edges, then
snapshots `gBenchCounters` around K=256 distinct demand probes.

## MEASURED (per 256 probes)

| counter            | OFF (join.7 in) | ON (dropped) | Δ      |
|--------------------|-----------------|--------------|--------|
| `finds`            | 2816            | 1536         | −45%   |
| `probe_steps`      | 17114           | 9077         | −47%   |
| `idx_adds`         | 2048            | 1024         | −50%   |
| `idx_hops`         | 3072            | 2048         | −33%   |
| `idx_first`        | 768             | 512          | −33%   |

## TWO MEASURED LAWS (the cost model must reproduce these)

1. **`idx_adds` drops by EXACTLY F per probe** (2048→1024 = 256·F, F=4). The
   redundant join.7 re-materialized the demanded F neighbors into an extra
   intermediate table once per probe; dropping it saves exactly F index inserts
   per demanded key. A clean, falsifiable predicted-vs-measured law:
   `ΔidxAdds = F·K`. This IS the cost model — a prediction reality confirms to the
   operation.
2. **N-independence**: N=1000 and N=10000 give IDENTICAL probe-loop counts. The
   demand query cost is O(K·F), NOT O(N) — demand pruning is real and measured,
   not asserted. (A scenario family that samples N ∈ {1k,10k,100k} witnesses this
   flatness directly — [[cost-scenario-family]].)

## CONSEQUENCE FOR THE COST MODEL (the reshape)

The cost model is NOT a symbolic document — it is a NUMERIC SIMULATOR whose
predictions are VALIDATED against `gBenchCounters`:
- A scenario supplies concrete input data.
- The tool PREDICTS operation counts (finds/idx_adds/idx_hops/…) by simulating
  over the real `.rel` ops.
- The generated code RUNS with counters on the same data → MEASURED counts.
- predicted ≈ measured → calibrated. predicted ≠ measured → the model is WRONG,
  loudly, and gets fixed.
The symbolic `N·M` algebra (and its polynomial-normalizer risk) is dropped; the
asymptotic SHAPE is recovered by sampling the scenario family, not by carrying
symbols. This calibration (ΔidxAdds = F·K) is goldenable as the first
predicted-vs-measured pin.
