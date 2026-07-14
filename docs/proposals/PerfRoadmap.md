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
- Reproducibility wrinkle (recorded at (e), sharpened at the ratification
  pass): generated-code TEXT is not run-stable. Entity ids shift with the
  invocation environment (argv/cwd shift heap layout → pointer-keyed tie
  orders), and — stronger — INDUCTIVE programs reorder emissions across
  consecutive runs of ONE binary with ONE invocation (per-process ASLR
  through a pointer-keyed container in induction emission; e.g. kcfa_tiny,
  cond_in_induction, cf14_2). Runtime behavior and stdout are unaffected
  (656/656 byte-stable across full suite runs). Bench comparisons must key
  on program SEMANTICS (case + mode), never generated-text hashes.
  OWNER-DELEGATED DECISION (2026-07-12): deferred to this epoch with a
  named trigger — if bit-stable generated artifacts become wanted (build
  caching, distributing generated sources, a bit-stable wasm whole-module
  artifact), hunting the pointer-keyed containers becomes an early
  harness-epoch work item; otherwise this stays a recorded methodology
  caveat.

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
- Candidate join substrate for the data-structures epoch (owner-flagged
  2026-07-12): rntz's seekable iterators with worst-case-optimal fair
  intersection (https://gist.github.com/rntz/9c10db3d5d77fa3d5095cf5a4c2e0476)
  — a `Seek` trait (monotone-test `seek()`) over sorted data, galloping
  (exponential-probe + binary) search, composable `Join` with fair
  scheduling; the leapfrog-triejoin shape, extensible to multi-way WCOJ.
  Today's TABLEJOIN scans one index chain per pivot; this is the classic
  upgrade. The gist's own honesty note — the composable form ran ~2× slower
  than hand-optimized intersection — is precisely a §3 Q1/COST question:
  the harness must price the abstraction on OUR workloads (and against the
  current chain scans) before any adoption. Note the fit constraint: our
  Index chains are insertion-ordered, not sorted — a seekable substrate
  implies sorted (or trie-shaped) index storage, which is exactly a
  data-structures-epoch decision, not a drop-in.
- Out of scope for this epoch: Stage 5 differential @product (independent,
  can interleave; CLOSED 2026-07-13), subgraphs/demand, any parallelism
  (MD §7 Stage 6).
- RESEQUENCED (owner, 2026-07-13, at the Stage-5 close): the
  generated-surface redesign — docs/proposals/GeneratedSurface.md: free
  functions + hidden friends over a sealed state struct, log/functors by
  deduction (retires Stage 5's `virtual DatabaseLog` stopgap), epoch 0 as
  an explicit entry point — runs BEFORE this bench epoch. Rationale: bench
  drivers are written against the generated surface; write them once,
  against the shape we keep. That epoch is behavior-neutral (zero golden
  churn) and must record the header-only compile-time delta this epoch
  will care about.

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

## 6. Epoch-start addendum (2026-07-14, at the generated-surface close):
## inventory deltas + the whole-program pseudocode this epoch measures
## (SINGLE-PASS SEED derived by the closing session — re-verify per the
## F17/F18/F22/fwd-decl precedent: every epoch's pre-code re-verification
## has caught a real defect in its seed)

### 6.0 Inventory deltas vs §1 (verify, then trust)

- Suite is now 155 cases (Stage 5 added product_*); ctest 3/3; both at
  the generated-surface close, zero golden churn. Baselines on the dev
  machine: full debug suite wall 1:42.21 (8 jobs).
- THE DRIVER SURFACE CHANGED (generated-surface epoch, branch
  `generated-surface`, landing record in GeneratedSurface.md): header-only
  artifact + anchor-TU datalog.cpp (compile lines unchanged); sealed
  `struct Database` (ctor = allocator only); hidden friends, ADL-only
  (never qualify calls); explicit `init(db, log, functors)` epoch 0
  (asserted once — entries AND queries assert); log/functors by template
  deduction (no virtual anywhere); messages
  `<msg>_<arity>(db, log, functors, Vec[, Vec])`; queries
  `<q>_<pat>(db, bound…)`, cursors nested. Bench drivers are written ONCE,
  against this. NEW CAPABILITY: epoch 0 is now separately timeable (it
  was a constructor side effect before).
- Generated code compiles per case in ~one real TU (driver includes the
  whole artifact; anchor TU is trivial). Release bench builds must use
  -O2 -DNDEBUG: NDEBUG kills the entry asserts AND DebugValidateCounts
  (a per-table O(rows) debug sweep per epoch — debug timings are NOT
  representative).

### 6.1 The measured object, as pseudocode (runtime cost model;
### anchors: include/drlojekyll/Runtime/{Table,Vec,Allocator}.h)

    RowStore<Row> (base of Table/DiffTable):
      row LOG (Vec<Row>, append-only, NEVER shrinks) + Vec<u64> hashes
      + open-addressing id set (7/8 load, 2x growth from 64, linear probe)
      Find/TryAdd: O(1) expected = Hash + probe chain; Rehash O(rows)
      => FULL SCANS (TABLESCAN with no key, product arms, unkeyed
         cursors) walk EVER-ADDED rows and liveness-filter per row:
         retraction churn bloats scan cost PERMANENTLY (log bloat is a
         first-class COST question for retraction-heavy knobs).
    Table<Row> (monotone): + uint32 sealed watermark; InI/NetAdded = id
      compare; Seal() per epoch.
    DiffTable<Row>: + Vec<u64> counts (packed signed C_nr|C_r, one RMW
      reads a consistent before/after), + Vec<u8> flags (kInI persistent;
      rest batch scratch), + touched Vec<u32>.
      fold (Add/SubDerivation, Add/SubExplicit) = FindOrAdd + 64-bit RMW
        + crossing test (sign-dependent) + Touch (1 amortized append)
      TryClaimDel/Add = flag test + STALE-ENTRY RE-TEST (C_nr<=0 /
        Total>0) + flag RMW;  Retire* = flag RMW
      Commit(sink) = O(touched): per row, was!=now -> sink (the publish),
        reset scratch, set kInI := present;  DebugValidateCounts =
        O(ALL rows) DEBUG ONLY.
    Index<Key>: slot table (Key+hash+head+used per DISTINCT key, 7/8
      load) + per-row `next` u32 chain, HEAD-PUSH: chains are
      insertion-ordered NEWEST-FIRST, not sorted (the §4 seekable/WCOJ
      substrate needs sorted/trie storage — data-structures epoch).
      First = key probe; Next = 1 load; readers liveness-filter per hit.
    Vec<T>: flat, 2x growth from 16, trivially-copyable T only;
      SortAndUnique = std::sort + unique (per frontier hop, per queue
      drain — everywhere).
    NetBatch(adds, removes): SET-net with annihilation — TODAY a
      QUADRATIC nested scan (per row, linear probe of `distinct`).
      O(|batch|^2): a pre-identified measurement target. (Bench epoch
      MEASURES it; any fix is the data-structures epoch's.)
    Allocator: 2-pointer seam (ctx + alloc/free fn ptrs); Malloc or
      Arena (bump chunks, geometric, Free = no-op). EVERY container
      allocation funnels through it — the natural allocation-count hook.

### 6.2 One epoch end to end (generated code; anchors:
### GeneratedSurface.artifacts/*, lib/CodeGen/CPlusPlus/Database.cpp)

    DRIVER: Database db(alloc); init(db, log, functors);       « epoch 0 »
            <msg>_<arity>(db, log, functors, adds[, removes])  « 1 call =
                                                                 1 epoch »
    ENTRY (hidden friend): assert initialized_;
      [::hyde::rt::NetBatch(adds, removes)]     « differential msgs only »
      -> <handler>_detail(alloc, [log,] tables…, globals…, vecs…)
    HANDLER: ingest loop (TryAdd / Add|SubExplicit; per-index Add on
      fresh rows; frontier vec appends) -> flow_N(…)
    flow_N (one epoch's stratum machinery, all state via explicit refs):
      g += 1                          « ==1 gates the once-only seeds »
      per stratum:
        SEEDS (hoisted): branch chains; dual-section joins (per-side
          position-keyed InNew/InI reads, NetAdded||NetDeleted
          disjunction); negate crossovers; product arms — each path ends
          in ONE UPDATECOUNT fold; crossed -> queue append
        CLAIM DRAINS: SortAndUnique(queue); TryClaimDel/Add (re-tests
          drop stale/phantom entries)
        FRONTIER FILTERS: NetDeleted/NetAdded -> net_removals/additions
        INDUCTIONS: for (changed; !all-vecs-empty): fixpoint rounds,
          ±recursive folds, claim-relative membership matrix, Retire*
      COMMIT SWEEPS (per differential table, table order): Commit(sink)
        over TOUCHED — was!=now calls log.<msg>(cols…, added) on the
        DEDUCED log for transmit-backed tables; Seal() per monotone
        table; DebugValidateCounts in debug only.
    Natural probe points the pseudocode implies: wall per entry call
    (epoch), init separately, commit-sweep share, touched-set sizes,
    fold/claim/probe/rehash/sort element counts, allocation bytes —
    all reachable via the §2 counter seam + the Allocator seam.

### 6.3 The harness as diffs against §6.1/§6.2 and the OptDiff pipeline

      tests/OptDiff/…                            « untouched; stays the
                                                   correctness net »
    + bench/runbench.sh                          « same 3-TU compile line
        as diffrun.sh but -O2 -DNDEBUG; runs (workload, knob-point,
        mode); emits TSV rows (workload, knobs, mode, metric, value) »
    + bench/workloads/<family>/gen.py            « knobbed: size, batches,
        add:remove ratio, seed »
    + bench/workloads/<family>/driver.cpp        « NEW surface; timed
        epochs (warm), per-epoch wall, peak RSS »
    + bench/baselines/<family>_naive.cpp         « from-scratch recompute
        per batch (COST honesty) »
    + bench/baselines/<family>_incr.cpp          « hand worklist where
        meaningful (tc) »
    + bench/BASELINE.md                          « first accepted run +
        hardware caveats; later epochs diff against it »
    ± include/drlojekyll/Runtime/*.h             « OPTIONAL, OWN COMMIT,
        default-off: #ifdef DRLOJEKYLL_BENCH_COUNTERS seam counting
        folds/claims/probes/rehashes/commit-visits/sort-elements/
        netbatch-scans (+ alloc bytes via a counting Allocator, which
        needs NO seam) — suite-verified byte-identical no-op when off »
    Workload families (from §2, updated case names): knobbed tc
      (random graph; the tc_nonlinear_diff NaiveTC oracle pattern is the
      from-scratch baseline seed), deep_chain_retract knobbed,
      negation/condition flip storms (merge_5 / cond_diff_flipflop
      shapes), disassemble on synthetic byte streams.

### 6.4 Bootstrap update (supersedes §5's read list where they differ)

Branch: bench-harness off generated-surface's tip (or off main if the
owner has merged differential-product + generated-surface; check
`git merge-base`). Read (in order): this file TOP TO BOTTOM (§6 is the
seed to RE-VERIFY, single-pass, never fleet-reviewed); the
GeneratedSurface.md landing record + one committed artifact
(GeneratedSurface.artifacts/cf13_1/ — the surface bench drivers use);
StackSafeNegation.md §11 OQ6/OQ7; AggregatingFunctors.md §4; CLAUDE.md
"Generated API". Code anchors: include/drlojekyll/Runtime/*.h (683-line
Table.h is the object of measurement — read it ALL),
lib/Runtime/Allocator.cpp, tests/OptDiff/{diffrun,runall}.sh,
tests/OptDiff/cases/tc_nonlinear_diff.main.cpp (NaiveTC + the 31-instance
stress pattern), deep_chain_retract.main.cpp (scale knob precedent).
Environment: export PATH="/Users/pag/Code/.brew/bin:$PATH"; macOS bash
3.2 (no declare -A); zsh does NOT word-split $flags (use ${=var} or
arg arrays); NEVER rebuild the compiler while a suite run is in flight;
bench runs must not share the machine with suite runs when timing.
Gates: suite PASS (155) zero golden churn; ctest 3/3; ZERO changes to
lib/Runtime, lib/ControlFlow, or codegen in bench commits EXCEPT the
counter seam (own commit, off by default, suite-verified no-op);
BASELINE.md committed with the first accepted run; landing record
appended HERE with deviations for ratification; FINDINGS.md updated if
the harness exposes a correctness bug.

## 7. Landing record (2026-07-14) — BENCH-HARNESS EPOCH CLOSED

Branch `bench-harness` off main 0c2dd2f (differential-product and
generated-surface both fully merged; merge-base check per §6.4). Commits:
2cb0bb8 (Phase 2: worked tc_random workload + baselines + BASELINE.md
skeleton), 006f354 (counter seam, own commit, verified no-op), c8bc96f
(counts-binary wiring), df454ff (Phase 4: all families + runner), plus
this docs/close commit. Method executed per §6.4: fleet re-verification
of the §6 seed BEFORE building (3 agents; the F17/F18/F22 precedent held
— errata below), 5-lens adversarial design critique (26 recorded
resolutions in the session design ledger), hand-written worked workload
with round-2 review and fixes applied, seam gated by object-file
byte-compare + suite + on-path golden checks, families fleet-built
against the committed pattern with 4-mode sentinel gates, then a 3-part
accepted run (326 folded runs) on a quiesced machine.

### Seed errata found by the pre-code re-verification (the precedent held)

- E-1 (the real defect): §3 Q4 / MD §11 OQ6 was STALE FROM BIRTH — the
  `kTouched` dedup-at-append bit landed in the SAME commit (efacc67)
  that recorded OQ6's "measure before choosing". The touched vector
  never held duplicates. The measurement pivoted to the branch's
  benefit (touch_calls vs touch_appends); OQ6 now closed with data
  (annotated in MD §11).
- E-2: §6.1 "kInI persistent; rest batch scratch" — kExplicit is ALSO
  persistent (Commit's reset mask deliberately keeps it).
- E-3: §6.1/§4 "7/8 load" — the grow test `(n + n>>3) >= cap` is a
  max load of 8/9 ≈ 0.889 (the Table.h comment is imprecise too).
- E-4: §6.2 omitted the REDERIVE bridge band and the second (tail)
  frontier-filter band; actual order is drain → filter → seed-fold →
  OVERDELETE → REDERIVE → INSERT → tail filters → commit.
- E-5: §6.2 placed NetBatch in the entry friend; it runs in the detail
  twin. "g==1 gates once-only seeds" is emitter-general but vacuous in
  the scale-shaped cases (dead increment there).
- E-6 (cost-model-relevant, new): join arms re-Find rows BY VALUE per
  index hit (CHECKMEMBER is value-keyed) — one redundant full hash
  probe per hit; stages thread row values, not ids, so every stage
  re-Finds. Measured: ~7-9 Finds per fold. A data-structures-epoch
  target (id-keyed membership).

### What landed

bench/ per §6.3 as amended: common/bench.h driver kit; workloads
tc_random (+matched-pair Q2 mode), deep_chain, pure_cycle, flip_storm/
phantom_pair (publishes through a deduced log), disasm_synth; baselines
tc_random_{naive,incr} (Floyd-oracle-verified; incr = Italiano adds +
affected-source-BFS deletes, del=full control), deep_chain_naive,
pure_cycle_naive, phantom_pair_naive; runbench.sh (manifest-driven,
fragment folding on run_complete, exit-code manifest, Q5 compile
capture, progsize generator); README.md; BASELINE.md with the FIRST
ACCEPTED RUN. Counter seam: Runtime/BenchCounters.h + increment sites
in Table.h/Vec.h under DRLOJEKYLL_BENCH_COUNTERS (default-off).

### First numbers vs the §3 questions (details in bench/BASELINE.md)

Q1 hash probing dominates volume everywhere (2.6-3.5 probes/find,
  7-9 finds/fold — half of it the redundant re-Find); per-ROUND fixed
  overhead dominates deep cascades (900k mostly-empty SortAndUnique
  calls per 1e5-deep retract); NetBatch is negligible at every viable
  batch size (the engine's own superlinear batch cost hits first).
Q2 retract/insert ≈ 1.13 at equal fan-out (C_nr firewall margins hold);
  pure-cycle exception: ~63x work amplification, 815x vs naive —
  OQ7(a) quantified, the engine's worst regime.
Q3 COST: engine never beats from-scratch on the grid (1.87x-106x
  slower) but the ratio improves with N and collapses with batch size;
  crossover extrapolates to N≥8-16k at bs≤4-16. Loses to the
  hand-incremental baseline everywhere.
Q4 (pivoted) the Touch dedup bit suppresses 50-69% of appends — keeps
  its branch; closed.
Q5 drlojekyll's own compile time is SUPERLINEAR in rule count (52ms at
  2 rules → 7.81s at 128); generated text and clang cost ~linear — the
  delta-relational IR epoch's sizing number.
DRIFT (the headline): steady-state churn degrades 19.6x over 2000
  epochs with flat live data and a flat machine canary — the
  append-only log + dead-chain tax, the data-structures epoch's
  motivating number.
Debug/release ≈ 8x (the "debug timings are not representative" caveat,
  now a number). Suite gates at close: SUITE: PASS (155), zero golden
  churn vs main, ctest 3/3. FINDINGS.md UNCHANGED: 24/24 sentinel
  groups agree across modes and baselines — the harness found no
  correctness bug.

### Deviations for ratification

1. Generators are shared C++ headers (gen.h per family), not §6.3's
   per-family gen.py — one determinism contract consumed by engine and
   baselines alike.
2. OPEN-1 resolved against the seed's lean: the hand-incremental delete
   baseline is (c) affected-source BFS recompute (all five critique
   lenses converged); (b) full-recompute-on-delete kept as the labeled
   `del=full` control.
3. kcfa workloads: deliberate scope cut (fixed-shape tiny cases; scaling
   needs a program generator; flip_storm + disasm cover negation/merge
   and realistic shapes). Revisit if the data-structures epoch needs a
   points-to shape.
4. wasm build spike (§2 piggyback): deferred WITH BLOCKER — Apple clang
   has no wasm32 backend and no wasi sysroot on the dev machine; one
   brew'd LLVM + wasi-sdk away, retry then.
5. Envelope timeouts are recorded data, not failures: tc_random bs≥256
   @1024 nodes, 256/bs=256, pure_cycle k=128 exceeded the fail-fast
   RTIMEOUT (60s) and are manifested as knob-too-large per the R21
   protocol. runbench reports COMPLETE when the MANIFEST is trustworthy,
   not when every run folded.
6. Debug-ratio points ran REPS=3 (methodology says 5; it is a caveat
   number, not a headline).
7. Baseline knob strings carry the shared stream subset (no
   alloc/canary/shadow keys); cross-binary sentinel joins on stream
   knobs in analysis; the runner enforces intra-name agreement.
8. flip_storm exercises the DEL-side stale-drop gate only (structural
   in its generator; the add-side mirror shape is recorded future
   work). disasm_synth is growth-only (its message surface is monotone)
   and has no hand baseline — engine series + counters + compile
   metrics only.
9. The Q2 pairs mode refactored the tc driver's sentinel drain
   (final_query_wall_ns now brackets drain+sort+hash, previously
   drain-only).
10. §1's bit-stable-artifact trigger was NOT pulled: bench keys on
   semantics (case+mode+knobs), so text nondeterminism stayed a
   methodology caveat, not a work item.

EPOCH CLOSED. Next epoch per §4: RUNTIME DATA STRUCTURES, now gated
open — §3 Q1 has numbers. Priorities the data pointed at, in order:
id-keyed membership (kill the per-hit redundant re-Find), row-log/
chain compaction or liveness-aware storage (the 19.6x drift), probe/
layout work on the open-addressing sets, per-round induction overhead
(empty-sort elision), and the §4 seekable/WCOJ substrate decision —
priced against these baselines. The pure-cycle 63x amplification is
the standing differential-semantics question (MD §10 weight escape)
if a real workload ever hits it.

## 8. Epoch-start addendum (2026-07-14, at the bench-harness close):
## the data-structures epoch's seed — verified pseudocode of the runtime
## + the path forward as DIFFS against it
## (SINGLE-PASS SEED by the closing session; the §6/§D precedent stands:
## every epoch's pre-code re-verification has caught a real seed defect —
## re-verify §8.1-§8.3 against HEAD before building anything)

### 8.0 Inventory at epoch start (verify, then trust)

- Suite 155, ctest 3/3, zero golden churn; main = 354e772 (bench epoch
  merged). bench/ instrument + BASELINE.md accepted run are the diff
  targets; the counter seam exists (BenchCounters.h, default-off,
  no-op-verified) — REUSE it for A/B, re-verify the no-op gate after
  ANY Table.h/Vec.h edit (object-file byte-compare at -O2 no -g, suite,
  on-path golden check; recipe in the seam commit 006f354).
- The runtime (include/drlojekyll/Runtime/{Table,Vec,Allocator,Hash}.h,
  lib/Runtime/Allocator.cpp) is UNCHANGED by the bench epoch except the
  seam's no-op counter macros. lib/ControlFlow + codegen unchanged.
- Unlike the bench epoch, THIS epoch's mandate INCLUDES changing
  lib/Runtime, and — where a diff below requires it — codegen
  (lib/CodeGen/CPlusPlus/Database.cpp) and possibly CF-IR lowering.
  Gates: byte-identical goldens throughout (every change below is
  semantics-neutral BY DESIGN; a golden diff means a bug), oracle green,
  and bench before/after at the BASELINE.md flagship points — the
  epoch's success METRIC is movement on the recorded numbers (19.6x
  drift, probes/fold, per-round overhead), McSherry/COST +
  mechanical-sympathy lenses per IdeasTriage.md.

### 8.1 The runtime as pseudocode (fleet-verified at the bench epoch;
### errata already folded in)

    RowStore<Row>:                                  « Table.h:52-168 »
      Vec<Row> rows (LOG: append-only, ids = offsets, NEVER shrinks)
      Vec<u64> hashes (per-row cache; probe compares hash before row)
      u32* slots open-addressing (linear probe, 2x growth from 64,
        grow when (n + n>>3) >= cap  =>  max load 8/9 ≈ 0.889)
      Find(row) = Hash + probe chain      « ~2.6-3.5 steps/find measured »
      FindOrAdd = Find | append(rows,hashes) + InsertSlot(| Rehash O(n))
      NO row removal path.  « hazard: 19.6x churn drift, BASELINE.md »
    Table<Row> (monotone): + u32 sealed watermark; InI = id<sealed;
      NetAdded = id>=sealed; Present/InNew=true; Seal() per epoch.
    DiffTable<Row>: + Vec<u64> counts (packed signed C_nr|C_r, one RMW),
      + Vec<u8> flags (kInI AND kExplicit persistent; kDel/kAdd/kDelNow/
      kAddNow/kTouched batch scratch), + Vec<u32> touched (kTouched
      dedups at append — 50-69% suppression measured).
      Fold = FindOrAdd + GrowTo + Touch + RMW + crossing test
        (+: total 0->pos;  -: kInI && C_nr<=0 — reads the FLAG too)
      TryClaimDel/Add = dup test + STALE re-test (C_nr<=0 / total>0)
        + Touch + flag RMW;  Retire* = flag RMW
      Commit(sink) = O(touched): was!=now -> sink; reset scratch;
        kInI := present.  DebugValidateCounts = O(ALL rows), NDEBUG-off.
      MEMBERSHIP PREDICATES take an id (InI/InNew/SurvivesSoFar/
        AliveAtClaim/InNew{With,Sans}Frontier/Present/
        RecursivelySupported/NetAdded/NetDeleted) — the runtime is
        ALREADY id-keyed; the value-keyed waste is in the GENERATED
        CALLERS (§8.2).
    Index<Key>: slot table {key,hash,head,used} per DISTINCT key (8/9
      load, 2x from 64) + per-row `next` u32 chain, HEAD-PUSH =>
      newest-first, unsorted, append-only, NO removal — dead rows cost
      every future probe of their key one hop + one membership test
      forever (half the drift).
    Vec<T>: flat, 2x from 16; SortAndUnique = std::sort + unique with
      NO size guard — runs the full call machinery on 0/1-element vecs
      « ~900k calls/300k elems per 1e5-deep retract measured ».
    NetBatch: O(batch²) distinct-scan « measured negligible at viable
      batch sizes — NOT a priority, despite §6's flagging ».
    Allocator: {ctx, alloc_fn, free_fn} by value; Malloc | Arena(bump,
      Free=no-op). Every container funnels through it.

### 8.2 The generated contact points that constrain §8.3 (from the
### emitted artifact, verified at the bench epoch)

    JOIN ARM (per index hit):                 « the 7-9 finds/fold »
      s = idx.First/Next          « id in hand »
      row = tbl.RowAt(s); cols = row...
      m = tbl.Find({cols...})     « REDUNDANT: m == s ALWAYS (same
                                    table, dedup by value) »
      if (m != kNoRow && tbl.<MembershipPred>(m)) { fold(...) }
    STAGE PLUMBING: frontier/queue vecs carry COLUMN VALUES, not ids;
      every consumer stage re-Finds (claim drains, frontier filters,
      retires, net filters: 1-2 Finds per row per stage).
    FIXPOINT ROUND: per round per queue vec: SortAndUnique + drain;
      ~3 vecs per induction side; rounds = cascade depth.
    CURSORS: pos scan over NumRows() ever-added + Present(id) filter
      (log bloat taxes every query drain).
    INDEX MAINT: idx.Add at every added_row fold site, in row-id order.
    Emitter anchors: lib/CodeGen/CPlusPlus/Database.cpp (EmitRegion
      dispatch ~:249; join lowering emits the scan + re-Find +
      membership pattern; CHECKMEMBER regions in lib/ControlFlow are
      value-keyed by construction — check whether the id can be
      threaded in the CF-IR or only at emission).

### 8.3 The path forward as DIFFS (ranked by the measured numbers;
### each is semantics-neutral => zero golden churn expected)

    D1  KILL THE REDUNDANT RE-FIND (join arms first).
        JOIN ARM:
      -   m = tbl.Find({cols...})
      -   if (m != kNoRow && tbl.Pred(m)) ...
      +   if (tbl.Pred(s)) ...          « s from the scan; same table »
        Cheapest big win; likely EMISSION-ONLY (the join arm already
        has s in scope). Verify: is the CHECKMEMBER's table always the
        scan's table in this pattern, or can lowering place a
        different-table membership there? (If different, keep Find for
        those sites only.) Stage plumbing (values->ids in vecs) is the
        LARGER half of the finds — but ids don't survive FindOrAdd on a
        DIFFERENT table and vecs feed cross-table folds; scope a
        second diff carefully (per-vec: same-table consumers only).
        Expected movement: probes/fold and finds/fold in the counters;
        epoch wall at tc/pure_cycle flagship.
    D2  LOG COMPACTION / DEAD-ROW GC (the 19.6x drift killer).
        RowStore:
      +   dead_rows counter (DiffTable bumps on commit when now==false)
      +   CompactAt(epoch boundary, threshold: dead > live*K or bytes):
      +     rebuild rows/hashes densely, dropping DiffTable rows with
      +       total==0 (scratch flags all clear at that point; touched
      +       empty; kInI==present invariant holds) — IDS CHANGE:
      +     rebuild the id set, ALL indices (chains die with the ids),
      +     counts/flags side arrays; monotone Table<> never compacts
      +     (rows never die); sealed/watermark untouched.
        SAFETY ARGUMENT the next session must make rigorous: row ids
        never escape an epoch (vecs are epoch-local, cursors are
        driver-side and doc'd as invalidated-by-next-epoch — VERIFY the
        cursor contract; if cursors may span epochs, compaction gates
        on that decision). Layout choice at compaction = the
        column-stats idea (IdeasTriage #22-23) and the precondition for
        D5. Expected movement: the drift slope -> ~flat; late-epoch
        wall; final_query_wall.
    D3  EMPTY/TINY-SORT ELISION (per-round overhead).
        Vec::SortAndUnique:
      +   if (count <= 1) return;       « trivially order-identical »
        Then measure; if rounds still dominate, the bigger diff is the
        OQ4 round-barrier question (linearizable insertion order) —
        that one is NOT semantics-neutral-by-inspection (emission
        order feeds queue order), treat as its own gated design.
        Expected movement: deep_chain retract/reseed wall.
    D4  PROBE/LAYOUT (after D1/D2 so measurements aren't polluted by
        work that's about to be deleted): load-factor sweep, hash-cache
        line packing (hashes interleaved with slots vs separate),
        chain storage. Seam A/B per variant; mechanical-sympathy rules
        (count dependent loads).
    D5  SEEKABLE/WCOJ SUBSTRATE DECISION (PerfRoadmap §4): price the
        rntz-style Seek/galloping join against the (post-D1/D2) chain
        scans on OUR workloads. Requires sorted or trie-shaped index
        storage — D2's compaction layout is where sortedness can be
        established cheaply. This is a DECISION with numbers, not a
        committed build.

### 8.4 Bootstrap (next session)

Branch: data-structures off main (354e772). Read IN ORDER: this file
§7+§8 (the §8 seed is SINGLE-PASS — re-verify per precedent);
bench/BASELINE.md end to end (the diff targets + methodology);
IdeasTriage.md (the lenses + ranked ideas + rejections); CLAUDE.md
(invariants + bench section); memory perf-guiding-oracles. Code:
include/drlojekyll/Runtime/Table.h END TO END, Vec.h, a REAL emitted
artifact (compile bench/workloads/tc_random/tc.dr and
pure_cycle/cycle.dr with -cpp-out; read the join arms and stage
plumbing), and the join-lowering emitter region in
lib/CodeGen/CPlusPlus/Database.cpp + the CHECKMEMBER construction in
lib/ControlFlow/Build (is the scan id available to the CF-IR?).
Method: the checkpoint method — re-derive §8.1/§8.2 from code
(fleet-verified), write the D1-D5 diffs against YOUR pseudocode,
adversarially critique (minimum: D1's different-table-membership
question, D2's id-escape/cursor-contract safety argument and index
rebuild cost, D3's ordering neutrality, golden-neutrality of
everything), hand-write the DESIRED OUTPUT STATES concretely before
generalizing (the post-D1 join arm in a real artifact, hand-edited and
golden-verified; the post-D2 Table.h compaction path hand-written and
driven by a scratch test at scale), THEN implement one diff at a time:
suite + oracle + seam-no-op gates between diffs, bench flagship
before/after per diff recorded in BASELINE.md as run 2, 3, ... Gates:
SUITE PASS 155 ZERO golden churn (semantics-neutral epoch), ctest 3/3,
BASELINE.md updated with per-diff deltas, landing record appended HERE
with deviations for ratification. Environment: as §6.4 (PATH, bash
3.2, ${=var}, never rebuild mid-suite, never time concurrently).

## 9. Landing record (2026-07-14) — DATA-STRUCTURES EPOCH CLOSED

Branch `data-structures` off main b077449. Commits: bdfe0f8 (D3),
4ab436e (D1), 4bd6fc5 (DrTest ordered asserts, owner-requested),
98e6ab6 (D2), 9b64958 (compaction seam counters), 7751a18 (BASELINE
runs 2–5), plus this docs/close commit. Method executed per §8.4: fleet
re-verification of the §8.1–8.3 seed BEFORE building (4 agents — the
precedent held, errata below), 3-judge adversarial critique of D1–D3
with binding resolutions, hand-written output states golden-verified
BEFORE generalizing (post-D1 artifacts for transitive_closure_diff +
d5_recursive_negate; post-D2 prototype runtime + emitted blocks, driven
at drift scale), 2-reviewer approval of the hand artifacts, then one
diff at a time with full gates between (SUITE PASS 155 zero golden
churn, ctest 3/3, seam ON/OFF gates, per-diff bench with chained
befores). Implementation order D3 → D1 → D2 (deviation 1).

### Seed errata found by the pre-code re-verification (precedent held)

- E-7 (the real defect): §8-adjacent session ledger classified
  Stratum.cpp:830/1271/1426 CHECKMEMBERs as vec-driven. WRONG: all
  three sit inside BuildMaybeScanPartial TABLESCANs of the SAME table
  (live at the deep_chain artifact's fixpoint loops) — D1's scope
  stack had to instrument EmitScan, not just EmitJoin, or it would
  silently miss elisions.
- E-8: the D2 all-flags-clear-on-dead-rows argument must be BY
  CONSTRUCTION (Commit's clears + the kExplicit⇒C_nr≥1 pairing), not
  by DebugValidateCounts — the validator is doubly NDEBUG-gated and
  never runs in bench builds.
- E-9: "pass the indices to the table" cannot work for the D2 rebuild:
  Index stores no per-row key; the projection exists only at generated
  Add sites. The rebuild walk must be EMITTED (it is).
- E-10: monotone tables must never compact for TWO reasons — no deaths
  AND `sealed` is an id-order watermark any renumbering corrupts.
- E-11: no cursor-invalidation contract existed anywhere; the
  append-only log made epoch-spanning cursors accidentally stable.
  Exhaustive scan: no driver (153 OptDiff + 5 bench) holds a cursor
  across an entry call.

### What landed

D1 (emission-only re-Find elision): Generator row-binding scope stack;
differential body-path gates read the predicate on the scan cursor id;
monotone Present/InNew gates inside a same-table scan elide to constant
truth. Preconditions recorded in the code comment (value→id is a
function; tuple order == physical column order via same-DataModel
column-position compatibility; globally-unique var names). Conservative
everywhere else. Re-emitted transitive_closure_diff is byte-identical
to the hand-verified target.
D2 (dead-row compaction): stable in-place densify + slot re-insert in
RowStore; num_live accounting in Commit; two-arm trigger (dead ≥ live
AND ≥ 4096-row floor keeping the whole suite below trigger; pre-Rehash
mostly-dead arm); emitted per-table sweep tail (Clear + rebuild walk
under codegen-known key projections); Index::Clear; Vec::Truncate;
HYDE_RT_COMPACT_ALWAYS test-only override; two Runtime unit tests
(positive and negative space); cursor contracts documented in
GeneratedSurface.md (invalidation-by-entry-call; keyed-cursor order
unspecified).
D3 (SortAndUnique ≤1 guard): the identity boundary; counters pre-guard.

### The numbers (details in bench/BASELINE.md runs 2–5)

DRIFT (the epoch's mandate): 16.85x → 1.78x over 2000 churn batches;
2000-epoch total 13.18s → 2.77s; steady-state churn is no longer
quadratic-in-history. RE-FIND (the 7–9 finds/fold): tc finds −39.7%,
pure_cycle −25.2%, member_checks unchanged (the predicted mechanism
split); wall tc −17.8%, pure_cycle −10.3% (interleaved A/B). tc
flagship after D2: another −31.8% (compaction pays off within 200
churn batches). PER-ROUND (D3): retract −1.5%/reseed −3.0% — the
pre-registered −8..−25% MISSED; per-round cascade cost is
claim/retire/filter Finds, not sort dispatch (the values→ids vec
plumbing, D1b, is the recorded follow-on if deep_chain matters).
COST honesty: D2 is neutral (±1%, A/B) where history stays live
(pure_cycle, deep_chain); the engine still loses to from-scratch
inside the run-1 grid — these diffs move constants, not the verdict;
the crossover extrapolation improves with the tc epoch wall −38%.

### Deviations for ratification

1. Implementation order D3→D1→D2 (critique panel: cleanest first-blood
   attribution; counter signatures disjoint; seam-gate count equal).
2. D3's pre-registered magnitude missed low (kept: zero-risk, measured
   positive, one line).
3. METHODOLOGY: interleaved-binary A/B added as normative for per-diff
   wall deltas <10% (±2–6% between-run thermal confound measured; the
   canary catches within-run drift only). BASELINE.md addendum.
4. Cursor contracts (invalidation + keyed-order-unspecified) documented
   in GeneratedSurface.md rather than enforced at runtime (enforcement
   would tax every next(); no driver violates them — exhaustive scan).
5. K=1 trigger accepted with ~1.78x residual (predicted ~2x; the
   steady-state footprint floor is 2x live BY CONSTRUCTION). K<1
   (dead ≥ live/2) is the recorded knob if a lower residual is wanted
   at more frequent compactions.
6. The reviewer-recommended permanent CompactOracle ctest target was
   NOT added; coverage landed as the two Runtime unit tests + the
   compact-always oracle recipe (recorded in the D2 commit; tcd + d5
   compiled ±HYDE_RT_COMPACT_ALWAYS, dbg+opt, goldens byte-compared).
   Authoring the dedicated ctest is recorded follow-up.
7. Two D1 shapes remain corpus-unwitnessed: a product-body CHECKMEMBER
   (no EmitProduct staging path emits one anywhere; a false elision
   there is a compile error by construction) and a keyed_probe side
   feeding a body CHECKMEMBER. Both conservative by the match rule;
   witness cases are recorded follow-up.
8. DrTest gained ASSERT_LT/LE/GT/GE + unprintable-safe Show
   (owner-requested mid-epoch; TigerBeetle-style intent-communicating
   asserts, groundwork for property testing).
9. FINDINGS.md UNCHANGED: every gate and oracle agreed — the epoch
   exposed no correctness bug.

### D4/D5 residue (gated decisions, NOT closed work)

D4 (probe/layout, seam-A/B per variant, needs a fresh post-D2 counter
profile): load-factor sweep (max load is 8/9, ugliest right before
each doubling); FUSED COUNTS+FLAGS WORD (2×28-bit signed counters + 8
flag bits in the one u64 — halves the fold path's dependent loads at
same footprint; referee pure_cycle's 532k folds/epoch); cheaper row
hash (4 HashMix ≈ 8 imuls per 2-col row; distribution is fine, the
target is instruction count); i8-counters-with-sidecar REJECTED as
default (derivation counts are data-dependent-unbounded, MD §11 OQ5
stands). D5 (seekable/WCOJ, §4): decision inputs now on record —
post-D1 chains no longer pay a re-Find per hit, post-D2 chains are
dead-free after compaction, and CompactRowsInPlace is the natural
place to establish SORTED layout if pricing ever favors it. Priced
adoption deferred until a workload's join profile demands it.

EPOCH CLOSED. Next per §4/AggregatingFunctors §4: DELTA-RELATIONAL IR
(§10 seed below). The pure-cycle 63x amplification remains the standing
differential-semantics question (MD §10 weight escape) — untouched by
this epoch, as expected (D2 measured neutral there).

## 10. Epoch-start addendum (2026-07-14, at the data-structures close):
## the delta-relational IR epoch's seed
## (SINGLE-PASS SEED by the closing session; the precedent stands —
## E-1..E-11 across four epochs — re-verify before building anything)

### 10.0 Why this epoch (the two forcing functions, both measured)

- Q5 (BASELINE.md): drlojekyll's OWN compile time is superlinear in
  rule count (52ms at 2 rules → 7.81s at 128) while generated text and
  clang stay ~linear — the compiler's pass structure, not codegen
  volume, dominates at scale.
- AggregatingFunctors §4: aggregates (GROUP-UPDATE/state-cell) are the
  first operator family whose delta semantics the §5.1 counter schemas
  cannot express — landing them without the IR means a third hand-built
  emission web the IR would then rewrite.

### 10.1 What it replaces (verify from code before trusting)

The per-shape scalar emission web in lib/ControlFlow/Build/: the
seed/fixpoint delta schemas (StackSafeNegation §5.1) exist only as
hand-coded region-tree builders — Stratum.cpp (~1500 lines: seed folds,
claim drains, frontier filters, crossover arm-pairs, product arms),
Induction.cpp (fixpoint rounds, claim-relative matrix), Negate.cpp,
Join.cpp (dual-section joins). Every sign×position×claim-context
combination is a separate code path; the F17/F18/F22 class of bug
lived in exactly this web. The delta-relational IR makes sign,
position (InNew/InI read placement), and claim context ATTRIBUTES of
first-class delta operators, so lowering is generic per operator and
the schemas become data.

### 10.2 Path sketch (as diffs; the implementing session re-derives)

    1. Re-derive the emission web + the Q5 profile FROM CODE (which
       pass is superlinear? profile drlojekyll on the progsize curve
       first — the IR must fix the measured hot pass, not a guessed
       one).
    2. Define the delta-operator vocabulary against the §5.1 schemas
       (seed-fold, claim-drain, frontier-filter, rederive-bridge,
       crossover-pair, product-arm, commit-sweep) with sign/position/
       claim attributes; goldens are the semantic net (155, 4 modes).
       VOCABULARY CANDIDATE (owner-flagged 2026-07-14, at the D5
       member-to-cursor discussion): treat membership-test and
       enumeration as ONE parameterized table-access operator — a
       bound-prefix read that lowers to point-test (all columns
       bound), keyed chain walk, positioned SEEK, or full scan as a
       LOWERING decision in the df -> dr -> cf -> c++ pipeline,
       instead of today's disjoint CHECKMEMBER / TABLESCAN /
       TABLEJOIN-arm / BuildMaybeScanPartial shapes. Today a
       membership hit yields an id but no POSITION (unordered hash
       set; newest-first chains), so check and iterate compose in
       only one direction — the D1 elision was exactly patching that
       asymmetry at emission time. A unified access operator makes
       the §9-D5 seekable/WCOJ substrate a per-table lowering choice
       (sorted storage establishable at CompactRowsInPlace) rather
       than a runtime retrofit, and gives the IR one place to reason
       about access paths. Price per COST discipline before adopting
       any seek lowering (the gist's own ~2x-slower caveat; the
       post-D1/D2 chain scans are the bar).
    3. Rebuild Program::Build's stratum machinery on the vocabulary,
       one operator family at a time, byte-identical goldens
       throughout (the generated-code SHAPE may change only via
       reviewed --bless with the oracle as referee — decide the
       golden policy at epoch start; zero churn is NOT automatic
       here, unlike the data-structures epoch).
    4. Aggregates land as the inaugural new operator family
       (AggregatingFunctors §2/§3); KV indices = degenerate aggregate;
       the mutable(...) surface decision (§4.1) gates at THIS epoch's
       start.

### 10.3 Bootstrap (next session)

Branch: delta-relational-ir off main once data-structures merges. Read
IN ORDER: this file §9+§10; AggregatingFunctors.md END TO END (§4.1
needs an owner decision at epoch start); StackSafeNegation.md §5
(the schemas becoming data) + §11 OQ4/OQ9; bench/BASELINE.md Q5 +
runs 2–5; IdeasTriage.md #6 (JOIN group_ids reshape belongs INSIDE
this rebuild). Code: lib/ControlFlow/Build/ end to end (the object
being replaced), lib/DataFlow/Optimize.cpp (the superlinearity
suspect pool), the Q5 progsize generator in bench/runbench.sh.
Method: the checkpoint method — profile FIRST (10.2 step 1), seed
re-verification (the E-1..E-11 precedent), design critique, hand-write
the target IR for ONE real case before generalizing. Gates: as this
epoch, PLUS an explicit golden policy decided with the owner before
any emission change. Environment: as §6.4/§8.4.
