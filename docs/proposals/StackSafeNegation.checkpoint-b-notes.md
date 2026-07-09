# Stage 3 checkpoint (b) working notes (session scratch, not committed)

Branch: derivation-counters. HEAD after ee61d77 (unstratified-negation
error hardening). Suite state: monotone cases green; ALL differential
cases red (removals are no-ops since checkpoint (a)); expected-diagnostic
cases: aggregate_1, kvindex_1-4, evm_func_parse.

## What checkpoint (a) left (verified by reading)

- Runtime Table.h: monotone `Table` (TryAdd/Present) + `DiffTable` complete
  per §3.1 (AddDerivation/SubDerivation/Add-SubExplicit, 8 predicates,
  TryClaimDel/TryClaimAdd, RetireDelFrontier/RetireAddFrontier(Vec<u32>),
  Commit(sink), DebugValidateCounts).
- IR: UPDATECOUNT (is_add + DerivClass, body = on-crossing),
  CHECKMEMBER (MembershipPredicate, IfPresent/IfAbsent), COMMITSWEEP
  (table + optional message). Emitters in Database.cpp:1107/1198/1253 work.
- Build side is ADD-ONLY: ExtendEagerProcedure loops receive vectors,
  UPDATECOUNT(+, kNonRecursive) per row, eager walk via
  BuildEagerInsertionRegionsImpl/InTryInsert (UPDATECOUNT(+ ,
  EmissionDerivClass)); removal vectors are declared parameters consumed by
  NOTHING. Inductions: add-polarity fixpoint only (Induction.cpp), gates
  via CHECKMEMBER(kPresent). Commit sweeps appended at end of entry proc
  (PublishDifferentialMessageVectors), publish via
  context.commit_published_view. ExtractPrimaryProcedure splits on message
  vector uses.
- EmissionDerivClass(view): kRecursive iff view's induction is in
  kAccumulatingCycleRegions state.

## Checkpoint (b) target architecture

One received batch (entry+primary proc pair):

    entry proc:
      [ingest] per differential message: NETBATCH(add_vec, rem_vec);
        loop rem_vec -> UPDATECOUNT(-, explicit) crossing -> append
          delQ[table]; loop add_vec -> UPDATECOUNT(+, explicit) crossing ->
          append addQ[table]   (explicit = new is_explicit flag on
          UPDATECOUNT? NO - use separate ops: AddExplicit/SubExplicit are
          UPDATECOUNT with a new `is_explicit` bit. DECISION: add bool
          is_explicit to UPDATECOUNT; codegen emits Add/SubExplicit.)
        monotone receives: today's path verbatim.
        eager walk: monotone stretches unchanged; when the walk folds into
        a DIFFERENTIAL table (InTryInsert with differential table), the
        crossing body appends the row to addQ[table] and the walk STOPS
        (successors run in that table's stratum INSERT phase).
    primary proc (stratum SERIES, topological order; only strata owning
    differential tables get phases; monotone-only strata: eager, already
    inside ingest walk):
      for each differential table t owned by stratum s (owner rule: the
      model's unique multi-view stratum if one exists, else highest member
      stratum; EquivalenceSet::stratum already = max member stratum —
      REVISIT for multi-view case at (c)):
        OVERDELETE: [seed joins: for each consumer view v of a lower
            differential table p: loop outDel(p) / outAdd(negated p) ->
            enumerate v's instances (delta schema) -> UPDATECOUNT(-,class)
            crossing -> append delQ[t_v]]  (b: only non-recursive strata)
          sort+unique delQ; VECTORLOOP delQ -> CLAIM(del) -> append D_t
          (single round in (b); loop comes at (c)); RETIRE loop over D_t
          (uniform with (c), clears kDelNow)
        REDERIVE: VECTORLOOP D_t -> CHECKMEMBER(kRecursivelySupported) ->
          append addQ[t]
        INSERT: [seed joins +, mirror] sort+unique addQ; VECTORLOOP addQ ->
          CLAIM(add) -> append A_t -> BuildEagerInsertionRegions(v,
          successors...)  <- the cut walk continues here, per view of t
          ... wait: addQ is per TABLE; successors are per VIEW. Rows in a
          shared table belong to any member view; enqueue must carry view
          identity OR queues are per VIEW-with-distinct-successors. For (b)
          DECISION: queues are PER VIEW (kind + view tag on the vector),
          claims/folds hit the view's table. Straddling models: each view
          drains at its own stratum? NO - phases per table at owning
          stratum. For (b) simplification: per-view queues drained at the
          view's stratum in topo order; correct for singleton strata
          because view stratum = table owner stratum for the common case;
          REVISIT at (c) for multi-view strata.
          RETIRE loop over A_t.
        BUILDFRONTIERS: outDel(t): VECTORLOOP D_t ->
          CHECKMEMBER(kNetDeleted) -> VECTORAPPEND outDel;
          outAdd(t): VECTORLOOP A_t -> CHECKMEMBER(kNetAdded) -> append.
          outDel/outAdd looped by HIGHER strata seed joins.
      commit sweeps + monotone-backed publishes: unchanged from (a).

## Delta schema reads (§5.1) for seed joins at view v, delta at pred p_i

Position order: predecessors sorted by (stratum, syntactic index).
- lower j < i: CHECKMEMBER(kInNew) (negated: absent-in-InNew)
- lower j > i: CHECKMEMBER(kInI) (negated: absent-in-InI)
- same-stratum j (only in (c) fixpoint): see §5.1 fixpoint table.
Sign: OVERDELETE loops p's outDel (positive atom) and p's outAdd for
NEGATED p (checkpoint (d)); INSERT dual. JOIN enumeration: delta row gives
pivot values; other preds via index scans (BuildMaybeScanPartial) gated by
CHECKMEMBER instead of raw scans. TUPLE/CMP/MAP/MERGE: single-pred map
through per node semantics. UPDATECOUNT class = DerivationClassInto(head)
per RULE (errata 2: the rule's fixed class = recursive iff ANY body atom
recursive w.r.t. head) — for (b) non-recursive strata it's kNonRecursive.

## New vocabulary (delegated to subagent, mechanical)

1. UPDATECOUNT gains `bool is_explicit` (default false) -> codegen emits
   AddExplicit/SubExplicit (is_add still selects sign; is_explicit only
   valid on differential tables; assert no DerivClass::kRecursive with it).
2. CLAIM op: table + col_values + bool is_del; body on successful claim.
   Codegen: Find(row) -> TryClaimDel/Add(id) -> body.
3. RETIRE op: table + col_values + bool is_del; no alternate body.
   Codegen: Find(row) != kNoRow -> RetireDel/Add(id).
   Runtime: REPLACE RetireDelFrontier/RetireAddFrontier(Vec) with per-id
   RetireDel(uint32_t)/RetireAdd(uint32_t) (delete Vec forms + port
   tests/Runtime).
4. NETBATCH op: two vector refs (adds, removes). Runtime helper
   rt::NetBatch(Vec<Row>&, Vec<Row>&): arithmetic per-row net; adds :=
   unique net>0 rows, removes := unique net<0 rows. New ProgramOperation
   value; ClassifyVector: read+written for both.
5. MembershipPredicate kNetDeleted (kDel && !kAdd), kNetAdded (kAdd &&
   !kDel) + DiffTable::NetDeleted/NetAdded(id) + Format + PredicateMethod.
6. VectorKind: kDeleteQueue, kAddQueue, kOverdeleteSet, kAdditionSet,
   kNetRemovals, kNetAdditions (+ printers/codegen vector decl sites).
Threading recipe: exactly the sites checkpoint (a) used for UPDATECOUNT/
CHECKMEMBER/COMMITSWEEP — see `git show d817785` per file:
Program.h(pub+int), Operation.cpp, Program.cpp, Visitor.cpp, Format.cpp,
Optimize.cpp (CLAIM: dead-body treatment like UPDATECOUNT, region not
removable; RETIRE/NETBATCH: never no-op), Analyze.cpp (mirror how (a)
handled UPDATECOUNT — conservative), Database.cpp (Walk/Emit dispatch +
emitters). No builder emissions yet; suite must stay byte-identical.

## Builder work (mine, after vocabulary lands)

- Procedure.cpp ExtendEagerProcedure: differential receives -> NETBATCH +
  two explicit-fold loops + queue appends; remove the "no region consumes
  it yet" comment.
- InTryInsert / BuildEagerInsertionRegionsImpl: differential-table fold ->
  crossing appends to the view's addQ and stops the walk (unless view is
  inductive — (c) keeps induction path for now; (b) scope = views outside
  any induction).
- New file suggestion: lib/ControlFlow/Build/Stratum.cpp with the
  per-stratum phase builder (seed joins, claim/retire loops, REDERIVE,
  BUILDFRONTIERS), called from BuildEntryProcedure between
  CompleteProcedure and PublishDifferentialMessageVectors.
- ClassifyVector: new ops.
- Gate for (b): monotone subset byte-identical; non-recursive differential
  cases (two_hop_phantom, cond_*, booleans_diff, select_6, compare_1,
  tower_of_compares?) go green incl. oracle agreement; recursive cases
  (tc_*, deep_chain_retract, negation cases) stay red until (c)/(d).

## Open items / risks

- Induction interplay: inductive views keep their (a) add-only fixpoint
  until (c); their tables ARE differential -> their InTryInsert folds must
  NOT be cut to queues yet (guard: view.InductionGroupId() or
  context.view_to_induction presence).
- kInductionRechecks/etc already deleted; MODESWITCH gone.
- Query forcing procs (kQueryMessageInjector) call the message handler ->
  full mini-epoch runs automatically since phases live in primary proc.
- Publish-order determinism: COMMITSWEEP iterates `touched` in first-touch
  order — deterministic per program but differs from old flusher order;
  the Stage-0 audited re-bless list (negation_flap flap B,
  transitive_closure_diff F16 fix, plus order-only churn) applies at (e).

## Design increment 2 (after reading Join.cpp)

FACT: today's add-path TABLEJOIN loops PIVOT vectors (pivots of new rows)
and scans ALL sides' full tables at each touched pivot -> re-enumerates
old*old instances at re-touched pivots. Set-semantics-idempotent before;
MULTISET-WRONG for counters. Therefore:

1. Differential-position enumeration NEVER uses pivot-vector joins. Seed
   (and (c) fixpoint) enumeration = nested-loop shape (see the disabled
   BuildNestedLoopJoin in Join.cpp): VECTORLOOP over the delta table's
   frontier vector (FULL rows) -> index scans of the other sides gated by
   CHECKMEMBER with §5.1 position predicates (lower j<i: kInNew; lower
   j>i: kInI; same-stratum only at (c)). Read positions of a head-chain =
   delta table + each JOIN's other sides (+ negated tables at (d)).
   Position order: fixed total order by (stratum, syntactic index).
2. Cut rule: the eager walk stops at every edge into a DIFFERENTIAL view.
   Monotone stretches run eagerly as today. A monotone view feeding a
   differential consumer accumulates an outAdd frontier vector (append
   under its TryAdd crossing / at its delivery point in the eager walk).
   Differential tables get outDel/outAdd from BUILDFRONTIERS.
3. Monotone Table gains a sealed-row-id watermark: `sealed` (uint32);
   InI(id) = id < sealed; InNew = row exists. Sealed := NumRows() at end
   of every epoch INCLUDING the init proc epoch. Reuse COMMITSWEEP on
   monotone tables = Seal() (no new region kind). Init proc must also run
   COMMITSWEEPs for differential tables it touches (unit relations!).
4. Accepted inexactness (documented): instances whose body reads only
   monotone tables can re-fire across batches at re-touched pivots when
   they sit upstream of the differential cut (eager pivot-join territory)
   -> C_nr of the boundary table INFLATED by never-retractable instances.
   Harmless for presence correctness (those instances can never stop
   firing); counters are exact for all differential-position folds.
   Frontier vectors are sort-uniqued so within-batch dups collapse.
5. Claimed adds/dels do NOT walk successors: downstream propagation is
   exclusively higher-stratum seed joins over outDel/outAdd. For (b)
   singleton (non-recursive) strata: OVERDELETE = seeds, sort-unique delQ,
   loop+CLAIM(del)+append D; REDERIVE skipped (C_r impossible); INSERT =
   seeds, sort-unique addQ, loop+CLAIM(add)+append A; BUILDFRONTIERS =
   filter D by kNetDeleted -> outDel, A by kNetAdded -> outAdd. No
   retire/swap needed in singleton strata.
6. (b) scope guard: multi-view strata (recursive) keep the checkpoint-(a)
   induction path untouched; only views outside inductions get the new
   phases. Frontier vectors of recursive tables at (b): built the same way
   at their stratum? NO - (b) leaves recursive strata alone entirely; their
   downstream monotone... their downstream differential consumers' seeds
   loop empty vectors (declared, unfilled) until (c). Differential cases
   whose cycles are recursive stay red until (c) - expected.
7. Vector lifetime: generated vectors are per-invocation locals; queues
   written by entry proc reach primary proc via the existing
   ExtractPrimaryProcedure param plumbing (ClassifyVector must know the
   new append/loop ops).

## Design increment 3: implementation sketch (final before code)

Frontier/queue vectors are PER TABLE (not per view): model-sharing member
views observe the same physical rows; the delta walk fans out over every
member view's consumer edges with the already_added/last_table discipline
preventing re-folds (mirror of BuildEagerInsertionRegionsImpl).

Boundary rule (edge into a differential consumer):
- ExtendEagerProcedure, differential receive: NETBATCH; add-loop:
  UPDATECOUNT(+ explicit) crossing -> append addQ[T_recv]; remove-loop:
  UPDATECOUNT(- explicit) crossing -> append delQ[T_recv]; NO successor
  walk. (Differential receives always have tables: FillDataModel persists
  every differential view's predecessors.)
- BuildEagerInsertionRegionsImpl: after InTryInsert(view):
  * view's table differential && view non-inductive: crossing body =
    VECTORAPPEND addQ[T]; return (no successors).
  * else per successor: if succ.CanReceiveDeletions() && succ
    non-inductive: skip walking it, and ensure the boundary append: if
    view's table is MONOTONE: append row to outAdd[T_view] under the fold
    crossing (once per table; table exists by FillDataModel). If view's
    table is differential+inductive (union output case, below) nothing
    here. Else walk BuildEagerRegion(succ) as today.
- Induction output loops (BuildOutputLoop body, post kPresent gate):
  replace BuildEagerInsertionRegions(NonInductiveSuccessors) with
  VECTORAPPEND outAdd[T_union]. Downstream propagation becomes uniform
  phase seeds. (OVERDELETE never seeded from inductions until (c):
  recursive removal stays red.) Monotone inductions (cannot produce
  deletions): keep eager successor walk, subject to the same per-succ cut.

Phase construction (new file lib/ControlFlow/Build/Delta.cpp):
  void BuildDeltaPhases(ProgramImpl*, Context&, Query) called in
  BuildEntryProcedure AFTER CompleteProcedure, BEFORE
  PublishDifferentialMessageVectors. Steps:
  1. Collect tables: differential tables + monotone boundary tables;
     stratum(T) = max Stratum() over T->views. Sort strata ascending.
  2. Vectors (entry proc; per differential table in a SINGLE-VIEW stratum):
     delQ/addQ (kDeleteQueue/kAddQueue), D/A (kOverdeleteSet/kAdditionSet),
     outDel/outAdd (kNetRemovals/kNetAdditions). Monotone boundary tables:
     outAdd only. Recursive-strata differential tables at (b): outAdd
     only (filled by induction output append); delQ declared if consumers
     exist? NO — (b): outAdd only, no del side.
  3. SERIES per stratum s ascending, appended to entry body after the
     ingest PARALLEL: for each owned differential table T (single-view
     stratum only at (b)):
     OVERDELETE: for each rule chain (head T, delta position table P lower)
       where P differential: loop outDel(P) -> DeltaChain(sign -) ->
       UPDATECOUNT(-, class(rule)) on T, crossing -> append delQ[T].
       [negated P at (d)]. Then VECTORUNIQUE delQ; VECTORLOOP delQ ->
       CLAIM(del, T) -> VECTORAPPEND D.
     [REDERIVE skipped: singleton stratum, C_r structurally impossible]
     INSERT: seeds: for delta P differential: loop outAdd(P); for P
       monotone boundary: loop outAdd(P) -> DeltaChain(sign +) ->
       UPDATECOUNT(+, class) on T, crossing -> append addQ[T]. Then
       VECTORUNIQUE addQ; VECTORLOOP addQ -> CLAIM(add, T) -> append A.
     BUILDFRONTIERS: loop D -> CHECKMEMBER(kNetDeleted, T) -> append
       outDel; loop A -> CHECKMEMBER(kNetAdded, T) -> append outAdd.
  4. DeltaChain emission = walk from (member view of P) consumer edges
     through non-materialized/non-cut views to the FIRST table fold:
     TUPLE/UNION: map vars, continue. CMP: CreateCompareRegion. MAP:
     CreateGeneratorCall(bottom_up=true). JOIN: for each other pred p_j in
     position order: BuildMaybeScanPartial-style index scan of T_pj +
     TUPLECMP + CHECKMEMBER(j<i ? kInNew : kInI) — monotone p_j: table
     watermark answers both. NEGATE: (d); for (b) assert unreachable in
     differential chains? NO — forward gate exists in eager; in delta
     chains gate on kInNew(negated) for + ... defer: (b) emits the forward
     CHECKMEMBER(kInNew) gate for + chains and kInI for - chains per the
     dualized read; crossover joins deferred to (d) — cases relying on
     negated-side flips stay red.
     Rule class: kRecursive iff any read table (incl. P) shares T's
     stratum — impossible at (b) singleton strata -> kNonRecursive.
  5. Watermarks: COMMITSWEEP emitted for every monotone boundary table
     (Seal) alongside existing differential sweeps; also append sweeps for
     tables touched by the INIT proc at its end (unit relations + constant
     flows), incl. monotone seals.

Position order over a chain's read tables: order by (stratum(T_read),
first-encounter order along the chain walk); the delta table P occupies
its own position; j comparisons per read site.
    CAVEAT: with singleton strata all read positions are distinct strata —
    (stratum) alone is a total order at (b).

Post-(b) expected green: two_hop_phantom, booleans_diff, cond_*,
compare_1, select_6, optimize_6?, tower_of_compares, cf13-16 subset
(whichever have no recursion/negation-crossover). Still red: tc_*,
deep_chain_retract, merge_5?, union_sibling_diff (recursive),
negation_flap + negate_* (crossover, (d)).

## Design increment 4 (OWNER INPUT, supersedes increment 2 pt.1 and
## increment 3 pt.4's nested-scan seeds)

Owner asked whether TABLEJOIN itself should be restructured instead of
building a parallel nested-loop delta walk. Adopted: KEEP the TABLEJOIN
pivot-loop enumeration and make the per-combination body DELTA-EXACT with
change-dispatch gates. Rationale: the pivot loop enumerates each
combination exactly once per batch (sort-uniqued vector, join emitted
once), so gating gives exact multiset counting while reusing the existing
join machinery; cost = what today's compiler already pays per add.

Dual gate chains under the join's TUPLECMP, in a PARALLEL:
  + arm: chain CHECKMEMBER(kInNew) over all sides (non-delta reads), then
    first-changed dispatch: CHECKMEMBER(kNetAdded r1)[present: fold +1]
    [absent: CHECKMEMBER(kNetAdded r2)[...]...]. Monotone side "net-added"
    = row id >= sealed watermark; monotone InNew = existence.
  - arm: chain CHECKMEMBER(kInI) over all sides, then first-net-deleted
    dispatch -> fold -1.
  Folds: UPDATECOUNT(sign, class) into the head chain's first table,
  crossing -> addQ/delQ of that table. Head chain after the join =
  single-pred plumbing only (every joined view is persisted by
  FillDataModel => at most one JOIN per table-to-table segment).
  Case matrix verified: pure add +1 once; pure delete -1 once; mixed
  add/delete same instance: NEITHER arm fires (no seed phantom pairs, no
  negative dips from seeds); both-added / both-deleted: fires once via
  first-changed dispatch; kDel&kAdd rows (delete-then-rederive) read as
  unchanged. First-changed-position attribution = §5.1's rule.

Placement:
  - Non-recursive-stratum differential joins: join emission moves to the
    join's stratum phase; pivot appends come from seed loops over each
    pred's outDel/outAdd frontiers (append pivot cols only, one per
    frontier row, both signs into ONE pivot vector). Enumeration runs ONCE
    at stratum start (reads only lower strata), writing delQ/addQ; then
    OVERDELETE claims delQ, INSERT claims addQ, BUILDFRONTIERS as before.
  - Monotone joins (incl. monotone->differential boundaries): stay in the
    eager walk with the same +arm gates (watermark-based) => boundary
    folds become EXACT; drop increment-3's accepted C_nr inflation.
  - Single-pred chains (TUPLE/CMP/MAP/UNION segments): trivial delta walk,
    no position discipline (only read table = the delta itself).
  - (c) recursive strata: telescoped fixpoint schema over claim rounds
    stays (final-state reads don't exist mid-fixpoint) — gate form is
    seed-only.
  - Today's kPresent gate chain in ContinueJoinWorkItem::Run (deletable
    preds) is subsumed by the +arm InNew chain for counter-fed joins.
Vocabulary impact: none removed — kNetDeleted/kNetAdded now also serve as
join gates. ADD to my builder work: monotone Table sealed watermark
(uint32, Seal() via COMMITSWEEP on monotone tables incl. init proc) and
predicate-sensitive CHECKMEMBER emission on monotone tables (kInI: id <
sealed; kNetAdded: id >= sealed; kInNew/kPresent: existence).

## Design increment 5 (OWNER INPUT, refines increment 4): delta sections
## INSIDE TABLEJOIN, not gate chains under it

Owner: introduce a new operation or a new "section" of the table join /
fixpoint loop; creative liberties with pre-existing operations allowed.

Adopted: TABLEJOIN gains two optional section bodies alongside `body`:
  added_body:   per combination, all sides InNew AND >=1 side net-added
                (kAdd && !kDel; monotone side: id >= sealed watermark)
  removed_body: per combination, all sides InI AND >=1 side net-deleted
                (kDel && !kAdd; monotone side: never)
Emitter computes the per-side booleans DIRECTLY ON SCANNED ROW IDS (flag
byte reads / watermark compares) — no by-value Find() re-lookups, which is
why sections beat external CHECKMEMBER chains. Case matrix from increment
4 unchanged (delta-exact, no seed phantoms, first-changed attribution now
implicit: the whole-combination predicate fires exactly once per batch
because the join runs once over a sort-uniqued pivot vector).
- Today's `body` = monotone eager section; monotone programs must emit
  byte-identical code (goldens).
- The CHECKMEMBER(kPresent) side-gate chain in ContinueJoinWorkItem::Run
  dissolves into the sections for differential joins (it was also wrong
  mid-batch: kPresent reads live counters, not the frozen discipline).
- Precedent for multi-section regions: INDUCTION (init/cyclic/output).
  Printer shows named added:/removed: sections.
- (c) plan: fixpoint claim-round reads (SurvivesSoFar/AliveAtClaim/
  InNewWithFrontier/InNewSansFrontier) become a third section flavor of
  the same join emission (round-relative discipline), keeping one join
  region kind with named section disciplines.
- IR shape: ProgramTableJoinRegion gains AddedBody()/RemovedBody()
  optionals + impl UseRef<REGION> added_body/removed_body; Optimize.cpp
  join OptimizeImpl treats empty sections as removable bodies; Equals/
  MergeEqual/Hash account for sections.
- Monotone Table: uint32 sealed watermark + Seal() (COMMITSWEEP on
  monotone tables, incl. init proc) so "net-added this batch" = id >=
  sealed is well-defined all epoch.

## Part-3 worklist (builder; parts 1+2 committed as f94b23a, 452c1d7)

STATE: vocabulary complete (CLAIM/RETIRE/NETBATCH/is_explicit/kNet*/
VectorKinds; TABLEJOIN added_body/removed_body emitted on scanned ids;
monotone Table sealed watermark, COMMITSWEEP-on-monotone = Seal(); \
CHECKMEMBER on monotone accepts kInI/kNetAdded/kNetDeleted). Nothing
constructs them yet; suite state unchanged from checkpoint (a) + expected-
diagnostic evm_func_parse.

3a (commit separately; monotone goldens byte-identical, differential still
red but compiling):
  1. Context (Build.h): per-TABLE vector maps: delq/addq/dset/aset/outdel/
     outadd (only for differential tables in single-view strata) + outadd
     for monotone boundary tables + helper GetOrCreate.
  2. ExtendEagerProcedure (Procedure.cpp): differential receives: NETBATCH
     on (vec, removal_vec); remove-loop UPDATECOUNT(-, explicit) crossing
     -> VECTORAPPEND delQ[T]; add-loop UPDATECOUNT(+, explicit) crossing ->
     append addQ[T]; NO successor walk. Monotone receives unchanged.
  3. Cut rule (Build.cpp BuildEagerInsertionRegionsImpl): view's table
     differential && !view inductive -> InTryInsert crossing body appends
     addQ[T], return. Else per-succ: differential non-inductive succ ->
     skip walk + ensure ONE outAdd append under this view's monotone fold
     crossing (boundary). NOTE: check succ handling for JOINs — an eager
     JOIN with a differential view is CUT at the join view itself (its
     pivot appends move to phases at 3b; at 3a differential joins simply
     don't get eager pivot appends anymore... CAREFUL: that changes
     -ir-out for differential cases (fine — they're red) but MUST NOT
     change monotone-only programs).
  4. BuildOutputLoop (Induction.cpp): differential unions: replace eager
     successor walk with VECTORAPPEND outAdd[T_union]; monotone unions
     unchanged (cut rule applies per succ).
  5. Sweeps: PublishDifferentialMessageVectors also emits COMMITSWEEP for
     every monotone boundary table (Seal); BuildInitProcedure end gets
     sweeps for tables its flows touch (differential Commit + monotone
     Seal) — check init proc structure first.
3b (the phase builder, new lib/ControlFlow/Build/Stratum.cpp, called from
BuildEntryProcedure between CompleteProcedure and
PublishDifferentialMessageVectors):
  Per differential table T in a single-view stratum, ascending stratum
  order, a SERIES:
    OVERDELETE: [for each rule chain with head-table T and delta at a
      lower table P: VECTORLOOP outDel(P) (or the join's removed section —
      joins: ONE dual-section TABLEJOIN per join view fed by pivot appends
      from ALL its preds' outDel+outAdd loops) -> plumbing walk ->
      UPDATECOUNT(-, kNonRecursive) on T, crossing -> append delQ[T]];
      VECTORUNIQUE delQ; VECTORLOOP delQ -> CLAIM(del,T) -> append D_T.
    INSERT: [+ seeds mirror, added section, UPDATECOUNT(+)]; VECTORUNIQUE
      addQ; loop -> CLAIM(add,T) -> append A_T.
    BUILDFRONTIERS: loop D_T -> CHECKMEMBER(kNetDeleted,T) -> append
      outDel(T); loop A_T -> CHECKMEMBER(kNetAdded,T) -> append outAdd(T).
  Seed sources: lower differential tables' outDel/outAdd; monotone
  boundary tables' outAdd (adds only). Join placement: the join's pivot
  vector gets appends inside each source loop (pivot cols only); the
  TABLEJOIN with sections sits after all its source loops in the stratum
  SERIES, before the claim drains of the tables its folds target. Plumbing
  walk after join/loop: per-node var mapping like MapVariablesInEagerRegion
  + CreateCompareRegion/CreateGeneratorCall for CMP/MAP; NEGATE: forward
  gate CHECKMEMBER(kInNew for +, kInI for -) on negated table ((d) does
  crossover); fold at FIRST table (same-model forwarding: already_added
  discipline).
  ORDERING within stratum: all seed enumeration FIRST (reads only lower
  strata, writes queues), then OVERDELETE drain, then INSERT drain, then
  BUILDFRONTIERS. (REDERIVE omitted: singleton strata cannot have C_r.)
Gate for (b): monotone subset byte-identical; two_hop_phantom, booleans_
diff, cond_*, compare_1, select_6 (etc non-recursive) green vs goldens +
oracle; recursive/negation-crossover cases stay red (tc_*, negate_*,
negation_flap, deep_chain_retract, merge_5, union_sibling_diff).

## 3b final spec (post-3a; 3a committed as fec8eeb)

Chain discovery (per source TABLE P over all member non-insert views with
full-width columns): dfs consumer edges, skipping inductive successors and
commit-published stream inserts; terminal = first table-backed view
(kHead, fold target) or first JOIN (kJoin). Plumbing between tables never
contains >1 join (FillDataModel persists every joined view; negate preds
persisted; negate views may be table-less mid-chain). No cycles among
non-inductive views.

Emission stratum: kHead branch -> owner stratum of head table (max member
view stratum); kJoin branch -> stratum of the join view. Reads on a branch
(join sides, negated tables) are all strictly lower strata; folds target
queues of tables at >= emission stratum (drained at their owner stratum).

Per stratum s ascending, one SERIES appended after CompleteProcedure,
before PublishDifferentialMessageVectors (so sweeps run last):
  [seeds] per branch-chain emitted at s, per sign (+: loop outAdd(P);
    -: loop outDel(P), differential non-inductive P only):
    VECTORLOOP binds source_member's columns; walk the path:
      per-view dispatch mirrors BuildEagerRegion:
      MapVariablesInEagerRegion (de-static) then:
      TUPLE/MERGE: continue; CMP: CreateCompareRegion (de-static);
      MAP: CreateGeneratorCall (de-static, bottom_up=true);
      NEGATE: CHECKMEMBER on negated table, ABSENT arm continues;
        predicate kInNew for + walks, kInI for - walks;
      JOIN terminal: append join.PivotColumns() values to the join's
        delta pivot vector (one shared vector both signs, kJoinPivots);
      head terminal (view with table != source's && != last folded):
        UPDATECOUNT(sign, kNonRecursive, false) + crossing VECTORAPPEND
        into addQ/delQ[T]; insert views fold InputColumns().
  [joins at s] per join view: VECTORUNIQUE(kSortAndUniquePivotVector) on
    its delta pivot vec; BuildJoin(for_delta=true): include unit sides as
    scan arms (point-probe Find of all-cols when index covers all), no
    body/cmp; added_body/removed_body = LETs -> walk continues per sign to
    the head terminal(s) downstream (plumbing only).
  [drains for tables owned by s] OVERDELETE: VECTORUNIQUE delQ; VECTORLOOP
    delQ -> CLAIM(del,T) -> VECTORAPPEND D_T. INSERT: same for addQ ->
    CLAIM(add,T) -> A_T. (REDERIVE omitted: singleton strata have no C_r.)
  [frontiers] loop D_T -> CHECKMEMBER(kNetDeleted,T) -> append outDel(T);
    loop A_T -> CHECKMEMBER(kNetAdded,T) -> append outAdd(T).
EMITTER ADDENDUM: EmitJoin's emit_section predicates must ALSO include
per-side pivot-equality (scanned indexed col fields == pivot exprs — the
sections' analogue of the body's approximate-index TUPLECMP recheck);
document on the section class comment that a "joined combination" means
scanned keys equal to the pivot.
Sources with no frontier vector yet (never created): TableDeltaVector
creates empty vectors on demand — loops over never-written vectors are
no-ops, harmless.

## HANDOFF STATE (2026-07-09, WIP commit after this section)

Committed parts: 1 (vocabulary, f94b23a), 2 (TABLEJOIN delta sections +
monotone watermark, 452c1d7), 3a (ingest netting/queue parking/boundary
frontiers, fec8eeb). This WIP commit adds part 3b: Stratum.cpp (762 lines,
the per-stratum seed/claim/frontier phase builder) + supporting refactors
(de-static MapVariablesInEagerRegion/CreateCompareRegion/
CreateGeneratorCall; BuildJoin delta variant incl. unit scan arms; EmitJoin
section pivot-equality). Built by a subagent to the "3b final spec" above;
it died at the verification stage — the code is UNREVIEWED beyond the spot
checks below.

Verified on this WIP:
- build clean; ctest Runtime + PointsTo pass.
- PASS: two_hop_phantom (the §5.1.1 phantom-pair sentinel!), booleans_diff,
  compare_1, select_6, cond_multi_setter.
- Monotone byte-identity holds for join_1, optimize_1.

KNOWN BROKEN (fix first):
- negate_2 (zero @differential, i.e. a MONOTONE negation case) FAILS and
  its generated code DIFFERS from pre-checkpoint — the 3b refactor leaked
  into the monotone negation path (suspects: Join.cpp delta-variant
  refactor, or a Negate/walk change). The monotone subset must go back to
  byte-identical.

NOT YET DONE for checkpoint (b):
- Full-suite tally (runall.sh) never completed on this tree.
- Review Stratum.cpp against the "3b final spec" (unreviewed agent code).
- Remaining expected-green candidates to verify: cond_both_polarities,
  cond_diff_flipflop, tower_of_compares, cf13_*/cf14_*/cf15_*/cf16_*,
  merge_5?, optimize_6, deep_chain_retract (reduced depth), union_sibling_
  diff — sort each into green / needs-(c) / needs-(d).
- Expected red until (c): tc_*, deep_chain_retract full depth, recursive
  cases; until (d): negate_* (differential ones), negation_flap.
- Then: checkpoint (c) (inductions -> D/R/I), (d) (negation crossover +
  evm_func_parse rewrite attempt), (e) (final deletion sweep + merge
  criteria incl. the single audited --bless for negation_flap +
  transitive_closure_diff).

This notes file is stage scratch: delete it from the repo when checkpoint
(b) lands (the durable record belongs in StackSafeNegation.plan.md).
