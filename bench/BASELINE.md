# Bench baseline ledger

Status: FIRST ACCEPTED RUN recorded 2026-07-14 (bench-harness epoch close).
Later epochs diff against the numbers below; the raw TSVs and analysis are
regenerable via runbench.sh with the runspecs recorded in the landing
record (PerfRoadmap.md). Methodology was fixed before the run and is
normative for future accepted runs.

## Hardware / environment (the accepted run)

- Machine: Apple Silicon macOS dev machine (Darwin 25.5.0, arm64),
  8 suite-jobs class; quiesced, caffeinate -i, no concurrent load.
- Toolchain: clang++ (Apple, -std=c++23), drlojekyll debug build at the
  epoch branch head; DR sha256 + clang version + loadavg pinned in each
  results.tsv '#' header (R22).
- Runs: timing grid (opt, REPS=5, RTIMEOUT=60 fail-fast), flagship
  (opt/nodf/nocf/none, REPS=5, +counts binaries), debug-ratio points
  (mode=debug, REPS=3). 326 folded runs; every non-folded run is
  manifested with its exit code (all were deliberate fail-fast
  timeouts at too-large knob-points, per the R21 protocol).
- Caveats: no frequency pinning / core affinity on macOS (canary epochs +
  IQR reporting control for it; canary was FLAT across the long runs);
  clock_overhead_ns ~14-29ns (optimistic lower bound — adjacent reads);
  netshadow under-estimates cold in-entry NetBatch (warm-cache bias);
  alloc_* counts database allocations only (marshalling + NetBatch
  scratch excluded by design).

## Methodology (fixed; unchanged from the pre-run skeleton)

- Bench builds: `-std=c++23 -O2 -DNDEBUG`, the diffrun.sh 3-TU line.
  NDEBUG removes entry asserts and DebugValidateCounts (O(all-rows) per
  table per epoch). WALL and COUNTS come from separate binaries (seam
  off/on); never multiply them; the wall<->counts bridge is unmeasured.
- Timed region per epoch = exactly the message entry call. Sentinels,
  shadows, canaries, query drains sit outside the brackets.
- R=5 process reps, medians with warmup (first 3 churn epochs) dropped;
  full raw series kept. RSS comparable under alloc=malloc only.
- Sentinels: final_*_count/final_*_hash cross-checked across all modes
  and baselines at shared stream knobs. THE ACCEPTED RUN: 24 sentinel
  groups, ALL singletons — no cross-mode or engine-vs-baseline
  disagreement anywhere; the harness found NO correctness bug (no
  FINDINGS entry from this epoch).
- COST is a crossover curve; scalar ratios are captions.

## The §3 answers (first numbers, opt mode, medians)

### Q1: where does batch time go?

Operation-volume breakdown per epoch (counts binaries, flagship points):

| family (point)            | finds | probe_steps | folds | member | idx_hops | sort calls/elems | netbatch cmp |
|---------------------------|-------|-------------|-------|--------|----------|------------------|--------------|
| tc_random 1024/16/rr50    | 9.8k  | 34.3k       | 1.1k  | 5.7k   | 3.8k     | 58 / 1.4k        | 120          |
| pure_cycle 8x32 remove    | 2.02M | 3.03M       | 532k  | 925k   | 508k     | 57 / 405k        | 28           |
| deep_chain 1e5 retract    | 1.45M | 3.79M       | 300k  | 450k   | 100k     | 900k / 300k      | ~0           |
| phantom_pair 256/32       | 7.9k  | 28.9k       | 2.9k  | 76.9k  | 19.5k    | 15 / 4.6k        | 2.0k         |
| disasm_synth 2000-blk     | 2.2k  | 15.8k       | 0.5k  | 0.5k   | ~0       | 37 / 1.3k        | 0            |

Findings: (1) HASH PROBING is the universal #1 — 2.6-3.5 probe steps per
find and ~7-9 Finds per fold in join workloads (the value-keyed re-Find
pattern: chains scan by id, membership re-Finds by value; every pipeline
stage re-Finds rows whose ids the previous stage held). (2) In deep
cascades the PER-ROUND fixed overhead is first-class: deep_chain runs
~100k fixpoint rounds per retract and makes ~900k SortAndUnique CALLS
carrying only 300k total elements — mostly-empty sorts at ~0.3
elements/call. (3) Negation/context-heavy shapes (phantom_pair) are
dominated by member_checks (77k vs 2.9k folds). (4) NetBatch is
NEGLIGIBLE at every viable batch size (shadow: 84ns at bs=4 to 13.7us at
bs=256; the quadratic knee sits beyond the engine's own epoch-cost
envelope — the engine's superlinear batch-size growth hits first, see
Q3). Data-structures epoch priorities, in order: id-keyed membership
(kill the redundant re-Find), probe-chain/layout work, per-round
overhead in inductions, chain storage.

### Q2: retraction vs insertion at equal fan-out (OQ7 margins)

Matched pairs (R15; 1024 nodes, ef=80, 64 pairs, median fan-out
|delta tc| = 4.5): engine add 3.65us vs del 4.10us — RETRACT/INSERT
~ 1.13 at equal fan-out. The C_nr firewall keeps retraction nearly at
insertion cost on mixed-support graphs; the overdeletion design's
margins hold at this shape. (Hand-incremental: 834ns/833ns ~ 1.00;
naive: 24.5us both.) THE EXCEPTION is pure-cycle support: an 8-ring x32
remove batch overdeletes the whole ring cone — 266k folds against a net
change of ~4.2k rows (~63x work amplification), 48.6ms/epoch, 815x the
naive recompute (59.6us) — OQ7(a) quantified: pure cycles are the
engine's worst regime by three orders of magnitude. deep_chain (acyclic
cascade, depth 1e5): retract 26.8ms vs reseed 22.3ms (ratio 1.20),
constant-stack verified at -O2 in all 4 modes.

### Q3: COST crossover

Engine vs from-scratch (naive, per-batch wall ratio; steady-state churn
rr=50, ef=80, uniform):

| nodes \ bs | 4     | 16    | 64   | 256          |
|------------|-------|-------|------|--------------|
| 256        |       | 35x   |      | TIMEOUT      |
| 1024       | 1.87x | 10.9x | 75x  | TIMEOUT      |
| 4096       |       | 2.97x |      | 106x         |

(ratios > 1 = engine SLOWER; TIMEOUT = engine exceeded 60s for 100
batches, recorded fail-fast.) The crossover vs from-scratch is real and
sits just beyond the grid: the ratio improves with graph size (naive
recompute grows with N) and collapses with batch size (engine epoch cost
grows superlinearly in bs — its envelope tops out below bs=256 at 1024
nodes, BEFORE NetBatch matters). Extrapolation puts engine-beats-naive
at roughly N>=8-16k with bs<=4-16; the from-scratch baseline's own
bitset-closure representation also stops scaling there, which is itself
the honest COST statement: at every point where the competent
single-threaded program can hold the closure, it beats the engine.
Against the hand-incremental baseline (Italiano bitset) the engine loses
everywhere measured (23x at 1024/16 to ~400x+). Amortized-total COST
(init + seed + churn) does not change the verdict at any grid point.
Powerlaw vs uniform at 1024/16: engine 271us vs 314us (slightly FASTER
on powerlaw at equal edge count), naive 32us — ratio 8.4x.

### Q4 (pivoted): the Touch dedup benefit

OQ6's dedup bit shipped in efacc67 (pre-epoch; the roadmap text was
stale). Measured benefit: touch_calls / touch_appends = 2.5x (tc
flagship 2797/1105), 3.2x (pure_cycle 810k/257k), 2.0x (deep_chain,
phantom, disasm) — the branch suppresses 50-69% of touched-vector
appends. Commit sweeps visit exactly touched (commit_visits ==
touch_appends everywhere), publishing 90-100% of visits on transmit
tables. Verdict: the dedup bit earns its branch; OQ6 closed with data.

### Q5: compile time / code size vs program size

progsize chained-rule curve (opt):

| rules | dr_wall  | header    | cxx_wall (anchor) |
|-------|----------|-----------|-------------------|
| 2     | 52ms     | 21KB/611L | 323ms             |
| 8     | 59ms     | 64KB/1.8kL| 349ms             |
| 32    | 195ms    | 242KB/6.5kL| 511ms            |
| 128   | 7.81s    | 963KB/25kL| 1.20s             |

dr_wall is SUPERLINEAR (40x for the last 4x of rules) — the compiler's
own passes, not codegen volume, dominate at scale; this is the
delta-relational IR epoch's Q5 number. Generated text and cxx cost grow
~linearly. Mode deltas (tc flagship): opt header 21.9KB vs none 26.9KB
(-19%), binary 96KB vs 114KB (-16%); dataflow-opt buys ~19% runtime on
deep_chain (nodf 33.2ms vs opt 26.8ms retract), controlflow-opt ~0% —
compiler opts are NOT where the engine's constant factors live (opt vs
none epoch wall differs <10% on most workloads).

### Drift (hazards 2+3 priced: the log-bloat tax)

tc_random 1024/16/rr50, 2000 steady-state churn batches (live edge count
~constant): per-epoch wall degrades from ~301us (early) to ~5.89ms
(late) — a 19.6x SLOWDOWN with NO growth in live data, while the
interleaved machine canary stayed flat (7.04ms -> 6.90ms). This is the
append-only row log + dead index-chain + counters/flags side-array tax:
ever-added rows are scanned, probed past, and chain-hopped forever.
Single most important number for the data-structures epoch: steady-state
churn is quadratic-in-history today.

Publish cost (flip_storm, 256/m=32): ~345 adds + ~397 dels published per
epoch through the deduced log at ~409us/epoch total; publication-stream
sentinel equals query sentinel in every mode. OQ7(b) machinery observed
live: stale-drop gates fire on 94/100 mixed batches (med 6/epoch,
del-side; the add-side mirror shape is future work).

### Debug/release ratio (R25 caveat number)

deep_chain depth=1e4: debug retract 17.4ms vs release 2.2ms — 7.9x.
tc flagship: debug ~8.3x. Debug timings are not representative; the
number to remember is roughly ONE ORDER OF MAGNITUDE.

### Other run-scoped observations

- init_wall (epoch 0): 2.5-9us across families (trivial-init programs;
  first-touch dominated as caveated) — not a cost center today.
- peak RSS at deep_chain 1e5: 31MB opt (alloc_bytes 401MB cumulative,
  free_bytes 380MB — geometric growth churn); nodf/none allocate 542MB
  cumulative (+35%) for the same job: dataflow opt also saves state.
- Engine epoch-cost envelope (recorded fail-fast): tc_random tops out
  below bs=256 (1024 nodes) / bs=256 works only at 4096 nodes (106x
  slower); pure_cycle k=128 exceeds 60s/40 batches. Dense graphs
  (ef>=120 at any size) explode via the nonlinear-TC witness table
  (materializes tc JOIN tc triples — cubic-ish in SCC size).
