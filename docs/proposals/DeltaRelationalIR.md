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

## 7. Owner decisions (2026-07-14, ratified at the design checkpoint)

1. GOLDEN POLICY (R2): permutation-only bless. R1 construction is a
   byte-identical-stdout hard gate. R2, per operator family: stdout
   diffs are acceptable ONLY as order-permutations of published deltas
   within one epoch, with the derivation-counter oracle AND the
   monotone projection green; then one reviewed `--bless` per family,
   with the diff summary recorded here. Any non-permutation diff is a
   bug — never blessed.
2. mutable(...): option (a) confirmed — kept as sugar desugaring to a
   degenerate aggregate at R3; merge functors must carry declared
   algebra or be rejected; the KVINDEX dataflow node becomes a
   desugaring-deletion candidate.
3. R4 (group_ids reshape): gated in-scope — only after R2 is fully
   green, only with a reviewed invariant-preservation argument for the
   CSE guard; dropped without ceremony (re-seeded for the next epoch)
   if this epoch runs long.
4. R0 re-ranking ratified: RQ5a (FULL taint removal: passes + members +
   accessors) and RQ5b (deep hash KEEPING deletion flags, B-2) land
   before R1.

## 8. Implementation record (one diff at a time; gates per CLAUDE.md +
## §7 golden policy)

### RQ5a — dead taint passes deleted (2026-07-15)

Removed: lib/DataFlow/Taint.cpp (236 lines), the two Query::Build calls,
QueryColumnImpl/QueryImpl taint members, the four public accessors
(include/drlojekyll/DataFlow/Query.h + Query.cpp). Format.cpp's
commented dump lines now cite git history. Net −310 lines. The
write-only STRONG Use edges the passes injected into every column's
use list are gone with them (critique §6.1 RQ5a-depth resolution).
GATES: debug+release builds green; ctest 3/3; SUITE PASS (155), zero
golden churn; no Runtime header touched (seam gate n/a); generated
code semantics unchanged (analysis-only pass, results unread).
Q5 per-diff (medians of 3, vs §1.1; rules=128):
  debug: opt 7832→6766 (−13.6%), nodf 7405→614 (−91.7%),
         none 7132→363 (−94.9%), nocf −11.0%
  release: opt 1077→1061 (−1.5%), nodf 892→100 (−88.8%),
           none 850→81 (−90.4%), nocf noise-band (5-rep recheck
           ~1070ms ≈ +5%, within the ±2-6% sequential band)
Pre-registered predictions (§5): nodf/none −85..95% HIT; opt −4..8%
BRACKETED (release −1.5%, debug −13.6% — the taint share of the
CSE'd-graph opt path was smaller in release, larger in debug than
predicted). The opt-mode curve remains ~n^3 — that is RQ5b's target.

### RQ5b — CSE color-refinement bucketing (2026-07-15)

lib/DataFlow/Optimize.cpp: candidate bucketing replaced — round-0 color =
HashInit (+ Select stream/relation pointer identity, + Insert declaration
id; both REQUIRED-equal by those kinds' Equals, hence conservative), then
position-salted refinement over each kind's exact Equals recursion
(per-kind audit on record: every Equals is POSITIONAL — no set semantics
anywhere), iterated to partition stability (two-round plateau grace;
bound #views+2; the first cut's 16-round cap was the F-3 near-miss: on
chains, distinguishing information travels ONE hop per round, so capped
refinement left ~120-view buckets and moved nothing). Buckets iterate in
first-seen order (pointer-derived colors must not order merges). Pair
loop gains the always-on-shape assert (same columns.Size within bucket).
group_ids stay OUT of colors; InsertSetsOverlap stays inside Equals.

MERGE-TRAJECTORY AUDIT (the F-3 tripwire, mechanized before commit):
old-vs-new bucketing A/B over all 156 corpus programs (155 cases +
progsize@128), instrumented builds (temporary always-on prints, deleted):
30/156 differ in merge-EVENT counts (bucket granularity changes the
fixpoint trajectory — inherent to ANY bucketing change; each merge is
Equals-verified at application time), 0/156 differ in final per-kind
view counts. B-2's "identical merge set" gate is AMENDED (for
ratification) to the measurable equivalent: identical final view counts
on the full corpus + suite byte-identical + oracle green.

GATES: builds green; ctest 3/3; SUITE PASS (155) zero golden churn on
the exact committed code; no Runtime header touched.
Q5 per-diff (medians of 3, vs post-RQ5a; rules=128):
  debug:   opt 6766→970 (−85.7%), nocf −87.5%, nodf +11.1%, none +12.4%
  release: opt 1061→146.5 (−86.2%), nocf −88.7%, nodf +13.7%, none +6.6%
  small sizes (2–8 rules): +9..20% (+3..5ms absolute) — the refinement's
  fixed cost; accepted (suite-wide impact <1s); knob recorded if it ever
  matters: skip refinement below a bucket-size threshold.
  vs R0 EPOCH START @128: opt −87.6% debug / −86.4% release; the
  32→128 tail is now ~LINEAR (3.5x time per 4x rules).
Pre-registered prediction (−60..80% opt@128): EXCEEDED.

## 9. Independent Fable design review (2026-07-15; optimizability mandate
## added by owner mid-review) — verdict + adopted resolutions

Verdict: CONDITIONAL GO. The IR's billing is corrected: aggregates are
acyclic-band (simpler than the hand-landed @product family) and would
NOT alone justify the IR; the carry is (1) the F17/F18/F22 bug class
dies only when the E-16 shared-state web becomes checked data, and
(2) the rewrite substrate (D5 access paths, Stage-6 parallelism,
fusion/DCE). Judged AS a rewrite substrate, vocabulary v2 is the right
operator inventory but the WRONG FACTORING — it encodes ordering as enum
attributes and dataflow as shared named vectors, reproducing one level
up exactly what makes today's web unoptimizable.

Adopted resolutions (binding; vocabulary v3 requirements before R1 code):
- F-1: vectors become TYPED IR VALUES with explicit defs/uses
  ((table, VectorKind) demoted to debug info; element shape
  values|ids|id+cols is an attribute — F-9); every operator declares an
  EFFECT SET over {vector append/drain/clear, table counter±(class),
  flag families read/write, frozen-kInI read, statecell fold/emit/old}.
  Seed-before-drain, retire-after-fires, E-17 deferral, dual-append,
  G-6 fusion, G8 dead filters all become dependence-graph facts; the
  B-3 asserts become graph validators; the scheduling fixpoint's output
  becomes a CHECKED LINEARIZATION of the dependence graph.
- F-7: SEED_FOLD/FIXPOINT_FIRE bodies are explicit nested ACCESS-PLAN
  TREES (matches EmitJoinFire's scan_next nesting at identity; the only
  shape that admits the D5/WCOJ joint-ordering decision later).
- F-2: R2 cuts over ACYCLIC FAMILIES FIRST (claim drains, frontier
  filters, crossovers, product arms, commit sweeps — everything R3
  needs), fixpoint families last; each cutover deletes its replaced
  Emit* in the same diff. R3 gates only on the acyclic families.
- F-4: byte-identical program.ir pinning for family #1 ONLY; thereafter
  the §7.1 stdout/permutation/oracle gate. IR-vs-lowering sort recorded:
  keep in IR = everything the §5.1 algebra forces (sign, position,
  claim-context, ten predicates, dual-append, retire/deferral
  dependence, seed suppression DERIVED from the schema + asserted);
  demote to lowering = guard-nesting templates (G11), sort-unique
  placement (G6), commit order (G9), dead-op emission (G8).
- F-5: §4's matrix is corrected here: the EAGER negate forward gate
  (Negate.cpp:91-94) is `HasNeverHint() ? kPresent : kInI` — a THIRD
  context distinct from the seed chain and the fixpoint refire; the
  @never/kPresent row belongs to the EAGER context, and the eager-normal
  (kInI) cell was missing. NEGATE_GATE{context∈{eager,seed,fixpoint},
  hint} covers the space; constructors derive predicate semantics from
  Table.h + this correction, not §4-as-was.
- F-6: B-3's always-on assert set grows: single-fold-per-section-walk
  (today structural, Stratum.cpp:586-593), readiness
  (reads-lower-or-same-SCC, today NDEBUG at :1860-1875), R3 sole-deriver
  (agg table's member list == the agg view). Cite fix: the F19
  arm-pair assert is Stratum.cpp:1616 (inside the #ifndef NDEBUG block
  at :1595), not :1591.
- F-8: per-family R2 cutovers add .batches oracle sidecars to the
  differential cases each family touches; the §7.1 permutation check is
  MECHANIZED (sort published deltas per epoch, byte-compare) rather
  than eyeballed; DebugValidateCounts stays in every driver through R2.
- F-2 restatement adopted in §5: R1's justification leads with
  optimizability + bug-class + unified access; aggregates are the first
  beneficiary, not the driver. Constructor derives SEED_FOLD/
  FIXPOINT_FIRE terms from the §5.1 telescoped expansion per rule (the
  sign/position attributes ARE the Σ New^{<i} ⊗ Δ ⊗ Old^{>i} terms
  reified — kept exactly), so G1-class omissions are structurally
  impossible.
- Risk ledger (review Q4) recorded: #1 RQ5b merge-set drift — CLOSED by
  the mechanized audit above; #2 fixpoint-family lowering bugs masked by
  small batches — directed OptDiff case (same-round double-claim +
  remove-then-rederive) REQUIRED before the fixpoint-family cutover;
  #3 construction/scheduling divergence — always-on readiness asserts +
  a straddling-model stress case; #4 all-modes-identical counter drift —
  oracle + recount stay mandatory; #5 two-webs-alive stall — acyclic-
  first ordering + delete-with-cutover rule.

## 10. Vocabulary v3 (the binding pre-R1 object model) — record

Drafted, instantiated, and judge-checked 2026-07-15 (workflow: opus spec
writer → opus tc-artifact rewrite → dependence-completeness judge; the
identity-lowering judge re-ran separately after an API failure). The
BINDING artifacts live in DeltaRelationalIR.artifacts/:
- v3-spec.md — the object model (typed Vec values with def/use edges +
  element-shape/role/unique-contract; per-op and PER-ARM effect sets
  over {vector, counter±class, flag read/write, frozen-kInI,
  statecell(R3)}; access-plan trees for fold bodies; ordering DERIVED
  as WAR/RAW/WAW with the schedule a CHECKED LINEARIZATION; validators
  = B-3 + F-6 as graph checks; constructor derives Σ-terms from the
  §5.1 telescoped expansion) — WITH §A amendments A-1..A-6 resolving
  the dependence judge's findings (loop-carried epoch-boundary edges;
  PinnedOrder = topo sort + lowering-default tie-breaks, commit order
  is a lowering default not a graph edge; per-arm effect granularity;
  multi-def accumulating queues; element-shape as constructor
  knowledge; monotone-elision hook attribute).
- v3-tc-artifact.md — the tc case fully instantiated in v3 (dependence
  edges named per resource; validators evaluated; the §9 rewrite test
  list realized as graph facts: filter/drain fusion edges, G8 dead
  filters as zero-use defs, WCOJ plan-tree change sites).
- v3-judge-dependence.md — verdict: most orderings derive soundly;
  the two HIGH holes are closed by §A-1/§A-2.
R1 DISCIPLINE (spec §7.3, adopted): construct-alongside-and-validate —
BuildDRFlow + ValidateDRFlow + LinearizeAndCheck + V-OLD-EQUIV
(isomorphism against the old discovery's E-16 shared state) run inside
the existing build; the emission half is UNTOUCHED until R2 family
cutovers; R1 is a byte-identical-golden hard gate per §7.1.

### R1a — DR-IR inventory layer, construct-alongside (2026-07-15)

NEW lib/ControlFlow/Build/DR.h (300 lines: the v3 object-model core —
ElementShape/VecRole/UniqueContract/EffKind/Ctx/Pred(the ten)/DROpKind,
DREffect, DRVec, DRTable, DROp, DRFlowGraph) + DR.cpp (~430 lines):
BuildDRInventory derives the crossover and product-arm op families
INDEPENDENTLY from the Query (crossover pair per non-@never negate,
minus always / plus iff negated table differential, RuleClass
replicated exactly; product arm per side×sign, minus iff side
differential, position-keyed reads j<i kInNew / j>i kInI), and
ValidateDRInventory runs V-OLD-EQUIV (same crossover/product sets +
SCC map as the old discovery) plus the PROMOTED always-on B-3
validators (V-XOVER-ONE — lifting the NDEBUG assert at Stratum.cpp:
1595-1617 — V-PROD-MONO, V-PROD-CLASS), fprintf+abort style (survives
NDEBUG, matching the Unsupported convention). Hook: unconditional, in
BuildStratumPhases after the scheduling fixpoint converges, before the
readiness asserts; flat OldCrossoverRef/OldProductRef payloads cross
the anonymous-namespace boundary. Zero emission change; zero Runtime
change; LowerDRFlow not yet declared (R2).
GATES: debug+release builds; ctest 3/3; SUITE PASS (155) byte-identical;
Q5 spot @128: release/opt 134ms, none 76ms, debug/opt 940ms — no
regression vs the post-RQ5b medians (146.5/77/970).
R1b QUEUE (from the implementer + identity judge B-13): materialize
queues/frontiers as first-class DRVec def/use edges (multi-def per
A-4); Σ-term SEED_FOLD/FIXPOINT_FIRE derivation (subsumes the
RuleClass/position-rule replication); linearization SEEDED from the
integer lift; V-QCLEAR + round-scoped V-LOOP; per-arm effects; the
directed reconvergent-plumbing OptDiff case gating DiscoverBranches
memoization; join-pivot element-shape decision.

### R1b — DRVec materialization + branch/join inventory (2026-07-15)

DR.h +143 / DR.cpp +462 / Stratum.cpp hook +42; new corpus case
reconverge_1 (golden via --bless, new-case authoring). Six typed DRVecs
per differential non-induction-owned table (kIds; queues
sort-unique-at-drain, sets/frontiers multiset) + one shared join-pivots
vec per join (element_shape=kIdCols, union pivot columns — the carried
join-pivot OQ resolved; per-arm projection lives in the plan tree);
crossover/product queue-append DEF-EDGES recorded (multi-def per A-4).
Branch/join inventory derived independently with the MEMOIZED
path-SUFFIX worklist: the old DiscoverBranches is per-PATH (no visited
set), and suffix-memoization + prefix cross-products preserve the exact
per-path multiset — proven by the V-OLD-EQUIV multiset comparison on
every corpus case, not assumed. Empirical: the §1.4 exponential hazard
is LATENT today (dense table materialization keeps table-less paths ≤2
hops corpus-wide); reconverge_1 + the multiset check are the standing
guard. SeedDRStrata stores the old lift's integers per B-13 (the
independent linearizer is R1c); V-OLD-EQUIV extended with per-unit
strata equality.
Also this session: fixpoint_stress_1 (7c43eda) — the §9-risk-#2
directed witness: same-round double-claim (tc13+tc35 in round 1;
tc(1,5) exactly-once rests on InNewWithFrontier vs InNewSansFrontier),
REDERIVE partial restore, §5.1.1 phantom pairs, add-side stale drops;
oracle 5764 assertions. SUITE IS NOW 157 CASES.
GATES: debug+release builds; ctest 3/3; FULL SUITE PASS (157)
byte-identical (zero emission change; validators green corpus-wide);
Q5 spot @128: release/opt 135ms, debug/opt 936ms — no regression.

### R2 gate tooling — permcheck.py (2026-07-15)

tests/OptDiff/permcheck.py mechanizes the §7.1 permutation-only bless
check (F-8): segments old/new stdout by epoch label lines; published-
delta tokens ([+-](...)) compare as an order-free multiset per segment;
ALL other lines byte-identical in order; conservative (boundary
mismatch = FAIL, never resegmentation); per-case spec seam
(boundary_re/delta_re/inline_delta) exists but the defaults cover the
whole current corpus. Survey fact bounding the bless surface: only the
four product_* drivers print published deltas via deduced-log hooks
(each sorts within flush today); the other 42 differential cases
observe via query drains, whose lines are STRUCTURAL (token reordering
correctly FAILS). Self-tested against real goldens: accepts a
within-epoch shuffle, rejects dropped boundaries, changed deltas, and
drain-token reorders.

### R1c — op families + per-arm effects + Σ-term derivation (2026-07-15)

DR.h 591 lines / DR.cpp 2073 lines / Stratum.cpp hook +5. Nine new op
kinds (seed-fold, fixpoint-fire, chain-fold, claim-drain, retire,
rederive, frontier-filter, commit-sweep, negate-gate); PlanNode access
spines (B-1 bound-cols, pivot re-test, lowering choice); per-ARM
effects (A-3). Derivation from the §5.1 telescoped expansion with every
counting rule citing its Stratum.cpp anchor: sign availability, same-
SCC seed suppression DERIVED (+ V-SEED-SUP both spaces), CHAIN_FOLDs
for same-SCC head projections, the four-cell claim-relative matrix per
static position, F17 claim gates AS DATA, B-7 in-round clears + G10
dual-appends, E-17 deferral derived, commit sweeps incl. monotone Seal.
Negate gates ride plan spines CONTEXT-keyed (E-13: emitter's rule
encoded; §5.1's per-position split confirmed as the only live
schema/emitter delta — no new disagreements). Always-on validators:
V-QCLEAR, V-CLAIM-GATE, V-NEG-CTX, V-RETIRE-AFTER (structural until
R1d edges), V-DEFER, V-ONE-FOLD, V-SEED-SUP + the V-OLD-EQUIV CENSUS
(independent recount of the emitter-expected inventory, compared
per-kind, corpus-wide all 4 modes). Bug found+fixed in-stage:
use-after-move in FillSeedFoldArm (SIGSEGV on negate-on-lower-chain
cases; caught by the suite subset — the anchors under-covered that
path). Anchor censuses recorded in the R1c report (e.g. tcd: 2 seeds,
4 chain folds, 2 fires×2 arms, 8 drains, 6 retires, 3 rederives, 8
filters, 4 sweeps).
GATES: builds green; ctest 3/3 (one PointsTo timeout from machine
contention, clean rerun passed); FULL SUITE PASS (157) byte-identical
on the final binary; Q5 spot @128 release/opt 134ms, debug/opt 946ms —
no regression.
R1d QUEUE: materialize use-edges uniformly then derive RAW/WAR/WAW;
model the FIXPOINT_ROUND region shell (round-start frontier clears,
Δ-emptiness break) for round-scoped V-LOOP (B-10); inventory
PIVOT_ASSEMBLE before the R2 join-family cutover; eager-walk
INGEST_FOLD + standalone eager negate-gate; independent linearizer
behind V-OLD-EQUIV retiring B-13 seeded strata; give commit-sweep
publish-target a named field (currently on a reused bool, flagged).

### R1d — checked linearization + region shells (2026-07-15)

DR.h 699 lines / DR.cpp 3029 lines / Stratum.cpp hook +8 (the LAST
construction stage; still construct-alongside, zero emission/Runtime
change). `LinearizeAndValidateDRFlow` in one pass: (1) uniform vec
use-edge materialization from every op's drain effects (def-edges as
R1b/R1c minted, multi-def A-4); (2) §4 RAW/WAR/WAW derivation over
vecs + table-flag classes (kInI frozen), with loop-carried edges at
BOTH scopes — round-carried Δ frontiers (B-10) and epoch net_* feedback
(A-1) — excluded from the intra-scope topo and checked by V-LOOP; (3)
an INDEPENDENT linearizer = a Kahn topo sort under the B-9 band-key
tie-break (lead: eager gates / phase / commit-trailing; stratum asc;
band {seeds·crossovers·products·pivot-assembles, acyclic drains,
immediate filters, SCC round bodies claims/fires/chain-folds/retires,
round output rederive/deferred-filters, commit}; table-id; sign − before
+). New inventory: PIVOT_ASSEMBLE per SCC join (the shared kJoinPivots
union, dual-section registration mirror); the standalone EAGER
NEGATE_GATE (context=eager: normal→kInI, @never→kPresent — the F-5
cells, one per query.Negations()); FIXPOINT_ROUND region shells per
SCC×phase owning body/output op ids + claimed-* test vecs; the per-round
claimed-* frontier DRVecs (Δ_D/Δ_A) minted for recursive tables. Commit-
sweep publish-target now its OWN named field (`publish_target`), dropping
the reused `join_pivot` bool the R1c sweep borrowed. Always-on
validators promoted: V-LINEAR (pinned order topo-sorts the non-carried
edges), V-LOOP (both scopes), V-RETIRE-AFTER upgraded to ARM-granular
ORDERING (every same-group same-sign fire precedes its retire in the
pinned order — was structural in R1c), V-READY promoted always-on (every
RAW edge's writer stratum ≤ reader stratum — the :1917-1962 NDEBUG
readiness asserts as DR-graph checks), V-OLD-EQUIV(strata) now COMPARES
each op's derived-stratum against the seeded lift (replacing B-13's
stored-copy check; abort on mismatch) + V-OLD-EQUIV(order) certifying the
pinned order is key-monotonic (== the emission band walk).

DERIVED-vs-OLD-LIFT: no disagreement surfaced. This is expected, not a
missed check: per B-13/B-14 the STRATA stay SEEDED from the integer lift
(a full independent stratum fixpoint is not in R1d's scope), so
op_stratum reads the seed and V-OLD-EQUIV(strata) is an orphan-map guard
(every op resolves to a seeded unit) rather than an independent recompute.
The genuinely-independent derivation R1d lands is the LINEARIZATION; to
keep the §4 graph acyclic given R1c effects carry no per-RowFlag-bit
granularity, every non-carried hazard edge is DIRECTED by the band key,
so V-LINEAR and V-OLD-EQUIV(order) are consistency guards (self-satisfied
by construction, standing to catch a future key-inverting forced edge)
rather than cross-checks that could fail today. The load-bearing checks
that CAN fail on a perturbation are V-RETIRE-AFTER (arm-granular fire<
retire), V-READY (writer≤reader stratum — catches a mis-seeded lift),
V-LOOP (carried-edge shape), and the retained R1c census/isomorphism.

EAGER-WALK INVENTORY DECISION (gates-only cut, recorded): the standalone
EAGER NEGATE_GATE is inventoried (cleanly derivable — Build.cpp:1002
emits exactly one per negate view, pred = HasNeverHint()?kPresent:kInI).
The eager INGEST_FOLD family is CUT: the message→table fold sites live
inside the recursive `BuildEagerRegion` walk (Build.cpp:823/930+,
per-view UPDATECOUNT folds emitted inline as it descends successors) with
no externalized discovery struct to mirror faithfully the way the stratum
bands externalize {branches,joins,crossovers,products}. kIngestFold stays
a reserved DROpKind; R2's ^entry-proc family (or an R1e eager-web
externalization) is the place to inventory it against a real seam.

GATES: debug build green; ctest 3/3 (MiniDisassembler + PointsTo compile
the always-on DR validators at build time); anchors
transitive_closure_diff / d5_recursive_negate / fixpoint_stress_1 /
reconverge_1 all 4 modes exit 0 validators PASS; subset
'negate|product|transitive|cond|reconverge|fixpoint_stress' PASS (35);
FULL SUITE PASS (157) byte-identical on the final binary.
R2 QUEUE (family #1 = ACYCLIC seed/crossover/product — the lowest-risk
cutover per F-2): LowerDRFlow needs (a) the PlanNode→region emitter
(Access/Gate/Fold → scan-index/CHECKMEMBER/UPDATECOUNT, §3.2 identity
lowering); (b) the DRVec→VECTOR binding (the demoted debug (table,kind)
is the bridge to Context::table_delta_vecs during cutover); (c) the
pinned_order walk restricted to band 0 + acyclic drains/filters, deleting
the matching EmitSeedLoop/EmitCrossover/EmitProductArms + acyclic
EmitClaimDrain/EmitFrontierFilter calls in the same diff; the round
shells + fires stay on the old web until the recursive family cutover.
permcheck.py (F-8) is the bless referee; the 4 product_* drivers are the
only published-delta surface.

### R2 family #1 — acyclic-band cutover (2026-07-15)

THE FIRST EMISSION CUTOVER: seeds, crossovers, product arms, and the
acyclic claim-drain/frontier-filter bands now lower from the DR-IR
(LowerDRFlow + LowerSectionWalk/LowerCrossoverArm/LowerProductArm);
EmitSectionWalk, EmitCrossover, EmitProductArms and the old driver's
acyclic bands are DELETED in the same diff (555 lines deleted / 485
added, net −70; deleted names survive only in comments). Emit* kept for
the SCC path with caller evidence (EmitSeedLoop/EmitChainStep/
EmitHeadFold/EmitClaimDrain/EmitFrontierFilter/EmitJoinFire/
EmitRetireFrontier/EmitRederive). V-OLD-EQUIV legs all retained
(discovery untouched) and green corpus-wide.
HAZARD FOUND+FIXED: ProgramProcedureImpl::VectorFor is NOT memoized —
re-calling it minted a duplicate join-pivot vec, cascading every
downstream id (caught by the d5 golden diverge); LowerDRFlow reuses the
discovery-minted VECTOR* via a join_pivots map. Related lowering fact
recorded in code: the DR pinned_order's band key is sign-major and is
NOT the emission sibling order — the old driver's per-stratum band walk
is the lowering default.
GATES: builds green; ctest 3/3; 8 anchors ×4 modes OK; B-14 check came
back STRONGER than required — program.ir RAW-BYTE-IDENTICAL on both
anchors (id bijection = identity); FULL SUITE PASS (157) byte-identical,
zero churn (permcheck not exercised — nothing to bless); Q5 spot @128
release 136ms / debug 958ms (no regression). FLAGSHIP BENCH: the
interleaved A/B is vacuous by construction — generated datalog.h/.cpp
byte-compared IDENTICAL pre/post cutover on all four flagship workloads
(tc_random, pure_cycle, deep_chain, flip_storm; frozen 0d8d00e compiler
vs cutover compiler, same environment) — bit-identical case binaries;
recorded as the perf-neutrality witness in lieu of a tautological run.
FAMILY #2 QUEUE: SCC/recursive band (round shells via DRRound,
claim/fire/chain-fold/retire bands, deferred filters), driving the
surviving Emit* primitives; VectorFor-reuse discipline generalized via
the (table,kind)→VECTOR* bridge; old discovery vectors + scheduling
fixpoint become deletable only after family #2; add the SCC anchors to
the B-14 check (they exercise the round shells this cut left on the old
web).

### R2 family #2 — SCC/recursive-band cutover (2026-07-16)

THE HIGHEST-RISK DIFF, LANDED CLEAN. Stratum.cpp +240/−196: LowerDRRounds
+ LowerRoundBody lower the per-SCC×phase FIXPOINT_ROUND shells into the
exact old region tree (OVERDELETE induction: round-vec clears, in-round
claim drains, fires, chain folds, retires; output REDERIVE; INSERT
mirror; BOTH deferred filters in add-loop output — E-17), band structure
as the lowering default per the family-#1 lesson, all vecs via the
memoized TableDeltaVector bridge (zero re-minting). DELETED:
build_claim_round_loop, the del/add SCC driver band, the acyclic/scc
partition, scc_joins derivation, the is_recursive lambda. All Emit*
primitives retain callers as lowering templates. Bug found+fixed
in-stage: kFixpointFire ops carry no scc_group (derived from
fire_table's SCC); the first draft filtered on the always-~0u field and
emitted ZERO fires — tc collapsed to round-0 and the anchors caught it.
GATES: builds green; ctest 3/3; 7 anchors ×4 modes OK — fixpoint_stress_1
(exactly-once sentinel) and deep_chain_retract (constant-stack sentinel)
both PASS; B-14 vs a83ce1c on the three SCC anchors: RAW-BYTE-IDENTICAL
(bijection = identity, exceeded); FULL SUITE PASS (157) zero churn
(permcheck not exercised); flagship generated code BYTE-IDENTICAL vs the
frozen PRE-R2 binary (0d8d00e) on all four workloads — perf-neutrality
witness transitively through both families; Q5 spot @128 release 132ms /
debug 937ms.
FAMILY #3 QUEUE: commit sweeps (kCommitSweep) + entry/ingest
(kIngestFold, reserved cut) still on the old web; old discovery +
scheduling fixpoint still load-bearing (DR strata seeding, V-OLD-EQUIV,
LowerDRRounds reads drain_stratum) — family #3 stamps the drain stratum
onto DRRound, provides DR-native stratum gating, then deletes the
fixpoint + discovery vectors and retires the V-OLD-EQUIV legs whose
comparands die; consider adding scc_group to kFixpointFire (currently
derived via RecursiveSCC).

### R2 family #3 — commit sweeps + the OLD-DISCOVERY DELETION (2026-07-16)

THE EPOCH'S HEADLINE DELETION: the DR-IR is now the SOLE authority for the
stratum machinery. −974/+415 across five files (the deletion total; net
−559). Cumulative in lib/ControlFlow/Build/ since a83ce1c (R2 family #1):
−1070/+590. What died: the hand-coded scheduling fixpoint (the
`while(changed)` integer lift over branches/joins/crossovers/products +
SCC pinning); `DiscoverBranches` (the un-memoized path-DFS §1.4 hazard),
`CollectSectionTargets`, `TableOwnerStratum`, the `CrossoverEmission` /
`ProductEmission` structs; `SeedDRStrata` + the flat Old*Ref payloads +
the entire `#ifndef NDEBUG` readiness-assert block; the hand-coded
commit-sweep `impl->tables` loop in `PublishDifferentialMessageVectors`;
and the V-OLD-EQUIV legs (SCC-map / crossover-SET / product-SET /
branch-MULTISET / join-SET / per-unit-strata comparisons against the old
discovery). `BuildStratumPhases` shrank from ~530 body lines to ~160.

WHAT BECAME DR-AUTHORITATIVE:
- STRATA DERIVATION PORTED (B-13 RETIRED). `DeriveDRStrata` (DR.cpp,
  replaces `SeedDRStrata`) is the same monotone integer lift, now run over
  DR branches / joins / crossover ops / product ops + the scc_map: initial
  strata from the spec (owner_stratum of the head table / join view
  stratum / negate|product view stratum — each rule citing its Stratum.cpp
  anchor), then ready_after/ready_across/negated_tables_ready/SCC-pinning
  to fixpoint. It fills branch_stratum / join_stratum / crossover_stratum /
  product_stratum / drain_stratum — the exact maps LowerDRFlow/LowerDRRounds
  already read. The DR side now COMPUTES strata, not copies them.
- DRAIN-STRATUM NATIVIZATION. `DeriveDRStrata` STAMPS the SCC drain stratum
  onto each `DRRound` (new field `DRRound::drain_stratum`) and `scc_group`
  onto each `kFixpointFire` op. `LowerDRRounds` reads `round.drain_stratum`
  (dropping its `drain_stratum` map param) and `LowerRoundBody` reads
  `op->scc_group` (retiring the family-#2 RecursiveSCC re-derivation
  workaround).
- COMMIT SWEEPS LOWERED. `LowerCommitSweeps` (Stratum.cpp) emits one
  COMMITSWEEP per `kCommitSweep` op (differential: flavor + publish-target
  message-attach; monotone: Seal), in `impl->tables` order (== the old
  loop's order). Placement UNCHANGED — the AUDIT established that the sweep
  band must stay siblings of the monotone `iter_par` inside
  `PublishDifferentialMessageVectors`' outer series (moving it into
  BuildStratumPhases' stratum series would change region-tree nesting and
  break byte-identity), so the graph is stashed on `context.dr_flow`
  (a `std::shared_ptr<DRFlowGraph>`, forward-declared in Build.h) and the
  sweep call fires from Procedure.cpp at the exact old point.
- VALIDATORS RETAINED: `ValidateDRInventory` keeps the INTRINSIC B-3 legs
  (V-XOVER-ONE, V-PROD-MONO, V-PROD-CLASS, V-JOIN-ONE) over `flow` alone;
  `ValidateDROps` (the census — V-ONE-FOLD/V-SEED-SUP/V-NEG-CTX/
  V-CLAIM-GATE/V-DEFER/V-RETIRE-AFTER-structural + the per-kind op count
  recompute) and `LinearizeAndValidateDRFlow` (V-LINEAR/V-LOOP/
  V-RETIRE-AFTER-arm-granular/V-READY — the promotion of the deleted NDEBUG
  readiness asserts) stand unchanged; both recompute expectations from the
  query, never the deleted discovery. The old part-(a) V-OLD-EQUIV(strata)
  per-op check is DELETED, not kept: with B-13's separate seeded copy gone and
  DeriveDRStrata the sole writer of the maps op_stratum reads, it compared a
  value against itself (tautology). The order-consistency leg
  (V-ORDER-CONSISTENT: pinned order non-decreasing in the band key) survives.

REVIEW FIXES (2 low-severity findings, both fixed before the final suite run):
- GUARD/SWEEP: the early-return "no phase work" guard now runs AFTER stashing
  `context.dr_flow`, because the old code emitted commit sweeps for every
  differential table (INCLUDING induction-owned) even when BuildStratumPhases
  short-circuited; an early return before the stash would drop those sweeps.
  Strata derivation + validators allocate no ids, so the stash-then-guard
  reorder is byte-neutral.
- STRATA-SET keys on PHASE OWNERSHIP, not the raw `flow.drain_stratum` key set:
  the crossover/product lifts `operator[]`-insert non-phase (monotone/
  induction) fold targets, so iterating ALL drain_stratum keys could (on an
  exotic induction-owned-negate-in-SCC shape, unexercised by the corpus) add a
  stratum series the old code omitted. The set now filters `TableIsDifferential
  && !TableIsInductionOwned` == the old `phase_table_order`, closing the latent
  divergence.

kIngestFold DECISION: CUT (unchanged from R1d). The eager INGEST_FOLD sites
live inside the recursive `BuildEagerRegion` walk with no externalized
discovery seam to mirror; family #3 lowered the commit-sweep band (the
ingest-ADJACENT target) but did NOT touch the entry emission
(`ExtendEagerProcedure` / `build_explicit_loop` stay hand-coded).
kIngestFold stays a reserved DROpKind for an R1e/R2-entry-family seam.

GATES: (a) debug build green, zero new warnings (dead discovery helpers
removed). (b) 8 anchors ×4 modes OK (transitive_closure_diff,
d5_recursive_negate, fixpoint_stress_1, reconverge_1, deep_chain_retract,
tc_nonlinear_diff, product_diff, cond_diff_flipflop). (c) B-14 vs 6c4ca4f
on transitive_closure_diff / fixpoint_stress_1 / product_diff:
RAW-BYTE-IDENTICAL (bijection = identity, exceeded — the DR-derived strata
equal the old lift's byte-for-byte). (d) FULL SUITE PASS (157) ZERO CHURN
(permcheck not exercised — nothing to bless; the 4 product_* publish-order
drivers unchanged). (e) ctest 3/3. (f) no rebuild mid-suite.

FAMILY #4 / R3 QUEUE: the tautological V-OLD-EQUIV(strata) self-check in
LinearizeAndValidateDRFlow is comment-stale (harmless); the eager
INGEST_FOLD externalization (R1e or an R2 entry-proc family) is the last
hand-coded emission web; R3 (aggregates / GROUP_UPDATE) gates only on the
acyclic families, all now DR-lowered.
