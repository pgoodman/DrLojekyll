# Bench baseline ledger

Status: SKELETON (bench-harness epoch, 2026-07-14). The first ACCEPTED run
fills the tables below; until then only the pilot-scoping observations at
the bottom are data. Methodology is fixed here BEFORE the accepted run so
the numbers can't quietly renegotiate their own rules.

## Hardware / environment (fill at the accepted run)

- Machine: (Apple Silicon model, core count, RAM)
- OS: (sw_vers), clang: (clang++ --version), commit: (git rev), DR sha256:
- Caveats: no frequency pinning or core affinity on macOS; P/E-core
  migration and thermal throttle are controlled for by canary epochs +
  inter-rep IQR reporting, not eliminated. Never timed concurrently with
  suite runs or other load (loadavg recorded per run).

## Methodology (fixed; design record in the PerfRoadmap landing record)

- Bench builds: `-std=c++23 -O2 -DNDEBUG`, the diffrun.sh 3-TU line.
  NDEBUG removes entry asserts and `DebugValidateCounts` (a per-table
  O(all-rows) debug sweep per epoch); debug timings are NOT representative.
  One flagship point per family is also built `-g` debug (mode=debug) so
  the debug/release ratio is a recorded caveat number.
- WALL and COUNTS are separate narratives from separate binaries: the
  timing binary (seam OFF) produces wall/RSS/alloc rows; the counts binary
  (`-DDRLOJEKYLL_BENCH_COUNTERS`) produces operation counters and its wall
  is discarded. Counts are never multiplied by wall; the bridge between
  them is explicitly unmeasured this epoch.
- Timed region per epoch = exactly the message entry call. Input-vector
  construction, the NetBatch shadow, canaries, query drains, and sentinels
  sit outside the brackets; sentinels run with the clock stopped.
- NetBatch wall attribution = driver-side shadow (same
  `hyde::rt::NetBatch`, same data, timed separately each epoch). The
  engine's in-entry NetBatch cost at a point is (to first order) the
  shadow number; `netbatch_compares` from the counts binary corroborates.
- R = 5 process-level repetitions; medians reported with IQR, raw series
  kept. Warmup = first 3 churn epochs, recorded but dropped from
  aggregates. Clock overhead recorded per run
  (`clock_overhead_ns`); drift slopes are reported only where the
  per-point signal exceeds 20x that floor.
- RSS is comparable only under `alloc=malloc` (counting wrapper). Arena
  rows are labeled cumulative reservation INCLUDING the grown-Vec
  leak-by-design — never read as live bytes. `alloc_bytes - free_bytes`
  is the live-requested companion.
- Recorded estimator caveats (round-2 review): `clock_overhead_ns` is an
  optimistically pipelined lower bound (adjacent clock reads), so the 20x
  trust margin is kept generous; `netshadow_wall_ns` runs second on
  cache-warm data and systematically UNDER-estimates the engine's cold
  in-entry NetBatch cost (counts-binary `netbatch_compares` corroborates
  trends); `alloc_*` rows count DATABASE allocations only — message
  marshalling Vecs and the engine's NetBatch scratch (allocated from the
  moved-in input vec's allocator) are deliberately uncounted and the
  attribution choice is fixed here.
- Every binary at a knob-point emits `final_tc_count` / `final_tc_hash`
  (canonical FNV over ascending pairs); the runner cross-checks engine
  modes AND baselines. A mismatch is a FINDINGS-class event, recorded as
  SENTINEL_FAIL, never silently dropped. Runs without a `run_complete`
  marker row are discarded as partial.
- COST (Q3) is a crossover CURVE over (batch size, density), reported
  both per-batch and amortized-total (init + seed + churn), and normalized
  per changed tuple (|Δtc| from the baseline series, joined on epoch).
  Scalar ratios are captions, not answers.

## The §3 questions (fill at the accepted run)

### Q1: where does batch time go?
(counts-binary breakdown at flagship points; netbatch share via shadow)

### Q2: retraction vs insertion at equal fan-out
(matched-pair protocol; the RR knob is a DENSITY-trajectory knob, not a
retraction-cost knob — see pilot note below)

### Q3: COST crossover curve
(engine vs tc_random_naive vs tc_random_incr; the pilot says the
crossover, if it exists, lives at large-N sparse tiny-batch)

### Q4 (pivoted): Touch dedup benefit
(OQ6's dedup bit shipped in efacc67; measured here as touch_calls vs
touch_appends + touched/NumRows commit share)

### Q5: compile time / code size vs program size
(per-mode dr_wall, cxx_wall, header bytes/lines, binary bytes + the
progsize scaled-rule-count curve)

## Pilot-scoping observations (2026-07-14, pre-seam, single rep — NOT the
## accepted run; recorded because they fix the flagship envelope)

- Engine viable envelope is NARROW on tc_random: at 256-1024 nodes,
  ef>=120 (mean degree >=1.2 at seed, densifying under churn) or bs=128
  at ef=120 TIMES OUT (>120s for 50 batches) while both baselines finish
  in microseconds-milliseconds. The nonlinear tc(F,X),tc(X,T) witness
  table (table_8 triples) is the dominant state: dense SCCs make it
  ~cubic in nodes. Flagship points must sit SPARSE (ef<=80-100) with
  small batches; density is the wall, not node count.
- Steady-state grid (ef=80..120, bs=16..128, rr=50, batches=50, opt):
  engine NEVER beat from-scratch: 11x-856x slower at viable points
  (e.g. 1024/80/16: engine 313us vs naive 29us vs incr 14us median).
  The honest COST statement so far: at small scale the counter machinery
  loses to recompute by 1-3 orders of magnitude; the crossover hunt
  belongs at large-N sparse where naive recompute cost grows while
  engine delta work doesn't.
- The RR knob is a density-TRAJECTORY knob: rr=0 grows the graph +bs
  edges/batch (engine median 22.6ms and p90 5.6s at 1024/80/16 by
  densification), rr=50 is steady state (308us), rr=70 shrinks (85us).
  Q2 conclusions must come from the matched-pair protocol, never from RR.
- NetBatch shadow at bs<=128 is 250ns-4.5us — negligible at pilot batch
  sizes; the quadratic knee lives at much larger bs (sweep in the
  accepted run).
- Suite-case semantics reuse works: engine (opt), naive, and incr agree
  on final_tc_count and final_tc_hash at every completed pilot point
  (and an early apparent mismatch was a harness knob-parsing bug, caught
  by the sentinel and fixed: malformed knob values now abort).
