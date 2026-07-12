# Perf roadmap: bench harness → data structures → delta-relational IR (seed ledger)

Status: SEED LEDGER, recorded 2026-07-12 at Stage-3 close of the
derivation-counters migration (StackSafeNegation). Nothing here is
implemented. This file seeds the NEXT epoch per the sequencing recorded in
AggregatingFunctors.md §4: Stage-3 close → **bench harness (COST
instrument)** → runtime data structures → delta-relational IR → aggregating
functors + KV indices → subgraphs. Like the checkpoint ledgers, everything
below is a seed hypothesis: the implementing session must re-derive the
inventory, critique the design, and check it against the code before
building anything.

## 1. What exists at epoch start (verify, then trust)

- Correctness net: tests/OptDiff — 151 cases × 4 optimization modes, every
  mode byte-compared against one golden; oracle + monotone-projection
  variants per .batches sidecar; ctest 3/3 (MiniDisassembler, PointsTo,
  Runtime). SUITE: PASS at Stage-3 close. Perf work must keep this net
  green untouched — goldens change only via new-case authoring.
- NO performance measurement exists anywhere: no timing, no allocation
  counters, no benchmark harness, no committed baselines.
- The objects of later optimization (lib/Runtime + include/drlojekyll/
  Runtime): Table (append-only row log + open-addressing hash set),
  DiffTable (packed signed counter pair + flags byte + touched vector),
  Index (per-row next-chain, no per-key allocation), Vec (flat arrays,
  sort-unique discipline), the Arena/Malloc allocators. All deliberately
  simple; none ever profiled.
- Scale-shaped cases that can become workloads: deep_chain_retract (10^5
  chain, the constant-stack gate), transitive-closure family (nonlinear,
  mixed batches, 30-seed randomized stress driver pattern in
  tc_nonlinear_diff), kcfa_tiny/kcfa_tiny_merged, disassemble (the largest
  realistic program). tests/OptDiff drivers print correctness output, not
  timings — workloads need scale KNOBS (graph size, batch count, retraction
  ratio) the current fixtures lack.
- Known overhead classes designed-in and unmeasured: overdeletion churn in
  pure cycles (MD §11 OQ7), phantom-pair churn on mixed batches, per-RMW
  crossing checks, touched-vector growth (OQ6 dedup question), commit-sweep
  scans, sort-unique passes on every frontier hop.
- Reproducibility wrinkle (recorded at (e)): compiler ENTITY IDS are
  invocation-environment-sensitive (argv/cwd shift heap layout → pointer-
  keyed tie orders), so generated-code TEXT is not run-stable, though
  runtime behavior and stdout are (656/656 byte-stable across fresh suite
  runs). Bench comparisons must key on program SEMANTICS (case + mode), not
  generated-text hashes.

## 2. The bench harness (this epoch's deliverable)

Owner's framing: a COST-style instrument (McSherry's "Scalability! But at
what COST?") — measure the generated databases against straightforward
single-threaded baselines so the counter machinery's overhead is priced
honestly BEFORE the data-structure and IR epochs spend anything on speed.

Seed design (critique before building):

- Harness shape: a bench/ directory sibling to tests/OptDiff, NOT a suite
  variant — perf runs must never gate correctness CI. Reuse the OptDiff
  compile pipeline (compile_datalog pattern or diffrun.sh's manual compile)
  with -O2/release generated code; the debug suite stays the correctness
  net. One driver per workload, a knobbed generator per workload family.
- Measurements per (workload, knob-point, mode): wall-clock per batch
  (ingest→commit epochs, warm), peak RSS, rows/counters touched (the
  runtime can expose cheap counters under a bench-only #define — never in
  golden-compared builds), allocation counts via the Allocator seam (it
  already funnels every container).
- Baselines per workload, per COST: (a) from-scratch recomputation per
  batch (the oracle's NaiveTC pattern, compiled C++, no incrementality);
  (b) where meaningful, a hand-written incremental baseline (e.g. tc with
  a worklist). The interesting ratio is generated-incremental vs both.
- Workload axes: chain depth (deep_chain_retract knobbed), random-graph tc
  (nodes/edges/batch-size/add:remove ratio, seeded), negation flip storms
  (merge_5/negate shapes scaled), condition flip-flop, disassemble on
  synthetic byte streams. Retraction-heavy points are the whole reason the
  counter design exists — they get first-class knobs.
- Output: one flat TSV/CSV per run (workload, knobs, mode, metric, value) +
  a committed BASELINE.md summarizing the first accepted run on the dev
  machine, so later epochs diff against recorded numbers with recorded
  hardware caveats. No CI gating on absolute numbers.
- Cheap early piggyback (from AggregatingFunctors §4): once the harness
  exists, the wasm whole-module build spike (clang --target=wasm32-wasi
  LTO of one case, byte-compared output, timed) becomes a harness workload
  rather than its own scaffolding.

## 3. What the harness must answer (the questions that gate the next epochs)

1. Where does batch time go: counter RMWs, hash probes, index chains,
   frontier sort-uniques, commit sweep, or allocation? (→ prioritizes the
   data-structures epoch.)
2. What does a retraction cost relative to an insertion at equal fan-out,
   and how does the C_nr firewall's pruning show up at scale? (→ validates
   or indicts the overdeletion design margins, OQ7.)
3. COST ratio: at what scale (if any) does the incremental database beat
   from-scratch recomputation per batch? (→ the honesty number.)
4. Touched-vector duplication rate on hot rows (→ decides OQ6's dedup-bit
   question with data instead of taste.)
5. Compile-time and generated-code-size growth vs program size (→ sizes the
   delta-relational IR epoch's win beyond maintainability).

## 4. Sequencing and scope fences

- Bench harness lands ADDITIVELY: zero changes to lib/Runtime,
  lib/ControlFlow, or codegen in the same commits, except the bench-only
  counter seam if adopted (own commit, off in normal builds, suite-verified
  no-op).
- The data-structures epoch (open addressing tuning, index layouts, arena
  strategies) starts only after §3 Q1 has numbers; the delta-relational IR
  epoch follows per AggregatingFunctors.md §4 (aggregates are its first
  operator family — do not start aggregates from here).
- Out of scope for this epoch: Stage 5 differential @product (independent,
  can interleave), subgraphs/demand, any parallelism (MD §7 Stage 6).

## 5. Session bootstrap (fresh-session checklist)

- Read: this file top-to-bottom; AggregatingFunctors.md §4 (sequencing) and
  the wasm-direction bullet; StackSafeNegation.md §11 OQ6/OQ7 (the two
  open questions the harness must answer with data); the
  checkpoint-e-notes landing record (Stage-3 close state, the id-
  nondeterminism observation).
- Method (the checkpoint method): before building, inventory §1 against
  the tree at HEAD; write the harness design as a concrete file layout +
  one worked workload (knobbed tc) end to end; adversarially critique the
  measurement plan (what does the Allocator seam miss? do bench-only
  runtime counters risk drifting the golden-compared build? is the
  from-scratch baseline honest or a strawman?); only then implement.
- Environment: export PATH="/Users/pag/Code/.brew/bin:$PATH"; suite gates
  as in CLAUDE.md (SUITE: PASS required, byte-identical goldens — perf work
  changes no goldens, ever).
