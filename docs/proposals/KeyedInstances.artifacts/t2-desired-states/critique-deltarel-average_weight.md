# Adversarial critique — t2-desired-deltarel-average_weight.md

CRITIC pass, KeyedInstances epoch, tip b577735e. Target artifact:
`/private/tmp/.../scratchpad/desired-states/t2-desired-deltarel-average_weight.md`.
Ground truth read this session: `dump-inputs/average_weight.ir` (the lowered
.ir, authority for op inventory + emission order), `dump-inputs/average_weight.dot`
(the stratum map), `lib/DeltaRel/DeltaRel.cpp` (mint code + band comparator),
`lib/ControlFlow/Build/Stratum.cpp` (LowerCommitSweeps / LowerDRFlow / the
validate-exit), spec `t2-dump-spec.md`, `ir-dump-formats.md`.

Method: every op/vec/edge/kind checked against the .ir + the cited code lines
(E-54: read the lines, not the citations). Findings ranked most-severe first.

--------------------------------------------------------------------
## C-1 (CRITICAL, determinism + internal contradiction) — the dump's commit-sweep order is NOT the .ir order the artifact renders

The artifact renders the 8 COMMIT_SWEEP ops in ".ir observed order"
4,8,12,17,23,28,32,36 (§1 lines 353-377; the comment at :351-352 and F-1 both
say this). It simultaneously asserts (spec §2.3 correction 2; artifact §2.4;
F-6) that **the dump walks `pinned_order`**. These two are INCOMPATIBLE for the
commit band, and the discrepancy is load-bearing for a byte-compare golden.

EVIDENCE — two distinct orderings, verified in code:

1. The .ir commit-sweep textual order (4,8,12,17,23,28,32,36) is produced by
   `LowerCommitSweeps` (`lib/ControlFlow/Build/Stratum.cpp:2018`), which iterates
   `dr_flow.ops` DIRECTLY — i.e. MINT order — NOT pinned_order. The function's
   own header comment (:1994-1996) says so: "kCommitSweep ops are minted in
   `impl->tables` order ... so the sibling order is the lowering default."
   And the mint loop (`DeltaRel.cpp:1672`) is `for (TABLE *table : impl->tables)`.
   `impl->tables` is Id-ascending (tables created via `impl->tables.Create(
   impl->next_id++)`, `Data.cpp:194` — DefList append in view-walk order, Id
   monotone). So the .ir order = impl->tables = Id-ascending = 4,8,12,17,23,28,
   32,36. This is deterministic and POINTER-FREE.

2. The DUMP, per the artifact's own stated rule, walks `pinned_order`
   (`LinearizeAndValidateDRFlow`). For a kCommitSweep, `key_of`
   (`DeltaRel.cpp:3445-3447`) returns `Key{2, max_stratum+1, 9, op_table_id(op),
   0, oi}`, and `key_less` (:3461) breaks the (lead=2,stratum,band=9) tie on
   `table_id`, which is `reinterpret_cast<uintptr_t>(t)` (`op_table_id`,
   :3387-3394) — a RAW POINTER. So pinned_order sorts the 8 sweeps by table
   POINTER, which is NOT guaranteed to equal Id-ascending order.

CONSEQUENCE: if the dump faithfully walks pinned_order (the artifact's rule),
the commit sweeps appear in POINTER order, which (a) is not the 4,8,12,17,23,28,
32,36 the artifact hand-wrote, and (b) is not deterministic across allocator
changes. The artifact both mis-predicts its own commit-sweep order AND embeds a
pointer-derived sequence into a would-be byte-compare golden. This is the single
most important defect: a golden blessed from this ordering churns on any
allocation change with zero semantic change.

This makes the writer's F-1 UNDER-stated: F-1 frames the pointer tie-break as a
"harden it before blessing" nicety and claims "I rendered them in the .ir's
observed order." But the .ir order is a DIFFERENT lowering path (mint order via
LowerCommitSweeps) than the dump's stated path (pinned_order). The artifact
should either (i) render commit sweeps in pinned_order (pointer order — unknowable
without a run, so the golden is unwriteable by hand), or (ii) declare the dump
sorts the commit band by a pointer-free key (DataTable::Id()), in which case the
4,8,12,17,23,28,32,36 rendering is correct BUT the dump no longer "walks
pinned_order" verbatim. The spec must pin this; the artifact cannot be a valid
golden until it is.

SAME DEFECT for the 3 STATE_SEAL ops (op.1/op.3/op.5, §1 lines 378-386): `key_of`
(`DeltaRel.cpp:3448-3452`) gives them `Key{2, max+1, 10, op_table_id(op), 0, oi}`
— band 10, tie-broken on the SAME table POINTER. The artifact renders them in
sc#/Id order (sc#0 table:4, sc#1 table:8, sc#2 table:12), but pinned_order sorts
them by pointer. If the dump walks pinned_order, the seal order is pointer-derived
too. (Unlike the commit sweeps, seals are NOT re-lowered by an Id-order loop —
LowerCommitSweeps attaches the seal to its sweep via `table_to_statecell`,
Stratum.cpp:2013-2037, so the seal's .ir position rides its sweep. But the DUMP
still emits seals as their own pinned_order entries — pointer order.)

--------------------------------------------------------------------
## C-2 (HIGH, wrong ordering rule) — the vec-block emission order is wrong: impl->tables is Id-ascending (4,8,12,...), not 12,28,4,8,...

The artifact emits the vec section (§1 lines 70-111) grouped by table in the
order **12, 28, 4, 8, 23, 17, 32**. The spec (correction 2) says vecs "iterate
their OWN ordered vector" = `DRFlowGraph::vecs` in MINT order. Vecs are minted
`for (TABLE *table : impl->tables)` (`DeltaRel.cpp:878`), six per differential
non-induction table (delQ, addQ, overdel, addSet, netRem, netAdd — :882-893).
So the vec-block order is exactly `impl->tables` order.

`impl->tables` order is **Id-ascending = 4, 8, 12, 17, 23, 28, 32** (36 is
monotone → no phase vecs). PROOF: `LowerCommitSweeps` iterates `dr_flow.ops`
(mint order = `impl->tables`, `DeltaRel.cpp:1672`) and the resulting .ir
commit-sweep lines (.ir:335-342) are 4,8,12,17,23,28,32,36 — direct evidence of
`impl->tables` iteration order. Tables are created with monotone `next_id`
(`Data.cpp:194`), so DefList order == Id order.

The artifact's "12,28,4,8,23,17,32" ordering appears NOWHERE in the code and is
contradicted by the .ir. The `.ir` `vector-define` block (.ir:90-131) is ALSO
Id/stratum-ascending (table:12's queues at :90-95 come first only because
table:12 is stratum 1; but that is LOWERING order in ^flow, keyed by stratum,
NOT mint order). The dump's vec section is spec'd to follow vecs MINT order,
which is impl->tables = 4,8,12,17,23,28,32.

This is a WRONG-RULE finding (task rubric item 2 = HIGH), independent of the
illustrative-integer exemption (F-7): the SEQUENCE of vec lines in a byte-golden
is wrong. Note the writer's own §2.2 (lines 552-558) states the SAME wrong order
("vec.0..5 %table:12 ... vec.6..11 %table:28 ... vec.12..17 %table:4"), so the
error is consistent between §1 and §2.2 — both derive from the mis-stated
impl->tables order.

Corollary: §2.1's MINT-order tables ("op.16..19 %table:12 ... op.20..23
%table:28 ... op.24..27 %table:4", lines 508-514; PHASE 8 "12,28,4,8,23,17,32",
line 519) carry the same wrong impl->tables order. The OP EMISSION section in §1
is NOT affected because it is stratum-ordered (pinned_order phase), not
mint-ordered — but every mint-order-derived index PREDICTION in §2.1/§2.2 is
built on the wrong table sequence.

--------------------------------------------------------------------
## C-3 (MED, format/faithfulness) — the `deps:` section is abbreviated and cannot be a byte-golden

The artifact's `deps:` section (§1 lines 388-421) is hand-curated: it packs two
edges per line ("op.6 -> op.20 RAW epoch  ;  op.7 -> op.21 RAW epoch") and ends
with "WAR/RAW vs the per-table filters, elided for brevity". A byte-compare
golden CANNOT elide. The real `flow.dep_edges` (`DeltaRel.h:657`, a
`std::vector<DRDep>`) is populated exhaustively in `LinearizeAndValidateDRFlow`
(`DeltaRel.cpp:3527ff` emit_waw / the RAW/WAR emitters) and includes WAW/WAR
edges over every shared vec AND every flag class — far more than the RAW chain
shown. The dump must render ALL of them, one per line, in `dep_edges` vector
order. The artifact's rendering is illustrative-only and materially incomplete;
it is not a candidate golden for the deps section. (This compounds C-1: some
dep-edge FROM/TO orientations are decided by `key_less`'s pointer tie-break when
two ops share a key, so even a complete deps rendering inherits the pointer
non-determinism until op_table_id is hardened.)

This is partly the writer's F-2 concern but broader: F-2 is about joins/branches
having no skeleton slot; C-3 is that the deps SECTION ITSELF is unfaithful.

--------------------------------------------------------------------
## Assessment of the writer's declared frictions (F-1..F-7)

- **F-1 (writer: HIGH)** — CORRECT and UNDER-STATED. The pointer tie-break is
  real (`op_table_id`, `DeltaRel.cpp:3387-3394`; `key_less`, :3461). But F-1
  claims "I rendered them in the .ir's observed order" as if the .ir order and
  the dump order are the same surface — they are NOT (C-1: the .ir uses
  LowerCommitSweeps' Id-order mint loop; the dump uses pinned_order's pointer
  key). Promote to CRITICAL for the golden. The recommendation (harden
  op_table_id to DataTable::Id()) is exactly right and should be a PRECONDITION
  of blessing, not an optional follow-on.

- **F-2 (writer: MED)** — VALID. average_weight has one DRJoin (`flow.joins`,
  not a DROp — confirmed: the join lowers via `LowerSectionWalk`,
  Stratum.cpp:1549/1554, and BuildDRInventory mints branches/joins at :1108ff,
  never an op). The spec §2.3 skeleton (vec/op/deps/census) genuinely has no slot
  for joins/branches/rounds. The artifact renders the join as an inline `;;`
  comment (§1 lines 270-272) — a reasonable choice, but the spec MUST pin it.
  Real spec gap.

- **F-3 (writer: MED)** — VALID. The vec `<shape>` rendering is spec-under-
  specified. The artifact's `<ids %table:N>` / `<id-cols>` maps to
  `ElementShape::kIds`/`kIdCols` (`DRVec.shape`, DeltaRel.h:247) and `uniq=` to
  `UniqueContract` (:249) — both real fields. The `.` vs `:` sigil divergence
  (dump uses `.`, .ir uses `:`) is a real, deliberate choice the spec should
  confirm. Note: the vec ROLE the artifact bakes into the sigil name
  (`$net-removal.4`) is `DRVec.role` (:248); putting it in the name vs a `role=`
  field is a live open the spec must settle.

- **F-4 (writer: LOW)** — VALID but note a deeper issue: the DeltaRel dump has
  NO ParsedModule in scope at the emit point (Stratum.cpp:2166 is inside
  BuildStratumPhases, which has `query` but the module-name provenance is thin).
  `average_weight.dr` declares no `#module`; the basename fallback is a
  reasonable pin but the emitter has no `.dr` path at that call site either — the
  fallback may have to be a literal or empty. Real open; LOW.

- **F-5 (writer: LOW)** — CORRECT. kStateEmit/kStateOld are `EffKind` members
  (DeltaRel.h EffKind), not `Pred` reads; keeping them under `effects:` (not
  `reads:`) is faithful to the model. Confirmed against BuildGroupUpdateOps
  (:710-717 push kStateEmit/kStateOld as effects). No defect; the spec should
  simply confirm.

- **F-6 (writer: LOW)** — CORRECT framing but interacts with C-1. op.52
  (INGEST_FOLD) is lead-0 in pinned_order (`key_of` :3442-3444) and lowers into
  ^entry (confirmed: it is the monotone ingest fold, ctx=kEager,
  MakeMonotoneIngestFold :818-825). Rendering it first (pinned_order) with a
  "lowered in ^entry" note is faithful. But this CONFIRMS the dump follows
  pinned_order — which is exactly what makes the commit-sweep rendering (C-1)
  inconsistent: the artifact follows pinned_order for op.52 but Id-order for the
  commit sweeps. Pick one rule.

- **F-7 (writer: INFO)** — the illustrative-index disclaimer is appropriate and
  covers the wrong INTEGER assignments. It does NOT cover a wrong ORDERING RULE
  (C-2) — the sequence of vec/op lines is structural, not an integer label, and
  the mis-stated impl->tables order (12,28,4,8,... vs the true 4,8,12,...) is a
  rule error the disclaimer does not excuse.

## Structural checks that PASSED (verified against .ir + code)

- sc# numbering (sc#0=sum, sc#1=count, sc#2=kv): CONFIRMED — Aggregates() minted
  before KVIndices() (`DeltaRel.cpp:1057/1085`), `sc = statecells.size()` at mint
  (:667); matches .ir sc# labels (.ir:136/187/210).
- GROUP_UPDATE effect multiset (2 kVecDrain, 2 kStateFold, 1 kStateEmit, 1
  kStateOld, 2 kCounter, 2 kInIReadFrozen, 2 kVecAppend) and push ORDER:
  CONFIRMED line-by-line against BuildGroupUpdateOps (:689-740).
- STATE_SEAL effect {1 kStateFold(sign=0)}: CONFIRMED (:755-759).
- op.52 ingest: ctx=eager, sign=+1, klass=NonRecursive (EmissionDerivClass over
  a non-induction receive returns kNonRecursive, Build.cpp:698), role=kNetAddition
  (MonotoneIngestRoleDR → AnyCutSuccessorDR true, :154-157): ALL CONFIRMED.
- 10 seed folds (6 head-chain + 4 join-pivot), 14 claim, 14 filter, 8 commit
  sweeps (7 diff + 1 mono), 3 GROUP_UPDATE, 3 STATE_SEAL: census CONFIRMED against
  the .ir (claim-del/claim-add/check-member counts per table) and mint code.
- GROUP_UPDATE strata (kv=1, sum=3, count=5): CONFIRMED — group_update_stratum =
  agg_view.Stratum() (`DeltaRel.cpp:2152`); .dot gives view strata 1/3/5.
- table:36 monotone, owns no phase vec: CONFIRMED (TableIsDifferential(36)=false;
  MintTableVec skips non-differential, :879).
- group/summary column SPACES (input-view space): CONFIRMED — Aggregates use
  InputGroupColumns/InputAggregatedColumns (:1064/1071); KV uses InputKeyColumns/
  InputValueColumns (:1087/1091).
- .uses/.defs vec edges are real state (DeltaRel.h:258-259), .uses populated by
  drain effects in the linearizer (:3193-3195): the vec `def=/use=` MODEL is
  sound (only the integer values are illustrative).
- emit point (Stratum.cpp:2166) is post-LinearizeAndValidateDRFlow (:2097) and
  post-Site-5 (:2113-2158): a dump there sees a fully-linearized, validated,
  use-edge-populated flow. Spec §2.2 emit point is SOUND.

## VERDICT: UNSOUND (as a byte-compare golden)

The artifact is an excellent structural map — every op kind, effect set, census
count, sc# assignment, stratum, and column space checks out against the .ir and
the construction code. But it FAILS its stated purpose (a byte-compare
`deltarel opt` golden) on three counts:

  1. C-1 (CRITICAL): the commit-sweep and state-seal ORDER embedded in the dump
     is pointer-derived under the artifact's own "walk pinned_order" rule, yet
     rendered in Id order — internally inconsistent AND non-deterministic. Not
     blessable until op_table_id is hardened to DataTable::Id() (the writer's own
     F-1 recommendation, which must be a precondition, not a follow-on).
  2. C-2 (HIGH): the vec-block emission order (and the §2.1/§2.2 mint-order
     tables) use a wrong impl->tables sequence (12,28,4,8,... instead of the true
     Id-ascending 4,8,12,17,23,28,32) — a wrong ORDERING RULE, not excused by the
     illustrative-integer disclaimer.
  3. C-3 (MED): the deps section is abbreviated/elided and materially incomplete
     — unusable as a golden as written.

The structure is SOUND; the ordering and completeness are not. Fix C-1 (harden
the key OR redefine the dump's commit-band order as Id-sorted and say so), C-2
(reorder vec blocks to 4,8,12,17,23,28,32), and C-3 (render the full dep_edges
vector), and the artifact becomes a valid desired-state golden.
