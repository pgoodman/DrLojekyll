# T2 desired-state — `-deltarel-out` dump of average_weight.dr

FIRST-EMISSION PIN (2026-07-19, the T2b landing tree; ledger §15):
§1 is now the ADJUDICATED LIVE EMISSION (5-run stable, debug==release
byte-identical — the config-invariance audit passed DIRECTLY). The
adjudications, each code-read (never matched-to-output blindly):
- IDS PINNED (the symrec precedent): only the 10 SEED_FOLDs permuted
  vs the illustrative guess (actual mint order: join-pivot folds
  FIRST — old->new 6->10 7->11 8->14 9->15 10->12 11->13 12->6
  13->7 14->8 15->9); branch ids bijection 0->2 1->4 2->3 3->0
  4->1; vec ids IDENTITY; all other 43 op ids IDENTITY.
- p13 MECHANICAL WHITESPACE: single-space separators, no column
  padding, one-line effects (the p9 precedent; the old hand
  alignment was internally inconsistent and unpinned).
- p14 DEPS: canonical (from,to,kind,scope,carried) sort + exact-
  duplicate-row dedup (the flag-enrollment walk mints the same
  edge once per access pair; the section renders the dependence
  RELATION — no edge elided). 174 unique edges; the old 36-edge
  floor is a strict subset (F-9/F-10 RESOLVED — this is the full
  live enrollment, the floor caveat retires).
- 4a agg= renders the DERIVABLE functor name (kOver: the aggregate
  functor; kKv: the merge functor — op.4 agg=new_weight_i32, was
  the unrenderable relation-name guess edge_weight; no model field
  added).
- 4b vec def=[] on overdelete/addition/net/join-pivot vecs is the
  FAITHFUL render: the DR-IR registers no def edges for them
  (mint_claim/filter/join-arm record effects only). MODEL-FIDELITY
  NOTE: def enrollment for these classes is a future model
  improvement; the dump will surface it via re-bless when added.
- 4c spine: — renders on ALL FOUR join-pivot seeds (mechanical
  uniformity; the old block marked only one).
§2's derivation prose keeps the PRE-PIN illustrative ids and the
old hand-rendered forms — HISTORICAL; read op ids through the
bijection above. The §1 block is the byte contract.

RE-RENDERED 2026-07-19 (round-3 grammar adjudication, ledger §11;
pinned grammar = scratchpad t2b-grammar.md R-1..R-10, verifier SOUND):
§1 re-rendered under the pinned T2b emitter grammar. ALL `;;` banners
and `;` comments STRIPPED — the deltarel byte-block carries ZERO
comments (pin p10, the p8 analogue; every stripped annotation
survives in §2's derivation prose). join.0's section-walk subline
DROPPED (J-2: %index ids are ControlFlow DataIndex ids, absent from
the DR-IR at the emit point — UNRENDERABLE). op.51 gains
publish_target=false (uniform render of the real bool). op.52's
args drop receive=<recv %table:36> (no such primitive; = table=)
and its spine is — (INGEST_FOLD has no PlanTree). op.8/9 spine
functor gloss stripped (SP-1: PlanNode carries no functor name;
the true spine shape is pinned at first emission). branch=<...>
composites dropped from all seed args (AR-1; redundant with
src=/tgt=). census FLATTENED to the 15-kind DROpKind enum-order
one-liner. Section layout pinned: single blank line between
sections, none within (pin p11). Op/vec/branch ids remain
ILLUSTRATIVE (pinned at first emission — the symrec precedent).
deps remains the F-9 canonical-(from,to,kind)-sort floor (F-10
flag-class completeness open — first emission supersedes the
floor). GU-3 (verifier G-1): `mutable(new_weight_i32)` IS the live
.ir spelling — the byte stands, the earlier "renders as mut" claim
is struck. Multibyte-glyph note (G-2): `·` (sign) and `—` (spine)
are the corpus's first multibyte golden bytes — byte-verify both
at bless.

AMENDED 2026-07-19 (round-2 adjudication, ledger §10 — C-AW-1/C-AW-2,
the consolidator-CONFIRMED p4 nonconformance this artifact's own F-5
flagged as a known unfixed defect): the NINE `reads: InI(%table:N)`
lines DELETED (ops 4, 6, 7, 0, 2, 8, 9, 10, 11 — the frozen-InI
crossing is the kInIReadFrozen EffKind, already present under each
op's `effects:`; pin p4 restricts `reads:` to Pred spellings from
kFlagRead, so those ops now carry NO reads: line) and the TWO
`reads: —` placeholders removed (op.52's line deleted; op.24's
TryClaimDel note kept as a standalone `;` annotation line, the op.52
idiom) to match the thirteen claim drains that omit the line
entirely. FRONTIER_FILTER `reads: NetDeleted/NetAdded` lines are
kFlagRead-backed and KEPT. Within-band op order re-CONFIRMED under
the as-landed +1 band key (monotone shift; round-2 contract-deltarel
critic). All other bytes UNCHANGED.

## §0 Provenance

DESIRED-STATE WRITER deliverable, KeyedInstances epoch, branch `keyed-instances`.
REVISED 2026-07-18 against ratified t2-dump-spec v3 (post-E-61..E-66), folding
the committed critique `critique-deltarel-average_weight.md` (C-1/C-2/C-3) — the
DRAFT-PENDING-REVISION banner is retired. This hand-writes the EXACT bytes the
future `-deltarel-out <PATH>` dump must emit for
`tests/OptDiff/cases/average_weight.dr`, per the BINDING spec
`docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md` §2.3 (the ratified
DeltaRel dump format). average_weight is the GROUP_UPDATE witness — a
`@recompute` KV index (`edge_weight`) feeding two `@invertible` summary
aggregates (`sum_i32`, `count_i32`), joined and divided into a per-node average.
It exercises THREE `kGroupUpdate` + THREE `kStateSeal` ops (one KV + two summary
aggregates), the strongest R3 fixture.

AMENDMENTS APPLIED THIS PASS (against v3 + the critique):
  - C-1 (CRITICAL): the dump presents the POST-T2b.0 pinned_order. `op_table_id`
    is hardened to the table's deterministic id (`t ? t->id : 0u`, spec §2.0 /
    decision (b2)), so within a band the tie-break is table-id ASCENDING (null→0
    first; real ids ≥3), then sign − before +, then ctor. The 8 commit sweeps
    (band 9) and 3 state seals (band 10) therefore emit in id-ascending order —
    4,8,12,17,23,28,32,36 for the sweeps, sc#0/sc#1/sc#2 (tables 4,8,12) for the
    seals — now a DETERMINISTIC pinned_order fact, no longer "the .ir observed
    order" (the internal contradiction the critique flagged is gone; the writer's
    old F-1 is folded into T2b.0 as a landed precondition).
  - C-2 (HIGH): the vec block and every mint-order-derived table sequence are
    reordered to `impl->tables` id-ascending = 4,8,12,17,23,28,32 (36 monotone,
    no phase vec). The old 12,28,4,8,… order matched nothing in the code.
  - C-3 (MED): the deps section is rendered EXHAUSTIVELY in `flow.dep_edges`
    vector order (no elision, no two-per-line packing), with `loop_carried`
    carried there; a byte-golden cannot elide.
  - E-61 + column-token rule: op-kind traversal / kind-order rulings re-checked
    (this case has no negations/compares, so the compares/negations transposition
    does not bite the body). The header is now the bare `deltarel` (v3 §2.3 line-1,
    NO module name). GROUP_UPDATE group@/summary@ column tokens are rewritten to
    the v3 `<var-or-cN>:<type>` form (From:i32 not col:37(From)); cN is fallback
    only, no @cN suffix, never _MissingVar.
  - Validator tokens: any V-* NAME the notes present is tagged as either an
    EMITTED `ValidatorFail(...)` literal (V-AGG-EFFECT, V-AGG-SOLE) or a
    comment-only design title (V-COMMIT-TRAILS, V-SEED-DRAIN — never emitted);
    the dump BODY renders no validator token.
  - Config-invariance audit (a3-extended): CLEAN — no §2.3-rendered field reads
    an `#ifndef NDEBUG` member (DeltaRel.h has zero NDEBUG-gated members; the two
    NDEBUG hits are comments). See §3 F-8.

INPUTS READ THIS SESSION (all absolute):
  - THE SPEC: `docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md` §2.3
    (format + the 9 binding corrections), §2.2 (emit point Stratum.cpp:2166),
    §2.1 (wiring).
  - `docs/proposals/KeyedInstances.artifacts/ir-dump-formats.md` §1/§2 (the BB
    form + the pre-reconciliation §2 draft the spec refines).
  - `/private/tmp/.../fleet-ckpt/consolidated.md` §1.B (DeltaRel inventory: 15
    DROpKind, 10 Pred, 10 EffKind, 14 VecRole spellings + line cites), §3
    (the §2 reconciliation), §1.A ((F) det_seq substrate).
  - `/private/tmp/.../fleet-ckpt/lane-dump-surfaces.md` (depth: struct fields,
    the validate-exit hook, the .ir sigil harmonization).
  - `docs/proposals/KeyedInstances.artifacts/witness-deltarel-target.md` §2.6
    (the op-line STYLE MODEL — adapted here to the spec §2.3 corrections:
    real DROpKind/Pred/EffKind spellings, index ids, NO `band=<key>` header,
    the deps section for carriage, the census tail).

GROUND TRUTH (evidence, all re-read/re-generated this revision pass):
  - SOURCE: `tests/OptDiff/cases/average_weight.dr`.
  - DATAFLOW: `ground/average_weight.dot` (the stratum map — see §2 table below),
    regenerated from the FROZEN baseline binary `drlojekyll.debug.63c8443c`.
  - CONTROLFLOW: `ground/average_weight.ir` (the lowered .ir — THE authority for
    the table-id space (`%table:<id>` create blocks) + the commit-sweep textual
    order, regenerated from the same frozen baseline). NOTE: the .ir commit-sweep
    order (4,8,12,17,23,28,32,36) is produced by `LowerCommitSweeps` iterating
    `dr_flow.ops` in MINT order (`impl->tables` id-ascending) — it COINCIDES with
    the post-T2b.0 pinned_order because both are id-ascending, but the dump's
    authority is pinned_order, not the .ir walk (see §2.4 / C-1).
  - CONSTRUCTION CODE: `lib/DeltaRel/DeltaRel.cpp` — `BuildDRInventory`
    (843-1860), `BuildGroupUpdateOps` (638-761), `MakeMonotoneIngestFold`
    (813-842), `DeriveDRStrata` op_stratum (3276-3346), the linearizer band
    comparator `op_band`/`op_table_id`/`op_sign`/`key_of`/`key_less`
    (3354-3464).

ILLUSTRATIVE-PREDICTED indices: op/vec ids are the dense mint-order indices
into `DRFlowGraph::ops` / `::vecs` (no id field — identity IS the vector
position; consolidated §1.B "OP/VEC ID SCHEME"). I predict them from the
construction-code walk order below. The FIRST compiler run pins the exact
integers; STRUCTURE and ORDER are what this artifact gets right. Every op
index is marked with a `; PRED` provenance note in §2. The table/index/col
`%…:<id>` ids ARE authoritative (they are the DataTable::Id() space the .ir
prints — consolidated §1.B ".ir NAMING", spec §2.3 correction 8).

## §1 THE DESIRED DUMP TEXT

Per spec §2.3: header `deltarel <module-name>`; then all vecs in INDEX order;
then ops in `pinned_order` (emission order — the checked linearization); then
`deps`; then `census`. Op/vec ids are the dense mint indices (ILLUSTRATIVE-
PREDICTED, marked in §2 — the first compiler run pins them). `%table:`/`%index:`
are authoritative (.ir id space). No `band=<key>` header (spec correction 5) —
strata are rendered as `stratum=<k>` per op via lookup + `;;` grouping comments.
HEADER (v3, E-61-era): the header line is EXACTLY `deltarel` — NO module name (the
ratified spec §2.3 line-1 rule, mirroring the `.df` bare `dataflow` header; the
v1 draft's `deltarel average_weight` is retired — see §3 F-4).

```
deltarel

vec $delete-queue.0 <ids %table:4> uniq=sort-unique-at-drain def=[op.0] use=[op.16]
vec $add-queue.1 <ids %table:4> uniq=sort-unique-at-drain def=[op.0] use=[op.17]
vec $overdelete-set.2 <ids %table:4> uniq=multiset def=[] use=[op.18]
vec $addition-set.3 <ids %table:4> uniq=multiset def=[] use=[op.19]
vec $net-removal.4 <ids %table:4> uniq=multiset def=[] use=[op.6]
vec $net-addition.5 <ids %table:4> uniq=multiset def=[] use=[op.7]
vec $delete-queue.6 <ids %table:8> uniq=sort-unique-at-drain def=[op.2] use=[op.20]
vec $add-queue.7 <ids %table:8> uniq=sort-unique-at-drain def=[op.2] use=[op.21]
vec $overdelete-set.8 <ids %table:8> uniq=multiset def=[] use=[op.22]
vec $addition-set.9 <ids %table:8> uniq=multiset def=[] use=[op.23]
vec $net-removal.10 <ids %table:8> uniq=multiset def=[] use=[op.8]
vec $net-addition.11 <ids %table:8> uniq=multiset def=[] use=[op.9]
vec $delete-queue.12 <ids %table:12> uniq=sort-unique-at-drain def=[op.4] use=[op.24]
vec $add-queue.13 <ids %table:12> uniq=sort-unique-at-drain def=[op.4] use=[op.25]
vec $overdelete-set.14 <ids %table:12> uniq=multiset def=[] use=[op.26]
vec $addition-set.15 <ids %table:12> uniq=multiset def=[] use=[op.27]
vec $net-removal.16 <ids %table:12> uniq=multiset def=[] use=[op.10]
vec $net-addition.17 <ids %table:12> uniq=multiset def=[] use=[op.11]
vec $delete-queue.18 <ids %table:17> uniq=sort-unique-at-drain def=[op.14] use=[op.28]
vec $add-queue.19 <ids %table:17> uniq=sort-unique-at-drain def=[op.15] use=[op.29]
vec $overdelete-set.20 <ids %table:17> uniq=multiset def=[] use=[op.30]
vec $addition-set.21 <ids %table:17> uniq=multiset def=[] use=[op.31]
vec $net-removal.22 <ids %table:17> uniq=multiset def=[] use=[op.12]
vec $net-addition.23 <ids %table:17> uniq=multiset def=[] use=[op.13]
vec $delete-queue.24 <ids %table:23> uniq=sort-unique-at-drain def=[] use=[op.32]
vec $add-queue.25 <ids %table:23> uniq=sort-unique-at-drain def=[] use=[op.33]
vec $overdelete-set.26 <ids %table:23> uniq=multiset def=[] use=[op.34]
vec $addition-set.27 <ids %table:23> uniq=multiset def=[] use=[op.35]
vec $net-removal.28 <ids %table:23> uniq=multiset def=[] use=[op.14]
vec $net-addition.29 <ids %table:23> uniq=multiset def=[] use=[op.15]
vec $delete-queue.30 <ids %table:28> uniq=sort-unique-at-drain def=[op.10] use=[op.36]
vec $add-queue.31 <ids %table:28> uniq=sort-unique-at-drain def=[op.11] use=[op.37]
vec $overdelete-set.32 <ids %table:28> uniq=multiset def=[] use=[op.38]
vec $addition-set.33 <ids %table:28> uniq=multiset def=[] use=[op.39]
vec $net-removal.34 <ids %table:28> uniq=multiset def=[] use=[op.0,op.2]
vec $net-addition.35 <ids %table:28> uniq=multiset def=[] use=[op.0,op.2]
vec $delete-queue.36 <ids %table:32> uniq=sort-unique-at-drain def=[op.12] use=[op.40]
vec $add-queue.37 <ids %table:32> uniq=sort-unique-at-drain def=[op.13] use=[op.41]
vec $overdelete-set.38 <ids %table:32> uniq=multiset def=[] use=[op.42]
vec $addition-set.39 <ids %table:32> uniq=multiset def=[] use=[op.43]
vec $net-removal.40 <ids %table:32> uniq=multiset def=[] use=[]
vec $net-addition.41 <ids %table:32> uniq=multiset def=[] use=[]
vec $join-pivots.42 <id-cols> uniq=sort-unique-at-drain def=[] use=[]

branch.0 src=%table:4 -> join.0 ends_at_join=true
branch.1 src=%table:8 -> join.0 ends_at_join=true
branch.2 src=%table:12 -> tgt=%table:28 ends_at_join=false
branch.3 src=%table:17 -> tgt=%table:32 ends_at_join=false
branch.4 src=%table:23 -> tgt=%table:17 ends_at_join=false

join.0 view=<X,Sum,Count> pivot_vec=$join-pivots.42 targets=[%table:23]

op.52 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:36, +, NonRecursive), kVecAppend(%table:36, kNetAddition)}
    spine: —
    args: table=%table:36 message=add_edge/3
op.4 kGroupUpdate sign=· ctx=seed stratum=1 sc#2
    agg=new_weight_i32 prov=kKv algebra=kRecompute agg_table=%table:12
    group@{From:i32,To:i32} summary@{Weight:mutable(new_weight_i32)} config=0 input=%table:36
    effects: {kVecDrain(%table:36, kNetRemoval), kStateFold(%table:12, -), kVecDrain(%table:36, kNetAddition), kStateFold(%table:12, +), kStateEmit(%table:12), kStateOld(%table:12), kCounter(%table:12, -, NonRecursive), kInIReadFrozen(%table:12, InI, seed), kVecAppend(%table:12, kDeleteQueue), kCounter(%table:12, +, NonRecursive), kInIReadFrozen(%table:12, InI, seed), kVecAppend(%table:12, kAddQueue)}
    spine: —
    args: agg_table=%table:12 input=%table:36 statecell=sc#2
op.24 kClaimDrain sign=- ctx=seed stratum=1 form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:12, kDeleteQueue), kFlagWrite(%table:12, -), kVecAppend(%table:12, kOverdeleteSet)}
    args: table=%table:12
op.25 kClaimDrain sign=+ ctx=seed stratum=1 form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:12, kAddQueue), kFlagWrite(%table:12, +), kVecAppend(%table:12, kAdditionSet)}
    args: table=%table:12
op.26 kFrontierFilter sign=- ctx=seed stratum=1 deferral=immediate
    reads: NetDeleted(%table:12)
    effects: {kVecDrain(%table:12, kOverdeleteSet), kVecAppend(%table:12, kNetRemoval)}
    args: table=%table:12
op.27 kFrontierFilter sign=+ ctx=seed stratum=1 deferral=immediate
    reads: NetAdded(%table:12)
    effects: {kVecDrain(%table:12, kAdditionSet), kVecAppend(%table:12, kNetAddition)}
    args: table=%table:12
op.10 kSeedFold sign=- ctx=seed stratum=2 src=%table:12 tgt=%table:28 class=NonRecursive
    effects: {kVecDrain(%table:12, kNetRemoval), kCounter(%table:28, -, NonRecursive), kInIReadFrozen(%table:28, InI, seed), kVecAppend(%table:28, kDeleteQueue)}
    spine: kFold(%table:28, -, NonRecursive)
    args: src=%table:12 tgt=%table:28
op.11 kSeedFold sign=+ ctx=seed stratum=2 src=%table:12 tgt=%table:28 class=NonRecursive
    effects: {kVecDrain(%table:12, kNetAddition), kCounter(%table:28, +, NonRecursive), kInIReadFrozen(%table:28, InI, seed), kVecAppend(%table:28, kAddQueue)}
    spine: kFold(%table:28, +, NonRecursive)
    args: src=%table:12 tgt=%table:28
op.36 kClaimDrain sign=- ctx=seed stratum=2 form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:28, kDeleteQueue), kFlagWrite(%table:28, -), kVecAppend(%table:28, kOverdeleteSet)}
    args: table=%table:28
op.37 kClaimDrain sign=+ ctx=seed stratum=2 form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:28, kAddQueue), kFlagWrite(%table:28, +), kVecAppend(%table:28, kAdditionSet)}
    args: table=%table:28
op.38 kFrontierFilter sign=- ctx=seed stratum=2 deferral=immediate
    reads: NetDeleted(%table:28)
    effects: {kVecDrain(%table:28, kOverdeleteSet), kVecAppend(%table:28, kNetRemoval)}
    args: table=%table:28
op.39 kFrontierFilter sign=+ ctx=seed stratum=2 deferral=immediate
    reads: NetAdded(%table:28)
    effects: {kVecDrain(%table:28, kAdditionSet), kVecAppend(%table:28, kNetAddition)}
    args: table=%table:28
op.0 kGroupUpdate sign=· ctx=seed stratum=3 sc#0
    agg=sum_i32 prov=kOver algebra=kInvertible agg_table=%table:4
    group@{X:i32} summary@{BX_Weight:i32} config=0 input=%table:28
    effects: {kVecDrain(%table:28, kNetRemoval), kStateFold(%table:4, -), kVecDrain(%table:28, kNetAddition), kStateFold(%table:4, +), kStateEmit(%table:4), kStateOld(%table:4), kCounter(%table:4, -, NonRecursive), kInIReadFrozen(%table:4, InI, seed), kVecAppend(%table:4, kDeleteQueue), kCounter(%table:4, +, NonRecursive), kInIReadFrozen(%table:4, InI, seed), kVecAppend(%table:4, kAddQueue)}
    spine: —
    args: agg_table=%table:4 input=%table:28 statecell=sc#0
op.16 kClaimDrain sign=- ctx=seed stratum=4 form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:4, kDeleteQueue), kFlagWrite(%table:4, -), kVecAppend(%table:4, kOverdeleteSet)}
    args: table=%table:4
op.17 kClaimDrain sign=+ ctx=seed stratum=4 form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:4, kAddQueue), kFlagWrite(%table:4, +), kVecAppend(%table:4, kAdditionSet)}
    args: table=%table:4
op.18 kFrontierFilter sign=- ctx=seed stratum=4 deferral=immediate
    reads: NetDeleted(%table:4)
    effects: {kVecDrain(%table:4, kOverdeleteSet), kVecAppend(%table:4, kNetRemoval)}
    args: table=%table:4
op.19 kFrontierFilter sign=+ ctx=seed stratum=4 deferral=immediate
    reads: NetAdded(%table:4)
    effects: {kVecDrain(%table:4, kAdditionSet), kVecAppend(%table:4, kNetAddition)}
    args: table=%table:4
op.2 kGroupUpdate sign=· ctx=seed stratum=5 sc#1
    agg=count_i32 prov=kOver algebra=kInvertible agg_table=%table:8
    group@{X:i32} summary@{BX_Weight:i32} config=0 input=%table:28
    effects: {kVecDrain(%table:28, kNetRemoval), kStateFold(%table:8, -), kVecDrain(%table:28, kNetAddition), kStateFold(%table:8, +), kStateEmit(%table:8), kStateOld(%table:8), kCounter(%table:8, -, NonRecursive), kInIReadFrozen(%table:8, InI, seed), kVecAppend(%table:8, kDeleteQueue), kCounter(%table:8, +, NonRecursive), kInIReadFrozen(%table:8, InI, seed), kVecAppend(%table:8, kAddQueue)}
    spine: —
    args: agg_table=%table:8 input=%table:28 statecell=sc#1
op.20 kClaimDrain sign=- ctx=seed stratum=6 form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:8, kDeleteQueue), kFlagWrite(%table:8, -), kVecAppend(%table:8, kOverdeleteSet)}
    args: table=%table:8
op.21 kClaimDrain sign=+ ctx=seed stratum=6 form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:8, kAddQueue), kFlagWrite(%table:8, +), kVecAppend(%table:8, kAdditionSet)}
    args: table=%table:8
op.22 kFrontierFilter sign=- ctx=seed stratum=6 deferral=immediate
    reads: NetDeleted(%table:8)
    effects: {kVecDrain(%table:8, kOverdeleteSet), kVecAppend(%table:8, kNetRemoval)}
    args: table=%table:8
op.23 kFrontierFilter sign=+ ctx=seed stratum=6 deferral=immediate
    reads: NetAdded(%table:8)
    effects: {kVecDrain(%table:8, kAdditionSet), kVecAppend(%table:8, kNetAddition)}
    args: table=%table:8
op.6 kSeedFold sign=- ctx=seed stratum=7 src=%table:4 join_pivot
    effects: {kVecDrain(%table:4, kNetRemoval), kVecAppend($join-pivots.42)}
    spine: —
    args: src=%table:4 pivots=$join-pivots.42
op.7 kSeedFold sign=+ ctx=seed stratum=7 src=%table:4 join_pivot
    effects: {kVecDrain(%table:4, kNetAddition), kVecAppend($join-pivots.42)}
    spine: —
    args: src=%table:4 pivots=$join-pivots.42
op.8 kSeedFold sign=- ctx=seed stratum=7 src=%table:8 join_pivot
    effects: {kVecDrain(%table:8, kNetRemoval), kVecAppend($join-pivots.42)}
    spine: —
    args: src=%table:8 pivots=$join-pivots.42
op.9 kSeedFold sign=+ ctx=seed stratum=7 src=%table:8 join_pivot
    effects: {kVecDrain(%table:8, kNetAddition), kVecAppend($join-pivots.42)}
    spine: —
    args: src=%table:8 pivots=$join-pivots.42
op.32 kClaimDrain sign=- ctx=seed stratum=7 form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:23, kDeleteQueue), kFlagWrite(%table:23, -), kVecAppend(%table:23, kOverdeleteSet)}
    args: table=%table:23
op.33 kClaimDrain sign=+ ctx=seed stratum=7 form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:23, kAddQueue), kFlagWrite(%table:23, +), kVecAppend(%table:23, kAdditionSet)}
    args: table=%table:23
op.34 kFrontierFilter sign=- ctx=seed stratum=7 deferral=immediate
    reads: NetDeleted(%table:23)
    effects: {kVecDrain(%table:23, kOverdeleteSet), kVecAppend(%table:23, kNetRemoval)}
    args: table=%table:23
op.35 kFrontierFilter sign=+ ctx=seed stratum=7 deferral=immediate
    reads: NetAdded(%table:23)
    effects: {kVecDrain(%table:23, kAdditionSet), kVecAppend(%table:23, kNetAddition)}
    args: table=%table:23
op.14 kSeedFold sign=- ctx=seed stratum=8 src=%table:23 tgt=%table:17 class=NonRecursive
    effects: {kVecDrain(%table:23, kNetRemoval), kCounter(%table:17, -, NonRecursive), kInIReadFrozen(%table:17, InI, seed), kVecAppend(%table:17, kDeleteQueue)}
    spine: kFold(%table:17, -, NonRecursive)
    args: src=%table:23 tgt=%table:17
op.15 kSeedFold sign=+ ctx=seed stratum=8 src=%table:23 tgt=%table:17 class=NonRecursive
    effects: {kVecDrain(%table:23, kNetAddition), kCounter(%table:17, +, NonRecursive), kInIReadFrozen(%table:17, InI, seed), kVecAppend(%table:17, kAddQueue)}
    spine: kFold(%table:17, +, NonRecursive)
    args: src=%table:23 tgt=%table:17
op.28 kClaimDrain sign=- ctx=seed stratum=8 form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:17, kDeleteQueue), kFlagWrite(%table:17, -), kVecAppend(%table:17, kOverdeleteSet)}
    args: table=%table:17
op.29 kClaimDrain sign=+ ctx=seed stratum=8 form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:17, kAddQueue), kFlagWrite(%table:17, +), kVecAppend(%table:17, kAdditionSet)}
    args: table=%table:17
op.30 kFrontierFilter sign=- ctx=seed stratum=8 deferral=immediate
    reads: NetDeleted(%table:17)
    effects: {kVecDrain(%table:17, kOverdeleteSet), kVecAppend(%table:17, kNetRemoval)}
    args: table=%table:17
op.31 kFrontierFilter sign=+ ctx=seed stratum=8 deferral=immediate
    reads: NetAdded(%table:17)
    effects: {kVecDrain(%table:17, kAdditionSet), kVecAppend(%table:17, kNetAddition)}
    args: table=%table:17
op.12 kSeedFold sign=- ctx=seed stratum=10 src=%table:17 tgt=%table:32 class=NonRecursive
    effects: {kVecDrain(%table:17, kNetRemoval), kCounter(%table:32, -, NonRecursive), kInIReadFrozen(%table:32, InI, seed), kVecAppend(%table:32, kDeleteQueue)}
    spine: kFold(%table:32, -, NonRecursive)
    args: src=%table:17 tgt=%table:32
op.13 kSeedFold sign=+ ctx=seed stratum=10 src=%table:17 tgt=%table:32 class=NonRecursive
    effects: {kVecDrain(%table:17, kNetAddition), kCounter(%table:32, +, NonRecursive), kInIReadFrozen(%table:32, InI, seed), kVecAppend(%table:32, kAddQueue)}
    spine: kFold(%table:32, +, NonRecursive)
    args: src=%table:17 tgt=%table:32
op.40 kClaimDrain sign=- ctx=seed stratum=10 form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:32, kDeleteQueue), kFlagWrite(%table:32, -), kVecAppend(%table:32, kOverdeleteSet)}
    args: table=%table:32
op.41 kClaimDrain sign=+ ctx=seed stratum=10 form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:32, kAddQueue), kFlagWrite(%table:32, +), kVecAppend(%table:32, kAdditionSet)}
    args: table=%table:32
op.42 kFrontierFilter sign=- ctx=seed stratum=10 deferral=immediate
    reads: NetDeleted(%table:32)
    effects: {kVecDrain(%table:32, kOverdeleteSet), kVecAppend(%table:32, kNetRemoval)}
    args: table=%table:32
op.43 kFrontierFilter sign=+ ctx=seed stratum=10 deferral=immediate
    reads: NetAdded(%table:32)
    effects: {kVecDrain(%table:32, kAdditionSet), kVecAppend(%table:32, kNetAddition)}
    args: table=%table:32
op.44 kCommitSweep sign=· ctx=seed band=9 flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:4, InI, seed), kFlagWrite(%table:4)}
    args: table=%table:4
op.45 kCommitSweep sign=· ctx=seed band=9 flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:8, InI, seed), kFlagWrite(%table:8)}
    args: table=%table:8
op.46 kCommitSweep sign=· ctx=seed band=9 flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:12, InI, seed), kFlagWrite(%table:12)}
    args: table=%table:12
op.47 kCommitSweep sign=· ctx=seed band=9 flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:17, InI, seed), kFlagWrite(%table:17)}
    args: table=%table:17
op.48 kCommitSweep sign=· ctx=seed band=9 flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:23, InI, seed), kFlagWrite(%table:23)}
    args: table=%table:23
op.49 kCommitSweep sign=· ctx=seed band=9 flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:28, InI, seed), kFlagWrite(%table:28)}
    args: table=%table:28
op.50 kCommitSweep sign=· ctx=seed band=9 flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:32, InI, seed), kFlagWrite(%table:32)}
    args: table=%table:32
op.51 kCommitSweep sign=· ctx=seed band=9 flavor=monotone publish_target=false
    effects: {kFlagWrite(%table:36)}
    args: table=%table:36
op.1 kStateSeal sign=· ctx=seed band=10 sc#0
    effects: {kStateFold(%table:4, sign=0)}
    args: agg_table=%table:4 statecell=sc#0
op.3 kStateSeal sign=· ctx=seed band=10 sc#1
    effects: {kStateFold(%table:8, sign=0)}
    args: agg_table=%table:8 statecell=sc#1
op.5 kStateSeal sign=· ctx=seed band=10 sc#2
    effects: {kStateFold(%table:12, sign=0)}
    args: agg_table=%table:12 statecell=sc#2

rounds:

deps:
  op.0 -> op.1 WAR epoch
  op.0 -> op.1 WAW epoch
  op.0 -> op.16 RAW epoch
  op.0 -> op.16 WAR epoch
  op.0 -> op.16 WAW epoch
  op.0 -> op.17 RAW epoch
  op.0 -> op.17 WAR epoch
  op.0 -> op.17 WAW epoch
  op.0 -> op.18 RAW epoch
  op.0 -> op.19 RAW epoch
  op.0 -> op.44 WAR epoch
  op.0 -> op.44 WAW epoch
  op.2 -> op.3 WAR epoch
  op.2 -> op.3 WAW epoch
  op.2 -> op.20 RAW epoch
  op.2 -> op.20 WAR epoch
  op.2 -> op.20 WAW epoch
  op.2 -> op.21 RAW epoch
  op.2 -> op.21 WAR epoch
  op.2 -> op.21 WAW epoch
  op.2 -> op.22 RAW epoch
  op.2 -> op.23 RAW epoch
  op.2 -> op.45 WAR epoch
  op.2 -> op.45 WAW epoch
  op.4 -> op.5 WAR epoch
  op.4 -> op.5 WAW epoch
  op.4 -> op.24 RAW epoch
  op.4 -> op.24 WAR epoch
  op.4 -> op.24 WAW epoch
  op.4 -> op.25 RAW epoch
  op.4 -> op.25 WAR epoch
  op.4 -> op.25 WAW epoch
  op.4 -> op.26 RAW epoch
  op.4 -> op.27 RAW epoch
  op.4 -> op.46 WAR epoch
  op.4 -> op.46 WAW epoch
  op.6 -> op.7 WAW epoch
  op.6 -> op.8 WAW epoch
  op.6 -> op.9 WAW epoch
  op.7 -> op.8 WAW epoch
  op.7 -> op.9 WAW epoch
  op.8 -> op.9 WAW epoch
  op.10 -> op.11 WAW epoch
  op.10 -> op.36 RAW epoch
  op.10 -> op.36 WAW epoch
  op.10 -> op.37 WAW epoch
  op.10 -> op.38 RAW epoch
  op.10 -> op.39 RAW epoch
  op.10 -> op.49 WAW epoch
  op.11 -> op.36 WAW epoch
  op.11 -> op.37 RAW epoch
  op.11 -> op.37 WAW epoch
  op.11 -> op.38 RAW epoch
  op.11 -> op.39 RAW epoch
  op.11 -> op.49 WAW epoch
  op.12 -> op.13 WAW epoch
  op.12 -> op.40 RAW epoch
  op.12 -> op.40 WAW epoch
  op.12 -> op.41 WAW epoch
  op.12 -> op.42 RAW epoch
  op.12 -> op.43 RAW epoch
  op.12 -> op.50 WAW epoch
  op.13 -> op.40 WAW epoch
  op.13 -> op.41 RAW epoch
  op.13 -> op.41 WAW epoch
  op.13 -> op.42 RAW epoch
  op.13 -> op.43 RAW epoch
  op.13 -> op.50 WAW epoch
  op.14 -> op.15 WAW epoch
  op.14 -> op.28 RAW epoch
  op.14 -> op.28 WAW epoch
  op.14 -> op.29 WAW epoch
  op.14 -> op.30 RAW epoch
  op.14 -> op.31 RAW epoch
  op.14 -> op.47 WAW epoch
  op.15 -> op.28 WAW epoch
  op.15 -> op.29 RAW epoch
  op.15 -> op.29 WAW epoch
  op.15 -> op.30 RAW epoch
  op.15 -> op.31 RAW epoch
  op.15 -> op.47 WAW epoch
  op.16 -> op.1 WAW epoch
  op.16 -> op.17 WAW epoch
  op.16 -> op.18 RAW epoch
  op.16 -> op.19 RAW epoch
  op.16 -> op.44 WAW epoch
  op.17 -> op.1 WAW epoch
  op.17 -> op.18 RAW epoch
  op.17 -> op.19 RAW epoch
  op.17 -> op.44 WAW epoch
  op.18 -> op.1 WAR epoch
  op.18 -> op.6 RAW epoch
  op.18 -> op.44 WAR epoch
  op.19 -> op.1 WAR epoch
  op.19 -> op.7 RAW epoch
  op.19 -> op.44 WAR epoch
  op.20 -> op.3 WAW epoch
  op.20 -> op.21 WAW epoch
  op.20 -> op.22 RAW epoch
  op.20 -> op.23 RAW epoch
  op.20 -> op.45 WAW epoch
  op.21 -> op.3 WAW epoch
  op.21 -> op.22 RAW epoch
  op.21 -> op.23 RAW epoch
  op.21 -> op.45 WAW epoch
  op.22 -> op.3 WAR epoch
  op.22 -> op.8 RAW epoch
  op.22 -> op.45 WAR epoch
  op.23 -> op.3 WAR epoch
  op.23 -> op.9 RAW epoch
  op.23 -> op.45 WAR epoch
  op.24 -> op.5 WAW epoch
  op.24 -> op.25 WAW epoch
  op.24 -> op.26 RAW epoch
  op.24 -> op.27 RAW epoch
  op.24 -> op.46 WAW epoch
  op.25 -> op.5 WAW epoch
  op.25 -> op.26 RAW epoch
  op.25 -> op.27 RAW epoch
  op.25 -> op.46 WAW epoch
  op.26 -> op.5 WAR epoch
  op.26 -> op.10 RAW epoch
  op.26 -> op.46 WAR epoch
  op.27 -> op.5 WAR epoch
  op.27 -> op.11 RAW epoch
  op.27 -> op.46 WAR epoch
  op.28 -> op.29 WAW epoch
  op.28 -> op.30 RAW epoch
  op.28 -> op.31 RAW epoch
  op.28 -> op.47 WAW epoch
  op.29 -> op.30 RAW epoch
  op.29 -> op.31 RAW epoch
  op.29 -> op.47 WAW epoch
  op.30 -> op.12 RAW epoch
  op.30 -> op.47 WAR epoch
  op.31 -> op.13 RAW epoch
  op.31 -> op.47 WAR epoch
  op.32 -> op.33 WAW epoch
  op.32 -> op.34 RAW epoch
  op.32 -> op.35 RAW epoch
  op.32 -> op.48 WAW epoch
  op.33 -> op.34 RAW epoch
  op.33 -> op.35 RAW epoch
  op.33 -> op.48 WAW epoch
  op.34 -> op.14 RAW epoch
  op.34 -> op.48 WAR epoch
  op.35 -> op.15 RAW epoch
  op.35 -> op.48 WAR epoch
  op.36 -> op.37 WAW epoch
  op.36 -> op.38 RAW epoch
  op.36 -> op.39 RAW epoch
  op.36 -> op.49 WAW epoch
  op.37 -> op.38 RAW epoch
  op.37 -> op.39 RAW epoch
  op.37 -> op.49 WAW epoch
  op.38 -> op.0 RAW epoch
  op.38 -> op.2 RAW epoch
  op.38 -> op.49 WAR epoch
  op.39 -> op.0 RAW epoch
  op.39 -> op.2 RAW epoch
  op.39 -> op.49 WAR epoch
  op.40 -> op.41 WAW epoch
  op.40 -> op.42 RAW epoch
  op.40 -> op.43 RAW epoch
  op.40 -> op.50 WAW epoch
  op.41 -> op.42 RAW epoch
  op.41 -> op.43 RAW epoch
  op.41 -> op.50 WAW epoch
  op.42 -> op.50 WAR epoch
  op.43 -> op.50 WAR epoch
  op.44 -> op.1 WAW epoch
  op.45 -> op.3 WAW epoch
  op.46 -> op.5 WAW epoch
  op.52 -> op.51 WAW epoch

census: kCrossover=0 kProductArm=0 kSeedFold=10 kFixpointFire=0 kChainFold=0 kClaimDrain=14 kRetire=0 kRederive=0 kFrontierFilter=14 kCommitSweep=8 kNegateGate=0 kPivotAssemble=0 kIngestFold=1 kGroupUpdate=3 kStateSeal=3
```

## §2 Derivation notes (every op traced to its minting code path + .ir line)

### §2.0 The table / stratum map (from average_weight.dot + .ir create blocks)

The 8 tables (`%table:<DataTable::Id()>`, the authoritative id the .ir prints).
STRATUM from the .dot node labels; differential = has claim-drains in the .ir.

    %table:36 [i32,i32,mut]  edge_weight base (RECEIVE add_edge)  STRATUM 0  MONOTONE
    %table:12 [i32,i32,mut]  edge_weight KV index (@recompute)    STRATUM 1  differential
    %table:28 [i32,i32]      TUPLE X,BX_Weight (edge_weight->agg) STRATUM 2  differential
    %table:4  [i32,i32]      sum_i32 aggregate (@invertible)      STRATUM 3/4  differential
    %table:8  [i32,i32]      count_i32 aggregate (@invertible)    STRATUM 5/6  differential
    %table:23 [i32,i32,i32]  JOIN X,Sum,Count                     STRATUM 7  differential
    %table:17 [i32,i32,i32,i32] MAP div_i32 -> Sum,Count,Avg,X    STRATUM 8  differential
    %table:32 [i32,i32]      average_incoming_weight MATERIALIZE  STRATUM 9/10 differential

Note: STRATUM `a/b` on t4, t8, t32 is GU-stratum/drain-stratum — the table's
own GROUP_UPDATE (or, for t32, its SEED_FOLDs) sits at group_update_stratum
= `a`, one stratum BELOW its own claim-drains/frontier-filters at
drain_stratum = owner_stratum = `b`, because each of these three tables
carries a monotone TUPLE-alias member view one stratum above its
aggregate/materialize view (same model/EQ-SET). Every other table has a
single member view, so its GU/SEED_FOLD and its drains share one stratum.

Note: %table:36 is MONOTONE (add_edge is add-only; the .ir folds it with a
single `+nonrecursive` update-count in `^entry:41` and a monotone `commit-sweep`
Seal at the tail — no claim-drain). It is the ONLY monotone table; the other 7
are differential. None is induction-owned (the program is fully ACYCLIC — no
SCC, no FIXPOINT_FIRE, no PIVOT_ASSEMBLE, no round vecs, no CHAIN_FOLD).

### §2.1 The op inventory in MINT order (construction-code walk, BuildDRInventory)

BuildDRInventory (DeltaRel.cpp:843) walks in this fixed phase order; each phase
is cited. NO crossovers (no negates), NO product arms (the join is a pivot join,
not a 0-pivot @product), NO fixpoint fires / pivot assembles / chain folds
(acyclic), NO eager negate gates.

MINT PHASE 3 — group updates (DeltaRel.cpp:1057-1098). `query.Aggregates()`
first (sum, count), THEN `query.KVIndices()` (edge_weight). BuildGroupUpdateOps
(638) pushes GROUP_UPDATE then STATE_SEAL per view; statecell_id `sc` =
statecells.size() at mint (667) → mint order fixes sc#:
  op.0  kGroupUpdate  sum   sc#0 agg_table=%table:4  input->%table:28  [.ir:187]
  op.1  kStateSeal    sum   sc#0 agg_table=%table:4                    [seal — §2.4]
  op.2  kGroupUpdate  count sc#1 agg_table=%table:8  input->%table:28  [.ir:210]
  op.3  kStateSeal    count sc#1 agg_table=%table:8
  op.4  kGroupUpdate  KV    sc#2 agg_table=%table:12 input->%table:36  [.ir:136]
  op.5  kStateSeal    KV    sc#2 agg_table=%table:12
  (BuildGroupUpdateOps walks the summarized input through table-less plumbing to
   the first table-owning view, 653-662: sum/count read %table:28 through the
   TUPLE chain; the KV reads %table:36. input_table asserts non-null + non-alias,
   663-665.)

MINT PHASE 5 — SEED_FOLD (DeltaRel.cpp:1231-1360), per branch × available sign.
Branches minted PHASE 4 (1108-1150) iterating impl->tables' source tables, each
SuffixesOf the member views; the aggregate/KV views are chain-BREAKERS
(SuffixesOf stops, 194-263), so NO branch traverses %table:12/%table:4/%table:8
as an interior edge, and %table:36->%table:12 / %table:28->agg produce NO seed
fold (the group-update's band-(a) drain consumes those frontiers instead). The
10 surviving seed folds (source differential & not induction-owned ⇒ BOTH signs,
`-` at k=0 then `+`, 1341-1342):
  op.6  kSeedFold −  src=%table:12 -> tgt=%table:28   [.ir:159]
  op.7  kSeedFold +  src=%table:12 -> tgt=%table:28   [.ir:164]
  op.8  kSeedFold −  src=%table:23 -> tgt=%table:17   (div_i32 MAP on chain) [.ir:279]
  op.9  kSeedFold +  src=%table:23 -> tgt=%table:17   [.ir:286]
  op.10 kSeedFold −  src=%table:17 -> tgt=%table:32   [.ir:309]
  op.11 kSeedFold +  src=%table:17 -> tgt=%table:32   [.ir:314]
  op.12 kSeedFold −  src=%table:4  -> JOIN (join_pivot, no tgt) [.ir:232-233]
  op.13 kSeedFold +  src=%table:4  -> JOIN (join_pivot)         [.ir:235-236]
  op.14 kSeedFold −  src=%table:8  -> JOIN (join_pivot)         [.ir:238-239]
  op.15 kSeedFold +  src=%table:8  -> JOIN (join_pivot)         [.ir:241-242]
  (PREDICTED mint order among branches is impl->tables order × member views ×
   SuffixesOf order; the exact op-index assignment is pinned by the first run.
   join_pivot seeds carry NO fold leaf / no target queue def, only a pivot
   append — FillSeedFoldArm, "join-terminal seed appends pivots".)

The JOIN itself (the `join-tables` section walk into %table:23, .ir:244-255) is
NOT a DROp — it is a `DRJoin` (flow.joins) that `LowerSectionWalk`
(Stratum.cpp:1512-1554) emits at lowering time. The join-terminal SEED_FOLDs
(op.12-15) only produce the pivots the section walk consumes. So the join has NO
`op.<idx>`; per the ratified spec it is a FIRST-CLASS `join.0` section (§1, F-2),
not an inline comment.

MINT PHASE 7 — CLAIM_DRAIN / RETIRE / REDERIVE / FRONTIER_FILTER
(DeltaRel.cpp:1594 `for (TABLE *table : impl->tables)`), per differential
non-induction table. All 7 differential tables are ACYCLIC ⇒ each mints exactly:
claim−, claim+, filter−, filter+ (single-pass / immediate, 1653-1656). mint_claim
then mint_filter. C-2 FIX — table iteration is impl->tables ID-ASCENDING
(4,8,12,17,23,28,32), NOT the v1-draft's 12,28,4,8… (which matched nothing in the
code). PREDICTED indices, id-ascending mint (Option A):
  op.16..19  %table:4   claim− claim+ filter− filter+   [.ir:192,197,201,205]
  op.20..23  %table:8   claim− claim+ filter− filter+   [.ir:215,220,224,228]
  op.24..27  %table:12  claim− claim+ filter− filter+   [.ir:141,146,150,154]
  op.28..31  %table:17  claim− claim+ filter− filter+   [.ir:291,296,300,304]
  op.32..35  %table:23  claim− claim+ filter− filter+   [.ir:259,264,268,272]
  op.36..39  %table:28  claim− claim+ filter− filter+   [.ir:169,174,178,182]
  op.40..43  %table:32  claim− claim+ filter− filter+   [.ir:319,324,328,332]
  (NOTE: the .ir line order is STRATUM order, not mint order — %table:12 (stratum
   1) lowers first at .ir:141 though it mints 3rd (op.24..27). The EMISSION §1
   op-line order is likewise stratum/pinned_order; the op-INDEX is mint order, so
   the §1 labels are non-monotone by design — see §2.4.)

MINT PHASE 8 — COMMIT_SWEEP (DeltaRel.cpp:1660-1706). One per differential table
(flavor=differential); one Seal per MONOTONE boundary table with a delta-vec
entry. Iterates impl->tables ID-ASCENDING (:1672 `for (TABLE *table :
impl->tables)`), PREDICTED:
  op.44..50  COMMIT_SWEEP differential: %table:4,8,12,17,23,28,32
  op.51      COMMIT_SWEEP monotone (Seal): %table:36            [.ir:342]
  (C-2/C-1 FIX: the MINT order IS id-ascending 4,8,12,17,23,28,32,36 — the
   SAME order the .ir textual sweeps show (.ir:335-342), and — POST-T2b.0 — the
   SAME order pinned_order emits (op_table_id = table id ascending, no pointer).
   So mint = .ir = pinned_order for the commit band; the v1 draft's claim of a
   "DIFFERENT textual order via a pointer key" is retired. §2.4.)

MINT PHASE 11 — INGEST_FOLD (DeltaRel.cpp:1821-1860). `query.IOs()`: add_edge is
the sole message; its receive is MONOTONE (not CanReceiveDeletions), table
%table:36 non-null ⇒ ONE MakeMonotoneIngestFold (813): single +1,
is_explicit=false, stage1=false, role=MonotoneIngestRoleDR (kNetAddition — a cut
successor feeding the KV group-update), counter klass=EmissionDerivClass.
  op.52  kIngestFold + %table:36 (add_edge)  [.ir:75, in ^entry:41]
  (Emitted in ^entry, not ^flow: LowerIngestFold plants it at the original walk
   position — the ingest-cursor hole the eager descent fills, CLAUDE.md.)

TOTAL: 53 ops (indices 0..52). Census (per-kind, DeltaRel.cpp:2854-2879 mold):
  kGroupUpdate=3  kStateSeal=3  kSeedFold=10  kClaimDrain=14  kFrontierFilter=14
  kCommitSweep=8  kIngestFold=1
  (14 = 7 tables × 2 signs for claim; 14 = 7 × 2 for filter; 8 = 7 diff + 1 mono
   commit sweeps.) All other kinds = 0.

### §2.2 The vec inventory in MINT order (DeltaRel.cpp:878-908 + 1166)

Per differential non-induction-owned table, MintTableVec (606) pushes SIX vecs
in this fixed order: kDeleteQueue, kAddQueue, kOverdeleteSet, kAdditionSet,
kNetRemoval, kNetAddition (882-893). No claimed-* round vecs (no SCC, 901). The
7 differential tables × 6 = 42 vecs; the join mints ONE kJoinPivots vec in the
branches/joins dedup loop (1166-1173) AFTER all per-table vecs = 43 vecs total.
%table:36 (monotone) mints NO phase vec — its net-additions parks via
TableDeltaVector at lower time (the ingest fold's append has no DRVec def, 1855).
The SAME absent-vec situation recurs on op.4 (KV GROUP_UPDATE, §2.1): its
band-a `kVecDrain(%table:36, kNetRemoval)` / `kVecDrain(%table:36,
kNetAddition)` effects (BuildGroupUpdateOps unconditionally drains both signs
of input_table=t36) reference the SAME never-minted t36 vecs the ingest
fold's append references — faithful to the model, not a dump-body error, but
LOUD-worthy: `flow.TableVec(t36, kNetRemoval/kNetAddition)` behaviour
(sentinel-return vs. mint-on-demand) needs a look at implementation time,
since it determines whether op.4's two kVecDrains — and the ingest fold's
kVecAppend — render a `$role.idx` vec ref at all, or an absent/sentinel form.

vec-block index assignment (C-2 FIX: impl->tables ID-ASCENDING for the per-table
six-packs; the join pivot vec last):
  vec.0..5    %table:4   {delQ,addQ,overdel,addSet,netRem,netAdd}
  vec.6..11   %table:8   {…}
  vec.12..17  %table:12  {…}
  vec.18..23  %table:17  {…}
  vec.24..29  %table:23  {…}
  vec.30..35  %table:28  {…}
  vec.36..41  %table:32  {…}
  vec.42      join-pivots (shape=kIdCols, uniq=kSortUniqueAtDrain, debug_table=∅)

DEF/USE edges (op-index vectors on DRVec, populated at mint):
  - Each GROUP_UPDATE registers its agg table's delQ+addQ .defs (744-747): so
    %table:4 delQ/addQ.defs=[op.0]; %table:8=[op.2]; %table:12=[op.4].
  - Each head-chain SEED_FOLD registers its target's sign queue .def (1355-1357):
    %table:28 delQ.defs+=[op.6], addQ.defs+=[op.7]; %table:17 delQ+=[op.8],
    addQ+=[op.9]; %table:32 delQ+=[op.10], addQ+=[op.11]. Join-pivot seeds
    (op.12-15) register NO queue def (join_pivot branch, 1355 guard).
  - Each CLAIM_DRAIN drains its table's del/add queue (a USE) and appends the
    overdelete/addition SET; each FRONTIER_FILTER drains the SET and appends the
    net-removal/net-addition frontier; the next stratum's SEED_FOLD/GROUP_UPDATE
    drains that frontier. These use-edges are DERIVED by the linearizer's
    effect→vec matching (dep-edge derivation, DeltaRel.cpp:3472ff), not recorded
    at mint — the dump renders def=[…]/use=[…] from the final DRVec.defs/uses.
  NOTE (spec §2.3 correction 6): loop-carriage is a DRDep attribute
  (loop_carried, DeltaRel.h:613), NOT a vec field. This program has NO
  loop_carried edge (acyclic — no round vecs), so the deps section prints
  scope=epoch on every edge; carried=… never appears.

### §2.3 EFFECT sets per op kind (DeltaRel.h:73-84 EffKind; the tagged-union
###       live fields per spec §2.3 correction 4)

GROUP_UPDATE (BuildGroupUpdateOps 689-740), effects in push order:
  band (a), per sign −1 then +1: kVecDrain(input_table, netRemoval|netAddition)
                                 kStateFold(agg_table, sign)
  band (b): kStateEmit(agg_table)  kStateOld(agg_table)
            per sign −1 then +1: kCounter(agg_table, sign, NonRecursive)
                                 kInIReadFrozen(agg_table, InI, seed)
                                 kVecAppend(agg_table, delQ|addQ)
  → the effect multiset: {2 kVecDrain, 2 kStateFold, 1 kStateEmit, 1 kStateOld,
    2 kCounter, 2 kInIReadFrozen, 2 kVecAppend} (the V-AGG-EFFECT totality).

STATE_SEAL (749-760): EXACTLY {1 kStateFold(agg_table, sign=0)} — the
sealed:=working global:rmw swap. No vec, no counter (store-internal).

SEED_FOLD head-chain (FillSeedFoldArm): {kVecDrain(src, netRem|netAdd),
  kCounter(tgt, sign, klass), kInIReadFrozen(tgt, InI, seed),
  kVecAppend(tgt, delQ|addQ)} + one kFlagRead per negate gate (none here).
  klass=RuleClass(tgt,{src})=kNonRecursive (acyclic).
SEED_FOLD join-pivot (op.12-15): {kVecDrain(src, netRem|netAdd),
  kVecAppend(join-pivots)} — NO counter, NO crossing, NO fold leaf.

CLAIM_DRAIN acyclic (mint_claim single-pass, 1530-1548):
  {kVecDrain(table, delQ|addQ), kFlagWrite(table, sign), kVecAppend(table,
   overdeleteSet|additionSet)}; claim_gate = kDelGateCnrNonPositive (−) /
   kAddGateTotalPositive (+); claim_form=kSinglePass. (No dual-append/clear —
   those are in-round only.)
FRONTIER_FILTER immediate (mint_filter 1566ff):
  {kVecDrain(table, overdeleteSet|additionSet), … kVecAppend(table,
   netRemoval|netAddition)}; deferral=kImmediate.
COMMIT_SWEEP differential (1690-1705): {kInIReadFrozen(table, InI, seed),
  kFlagWrite(table)}; sweep_flavor=kDifferential; publish_target=false (no
  @differential transmit — average_incoming_weight is a query, not a transmit).
COMMIT_SWEEP monotone Seal (1679-1687): {kFlagWrite(table)};
  sweep_flavor=kMonotone.
INGEST_FOLD monotone (MakeMonotoneIngestFold 828-841): {kCounter(table, +1,
  EmissionDerivClass), kVecAppend(table, kNetAddition)}; ingest_is_explicit=false,
  ingest_stage1=false, ingest_sign=+1.

### §2.4 Emission ORDER = pinned_order (the band comparator, 3354-3464), POST-T2b.0

pinned_order is a Kahn topological linearization (:3702-3739) whose ready-set
tie-break is `key_less` (:3457) over Key{lead, stratum, band, table_id, sign,
ctor}. lead: 0=eager-gate/ingest-fold, 1=phase, 2=commit-sweep/seal. band
(op_band 3354): GROUP_UPDATE/SEED_FOLD=0, single-pass CLAIM=1, immediate
FILTER=2, COMMIT_SWEEP=9. STATE_SEAL trails at sentinel stratum max+1, band 10;
COMMIT_SWEEP at sentinel stratum max+1, band 9 (key_of 3445-3453).

C-1 / T2b.0 (spec §2.0, decision (b2)): `op_table_id` (3387-3394) is HARDENED —
`return t ? uintptr_t(t->id) + 1u : 0u;` — so the level-4 tie-break is the
table's DETERMINISTIC DataTable::Id() shifted +1 (the `-first-id` wraparound
sentinel-collision fix; the same id the .ir prints, just offset), null→0 (real
ids ≥3 so shifted ids ≥4, disjoint from the null sentinel by construction). The
`reinterpret_cast<uintptr_t>(t)` pointer key is GONE.
Consequence: within a (lead,stratum,band) cell, ops order by table id ASCENDING,
then sign − before +, then ctor. The 8 commit sweeps (band 9) emit 4,8,12,17,23,
28,32,36; the 3 seals (band 10) emit sc#0(t4),sc#1(t8),sc#2(t12). NO pointer
order anywhere in the dump — the golden is allocation-stable.

op_stratum (3276): GROUP_UPDATE→group_update_stratum (KV=1, sum=3, count=5);
CLAIM/FILTER→drain_stratum(table); SEED_FOLD→branch_stratum[seed_branch].
drain_stratum(table) = owner_stratum = the MAX spec-stratum over ALL member
views of that table (DeltaRel.cpp:2098-2103, 2115) — NOT always equal to
group_update_stratum. Three tables (t4, t8, t32) carry a monotone TUPLE-alias
member view ONE stratum above their aggregate/materialize view (same
model/EQ-SET; confirmed in the .dot), so their CLAIM/FILTER ops render one
stratum ABOVE their own GROUP_UPDATE/SEED_FOLD: t4's op.0 (GU) is stratum 3
but op.16-19 (its claims/filters) are stratum 4; t8's op.2 (GU) is stratum 5
but op.20-23 are stratum 6; t32's seed folds op.10-11 and claims/filters
op.40-43 are ALL stratum 10 (branch_stratum lifts to owner_stratum(t32)=10 via
ready_after(t17)=9). This reproduces the .ir `^flow` order EXACTLY (traced
line-by-line against the .ir above). The INGEST_FOLD is lead-0 → sorts before
all phase ops, but LOWERS into `^entry:41` (not `^flow`) because
LowerIngestFold plants it at the walk position; the dump renders it FIRST
(lead-0), matching pinned_order, with a `; lowered in ^entry (ingest-cursor
hole)` note.

Note (why the op-INDEX labels in §1 are non-monotone): op ids are the dense MINT
index into flow.ops; the §1 op LINE order is pinned_order (stratum/band). Because
mint order (phase order: group-updates, then seeds, then claims/filters
id-ascending, then sweeps, then ingest) differs from stratum order, the §1 labels
jump (e.g. stratum 1's %table:12 claims are op.24..27, stratum 4's %table:4
claims are op.16..19 — one stratum ABOVE op.0's GROUP_UPDATE at stratum 3, per
the owner_stratum split above). This is correct and intended.
## §3 Open questions / spec frictions (LOUD)

F-1 (RESOLVED by T2b.0 — was HIGH) — THE POINTER TIE-BREAK IN pinned_order. The
band comparator's `op_table_id` (DeltaRel.cpp:3387-3394) returned
`reinterpret_cast<uintptr_t>(table)`, used at key_less level 4 (:3461) — the
pointer-order anti-pattern the (F) det_seq substrate fixed on the DataFlow side.
The critique promoted this to CRITICAL (C-1): the COMMIT_SWEEP band-9 cell holds
all 8 sweeps at one (lead=2,stratum=max+1,band=9) key, tie-broken ONLY by the
pointer, and the 3 seals likewise at band 10 — so their dump emission order was
pointer-derived and a golden blessed from it would churn on any allocator change.
RESOLUTION: the ratified spec's T2b.0 (decision (b2)) HARDENS op_table_id to
`t ? uintptr_t(t->id) + 1u : 0u` BEFORE the dump lands (a HARD precondition — the E-64 null-guard;
`op.fire_table` is null for several op kinds and a bare `t->id` null-derefs). With
the id key, the commit sweeps emit id-ascending 4,8,12,17,23,28,32,36 and the
seals sc#0/sc#1/sc#2 (t4,t8,t12) — DETERMINISTIC, allocation-stable. This artifact
now renders exactly that post-T2b.0 order (§1, §2.4). Landing order is T2b.0 →
T2b; the deltarel golden is blessable ONLY after T2b.0 (else it bakes pointer
order — the (F) anti-pattern). The critique's "render pinned_order OR say the band
sorts by id" dichotomy is answered: the band DOES sort by id, and that IS
pinned_order post-T2b.0.

F-2 (RESOLVED — was MED) — JOINS / BRANCHES are now FIRST-CLASS. The ratified
spec §2.3 makes `join.<idx>` / `branch.<idx>` first-class sections
(flow.joins / flow.branches in vector order). This revision renders both between
the vec block and the ops (branch.0..4 + join.0), and the stratum-7 op block
carries only a one-line back-reference to join.0 (no inline `;; join …`
substrate comment). average_weight has ONE DRJoin (%table:4⋈%table:8 on X, NOT a
DROp — lowered by LowerSectionWalk, Stratum.cpp:1512-1554) and ~5 branches. NOTE:
the branch inventory (paths through table-less plumbing) is derived by the §1.4
memoized worklist; branch.0..4 above is the STRUCTURALLY-certain source→terminal
set, but the exact branch COUNT and mint order (a diamond of plumbing produces
per-path multiplicity, DeltaRel.h:281-285) I could not fully enumerate by hand —
see F-11. DRRound.body_ops/output_ops (the `; substrate (unread by lowering)`
banner) is UNTESTED by this witness: average_weight has ZERO rounds (acyclic).

F-3 (MED, format) — THE `vec` LINE SHAPE is under-specified. Spec §2.3 line 2:
`vec $<role>.<idx> <shape> def=[...] use=[...]`. I chose `$<role>.<idx>` naming
mirroring the .ir's `$<kind>:<id>` VectorKind sigils (consolidated §1.B ".ir
NAMING" — but note the .ir uses `:` not `.`; the spec's dump uses `.` for op/vec
ids per §2.3 line 2). I rendered `<shape>` as `<ids %table:N>` / `<id-cols>`
(ElementShape kIds/kIdCols) and added `uniq=<contract>`. The spec does not
enumerate the shape rendering; correction 6 says "Vec lines print
shape/role/uniq/def/use". I included all four. OPEN: whether the role goes in the
sigil name (`$net-removal.4`) or as a separate `role=` field, and whether the
debug_table cross-ref (`%table:N`) belongs in `<shape>` or its own field. I put
it in `<shape>` since ElementShape kIds means "row ids INTO a table" and the
table is the referent.

F-4 (RESOLVED — was LOW) — MODULE NAME. The v1 draft rendered
`deltarel average_weight`, requiring a basename/fallback decision. The ratified
spec §2.3 line-1 rule settles it: the header is EXACTLY `deltarel`, NO module
name (matching the `.df` bare `dataflow` header, and side-stepping the
"no ParsedModule::Name() in scope at the emit point" problem the critique noted).
This revision renders the bare header.

F-5 (LOW, effect rendering) — THE GROUP_UPDATE / SEED_FOLD `reads:` LINE. Spec
§2.3 line "reads: <Pred spellings>". A GROUP_UPDATE's only membership-predicate
read is its emit_touched `kInIReadFrozen(InI)` crossing; its band-(a) folds are
kStateFold (not a Pred read) and its drains are kVecDrain. I rendered `reads:
InI(%table:N)` (the one crossing) and put everything else under `effects:`. This
matches the witness-target §2.6 shape but the spec doesn't say whether kStateEmit
/kStateOld (frozen/working reads) should ALSO surface on `reads:` — they are
EffKind members, not Pred reads, so I kept them in `effects:`. OPEN for the owner.
RESIDUAL (v3.1 pin (p4)): the spec now rules `reads:` renders Pred spellings
ONLY and kInIReadFrozen NEVER surfaces there (effects: only). Taken strictly,
this pin would ALSO strip the `reads: InI(%table:N)` lines still standing on
every GROUP_UPDATE (op.0/op.2/op.4) and SEED_FOLD (op.6/7/8/9/10/11) below —
this critique round's A3 fix scoped the cleanup to the one demonstrably stray
commit-sweep instance (op.44) and left these GU/SEED_FOLD lines as-is pending
an explicit owner-scoped pass; they are the same defect and should be swept in
the same edit that resolves this note.

F-6 (LOW, ordering) — INGEST_FOLD lead-0 vs its ^entry lowering. op.52 sorts
FIRST in pinned_order (lead-0, key_of 3442-3444) but LOWERS into `^entry:41`, not
`^flow`. The dump walks pinned_order, so it renders op.52 first — which is
faithful to the DR model but does NOT match the .ir's textual position (the
ingest is in a different proc). I flagged it inline (`; lowered in ^entry`). The
spec should confirm the dump follows pinned_order (model order) rather than
lowered-proc order — I assumed pinned_order per spec §2.3 correction 2.

F-7 (INFO, predicted indices) — every `op.<idx>` / `vec.<idx>` integer is
ILLUSTRATIVE-PREDICTED from the construction walk (§2.1/§2.2). The three that are
STRUCTURALLY load-bearing and I'm CONFIDENT of: the sc# statecell numbering
(sc#0=sum, sc#1=count, sc#2=KV — mint order Aggregates-then-KVIndices, 1057/1085,
CONFIRMED against .ir sc# labels 136/187/210) and the effect-set contents per
kind (traced to BuildGroupUpdateOps/FillSeedFoldArm/mint_claim/mint_filter line
by line). The op-INDEX assignment (0..52) depends on impl->tables iteration order
which I could not read directly — the dump's ids will differ if that order
differs from my assumption; the STRUCTURE, EMISSION ORDER, and effect sets are
what this artifact pins. THIS REVISION additionally pins that the impl->tables
order is ID-ASCENDING (4,8,12,17,23,28,32,36 — grounded against ground/average_
weight.ir's create blocks + the direct-mint-order commit-sweep tail), which fixes
the C-2 wrong-sequence and makes the vec/op/deps indices internally consistent.

F-8 (INFO, config-invariance — the (a3)-extended audit, CLEAN) — spec §2.3 asks:
before blessing, audit every rendered field for `#ifndef NDEBUG`-gated members
(the trap the .df producer= ruling fixed). RESULT: CLEAN. `grep -n NDEBUG
lib/DeltaRel/DeltaRel.h` returns only TWO hits (:714, :739) and BOTH are comment
prose ("survive NDEBUG"); NO struct member is `#ifndef NDEBUG`-gated. Every field
this dump renders — DROpKind, Ctx, sign, stratum, EffKind + tagged-union fields,
Pred, PlanKind, AggProvenance/Algebra, statecell_id, group_cols/summary_cols,
ClaimGate/Deferral/SweepFlavor, VecRole/ElementShape/UniqueContract, DRDep
kind/scope/loop_carried — is an always-compiled member. NOTE: DRVec.debug_table /
debug_kind (DeltaRel.h:252-253) and DRFlowGraph.tables/rounds carry "DEBUG
annotation only" / "dead-but-alive" COMMENTS but are NOT NDEBUG-gated fields —
they compile in every preset, so rendering %table:N from debug_table is config-
invariant. The deltarel golden class is therefore config-clean like .ir/.h.

F-9 (HIGH, determinism — NEW, supersedes C-3's completeness point) — THE deps
SECTION IS NOT BYTE-STABLE AS SPEC'D. The linearizer builds flow.dep_edges by
iterating TWO `std::unordered_map`s: `by_vec` (DeltaRel.cpp:3628, keyed on vec
index) and `by_flag` (:3656, keyed on `TABLE *`). `unordered_map` traversal is
HASH order, so the VECTOR ORDER of dep_edges is NOT deterministic across builds/
runs — a SECOND determinism hole, ORTHOGONAL to the op_table_id one T2b.0 fixes
(T2b.0 hardens the linearization KEY, not the dep-edge emission order). Spec §2.3
says the deps section renders "flow.dep_edges VECTOR ORDER, EXHAUSTIVE" — but that
vector order is itself unstable, so a deps byte-golden blessed from it would flake.
REQUIRED FIX (must be pinned in the spec + implemented before any deltarel golden
carries deps): the dump MUST SORT dep_edges into a canonical order
(by (from, to, kind)) before rendering. The deps block in §1 IS that canonical
post-sort order (the desired bytes). Until the emitter sorts, the deps section is
UNBLESSABLE. (Note: this does not affect pinned_order itself — the Kahn ready-set
tie-break is key_less, deterministic post-T2b.0 — only the dep_edges vector.)

F-10 (MED, completeness) — THE FLAG-CLASS EDGE SET IS A FLOOR, NOT COMPLETE. The
deps §1 renders the full RAW def→use chain (derivable byte-exactly) plus the
STRUCTURALLY-certain filter→sweep WAR subset. But `by_flag` (:3656) enrolls EVERY
FlagAccess over each table's flag class — group-update counter writes (kCounter),
claim/filter FlagWrites, commit-sweep kInI reads/writes — and emits a WAW for
each write/write pair + a RAW/WAR for each write/read pair. The complete
per-table flag edge set (and its canonical sort position under F-9) I could not
enumerate by hand from the .ir; the rendered WAR block is the confirmed floor. A
byte-golden needs the emitter's actual FlagAccess enrollment — pin at
implementation time (dump the real dep_edges once, sort, diff against this floor).

F-11 (LOW, branch inventory) — branch.0..4 (§1) is the source→terminal set I am
confident of, but the exact flow.branches COUNT and mint order depend on the §1.4
memoized-worklist path multiplicity (a diamond of table-less plumbing yields one
branch PER DISTINCT PATH, DeltaRel.h:281-285). average_weight's plumbing is a
simple chain (no diamonds visible in the .ir), so 5 branches is the likely count,
but the first compiler run pins it. The SEED_FOLD count (10 = 5 branches × 2
signs) is CONFIRMED against the .ir; only the per-branch index labels are
illustrative.
