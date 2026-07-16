# ADL / functor-surface epoch — design ledger

Status: EPOCH IN PROGRESS, opened 2026-07-16 on branch `adl-functor-surface`
off main 88058879. This file is the epoch's working ledger, in the
DeltaRelationalIR.md mold: the PerfRoadmap §12.2/§12.3 seed's
re-verification record, the diff plan as amended, the hand-written
target artifacts index, and — as they land — the per-diff records. The
landing record goes to PerfRoadmap §13 at close. Owner theme decision:
§12.0(a) — P1 MAP-functor migration onto the C-5 free-function surface,
P2 eager-web externalization + kIngestFold, P3 R4 group_ids (gated on
the invariant-preservation argument), P4 residue.

## 0. Epoch-start baseline (2026-07-16, branch tip == main 88058879)

- debug + release builds green (cmake presets, C++23).
- ctest 3/3; runtime_test 43/43 (the §11-close count).
- FULL SUITE: PASS (164 cases), zero churn — the epoch-start baseline.
- Q5 spot baseline carried from §11 close: release/opt@128 149.4ms,
  debug/opt@128 925.1ms (regressions >10% stop the line; re-spotted
  per-diff on an otherwise-idle machine).
- P1 pre-registered driver-churn count (the §12.3 P1 prediction slot):
  13 files under tests/OptDiff/cases/ define a `DatabaseFunctors::`
  member out-of-line — algebra_invertible_1, average_weight, cf15_6,
  evm_func_parse (expected-diagnostic; its driver still compiles),
  fibonacci, fibonacci_iterative, force, map_1..map_5,
  pairwise_average_weight. No `DatabaseFunctors::` definitions exist
  anywhere else in the tree (tests/MiniDisassembler, tests/PointsTo,
  bin/, data/ all clean).

## 1. Seed re-verification record (§12.2/§12.3 vs HEAD; 2026-07-16)

Fleet: 5 opus derivation lanes (functor-delivery / eager-web / DR-spine
/ group_ids / emitted-artifacts), each re-deriving pseudocode FROM CODE
before reading the seed, each followed by an adversarial opus verifier
(10 agents, ~669k tokens). Verdict counts: 68 seed claims checked —
61 CONFIRMED, the rest below. THE PRECEDENT HELD AGAIN (E-18 is real).
Full lane reports + inventories: session scratchpad fleet1-*.txt; the
load-bearing extracts are §2/§3 inputs and are restated where used.

### Seed errata (E-18.. continuing the house numbering)

- E-18 (REAL-DEFECT, verifier-confirmed): §12.2(A):1316-1317 says
  new_weight_i32 "carries kAggregate/kSummary". FALSE — it is
  (bound,bound,free), a bbf @range @recompute merge functor
  (average_weight.dr:12); only sum_i32/count_i32 carry
  kAggregate/kSummary. The seed contradicts itself (line 1347 correctly
  shows the member new_weight_i32_bbf, which exists precisely BECAUSE
  the functor fails EmitFunctorsDecl's is_reduction test at
  Database.cpp:1183-1191). P1 consequence: the member→free migration
  MUST migrate new_weight_i32_bbf too — reduction-classification does
  not exclude it.
- E-19 (NUANCE, epoch-wide): the "ADL" characterization
  (§12.2(A):1329-1330/1345, §12.3 P1:1511 "« free function, ADL »") is
  wrong for the whole corpus: all args are builtin int32_t, which have
  NO associated namespaces — the reach is PURE ordinary unqualified
  lookup, bound at template-DEFINITION point (the generator's own
  comment, Database.cpp:1110-1112). Load-bearing implication for P1:
  the free forward-decls MUST be emitted before the detail template
  DEFINITIONS (two-phase lookup); the header assembly already
  guarantees this (EmitFunctorsDecl slot :2918 precedes detail defs
  :2930) — P1 emits its free decls at that same slot. (The epoch
  keeps its name; the mechanism is unqualified-lookup, not ADL.)
- E-20 (NUANCE): §12.2(A)'s threading list and §12.3 P1's threading
  diff omit the FORCED-QUERY entry point (EmitQueryFriends,
  Database.cpp:1465-1469), which threads `Functors &functors`
  identically (template header + name gated on uses_functors). P1's
  threading decision covers three entry-point families, not two:
  init :1327-1330, message :1350-1353, forced query :1465-1469.
- E-21 (NUANCE): new_weight_i32's member new_weight_i32_bbf is
  declared (datalog.h:228) and driver-defined (average_weight.main.cpp
  :43) but has NO emitted call site — the KV merge delivery rides the
  @recompute reduction path (new_weight_i32_reduce). The two-ABI
  witness is a DECLARATION-surface witness, not a dual-call-at-runtime
  one (EmitFunctorsDecl has no call-site gate, so P1 must still
  migrate the decl). Related: §12.2(A):1281 "MAP functors never carry
  [algebra pragmas]" is imprecise — the member surface never CONSULTS
  them, but a member-surface functor may carry them (new_weight_i32
  does).
- E-22 (P2-load-bearing census fact, verifier-found): the eager
  ingest-fold surface is NOT a single InTryInsert funnel.
  BuildEagerInsertRegion (Insert.cpp:9-80, dispatched from
  BuildEagerRegion Build.cpp:1043-1046) mints its OWN BuildUpdateCount
  fold (Insert.cpp:25-31, its own `table != last_table` guard), and
  branches on insert.IsStream() into a PUBLISH / publish-vec
  VECTORAPPEND (Insert.cpp:50-69) vs IsRelation() descent (:73-75).
  The P2 discovery object must inventory: build_explicit_loop's folds
  (Procedure.cpp:54-57), the InTryInsert funnel (Build.cpp:722-725),
  AND the INSERT-view fold + publication branch (Insert.cpp) — plus
  the three appends (add-queue :766-771, net-additions :888-893 —
  guard at :888 — and the explicit-loop appends :68-78).
- E-23 (the R4 design pivot, verifier-found; sharper than §12.2(C)):
  group_ids NEVER OUTLIVE ONE CSE() CALL. Population window is exactly
  [Optimize.cpp:305 RelabelGroupIDs, :378 ClearGroupIDs] per CSE
  invocation; Build.cpp:2551 clears them before optimization begins;
  after EVERY successful merge they are recomputed FROM SCRATCH
  (Optimize.cpp:372 → clear :421 + re-seed :423-425). The third writer
  the seed's census missed — CopyDifferentialAndGroupIdsTo
  (View.cpp:544-549, funneled through SubstituteAllUsesWith :575 from
  12 rewrite sites) — copies group_ids transiently inside CSE
  (immediately superseded by the :372 recompute) and copies EMPTY sets
  everywhere else (its live payload there is the differential flags).
  P3 consequence: R4's replacement distinguishing-state may be
  CSE-scoped-and-recomputed (today's shape — re-seed at the two
  refresh points) OR persistent-across-passes (then substitution-site
  propagation becomes an obligation); the argument must pick one and
  state it.
- E-24 (STALE-CITE, in-code — fix in the first P2 comment sweep):
  (a) DR.cpp:1645 still cites "Build.cpp:1002" for the eager negate
  gate (live: Build.cpp:1048-1051); the seed's own finding text
  misattributes it to both 1643/1645 — :1643 cites Negate.cpp:91-94,
  which is CORRECT. (b) DR.h:81-83 marks kStateFold/kStateEmit/
  kStateOld "R3 — reserved" but they are LIVE (BuildGroupUpdateOps
  emits them, DR.cpp:663/673/677/718). (c) DR.h:110-112 still says
  "STILL construct-alongside: no op is emitted" — false since R2
  family #3. (d) §12.2(B)'s ':886-893' net-additions cite: guard is
  :888, body :890-893.
- E-25 (P2 checklist correction): the R3 GROUP_UPDATE/STATE_SEAL
  family is NOT census-covered by ValidateDROps and has NO op-family
  structural validator — its only always-on guard is a plain assert
  (DR.cpp:629) stripped under NDEBUG. P2's kIngestFold must NOT copy
  that gap (census coverage + validators are part of its definition of
  done); adding the missing GROUP_UPDATE census is P4 residue work.
- E-26 (NUANCE): V-PRED-XCHECK's code site numbering is Site 1 =
  EmitChainStep negate gate, Site 2 = EmitJoinFire, Site 3 =
  EmitClaimDrain (the seed's reading order inverts it). The impure-MAP
  assert (Build.cpp:1016) is structurally unreachable for user input —
  the Program::Build pre-pass diagnostic (Build.cpp:1144-1149, then
  the :1163 nullopt return) dominates it.

### Verified facts the P1 design stands on (fleet-confirmed)

- The entry points ALWAYS emit `template <typename Log, typename
  Functors>` + a `Functors &` parameter — even for functor-free
  programs (only the param NAME is uses_functors-gated). Keeping the
  vestigial param therefore costs zero driver churn at init/message/
  forced-query call sites.
- uses_functors is set ONLY by a non-inline MAP GENERATE
  (Database.cpp:797-798), OR-propagated across calls (:812), consumed
  by DetailTemplateHeader (:264-273), DetailStateParams (:842-843),
  DetailStateArgs (:897-898), and the three entry-point families.
  GROUP_UPDATE never sets it. Post-P1 no body reads `functors` — the
  detail-threading arms of the machinery go dead (the entry-point
  params stay, per the recommendation).
- Header assembly order (Database.cpp:2916-2932): RowStructs →
  EmitStateCellStructs(2917) → EmitFunctorsDecl(2918) → Log(2919) →
  DetailDecls(2925) → Database(2927) → detail defs(2930). Free decls
  at the 2918 slot are visible to every detail body.
- The sole emitted member-call shape is Database.cpp:2736
  (`call = "functors." + name + "_" + BindingPattern`); the four
  range-shaped wrappers (:2769-2822) wrap the call expression and are
  ABI-agnostic (the migration touches only the callee expression).
  Inline functors (InlineName(kCxx), :2733-2734) bypass the member
  path entirely and are untouched by P1.
- Name-collision audit on both artifacts: free `add_i32_bbf` /
  `div_i32_bbf` / `new_weight_i32_bbf` collide with NO emitted global
  (messages, queries, Vec, log hooks, reduction free fns, Reduce_<id>/
  Key_<id> structs); the MAP suffix is a BindingPattern, the reduction
  suffixes are role names — disjoint by construction.
- map_1 witness scale: FOUR functors.add_i32_bbf call sites (datalog.h
  :195/:198/:201/:206 — one per emitted GENERATE, not per clause);
  average_weight: two functors.div_i32_bbf sites (:762/:773),
  new_weight_i32_bbf decl-only. The migration is mechanical over
  EmitGenerate, never a site list.

## 2. The diff plan as amended (checkpoint 2+3: designs + hand-written
## artifacts, each adversarially judged; 2026-07-16)

Workflow: 3 opus designers (P1/P2/P3), each delivering the diff plan +
the hand-written target artifact + predictions + gate proposal; 3 opus
judges; P2 additionally went through a full revision round against its
judge's findings. Verdicts: P1 REVISE→amended (3 factual fixes, no
CRITICAL), P2 REVISE→revision round (record below when re-judged),
P3 GO — with a DEFER recommendation that RETIRES the R4 charter.

### P1 — MAP-functor migration (artifact:
### ADLFunctorSurface.artifacts/p1-map1-target.md, the identity target)

The compiler diff is TWO emission surfaces and nothing else:
- EmitGenerate callee (Database.cpp:2736): drop the `"functors."`
  literal — the sole emitted member-call shape (grep-confirmed single
  hit). Inline branch, wrapper shapes, empty_body scaffolding untouched.
- EmitFunctorsDecl (Database.cpp:1162-1254): hoist the per-functor
  member declarations to FREE forward-decls emitted BEFORE the struct
  (mirroring EmitStateCellStructs :1113-1124); `struct DatabaseFunctors
  {};` SURVIVES EMPTY (driver/entry-point ABI stability); signature
  synthesis byte-identical, only the emission target moves; the dead
  new_weight_i32_bbf migrates too (E-18/E-21 — no use-gating added).
- uses_functors threading web: LEFT DEAD, NOT DELETED (the set-site
  keys on !InlineName, so MAP-bearing procs still thread an unread
  `Functors &functors`; keeps the header byte-identical outside the two
  migrated surfaces and all driver call sites unchanged). Clean
  excision = P4 residue gated on the WASM object-owned-state decision.
- All THREE entry-point families (init/message/forced-query, E-20)
  keep the vestigial templated Functors param. Zero driver-call churn.
- 13 corpus drivers drop `DatabaseFunctors::` from member definitions
  in the SAME diff. Judge corrections applied: evm_func_parse's driver
  is UNVERIFIABLE BY ANY GATE (expected-diagnostic; $CXX never invoked)
  — migrated for corpus consistency only; the functor-free byte-compare
  gate names deterministic SUITE cases (booleans.dr verified) + the four
  bench flagship .dr sources, NOT "tc_random as a case". The two-DB-in-
  one-TU question is MOOT (pre-existing: two namespace-free headers
  already collide on Database/DatabaseLog/Tup_i32 before any functor).
PRE-REGISTERED PREDICTIONS (P1): stdout golden churn ZERO (hard gate,
no --bless; a stdout diff is a bug); oracle/monotone churn ZERO; suite
stays 164; driver churn exactly the 13 files; functor-free programs
byte-identical headers; Q5 neutral (codegen string change only);
DR validators unchanged; ctest 3/3.

### P3 — R4 group_ids reshape: DEFER, charter RETIRED (artifact:
### ADLFunctorSurface.artifacts/p3-tc-selfjoin-target.md; judge GO)

THE HEADLINE (measured, then independently re-derived by the judge on
dual witnesses — recursive tc and non-recursive edge⋈edge): the seed's
premise "the distinctness IS the cubic materialization … value-keyed
cross of two SELECT scans" (§12.2(C):1478-1481) is FALSE at HEAD. The
self-join lowers to ONE physical table + TWO hash-keyed indexes
(Index::First = hash probe, Table.h:762-777) + a pivot loop — cost
O(join output), worst-case-optimal for a binary equi-join. The two
distinct proxy TUPLEs are a dataflow-representation artifact whose
lowering is already the self-index access path R4 was chartered to
introduce — which is moreover ALREADY FIRST-CLASS DR-IR vocabulary
(Lowering::kSectionWalk + PlanNode::bound_cols/pivot_col, DR.h:324-349).
The invariant-preservation argument (a)-(e) is discharged trivially by
the empty diff; Candidates A (self-pivot JOIN annotation) and B
(persistent distinguishing tag) were built and rejected — each rewrites
the epoch's only documented silent-miscompile guard for ZERO emission
change, and B's persistent fork newly incurs the E-23 substitution-
propagation obligation the seed's (a)-(e) omitted. The one genuine
residual: the provably-redundant pivot re-compare in the join inner
loop (elidable iff the ACCESS node is kSectionWalk with valid
pivot_col) — filed as P4-adjacent measure-first emission cleanup,
decoupled from group_ids. RECOMMENDATION TO OWNER (decision (c)):
DEFER and re-charter R4 as a materialization/access-path item gated on
a concrete WCOJ/3+-way self-join witness; re-label group_ids/
InsertSetsOverlap a correctness guard, never an optimization target.

### P2 — eager-web externalization (artifact:
### ADLFunctorSurface.artifacts/p2-ingest-inventory-target.md, revised;
### re-judge: R1e LANDABLE, cutover gated on one amendment)

The original design drew the seam as {seed folds externalized, descent
hand-coded} for all receives; its judge found that boundary is NOT a
tree cut (a monotone receive NESTS the descent — including the
INSERT-stream `publish` — inside the ingest fold's own if-crossed body,
verified on cf16_4's entry proc), the hoisted construction point read
the lazily-populated context.table_delta_vecs before the walk filled it,
and the DR band key's sign-major order inverts the emitted
ADD-before-REMOVE. The REVISION resolves all seven findings
(re-derived and confirmed by a second judge):
- SCOPE: stage 1 cuts over DELETION-CAPABLE receives ONLY (IF-2/IF-3 —
  build_explicit_loop×2, fold body = pure queue append, NO descent; a
  clean tree cut verified on nls). Monotone receives + the eager
  descent (IF-1/IF-4) stay hand-coded, deferred behind an explicit
  if-crossed-body HOLE CONTRACT (§6 of the artifact) with the
  interior-fold net-additions .dr witness as prerequisite (Build.cpp
  :870-893: the append may sit at an ANCESTOR fold of a deeper view —
  never assume the receive owns it).
- CONSTRUCTION: BuildIngestInventory runs INSIDE BuildStratumPhases
  after the eager walk (context.table_delta_vecs full); ONE
  DRFlowGraph — the censused flow IS the stashed context.dr_flow the
  lowering consumes (the LowerCommitSweeps precedent). Stage-1 queue
  roles derive from polarity alone; the monotone kNetAddition/kEmpty
  derivation is cross-checked against the walk-produced map on all 164
  (always-on abort).
- ORDERING: the R2 family-#1 lesson verbatim — LowerIngestFolds
  iterates query.IOs()×Receives()×(+before−) construction order (the
  lowering default); the lead-0 band Key is validator-only.
- VALIDATORS (E-25 honored): DERIVED-vs-DERIVED census in the
  ValidateDROps mold (expect(kIngestFold, exp_ingest) + order-free
  per-op key multiset recomputed from the Query) — NEVER a region-tree
  walk (no precedent; immune to the :716-719 body-swap); + effect-set
  totality, one-op-per-(message,receive,polarity), queue-role
  agreement (in DR VecRole terms; VecRole::kNetAddition singular ↔
  VectorKind::kNetAdditions plural mapped only at TableDeltaVector).
- V-PRED-XCHECK completion: Site 4 (EmitFrontierFilter) rides the
  cutover as an additive cross-check; the position-keyed site-2
  correlation stays deferred (an R4/self-join concern, retired with
  P3).
RE-JUDGE VERDICT: all original findings resolved; ONE new HIGH confined
to the CUTOVER stage — the splice mechanism (which parent region the
lowered folds attach to; the per-io `par` at Procedure.cpp:712 has no
Context handle and is EMPTY for a deletion-capable-only message once
:85-90 is deleted) must be specified + verified by a REGION-TREE
STRUCTURAL COMPARISON gate (not a next_id count) before the cutover is
authorized. R1e (construct-alongside: inventory + census + validators
+ cross-check + the E-24 comment sweep, zero emission change) is
LANDABLE AS-IS under the hard byte-identical gate.
PRE-REGISTERED PREDICTIONS (P2): R1e suite churn ZERO byte-identical;
cutover byte-identity the gated TARGET (fallback: permutation-only
bless, permcheck + oracle/monotone referees, never to green a red
case); suite stays 164 through stages 1-2 (both witnesses in-corpus);
FINDINGS entry (F23+) iff a census/cross-check fires on a real
divergence (the house bet); Q5 neutral.

## 3. Artifact index

- p1-map1-target.md — P1 identity target: the exact post-P1 datalog.h
  unified diff for map_1 + average_weight hunks + driver diffs + the
  contract analysis (§5) + the gate proposal (§6).
- p2-ingest-inventory-target.md — P2 seam: the IF-1..IF-4 receive
  classes, the stage-1 deletion-capable scope, the kIngestFold
  inventory with effect sets mapped to verbatim region-tree lines (nls
  + cf16_4 witnesses), staging + gates, the re-judge record (§11).
- p3-tc-selfjoin-target.md — P3/R4: today's dataflow + emission for
  tc⋈tc (measured), the three candidates, the DEFER argument, the
  retirement recommendation.
