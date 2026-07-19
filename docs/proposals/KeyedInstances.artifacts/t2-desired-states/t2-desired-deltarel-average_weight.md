# T2 desired-state — `-deltarel-out` dump of average_weight.dr

## §0 Provenance

DESIRED-STATE WRITER deliverable, KeyedInstances epoch, branch `keyed-instances`
@ tip b577735e. This hand-writes the EXACT text the future `-deltarel-out`
`<PATH>` dump must emit for `tests/OptDiff/cases/average_weight.dr`, per the
BINDING spec `docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md` §2.3
(the fleet-reconciled DeltaRel dump format). average_weight is the GROUP_UPDATE
witness — a `@recompute` KV index (`edge_weight`) feeding two `@invertible`
summary aggregates (`sum_i32`, `count_i32`), joined and divided into a per-node
average. It exercises THREE `kGroupUpdate` + THREE `kStateSeal` ops (one KV +
two summary aggregates), the strongest R3 fixture.

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

GROUND TRUTH (evidence, all read this session):
  - SOURCE: `tests/OptDiff/cases/average_weight.dr`.
  - DATAFLOW: `/private/tmp/.../dump-inputs/average_weight.dot` (the stratum
    map — see §2 table below).
  - CONTROLFLOW: `/private/tmp/.../dump-inputs/average_weight.ir` (the lowered
    .ir — THE authority for op inventory + emission ORDER, since LowerDRFlow
    walks pinned_order; every DR op below is traced to its .ir consequence).
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
Module name = `average_weight` (the .dr basename; average_weight.dr declares no
`#module`, so the emitter's fallback label is the source basename — see §3).

```
deltarel average_weight

;; ============================ VECS (index order) ============================
;; 7 differential tables × 6 phase vecs + 1 join-pivots vec = 43.
;; %table:36 (monotone edge_weight base) owns NO phase vec (§2.2).
vec $delete-queue.0  <ids %table:12> uniq=sort-unique-at-drain def=[op.16] use=[op.16]
vec $add-queue.1     <ids %table:12> uniq=sort-unique-at-drain def=[op.17] use=[op.17]
vec $overdelete-set.2 <ids %table:12> uniq=multiset def=[op.16] use=[op.18]
vec $addition-set.3  <ids %table:12> uniq=multiset def=[op.17] use=[op.19]
vec $net-removal.4   <ids %table:12> uniq=multiset def=[op.18] use=[op.6]
vec $net-addition.5  <ids %table:12> uniq=multiset def=[op.19] use=[op.7]
vec $delete-queue.6  <ids %table:28> uniq=sort-unique-at-drain def=[op.6] use=[op.20]
vec $add-queue.7     <ids %table:28> uniq=sort-unique-at-drain def=[op.7] use=[op.21]
vec $overdelete-set.8 <ids %table:28> uniq=multiset def=[op.20] use=[op.22]
vec $addition-set.9  <ids %table:28> uniq=multiset def=[op.21] use=[op.23]
vec $net-removal.10  <ids %table:28> uniq=multiset def=[op.22] use=[op.0,op.2]
vec $net-addition.11 <ids %table:28> uniq=multiset def=[op.23] use=[op.0,op.2]
vec $delete-queue.12 <ids %table:4> uniq=sort-unique-at-drain def=[op.0] use=[op.24]
vec $add-queue.13    <ids %table:4> uniq=sort-unique-at-drain def=[op.0] use=[op.25]
vec $overdelete-set.14 <ids %table:4> uniq=multiset def=[op.24] use=[op.26]
vec $addition-set.15 <ids %table:4> uniq=multiset def=[op.25] use=[op.27]
vec $net-removal.16  <ids %table:4> uniq=multiset def=[op.26] use=[op.12]
vec $net-addition.17 <ids %table:4> uniq=multiset def=[op.27] use=[op.13]
vec $delete-queue.18 <ids %table:8> uniq=sort-unique-at-drain def=[op.2] use=[op.28]
vec $add-queue.19    <ids %table:8> uniq=sort-unique-at-drain def=[op.2] use=[op.29]
vec $overdelete-set.20 <ids %table:8> uniq=multiset def=[op.28] use=[op.30]
vec $addition-set.21 <ids %table:8> uniq=multiset def=[op.29] use=[op.31]
vec $net-removal.22  <ids %table:8> uniq=multiset def=[op.30] use=[op.14]
vec $net-addition.23 <ids %table:8> uniq=multiset def=[op.31] use=[op.15]
vec $delete-queue.24 <ids %table:23> uniq=sort-unique-at-drain def=[join] use=[op.32]
vec $add-queue.25    <ids %table:23> uniq=sort-unique-at-drain def=[join] use=[op.33]
vec $overdelete-set.26 <ids %table:23> uniq=multiset def=[op.32] use=[op.34]
vec $addition-set.27 <ids %table:23> uniq=multiset def=[op.33] use=[op.35]
vec $net-removal.28  <ids %table:23> uniq=multiset def=[op.34] use=[op.8]
vec $net-addition.29 <ids %table:23> uniq=multiset def=[op.35] use=[op.9]
vec $delete-queue.30 <ids %table:17> uniq=sort-unique-at-drain def=[op.8] use=[op.36]
vec $add-queue.31    <ids %table:17> uniq=sort-unique-at-drain def=[op.9] use=[op.37]
vec $overdelete-set.32 <ids %table:17> uniq=multiset def=[op.36] use=[op.38]
vec $addition-set.33 <ids %table:17> uniq=multiset def=[op.37] use=[op.39]
vec $net-removal.34  <ids %table:17> uniq=multiset def=[op.38] use=[op.10]
vec $net-addition.35 <ids %table:17> uniq=multiset def=[op.39] use=[op.11]
vec $delete-queue.36 <ids %table:32> uniq=sort-unique-at-drain def=[op.10] use=[op.40]
vec $add-queue.37    <ids %table:32> uniq=sort-unique-at-drain def=[op.11] use=[op.41]
vec $overdelete-set.38 <ids %table:32> uniq=multiset def=[op.40] use=[op.42]
vec $addition-set.39 <ids %table:32> uniq=multiset def=[op.41] use=[op.43]
vec $net-removal.40  <ids %table:32> uniq=multiset def=[op.40] use=[]
vec $net-addition.41 <ids %table:32> uniq=multiset def=[op.41] use=[]
vec $join-pivots.42  <id-cols> uniq=sort-unique-at-drain def=[op.12,op.13,op.14,op.15] use=[join]

;; ====================== OPS (pinned_order = emission) ======================

;; --- lead 0: ingest folds (lowered in ^entry, off the ^flow lattice) ---
op.52 kIngestFold sign=+ ctx=eager  stratum=0
    ; lowered in ^entry:41 (ingest-cursor hole; .ir:75). role=kNetAddition
    reads: —
    effects: {kCounter(%table:36, +, NonRecursive),
              kVecAppend(%table:36, kNetAddition)}
    spine: kFold(%table:36, +, NonRecursive)
    args: table=%table:36 message=add_edge/3 receive=<recv %table:36>

;; --- stratum 1: the KV index edge_weight (@recompute GROUP_UPDATE sc#2) ---
op.4  kGroupUpdate sign=· ctx=seed  stratum=1  sc#2
    agg=edge_weight prov=kKv algebra=kRecompute agg_table=%table:12
    group@{col:37(From),col:38(To)} summary@{col:39(Weight)} config=0 input=%table:36
    ; KV group/summary cols are in the INPUT (%table:36) space (InputKeyColumns/
    ; InputValueColumns, 1086-1092)
    reads: InI(%table:12)               ; the emit_touched crossings
    effects: {kVecDrain(%table:36, kNetRemoval), kStateFold(%table:12, -),
              kVecDrain(%table:36, kNetAddition), kStateFold(%table:12, +),
              kStateEmit(%table:12), kStateOld(%table:12),
              kCounter(%table:12, -, NonRecursive), kInIReadFrozen(%table:12, InI, seed),
              kVecAppend(%table:12, kDeleteQueue),
              kCounter(%table:12, +, NonRecursive), kInIReadFrozen(%table:12, InI, seed),
              kVecAppend(%table:12, kAddQueue)}
    spine: —   ; @recompute rescan is a store-internal reduce, no PlanTree
    args: agg_table=%table:12 input=%table:36 statecell=sc#2
op.16 kClaimDrain sign=- ctx=seed  stratum=1  form=single-pass gate=kDelGateCnrNonPositive
    reads: — ; TryClaimDel re-tests C_nr<=0 at dequeue
    effects: {kVecDrain(%table:12, kDeleteQueue), kFlagWrite(%table:12, -),
              kVecAppend(%table:12, kOverdeleteSet)}
    args: table=%table:12
op.17 kClaimDrain sign=+ ctx=seed  stratum=1  form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:12, kAddQueue), kFlagWrite(%table:12, +),
              kVecAppend(%table:12, kAdditionSet)}
    args: table=%table:12
op.18 kFrontierFilter sign=- ctx=seed  stratum=1  deferral=immediate
    reads: NetDeleted(%table:12)
    effects: {kVecDrain(%table:12, kOverdeleteSet), kVecAppend(%table:12, kNetRemoval)}
    args: table=%table:12
op.19 kFrontierFilter sign=+ ctx=seed  stratum=1  deferral=immediate
    reads: NetAdded(%table:12)
    effects: {kVecDrain(%table:12, kAdditionSet), kVecAppend(%table:12, kNetAddition)}
    args: table=%table:12

;; --- stratum 2: TUPLE %table:28 (edge_weight KV -> summarized agg input) ---
op.6  kSeedFold sign=- ctx=seed  stratum=2  src=%table:12 tgt=%table:28 class=NonRecursive
    reads: InI(%table:28)
    effects: {kVecDrain(%table:12, kNetRemoval), kCounter(%table:28, -, NonRecursive),
              kInIReadFrozen(%table:28, InI, seed), kVecAppend(%table:28, kDeleteQueue)}
    spine: kFold(%table:28, -, NonRecursive)
    args: branch=<%table:12..%table:28> src=%table:12 tgt=%table:28
op.7  kSeedFold sign=+ ctx=seed  stratum=2  src=%table:12 tgt=%table:28 class=NonRecursive
    reads: InI(%table:28)
    effects: {kVecDrain(%table:12, kNetAddition), kCounter(%table:28, +, NonRecursive),
              kInIReadFrozen(%table:28, InI, seed), kVecAppend(%table:28, kAddQueue)}
    spine: kFold(%table:28, +, NonRecursive)
    args: branch=<%table:12..%table:28> src=%table:12 tgt=%table:28
op.20 kClaimDrain sign=- ctx=seed  stratum=2  form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:28, kDeleteQueue), kFlagWrite(%table:28, -),
              kVecAppend(%table:28, kOverdeleteSet)}
    args: table=%table:28
op.21 kClaimDrain sign=+ ctx=seed  stratum=2  form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:28, kAddQueue), kFlagWrite(%table:28, +),
              kVecAppend(%table:28, kAdditionSet)}
    args: table=%table:28
op.22 kFrontierFilter sign=- ctx=seed  stratum=2  deferral=immediate
    reads: NetDeleted(%table:28)
    effects: {kVecDrain(%table:28, kOverdeleteSet), kVecAppend(%table:28, kNetRemoval)}
    args: table=%table:28
op.23 kFrontierFilter sign=+ ctx=seed  stratum=2  deferral=immediate
    reads: NetAdded(%table:28)
    effects: {kVecDrain(%table:28, kAdditionSet), kVecAppend(%table:28, kNetAddition)}
    args: table=%table:28

;; --- stratum 3: sum_i32 (@invertible GROUP_UPDATE sc#0), input %table:28 ---
op.0  kGroupUpdate sign=· ctx=seed  stratum=3  sc#0
    agg=sum_i32 prov=kOver algebra=kInvertible agg_table=%table:4
    group@{col:29(X)} summary@{col:30(BX_Weight)} config=0  input=%table:28
    ; group/summary cols are in the INPUT (%table:28) space (BuildGroupUpdateOps
    ; 1058-1073 uses InputGroupColumns/InputAggregatedColumns)
    reads: InI(%table:4)
    effects: {kVecDrain(%table:28, kNetRemoval), kStateFold(%table:4, -),
              kVecDrain(%table:28, kNetAddition), kStateFold(%table:4, +),
              kStateEmit(%table:4), kStateOld(%table:4),
              kCounter(%table:4, -, NonRecursive), kInIReadFrozen(%table:4, InI, seed),
              kVecAppend(%table:4, kDeleteQueue),
              kCounter(%table:4, +, NonRecursive), kInIReadFrozen(%table:4, InI, seed),
              kVecAppend(%table:4, kAddQueue)}
    spine: —   ; @invertible fold/unfold is the sum_i32_combine/uncombine reduction
    args: agg_table=%table:4 input=%table:28 statecell=sc#0
op.24 kClaimDrain sign=- ctx=seed  stratum=3  form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:4, kDeleteQueue), kFlagWrite(%table:4, -),
              kVecAppend(%table:4, kOverdeleteSet)}
    args: table=%table:4
op.25 kClaimDrain sign=+ ctx=seed  stratum=3  form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:4, kAddQueue), kFlagWrite(%table:4, +),
              kVecAppend(%table:4, kAdditionSet)}
    args: table=%table:4
op.26 kFrontierFilter sign=- ctx=seed  stratum=3  deferral=immediate
    reads: NetDeleted(%table:4)
    effects: {kVecDrain(%table:4, kOverdeleteSet), kVecAppend(%table:4, kNetRemoval)}
    args: table=%table:4
op.27 kFrontierFilter sign=+ ctx=seed  stratum=3  deferral=immediate
    reads: NetAdded(%table:4)
    effects: {kVecDrain(%table:4, kAdditionSet), kVecAppend(%table:4, kNetAddition)}
    args: table=%table:4

;; --- stratum 5: count_i32 (@invertible GROUP_UPDATE sc#1), input %table:28 ---
op.2  kGroupUpdate sign=· ctx=seed  stratum=5  sc#1
    agg=count_i32 prov=kOver algebra=kInvertible agg_table=%table:8
    group@{col:29(X)} summary@{col:30(BX_Weight)} config=0  input=%table:28
    ; SAME input space as sum (both aggregate over %table:28's net frontiers)
    reads: InI(%table:8)
    effects: {kVecDrain(%table:28, kNetRemoval), kStateFold(%table:8, -),
              kVecDrain(%table:28, kNetAddition), kStateFold(%table:8, +),
              kStateEmit(%table:8), kStateOld(%table:8),
              kCounter(%table:8, -, NonRecursive), kInIReadFrozen(%table:8, InI, seed),
              kVecAppend(%table:8, kDeleteQueue),
              kCounter(%table:8, +, NonRecursive), kInIReadFrozen(%table:8, InI, seed),
              kVecAppend(%table:8, kAddQueue)}
    spine: —   ; @invertible count_i32_combine/uncombine
    args: agg_table=%table:8 input=%table:28 statecell=sc#1
op.28 kClaimDrain sign=- ctx=seed  stratum=5  form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:8, kDeleteQueue), kFlagWrite(%table:8, -),
              kVecAppend(%table:8, kOverdeleteSet)}
    args: table=%table:8
op.29 kClaimDrain sign=+ ctx=seed  stratum=5  form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:8, kAddQueue), kFlagWrite(%table:8, +),
              kVecAppend(%table:8, kAdditionSet)}
    args: table=%table:8
op.30 kFrontierFilter sign=- ctx=seed  stratum=5  deferral=immediate
    reads: NetDeleted(%table:8)
    effects: {kVecDrain(%table:8, kOverdeleteSet), kVecAppend(%table:8, kNetRemoval)}
    args: table=%table:8
op.31 kFrontierFilter sign=+ ctx=seed  stratum=5  deferral=immediate
    reads: NetAdded(%table:8)
    effects: {kVecDrain(%table:8, kAdditionSet), kVecAppend(%table:8, kNetAddition)}
    args: table=%table:8

;; --- stratum 7: the JOIN (%table:4 ⋈ %table:8 on X) -> %table:23 ---
;; The 4 join-terminal SEED_FOLDs build the shared $join-pivots.42; the join
;; itself is a DRJoin (substrate, no op.<idx>) lowered by LowerSectionWalk.
op.12 kSeedFold sign=- ctx=seed  stratum=7  src=%table:4 join_pivot
    effects: {kVecDrain(%table:4, kNetRemoval), kVecAppend($join-pivots.42)}
    spine: —   ; join-pivot seed: no fold leaf, no target queue
    args: branch=<%table:4..join> src=%table:4 pivots=$join-pivots.42
op.13 kSeedFold sign=+ ctx=seed  stratum=7  src=%table:4 join_pivot
    effects: {kVecDrain(%table:4, kNetAddition), kVecAppend($join-pivots.42)}
    args: branch=<%table:4..join> src=%table:4 pivots=$join-pivots.42
op.14 kSeedFold sign=- ctx=seed  stratum=7  src=%table:8 join_pivot
    effects: {kVecDrain(%table:8, kNetRemoval), kVecAppend($join-pivots.42)}
    args: branch=<%table:8..join> src=%table:8 pivots=$join-pivots.42
op.15 kSeedFold sign=+ ctx=seed  stratum=7  src=%table:8 join_pivot
    effects: {kVecDrain(%table:8, kNetAddition), kVecAppend($join-pivots.42)}
    args: branch=<%table:8..join> src=%table:8 pivots=$join-pivots.42
;; join JOIN view=<X,Sum,Count> pivot_vec=$join-pivots.42 targets=[%table:23]
;;      section-walk: drain $join-pivots.42, point-test %table:4 via %index:149
;;      [X,_], %table:8 via %index:150 [X,_]; fold ± into %table:23. (.ir:244-255)
op.32 kClaimDrain sign=- ctx=seed  stratum=7  form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:23, kDeleteQueue), kFlagWrite(%table:23, -),
              kVecAppend(%table:23, kOverdeleteSet)}
    args: table=%table:23
op.33 kClaimDrain sign=+ ctx=seed  stratum=7  form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:23, kAddQueue), kFlagWrite(%table:23, +),
              kVecAppend(%table:23, kAdditionSet)}
    args: table=%table:23
op.34 kFrontierFilter sign=- ctx=seed  stratum=7  deferral=immediate
    reads: NetDeleted(%table:23)
    effects: {kVecDrain(%table:23, kOverdeleteSet), kVecAppend(%table:23, kNetRemoval)}
    args: table=%table:23
op.35 kFrontierFilter sign=+ ctx=seed  stratum=7  deferral=immediate
    reads: NetAdded(%table:23)
    effects: {kVecDrain(%table:23, kAdditionSet), kVecAppend(%table:23, kNetAddition)}
    args: table=%table:23

;; --- stratum 8: MAP div_i32 -> %table:17 (div on the %table:23 branch) ---
op.8  kSeedFold sign=- ctx=seed  stratum=8  src=%table:23 tgt=%table:17 class=NonRecursive
    reads: InI(%table:17)
    effects: {kVecDrain(%table:23, kNetRemoval), kCounter(%table:17, -, NonRecursive),
              kInIReadFrozen(%table:17, InI, seed), kVecAppend(%table:17, kDeleteQueue)}
    spine: kAccess(div_i32 MAP, bbf) -> kFold(%table:17, -, NonRecursive)
    args: branch=<%table:23..%table:17 via div_i32> src=%table:23 tgt=%table:17
op.9  kSeedFold sign=+ ctx=seed  stratum=8  src=%table:23 tgt=%table:17 class=NonRecursive
    reads: InI(%table:17)
    effects: {kVecDrain(%table:23, kNetAddition), kCounter(%table:17, +, NonRecursive),
              kInIReadFrozen(%table:17, InI, seed), kVecAppend(%table:17, kAddQueue)}
    spine: kAccess(div_i32 MAP, bbf) -> kFold(%table:17, +, NonRecursive)
    args: branch=<%table:23..%table:17 via div_i32> src=%table:23 tgt=%table:17
op.36 kClaimDrain sign=- ctx=seed  stratum=8  form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:17, kDeleteQueue), kFlagWrite(%table:17, -),
              kVecAppend(%table:17, kOverdeleteSet)}
    args: table=%table:17
op.37 kClaimDrain sign=+ ctx=seed  stratum=8  form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:17, kAddQueue), kFlagWrite(%table:17, +),
              kVecAppend(%table:17, kAdditionSet)}
    args: table=%table:17
op.38 kFrontierFilter sign=- ctx=seed  stratum=8  deferral=immediate
    reads: NetDeleted(%table:17)
    effects: {kVecDrain(%table:17, kOverdeleteSet), kVecAppend(%table:17, kNetRemoval)}
    args: table=%table:17
op.39 kFrontierFilter sign=+ ctx=seed  stratum=8  deferral=immediate
    reads: NetAdded(%table:17)
    effects: {kVecDrain(%table:17, kAdditionSet), kVecAppend(%table:17, kNetAddition)}
    args: table=%table:17

;; --- stratum 9: MATERIALIZE average_incoming_weight -> %table:32 ---
op.10 kSeedFold sign=- ctx=seed  stratum=9  src=%table:17 tgt=%table:32 class=NonRecursive
    reads: InI(%table:32)
    effects: {kVecDrain(%table:17, kNetRemoval), kCounter(%table:32, -, NonRecursive),
              kInIReadFrozen(%table:32, InI, seed), kVecAppend(%table:32, kDeleteQueue)}
    spine: kFold(%table:32, -, NonRecursive)
    args: branch=<%table:17..%table:32> src=%table:17 tgt=%table:32
op.11 kSeedFold sign=+ ctx=seed  stratum=9  src=%table:17 tgt=%table:32 class=NonRecursive
    reads: InI(%table:32)
    effects: {kVecDrain(%table:17, kNetAddition), kCounter(%table:32, +, NonRecursive),
              kInIReadFrozen(%table:32, InI, seed), kVecAppend(%table:32, kAddQueue)}
    spine: kFold(%table:32, +, NonRecursive)
    args: branch=<%table:17..%table:32> src=%table:17 tgt=%table:32
op.40 kClaimDrain sign=- ctx=seed  stratum=9  form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:32, kDeleteQueue), kFlagWrite(%table:32, -),
              kVecAppend(%table:32, kOverdeleteSet)}
    args: table=%table:32
op.41 kClaimDrain sign=+ ctx=seed  stratum=9  form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:32, kAddQueue), kFlagWrite(%table:32, +),
              kVecAppend(%table:32, kAdditionSet)}
    args: table=%table:32
op.42 kFrontierFilter sign=- ctx=seed  stratum=9  deferral=immediate
    reads: NetDeleted(%table:32)
    effects: {kVecDrain(%table:32, kOverdeleteSet), kVecAppend(%table:32, kNetRemoval)}
    args: table=%table:32
op.43 kFrontierFilter sign=+ ctx=seed  stratum=9  deferral=immediate
    reads: NetAdded(%table:32)
    effects: {kVecDrain(%table:32, kAdditionSet), kVecAppend(%table:32, kNetAddition)}
    args: table=%table:32

;; --- lead 2: commit band (sentinel stratum, band 9) then seals (band 10) ---
;; COMMIT_SWEEP order within band 9 is the pointer tie-break (op_table_id);
;; the .ir textual order is 4,8,12,17,23,28,32,36 — rendered here in that order.
op.46 kCommitSweep sign=· ctx=seed  band=9  flavor=differential publish_target=false
    reads: InI(%table:4)
    effects: {kInIReadFrozen(%table:4, InI, seed), kFlagWrite(%table:4)}
    args: table=%table:4
op.47 kCommitSweep sign=· ctx=seed  band=9  flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:8, InI, seed), kFlagWrite(%table:8)}
    args: table=%table:8
op.44 kCommitSweep sign=· ctx=seed  band=9  flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:12, InI, seed), kFlagWrite(%table:12)}
    args: table=%table:12
op.49 kCommitSweep sign=· ctx=seed  band=9  flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:17, InI, seed), kFlagWrite(%table:17)}
    args: table=%table:17
op.48 kCommitSweep sign=· ctx=seed  band=9  flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:23, InI, seed), kFlagWrite(%table:23)}
    args: table=%table:23
op.45 kCommitSweep sign=· ctx=seed  band=9  flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:28, InI, seed), kFlagWrite(%table:28)}
    args: table=%table:28
op.50 kCommitSweep sign=· ctx=seed  band=9  flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:32, InI, seed), kFlagWrite(%table:32)}
    args: table=%table:32
op.51 kCommitSweep sign=· ctx=seed  band=9  flavor=monotone
    effects: {kFlagWrite(%table:36)}
    args: table=%table:36
op.1  kStateSeal sign=· ctx=seed  band=10  sc#0
    effects: {kStateFold(%table:4, sign=0)}      ; sealed:=working swap (global:rmw)
    args: agg_table=%table:4 statecell=sc#0
op.3  kStateSeal sign=· ctx=seed  band=10  sc#1
    effects: {kStateFold(%table:8, sign=0)}
    args: agg_table=%table:8 statecell=sc#1
op.5  kStateSeal sign=· ctx=seed  band=10  sc#2
    effects: {kStateFold(%table:12, sign=0)}
    args: agg_table=%table:12 statecell=sc#2

;; ================================ DEPS ================================
;; DRDep RAW/WAR/WAW edges (kind/scope/loop_carried), derived by the linearizer.
;; Every edge scope=epoch (acyclic — NO loop_carried, spec §2.3 correction 6).
;; The seed-before-drain RAW chain per table (append -> claim drain):
deps:
  op.52 -> op.4  RAW epoch    ; ingest net-addition -> KV group-update band(a)
  op.4  -> op.16 RAW epoch    ; KV emit_touched delQ -> claim drain
  op.4  -> op.17 RAW epoch    ; KV emit_touched addQ -> claim drain
  op.16 -> op.18 RAW epoch    ; overdelete-set -> filter
  op.17 -> op.19 RAW epoch
  op.18 -> op.6  RAW epoch    ; net-removal -> seed fold into %table:28
  op.19 -> op.7  RAW epoch
  op.6  -> op.20 RAW epoch  ;  op.7 -> op.21 RAW epoch
  op.20 -> op.22 RAW epoch  ;  op.21 -> op.23 RAW epoch
  op.22 -> op.0  RAW epoch  ;  op.23 -> op.0  RAW epoch   ; %table:28 nets -> sum
  op.22 -> op.2  RAW epoch  ;  op.23 -> op.2  RAW epoch   ; %table:28 nets -> count
  op.0  -> op.24 RAW epoch  ;  op.0  -> op.25 RAW epoch
  op.24 -> op.26 RAW epoch  ;  op.25 -> op.27 RAW epoch
  op.26 -> op.12 RAW epoch  ;  op.27 -> op.13 RAW epoch   ; sum nets -> pivots
  op.2  -> op.28 RAW epoch  ;  op.2  -> op.29 RAW epoch
  op.28 -> op.30 RAW epoch  ;  op.29 -> op.31 RAW epoch
  op.30 -> op.14 RAW epoch  ;  op.31 -> op.15 RAW epoch   ; count nets -> pivots
  op.12,13,14,15 -> join RAW epoch                        ; pivots -> section walk
  join -> op.32 RAW epoch   ;  join -> op.33 RAW epoch    ; %table:23 queues
  op.32 -> op.34 RAW epoch  ;  op.33 -> op.35 RAW epoch
  op.34 -> op.8  RAW epoch  ;  op.35 -> op.9  RAW epoch
  op.8  -> op.36 RAW epoch  ;  op.9  -> op.37 RAW epoch
  op.36 -> op.38 RAW epoch  ;  op.37 -> op.39 RAW epoch
  op.38 -> op.10 RAW epoch  ;  op.39 -> op.11 RAW epoch
  op.10 -> op.40 RAW epoch  ;  op.11 -> op.41 RAW epoch
  op.40 -> op.42 RAW epoch  ;  op.41 -> op.43 RAW epoch
  ; commit sweeps + seals trail (band 9/10); each reads its table's InI after
  ; every emit_touched read of working (V-COMMIT-TRAILS) — WAR/RAW vs the
  ; per-table filters, elided for brevity (all scope=epoch).

;; =============================== CENSUS ===============================
census: kGroupUpdate=3 kStateSeal=3 kSeedFold=10 kClaimDrain=14
        kFrontierFilter=14 kCommitSweep=8 kIngestFold=1
        (kCrossover=0 kProductArm=0 kFixpointFire=0 kChainFold=0 kRetire=0
         kRederive=0 kNegateGate=0 kPivotAssemble=0)
```

## §2 Derivation notes (every op traced to its minting code path + .ir line)

### §2.0 The table / stratum map (from average_weight.dot + .ir create blocks)

The 8 tables (`%table:<DataTable::Id()>`, the authoritative id the .ir prints).
STRATUM from the .dot node labels; differential = has claim-drains in the .ir.

    %table:36 [i32,i32,mut]  edge_weight base (RECEIVE add_edge)  STRATUM 0  MONOTONE
    %table:12 [i32,i32,mut]  edge_weight KV index (@recompute)    STRATUM 1  differential
    %table:28 [i32,i32]      TUPLE X,BX_Weight (edge_weight->agg) STRATUM 2  differential
    %table:4  [i32,i32]      sum_i32 aggregate (@invertible)      STRATUM 3  differential
    %table:8  [i32,i32]      count_i32 aggregate (@invertible)    STRATUM 5  differential
    %table:23 [i32,i32,i32]  JOIN X,Sum,Count                     STRATUM 7  differential
    %table:17 [i32,i32,i32,i32] MAP div_i32 -> Sum,Count,Avg,X    STRATUM 8  differential
    %table:32 [i32,i32]      average_incoming_weight MATERIALIZE  STRATUM 9/10 differential

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
`op.<idx>`; it is rendered as substrate (the `; join` line under §1's joins).

MINT PHASE 7 — CLAIM_DRAIN / RETIRE / REDERIVE / FRONTIER_FILTER
(DeltaRel.cpp:1508-1658), per differential non-induction table. All 7
differential tables are ACYCLIC ⇒ each mints exactly: claim−, claim+, filter−,
filter+ (single-pass / immediate, 1653-1656). mint_claim (1514) then mint_filter
(1566). Table iteration is impl->tables order (PREDICTED indices):
  op.16..19  %table:12  claim− claim+ filter− filter+   [.ir:141,146,150,154]
  op.20..23  %table:28  claim− claim+ filter− filter+   [.ir:169,174,178,182]
  op.24..27  %table:4   claim− claim+ filter− filter+   [.ir:192,197,201,205]
  op.28..31  %table:8   claim− claim+ filter− filter+   [.ir:215,220,224,228]
  op.32..35  %table:23  claim− claim+ filter− filter+   [.ir:259,264,268,272]
  op.36..39  %table:17  claim− claim+ filter− filter+   [.ir:291,296,300,304]
  op.40..43  %table:32  claim− claim+ filter− filter+   [.ir:319,324,328,332]

MINT PHASE 8 — COMMIT_SWEEP (DeltaRel.cpp:1660-1706). One per differential table
(flavor=differential); one Seal per MONOTONE boundary table with a delta-vec
entry. Iterates impl->tables (PREDICTED):
  op.44..50  COMMIT_SWEEP differential: %table:12,28,4,8,23,17,32
  op.51      COMMIT_SWEEP monotone (Seal): %table:36            [.ir:342]
  (.ir:335-342 emits sweeps in a DIFFERENT textual order — 4,8,12,17,23,28,32,36
   — because LowerCommitSweeps walks pinned_order under the band-9 key which
   sorts by table pointer, §2.4; the MINT order here is impl->tables.)

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

vec-block PREDICTED index assignment (impl->tables order for the per-table
six-packs; the join pivot vec last):
  vec.0..5    %table:12  {delQ,addQ,overdel,addSet,netRem,netAdd}
  vec.6..11   %table:28  {…}
  vec.12..17  %table:4   {…}
  vec.18..23  %table:8   {…}
  vec.24..29  %table:23  {…}
  vec.30..35  %table:17  {…}
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

### §2.4 Emission ORDER = pinned_order (the band comparator, 3354-3464)

pinned_order sorts each op by Key{lead, stratum, band, table_ptr, sign, ctor}
(key_less 3457). lead: 0=eager-gate/ingest-fold, 1=phase, 2=commit-sweep/seal.
band (op_band 3354): GROUP_UPDATE/SEED_FOLD=0, single-pass CLAIM=1, immediate
FILTER=2, COMMIT_SWEEP=9. STATE_SEAL trails at sentinel stratum max+1, band 10;
COMMIT_SWEEP at sentinel stratum max+1, band 9 (key_of 3445-3453).

op_stratum (3276): GROUP_UPDATE→group_update_stratum (KV=1, sum=3, count=5);
CLAIM/FILTER→drain_stratum(table); SEED_FOLD→branch_stratum[seed_branch].
This reproduces the .ir `^flow` order EXACTLY (traced line-by-line against the
.ir above). The INGEST_FOLD is lead-0 → sorts before all phase ops, but LOWERS
into `^entry:41` (not `^flow`) because LowerIngestFold plants it at the walk
position; the dump renders it FIRST (lead-0), matching pinned_order, with a
`; lowered in ^entry (ingest-cursor hole)` note.

LOUD (§3): the Key tie-break `table_id = reinterpret_cast<uintptr_t>(t)`
(op_table_id 3387-3394) is a POINTER. Within one (lead,stratum,band) that holds
ops of DIFFERENT tables, order is pointer-derived. The (F) determinism fix
(consolidated §1.A) made the whole corpus byte-stable, so table allocation is
deterministic-per-build in practice, but this is NOT the det_seq-clean key the
-df-out side got — see §3. For average_weight each (stratum,band) cell holds at
most ONE table's ops (the strata are 1:1 with tables), so the pointer tie-break
never actually decides order here — a happy accident of this witness, not a
guarantee.
## §3 Open questions / spec frictions (LOUD)

F-1 (HIGH, determinism) — THE POINTER TIE-BREAK IN pinned_order. The band
comparator's `op_table_id` (DeltaRel.cpp:3387-3394) returns
`reinterpret_cast<uintptr_t>(table)`, and `key_less` (3461) uses it as a
tie-break within a (lead,stratum,band) cell. This is EXACTLY the pointer-order
anti-pattern the (F) det_seq substrate (consolidated §1.A) fixed on the DataFlow
side, but the DeltaRel linearizer still uses raw table pointers. The corpus is
byte-stable today ONLY because table allocation happens in a deterministic walk
per build; there is no det_seq-analogue guaranteeing it. For average_weight the
tie-break never fires (each (stratum,band) cell is 1:1 with one table — see §2.4),
so THIS witness's dump is stable regardless. But a program with two same-stratum
same-band tables (e.g. two acyclic sibling tables at one stratum) would order
their ops by pointer. The COMMIT_SWEEP band-9 cell IS such a case: all 8 commit
sweeps share (lead=2, stratum=max+1, band=9) and differ only by table pointer —
so the commit-sweep EMISSION ORDER (op.44..51 textual order in the dump) is
pointer-derived. I rendered them in the .ir's observed order (4,8,12,17,23,28,32,
36) but that order is NOT guaranteed by a pointer-free key. RECOMMENDATION: the
dump is the RIGHT surface to EXPOSE this (spec §2.4 "the dump exposes any
linearization nondeterminism byte-for-byte — that is its (F)-gate job"); the
golden gate will catch a run-to-run flip. But the owner should decide whether to
harden op_table_id to a pointer-free key (e.g. DataTable::Id()) before blessing
average_weight's `deltarel opt` golden — otherwise a future allocator change
churns the golden without any semantic change.

F-2 (MED, format) — JOINS / BRANCHES have NO PLACE in the spec §2.3 skeleton.
The format block (§2.3) shows only `vec / op / deps / census` sections, but
correction 2 says "Vecs/branches/joins/rounds iterate their OWN ordered vectors."
average_weight HAS one DRJoin (the %table:4⋈%table:8 join) that is NOT a DROp —
it drives the `join-tables` section walk (.ir:244-255) at lowering time via
LowerSectionWalk. I rendered it as a `;; join …` substrate comment inline where
it emits (stratum 7). The spec should PIN whether joins/branches get first-class
dump lines (e.g. a `join.<idx> …` / `branch.<idx> …` section) or stay comments.
Same question for DRRound.body_ops/output_ops (spec correction 9 — render under
a `; substrate (unread by lowering)` comment); average_weight has ZERO rounds
(acyclic), so that surface is untested by THIS witness.

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

F-4 (LOW, naming) — MODULE NAME. `deltarel <module-name>` (§2.3 line 1):
average_weight.dr declares no `#module`, and the .ir has no module-name line, so
there is no ParsedModule::Name() to print. I used the .dr basename
(`average_weight`), matching witness-deltarel-target.md §2.6's convention
(`deltarel demand_neighborhood_witness`). The emitter needs a defined fallback
(basename? empty? a fixed literal?) — PIN it so the golden is stable.

F-5 (LOW, effect rendering) — THE GROUP_UPDATE / SEED_FOLD `reads:` LINE. Spec
§2.3 line "reads: <Pred spellings>". A GROUP_UPDATE's only membership-predicate
read is its emit_touched `kInIReadFrozen(InI)` crossing; its band-(a) folds are
kStateFold (not a Pred read) and its drains are kVecDrain. I rendered `reads:
InI(%table:N)` (the one crossing) and put everything else under `effects:`. This
matches the witness-target §2.6 shape but the spec doesn't say whether kStateEmit
/kStateOld (frozen/working reads) should ALSO surface on `reads:` — they are
EffKind members, not Pred reads, so I kept them in `effects:`. OPEN for the owner.

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
what this artifact pins.
