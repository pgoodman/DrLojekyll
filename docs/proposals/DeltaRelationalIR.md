# Delta-relational IR — epoch design ledger

Status: EPOCH IN PROGRESS, opened 2026-07-14 on branch `delta-relational-ir`
off main 9e1a092. This file is the epoch's working ledger (the PerfRoadmap
§10 seed's re-verification record, the R0 profile artifact, the verified
pipeline pseudocode, and — as they land — the diff plan and hand-written
target-IR artifacts). The landing record goes to PerfRoadmap §11 at close.

## 1. R0 — the profile (this epoch's first artifact; gates all design)

Method: regenerated the runbench progsize chain (`r_i(X,Y) : r_{i-1}(X,Z),
r0(Z,Y).`, all differential) at 2–128 rules; timed `drlojekyll -cpp-out`
for BOTH builds × all 4 modes (3 reps, medians); `sample`-profiled debug
and release at 128 rules, opt and nodf. Raw data + profiles in the session
scratchpad (`r0/curve.tsv`, `r0/prof/*.sample.txt`).

### 1.1 The curve (median dr_wall, ms)

    rules  dbg/opt  dbg/nodf  dbg/nocf  dbg/none  rel/opt  rel/nodf  rel/nocf  rel/none
       2      43.3      44.9      44.5      43.8     25.3      26.1      25.5      25.2
       8      52.4      59.5      52.4      59.2     26.6      27.8      26.7      27.6
      32     189.5     254.6     179.4     237.6     43.8      50.2      40.5      48.1
      64    1014.9    1162.1     989.6    1097.3    143.2     162.4     141.3     157.3
     128    7832.5    7404.9    7594.9    7132.2   1077.0     892.1    1013.9     850.2

- Reproduces Q5 (recorded 7.81s@128 debug ≈ measured 7.83s).
- SUPERLINEARITY IS NOT A DEBUG ARTIFACT: release grows 26ms→1.08s over
  2→128 rules; 64→128 is ~7.5x per 2x rules in both builds — effective
  exponent ≈ n^3 at the tail. Debug/release ≈ 7.3x at 128 (the known ~8x).
- THE MODE ABLATION IS FLAT: `none` ≈ `opt` at every size (none is
  slightly FASTER at 128). Q5 suspect pools #2 (CF scheduling fixpoint)
  and #3 (ProgramImpl::Optimize) are exonerated ON THIS SHAPE — the cost
  is on the unconditional path, and it MOVES between passes by mode.

### 1.2 Attribution (sampling profiles, 128 rules)

opt mode (debug 6545 samples / release 885 — same shape):
- ~72–80% — CSE structural equality: `QueryImpl::Optimize` → `CSE`
  (lib/DataFlow/Optimize.cpp:69) → `Query*Impl::Equals`/`ColumnsEq`
  recursion. Split across the do_cse call sites as ~60% + ~10% + ~10%.
- ~5–6% — `QueryImpl::RunBackwardsTaintAnalysis` (lib/DataFlow/Taint.cpp).
- Program::Build ≈ 3%, ProgramImpl::Optimize ≈ 2%, codegen noise-level.

nodf mode (release 696 samples):
- ~92% — `RunBackwardsTaintAnalysis`, dominated by
  `unordered_set::__emplace_unique_key_args` inside the per-column taint
  unions (Taint.cpp:135+). Without CSE the graph is bigger and the taint
  fixpoint explodes — this is why the ablation looked flat.

### 1.3 Mechanisms (verified in code)

- CSE (Optimize.cpp:69–164): buckets by SHALLOW `HashInit()` (kind +
  deletion flags + column count only), then PAIRWISE `Equals` within a
  bucket with `eq.Clear()` per pair — no memoization across pairs, no
  deep-hash refinement. The progsize chain puts all r_i join/tuple views
  in one bucket → O(n²) pairs × O(n)-deep recursive Equals ⇒ O(n³),
  and NO merge ever succeeds (all r_i are distinct) — the entire cost
  discovers nothing. `do_cse` additionally loops CSE up to views.size()
  times (Optimize.cpp:538), and `RelabelGroupIDs` (its own fixpoint)
  re-runs after every successful merge.
- Taint (Taint.cpp:135+, both directions): a fixpoint unioning per-column
  `unordered_set<COL*>` taint sets; in a chain each set is O(n), unions
  O(n²) ⇒ O(n³) with hash constants. DEAD CODE ON THE TIMED PATH: the
  only consumers of `ForwardsColumnTaints`/`BackwardsColumnTaints`/
  `*TaintIds` anywhere in the tree are two COMMENTED-OUT debug lines in
  lib/DataFlow/Format.cpp:76-77. `BuildEquivalenceSets` does not read
  taints. Nothing in lib/ControlFlow, lib/CodeGen, bin, or tests does.
- Canonicalize's max_iters = max(2N, N²) cap (Optimize.cpp:404) and the
  max_depth outer loop (Optimize.cpp:581) are real superlinear BOUNDS but
  did not dominate the samples at this shape (the 8-slot hash-history
  break truncates them).

### 1.4 What R0 changes about the epoch plan

- The Q5 fix is DATAFLOW-side, not the emission web: (a) delete or
  lazy-gate the two dead taint passes; (b) fix CSE's candidate filter
  (deep structural hash / memoized Equals) so equality work is
  proportional to actual merges. Both are small, semantics-neutral,
  golden-neutral diffs measurable on the Q5 curve — they become this
  epoch's first implementation diffs (R0.5a/R0.5b in the re-ranked plan).
- The DR-IR (R1/R2) is therefore justified by its OTHER two forcing
  functions — aggregates need delta semantics the §5.1 schemas cannot
  express (AggregatingFunctors §4), and the F17/F18/F22 bug class lives
  in the hand-coded emission web — NOT by Q5. The emission web is ~3% of
  compile time. This re-ranking goes to the owner with the diff plan.
- LATENT CF hazards recorded for shapes the chain doesn't exercise
  (from the fleet re-derivation, unmeasured): DiscoverBranches
  (Stratum.cpp:352) is an un-memoized path-enumeration DFS — worst-case
  exponential on reconvergent table-less plumbing; the scheduling
  fixpoint (Stratum.cpp:1732) is O(units × lift-depth). Neither is hot
  on any measured shape; both belong to the R1 rebuild's design inputs.

## 2. The pipeline as VERIFIED pseudocode (supersedes PerfRoadmap §10.4;
## fleet-re-derived from code 2026-07-14, 4 agents, all claims cited)

    PIPELINE (bin/drlojekyll/Main.cpp):
      parse -> ParsedModule
      [debug builds only, Main.cpp:128-161: parser round-trip x2 +
       assert; linear in text, pre-CompileModule, not a Q5 term]
      Query::Build(module, log, optimize)        « lib/DataFlow/Build.cpp:2517 »
        BuildClause per clause (:2530)           « SELECT/TUPLE/CMP/MAP/JOIN/
          MERGE/NEGATE/INSERT; zero-arity predicates desugar to unit
          relations — positive use = ONE-PIVOT equi-JOIN against the unit
          SELECT (:1450), negative = NEGATE on the token »
        RemoveUnusedViews; ClearGroupIDs; TrackDifferentialUpdates (:2550-52)
        Simplify (:2557, UNCONDITIONAL — CSE over SELECTs + JOIN
          canonicalization, Optimize.cpp:335)
        ConnectInsertsToSelects (:2562)
        if optimize: QueryImpl::Optimize (:2568, Optimize.cpp:530):
          do_cse -> Canonicalize(conservative bottom-up) -> do_cse ->
          loop (≤ max INSERT depth): Canonicalize(aggressive top-down)
            + EliminateDeadFlows -> final do_cse
          « do_sink is a NO-OP (body commented out); Canonicalize caps at
            max(2N, N²) passes with an 8-slot hash-history break; CSE =
            shallow HashInit buckets + pairwise Equals + per-merge
            RelabelGroupIDs — THE MEASURED Q5 HOT PASS (§1) »
        ConvertConstantInputsToTuples; ProxyInsertsWithTuples; LinkViews;
        IdentifyInductions; FinalizeDepths; FinalizeColumnIDs;
        TrackDifferentialUpdates(final); TrackConstAfterInit;
        RunBackwards/ForwardsTaintAnalysis « DEAD on the timed path, §1.3 »
        BuildEquivalenceSets (:2611)
        Stratify (:2612) — LAST, unconditional, O(V+E) Tarjan; rejects
          unstratified negation in all 4 modes
      Program::Build(query, log, first_id, optimize)
                                        « lib/ControlFlow/Build/Build.cpp:1015 »
        pre-pass diagnostics (:1024-1050): AGG / KVINDEX / impure MAP /
          on-cycle differential @product (ViewSelfReachable fence, :179)
        BuildDataModel (:1062) + FillDataModel (:1096): views ->
          equivalence-class models -> one TABLE per model; indices from
          join/negate key columns
        BuildEntryProcedure (:1104, Procedure.cpp:655) — and NESTED
          INSIDE IT, after CompleteProcedure: BuildStratumPhases
          (Procedure.cpp:769). BuildIOProcedure per message runs AFTER.
        BuildStratumPhases (Stratum.cpp:1456-2328, ~872 lines; ONE
          function, three interleaved stages sharing mutable state
          {branches, joins, crossovers, products, drain_stratum,
          recursive_sccs, context.table_delta_vecs}):
          DISCOVERY (:1461-1671):
            ComputeRecursiveSCCs (:210) — per anchor group fwd-reach ∩
              bwd-reach; RuleClass (:291) kRecursive iff target and some
              read table share an SCC (fixed per rule, not per site)
            DiscoverBranches (:352) from every member view of every
              source table — SIGN-AGNOSTIC DFS (sign is chosen at
              emission); no visited set (path enumeration); terminal =
              first pivot-join or table boundary
            JoinEmission dedup by join view (:1541-57): ONE kJoinPivots
              vec shared by all feeding chains and signs; all-sides-
              same-SCC joins keep their JoinEmission but suppress seeds
              (round-0 carried by the claim-round fire)
            crossovers (:1577-1622): ONE arm-pair per non-@never negate
              (F19 assert at :1595, NDEBUG); products (:1631-55): one
              ProductEmission per acyclic differential @product
          SCHEDULING FIXPOINT (:1732-1828): monotone integer lift of
            stratum/drain_stratum over all emission units + SCC pinning;
            ORDER IS NOT EMITTED HERE — it only computes strata
          EMISSION (:1936-2327), per stratum ascending:
            branch/join seeds (skip same-SCC-internal) -> crossovers ->
            product arms -> acyclic tables {claim drains, frontier
            filters} -> per-SCC {claim-round loop(del) = OVERDELETE
            (EmitJoinFire claim-relative matrix + in-fixpoint seed
            chains + retire), EmitRederive, claim-round loop(add) =
            INSERT, then BOTH frontier filters in add-loop output —
            the deferral is correctness-load-bearing (a re-added row's
            kAdd must be final before the del filter runs)}
            « for SCC tables the claim drains ARE the OVERDELETE/INSERT
              loops, not a separate earlier band »
          commit sweeps are emitted DOWNSTREAM of BuildStratumPhases
            (Procedure.cpp, per differential table, table order)
        Induction.cpp: monotone/induction-owned fixpoint loops (input/
          swap/output vecs; deletion-capable induction outputs append to
          kNetAdditions — adds only)
        ProgramImpl::Optimize x2 around ExtractPrimaryProcedure (:1149,
          :1158): region flattening, no-op removal, procedure dedup
      C++ codegen (lib/CodeGen/CPlusPlus/Database.cpp): EmitRegion
        dispatch; D1 row-binding scope stack (emission-time id
        recovery); commit-sweep tail = Commit(sink) + DebugValidateCounts
        + D2 compaction/reindex; monotone Seal()
      runs against include/drlojekyll/Runtime (§8.1 pseudocode as amended
        by the data-structures epoch)

    The CF-IR remains VALUE-KEYED by construction (no row-id/position
    vocabulary; CHECKMEMBER carries a MembershipPredicate over column
    values) — confirmed; the D1 elision is emission-time only.

## 3. Seed errata found by the pre-code re-verification (the E-1..E-11
## precedent held again; fleet of 4 opus agents, 2026-07-14)

- E-12 (the real defect): include/drlojekyll/ControlFlow/Program.h:621's
  enum comment says kNetAdded = `kAdd && !kDel`. The runtime implements
  `kAdd && !kDel && !kInI` (Runtime/Table.h:429-433, with the documented
  reachable double-count if the `!kInI` conjunct is dropped: a same-batch
  −e1/+e2 whose del is gate-dropped leaves kAdd set on an already-present
  row). An R1 vocabulary derived from the IR header's comments would
  encode the wrong frontier predicate. Fix the comment in R1; derive
  predicate semantics from Table.h only.
- E-13: StackSafeNegation §5.1's seed table implies negated atoms get
  per-position InNew (j<i) / InI (j>i) reads. The emitter encodes ONE
  CONTEXT-KEYED read for the whole seed chain (Stratum.cpp:519-521:
  in_fixpoint ? kInNew : kInI), never position-keyed for negated seed
  atoms. The R1 vocabulary must model the negate-seed read as
  context-keyed (this is also exactly the F18 shape).
- E-14: the membership-predicate vocabulary is TEN, not §5.1's eight:
  kNetDeleted (kDel && !kAdd) and kNetAdded (kAdd && !kDel && !kInI) are
  first-class predicates used by the frontier filters (Stratum.cpp:
  1032-33) and crossover/product frontier vectors.
- E-15: §10.4's dataflow order is wrong: Stratify runs LAST in
  Query::Build (Build.cpp:2612), after Optimize — not adjacent to clause
  lowering. Simplify (:2557) is a separate UNCONDITIONAL pre-pass, and
  Optimize is interleaved (CSE-first, dead-flow inside the bounded
  loop), not the seed's linear 4-stage pipeline.
- E-16: BuildStratumPhases is NESTED inside BuildEntryProcedure
  (Procedure.cpp:769), not a Build.cpp sibling; it is ~872 lines of the
  2330-line file; and the DISCOVERY/EMISSION halves are NOT separable as
  written — one function body with the shared state listed in §2. The R1
  cut must externalize exactly that state as the DR-IR object.
- E-17: DiscoverBranches is sign-agnostic (both-signs vs adds-only is
  decided at emission, and induction-owned/monotone sources are
  adds-only because only their kNetAdditions vec is ever filled). The
  seed's intra-stratum order also conflated the scheduling fixpoint
  (which only lifts integers) with the emitted order, and missed that
  SCC-table frontier filters are deferred into the add-loop output for
  correctness.

## 4. The predicate matrix (enumerated from the emission code; the R1
## vocabulary's coverage spec — condensed; full cites in the fleet report)

    SEED context (in_fixpoint=false):
      join lower side                 -> kInNew          (Stratum.cpp:1193)
      chain NEGATE forward gate       -> kInI, BOTH signs (:521; F18)
      @never NEGATE forward gate      -> kPresent        (Negate.cpp:93)
    FIXPOINT context (claim round; delta side is the loop):
      same-SCC join side j < p        -> OVERDELETE kSurvivesSoFar /
                                         INSERT kInNewWithFrontier (≡InNew)
      same-SCC join side j > p        -> OVERDELETE kAliveAtClaim /
                                         INSERT kInNewSansFrontier
      lower join side                 -> kInNew
      chain NEGATE forward gate       -> kInNew (refire context)
    CROSSOVER (one arm-pair per non-@never negate; seed-before-drain):
      − arm over negated kNetAdditions (always); + arm over kNetRemovals
      (iff negated table differential); pred-row read kInNew both arms;
      fold ± RuleClass(negate_table, {pred, negated}) into negate's table
    PRODUCT arms (side i × sign; monotone side has no − arm):
      other side j < i -> kInNew; j > i -> kInI (sign-INDEPENDENT);
      full scans; fold kNonRecursive (asserted); seed-before-drain
    CLAIM gates (dequeue re-test, F17): TryClaimDel proceeds iff
      C_nr ≤ 0; TryClaimAdd proceeds iff total > 0
    REDERIVE: loop kOverdeleteSet, gate kRecursivelySupported (C_r > 0),
      append survivors to kAddQueue
    FRONTIER FILTERS: D_s gate kNetDeleted -> kNetRemovals; A_s gate
      kNetAdded -> kNetAdditions; SCC tables: both filters AFTER the add
      loop quiesces
    COMMIT SWEEP: Commit(sink) publishes was != now; then debug validate;
      then compaction + emitted reindex; monotone tables Seal() only

    Verdict: the code's matrix is internally consistent with F17/F18/F22
    and CLAUDE.md's differential invariants. The deltas against the DOCS
    are E-12/E-13/E-14 above.

## 5. The path as diffs (re-ranked by R0; predictions pre-registered
## BEFORE measurement; critique pass recorded in §6)

    RQ5a  DELETE THE DEAD TAINT PASSES from the timed path.
          Build.cpp:2592-2593 (RunBackwards/ForwardsTaintAnalysis) have
          zero consumers (§1.3). Remove the calls (keep the code +
          public API or delete both — owner's call at review; default:
          delete calls AND the passes; Format.cpp's commented consumers
          note how to regenerate).
          PREDICTIONS: Q5 release@128 nodf −85..95% (892→~60-120ms),
          opt −4..8%; debug@128 similar fractions; zero golden churn
          (analysis-only pass, results unread); zero runtime effect;
          suite PASS 155 untouched.
    RQ5b  CSE CANDIDATE FILTER: replace the shallow HashInit bucket +
          pairwise-Equals-of-everything with a memoized cycle-safe deep
          structural hash (refine buckets; only Equals within deep-hash
          buckets), or equivalently cross-pair memoization. MUST
          preserve: the group_ids guard (InsertSetsOverlap stays inside
          Equals — untouched), keep-last-edge, unit-SELECT non-folding,
          and CSE outcome (same merge set; merge ORDER may shift —
          goldens compare case STDOUT, not generated text, so this is
          suite-checkable, and keyed drains are sorted by contract).
          PREDICTIONS: Q5 release@128 opt −60..80% (1077→~200-400ms);
          debug@128 opt −60..80%; the curve exponent at the tail drops
          below quadratic; zero golden churn; zero runtime effect.
    R1    DR-IR VOCABULARY + CONSTRUCTION (df -> dr -> cf -> c++).
          A small typed IR between Query and Program. Operators carry
          sign / position / claim-context as ATTRIBUTES; membership
          predicates are the TEN of §4 with semantics derived from
          Runtime/Table.h (E-12), the negate seed read context-keyed
          (E-13). Vocabulary (first cut, to be critiqued):
            ACCESS(table, bound-prefix, pred) — the §10.2 unified
              table-access operator; lowering ∈ {point-test, keyed
              chain walk, positioned seek (future), full scan}
            SEED_FOLD(rule, delta_pos, sign, class) — seed-schema arm:
              frontier loop + ACCESS chain + head fold (UPDATECOUNT)
            FIXPOINT_FIRE(join, delta_pos, sign) — claim-relative
              matrix keyed on static position
            CROSSOVER(negate, sign) / PRODUCT_ARM(product, side, sign)
            CLAIM_DRAIN(table, sign) / RETIRE(table, sign) /
            REDERIVE(table) / FRONTIER_FILTER(table, sign) /
            FIXPOINT_ROUND(scc) / COMMIT_SWEEP(table)
          CONSTRUCTION replaces BuildStratumPhases' discovery half +
          scheduling fixpoint (externalizing exactly the E-16 shared
          state); the schemas become data. The latent CF hazards (§1.4:
          DiscoverBranches path enumeration, scheduling-fixpoint
          re-scans) get memoized/worklist forms in the rebuild.
          PREDICTIONS: Q5 ~neutral (CF build is ~3% of compile; must
          not regress >10% at 128); goldens byte-identical for the
          construction-only step (no emission change yet).
    R2    GENERIC LOWERING dr -> cf, one operator family at a time,
          replacing the Emit* web. Goldens are the semantic net; the
          golden policy (owner decision, §7) governs any emission-shape
          change; the derivation-counter oracle referees every blessed
          change. PREDICTIONS: byte-identical goldens per family EXCEPT
          where a reviewed --bless is recorded; flagship runtime
          neutral (same emitted shapes; any lowering change is its own
          measured decision).
    R3    AGGREGATES as the inaugural operator family (GROUP-UPDATE /
          state-cell per AggregatingFunctors §2/§3); KV index =
          degenerate aggregate; gated on the mutable(...) decision
          (§7). New corpus: average_weight.dr, pairwise_average_weight
          compile+run; oracle grows the per-group recompute referee.
          PREDICTIONS: no existing-golden churn; new goldens authored
          from oracle truth.
    R4    (gated, owner decision) JOIN group_ids reshape for native
          self-joins (IdeasTriage #6) — only with an invariant-
          preservation argument for the CSE guard.

    Ordering rationale: RQ5a/RQ5b first — they are the measured Q5 fix,
    tiny, and de-risk the epoch's flagship metric before the big
    rebuild; R1 construction next (golden-frozen), then R2 family by
    family, then R3. R0's re-ranking demotes "the IR fixes Q5" to "the
    IR must not regress Q5"; the IR's carry is aggregates + the
    F17/F18/F22 bug-class + the unified access operator.

## 6. Hand-written artifacts + adversarial critique (2026-07-14; 3 opus
## writers, 3 opus judges + 1 sonnet mechanical auditor; binding
## resolutions adopted)

Artifacts (committed in DeltaRelationalIR.artifacts/, with the pinned
program.ir identity targets): tc.drir.md (transitive_closure_diff — the
full differential recursive-SCC family), d5_recursive_negate.drir.md
(negate-on-cycle, both crossover arms, REDERIVE, deferred SCC filters),
average_weight.post-r3.drir.md (R3 DESIGN TARGET ONLY — per the coverage
judge, NOT coverage evidence for the R1 vocabulary; its old()/valued
sealed reads and value-fold are R3's separate matrix extension).

### 6.1 Binding resolutions (blockers first)

- B-1 (ACCESS shape): "bound-prefix" is WRONG — every real access binds
  an arbitrary column SET, canonicalized by GetOrCreateIndex's
  SortAndUnique (Data.cpp:349) into index.KeyColumns() order; scattered
  bound sets are live in-corpus (reaching_to keys on the SECOND column).
  ACCESS carries `bound-cols` (canonical column-id set == the index
  identity rule), never prefix contiguity. TABLEPRODUCT staging is OUT
  of ACCESS's scope (it is frontier×scan staging, not membership).
  Section walks carry the pivot-re-test structure explicitly.
- B-2 (RQ5b hash discipline): today's HashInit bucketing is non-lossy
  vs Equals for 7 of 10 node kinds, but Join/Merge/KVIndex Equals do
  NOT check the deletion flags HashInit encodes — a deep hash keyed on
  "exactly what Equals checks" could ADMIT A NEW MERGE today's pass
  misses, changing the merge set. Resolution: the deep hash KEEPS
  can_receive/produce_deletions for all kinds (matches today's merge
  set byte-for-byte); add the regression assert that no unit SELECT is
  ever bucketed with a non-unit view. Merge-ORDER invisibility at
  goldens rests on the keyed-cursor sort contract — recorded as the
  load-bearing premise (review gate on new drivers stands).
- B-3 (silent-breakage cluster): five construction properties enforced
  today only by code structure or NDEBUG asserts become ALWAYS-ON
  DR-IR construction asserts (TigerBeetle-style, positive+negative
  space): (1) exactly ONE crossover arm-pair folds into each non-@never
  negate's table (lift Stratum.cpp:1591 out of #ifndef NDEBUG); (2) no
  minus arm for monotone product sides + every product fold
  kNonRecursive; (3) RETIRE strictly after all same-round fires;
  sort-unique is a PER-VECTOR attribute asserted at placement; (4)
  seed-before-drain as a checkable ordering attribute on CROSSOVER/
  PRODUCT_ARM; (5) member-view identity (no structural dedup of the
  table member list). Plus: the F17 claim gate is MANDATORY DATA on
  CLAIM_DRAIN (C_nr<=0 / total>0 carried and asserted, never a
  template flourish); the F18 negate gate is an ACCESS attribute keyed
  on CONTEXT {eager, seed, fixpoint} × hint {normal, @never}, NEVER
  sign-derived.
- RQ5a depth: delete the passes AND the QueryColumnImpl/QueryImpl
  members AND the Query.cpp accessors (the taint UseLists are STRONG
  Use edges — write-only mutations of the DefUse graph; full removal
  forecloses the phantom-strong-use hazard returning). Format.cpp's
  commented consumers cite the git history instead.

### 6.2 Coverage holes the artifact set exposed (R1 must witness these)

- Off-cycle stratified negate: the seed-context kInI forward gate
  (E-13's cell) is UNEXERCISED by d5rn (whole positive path on-cycle).
  Need a corpus witness + artifact before R2 lowers negates.
- @never negate (kPresent) and the monotone-negated EAGER path: the
  negate has THREE lowering sites today (EmitChainStep fixpoint gate,
  crossover arms, and Negate.cpp:93's eager gate) — §5 named only
  CROSSOVER. Vocabulary v2 adds NEGATE_GATE{context, hint}.
- Differential @product arms: no artifact exercises them (conditions_
  to_bools.dr is the corpus witness to draft against before R2 touches
  products).

### 6.3 Vocabulary v2 (writers' gaps folded in; supersedes §5's list)

    ACCESS(table, bound-cols, pred, context)      « B-1; yields id+row;
      lowerings: point-test | keyed section walk (with pivot re-test) |
      full scan; position/seek reserved for the D5 substrate decision »
    NEGATE_GATE(negate, context∈{eager,seed,fixpoint}, hint)   « 6.2 »
    PIVOT_ASSEMBLE(join, sources...)  « the union of a join's delta
      sources into ONE shared kJoinPivots vec (sort-unique attribute);
      same-SCC seed suppression is an EXPLICIT construction rule here »
    SEED_FOLD / FIXPOINT_FIRE / CHAIN_FOLD        « CHAIN_FOLD = the
      claimed-frontier -> projection/merge-arm -> head folds the tc
      artifact mis-modeled as seeds (G7) »
    CROSSOVER(negate) as an ARM-PAIR object (one per negate, B-3.1)
    PRODUCT_ARM(product, side, sign)              « minus iff side
      differential, B-3.2 »
    CLAIM_DRAIN(table, sign, form∈{single-pass, in-round}, gate=DATA,
      dual-append targets explicit)               « F17; tc G10 »
    RETIRE(band, after=fires)                     « B-3.3 »
    REDERIVE(table) / FRONTIER_FILTER(table, sign, deferral∈{immediate,
      add-loop-output})                           « E-17 contract »
    FIXPOINT_ROUND(scc) as a STRUCTURED REGION (induction shell:
      vector-swap discipline, round vecs, retire bands — tc G4)
    COMMIT_SWEEP(table, flavor∈{diff: commit+validate+compact+reindex,
      monotone: seal}, publish-target?)           « d5rn gap; tc G9:
      table-id order is the identity contract »
    Vectors carry {kind, sort-unique?} as attributes (tc G6); tables
    carry the member-relation LIST (many relations per table, d5rn);
    monotone tables participate in differential arms via watermark
    NetAdded — the guard disjunction is part of SEED_FOLD's shape.

### 6.4 Mechanical audit results

Region-kind + predicate census of both program.ir files vs the traces:
complete except the ^receive:* net-batch procedures (d5rn trace) and
one vector-clear line — both recorded, neither a vocabulary hole (the
ingest band was §5-covered via NET_BATCH). §1.1's table now carries all
4 modes (the RQ5a prediction's 892ms cell is rel/nodf@128). Taint API
grep confirmed: no consumer anywhere beyond the two commented
Format.cpp lines.
