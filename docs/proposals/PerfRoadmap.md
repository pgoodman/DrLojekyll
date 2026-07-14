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
