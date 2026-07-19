# KeyedInstances §3 checkpoint — CONSOLIDATED as-landed record

Consolidator pass over the three derivation lanes + three adversarial
verifications, adjudicated against code read this session on branch
keyed-instances @ b577735e. Every disputed anchor below was re-read by the
consolidator (grep/sed cited inline); undisputed anchors carry the lane cite
plus the verifier's CONFIRMED verdict.

Adjudication rule applied: where a lane, its verifier, and the §3 seed
disagreed, the consolidator read the code line and ruled. Rulings appear as
`RULED:` lines. Errata (final numbers) are in §2; only real seed/artifact-vs-code
discrepancies get a number, stale anchors rolled up.

---

## (1) THE VERIFIED AS-LANDED PSEUDOCODE RECORD

### 1.A The determinism substrate — the "(F)" fix (§3(A) territory)

THE SINGLE PRIMITIVE — `det_seq`:
- `lib/DataFlow/Query.h:472` — `unsigned det_seq{~0u};` (~0u = unstamped
  sentinel). CONFIRMED (consolidator grep: field at 472).
- Public accessor `QueryView::DeterministicOrder()` — decl
  `include/drlojekyll/DataFlow/Query.h:434`, def `lib/DataFlow/Query.cpp:430-433`,
  asserts `impl->det_seq != ~0u`. CONFIRMED (verify-det-substrate PART 1).
- det_seq is a dense per-view sequence 0..N-1 = position in `ForEachView`
  (per-kind DefList insertion) order at the last stamp. Pointer-free by
  construction (DefLists are insertion-ordered vectors).

STAMP SITES — exactly TWO (this was the sharpest lane/verifier conflict):
- `lib/DataFlow/Induction.cpp:142-144` — head of `IdentifyInductions`
  (`next_det_seq=0; ForEachView(v -> v->det_seq = next_det_seq++)`).
- `lib/DataFlow/Optimize.cpp:285-287` — head of `CSE()`, identical pattern.
- RULED (consolidator grep `det_seq =` over lib/ include/): exactly TWO
  write sites (Induction.cpp:144, Optimize.cpp:287). The lane-det-substrate
  *structured summary / body prose* says "THREE stamp sites" — WRONG (it
  counts IdentifyInductions' re-entrant self-call at Induction.cpp:455, which
  re-executes line 144, as a distinct third entry; same code site re-run, not
  a third site). The SEED §3(A) says "TWO entries: CSE() head and
  IdentifyInductions head" — CORRECT. → erratum E-55 is against the LANE
  report, NOT the seed. IdentifyInductions IS re-entrant (recursive arg,
  Induction.cpp:129-134/455) so mid-pass-minted UNIONs are re-stamped before
  any ordered iteration — that mechanism is real, it is just not a third
  *site*.

THE COMPARATOR CHAIN — `OrderViewsDeterministically` (Induction.cpp:112-126),
a THREE-level TOTAL order:
1. structural hash (`a->Sort()` == `a->Hash()`, View.cpp:120-122) — pointer-free
   primary key.
2. first output column's id (`(*a->columns.begin())->id`) — at IdentifyInductions
   time this is the source-lexical VarId-derived id from BuildClause, BECAUSE
   the pass runs BEFORE FinalizeColumnIDs (load-bearing pass-order contract,
   comment-only + L3-backstopped).
3. `det_seq` — the guaranteed disambiguator; uniqueness asserted at
   Induction.cpp:124. CONFIRMED (verify-det-substrate PART 2, both lanes agree).
- `Sort()==Hash()` primary-key ingredients: `HashInit` (View.cpp:417-427) folds
  `HashCString(KindName())` (FNV-1a, prime 0x100000001b3) AND
  `can_receive_deletions`, `can_produce_deletions`, AND `columns.Size()`. All
  pointer-free. RULED: the SEED §3(A) phrase "FNV-1a over KindName CONTENT" is
  CORRECT-but-incomplete (also folds the two deletion flags + col count). NUANCE
  only, NOT an erratum — the primary-key/pointer-free claim stands; the lane
  report is the fuller statement. (Both verifiers independently reached this.)

THE CSE COMPARATOR CHAIN (Optimize.cpp) — the other det_seq consumer, its own
chain, final tie-break still det_seq:
- FillViews (409-422): stable_sort by Depth() (DefList insertion breaks ties).
- candidate sort within a color bucket (comment 320-322, sort 323-324): by
  det_seq — decides WHICH structurally-equal view survives.
- to_replace comparator (344-379): a_bad (matched-uphash first) → min-depth →
  v1.det_seq → v2.det_seq.
- group_order (294-301): color buckets iterated FIRST-SEEN over the
  FillViews-sorted `all_views`, NOT unordered_map bucket order.
  ALL CONFIRMED verbatim (verify-det-substrate PART 3; lane cite "320-324" for
  the candidate sort is de-minimis — comment 320-322/sort 323-324).

THE DEPTH SUBSTRATE (feeds work-list priorities):
- Join::Depth (Join.cpp:74-110): iterates input edges in OUTPUT-COLUMN LIST
  order (`for out_col : columns` then `out_to_in.find`), never out_to_in's
  pointer-bucket order.
- FinalizeDepths (Link.cpp:437-472): resets EVERY view's depth incl. dead ones
  over all 10 DefLists (451-460), then recomputes in fixed order. Called at
  Build.cpp:2602, immediately BEFORE FinalizeColumnIDs (2603).
  BOTH CONFIRMED (both verifiers).

CONTROLFLOW CONSUMERS — `OrderQueryViews`/`OrderedViewMap`:
- `lib/ControlFlow/Program.h:48` (`struct OrderQueryViews`) / `:55`
  (`using OrderedViewMap = std::map<QueryView, V, OrderQueryViews>`). RULED
  (consolidator grep): 48/55 confirmed. SEED §3(A) said "~46" — its own "~"
  approximate anchor; correct to 48/55. Rolled into E-56 (stale-anchor
  roll-up), low severity.
- The comparator uses `DeterministicOrder()` (det_seq). HOUSE-RULE comment
  (Program.h:42-47) warns QueryView's `operator<`, `UniqueId()`, wrapper
  `Hash()` are ALL impl-pointer-derived (Node.h:27-37) — a plain
  std::map<QueryView,...> varies run to run.
- FIVE OrderedViewMap induction consumers (all in ProgramInductionRegionImpl):
  `view_to_add_vec` (1832), `view_to_swap_vec` (1838), `view_to_output_vec`
  (1845), `output_cycles` (1851), `fixpoint_cycles` (1853). Iterating these in
  det_seq order fixes induction-vector id allocation + PARALLEL child order.
  Sibling `col_types_to_swap_vec` (1841) is std::string-keyed — benign.
  CONFIRMED (both verifiers).

THE TWO OrderViewsDeterministically-driven emission loops:
- injection-sites loop (Induction.cpp:404-440): source `injection_sites` is
  `std::set<VIEW*>` (167); copied + sorted (405-408); MINTS MERGE nodes (429)
  whose ids follow visit order. Single-site today; sort makes multi-site correct.
- merge-set labeling loop (Induction.cpp:578-604): source `merge_sets` is
  `unordered_map<VIEW*,MergeSet>` (147); keys copied + sorted (585-586); assigns
  `merge_set_id`/`group_id` + populates `related_merges`/`cyclic_views` (=
  InductiveSet(), walked in VectorFor id-allocation order). The `MergeSet`
  union-find itself is order-independent; only this labeling pass needs the sort.
  CONFIRMED; verify-det-substrate additionally checked the 8 OTHER raw
  merge_sets iterations (248/320/364/463/498/620/695/712) — all
  order-independent sinks or diagnostic-only. BENIGN.

QUERYVIEWIMPL SCALAR FIELDS (for the naming-key decision, §4.a below): RULED
(consolidator grep) — `merge_set_id{0}` at 245, `det_seq{~0u}` at 472,
`group_id{0u}` at 475, `depth{0U}` at 478, `color{0}` at 529. There is NO
integer view-id field. lane-det-substrate §9 cited `color` at "268" — WRONG,
it is 529 (rolled into E-56). The `unsigned id` at Query.h:115 is a COLUMN
field (QueryColumnImpl). View identity at the wrapper is the impl POINTER
(`UniqueId()` = reinterpret_cast<uintptr_t>(impl)).

NO FinalizeViewIDs PASS: RULED — grep `FinalizeView|RenumberView|view->id=`
over lib/DataFlow returns nothing (both lane + verifier confirmed). Columns DO
get a finalized renumber (`FinalizeColumnIDs`, Columns.cpp:13-25, Build.cpp:2603,
ForEachView order); views do NOT. det_seq is the only pointer-free, run-stable,
TOTAL per-view id — but it is MUTABLE / re-stamped, not a frozen post-optimize id.

THE EXISTING GraphViz (-dot-out) DUMP IS NON-DETERMINISTIC IN NODE NAMING:
lib/DataFlow/Format.cpp:98-198 names every VIEW/TABLE node `v<UniqueId()>` /
`t<UniqueId()>` = the raw impl pointer (Node.h:31). The (F) substrate fixed
emission ORDERING but did NOT touch this dump's view NAMING. NUANCE (both
verifiers): column PORTS use `col.Id()` (Format.cpp:186,207) = the finalized
deterministic column id, so ONLY node NAMES are pointer-derived. This is the
core §4.a decision-feeding fact: a deterministic -df-out cannot copy the DOT
dump's naming.

### 1.B The dump-adjacent surfaces + full DeltaRel inventory (§3(B)/(C) territory)

EXISTING OUTPUT-FLAG WIRING (bin/drlojekyll/Main.cpp):
- Global stream pointers (Main.cpp:54-56): `gDOTStream` (-dot-out),
  `gDRStream` (-dr-out amalgamated .dr source), `gIRStream` (-ir-out ControlFlow
  .ir). `-cpp-out` is a DIRECTORY string `gCxxOutDir` (Main.cpp:52), not a stream.
- Arg-parse arms: -cpp-out 261-269, -ir-out 271-283, -dr-out 287-300, -dot-out
  303-316; each opens a `hyde::FileStream` (Main.cpp:35-42) held in a unique_ptr
  local (253-255) and points g*Stream at its `.os`. Help text 180-194 (OUTPUT
  OPTIONS 180-186). ALL CONFIRMED EXACT (verify-dump-surfaces A1).
- DRAIN ORDER (load-bearing, Main.cpp:103-105 NOTE comment):
  - `-dr-out` drains in ProcessModule at 125-129, BEFORE CompileModule (166) —
    a PARSE-level dump (`<< module`, SetRenameLocals(true)).
  - `-ir-out` drains `*program_opt` at 74-77, right after Program::Build.
  - `-dot-out` drains `*query_opt` LAST at 106-109, AFTER Program::Build,
    "because ... we break abstraction layers ... to annotate the data flow IR
    with table IDs." So `QueryView::TableId()` is populated ONLY by the
    Program::Build side-effect. CONFIRMED.

WHERE -df-out / -deltarel-out WOULD SLOT (both lanes + verifiers agree):
- -df-out (DataFlow textual): drain inside CompileModule next to the DOT drain
  (Main.cpp:106-109) if it wants TableId annotation (post-Program), or right
  after Query::Build (64) for pre-table-id state. Streams `*query_opt`.
- -deltarel-out: THE HARD SEAM (loud finding, CONFIRMED). The DRFlowGraph is
  NEVER exposed on Program/ProgramImpl — it lives only as `context.dr_flow`
  (`std::shared_ptr<DRFlowGraph>`, Build.h:198, consolidator grep confirmed)
  inside BuildStratumPhases. grep for dr_flow/DRFlowGraph/DeltaRelFlow in
  Program.h + include/drlojekyll/ControlFlow/ = NO HITS. So a Main.cpp flag
  needs EITHER (a) a global-gated dump call planted at the validate-exit inside
  Stratum.cpp, OR (b) a new Program accessor publishing dr_flow. Shape (a) is
  the only one that works without new plumbing today.

THE DeltaRel INVENTORY (binding deliverable — internal to lib/DeltaRel, NO
include/drlojekyll surface). All enum spellings + line cites RE-READ by the
consolidator (sed of DeltaRel.h:73-138) and CONFIRMED against BOTH lanes:

- `DROpKind : uint8_t` (DeltaRel.h:113-138), 15 enumerators verbatim:
  kCrossover, kProductArm, kSeedFold, kFixpointFire, kChainFold, kClaimDrain,
  kRetire, kRederive, kFrontierFilter, kCommitSweep, kNegateGate, kPivotAssemble,
  kIngestFold, kGroupUpdate, kStateSeal.
- `Pred : uint8_t` (DeltaRel.h:94-105), THE TEN membership predicates in enum
  order: kPresent, kInI, kInNew, kSurvivesSoFar, kAliveAtClaim,
  kInNewWithFrontier, kInNewSansFrontier, kRecursivelySupported, kNetDeleted,
  kNetAdded. (The build layer's `hyde::MembershipPredicate` in
  ControlFlow/Build/Program.h has the same ten in a DIFFERENT order —
  DeltaRel.h:88-93 — a dump must print `Pred` spellings.)
- `EffKind : uint8_t` (DeltaRel.h:73-84), 10: kVecAppend, kVecDrain, kVecClear,
  kCounter, kFlagRead, kFlagWrite, kInIReadFrozen, kStateFold, kStateEmit,
  kStateOld (the three kState* LIVE since R3 stage-C).
- `VecRole` (52-67, 14): kParam, kNetRemoval, kNetAddition, kDeleteQueue,
  kAddQueue, kOverdeleteSet, kAdditionSet, kClaimedDel, kClaimedAdd, kJoinPivots,
  kProductInput, kTableScan, kMessageOutput, kEmpty.
- Other enums (all uint8_t, all EXACT per verify-dump-surfaces A2): ElementShape
  (47: kValues/kIds/kIdCols), UniqueContract (70), Ctx (87: kEager/kSeed/
  kFixpoint), AggProvenance (143: kOver/kKv), Algebra (148: kInvertible/
  kRecompute), NegateHint (153: kNormal/kNever), ClaimForm (166), Deferral (171),
  SweepFlavor (176: kDifferential/kMonotone), ClaimGate (186-189:
  kDelGateCnrNonPositive/kAddGateTotalPositive — a DEDICATED vocab, NOT a Pred),
  PlanKind (325: kAccess/kGate/kFold), Lowering (330: kPointTest/kSectionWalk/
  kFullScan/kSeek[D5-reserved]), DepKind (605: kRAW/kWAR/kWAW), DepScope (606:
  kEpoch/kRound), RoundPhase (565: kOverdelete/kInsert). DerivClass imported
  from Runtime/Table.h (DeltaRel.h:24).

STRUCT FIELDS (all field/type/default/line EXACT per verify-dump-surfaces A2):
- DREffect (197-226, tagged union over EffKind; kind default kFlagRead).
- DRVec (245-260): shape/role/uniq/debug_table/debug_kind, `defs`/`uses` are OP
  INDICES into DRFlowGraph::ops. NO id field — identity == vector position.
- DRTable (267-272), DRBranch (287-293), DRJoin (302-309, pivot_vec indexes vecs).
- PlanNode (332-359, the access-plan spine, ≤1 child), DRArm (367-373).
- DROp (383-552): NO id field (starts at kind 385); common fields effects(388)/
  ctx(391)/seed_before_drain(396); per-kind payload blocks CROSSOVER(398-410),
  PRODUCT(412-422), SEED_FOLD(424-438), FIXPOINT_FIRE(440-446), CHAIN_FOLD
  (448-455), table-family(457-478), PIVOT_ASSEMBLE(480-490), NEGATE_GATE
  (492-500), INGEST_FOLD(502-521), GROUP_UPDATE/STATE_SEAL(523-544); arms(549).
- DRRound (567-596): scc_group/phase/drain_stratum/test_vecs, plus body_ops(590)/
  output_ops(595) which are "populated, never read" R2+ SUBSTRATE (header comment
  584-595) — dead-but-alive, dump CAN render them but lowering ignores them today.
- DRDep (608-614), DRStateCell (628-642).
- DRFlowGraph (644-697) — the OWNER a dump reads: ORDERED vectors vecs(646)/
  statecells(647)/tables(650, R2+ dead-but-alive)/ops(651)/rounds(656)/
  dep_edges(657)/pinned_order(658)/branches(661)/joins(662); UNORDERED maps
  table_vecs(668-669)/scc_map(673)/branch_stratum(680)/join_stratum(681)/
  crossover_stratum(682)/product_stratum(683)/group_update_stratum(684)/
  drain_stratum(685). Accessors Crossovers/ProductArms/GroupUpdates/StateSeals/
  OpsOfKind/TableVec (688-696).

OP/VEC ID SCHEME (dump numbering): NO id field on DROp/DRVec — identity IS the
dense vector index, minted `op_idx = flow.ops.size(); flow.ops.push_back(...)`
(witnesses DeltaRel.cpp:742-743, 959-961, 977-979, 1037-1044, 1314-1318,
1753-1758, 1838-1843; vec mint 1166). Deterministic + stable within a build.
CAUTION (both lanes): a dump MUST iterate the ORDERED vectors and only LOOK UP
the unordered maps by key — iterating table_vecs/scc_map/*_stratum directly is
nondeterministic. Fragility note: numbering is tied to construction order; the
byte-identity golden gate already guards this.

LINEARIZED OP LIST survives to a post-validate hook: `pinned_order`
(DeltaRel.h:658) is the epoch-scope linearized op-index list, filled by
`LinearizeAndValidateDRFlow` (DeltaRel.cpp:3131). Because the whole DRFlowGraph
is std::move'd onto context.dr_flow AFTER validation, pinned_order/dep_edges/all
strata survive. A dump keyed on pinned_order renders emission order; keyed on
ops-index order renders construction order — both deterministic.

CENSUS re-emit at dump time: FEASIBLE trivially. The census is NOT a stored
object — recomputed inside ValidateDROps as local `exp_*` counters vs a
`count_kind()` lambda looping flow.ops (DeltaRel.cpp:2854-2879). A dump
reproduces the DERIVED half (per-kind counts) with the identical O(|ops|) loop,
no discovery inputs. The GU key census uses `(agg_table, provenance, algebra,
view.UniqueId())` order-free (2834-2851); statecell_id deliberately NOT keyed
(mint-order artifact, E-28). CONFIRMED (verify-dump-surfaces A6).

THE VALIDATE-EXIT HOOK POINT (consolidator RE-READ Stratum.cpp:2081-2168):
BuildStratumPhases (Stratum.cpp:2049). Sequence: BuildDRInventory(2081) →
DeriveDRStrata(2088) → ValidateDRInventory(2095) → ValidateDROps(2096) →
LinearizeAndValidateDRFlow(2097) → V-PRED-XCHECK/INGEST Site 5 block(2099ff) →
stash `context.dr_flow = make_shared(std::move(dr_flow))` (2166) →
`const DRFlowGraph &flow = *context.dr_flow` (2167). Lowering
(LowerDRFlow/LowerDRRounds) happens LATER (~2298ff) and does not mutate the
inventory. THE DUMP HOOK POINT is immediately after 2097 (all always-on
validators run, pinned_order/dep_edges populated) OR after 2166 (also past the
ingest cross-check, stable shared_ptr handle). CONFIRMED EXACT (both lane +
verifier).

THE .ir NAMING CONVENTIONS a new dump should harmonize with (ControlFlow/
Format.cpp): `%col:<id>` (66), `%index:<id>[…]` (71), `%table:<id>[…]` (93),
`$<kind>:<id>` DataVector (104-142, the `$param`/`$induction_*`/`$overdelete`/…
family), `@<name>:<id>` DataVariable (144), `^<kind>:<id>` ProgramProcedure
(972). A DeltaRel dump can reuse `%table:<id>` (same DataTable::Id() the .ir
uses — a DRVec.debug_table / DROp.*_table is a TABLE* whose DataTable(t).Id()
IS the .ir id) to CROSS-REFERENCE .ir tables. CONFIRMED (verify-dump-surfaces A7).

### 1.C The harness mechanics (§3(D) territory)

DIRECTORY CENSUS (all CONFIRMED by verify-harness Part 1 shell recount):
cases/ = 169 `.dr`, 167 `.main.cpp` (nonascii_1, truncated_decl_1 lack one),
52 `.batches`, 2 `.drflags` (demand_tc_witness, demand_multi_adorn_1). goldens/
= 158 plain `.stdout` + 52 `.oracle.stdout` + 52 `.monotone.stdout` = 262.
169 = 158 plain-golden + 11 always-diagnostic.

diffrun.sh (`<case.dr> <driver.cpp> <workdir>`): a PURE 4-mode
compile/build/run/compare primitive.
- Modes `opt/nodf/nocf/none` (diffrun.sh:45-51). Env: DR required (no default),
  CXX=clang++, TIMEOUT=60.
- `.drflags` sidecar append (53-59): CONTENTS appended to `$flags` for EVERY
  mode (NOT a 5th mode — same 4 iterations). This is diffrun.sh's ONLY sidecar
  awareness — a flag-APPEND, NOT a compare arm.
- Per-mode: compile (65-72, `$DR $CASE $flags -cpp-out $out`), build (74-80),
  run (82-88), compare vs `goldens/$NAME.stdout` via `cmp -s` (90-98).
- diffrun.sh has ZERO `.batches`/oracle/monotone knowledge (CONFIRMED, full
  read: no such string). Per-mode workdir `$WORK/$NAME.$mode/`.

runall.sh (289L):
- `--bless` branch (67-103, checked FIRST so DR not required): iterates
  `$WORKROOT/*/` (NESTED layout), copies `$d$name.opt/stdout` →
  goldens/$name.stdout, plus `.oracle`/`.monotone` blocks (81-92); THEN a
  HARDCODED flat-layout exception for kvindex_1 (94-100). --bless is the SOLE
  write path into goldens/.
- `--one <name> <workroot>` worker (115-250): owns ALL golden-comparison policy.
  Helpers: `flags_of(mode)` (mode→flags + .drflags append, DUPLICATED from
  diffrun.sh), `expect_diagnostic(mode)` (flat out, requires exit code EXACTLY 1
  — `rc -ne 1` fails, catching crashes too; NO C++ build), `run_vs_golden(mode)`
  (kvindex_1 only), `run_oracle()` (guarded by `cases/$NAME.batches` existence;
  oracle+monotone steps against `.oracle.stdout`/`.monotone.stdout`; NESTED
  `$WORKROOT/$NAME/$NAME.oracle`; .drflags NEVER passed to the oracle).
- Dispatch (229-249): the 11 always-diagnostic names run expect_diagnostic in
  all 4 modes; kvindex_1 is MODE-SPLIT (opt/nocf golden, nodf/none diagnostic);
  `*)` → diffrun.sh. `run_oracle || st=1` runs UNCONDITIONALLY after the esac
  (248).
- Parallel driver (252-289): xargs `-P $JOBS` self-reinvokes `runall.sh --one`;
  summary grep `FAIL|DIVERGE|EXPECT-ERROR|MISSING` (284) → SUITE PASS/FAIL.
  ALL CONFIRMED verbatim (verify-harness Part 2, all cited anchors accurate).

THE 11 ALWAYS-DIAGNOSTIC NAMES (runall.sh:231, verbatim): kvindex_2, kvindex_3,
kvindex_4, agg_in_scc_1, kv_in_scc_1, algebra_dup_1, algebra_conflict_1,
evm_func_parse, nonascii_1, truncated_decl_1, demand_multi_adorn_1.

permcheck.py: a STANDALONE order-free per-epoch delta-token permutation checker,
NEVER invoked by diffrun.sh or runall.sh (repo-wide grep = self-only). Scoped to
the product_diff/conds/self/mixed driver family. Strict-order structural lines +
multiset-equal `+(...)`/`-(...)` delta tokens per segment. NOTE for T3: the seed
says IR goldens are strict byte-compare (an IR permutation is exactly what they
catch) — permcheck stays a stdout-token referee, not an IR referee.

WORKDIR LAYOUT is inconsistent: diffrun.sh-driven cases →
`$WORKROOT/$NAME/$NAME.<mode>/` (NESTED, since --one passes `$WORKROOT/$NAME` as
diffrun's WORK arg); diagnostic-only + kvindex_1 → `$WORKROOT/$NAME.<mode>/`
(FLAT); the oracle step is NESTED for all kinds. --bless has the separate
hardcoded kvindex_1 flat block (94-100). CONFIRMED (both).

bash 3.2 CONSTRAINT: neither script uses `declare -A` (case/esac mode mapping);
any T3 extension must comply.

HARNESS EXTENSION POINTS for a `.dfgold`/`.deltarelgold` sidecar class (the T3
hook): the precedent-matching placement is a NEW helper in runall.sh's `--one`
worker parallel to `run_oracle()` (guarded by sidecar existence early-return,
invoked `run_dfgold || st=1` alongside 248), NOT in diffrun.sh. A new arm MUST
reuse a `FAIL|DIVERGE|EXPECT-ERROR|MISSING` substring in its verdicts or the
summary grep (284) silently misses divergences — the single sharpest trap.
Golden naming: dot-separated suffix `.df.stdout`/`.deltarel.stdout` (NOT `_df` —
underscore collides with case names). Mode-pinning: plain golden is already
implicitly opt-mode-sourced (runall.sh:75).

---

## (2) ERRATA — final numbers (adjudicated, REAL discrepancies only)

Note on continuity: all three verify lanes independently used tentative
"E-55/E-56/E-57" numbers for DIFFERENT findings. The consolidator assigns final
GLOBAL numbers E-55..E-60 below, disambiguating. The prior epoch ledger closed
at E-54; these continue it. IMPORTANT: most §3 SEED claims verified CLEAN — the
seed's determinism-substrate and dump-surface blocks broke the E-1..E-54
"every seed has a defect" streak. The errata below are dominated by LANE-REPORT
and cross-ARTIFACT discrepancies, not seed defects.

E-55 (LANE-REPORT overcount — against lane-det-substrate, NOT the seed).
  CLAIM: lane §1 body + structured summary say det_seq has "THREE stamp sites".
  REALITY: exactly TWO write sites — `v->det_seq = next_det_seq++` at
  Induction.cpp:144 and Optimize.cpp:287 (consolidator grep `det_seq =` over
  lib/ include/). The lane counts IdentifyInductions' re-entrant self-call
  (Induction.cpp:455, re-runs line 144) as a distinct third entry — same site
  re-run. EVIDENCE: Induction.cpp:142-144, Optimize.cpp:285-287.
  MISDIRECTS: a reader looking for a third stamp injection point. The SEED
  §3(A) "TWO entries" is CORRECT — do not "fix" the seed.

E-56 (STALE-ANCHOR ROLL-UP — minor, mixed seed + lane, none load-bearing).
  Three cosmetic anchor slips, each right-field/right-mechanism wrong-line:
  (i) lane-det-substrate §9 anchors the `color` field at Query.h:268 — REALITY
      Query.h:529 (consolidator grep). EVIDENCE: Query.h:529.
  (ii) SEED §3(A) cites OrderQueryViews/OrderedViewMap "Program.h :~46" —
      REALITY :48 (struct) / :55 (using) (consolidator grep). The seed's own
      "~" flags it approximate. EVIDENCE: Program.h:48,55.
  (iii) both lane reports list FinalizeColumnIDs(2603) before FinalizeDepths
      (2602) in prose — reversed LISTING order; both individual cites CORRECT
      (FinalizeDepths runs first at 2602). EVIDENCE: Build.cpp:2602-2603.
  MISDIRECTS: nothing load-bearing; navigation only.

E-57 (DESIGN TENSION — the -df-out block-id source; not a code defect but an
  UNRESOLVED owner decision the design round must settle).
  CLAIM: lane-dump-surfaces §4/§3 recommends -df-out SORT views by
  `QueryView::UniqueId()`; SEED §3(C) + ir-dump-formats.md §1 name
  `det_seq`/`DeterministicOrder()` as the `^kind.<id>` block-id source.
  REALITY: both are deterministic-by-construction post-build but yield DIFFERENT
  orderings AND UniqueId() is POINTER-DERIVED (stable within a build, NOT
  portable across builds — Node.h:31; the §3(A) HOUSE RULE (2) explicitly warns
  this). det_seq is a dense small int, pointer-free. EVIDENCE: Node.h:31,
  Program.h:42-47 (house-rule caution), Query.h:472 (det_seq).
  MISDIRECTS: an implementer following the lane's "sort by UniqueId" literally
  would build a dump whose block ids are NON-portable across builds — mild
  tension with the whole (F) determinism discipline. RULING: the lane is RIGHT
  that container order is wrong, but the determinism-conscious SORT KEY is
  det_seq (or a fresh FinalizeColumnIDs-style renumber), NOT raw UniqueId. The
  seed explicitly DEFERS this ("decide det_seq vs a renumber at design"); pick
  det_seq (or renumber) and reject raw UniqueId.

E-58 (SEED IMPRECISION — §3(D) harness placement; load-bearing for the T3
  implementer).
  CLAIM: SEED §3(D) (KeyedInstances.md:430): "runall.sh/diffrun.sh grow the
  compare arms" paired with "the .batches/.drflags idiom".
  REALITY: the `.batches` idiom the seed invokes grows runall.sh's `--one`
  worker ONLY (run_oracle defined + called entirely within runall.sh:180-248);
  diffrun.sh has NO comparison side-arm at all (full read) — its sole sidecar
  awareness is the `.drflags` flag-APPEND (diffrun.sh:57-59), which is not a
  compare arm. EVIDENCE: runall.sh:180-227,248; diffrun.sh:57-59.
  MISDIRECTS: the "diffrun.sh" half of the placement claim contradicts its own
  cited precedent. RULING: the safe, precedent-matching, clean-layering
  placement for the new compare arm is runall.sh's `--one` worker ONLY.

E-59 (CROSS-ARTIFACT INCONSISTENCY — ir-dump-formats.md §2.5 vs epoch-diffs T3
  vs the code precedent; the harness-hook placement).
  CLAIM: ir-dump-formats.md:126-129 (§2.5) places the new `.dfgold` compare arm
  in DIFFRUN.SH ("diffrun.sh additionally runs the compiler with -df-out");
  epoch-diffs T3 puts it in "the harness ... via runall.sh --bless" (unpinned);
  §3(D) says both.
  REALITY: the sole existing per-case golden-side-arm (`.batches`→run_oracle) is
  100% in runall.sh's --one worker; diffrun.sh is a pure primitive. EVIDENCE:
  ir-dump-formats.md:126-129 vs runall.sh:180-227,248 vs diffrun.sh (no side-arm).
  MISDIRECTS: §2.5 is the OUTLIER most at odds with precedent — a T3 implementer
  following it literally breaks the clean layering (diffrun.sh = pure 4-mode
  primitive; runall.sh --one = all golden-comparison policy). RULING: §2.5's
  diffrun.sh placement must be corrected to runall.sh's --one worker before T3.

E-60 (STALE DOC — the corpus count, gate-adjacent).
  CLAIM: CLAUDE.md:51 "168 corner-case programs as of the demand-seeds epoch";
  epoch-diffs T3 also predicts "suite 168".
  REALITY: live checkout has 169 `.dr` (verify-harness recount; +symrec_tie_1
  per §1 ledger "SUITE 168 → 169"). EVIDENCE: CLAUDE.md:51 vs `ls cases/*.dr`=169.
  MISDIRECTS: a byte-exact count in gate-adjacent docs is off by one. Low
  severity (documentation drift). NOTE: the earlier §3(A) tripwire text and the
  ledger already record symrec_tie_1 as the +1 case, so this is pure doc lag.

NON-ERRATA NUANCES (recorded, no number, per adjudication):
- HashInit "FNV-1a over KindName CONTENT" (seed §3(A)) is CORRECT-but-incomplete
  (also folds can_receive_deletions/can_produce_deletions/columns.Size(),
  View.cpp:417-427). Pointer-free claim stands. Not a defect.
- GraphViz -dot-out node NAMING by UniqueId() (pointer) is non-deterministic,
  but its column PORTS use the deterministic col.Id() — only NODE names are
  pointer-derived (Format.cpp:186/207). Lane's summary statement is accurate.

---

## (3) ir-dump-formats.md §2 RECONCILIATION (what the -deltarel-out draft needs)

ir-dump-formats.md §2 is a DRAFT written "from memory of the CLAUDE.md summary"
(artifact:10-13) and self-flags "reconcile against the drir lane report before
coding" (artifact:116-118). The consolidator RE-READ the DeltaRel.h enums; the
draft's op-kind/attribute vocabulary is SUBSTANTIALLY correct in spirit but the
draft's illustrative line-format is a proposal, not code-anchored. Corrections
the draft needs to match DeltaRel reality:

3.1 OP-KIND SPELLINGS: the draft's `op.<id> <KIND>` line should print the
   ACTUAL DROpKind spellings (DeltaRel.h:113-138): kCrossover, kProductArm,
   kSeedFold, kFixpointFire, kChainFold, kClaimDrain, kRetire, kRederive,
   kFrontierFilter, kCommitSweep, kNegateGate, kPivotAssemble, kIngestFold,
   kGroupUpdate, kStateSeal — 15 kinds. The draft does not enumerate them (it
   was memory-sourced); the dump must render one of these 15 verbatim.

3.2 MEMBERSHIP PREDICATES (the draft's `reads:` line): the ten-predicate
   vocabulary is `Pred` (DeltaRel.h:94-105): kPresent, kInI, kInNew,
   kSurvivesSoFar, kAliveAtClaim, kInNewWithFrontier, kInNewSansFrontier,
   kRecursivelySupported, kNetDeleted, kNetAdded. Print `Pred` spellings, NOT
   the build-layer `MembershipPredicate` (same ten, DIFFERENT order,
   DeltaRel.h:88-93). A NEGATE_GATE op's `gate_pred` equals
   `NegateGatePred(ctx,hint)` (decl DeltaRel.h:161, def DeltaRel.cpp:535) — the
   dump prints the stored `gate_pred`.

3.3 EFFECT VOCABULARY (the draft's `writes/effects:` line): the effect set is a
   vector of `DREffect` tagged on `EffKind` (DeltaRel.h:73-84): kVecAppend,
   kVecDrain, kVecClear, kCounter, kFlagRead, kFlagWrite, kInIReadFrozen,
   kStateFold, kStateEmit, kStateOld. Only the EffKind-selected fields are
   meaningful per effect (tagged union). The draft's freeform "{<effect set>}"
   should render each effect as its EffKind spelling + the live field(s)
   (e.g. kCounter → counter_table + sign + klass; kFlagRead → read_table +
   pred + ctx).

3.4 SIGN/POSITION/CLAIM ATTRIBUTES (the draft's `sign=`/`pos=`/`claim=`):
   - `sign` is a per-op/per-arm `int` (+1/-1); render `+`/`-` (the draft's
     `·` for "no sign" applies to per-table families that carry no arm sign).
   - `pos=<InNew|InI|...>` maps to `Pred` values (InNew/InI are Pred members) —
     these are the position-keyed non-delta reads; render the Pred spelling.
   - `claim=<ctx>` maps to `Ctx` (kEager/kSeed/kFixpoint, DeltaRel.h:87); the
     op carries `ctx` at DeltaRel.h:391. Also relevant: `ClaimForm`
     (kSinglePass/kInRound) and `ClaimGate` (kDelGateCnrNonPositive/
     kAddGateTotalPositive — a DEDICATED vocab, NOT a Pred).

3.5 THE `spine:` LINE (access-plan): a `PlanNode` tree (DeltaRel.h:332-359), ≤1
   child per node (a spine, not a tree-branch). Render PlanKind (kAccess/kGate/
   kFold) + table/bound_cols/pred/absent/lowering/ctx/pivot_col/fold_* per node
   down the `child` chain.

3.6 VEC LINE (`vec $<role>.<id> ...`): DRVec has NO id field — id == vecs index.
   `role` is `VecRole` (14 spellings, DeltaRel.h:52-67). `def=[op ids]`/
   `use=[op ids]` are the `defs`/`uses` op-index vectors (DeltaRel.h:258-259).
   `carried=<role>` in the draft is not a DRVec field — loop-carriage lives on
   `DRDep.loop_carried` (DeltaRel.h:613), a DEP-edge attribute, not a vec attr;
   the draft should move carriage rendering to the dep-edge section or drop it.

3.7 OPS IN CHECKED-LINEARIZATION ORDER: the draft is CORRECT — reuse
   `pinned_order` (DeltaRel.h:658, filled by LinearizeAndValidateDRFlow), NEVER
   re-sort. Vecs/branches/joins/rounds iterate their OWN ordered vectors; only
   the unordered maps (table_vecs/scc_map/*_stratum) are looked up by key.

3.8 CENSUS RE-EMIT: the draft's `census: <counts as emitted>` line is FEASIBLE
   — re-run the O(|ops|) count_kind loop (DeltaRel.cpp:2854-2879); no persisted
   census struct, no discovery inputs needed. The DERIVED half is trivial.

3.9 VALIDATE-EXIT HOOK: the draft's "emitted from the checked model AFTER
   validation" is CORRECT — the concrete point is immediately after
   Stratum.cpp:2097 (validators + linearizer done) or after 2166 (also past the
   ingest cross-check, with the stable shared_ptr). The draft's abstract
   statement should be pinned to these lines.

3.10 BAND STRUCTURE (the draft's `stratum <k> band=<band-key>`): there is NO
   first-class Band object. Bands are IMPLICIT three ways: (i) DRRound.phase
   (kOverdelete/kInsert) + the intra-round body_ops template order; (ii) epoch
   `pinned_order` under the band-key comparator (the band key is a comparator
   used by the linearizer, NOT a stored per-op field — the draft's
   `band=<band-key>` cannot read a stored field, it must recompute or drop it);
   (iii) the `*_stratum` maps + branch_stratum (DeriveDRStrata). Render "bands"
   by grouping on the stratum lookup + body_ops order + pinned_order.

3.11 DEAD-BUT-ALIVE SUBSTRATE surfaced by the dump: DRRound.body_ops/output_ops
   (DeltaRel.h:584-595) and DRFlowGraph.tables (648-650) are "populated, never
   read" R2+ substrate — the dump CAN render them (they carry real pinned order
   / table models) but a reviewer should know lowering IGNORES them today.

---

## (4) DECISION-FEEDING FACTS for the owner decisions

### 4.a -df-out block-id choice — what id spaces actually exist post-optimize
- FINALIZED VIEW RENUMBER: does NOT exist. No FinalizeViewIDs/RenumberView pass
  (grep empty); QueryViewImpl has no integer view-id field.
- det_seq (Query.h:472): pointer-free, run-stable, TOTAL, dense 0..N-1 =
  ForEachView insertion order at last stamp. BUT mutable/re-stamped (twice —
  IdentifyInductions + CSE), not frozen. Accessor DeterministicOrder()
  (Query.cpp:430) asserts stamped. STABILITY: deterministic run-to-run;
  portable across builds. THE substrate-sanctioned key.
- FinalizeColumnIDs (Columns.cpp:13-25, Build.cpp:2603): COLUMNS get a
  finalized ForEachView-ordered id space; VIEWS do not. A view-analogue
  renumber could be minted the same way if a frozen id is wanted.
- UniqueId() (Node.h:31 = reinterpret_cast<uintptr_t>(impl)): POINTER-derived —
  stable within one build, NON-portable across builds/runs. This is what the
  EXISTING -dot-out dump uses (Format.cpp:98-198). Choosing it for -df-out
  would make block ids non-portable — REJECTED by the §3(A) house rules.
  DECISION: choose det_seq (or a fresh FinalizeColumnIDs-style view renumber);
  do NOT use raw UniqueId (this is the E-57 ruling).

### 4.b Dump position — what the Build.cpp pipeline offers
- DataFlow (-df-out): Query::Build tail (Build.cpp:2518-2633): IdentifyInductions
  (2597) → FinalizeDepths (2602) → FinalizeColumnIDs (2603) → Stratify (2627).
  A POST-OPTIMIZE dump must be AFTER 2603 for stable column ids AND after
  Stratify for view.Stratum(). TableId() is NOT set here — it is annotated
  during Program::Build (Main.cpp:103-105 abstraction-break), so a Query-time
  -df-out shows NO table ids; a post-Program -df-out (drained alongside -dot-out
  at Main.cpp:106-109) does. PRE-OPTIMIZE dump (ir-dump-formats Q3, for
  SIP/demand review of the minted-but-not-CSE'd graph) would need a second
  drain position; the draft defers it. The demand transform slot
  (ApplyDemandTransform, Build.cpp:2576) is after ConnectInsertsToSelects (2564)
  and before Optimize (2585).
- DeltaRel (-deltarel-out): the ONLY validated-graph point is inside
  BuildStratumPhases (Stratum.cpp), after LinearizeAndValidateDRFlow (2097) or
  after the stash (2166). No Program-surface handle exists (§4.d / loud finding).

### 4.c Sidecar format — harness extension points and constraints
- Placement: a NEW helper in runall.sh's `--one` worker parallel to run_oracle
  (guarded by `[ -f cases/$NAME.<sidecar> ]` early-return), invoked
  `run_dfgold || st=1` alongside runall.sh:248 — NOT in diffrun.sh (E-58/E-59
  ruling: preserve diffrun.sh as the pure 4-mode primitive).
- Verdict grep trap: the new arm's failure verdicts MUST contain a
  `FAIL|DIVERGE|EXPECT-ERROR|MISSING` substring or the summary grep
  (runall.sh:284) silently passes a real divergence.
- Golden naming: dot-separated suffix (`.df.stdout`/`.deltarel.stdout`), NOT
  underscore (case names contain underscores). Sidecar names `.dfgold`/
  `.deltarelgold` are free (no collision with dr/main.cpp/batches/drflags).
- Mode pinning: IR is mode-sensitive; default the sidecar to opt-only (matching
  how the plain golden is implicitly opt-sourced, runall.sh:75). The 4 execution
  modes stay untouched for stdout goldens.
- Blessing: --bless-only (runall.sh:67-103); add a mirror block to the per-case
  --bless loop. bash 3.2: no declare -A — use case/esac or `[ -f ]` checks.
- IR goldens are STRICT byte-compare (permcheck.py stays a stdout-token referee;
  an IR permutation is exactly what byte-compare exists to catch).
- First carriers per seed §3(D): demand_tc_witness (.h + .ir demand-ON, THE
  restored (F) gate), symrec_tie_1 (.ir); ir-dump-formats §2.5 also names
  fixpoint_stress_1/reconverge_1/aggregate corpus — curated directed witnesses,
  NOT the full corpus (churn stays reviewable).

### 4.d P1 timing facts — the driver call sites a PassPolicy would thread through
The optimize toggles today are TWO global bools consumed at two call sites, both
in bin/drlojekyll/Main.cpp's CompileModule:
- gOptimizeDataFlow (Main.cpp:46-52) → passed to
  `Query::Build(module, error_log, gOptimizeDataFlow, gDemand)` (Main.cpp:60-61).
- gOptimizeControlFlow → passed to
  `Program::Build(*query_opt, error_log, gFirstId, gOptimizeControlFlow)`
  (Main.cpp:66-67).
- Set by -disable-dataflow-opt / -disable-controlflow-opt (Main.cpp:333-346).
- gDemand (the -demand flag, orthogonal to the 4 modes) also threads through
  Query::Build.
A PassPolicy (P1) would replace these two bools with a policy object threaded to
the same two Build entry points; the four golden modes re-expressed as aliases
must be byte-identical (seed §3(E)). The dump flags (-df-out/-deltarel-out) are
independent of this — they are drain-site flags, not pass toggles.

---

## (5) RESIDUAL-RISK CENSUS ROLL-UP — pointer-keyed iteration reaching emission

The det-substrate lane + verifier traced the DataFlow side comprehensively and
the DeltaRel emission-relevant maps. Adjudicated result: NO UNPROTECTED
emission-visible pointer-order iteration survives that the (F) substrate does
NOT cover. Detail:

PROTECTED (pointer-keyed but driven sorted by the (F) fix):
- Induction.cpp:147 merge_sets (unordered_map<VIEW*,MergeSet>) → sorted via
  OrderViewsDeterministically (585-586).
- Induction.cpp:167 injection_sites (std::set<VIEW*>) → sorted (405-408).
- Optimize.cpp:103 colors (unordered_map<VIEW*,uint64_t>) → iterated via
  group_order first-seen over FillViews-sorted all_views (289-301).
- Program.h:1832/1838/1845/1851/1853 the five OrderedViewMap → det_seq comparator.
- Join.cpp:74 out_to_in (unordered_map<COL*,...>) → iterated output-column order.

BENIGN (lookup-only, order-independent reduction, or validator-only): the full
list in lane-det-substrate §8 (top_map, in_to_out, decl_to_*, the DeltaRel
by_view/join_index/dr_join/receive_signs/join_pivot_vec + all *_stratum/scc_map
lookups, the ControlFlow Build.h work-item maps, hazard by_vec/by_flag classified
by accessor-kind not (i,j), scc_map max-reductions, proc-dedup keyed by uint64
hash). All re-verified benign by verify-det-substrate PART 2/3.

THE ONE FLAGGED-FOR-THE-FUTURE HAZARD (dead code, harmless today):
- ProgramImpl::Analyze() is DEAD — its sole call site is commented out at
  Build/Build.cpp:1399 (`// impl->Analyze();` + `#if 0`). DATARECORD/
  DATARECORDCASE appear in NO lib/CodeGen file and NO ControlFlow Format file.
  Inside that dead code, Analyze.cpp:1195 iterates `unique_table_sources`
  (unordered_map<TABLE*,...>, decl 110) and assigns DATARECORD ids via
  `table->records.Create(impl->next_id++, table)` (1196) — pointer-bucket-order
  next_id++ id assignment, EXACTLY the forbidden anti-pattern, but DEAD so
  harmless. RULING: any future D-lane reviving record-caching MUST sort
  unique_table_sources first. (verify-det-substrate corrected the lane here: the
  lane worried about the STRING-keyed key_to_provenance loop at 1213 which is
  run-stable anyway; the real latent hazard is the pointer-keyed 1195 loop the
  lane missed — both moot while Analyze is dead.)

VERDICT: the (F) substrate covers the live emission surface. No open
determinism hole beyond what (F) already fixes; the sole residual is
future-facing (record-caching revival), recorded for that D-lane.

---

## APPENDIX — §3 SEED CLAIM VERDICT TABLE (CONFIRMED / corrected)

§3(A) det_seq{~0u} @ Query.h:472 ...................... CONFIRMED
§3(A) DeterministicOrder() asserts stamped ............ CONFIRMED
§3(A) OrderViewsDeterministically 3-level TOTAL ....... CONFIRMED
§3(A) stamped at TWO entries (CSE + IdentifyInductions) CONFIRMED (lane's "THREE" wrong, E-55)
§3(A) OrderQueryViews/OrderedViewMap "~46" ........... CORRECTED to 48/55 (E-56)
§3(A) HashInit FNV-1a over KindName ................... CONFIRMED (incomplete: +flags+colcount; nuance)
§3(A) FinalizeDepths full reset / Join::Depth / CSE ... CONFIRMED
§3(A) house rules (1)(2)(3 symrec_tie_1)(4) .......... CONFIRMED (case exists; no .ir sidecar yet = future T3)
§3(B) -dot-out DataFlow→GraphViz ..................... CONFIRMED
§3(B) -ir-out ControlFlow textual, gIRStream "~271-282" CONFIRMED (actual 271-283)
§3(B) lib/DeltaRel NO dump surface (validators only) .. CONFIRMED
§3(C) -df-out BB tail-call form, det_seq block id ..... CONFIRMED as achievable; block-id source = DESIGN CHOICE (E-57: prefer det_seq over UniqueId)
§3(C) -deltarel-out ops in CHECKED-LINEARIZATION order  CONFIRMED (pinned_order)
§3(C) emitted from validate-exit ..................... CONFIRMED (Stratum.cpp:2097/2166)
§3(D) .batches/.drflags-idiom sidecars, --bless-only .. CONFIRMED
§3(D) "runall.sh/diffrun.sh grow the compare arms" .... IMPRECISE — runall.sh --one ONLY (E-58)
§3(D) first carriers demand_tc_witness / symrec_tie_1 . CONFIRMED (both real cases)
§3(D) suite count unchanged; goldens/ grows .......... CONFIRMED (but CLAUDE.md 168 stale vs 169, E-60)

END consolidated record.
