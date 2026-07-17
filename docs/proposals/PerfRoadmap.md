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

### 10.4 The whole pipeline as pseudocode (SINGLE-PASS SEED, recorded at
### the data-structures close from a structural read + this epoch's
### verified runtime/emitter/artifact pseudocode — re-verify per the
### E-1..E-11 precedent before building anything)

    PIPELINE (bin/drlojekyll/Main.cpp):
      parse (lib/Lex, lib/Parse) -> ParsedModule
      Query::Build(module, log, optimize)          « lib/DataFlow »
        BuildClause per clause -> SELECT/TUPLE/CMP/MAP/JOIN/MERGE/
          NEGATE/INSERT views; zero-arity predicates desugar to unit
          relations (1 bool col); Stratify rejects unstratified
          negation in all modes
        if optimize: QueryImpl::Optimize = Simplify -> per-node
          Canonicalize FIXPOINT -> CSE (group_ids guard) -> dead-flow
          elimination        « Q5 superlinearity suspect pool #1 »
        -> Query: dataflow DAG, every inter-view dependency a column
           edge
      Program::Build(query, log, first_id, optimize)
                                       « lib/ControlFlow/Build »
        pre-pass diagnostics: AGG / KVINDEX / impure MAP / on-cycle
          differential @product (ViewSelfReachable fence)
        BuildDataModel + FillDataModel: views -> DataModels
          (equivalence classes) -> one TABLE per model (monotone |
          differential by CanProduceDeletions reachability); indices
          from join/negate key columns
        BuildEntryProcedure + BuildIOProcedure per message (ingest
          loops: explicit folds, index Adds, frontier appends) ->
          the single flow proc
        BuildStratumPhases (Stratum.cpp:1456, ~2330 lines — THE
          EMISSION WEB §10.1 names; two halves worth separating):
          DISCOVERY half (becomes DR-IR *construction*):
            ComputeRecursiveSCCs -> RuleClass (kRecursive iff fold
              target and deriving view share an SCC)
            DiscoverBranches: branch CHAINS from every frontier
              source (differential table = both signs; induction-
              owned = adds only; monotone boundary = adds only)
              through TUPLE/CMP/MAP paths to fold targets
            JoinEmission per join view reached by a chain: ONE delta
              pivot vec shared by all feeding chains and signs; dual
              sections; suppressed when ALL sides share the join's
              own SCC (no lower atom -> no seed; round-0 carried by
              the claim-round fire)
            negation crossovers: exactly ONE arm-pair per non-@never
              negate (F19 invariant); @never gates on Present
            product arms: one signed frontier arm per side x sign
              (acyclic differential @product)
            a per-stratum scheduling FIXPOINT orders: seed folds
              (hoisted, both signs) -> claim drains -> OVERDELETE ->
              REDERIVE bridge -> INSERT -> tail frontier filters ->
              branch-chain walks -> commit sweeps
                                 « Q5 suspect pool #2 »
          EMISSION half (becomes a GENERIC DR->CF lowering):
            EmitSeedLoop / EmitCrossover / EmitClaimDrain /
            EmitRetireFrontier / EmitRederive / EmitFrontierFilter /
            EmitJoinFire (claim-relative predicate matrix keyed on
            STATIC join position) / EmitProductArms /
            EmitChainStep+EmitHeadFold+EmitSectionWalk — every
            sign x position x claim-context combination is a
            hand-coded region-tree builder; ALL VALUE-KEYED by
            construction (the CF-IR has no row-id or position
            vocabulary — the D1 elision recovers ids at C++ emission
            time, and the §10.2 unified access operator is where
            ids/positions could become first-class instead)
        Induction.cpp: fixpoint-round loops (queue vecs,
          SortAndUnique + drain per round, retire bands)
        ProgramImpl::Optimize x2 (around ExtractPrimaryProcedure):
          region flattening, no-op removal, procedure dedup
                                 « Q5 suspect pool #3 »
        -> Program: region tree (SERIES/PARALLEL/INDUCTION/LET/
           TUPLECMP/UPDATECOUNT/CHECKMEMBER/COMMITSWEEP/CLAIM/
           RETIRE/VECTOR*/TABLEJOIN/TABLESCAN/TABLEPRODUCT/...)
      C++ codegen (lib/CodeGen/CPlusPlus/Database.cpp):
        row/key structs -> sealed Database + hidden friends ->
        EmitRegion dispatch (D1 row-binding scope stack lives here)
        -> commit-sweep tail (D2 compaction + emitted reindex)
      runs against include/drlojekyll/Runtime (§8.1 pseudocode as
        amended by this epoch: compaction, num_live, Index::Clear)

    Q5 caveat for the profiler: dr_wall superlinearity (52ms@2 ->
    7.81s@128 rules) has NOT been attributed to a pass — the three
    suspect pools above are hypotheses, not findings. R0 measures
    before anything is designed.

### 10.5 The path as diffs against §10.4 (re-rank after R0's profile)

    R0  PROFILE FIRST. Run the progsize curve (runbench's generator)
        under a profiler; attribute the superlinearity to a pass.
        NO IR design commits before R0 lands — the IR must fix the
        measured hot pass, and its shape (e.g. whether DR-IR
        construction replaces the scheduling fixpoint or wraps it)
        depends on which pool is hot.
    R1  DR-IR VOCABULARY (df -> dr -> cf -> c++). First-class delta
        operators carrying sign / position / claim-context as
        ATTRIBUTES: table-access (the §10.2 unified bound-prefix
        operator: point-test | chain walk | seek | scan as lowering
        choices), seed-fold, claim-drain, rederive-bridge,
        frontier-filter, crossover-pair, product-arm, fixpoint-round,
        commit-sweep. The §5.1 schemas become DATA consumed by one
        generic lowering, not code shapes. BuildStratumPhases'
        DISCOVERY half becomes DR-IR construction.
    R2  GENERIC LOWERING dr -> cf, one operator family at a time,
        goldens as the semantic net. GOLDEN POLICY IS AN EPOCH-START
        OWNER DECISION: emission shape may change; changes go through
        reviewed --bless with the derivation-counter oracle as
        referee — zero churn is NOT the default here, unlike the
        data-structures epoch.
    R3  AGGREGATES as the inaugural new operator family
        (AggregatingFunctors §2/§3 GROUP-UPDATE/state-cell); KV index
        = degenerate aggregate; the mutable(...) surface decision
        (§4.1, recommendation (a) recorded) gates at epoch start.
    R4  (gated, optional) JOIN group_ids reshape for native self-joins
        (IdeasTriage #6, the measured cubic tc⋈tc witness blowup) —
        ONLY inside this rebuild, with an invariant-preservation
        argument for the CSE guard.

### 10.6 Bootstrap amendment (supersedes §10.3 where they differ)

As §10.3, plus: §10.4/§10.5 are THE seed to re-verify (single-pass,
never fleet-reviewed — the E-1..E-11 precedent says the verification
WILL find a defect; budget for it). The two §10.4 halves of
Stratum.cpp must be re-derived from code before R1's vocabulary is
fixed. R0's profile is the epoch's first artifact and gates all
design. Owner decisions to collect at epoch start: mutable(...)
syntax (§4.1), the R2 golden policy, and whether R4 is in scope.

## 11. Landing record (2026-07-16) — DELTA-RELATIONAL-IR EPOCH CLOSED

Branch `delta-relational-ir` off main 9e1a092. The epoch's working
ledger is docs/proposals/DeltaRelationalIR.md (§1-§12: R0 profile, the
verified pipeline pseudocode, the diff plan, every per-diff record, the
big review, and the §12 stage records); the hand-written and spec
artifacts are DeltaRelationalIR.artifacts/. Commits, grouped by R-stage:

- Ledger/seed: 30567273 (§1 R0 profile + fleet-verified pseudocode +
  seed errata E-12..E-17), 862b57cc (E-12 stale-enum fix), e8fbb113
  (§5 the path as diffs, RQ5a/RQ5b first, R1-R4 re-ranked), dedd9b30
  (§6 vocabulary v2 + 4-judge critique + B-1/B-2/B-3), e0a17062 (§7
  owner decisions), 3108a046 + b68acfdf (v2/v3 binding spec + judge),
  efbe0a28 (§B amendments B-7..B-13), 07679c6f (B-14 restatement),
  0b3ec874 (DR-IR v3 statecell extension for R3).
- RQ5a/RQ5b (the measured Q5 fix): 5e393944 (RQ5a — delete the dead
  taint passes), cd1d0349 (RQ5b — CSE color-refinement bucketing),
  e89541b6 (BASELINE runs 6-7).
- R1 (construct-alongside, golden-frozen): bdcea2ff (R1a inventory
  layer), 8adc85ff (R1b DRVec materialization + reconverge_1),
  7c43edac (fixpoint_stress_1 witness), 5d959121 (R1c op families +
  per-arm effects + Σ-terms), 0d8d00ef (R1d checked linearization +
  region shells), 124b499d (permcheck.py, the R2 bless referee),
  3108a046 (§10 vocabulary-v3 record).
- R2 (generic lowering, one family at a time, delete-with-cutover):
  a83ce1cc (family #1 acyclic band), 6c4ca4fb (family #2 SCC/recursive
  band), c794761a (family #3 commit sweeps + THE OLD-DISCOVERY
  DELETION).
- Big review + fixes: 7dacc097 (§11 big-review record), ecfe6746
  (review fixes F2/F3/F6/F7), e3559592 (V-PRED-XCHECK), 8fec79d7 (§8
  review-fixes record).
- R3 (aggregates + KV indices, the inaugural new operator family):
  8c1d169c (§7.1 algebra-pragma decision), 50f936bb (R3c-i @-algebra
  pragma surface), 504c9e4a (R3a StateCellStore runtime), 10599717
  (R3b AGG/KVINDEX stratification), be13cc9c + 012df83a + d07d89e5
  (§12 as-landed model + critique + C-1 occupancy amendment), 3749a16a
  (R3 stage A compiler-side cutover), 8dec9cbf (R3 stage B emission
  layer behind the fence), d6e31b79 (R3 stage C — THE FLIP),
  plus this docs/close commit.

Method executed (the checkpoint method, per §10.6): R0 profile FIRST —
NO IR design committed before it landed (§10.5's gate honored); fleet
re-verification of the §10.4/§10.5 seed BEFORE building (4 opus agents;
the E-1..E-11 precedent held again — errata E-12..E-17 below); v2/v3
vocabulary through THREE judge rounds (the 4-judge design critique with
B-1/B-2/B-3, the independent Fable optimizability review with F-1..F-8,
and the vocabulary-v3 dependence-completeness judge with §A amendments)
PLUS the owner-requested big review (§11 of the ledger) PLUS the §12
as-landed modelling round (pseudocode + diff plan + critique) — the
pre-code re-verification precedent held at EVERY review (each found a
real defect or oversell before code: RQ5b merge-set drift, the eager
INGEST_FOLD non-seam, the F1 decorative-payload finding, the C-1
birth/death occupancy CRITICAL). R1 was construct-alongside with a
byte-identical-golden hard gate; R2 cut over one family at a time,
deleting the replaced Emit* in the same diff, with the derivation-
counter oracle + permcheck.py as bless referees; R3 landed in three
staged commits (A compiler-side, B emission behind a fence, C the
flip) with the fence keeping the suite green until the whole path was
proven end to end.

### Seed errata found by the pre-code re-verification (precedent held)

- E-12 (the real defect): Program.h:621's enum comment said
  kNetAdded = `kAdd && !kDel`; the runtime is `kAdd && !kDel && !kInI`
  (Table.h:429-433 — the `!kInI` conjunct is load-bearing against a
  same-batch −e1/+e2 double count). An R1 vocabulary derived from the
  header comment would encode the wrong frontier predicate. Fixed the
  comment; R1 derives predicate semantics from Table.h only.
- E-13: the negate SEED read is CONTEXT-keyed for the whole chain
  (Stratum.cpp:519-521: in_fixpoint ? kInNew : kInI), never the §5.1
  per-position split — exactly the F18 shape. R1's NEGATE_GATE keys on
  context, not position.
- E-14: the membership-predicate vocabulary is TEN, not §5.1's eight —
  kNetDeleted and kNetAdded are first-class frontier-filter predicates.
- E-15: §10.4's dataflow order was wrong — Stratify runs LAST in
  Query::Build (after Optimize), Simplify is a separate unconditional
  pre-pass, and Optimize is CSE-first/interleaved, not a linear
  4-stage pipeline.
- E-16: BuildStratumPhases is NESTED inside BuildEntryProcedure
  (Procedure.cpp:769), ~872 lines of a 2330-line file, ONE function
  body with shared mutable state {branches, joins, crossovers,
  products, drain_stratum, recursive_sccs, table_delta_vecs} — the R1
  cut had to externalize exactly that state as the DR-IR object.
- E-17: DiscoverBranches is sign-agnostic (sign chosen at emission);
  the seed conflated the scheduling fixpoint (integer lift only) with
  emitted order, and missed that SCC-table frontier filters are
  deferred into the add-loop output for correctness.

### What landed

THE DR-IR IS NOW THE SOLE AUTHORITY for the stratum machinery.
lib/ControlFlow/Build/DR.{h,cpp} (the v3 object model: typed DRVec
values with def/use edges, per-arm effect sets, access-plan spines,
the ten membership predicates, derived strata + a checked
linearization, always-on graph validators) is constructed alongside
the build and, after the R2 cutovers, drives emission via
LowerDRFlow / LowerDRRounds / LowerCommitSweeps / LowerGroupUpdate
(Stratum.cpp). What died in R2: the hand-coded scheduling fixpoint,
DiscoverBranches (the §1.4 un-memoized path-DFS hazard),
CollectSectionTargets, TableOwnerStratum, the Crossover/Product
Emission structs, SeedDRStrata, the entire #ifndef NDEBUG readiness-
assert block, and the hand-coded commit-sweep loop — DELETIONS totalling
−1070/+590 in lib/ControlFlow/Build/ across the three families (family
#3 alone: −974/+415, BuildStratumPhases shrank ~530→~160 body lines).
STRATA became DERIVED (DeriveDRStrata, B-13 retired); every cutover was
raw-byte-identical on its anchors (B-14 exceeded — bijection = identity).

RQ5a/RQ5b (the measured Q5 fix, dataflow-side per R0): RQ5a removed the
dead taint passes end to end (Taint.cpp, the two Build.cpp calls, the
QueryColumnImpl/QueryImpl members, the four public accessors, the
write-only STRONG Use edges — net −310); RQ5b replaced CSE's shallow
HashInit-bucket + pairwise-Equals with color-refinement bucketing
(round-0 color = HashInit + Select/Insert identity, position-salted
refinement to partition stability), keeping the B-2 deletion-flag
discipline and the group_ids guard inside Equals.

R3 (aggregates + KV indices, the inaugural NEW operator family): AGG
and KVINDEX (the degenerate aggregate) lower end to end as ONE
GROUP_UPDATE op per view (BuildGroupUpdateOps), standing per-group
state in a StateCellStore (Runtime/StateCell.h — own dense-group-id
space, two-word sealed/working cell + C-1 occupancy). Band (a) folds
the summarized-input net frontiers into the cell; band (b)
emit_touched applies the occupancy-generalized ONE-NET-PAIR guard into
the agg table's counters + del/add queues, riding the acyclic
claim/frontier/commit tail. The C-5 ADL ABI: reduction bodies are
DRIVER-SUPPLIED FREE FUNCTIONS forward-declared in the header and
defined out-of-line — `<f>_identity()/<f>_combine(w,v)/<f>_uncombine
(w,v)` for `@invertible`, `<f>_reduce(values, counts, n)` for
`@recompute`. Suite 155→164 across the epoch (157 at R1b with
fixpoint_stress_1; 159 at R3b with agg_in_scc_1/kv_in_scc_1; 162 at
R3c-i with the algebra pragma cases; 164 at R3c/stage C with
average_weight + pairwise_average_weight new and aggregate_1 flipped
from diagnostic to a 4-mode golden). permcheck.py (tests/OptDiff)
mechanizes the §7.1 permutation-only bless referee (never exercised —
every cutover was byte-identical, nothing to bless). Witness cases
authored along the way: reconverge_1 (reconvergent table-less
plumbing, the DiscoverBranches-memoization guard), fixpoint_stress_1
(§9-risk-#2: same-round double-claim + REDERIVE partial-restore +
phantom pairs + add-side stale drops), and the R3 corpus
(average_weight / pairwise_average_weight / aggregate_1 with drivers +
.batches + oracle/monotone goldens; algebra_invertible_1 additionally
witnesses the @invertible surface).

### THE NUMBERS (details in the ledger §8 + bench/BASELINE.md runs 6-7)

Q5 (the epoch's flagship metric, @128 rules): release opt
1077→149.4ms (−86.1%) (the measured RQ5b landing was 146.5ms — the 32→128
tail went from ~n^3 to ~LINEAR, 3.5x time per 4x rules); debug opt
7833→925.1 (−88.2%) (RQ5b landing 970ms). RQ5a alone: nodf/none −88..95%
(the dead-taint explosion, R0's §1.2 finding); RQ5b: opt −86..88% at
128 (the CSE hot pass, R0's §1.2 finding — ~72-80% of opt samples).
R1/R2 held Q5 flat (CF build is ~3% of compile — R0 demoted "the IR
fixes Q5" to "the IR must not regress Q5"); R3 stage C measured
median dr_wall +0.3% (<10%) with a byte-identical header for non-agg
programs.

Runtime neutrality (the golden-master invariant): generated code was
BYTE-IDENTICAL through all of R2 — neutrality by construction, proven
by cpp-out byte-compare of the flagship workloads (tc_random,
pure_cycle, deep_chain, flip_storm) against the frozen pre-R2 compiler
(the interleaved A/B was vacuous, recorded as the perf-neutrality
witness in lieu of a tautological timed run). R3 touches only
aggregate programs — the non-agg generated header stays byte-identical
at stage C (the +0.3% above is the compile-time dr_wall A/B, not a
runtime move).

### Deviations for ratification (SEED TO NEXT EPOCH)

Collected from the ledger:

1. B-2 (RQ5b merge-set gate) AMENDED and RATIFIED: the original
   "identical merge set" is unmeasurable across a bucketing change
   (bucket granularity shifts the fixpoint trajectory — 30/156 corpus
   programs differ in merge-EVENT counts, each Equals-verified at
   application); the operational gate is identical final per-kind view
   counts on the full corpus + suite byte-identical + oracle green
   (0/156 final view-count diffs measured). The deep hash KEEPS
   can_receive/produce_deletions (matches today's merge set byte-for-
   byte) + the no-unit-with-non-unit bucketing assert.
2. B-14 (family-#1 identity gate) RESTATED and RATIFIED: raw IR-text
   byte identity is unachievable from a rewritten builder without
   sequence replay, so the gate is tree-shape-modulo-id-bijection +
   stdout; in fact EVERY family landed raw-byte-identical (bijection =
   identity), exceeding the restated gate.
3. R0 RE-RANKING ratified: the Q5 fix is dataflow-side (RQ5a/RQ5b),
   not the emission web; RQ5a is FULL taint removal (passes + members
   + accessors), RQ5b is a deep hash keeping deletion flags — both
   land before R1. The IR's carry is optimizability + the F17/F18/F22
   bug-class + the unified access operator, not Q5.
4. MERGE-TRAJECTORY AMENDMENT (the F-3 tripwire): mechanized old-vs-new
   bucketing A/B over all 156 programs before commit; the amendment
   above (view counts, not event counts) is the ratified form.
5. R2 FENCE STAGING vs the one-atomic-unit critique: F-2 adopted
   ACYCLIC-FAMILIES-FIRST (families #1-#3), fixpoint families are the
   round-shell lowering inside them; R3c-ii+R3d were merged into ONE
   suite-gated unit (the big-review F9 disposition — F14 retirement
   flips aggregate_1/kvindex_1-4 to compiling before R3d's goldens
   exist), and R3 itself landed as A/B/C with a fence rather than one
   atomic diff (stage A compiler-side FENCED, stage B emission behind
   the fence, stage C the flip). Disposition: staging kept the suite
   green at every commit; the "one atomic unit" concern was met at the
   SUITE level, not the commit level.
6. STATE_SEAL riding COMMITSWEEP: the aggregate table's Seal() +
   DebugValidate() are tagged onto its COMMITSWEEP (seal_statecell_id)
   and emitted at commit-sweep exit rather than as a standalone region
   — placement UNCHANGED from the monotone Seal band; ratified as a
   lowering default.
7. §C-5 ABI DECISION TRAIL: the critique found §4's compiler-known
   reduction semantics FABRICATED; the real dependency is the never-
   written functor driver ABI (an owner decision). Resolved (C-5) to
   ADL-free driver-supplied free functions over driver-owned state
   (the DatabaseLog declared-in-header/defined-by-driver idiom, no ADL
   needed on builtin summary types); verified compilable+executable
   against a hand-written average_weight driver. MAP-functor migration
   onto this surface is a recorded follow-on (its own epoch).
8. R4 (group_ids reshape) and FAMILY #4 (eager INGEST_FOLD
   externalization) NOT in scope this epoch — R4 stays gated (re-seeded
   to §12), and kIngestFold stays a reserved DROpKind (the eager web,
   ExtendEagerProcedure/build_explicit_loop, is the last hand-coded
   emission surface; it has no externalized discovery seam to mirror
   faithfully — an R1e/R2-entry-family seam).
9. FINDINGS.md UNCHANGED: no correctness bug escaped to a golden. The
   three in-stage catches (R1c use-after-move in FillSeedFoldArm, R2#1
   VectorFor non-memoization, R2#2 kFixpointFire scc_group filter) were
   all pre-commit and anchor-caught; the big review found no live
   miscompile. Rationale: FINDINGS.md records golden-master-discovered
   bugs in shipped compiler behavior, not caught-before-commit
   construction bugs (recorded in the ledger instead).

### Epoch-close residue (verbatim from the §12 stage-C record; out of
### R3 scope, seed to §12)

- StateCell dead-group compaction (§1.1, D5-style, own moved() callback);
- shared-input drain FUSION (§2.2 G-6, effect-graph CSE);
- algebra class (II) mergeable-sketch lowering (§1.3, ships (I)+(III)
  only);
- sorted-multiset MIN/MAX (§6 b', the seekable-iterators residue);
- KVINDEX dataflow-node deletion (§3, a later upstream simplification);
- aggregates over MONOTONE inputs (§C-4 enrollment path exists but the
  corpus is all differential-input — untested);
- configuration-column aggregates (clean feature-gap reject);
- MAP-functor migration onto the C-5 free-function surface (§C-5
  recorded follow-on, its own epoch);
- the recorded V-PRED-XCHECK residuals (thread the per-arm gate node
  into EmitChainStep; EmitFrontierFilter un-cross-checked) carried
  forward.

EPOCH CLOSED. Next epoch per §12 (owner picks the theme). The DR-IR is
the sole authority for the stratum machinery; the one remaining
hand-coded emission surface is the eager INGEST web (kIngestFold
reserved), and the F17/F18 bug-class kill is REAL through the walked
lowering paths + V-PRED-XCHECK but not yet complete (the per-arm gate
threading and EmitFrontierFilter cross-check are the recorded
residuals).

## 12. Epoch-start addendum (2026-07-16, at the delta-relational-IR
## close): the NEXT EPOCH SEED
## (SINGLE-PASS SEED by the closing session — to-be-re-verified per the
## house precedent: E-1..E-17 across five epochs, EVERY pre-code
## re-verification has found a real seed defect; budget for it)

### 12.0 Why an epoch here (candidate themes; owner decides)

OWNER DECISION (2026-07-16, at close): theme (a) — the ADL/functor-
surface epoch. Theme (b) subgraphs re-seeds after it.

The DR-IR now owns the stratum machinery and has ONE new operator
family (aggregates) proving the rewrite substrate works. Two coherent
next themes, plus the residue as early work either way:

(a) THE ADL / FUNCTOR-SURFACE EPOCH — finish what R3's C-5 ABI opened.
    Three deferred web items, all recorded in the §11 residue:
    - MAP-functor migration onto the C-5 free-function surface: today
      MAP functors are DatabaseFunctors members; the aggregate
      reductions are C-5 free functions. Unify the two surfaces (the
      §C-5 recorded follow-on) so functor bodies have one calling
      convention. This is behavior-neutral shape work with a golden
      policy to decide (the emitted functor call sites change).
    - FAMILY #4 = eager-INGEST externalization: the eager web
      (ExtendEagerProcedure / build_explicit_loop / BuildEagerRegion)
      is the LAST hand-coded emission surface; kIngestFold is a
      reserved DROpKind with no discovery seam. Build the seam (an R1e
      eager-web externalization), then lower kIngestFold from the
      DR-IR, deleting the hand-coded ingest folds — the same
      delete-with-cutover discipline R2 used.
    - R4 = JOIN group_ids reshape for native self-joins (IdeasTriage
      #6, the measured cubic tc⋈tc witness blowup) — gated in-scope
      per §7 owner decision, ONLY with a reviewed invariant-
      preservation argument for the CSE guard. Belongs inside the
      rebuild, not as a bolt-on.

(b) SUBGRAPHS / DEMAND per the original AggregatingFunctors §4
    sequencing — the next operator family after aggregates in the
    recorded plan. A DR-IR operator family in the R3 mold.

Residue items (early work under EITHER theme, from §11):
StateCell dead-group compaction; shared-input drain fusion (effect-
graph CSE — the DR-IR's first genuine REWRITE, the optimizability
thesis's first payoff); algebra class (II) mergeable-sketch lowering;
sorted-multiset MIN/MAX (the seekable-iterators residue, PerfRoadmap
§9-D5); KVINDEX dataflow-node deletion; aggregates over monotone
inputs (test the §C-4 enrollment path); the V-PRED-XCHECK residuals
(per-arm gate threading into EmitChainStep; EmitFrontierFilter cross-
check — finish the F17/F18 bug-class kill).

### 12.2 The three surfaces this epoch replaces, as pseudocode (SINGLE-
### PASS SEED at the delta-relational-IR close, from a structural read of
### the emission code — re-verify per the E-1..E-17 precedent before
### building anything; the anchors below are HEAD line numbers, and the
### stale-cite finding at the end of §12.2(B) is exactly why)

    (A) THE FUNCTOR-DELIVERY SURFACE TODAY — two incompatible calling
    conventions for the SAME thing (a driver-supplied C++ body invoked
    from generated code). MAP functors flow one way; aggregate/merge
    reductions flow another; average_weight.dr witnesses BOTH in one
    file.

      MAP functor (div_i32, add_i32 — @range functors; new_weight_i32
      is @range too and hits this surface as the KV mutable() merge):
        parse:   ParsedFunctor (+ the R3c-i algebra pragmas
                   IsInvertible/IsRecompute, Parse.h:694-704 —
                   "semantically inert" today, a manifest for R3's
                   lowering; MAP functors never carry them)
        df:      QueryMapImpl (Query.h:771), a MAP view holding the
                   ParsedFunctor
        cf:      EAGER build, NOT the DR-IR. BuildEagerRegion's MAP arm
                   (Build.cpp:1009-1017): pure -> BuildEagerGenerateRegion
                   (Generate.cpp:84 "Build an eager region for a
                   QueryMap") -> a GENERATE region (kCallFunctor)
                   carrying the functor. Impure -> assert-false
                   (Build.cpp:1016, the recorded gap).
        c++:     EmitGenerate (Database.cpp:2724): the call site is
      -            call = "functors." + name + "_" + BindingPattern
                   (Database.cpp:2736) — a MEMBER call on a threaded
                   `functors` object; the range shapes the wrapper
                   (filter -> if; 0/1 -> optional; 1:1 -> value;
                   0/1+..more -> range-for, :2769-2822).
                 THE CLASS: EmitFunctorsDecl (Database.cpp:1162-1254)
                   emits `struct DatabaseFunctors { ret name_pattern(
                   bound...); ... }` — one member DECLARATION per
                   non-inline functor; reduction functors (any
                   kAggregate/kSummary param) are SKIPPED (:1183-1194).
                 THE DRIVER: defines the member out-of-line —
                   `int32_t DatabaseFunctors::add_i32_bbf(...)`
                   (map_1.main.cpp:8; div_i32_bbf at
                   average_weight.main.cpp:40).
                 THE THREADING: `functors` rides as a by-ref parameter
                   `Functors &functors` through EVERY proc that
                   `uses_functors` — a per-proc effect flag set when a
                   GENERATE region has a non-inline functor
                   (Database.cpp:798), propagated across calls (:812),
                   declared (:842-843), passed (:897-898), and a
                   template parameter on the hidden-friend entry points
                   (init :1327-1330, message :1350-1353) and detail
                   twins (DetailTemplateHeader :264-273). The functor
                   object is STATE the engine plumbs everywhere.

      AGGREGATE / merge reduction (sum_i32/count_i32 @invertible;
      new_weight_i32 the KV @recompute — carries kAggregate/kSummary):
        parse:   same ParsedFunctor, but the algebra pragmas ARE
                   consulted (IsInvertible/IsRecompute pick the ABI)
        df:      AGG / KVINDEX view (not MAP) -> R3 GROUP_UPDATE op in
                   the DR-IR (BuildGroupUpdateOps)
        c++:     EmitStateCellStructs (Database.cpp:1076-1160): per
                   StateCell store, FORWARD-DECLARES driver FREE
                   functions in the header —
      +            @invertible:  F_identity(); F_combine(w,v);
      +                          F_uncombine(w,v)   (:1116-1120)
      +            @recompute:   F_reduce(vals, counts, n)  (:1122-1123)
                   — and a Reduce_<id> policy struct that bridges the
                   runtime Algebra interface to them by UNQUALIFIED
                   (ADL) call, NO `functors.` prefix, NO DatabaseFunctors
                   membership (:1129-1156). EmitGroupUpdate
                   (Database.cpp:1926) folds into the StateCellStore
                   (sc.Fold(gid,sign,val)); the reduction body is
                   reached inside the store's template via Reduce_<id>,
                   never through a threaded object.
                 THE DRIVER: defines FREE functions —
                   `int32_t sum_i32_combine(int32_t w, int32_t v)`
                   (average_weight.main.cpp:19-38).

      THE INCONSISTENCY, VISUALLY, in ONE driver
      (average_weight.main.cpp — same .dr, same TU):
      -   int32_t DatabaseFunctors::div_i32_bbf(int32_t, int32_t) {...}
      +   int32_t sum_i32_combine(int32_t w, int32_t v) {...}
        div_i32 is a MEMBER of a threaded object; sum_i32 is a FREE
        function reached by ADL from a template. Two ABIs, two mental
        models, one concept — and new_weight_i32 gets BOTH conventions
        in this one driver (member new_weight_i32_bbf for the mutable()
        merge delivery at :43; free new_weight_i32_reduce for the
        @recompute reduction at :29), the sharpest witness of the split.
        The C-5 ABI (§11 dev.7) chose free
        functions for reductions BECAUSE the engine must own the
        summary layout; the MAP surface predates that decision and
        still threads a `functors` object nothing but MAP delivery
        needs. §12.0(a) item 1 unifies them.

    (B) THE EAGER WEB — the last hand-coded emission surface, SEPARATE
    from and running BEFORE the DR-IR-owned stratum machinery.

      BuildEntryProcedure (Procedure.cpp:642-761) SEQUENCES:
        1. constant-TUPLE init -> BuildEagerRegion (Procedure.cpp:707)
        2. INGEST: per io, ExtendEagerProcedure (:714)    « the web »
        3. CompleteProcedure (:750): drain the eager work list
             (deferred joins/products/inductions)
        4. BuildStratumPhases (:756)          « the DR-IR, §11 »
        5. PublishDifferentialMessageVectors (:759)
      The comment at Procedure.cpp:752-755 states the ordering contract:
      the stratum phases "run AFTER the ingest walk above (whose fold
      crossings park rows in the queues the drains consume)". The web
      never touches context.dr_flow and is never Lower*'d — a wholly
      separate, still-hand-coded surface.

      ExtendEagerProcedure (Procedure.cpp:10-128) EMITS, per message:
        build_explicit_loop lambda (:43-79), one loop per receive per
        polarity:
          VECTORLOOP over the param vec (:45-48)
          UPDATECOUNT fold, kNonRecursive, is_explicit (:54-57)  « the
            INGEST FOLD — the "message-support bit" toggle, :36-42 »
          VECTORAPPEND into TableDeltaVector(kAddQueue|kDeleteQueue)
            (:68-78)                                    « frontier park »
        deletion-capable receive: build_explicit_loop x2 (add + remove,
          :85-90), then STOP — consumers run in the stratum phases
        monotone receive: VECTORLOOP + non-explicit add fold on fresh
          rows (:104-112) + per-index Add implied, then descend the
          eager walk: BuildEagerInsertionRegions (:125-126)
      BuildEagerInsertionRegionsImpl (Build.cpp:740-894): the fold +
        successor fan-out — InTryInsert -> BuildUpdateCount fold
        (Build.cpp:723-725), a PARALLEL over successors (:758-759), an
        add-queue frontier append for a non-inductive differential view
        (:766-771), the successor loop (:832-866) with the CUT test
        (:857-861: a CanReceiveDeletions / IsAggregate / IsKVIndex
        successor is fed by its OWN table's stratum phases / GROUP_UPDATE
        — do NOT descend), and a net-additions append for a monotone
        table whose consumer needs a frontier (:886-893).
      BuildEagerRegion (Build.cpp:973-1056): the type-switch dispatcher
        — JOIN (:978), MERGE (:987), AGG/KVINDEX (:997-1007, chain-
        BREAKER, returns), MAP (:1009, §12.2(A)), COMPARE (:1019),
        SELECT (:1022), TUPLE (:1039), INSERT (:1043), NEGATE (:1048-
        1051, the eager negate gate). Recursion goes through
        BuildEagerInsertionRegions per successor.
      NetBatch: emitted in BuildIOProcedure, NOT the entry proc — a
        NETBATCH op (Procedure.cpp:438) guarded on a differential
        message, netting explicit adds against removes before the CALL
        to the flow proc.
      Shared mutable state: Context &context (work list,
        monotone_negated_tables, delta-vector registry), the OP *parent
        cursor mutated as folds nest, TABLE *last_table / already_added
        (the "already-folded this model" guard), impl->work_list drained
        by CompleteProcedure.

      kIngestFold IS RESERVED WITH NO SEAM (DR.h:126-127): "§2.1
      INGEST_FOLD: entry-proc message->table seed (R1d cut — see the
      eager-walk inventory note; NOT populated in R1d)". The R1d cut
      rationale (DR.cpp:1640-1649, echoed in DeltaRelationalIR.md R1d
      §782-791 + the kIngestFold DECISION §950-955): the message->table
      fold sites "live inside the recursive BuildEagerRegion walk with
      no externalized discovery struct to mirror faithfully the way the
      stratum bands externalize {branches,joins,crossovers,products}."
      Unlike the DISCOVERY half of the stratum web (which R1 could
      shadow), the eager web has no discovery object at all — the folds
      are emitted inline as the walk descends. Building that discovery
      object IS §12.0(a) item 2 (the R1e seam).

      WHERE V-PRED-XCHECK DOES NOT REACH (§11 residuals + this read).
      V-PRED-XCHECK (commit e355959) cross-checks EmitJoinFire (matrix
      vs arm-plan spines), EmitClaimDrain (vs ClaimGate), and
      EmitChainStep's negate gate (vs NegateGatePred, DR.h:158-160
      Site 1). It does NOT reach: EmitFrontierFilter (reads
      un-cross-checked); EmitChainStep's per-arm gate (checked against
      the model function, not the actual per-arm gate NODE); site-2
      correlation is table-keyed where position-keyed is wanted for
      self-joins; and the ENTIRE eager INGEST web is outside the DR
      model, so it is never predicate-cross-checked at all. Completing
      the F17/F18 bug-class kill = closing these; item 2's seam is where
      the eager web finally enters the cross-checked model.

      SEED-DEFECT FINDING (report loudly, per the house precedent — this
      is exactly the E-1..E-17 pattern): the DeltaRelationalIR.md R1d
      note (§783-784) and DR.cpp cites (1643/1645) name "Build.cpp:1002"
      as the sole eager-negate-gate site; the LIVE dispatch is
      Build.cpp:1048-1051 (the MAP arm is now at :1009, AGG/KVINDEX at
      :997). The cited line numbers are STALE relative to HEAD. Any R1e
      seam that trusts those cites will inventory the wrong region.
      Re-derive the eager-walk dispatch table from Build.cpp:973-1056
      before building the seam.

    (C) GROUP_IDS — the load-bearing CSE guard R4 must preserve.

      RelabelGroupIDs (Optimize.cpp:408, doc :404-407): each JOIN /
        AGGREGATE / KVINDEX view SEEDS a unique nonzero scalar group_id
        (Query.h:465), pushed into its own sorted group_ids SET
        (Query.h:457) (Optimize.cpp:423-429); every other view seeds 0.
        A deepest-first fixpoint (Optimize.cpp:448-481) propagates each
        pivot's label DOWN to all views feeding it (a user's scalar or
        whole set is merged into its inputs, :458-471; sets sorted+
        uniqued, :473-475). Asserts `view != user` (:459) — the
        "no view is ever its own direct user" invariant. Refreshed
        before CSE and after each merge (:305, :372).
      InsertSetsOverlap (View.cpp:1468, doc :1455-1465): true iff two
        views' sorted group_ids sets intersect (linear merge-walk,
        :1482-1494). The doc's own witness: node_pairs(A,B):-node(A),
        node(B) — "two selects ... structurally the same, but cannot be
        merged because otherwise we would not get the cross product."
      THE SELF-JOIN CUBIC WITNESS (IdeasTriage #6, :41-44 — "the
      measured cubic tc⋈tc witness blowup"). Mechanism, confirmed:
        tc(x,z):-tc(x,y),tc(y,z) builds ONE DISTINCT SELECT per
        predicate occurrence, each over the SAME relation pointer:
        BuildClause -> one view per predicate (Build.cpp:1869-1874),
        BuildPredicate mints a fresh view via query->selects.Create
        (Build.cpp:246); the two distinct SELECTs feed ONE JOIN as
        distinct pivots (FindJoinCandidates, Build.cpp:2079).
        They are STRUCTURALLY EQUAL but CSE cannot merge them: both
        carry the JOIN's group_id in their propagated sets, so
        InsertSetsOverlap is true and every Equals returns false on it
        (the veto call in Select.cpp:237, Join.cpp:470, Tuple.cpp:278,
        Merge.cpp:1673, Compare.cpp:399, Map.cpp:366, Aggregate.cpp:293,
        KVIndex.cpp:76). If they WERE merged, Join.cpp:449-453 collapses
        the single-input join to a pass-through TUPLE
        (ConvertTrivialJoinToTuple) — a MISCOMPILE. The distinctness IS
        the cubic materialization; making it native (an intra-JOIN
        self-join with a witness index rather than two SELECTs +
        group_id veto) is the R4 payoff.
      THE INVARIANT WEB R4 MUST PRESERVE (CLAUDE.md "Core invariants
      (dataflow)", the load-bearing clauses):
        - no view is its own direct user (RelabelGroupIDs assert,
          Optimize.cpp:459; DirectlyUsesColumnsOf guard :56);
        - CSE never folds a unit SELECT into a non-unit one; a JOIN
          pivot whose non-user side is a unit relation is never removed;
        - a table's member-view list holds each view at most once BY
          IDENTITY — never dedup structurally (Data.cpp:231-249;
          DR.cpp:734-742 V-MEMBER-ID) — "distinct-but-equal views
          sharing a model are intentional, the group_ids CSE guard";
        - keep-last-edge (canonicalization never severs the last
          input-column edge);
        - group_ids are NEVER folded (the InsertSetsOverlap guard stays
          inside Equals, Optimize.cpp:94-95).
      An R4 that reshapes JOINs for native self-joins must show that
      every path currently relying on the distinct-SELECT + group_id
      veto (CSE non-merge, trivial-join non-collapse, member-view
      identity) is preserved OR made unnecessary by the new
      representation — before any code.

### 12.3 The path forward as DIFFS against §12.2 (SINGLE-PASS SEED —
### re-rank and re-verify per precedent; prediction slots are
### pre-registered, not results)

    P1  MAP-FUNCTOR MIGRATION onto the unified free-function surface
        (§12.2(A); §12.0(a) item 1). Behavior-neutral shape work.
        THE GENERATED-CODE DIFF (EmitGenerate call site,
        Database.cpp:2736):
      -   call = "functors." + name + "_" + BindingPattern
      +   call = name + "_" + BindingPattern    « free function, ADL »
        THE CLASS DIFF (EmitFunctorsDecl, Database.cpp:1162-1254):
      -   struct DatabaseFunctors { ret name_pattern(bound...); ... };
      +   ret name_pattern(bound...);   « free forward-decl per functor,
      +                                    mirroring EmitStateCellStructs
      +                                    :1113-1124 »
        THE THREADING DIFF (open design point — re-verify): with MAP
        calls no longer needing a threaded object, the uses_functors
        machinery (Database.cpp:798, :812, :842-843, :264-273) that
        exists SOLELY for MAP delivery becomes dead; but the hidden-
        friend entry points still take `Functors &functors` for driver
        ABI stability (drivers construct a DatabaseFunctors and pass it).
        DECIDE: keep the vestigial entry-point param (minimal churn) or
        drop it (a wider driver diff). Recommend keeping it this epoch —
        the param is the ABI seam a later object-owned-state functor
        design (WASM functors, memory: wasm-functor-direction) may
        reclaim.
        THE CORPUS-DRIVER DIFF SHAPE (every functor driver):
      -   int32_t DatabaseFunctors::add_i32_bbf(int32_t l, int32_t r)
      +   int32_t add_i32_bbf(int32_t l, int32_t r)
        THE GOLDEN QUESTION: case STDOUT must be byte-identical (pure
        delivery-mechanism change), but EVERY driver .main.cpp with a
        MAP functor member changes, AND the generated datalog.h changes
        (member decl -> free decl; call sites lose `functors.`). GATE:
        stdout goldens byte-identical (this is a HARD zero-churn gate —
        a stdout diff is a bug); driver files edited in the same diff;
        generated-header change is expected and reviewed, NOT blessed as
        a golden (headers aren't goldens). PRE-REGISTERED PREDICTION:
        driver-file churn = every cases/*.main.cpp that defines a
        `DatabaseFunctors::` member (count them at epoch start — the
        map_1..map_5 + average_weight + any negate/compare cases with
        functors); expected STDOUT golden churn = ZERO across all 164 x
        4 modes; expected oracle/monotone golden churn = ZERO.

    P2  FAMILY #4 — EAGER-WEB EXTERNALIZATION as an R1e seam (§12.2(B);
        §12.0(a) item 2). The R2 delete-with-cutover pattern applied to
        the last hand-coded surface.
        (i) DISCOVERY OBJECT the eager web needs (what R1 externalized
            for the stratum bands, now for ingest): an INGEST-FOLD
            INVENTORY per (message x table) carrying the effect set —
            polarity (add/delete/both), is_explicit, the target table,
            the queue it parks into (kAddQueue / kDeleteQueue /
            kNetAdditions), and the per-index Adds implied on fresh
            rows. Derive it INDEPENDENTLY from the Query (the receives +
            their data models), never by copying ExtendEagerProcedure —
            then cross-check against the emitted folds by an always-on
            validator (the V-OLD-EQUIV discipline R1 used).
        (ii) kIngestFold CONSTRUCTION: populate the reserved DROpKind
            (DR.h:126) from the inventory; add the graph validators
            (effect-set totality, one op per message-receive-polarity,
            queue-role agreement with the table's VecRole).
        (iii) LOWERING dr -> cf: LowerIngestFold emitting the VECTORLOOP
            + UPDATECOUNT + VECTORAPPEND that build_explicit_loop
            (Procedure.cpp:43-79) emits today, generically.
        (iv) DELETE-WITH-CUTOVER (the R2 pattern): delete
            ExtendEagerProcedure / build_explicit_loop in the SAME diff
            that lands the lowering, oracle + permcheck.py as bless
            referees. Note the eager NEGATE gate (Build.cpp:1048-1051)
            and the recursive BuildEagerRegion successor walk are
            ADJACENT but distinct — scope the seam to the INGEST folds
            first (the message->table seeds), leaving the successor walk
            for a follow-on, exactly as R2 took acyclic families first.
        V-PRED-XCHECK COMPLETION RIDES THIS: once the eager web is in the
        DR model, EmitFrontierFilter and the per-arm-gate-node cross-
        checks (§12.2(B) residuals) can be added on the same model —
        finishing the F17/F18 bug-class kill (§12.0 residue).
        PRE-REGISTERED PREDICTION: goldens are the semantic net, NOT
        byte-identity of generated text (emission shape MAY move — the
        §7 permutation-only bless policy applies, oracle + monotone
        referees); expected SUITE churn = ZERO (164 PASS); expected
        FINDINGS.md entries only if the independent derivation exposes a
        real ingest miscompile (the house bet: it will find at least one
        residual, per E-1..E-17).

    P3  R4 — GROUP_IDS RESHAPE for native self-joins (§12.2(C);
        §12.0(a) item 3). GATED IN-SCOPE per §7; ONLY with a reviewed
        invariant-preservation argument.
        DIFF SKETCH (not committed — the shape is the epoch's design
        question): represent a self-join as ONE JOIN view with a
        witness/self-pivot annotation instead of two distinct SELECTs +
        the group_id veto, so the cubic tc⋈tc materialization is a JOIN
        the runtime can key with a self-index rather than a value-keyed
        cross of two SELECT scans. The df->dr table-access operator
        (§10.2, R1's unified bound-prefix read) is the natural home for
        the self-pivot access path.
        THE MANDATORY INVARIANT-PRESERVATION ARGUMENT (scope — the
        reviewed argument MUST show, for the §12.2(C) web): (a) CSE
        still cannot wrongly merge the two logical sides (whatever
        replaces the group_id veto is at least as strong); (b) the
        trivial-join collapse (Join.cpp:449-453) never fires on a
        genuine self-join under the new shape; (c) the member-view
        identity invariant (Data.cpp:231-249) still holds — a table's
        member-view list is never structurally deduped; (d) no view
        becomes its own direct user (RelabelGroupIDs assert survives or
        is discharged by construction); (e) unit-relation CSE rules and
        keep-last-edge are untouched. If ANY of (a)-(e) cannot be shown,
        R4 stays gated — it is the ONE item here that can silently
        miscompile. PRE-REGISTERED PREDICTION: R4 lands ONLY if the
        argument passes review; otherwise it re-seeds to a later epoch
        (as it has for two epochs).

    P4  RESIDUE ITEMS as early work (§12.0, either theme; one line each):
        - StateCell dead-group compaction (D5-style, moved() callback);
        - shared-input drain FUSION (effect-graph CSE — the DR-IR's
          FIRST genuine rewrite, the optimizability thesis's first
          payoff);
        - algebra class (II) mergeable-sketch lowering (ships (I)+(III)
          today);
        - sorted-multiset MIN/MAX (the seekable-iterators §9-D5 residue);
        - KVINDEX dataflow-node deletion (an upstream simplification);
        - aggregates over MONOTONE inputs (exercise the §C-4 enrollment
          path — corpus is all differential-input today);
        - the V-PRED-XCHECK residuals (ride P2's seam).

### 12.4 Bootstrap (next session)

Branch: off main once delta-relational-ir merges. Read IN ORDER:
docs/proposals/DeltaRelationalIR.md END TO END (§1-§12 — the epoch's
working ledger: R0 profile, the verified pipeline pseudocode, the
vocabulary v2/v3 evolution, every per-diff record, the big review, and
the §12 stage records incl. the epoch-close residue); THIS file's §11
landing record; AggregatingFunctors.md §4 (the operator-family
sequencing — subgraphs is next in the recorded plan); the binding
artifacts in DeltaRelationalIR.artifacts/ (v3-spec.md, v3-spec-
statecell.md, big-review-2026-07-16.md). Code anchors:
lib/ControlFlow/Build/DR.{h,cpp} (the object model + the always-on
validators + BuildGroupUpdateOps + DeriveDRStrata + LinearizeAndValidate
DRFlow), the Lower* family in Stratum.cpp (LowerDRFlow / LowerDRRounds
/ LowerCommitSweeps / LowerGroupUpdate), Runtime/StateCell.h,
lib/CodeGen/CPlusPlus/Database.cpp EmitGroupUpdate, and — for the
eager-web theme — Build.cpp's ExtendEagerProcedure / build_explicit_loop
/ BuildEagerRegion (the un-externalized ingest surface).
Method: the checkpoint method — re-verify §12.0 AND §12.2/§12.3 against
HEAD before building (§12.2/§12.3 are the SINGLE-PASS SEED, never
fleet-reviewed; the E-1..E-17 precedent says it WILL find a defect — the
stale Build.cpp:1002 cite flagged in §12.2(B) is the first one already);
design critique; hand-write the target IR for ONE real case
before generalizing; for any emission-shape change, decide the golden
policy with the owner FIRST (permutation-only bless with the
derivation-counter oracle + monotone projection as referees, per §7 —
zero churn is not automatic once functor call sites move). Gates: SUITE
PASS (164 baseline) with the §7 golden policy; ctest 3/3; the DR-IR
always-on validators compile-time green; bench neutral at the flagship
points for any runtime-touching diff; landing record appended HERE with
deviations for ratification. Environment gotchas carried (per §6.4/§8.4/
§10.6): export PATH="/Users/pag/Code/.brew/bin:$PATH"; macOS bash 3.2
(no `declare -A`); zsh does NOT word-split $flags (use `${=var}` or arg
arrays); NEVER rebuild the compiler mid-suite; NEVER time bench runs
concurrently with suite runs; generated-code TEXT is not run-stable
(pointer-keyed container orders — key comparisons on case+mode+knobs
semantics, never generated-text hashes).

## 13. Landing record (2026-07-16) — ADL/FUNCTOR-SURFACE EPOCH CLOSED

Branch `adl-functor-surface` off main 88058879. The epoch's working
ledger is docs/proposals/ADLFunctorSurface.md (§0 baseline, §1 the seed
re-verification errata E-18..E-26, §2/§2.1 the judged diff plans + owner
decisions, §3 the per-diff implementation records); the hand-written and
binding artifacts are ADLFunctorSurface.artifacts/ (p1-map1-target.md,
p2-ingest-inventory-target.md §§0-13, p3-tc-selfjoin-target.md).
Commits: 490a00a1 (ledger §0-§1, the fleet record), 482b9c1c (§2 + P1/P3
artifacts), 4596c317 (P2 revision record), 0a0f8ba9 (P1 LANDED),
3c7bf417 (P2 artifact §12 splice design), cdd139aa (P2/R1e LANDED),
a1fa617d (P2 CUTOVER LANDED), cf84efab (P2 stage iv LANDED), plus this
docs/close commit.

Method executed (the checkpoint method, per §12.4): fleet re-verification
of the §12.2/§12.3 seed BEFORE building (5 opus derivation lanes + 5
adversarial verifiers; the precedent held — errata below); the three
designs each written WITH its hand-written identity-target artifact and
adversarially judged (P1 REVISE→amended; P2 REVISE→full revision
round→re-judged; P3 GO); owner decisions taken BEFORE emission-changing
code (P1 golden policy, P2 staging, P3 scope); a Fable review before
EACH emission cutover (P1: APPROVE-WITH-NITS, nits landed in-commit;
P2 cutover: APPROVE-WITH-NITS, incl. independent confirmation that the
committed splice design was infeasible). EVERY review round found
something real: the P1 judge caught the unverifiable evm_func_parse
driver + the wrong byte-compare target; the P1 byte-compare gate caught
the identity artifact's unconditional-banner defect; the P2 judge proved
the original seam boundary was not a tree cut; the P2 re-judge caught
the unspecified splice mechanism; the R1e §7d cross-check ABORTED on the
artifact's wrong monotone queue-role rule (deep_chain_retract) on day
one; the implementation caught §12.3's id-stream infeasibility, and the
cutover Fable review proved it independently.

### Seed errata found by the pre-code re-verification (precedent held;
### full text in the ledger §1)

- E-18 (REAL-DEFECT): §12.2(A) said new_weight_i32 "carries
  kAggregate/kSummary" — false (bbf @range @recompute; it stays on the
  member surface and P1 migrates it).
- E-19: the "ADL" characterization is wrong corpus-wide — builtin args
  have no associated namespaces; the mechanism is ordinary unqualified
  lookup bound at template-definition point (decl-before-detail-defs is
  the load-bearing ordering, already satisfied at the :2918 slot).
- E-20: the threading list omitted the FORCED-QUERY entry point
  (EmitQueryFriends) — three Functors param families, not two.
- E-21: new_weight_i32_bbf is decl-only (no emitted call site); the
  two-ABI witness is a declaration-surface witness.
- E-22: the eager ingest surface is NOT one InTryInsert funnel —
  BuildEagerInsertRegion mints its own fold + the IsStream() PUBLISH
  branch (Insert.cpp).
- E-23 (the R4 pivot): group_ids NEVER outlive one CSE() call
  (window [Optimize.cpp:305, :378]; recomputed from scratch after every
  merge; CopyDifferentialAndGroupIdsTo transient).
- E-24: in-code stale cites (DR.cpp:1645 "Build.cpp:1002"; DR.h
  reserved/construct-alongside comments) — all fixed in R1e.
- E-25: GROUP_UPDATE shipped with NO census coverage — kIngestFold must
  not copy the gap (it didn't; GROUP_UPDATE's own census is residue).
- E-26: V-PRED-XCHECK site numbering; the impure-MAP assert is
  structurally unreachable behind the pre-pass diagnostic.

### What landed

P1 — THE TWO-ABI FUNCTOR SPLIT IS DEAD. Every driver-supplied functor
body is now a FREE FUNCTION: EmitGenerate's callee dropped the
`functors.` prefix (the sole emitted member-call shape); EmitFunctorsDecl
hoists per-functor declarations to free forward-decls before a
now-EMPTY `struct DatabaseFunctors {}` (the deduction anchor drivers
still construct and pass); banners gated on any_map_decl so functor-free
AND reduction-only headers are byte-identical to pre-P1; 13 corpus
drivers dropped `DatabaseFunctors::` in the same diff; the uses_functors
detail-threading web is left dead, not deleted (P4 residue, WASM-gated);
all three entry-point families keep the vestigial templated Functors
param. average_weight's driver now shows ONE convention.

P2 — THE EAGER INGEST SEAM, deletion-capable-first (stages R1e + cutover
+ iv): kIngestFold is populated (BuildDRInventory derives one op per
io × receive × polarity from the Query, never by copying the walk),
censused (expect + order-free per-op key multiset) and guarded by four
always-on validators including the monotone queue-role cross-check
against the walk-produced map; build_explicit_loop is DELETED —
a deletion-capable receive's two explicit folds lower from the DR-IR's
stage-1 op pair via LowerIngestFold at the ORIGINAL WALK POSITION
(id-stream identity), with MakeStageOneIngestFolds the single payload
authority; V-PRED-XCHECK Site 4 (EmitFrontierFilter) closed with per-key
op lookup. The monotone/descent surface (IF-1/IF-4, the hole contract)
is re-seeded to §14 with its prerequisites.

P3 — R4 RETIRED, NOT DEFERRED. Measured on dual witnesses and
independently re-derived by the judge: the "cubic tc⋈tc materialization"
premise is FALSE at HEAD — the self-join lowers to ONE table + TWO
hash-keyed indexes + a pivot loop, O(join output), and the self-pivot
access path is already first-class DR-IR vocabulary
(Lowering::kSectionWalk + PlanNode::bound_cols/pivot_col). The
invariant-preservation argument is discharged by the empty diff;
group_ids/InsertSetsOverlap re-labeled a CORRECTNESS GUARD (CLAUDE.md).
Any future R4 is an access-path item gated on a WCOJ/3+-way self-join
witness. The one real residual (the provably-redundant pivot re-compare,
elidable iff kSectionWalk with valid pivot_col) is measure-first P4
residue.

### THE NUMBERS (bench/BASELINE.md run 9)

A shape-only epoch: zero golden churn across all landed diffs (permcheck
never exercised — nothing to bless); generated code byte-identical on
the 10 pre-registered targets per diff; the P2 cutover raw-byte-identical
on full -ir-out + datalog.h in opt AND nocf; suite 164 unchanged; ctest
3/3 (runtime 43); Q5 spots flat all epoch (release/opt 0.11-0.12s,
debug/opt 0.87-0.95s @128 vs the run-8 149.4ms/925.1ms baselines).

### Deviations for ratification (SEED TO NEXT EPOCH)

RATIFIED 2026-07-16 (owner, at close — all five as recorded below):
the landed P1 banner gating, the landed R1e table-level monotone
queue-role rule (+ the two §7(c) scopings and the omitted publication
field), the P2 cutover authority shape (walk-position lowering from
constructor-fresh copies; §12.3 superseded by artifact §12.6; the
emitted-tree↔flow cross-check stays the §6 follow-on's obligation),
the P3 retirement + re-charter, and the FINDINGS.md no-entry
disposition. Nothing re-opens; the §14 seed carries them as settled.

1. P1 BANNER GATING: the identity artifact emitted the new header
   banners unconditionally, breaking its own functor-free byte-identity
   prediction; the landed form gates them on any_map_decl. The gate
   caught it — ratify the landed form.
2. R1e MONOTONE QUEUE-ROLE RULE: the artifact's receive-site successor
   test is wrong (deep_chain_retract — the cut is met at an interior
   same-table view); the landed rule quantifies over the table's member
   views. The mandated §7d cross-check caught it in-stage. Plus two
   §7(c) scoping corrections and no publication DROp field (artifact
   §13).
3. P2 CUTOVER AUTHORITY SHAPE (artifact §12.6): §12.3's phase-time
   dispatch via Context.ingest_par was INFEASIBLE (fold ids + the queue
   vecs' first mint would move past CompleteProcedure in the shared
   next_id stream); the landed shape lowers constructor-fresh copies of
   the ops at the original walk position, MakeStageOneIngestFolds the
   single authority, tie = shared constructor + frozen view_to_model +
   census abort. A per-compile emitted-tree↔flow cross-check is the §6
   follow-on's obligation.
4. P3 RETIREMENT (owner-ratified): R4-as-group_ids-reshape retired on a
   false premise, re-chartered as access-path work behind a WCOJ
   witness.
5. FINDINGS.md UNCHANGED: nothing escaped to a golden. All catches
   (banner defect, monotone-rule defect, splice infeasibility) were
   pre-commit, caught by the epoch's own gates/validators — recorded in
   the ledger per the §11 dev.9 convention.

EPOCH CLOSED. Next epoch per §14 (the subgraphs/demand seed, owner
theme §12.0(b)). The remaining hand-coded emission surface is the
MONOTONE receive fold + eager successor walk (the §6 hole-contract
follow-on, carried with prerequisites); the F17/F18 bug-class kill now
covers EmitFrontierFilter (Site 4) with the eager-web tree↔flow
cross-check as the recorded completion step.

## 14. Epoch-start addendum (2026-07-16, at the ADL/functor-surface
## close): the NEXT EPOCH SEED — the SUBGRAPHS / DEMAND epoch
## (SINGLE-PASS SEED by the closing session — to-be-re-verified per the
## house precedent: E-1..E-26 across SIX epochs, EVERY pre-code
## re-verification has found a real seed defect; budget for it. The
## §14.2/§14.3 pseudocode below was written FROM a structural read of
## HEAD (branch adl-functor-surface, tip cf84efab) and never
## fleet-reviewed — the anchors are HEAD line numbers, re-derive every
## one before building. Continue the E-numbering at E-27.)

### 14.0 Why an epoch here

OWNER DECISION was pre-registered at the delta-relational-IR close
(§12.0): theme (b), SUBGRAPHS / DEMAND, re-seeds after the ADL/functor-
surface epoch. That epoch has now closed (§13 landing record): P1
unified the functor-delivery ABI onto free functions, P2 externalized
the eager INGEST web into the DR-IR (kIngestFold, family #4, stage 1)
and closed V-PRED-XCHECK Site 4, and P3 RETIRED the R4 group_ids charter
(the cubic-blowup premise was false at HEAD — the self-join already
lowers to one physical table + two hash indexes + a pivot loop, worst-
case-optimal). The DR-IR now owns the ENTIRE stratum machinery AND the
last hand-coded emission surface (the ingest folds); it has ONE mature
new operator family (aggregates / GROUP_UPDATE) and ONE genuine rewrite
substrate proven twice (the R1e census caught a real interior-fold
divergence on day one; the P2 cutover deleted build_explicit_loop).

SUBGRAPHS / DEMAND is the NEXT operator family in the recorded plan
(AggregatingFunctors.md §4). The §4 sequencing is explicit about WHY it
comes after aggregates and WHAT substrate it shares:

  "→ aggregating functors + KV indices → subgraphs (sharing the
   keyed-instance substrate: an aggregate object keyed on (group,
   config) IS a keyed nested instance in miniature; config columns that
   are compile-time constants specialize the instantiation — the same
   lexical-scope move as the carve-outs and SLDMagic's use_query_const
   knob)."

That is the load-bearing claim this epoch cashes in: the StateCellStore
(a peer table keyed on (group ++ config), dense-group-id namespace,
engine-owned) that R3 built for aggregates IS the miniature of the
keyed-instance store a subgraph/demand family needs. A subgraph is a
demand-driven, keyed, nested query instance; an aggregate is the
degenerate case where the instance holds one folded scalar rather than a
whole sub-relation. The epoch's thesis: subgraphs EXTEND the R3
state-cell family the way R3 EXTENDED the §5.1 counter family — a new
effect sub-domain filled in an already-total matrix, a new op family
plugged into the R3 GROUP_UPDATE integration points, a new peer runtime
store generalizing StateCellStore.

CANDIDATE SCOPE (owner picks the cut at epoch start; this seed sketches
the whole surface so the boundary is an informed choice, not a default):
  (a) DEMAND TRANSFORM as a dataflow rewrite — the SLDMagic direction
      (memory: push-method-origin-and-negation; SLDMagic is "a
      transformation, never an evaluator"). Rewrite a query into a
      demand-guarded form so only reachable ground instances
      materialize. This is a Query-level (DataFlow) pass, upstream of
      the DR-IR, and is the biggest design surface.
  (b) KEYED NESTED INSTANCES as a DR-IR operator family in the R3 mold
      — a SUBGRAPH_INSTANTIATE op keyed on (context, config), a
      generalized peer store (InstanceStore, the StateCellStore's
      non-degenerate parent), demand frontiers as the input, published
      instance rows as the output. This is the R3-shaped increment: one
      new op family, one new effect sub-domain, census + validators from
      day one.
  (c) CONFIG-COLUMN SPECIALIZATION — the compile-time-constant config
      columns AggregatingFunctors §4 names ("config columns that are
      compile-time constants specialize the instantiation"). This is the
      bridge that also closes the R3 residue item "aggregates with
      configuration columns" (a clean-diagnostic gap today, CLAUDE.md).
      A config column is either a demand key (runtime) or a
      specialization constant (compile-time) — the same lexical-scope
      move for both families.

RECOMMENDATION ON RECORD (owner to ratify): scope stage 1 as (c)+(b)
narrowly — land config-column aggregates FIRST (the smallest real
extension of the R3 family, closing a named gap and exercising the
(group ++ config) key that is already in the StateCell key projection),
THEN generalize the state cell to a keyed instance store. Defer (a) the
full demand transform to a follow-on, the way R2 took acyclic families
before recursive ones and R3 took KV-as-degenerate-aggregate before the
general aggregate. The demand transform is the payoff; the keyed-
instance substrate is the prerequisite the recorded plan says to build
first.

### 14.0.1 Carried residue (early work under this epoch — the §13 ledger
### + the ADLFunctorSurface epoch's open records, each with its home)

RIDES §6 (the eager monotone/descent stage — see §14.0.2, EARLY WORK):
  - The §6 hole contract (LowerIngestFold owns the vector-loop /
    update-count / if-crossed region, exposes the if-crossed body as a
    HOLE the hand-coded descent fills; a validator asserts the hole is
    filled exactly once). Prerequisite: the interior-fold net-additions
    .dr WITNESS (Build.cpp:870-893: the net-additions append may sit at
    an ANCESTOR fold of a deeper view — never assume the receive owns
    it; the R1e census already caught this beyond nls on
    deep_chain_retract, §13 P2/R1e record). Prerequisite: the emitted-
    tree↔flow CROSS-CHECK obligation — the P2 cutover deviation left
    "no per-compile V-PRED-XCHECK analog ties the emitted CF tree back
    to the flow ops yet" (artifact §12.6); §6 is where the eager web
    finally enters the cross-checked model, completing the F17/F18
    bug-class kill for the last surface.
  - The interior-fold CSE-collapse QUESTION (artifact §5): once the
    net-additions append can migrate across the seam, does the shared
    interior fold become a CSE candidate (two receives folding into the
    same interior TUPLE)? This is the FIRST place shared-input drain
    fusion (below) would fire on a real corpus shape — resolve the
    question here or record it as fusion's first witness.

THE R3 STATE-CELL / GROUP_UPDATE RESIDUE (this epoch's own substrate):
  - GROUP_UPDATE / STATE_SEAL census coverage — E-25's carried gap,
    live at HEAD (DR.cpp:2761 `TODO(P4): the R3 GROUP_UPDATE/STATE_SEAL
    family still has NO census`; its only always-on guard is a plain
    assert DR.cpp:629, stripped under NDEBUG). THE NEW FAMILY MUST NOT
    COPY THIS GAP (E-25's lesson, promoted to a charter): census +
    op-family structural validators from day one. Closing GROUP_UPDATE's
    own gap is the FIRST task — it is the template the subgraph family's
    census follows, and leaving it open means the substrate this epoch
    extends is itself un-censused.
  - StateCell dead-group compaction — the D5-style moved() callback
    (StateCell.h:20-23: "StateCell compaction … is explicitly OUT of R3
    scope — a D5-style residue if a future COST measurement shows dead-
    group accumulation"). A keyed-instance store makes this sharper
    (instances can die; a dead-instance sweep is the InstanceStore
    analog). Measure-first (a COST witness of dead-group accumulation)
    before building.
  - Algebra class (II) mergeable-sketch lowering (ships (I) @invertible
    + (III) @recompute today; the sketch class is a lowering selector,
    v3-spec-statecell §0 C-0e).
  - Sorted-multiset MIN/MAX (the seekable-iterators §9-D5 residue,
    memory: seekable-iterators-wcoj; a sorted-layout hook exists at
    CompactRowsInPlace).
  - KVINDEX dataflow-node deletion (AggregatingFunctors §4.1(a): the
    KVINDEX node becomes a desugaring candidate for deletion once
    mutable() desugars to a degenerate aggregate; an upstream
    simplification, v3-spec-statecell §3 records the concrete
    recommendation).
  - Aggregates over MONOTONE inputs (the §C-4 enrollment path,
    untested — the corpus is all-differential-input today, CLAUDE.md
    "aggregates over INDUCTION-OWNED inputs" is a separate reject).
  - Aggregates with CONFIGURATION COLUMNS (a clean-diagnostic gap
    today) — this is candidate (c) above, PROMOTED from residue to the
    recommended stage-1 core.

THE ADL/FUNCTOR-SURFACE EPOCH'S OTHER OPEN RECORDS (§13):
  - Shared-input drain FUSION (effect-graph CSE — the DR-IR's FIRST
    genuine REWRITE, the optimizability thesis's still-unclaimed
    payoff). The R1e census + the P2 cutover PROVED the substrate is
    optimizable but did not yet OPTIMIZE. Fusion's first witness is the
    §6 interior-fold CSE question (above); a demand transform
    (candidate (a)) multiplies the shared-input opportunities (many
    demand-guarded subgoals read the same base relation). Fusion is the
    natural rewrite the subgraph family's shared demand frontiers ask
    for — the epoch's chance to cash the thesis.
  - uses_functors DEAD-ARM excision (§13 P1 record: the detail-threading
    arms went DEAD, NOT DELETED, after the free-function migration; the
    entry-point Functors params stay for ABI stability). Clean excision
    is gated on the WASM object-owned-state decision (memory: wasm-
    functor-direction — the functor surface P1 just unified is the WASM
    substrate; the vestigial Functors param is the ABI seam a WASM
    engine-owned-state functor design reclaims). Keep it dead this epoch
    UNLESS a WASM spike (permitted early work, AggregatingFunctors
    §4 last para) reclaims the param.
  - The func.Name()-vs-Sanitize decl/call ASYMMETRY (§13 P1 FABLE
    review, recorded for P4): the free-function forward-decl uses
    `func.Name()` (Database.cpp:1271) while call sites use `Sanitize(...)`
    — a latent divergence for any functor name needing sanitization
    (none in-corpus today). A one-line correctness cleanup; fold into
    the first Database.cpp comment sweep.
  - The kSectionWalk REDUNDANT PIVOT-COMPARE elision (P3's one genuine
    residual, artifact §3: the `if (v27 == v30 && v27 == v31)` inner-
    loop compare the section-walk index already guarantees, elidable iff
    the ACCESS node is kSectionWalk with a valid pivot_col, DR.h:330/344).
    MEASURE-FIRST (P3's estimate: "at best one redundant register
    compare in the inner loop" — decoupled from group_ids, filed as
    measure-first emission cleanup). Do NOT build without a COST witness.

RE-CHARTERED (P3 retirement, owner-ratified §2.1 decision 3): R4 is now
a MATERIALIZATION / ACCESS-PATH item gated on a CONCRETE WCOJ / 3+-way
self-join WITNESS (memory: seekable-iterators-wcoj — the retired R4's
re-motivation bar). group_ids / InsertSetsOverlap are a CORRECTNESS
GUARD, never an optimization target (E-23: they never outlive one CSE()
call). A 3+-way self-join witness that measurably blows up would be an
ACCESS-PATH-epoch item, NOT this epoch — subgraphs/demand does not
depend on it. Note it here so the re-motivation bar is not lost.

### 14.0.2 The §6 monotone/descent stage is EARLY WORK this epoch (its
### prerequisites, restated as a checklist)

The ADL/functor-surface epoch landed P2 stage 1 (deletion-capable ingest
folds lower from the DR-IR) but explicitly DEFERRED the monotone/descent
folds (IF-1/IF-4 receive classes) behind the §6 hole contract
(ADLFunctorSurface.artifacts/p2-ingest-inventory-target.md §6). That
stage is the natural EARLY WORK of this epoch — it finishes externalizing
the last hand-coded emission surface, and it is the setting where
shared-input drain fusion first has a corpus witness. The §6 stage ships
IFF these prerequisites are met (all named in the artifact, re-verify):

  1. THE HOLE CONTRACT (artifact §6:436-448): LowerIngestFold emits the
     fold and RETURNS the if-crossed body OP* cursor;
     BuildEagerInsertionRegions (still hand-coded) is invoked with that
     cursor as `next_parent` so the descent emits INTO the DR-owned fold
     body; a validator asserts the hole is filled EXACTLY ONCE (empty
     for kEmpty monotone-no-successor; one descent subtree otherwise).
  2. THE INTERIOR-FOLD WITNESS (artifact §5/§6:446-448 + §13 P2/R1e
     record): a .dr case where the net-additions append sits at an
     ANCESTOR fold of a deeper view (Build.cpp:870-893). deep_chain_retract
     already witnesses this for the R1e census; §6 needs it as a LOWERING
     witness (does the descent claim the append, or does the seed?).
  3. THE CSE-COLLAPSE QUESTION (§14.0.1, ties §6 to fusion): if the
     net-additions append can migrate across the seam, is the shared
     interior fold a fusion candidate? Resolve or record as fusion's
     first witness.
  4. THE EMITTED-TREE↔FLOW CROSS-CHECK OBLIGATION (P2 cutover deviation,
     artifact §12.6:1017-1024): the stage-1 cutover left the emitted CF
     tree un-cross-checked against the flow ops. §6 must ADD the
     per-compile V-PRED-XCHECK analog for ingest (the eager web's entry
     into the cross-checked model). Site numbering continues after Site 4
     (EmitFrontierFilter, §13 P2 stage iv).

### 14.2 The surfaces this epoch extends, as pseudocode (SINGLE-PASS SEED
### at the ADL/functor-surface close, from a structural read of the R3
### state-cell family + the parse/dataflow surface — re-verify per the
### E-1..E-26 precedent before building; the anchors below are HEAD line
### numbers on branch adl-functor-surface, tip cf84efab)

    (A) THE KEYED-INSTANCE SUBSTRATE TODAY — R3's StateCellStore, the
    miniature a subgraph family generalizes. This is the surface the
    epoch EXTENDS, not replaces.

      RUNTIME: StateCellStore (include/drlojekyll/Runtime/StateCell.h) —
        standing per-group state for ONE aggregating functor. Keyed by
        (group cols ++ config cols) → a DENSE GROUP ID (0..num_groups),
        allocated on first touch exactly like RowStore::FindOrAdd
        (StateCell.h:9-25 doc; the NON-ALIASING invariant: StateCell
        dense group ids are a SEPARATE id space from the agg DiffTable's
        row ids — a DiffTable compaction does not touch the store,
        StateCell.h:10-25). TWO-WORD CELL per group (StateCell.h:26-37):
        `working` (WRITTEN by Fold, READ by Emit — a real read/write
        hazarded value) + `sealed` (batch-start snapshot, WRITTEN only by
        Seal, READ by Old — a frozen kInI-like value). OCCUPANCY bit +
        per-group WORKING member count (StateCell.h:38-48: the batch-1
        abort fix; occupied ⇔ count > 0; drives the occupancy-generalized
        emit_touched guard — birth: +new only; death: −old only; change:
        −old,+new). The store is "the SINGLE new emitted primitive R3
        adds" (v3-spec-statecell §1:130-132).
        THE GENERALIZATION: an INSTANCE store is a StateCellStore whose
        per-key cell holds a whole nested query instance (a sub-DiffTable
        or an instance handle) rather than a two-word scalar. The keying
        ((context ++ config) → dense instance id), the non-aliasing
        invariant, the occupancy/birth-death guard, and the dead-instance
        compaction residue ALL transfer. This is the R3-mold increment.

      DATAFLOW: the AGG / KVINDEX view (Query.h node classes) → ONE
        GROUP_UPDATE op (BuildGroupUpdateOps, DR.cpp:638). The op carries
        agg_view (a QueryAggregate or desugared QueryKVIndex,
        distinguished by AggProvenance), algebra (the lowering selector),
        agg_table (its OWN differential DiffTable — V-AGG-SOLE), input_view
        (the single summarized relation, NO join partner), statecell_id,
        and group_cols / summary_cols (the state-cell key / value column
        projections — DR.h:523-540). group_cols is ALREADY "group ++
        config (the cell key)" (DR.h, the field comment) — config columns
        are already in the key projection; the residue gap is that no
        corpus case EXERCISES a non-empty config today.

    (B) THE EFFECT MATRIX A NEW OPERATOR FAMILY EXTENDS — the R3 pattern
    to follow, verbatim.

      The v3-spec §2 effect domain (v3-spec.md §2:107-126) declares FIVE
      sub-domains, TOTAL from day one so the scheduler and validators are
      R3-ready before R3 exists:
        vector:    append(V) | drain(V) | clear(V)
        table:     counter±(T, class∈{NonRecursive,Recursive,Explicit})
        flags:     read(T, F) | write(T, F)  for F ∈ RowFlags
        kInI:      read-frozen(T)  — a distinguished always-legal read,
                     never a WAR/WAW hazard (the scheduler treats it as a
                     constant)
        statecell: fold(SC) | emit(SC) | old(SC)  — was RESERVED for R3
                     (v3-spec.md §2:123-126: "the domain exists so the
                     scheduler and validators are R3-total from day one"),
                     now LIVE (EffKind::kStateFold/kStateEmit/kStateOld,
                     emitted by BuildGroupUpdateOps at DR.cpp:699/709/713;
                     the E-24b in-code stale-comment "R3 — reserved" was
                     the ADL epoch's first comment-sweep fix).
      HOW R3 EXTENDED IT (the template, v3-spec-statecell.md §0): R3 did
      NOT add a sixth top-level domain — it FILLED the reserved statecell
      sub-domain and resolved the three conflicts that filling it exposed
      (§0 C-0b/C-0c/C-0e):
        - C-0b: a StateCell is NOT a pure constant like kInI (fold() WRITES
          the working value in the SAME band, so a within-band Fold-then-
          Emit is a real RAW hazard) NOR a plain counter (no presence
          crossing). It needed the two-word cell (working hazarded, sealed
          frozen) to keep the frozen-read dichotomy intact.
        - C-0c: a value-fold ≠ a counter-fold — "value-fold without
          immediate crossing" is a FIRST-CLASS effect kind, not an
          overloaded counter (resolves G-2).
        - C-0e: the algebra attribute (@invertible/@recompute) is a
          LOWERING SELECTOR (fold-via-unfold vs fold-via-rescan), exactly
          like ACCESS's `Lowering how` (point-test | section-walk | scan |
          seek, v3-spec.md §2:159) — NOT the ordering-as-enum anti-pattern
          §9 F-1 rejects. It picks an implementation; it pins no order and
          no def/use edge.
      THE SUBGRAPH EXTENSION FOLLOWS THIS SHAPE: a keyed-instance family
      adds an effect sub-domain (candidate spelling: `instance:
      instantiate(I) | demand(I) | publish(I)`), declared TOTAL from day
      one, and resolves the conflicts filling it exposes — the sharpest
      being: is a demand frontier a new hazarded write (like fold) or a
      frozen read (like kInI)? A demand is READ by the instantiation and
      WRITTEN by the demanding subgoal in a possibly-earlier band — the
      C-0b question, one level up. The seed does NOT resolve it (that is
      the epoch's first design task); it flags that the R3 resolution
      pattern (two-word cell = hazarded working + frozen snapshot) is the
      precedent to test against.

    (C) THE ABSENCE OF ANY SUBGRAPH/DEMAND HOOK — this is NEW surface,
    so the pseudocode is the SURROUNDING code the family must plug into.

      GREP CONFIRMS NO OPERATOR-FAMILY HOOK EXISTS (verified at HEAD):
        no `subgraph` / `demand` / `magic` / `SLDMagic` / `sideways` /
        `use_query_const` operator surface in lib/Parse, lib/DataFlow,
        lib/ControlFlow, or include/drlojekyll. The 18 `subgraph`/`demand`
        hits are INCIDENTAL PROSE — dataflow "subgraphs" (Optimize.cpp:31,
        :708; Compare.cpp:56), column "demand" (Compare.cpp:569-874,
        Join.cpp:155), and a clause-factoring "demand" (Clause.cpp:12).
        The family is entirely NEW; the seed must pseudocode the plug-in
        points, not an existing surface.

      THE R3 GROUP_UPDATE INTEGRATION POINTS — THE PLUG-IN CHECKLIST
      (verified at HEAD; the new family shadows each):
        1. df→dr CONSTRUCTION: BuildGroupUpdateOps (DR.cpp:638) — called
           per AGG (DR.cpp:1034) and per desugared KVINDEX (DR.cpp:1048),
           mints ONE kGroupUpdate op (DR.cpp:676) with its effect set
           (kStateFold/Emit/Old, DR.cpp:699-713) + a kStateSeal op
           (DR.cpp:754). The subgraph analog: a BuildSubgraphOps minting a
           SUBGRAPH_INSTANTIATE op per subgraph view.
        2. STRATUM LIFT: DeriveDRStrata (DR.cpp:2058) — a monotone integer
           lift replicating TableOwnerStratum (the max spec stratum over a
           table's member views, DR.cpp:2064-2072); the initial-stratum
           rules per unit (DR.cpp:2090-2094: head→owner_stratum, join→join
           view stratum, crossover→negate stratum, product→product stratum).
           The subgraph op needs its OWN initial-stratum rule (a nested
           instance's stratum vs its demanding context's — the
           stratification design question).
        3. LINEARIZER BAND KEY: LinearizeAndValidateDRFlow (DR.cpp:2963) —
           a Kahn linearizer under a band-key tie-break (DR.cpp:3543-3555:
           ready-set ordered by band key; the band key IS the emission
           walk order). GROUP_UPDATE rides an existing band; a demand
           frontier introduces a NEW ordering question (demand must be
           satisfied BEFORE the instance materializes — a band-hazard the
           V-BAND-HAZARD validator must be taught).
        4. CENSUS + VALIDATORS: ValidateDROps (DR.cpp:2323) — the DERIVED-
           vs-DERIVED census (recompute expected op counts + per-op key
           multisets from impl/context/query, NEVER a tree walk, DR.cpp:
           2646-2813). THE R3 GAP (E-25, LIVE): GROUP_UPDATE/STATE_SEAL is
           NOT in the expect() list (DR.cpp:2810-2813 has kSeedFold/
           kChainFold/kFixpointFire/kClaimDrain/kIngestFold — no
           kGroupUpdate); the TODO(P4) at DR.cpp:2761 records it. THE NEW
           FAMILY SHIPS ITS CENSUS FROM DAY ONE (E-25 promoted to charter),
           and closing the GROUP_UPDATE gap is the first task (the
           template the subgraph census copies).
        5. dr→cf LOWERING: LowerGroupUpdate (Stratum.cpp:1363) — folds the
           input table's net frontiers into the cell (band a) + emit_touched
           (band b), dispatched from LowerDRFlow (Stratum.cpp:1596). The
           subgraph analog: a LowerSubgraphInstantiate emitting the demand-
           guarded instantiation + publication.
        6. cf→c++ EMISSION: EmitGroupUpdate (Database.cpp:1970, region
           ProgramGroupUpdateRegionImpl at Program.h:1067) — folds into
           the StateCellStore via the Reduce_<id> policy struct; the store
           structs emitted by EmitStateCellStructs (Database.cpp:1076,
           Key_<id> + Reduce_<id> per cell, Database.cpp:2967 in the header
           assembly). The subgraph analog: an EmitSubgraphInstantiate + an
           InstanceStore struct generalizing the StateCell blob.
        7. REDUCTION-BODY ABI: the C-5 driver-supplied free functions
           (post-P1, the WHOLE functor surface is free functions,
           Database.cpp:1271 decl / Sanitize call sites) — a subgraph
           instance needs no reduction body, but a DEMAND PREDICATE (which
           ground instances to materialize) may want the same driver-hook
           ABI. The unified free-function surface P1 landed is the seam.

### 14.3 The path forward as DIFFS against §14.2 (SINGLE-PASS SEED — re-
### rank and re-verify per precedent; prediction slots are PRE-REGISTERED,
### not results; the recommended cut is (c)→(b)→(a), owner ratifies)

    P0  CLOSE THE R3 CENSUS GAP FIRST (E-25 residue → charter
        prerequisite). Before adding a new op family, the substrate it
        extends must be censused.
        THE DIFF (ValidateDROps, DR.cpp:2646-2813):
      +   exp_group_update = count of AGG + desugared-KVINDEX views;
      +   exp_state_seal   = count of statecells;
      +   expect(DROpKind::kGroupUpdate, exp_group_update, "group updates");
      +   expect(DROpKind::kStateSeal,   exp_state_seal,   "state seals");
      +   per-op key multiset (statecell_id, agg_table*, provenance,
      +     algebra) recomputed from the Query, compared order-free.
        Plus the missing op-family STRUCTURAL validator (effect-set
        totality: every kGroupUpdate has {kStateFold, kStateEmit,
        kStateOld}; every kStateSeal has {kStateFold global:rmw}; the
        promote-the-assert-DR.cpp:629 discipline). PRE-REGISTERED
        PREDICTION: zero emission change (observation-only, like V-PRED-
        XCHECK Site 4); SUITE PASS 164 byte-identical; the census may FIRE
        on a real GROUP_UPDATE miscount (the house bet — E-1..E-26 says a
        first-time census finds a divergence; the R1e ingest census did on
        day one). This is the template §14.3-P2's census copies.

    P1  §6 MONOTONE/DESCENT STAGE (the ADL epoch's deferred P2 stage 2;
        §14.0.2). EARLY WORK — finishes the last hand-coded emission
        surface + adds the eager web's V-PRED-XCHECK.
        THE DIFF (from artifact §6): LowerIngestFold (Stratum.cpp:1884)
        gains the hole-return shape —
      -   LowerIngestFold emits the fold, returns void;
      +   LowerIngestFold emits vector-loop / update-count / if-crossed,
      +     RETURNS the if-crossed body OP* cursor;
      +   BuildEagerInsertionRegions invoked with cursor as next_parent
      +     (descent emits INTO the DR-owned fold body);
      +   validator: hole filled EXACTLY ONCE (empty for kEmpty
      +     monotone-no-successor; one descent subtree otherwise);
      +   the net-additions append emitted by the DESCENT at its actual
      +     fold-nesting site, gated on the interior-fold witness;
      +   V-PRED-XCHECK Site 5+: emitted-tree↔flow cross-check for ingest.
        THE INTERIOR-FOLD CSE QUESTION resolved here or recorded as
        fusion's first witness (§14.0.2). PRE-REGISTERED PREDICTION:
        goldens are the SEMANTIC net (permutation-only bless per §7,
        oracle + monotone referees) — emission shape MAY move as the
        descent reparents into the DR-owned hole; expected SUITE churn
        ZERO (the emission is token-equivalent, the R2/P2 cutover
        precedent); FINDINGS entry iff the hole contract or the cross-
        check fires (the house bet).

    P2  THE NEW OPERATOR FAMILY — config-column aggregates (candidate (c),
        the smallest real R3 extension) then keyed nested instances
        (candidate (b)). Follows the §14.2(C) plug-in checklist.
        STAGE (c) — CONFIG-COLUMN AGGREGATES: the group_cols projection
        ALREADY carries "group ++ config" (DR.h:523-540); the gap is that
        no case exercises a non-empty config. The diff is mostly a
        DIAGNOSTIC-REMOVAL + a corpus case:
      -   configuration-column aggregates rejected (clean diagnostic);
      +   a config column is a demand key (runtime) OR a specialization
      +     constant (compile-time — AggregatingFunctors §4: "config
      +     columns that are compile-time constants specialize the
      +     instantiation"); the StateCell key already accommodates it;
      +     ADD an oracle-refereed corpus case (config_agg_1.dr +
      +     .batches + oracle/monotone goldens, the R3 corpus mold).
        STAGE (b) — KEYED NESTED INSTANCES: the R3-mold increment. ONE
        new op family (SUBGRAPH_INSTANTIATE) plugging into all seven
        §14.2(C) points; a new effect sub-domain (`instance:
        instantiate|demand|publish`, declared TOTAL); a new peer store
        (InstanceStore generalizing StateCellStore — same (context ++
        config) → dense id keying, same non-aliasing invariant, same
        occupancy/birth-death guard, same dead-instance compaction
        residue). CENSUS + VALIDATORS FROM DAY ONE (E-25 charter — do NOT
        copy the GROUP_UPDATE gap P0 just closed). THE FIRST DESIGN TASK
        (unresolved by this seed, §14.2(B)): is a demand frontier a
        hazarded write (fold-like) or a frozen read (kInI-like)? Test
        against the R3 two-word-cell resolution. HAND-WRITE THE TARGET IR
        for ONE real subgraph case before generalizing (the checkpoint
        method). PRE-REGISTERED PREDICTION: new corpus (suite grows past
        164 with the config + subgraph cases, each oracle-refereed —
        aggregate_1 FLIPPED diagnostic→golden at the R3 flip is the
        precedent); Q5 neutral (subgraphs are a new-feature path, not a
        Q5-chain change); the census fires at least once (the house bet).

    P3  THE DEMAND TRANSFORM (candidate (a), the payoff — DEFER to a
        follow-on unless owner scopes it in). The SLDMagic direction as a
        DataFlow REWRITE (memory: push-method-origin-and-negation —
        SLDMagic is a TRANSFORMATION, never an evaluator; keep it a Query-
        level pass upstream of the DR-IR). Rewrite a query into demand-
        guarded form so only reachable ground instances materialize;
        demand frontiers feed the §14.2(B) instance family's input.
        THE INVARIANT-PRESERVATION OBLIGATION (the R4-style gate — a
        DataFlow rewrite CAN silently miscompile; a reviewed argument
        required before code): (a) the demand rewrite preserves the
        stratification the Stratify pass computes (demand does not create
        an unstratified cycle the reject would have caught); (b) the
        group_ids CSE guard (E-23, a correctness guard never an
        optimization target) is untouched — demand-guarded copies of a
        subgoal must not wrongly CSE-merge; (c) the keep-last-edge and
        unit-relation CSE rules survive; (d) the transform is a rewrite
        the DR-IR then lowers, NOT a second evaluator. IF the argument
        does not pass review, P3 re-seeds (as R4 did for THREE epochs
        before its P3 retirement). PRE-REGISTERED PREDICTION: P3 lands
        ONLY with the reviewed argument; measure-first on a demand-benefit
        witness (a query that materializes far fewer instances under
        demand — the COST honesty referee, memory: perf-guiding-oracles);
        otherwise re-seeds.

    P4  RESIDUE as early work (§14.0.1; one line each, homed):
        - shared-input drain FUSION (the DR-IR's first genuine rewrite;
          first witness = P1's interior-fold CSE question; the demand
          transform multiplies its opportunities);
        - StateCell / instance DEAD-GROUP compaction (D5-style moved()
          callback; MEASURE-FIRST on a dead-group-accumulation witness);
        - algebra class (II) mergeable-sketch lowering;
        - sorted-multiset MIN/MAX (the §9-D5 seekable-iterators residue);
        - KVINDEX dataflow-node deletion (upstream simplification, once
          mutable() desugars to a degenerate aggregate);
        - aggregates over MONOTONE inputs (the §C-4 enrollment path);
        - uses_functors dead-arm excision (WASM-gated; a WASM whole-module
          spike is permitted early work, AggregatingFunctors §4);
        - the func.Name()-vs-Sanitize decl/call asymmetry (Database.cpp:
          1271; a one-line cleanup, fold into the first comment sweep);
        - the kSectionWalk redundant pivot-compare elision (MEASURE-FIRST,
          P3 artifact §3 — do not build without a COST witness);
        - R4 re-motivation: a concrete WCOJ / 3+-way self-join witness
          promotes the retired charter to an ACCESS-PATH epoch (NOT this
          one).

### 14.4 Bootstrap (next session)

Branch: off main once adl-functor-surface merges. Read IN ORDER:
docs/proposals/ADLFunctorSurface.md END TO END (this epoch's ledger —
the §1 errata E-18..E-26, the §2 designs incl. the P3/R4 RETIREMENT, the
§2.1 owner decisions, the §3 implementation records for P1 / P2-R1e / P2-
cutover / P2-stage-iv, the §3.1 artifact index); THIS file's §13 landing
record (the ADL/functor-surface epoch close); AggregatingFunctors.md §4
END TO END (the operator-family sequencing — subgraphs shares the keyed-
instance substrate; §4.1 the mutable() surface decision) + §5 (the
implementing-session method); the R3 binding artifacts
(DeltaRelationalIR.artifacts/v3-spec.md §2 — the five-domain effect
matrix the new family extends; v3-spec-statecell.md §0/§1/§2 — HOW R3
filled the reserved statecell domain, the pattern to follow, and the
StateCellStore the InstanceStore generalizes); and the P2 artifact
(ADLFunctorSurface.artifacts/p2-ingest-inventory-target.md §6 + §12.6 —
the deferred monotone/descent stage that is THIS epoch's early work, its
hole contract + interior-fold witness + emitted-tree↔flow obligation).
Code anchors: include/drlojekyll/Runtime/StateCell.h (the substrate the
InstanceStore generalizes); lib/ControlFlow/Build/DR.{h,cpp}
(BuildGroupUpdateOps DR.cpp:638, DeriveDRStrata DR.cpp:2058,
LinearizeAndValidateDRFlow DR.cpp:2963, ValidateDROps DR.cpp:2323 with
the LIVE E-25 census gap at DR.cpp:2761; the kIngestFold field block
DR.h:502, the GROUP_UPDATE field block DR.h:523); the Lower* family in
Stratum.cpp (LowerGroupUpdate Stratum.cpp:1363, LowerIngestFold
Stratum.cpp:1884, the dispatch in LowerDRFlow Stratum.cpp:1436/:1596);
lib/CodeGen/CPlusPlus/Database.cpp (EmitGroupUpdate Database.cpp:1970,
EmitStateCellStructs Database.cpp:1076, the header assembly Database.cpp:
2967); and lib/DataFlow (Query.h node classes + the Stratify pass) for
the P3 demand transform if scoped in.
Method: the checkpoint method — re-verify §14.0 AND §14.2/§14.3 against
HEAD before building (§14.2/§14.3 are the SINGLE-PASS SEED, never fleet-
reviewed; the E-1..E-26 precedent across SIX epochs says it WILL find a
defect — continue the numbering at E-27, report loudly). Do P0 (the
GROUP_UPDATE census gap) FIRST — it is the smallest real diff, closes a
carried gap, and is the template the new family's census copies.
Design critique (minimum: is a demand frontier a hazarded write or a
frozen read — the C-0b question one level up; does the demand transform
preserve stratification; the config-column runtime-key vs compile-time-
constant fork). Hand-write the target IR for ONE real config-column
aggregate AND ONE real subgraph case before generalizing. For any
emission-shape change (P1's descent reparenting), decide the golden
policy with the owner FIRST (permutation-only bless with the derivation-
counter oracle + monotone projection referees, per §7). Gates: SUITE
PASS (164 baseline, growing with new oracle-refereed corpus) with the §7
golden policy; ctest 3/3; the DR-IR always-on validators compile-time
green (now INCLUDING the GROUP_UPDATE census P0 adds); bench neutral at
the flagship points for any runtime-touching diff; landing record
appended to PerfRoadmap §15 with deviations for ratification.
Environment gotchas carried (per §6.4/§8.4/§10.6/§12.4): export
PATH="/Users/pag/Code/.brew/bin:$PATH"; macOS bash 3.2 (no `declare -A`);
zsh does NOT word-split $flags (use `${=var}` or arg arrays); NEVER
rebuild the compiler mid-suite; NEVER time bench runs concurrently with
suite runs; generated-code TEXT is not run-stable (pointer-keyed
container orders — key comparisons on case+mode+knobs semantics, never
generated-text hashes).

## 15. Landing record (2026-07-16) — SUBGRAPHS/DEMAND EPOCH CLOSED

Branch `subgraphs-demand` off main 0a4a9225. The epoch's working ledger
is docs/proposals/SubgraphsDemand.md (§0 baseline, §1 the seed
re-verification errata E-27..E-34, §2 the per-diff implementation
records, §3 the judged designs, §3.1 the owner decisions); the binding
artifacts are SubgraphsDemand.artifacts/ (p1-cf16-hole-target.md,
p2c-config-agg-target.md — both AMENDED post-judge; p2b-instance-
target.md, p3-demand-argument.md — both carried as next-epoch design
records). Commits: 88585bec (ledger §0-§1, the fleet record), 59113fa6
(P0 LANDED), 8f0b357a (§3 + the four design artifacts, judged),
cc4cfa2d (§3.1 owner decisions), efc8b8c0 (P1/P2c artifacts amended),
14790a28 (P1/§6 LANDED), 9d14f57e (P2c LANDED), plus this close commit.

Method executed (the checkpoint method): fleet re-verification of the
§14.2/§14.3 seed BEFORE building (5 opus derivation lanes + 5
adversarial verifiers, ~848k tokens, 96 claims — the precedent held a
SEVENTH time, and the verify pass also caught one of its own lanes'
errors); P0 landed first with the seed's OWN census sketch corrected
(E-27/E-28) and fire-tested; four designs each written WITH a
hand-written binding artifact and adversarially judged (ALL FOUR
REVISE — the judges materially re-shaped the epoch cut); owner
decisions taken BEFORE emission-changing code (§3.1: the cut, the §6
golden policy, the P3 re-charter); artifacts AMENDED per their judges
before implementation; a Fable review before EACH emission commit
(P1: APPROVE-WITH-NITS ×4 landed in-commit incl. the MEDIUM R-KLASS
payload-consumption fix; P2c: APPROVE-WITH-NITS ×3 in-commit, plus the
two-thresholds-one-sensor semantics empirically confirmed). EVERY
review round found something real: the P0 seed sketch was a tautology
that could never fire; the P1 judge proved the drafted hole validator
could never fire; the P2c judge caught a won't-compile prose
instruction and the @recompute config-routing contradiction; the P2b
judge holed both concrete deliverables; the P3 judge found the
CRITICAL premise hole (no demand seed exists in the eager/push model).

### Seed errata found by the pre-code re-verification (full text ledger §1)

- E-27 (REAL-DEFECT): the P0 sketch's `exp_state_seal = count of
  statecells` is a flow-derived tautology; both recounts must derive
  from the QUERY (|Aggregates()| + |KVIndices()|, provably tight).
- E-28 (REAL-DEFECT): statecell_id is a mint-order artifact — never a
  census key; the sound key is (agg_table, provenance, algebra, view).
- E-29: "the plain assert DR.cpp:629" is a blank line; the real asserts
  are :663-665 (V-AGG-SOLE) + :718 (acyclic fence).
- E-30: the census expect() list is NINE kinds at :2810-2818, not five.
- E-31 (REAL-DEFECT): P2(c) under-scoped — the config KEY projection is
  threaded end-to-end but the REDUCTION ABI never receives config; the
  corpus case must exercise a config-DEPENDENT reduction.
- E-32 (REAL-DEFECT): group_ids CANNOT keep demand copies distinct
  (empty-set ⇒ mergeable); a demand transform needs a STRUCTURAL input
  edge (the ⊥c-pivot precedent).
- E-33: P3 obligations extended — (e) zero-pivot-@product, (f) demand
  relations need real sources, + the Stratify induction cross-check.
- E-34: the §6 hole diff is not a bare signature change (assert
  relaxation + monotone routing move required).

### What landed

P0 — THE GROUP_UPDATE/STATE_SEAL CENSUS (E-25 charter DISCHARGED):
query-derived recount + order-free key multiset + the promoted
always-on V-AGG-EFFECT / V-AGG-SOLE / V-AGG-PAIR structural validators;
fire-tested both ways; observation-only, byte-identical. The substrate
this epoch extends is no longer un-censused, and P0 is the census
template the future instance family copies.

P1 — THE §6 MONOTONE/DESCENT STAGE: every ingest fold now lowers from
the DR-IR (MakeMonotoneIngestFold the single monotone authority;
LowerIngestFold returns the UPDATECOUNT cursor whose empty body the
hand-coded descent fills — the hole contract); the INGEST-CURSOR-SHAPE
always-on check; V-INGEST-XCHECK Site 5 (order-free 6-tuple incl. the
R-KLASS consumed payload class) puts the eager ingest web in the
cross-checked model — the P2-cutover deviation's obligation DISCHARGED,
the F17/F18 bug-class kill extended to the last fold surface. The
remaining hand-coded emission surface is the eager DESCENT itself.
BYTE-IDENTITY 20/20 under the ratified §12.4-precedent structural gate.

P2(c) — CONFIG-COLUMN AGGREGATES (@invertible arm): the CLAUDE.md gap
closed for the natural case — config-DEPENDENT @invertible reductions
lower end-to-end (StateCell raw-pack Fold, config-leading Combine/
Uncombine, the kHasConfig discriminator with DebugValidate kept alive
via identity-config probes, num_config threaded to the emitters, the
algebra-forked fold arm, a config-aware oracle). config_agg_1 is the
oracle-refereed witness; SUITE 164 → 165.

P2(b) — PAPER-ONLY (owner decision §3.1): the keyed-instance SUBSTRATE
design is ratified as the standing record (effect sub-domain totality,
the C-0b-one-level-up answer = ACYCLIC-FROZEN-FIRST under the
ViewSelfReachable fence, scheduler pricing at all seven plug-in
points); its two concrete deliverables are next-epoch obligations (the
InstanceStore sealed-word redesign around the nested table's OWN kInI
watermark — a store-held row-id watermark breaks non-aliasing under
CompactDead; the two-table double-count seam; a demand-WIRED witness).

P3 — DESIGN-ONLY, RE-CHARTERED (owner decision §3.1): the judge's
CRITICAL stands — a pure Query-IR magic-sets rewrite has NO demand-seed
injection point in this eager/push engine (a bound #query is a
read-time index probe over an already-materialized table; d_p is
source-less and dead-flow collapses). The re-charter question is the
SEED MECHANISM (generalize the forcing-message surface), plus
per-adornment splits and the all-free-forces-full interaction; the
amended invariant argument (E-32/E-33 + judge) is the standing gate;
any live transform is mode-gated off (37-38/164 corpus cases carry
bound queries).

### THE NUMBERS (bench/BASELINE.md run 10)

Byte-identity epoch with one suite-growing diff: P0/P1 byte-identical
(20/20 structural-gate comparisons per run); P2c config-free path
byte-identical, bench NEUTRAL BY BYTE-IDENTITY (no flagship uses
aggregates), counter-seam no-op re-verified after the one Runtime
header edit; suite 165 (the 164 zero churn all epoch; config_agg_1
blessed from oracle truth); ctest 3/3 throughout; Q5 spots flat all
epoch (release/opt 0.11-0.13s, debug/opt 0.93-0.95s @128 vs the run-9
0.11-0.12/0.87-0.95 baselines).

### Deviations for ratification (SEED TO NEXT EPOCH)

RATIFIED 2026-07-16 (owner, at close — all four as recorded below):
the P2c residual config-@recompute fence, the config_agg_2 follow-on
carry, the landed Site-5 coverage/payload scope (tree shape via the
structural golden gate), and the FINDINGS.md no-entry disposition.
Nothing re-opens; the §16 seed carries them as settled.

1. P2c RESIDUAL FENCE: config-column aggregates whose functor is not
   @invertible (incl. the over() undeclared default → @recompute) are
   rejected with a clean diagnostic — lifting the fence unconditionally
   would have regressed config @recompute from clean-diagnostic to
   uncompilable generated code. Ratify the landed fence.
2. config_agg_2 (@recompute config) is a FOLLOW-ON: the runtime/codegen
   ABI is present but the emit-arm wiring (Emit/Seal config forwarding;
   the store's bulk Seal() needs a caller-supplied or key-derived
   config) is not. Carried to §16.
3. P1 SITE-5 SCOPE: the seed promised an "emitted-tree↔flow
   cross-check"; the landed Site 5 is a fold-op COVERAGE + PAYLOAD
   multiset check, with tree SHAPE delegated (by judged design) to the
   ratified -ir-out structural golden gate. Ratify the landed scope.
4. FINDINGS.md UNCHANGED: nothing escaped to a golden; every catch
   (the P0 tautology, the never-fires validator, the won't-compile
   instruction, the P3 premise hole) was design/judge-time or
   fire-test-time, pre-commit — recorded in the ledger per convention.

EPOCH CLOSED. E-numbering continues at E-35. Next epoch per §16.

## 16. Epoch-start addendum (2026-07-16, at the subgraphs/demand
## close): the NEXT EPOCH SEED — DEMAND SEEDS + KEYED INSTANCES
## (SINGLE-PASS SEED by the closing session — to-be-re-verified per the
## house precedent: E-1..E-34 across SEVEN epochs, EVERY pre-code
## re-verification has found a real seed defect. Anchors are close-time
## line numbers on branch subgraphs-demand; re-derive every one.
## Continue the E-numbering at E-35.)

### 16.0 Why an epoch here

The subgraphs/demand epoch landed the SUBSTRATE half of its charter
(P0 census, §6 ingest completion, config-column aggregates) and
converted the SPECULATIVE half into two precisely-holed design records:
p2b-instance-target.md (substrate ratified; store + witness holed) and
p3-demand-argument.md (invariant argument amended; premise holed at the
seed). The next epoch's charter is to CLOSE THOSE TWO HOLES TOGETHER —
they are one problem: a keyed nested instance needs a demand frontier
as input, and a demand frontier needs an injection point the eager/push
model does not currently give a Query-IR rewrite. The judge's finding
is the design brief: the FORCING-MESSAGE surface (lib/ControlFlow/
Build/Build.cpp:246-290 at close — a #message-forced query that
re-enters the eager web) is the one query-driven re-injection mechanism
in the engine; generalizing it into a compiler-SYNTHESIZED demand
message is the candidate seed mechanism that keeps SLDMagic a
transformation, never an evaluator (the rewrite emits demand-relation
INSERTs fed by a synthesized forcing path; the runtime bound argument
becomes an ordinary message batch at query-call time).

### 16.1 Carried residue (each with its home)

  - config_agg_2: the @recompute config emit-arm wiring (Emit/Seal
    config forwarding — decide caller-supplied vs key-derived; the
    residual fence at lib/ControlFlow/Build/Build.cpp points at it).
  - InstanceStore REDESIGN (p2b judge HIGH): the sealed side must ride
    the nested DiffTable's OWN kInI watermark (Table.h Seal/sealed) —
    a store-held row-id watermark breaks non-aliasing under CompactDead
    renumbering; Emit/Old must be type-symmetric (both set-valued).
  - The TWO-TABLE DOUBLE-COUNT seam (p2b judge MEDIUM): nested-table
    commit sweep vs outer publish netting — specify before any emission.
  - The DEMAND-WIRED WITNESS (p2b judge HIGH): the #subgraph surface
    (or bound-#query lowering) must wire demand→instance with a REAL
    column edge (E-32) or the witness is dead-flow-collapsed inertness.
  - PER-ADORNMENT demand splits + the all-free-forces-full interaction
    (p3 judge CRITICAL-2/HIGH): transitive_closure.dr needs THREE
    adornments of tc; an all-free consumer makes sibling demand guards
    prune nothing — specify both before code.
  - E-32/E-33 amended P3 obligations (a)-(f): the standing gate; the
    d_p source-projection CSE question (empty group_ids) needs its own
    discharge, not just p'.
  - P4 residue carried: shared-input drain fusion (first witness still
    the interior-fold CSE question — see the P1 artifact's §4 record);
    uses_functors dead-arm excision (WASM-gated); sorted-multiset
    MIN/MAX; algebra class (II) mergeable-sketch lowering; KVINDEX
    dataflow-node deletion; aggregates over MONOTONE inputs (§C-4);
    func.Name()-vs-Sanitize; kSectionWalk pivot-compare elision
    (MEASURE-FIRST); R4 re-motivation bar (WCOJ 3+-way witness);
    StateCell/instance dead-group compaction (MEASURE-FIRST).

### 16.2 The surface the next epoch extends, as pseudocode (close-time
### anchors; re-verify)

  THE FORCING-MESSAGE PATH (the demand-seed candidate): a #query with a
  forcing #message lowers to a receive that APPENDS the bound tuple to
  the query relation's input path and re-runs the eager web + phases
  (lib/ControlFlow/Build/Build.cpp:246-290; the forced-query entry
  point takes (db, log, functors) — CLAUDE.md generated-API note).
  A bound #query WITHOUT a forcer is a pure read-time probe:
  DataTable + GetOrCreateIndex → scanned_index (Build.cpp:394-408).
  THE GENERALIZATION SKETCH: the demand transform synthesizes, per
  demanded adornment p^α, (i) a demand relation d_p^α with a REAL
  producing source (the demanding subgoals' projections + the query's
  own synthesized forcing message as the ROOT seed), (ii) the guarded
  copy p' = p ⋈ d_p^α (the ⊥c-pivot structural shape, E-32), and
  (iii) a forced-query entry point that BATCHES the runtime bound
  argument into the synthesized demand message — the seed enters as an
  ordinary message, the engine stays push-only, and cursors invalidate
  per the existing entry-point contract. Stratification, the (a)-(f)
  obligations, and mode-gating per p3-demand-argument.md as amended.

  THE INSTANCE FAMILY plugs into the seven §14.2(C) points exactly as
  ratified in p2b-instance-target.md §6 (anchors re-verified by its
  judge), with the store redesigned per §16.1 and the census copying
  the P0 template (query-derived recount, no mint-order keys).

### 16.3 The path as diffs (owner re-ranks at epoch start)

  D0  Re-verify this seed (the house precedent; E-35+).
  D1  THE SEED MECHANISM design + judge round: generalize the forcing
      path into synthesized demand messages; per-adornment splits; the
      all-free interaction; the (a)-(f) argument re-reviewed against
      the CONCRETE mechanism. NO CODE until it survives its judge.
  D2  config_agg_2 (@recompute config emit-arm) — small, independent,
      closes the P2c fence.
  D3  InstanceStore redesign on paper → the demand-wired witness IR
      hand-written → SUBGRAPH_INSTANTIATE emission, acyclic-frozen
      first (the ratified C-0b answer), census from day one.
  D4  The demand transform behind an off-by-default mode, on the D1
      mechanism, measure-first (the COST demand-benefit witness).

### 16.4 Bootstrap (next session)

Branch off main once subgraphs-demand merges. Read IN ORDER:
docs/proposals/SubgraphsDemand.md END TO END (this epoch's ledger —
errata E-27..E-34, the four judge records, §3.1 decisions, the three
landing records); THIS file's §15; the four SubgraphsDemand.artifacts/
(p1/p2c as landed-and-amended precedent, p2b/p3 as the holed design
records THIS epoch's charter closes); AggregatingFunctors.md §4;
v3-spec.md §2 + v3-spec-statecell.md §0-§2 (the extension pattern).
Code anchors: lib/ControlFlow/Build/Build.cpp:246-290 (forcing path)
+ :394-408 (bound-query probe) + the config fence; StateCell.h (the
config ABI + the Seal() config question); DR.cpp BuildGroupUpdateOps +
the P0 census (the template); Stratum.cpp LowerIngestFold (the cursor/
hole contract P1 landed); tests/OptDiff/cases/config_agg_1.* (the
corpus mold). Method: the checkpoint method — fleet re-verify §16.2/
§16.3 before building (E-35+, report loudly); hand-write the demand-
wired witness IR before generalizing; owner decisions before
emission-changing code; Fable review before each emission commit;
gates per the standing policy (suite 165 growing oracle-refereed,
byte-identity-with-structural-gate for shape-only diffs, permcheck
fallback never to green red, counter-seam no-op after any Runtime
header edit, bench neutral at the flagship points, ctest 3/3, landing
record → §17). Environment gotchas carried: export
PATH="/Users/pag/Code/.brew/bin:$PATH"; macOS bash 3.2 (no declare
-A); zsh ${=var}; never rebuild mid-suite; never time bench
concurrently; push to git@github.com:pgoodman/DrLojekyll.git (the
https origin tracking ref goes stale).
