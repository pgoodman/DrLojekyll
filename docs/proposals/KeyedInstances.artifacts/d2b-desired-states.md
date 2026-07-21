======================================================================
COMMITTED AT THE §20 EPOCH-CLOSE CHECKPOINT (2026-07-21). This is the
adjudicated BINDING implementation contract for its diff, verbatim from
the session scratchpad (d2b/d2b-desired-states.md); its ADJUDICATION LEDGER records every
folded amendment, and the KeyedInstances.md §19(K)-(O) landing records
carry the owner rulings (RAT-1..RAT-10) that resolved its open items.
THIS ARTIFACT SUPERSEDES d1-desired-states.md §B.4 IN WHOLE (see its
own banner + §19(N)).
======================================================================

======================================================================
D2.b DESIRED-STATES — the RE-DERIVED nested dump blocks for
demand_neighborhood_witness (`-demand -demand-instance`, STAGE-1
R-MONO-a, MECHANISM-NATURAL OD-4, post-D1.b HP-11 collapse).

**THIS ARTIFACT SUPERSEDES d1-desired-states.md §B.4 IN WHOLE.** The
stale §B.4 was hand-derived against the frozen tip 1aaca896 BEFORE any
D1.b code existed; it is now wrong on FOUR axes, each corrected here from
the LANDED code at tip 677977f8:
  (i)   OD-4 ruled MECHANISM-NATURAL (KeyedInstances.md §19(I)): BOTH
        ingest folds gain kVecAppend, TWO monotone sweeps, census
        kCommitSweep=2 (§B.4 pinned demand-only, kCommitSweep=1).
  (ii)  HP-11 COLLAPSE (landed): kInstanceEmit/kInstanceOld/
        kInstanceSealSwap became kStateEmit/kStateOld/kStateFold
        (DeltaRel.h:83-85, InstantiateEffects DeltaRel.cpp:798-806,
        SealEffect :865-871). §B.4's kInstance* effect spellings are dead.
  (iii) THE COLLAPSED SEAL ENROLLS A WRITE HAZARD. §B.4 wrote "NO edge
        touches the seal ... kStateSeal mints none, the mold holds" —
        FALSE. The seal's kStateFold(pub, sign 0) is a WRITE
        (DeltaRel.cpp:3781-3788 flag_accesses{oi, value_table, true}), so
        the instantiate→seal WAW+WAR edges are REAL. Re-derived below.
  (iv)  The 2-edge deps claim is stale: the mechanism-natural + seal-write
        re-derivation yields SIX edges.

METHOD: START FROM THE REAL FLAT DUMPS. demand_neighborhood_witness.dr
was compiled with the TIP binary (build/debug/bin/drlojekyll) under plain
`-demand` emitting -df-out/-deltarel-out/-ir-out into d2b/real/. Every
nested line below is a DIFF from those real bytes, mechanism-traced with a
file:line cite at tip for each predicted line. (`-demand-instance` does
NOT exist yet — DeltaRel.cpp:875-876; it arrives at THIS diff, D2.b, and
flips `context.demand_instance_enabled`, Build.h:207.)

BANNER — WHAT THIS BLOCK CERTIFIES (per HP-13(b), the symrec/T2b
illustrative-id precedent): the OP SET, the EFFECT MULTISETS, the LINE
ORDER (band-key derivation), the CENSUS counts, and the DEP-EDGE SET.
All op.N labels and i#/store ids are ILLUSTRATIVE — ⟨PIN⟩ AT FIRST
EMISSION. It remains UNBLESSABLE pre-emission; the HP-13(b) end-to-end
real-dump review runs against THIS contract before any nested golden is
blessed.

Repo /Users/pag/Code/DrLojekyll, branch keyed-instances, tip 677977f8.
Author pass: D2.b ds-writer, 2026-07-20.
======================================================================


======================================================================
§0. GROUND TRUTH — the real flat dumps (byte-pinned inputs)
======================================================================

Compiled: `drlojekyll demand_neighborhood_witness.dr -demand
-df-out … -deltarel-out … -ir-out …` → exit 0, clean stderr.
Bytes live in d2b/real/witness.{df,deltarel,ir}.

The flat `.deltarel` (d2b/real/witness.deltarel) is TWO ingest folds and
nothing else. NOTE the census tail NOW carries the THREE D1.b counters
(kSubgraphInstantiate/kInstanceDeath/kInstanceSeal), all =0, appended
after kStateSeal in kAllKinds enum order (Format.cpp:895-905):

    deltarel

    op.1 kIngestFold sign=+ ctx=eager stratum=0
        effects: {kCounter(%table:8, +, NonRecursive)}
        spine: —
        args: table=%table:8 message=demand__neighborhood_bf/1
    op.0 kIngestFold sign=+ ctx=eager stratum=0
        effects: {kCounter(%table:11, +, NonRecursive)}
        spine: —
        args: table=%table:11 message=add_edge/2

    rounds:

    deps:

    census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0

Table identities (from d2b/real/witness.{df,ir}), FIXED across the flat
and nested compiles (Alt-A: FillDataModel is knob-blind):
  - %table:4  = neighborhood ANSWER / pub (cols Start,Node; idx 7 full, idx 57 Start,_)
  - %table:8  = demand relation `demand__neighborhood_bf` (col c9; idx 10)
  - %table:11 = edge INPUT (cols From,To; idx 14 full, idx 32 From,_)
  - %table:15 = the intermediate join.6→tuple.3 copy (cols Start,Node; idx 18 full, idx 38 Start,_)

Message ingest folds, flat (the nested block re-uses these bytes + a
per-fold append): demand fold = MakeMonotoneIngestFold over %table:8,
edge fold = over %table:11 (DeltaRel.cpp:2080-2117; monotone arm
:2111-2113). Both carry role kEmpty flat ⇒ NO kVecAppend
(MonotoneIngestRoleDR returns kEmpty when no member view has a cut
successor, DeltaRel.cpp:147-160).


======================================================================
§1. BLOCK (1) — NESTED `.df`  ==  FLAT `.df`  BYTE-FOR-BYTE
======================================================================

PINNED: the nested `.df` is d2b/real/witness.df UNCHANGED (Alt-A, ratified
OD-R5 / HP-13). The block is not repeated; d2b/real/witness.df IS the
contract for both eqgate arms.

X-DS-4 RE-VERIFIED UNDER MECHANISM-NATURAL (the OD-4 blast radius does not
reach the `.df`):
  - `-df-out` drains post-Program::Build; `table=`/`class=` come from
    TableId()/CanReceiveDeletions() (ControlFlow/Format.cpp DefineTable
    path + the .df ATTRIBUTES emitter). Mechanism-natural adds
    `context.table_delta_vecs` entries + two monotone commit sweeps + two
    ingest appends, NONE of which changes any table's id or its
    differential-ness. %table:8/%table:11/%table:15 stay monotone.
  - The `-demand-instance` knob is an EMISSION/DR-lowering selector
    (BuildSubgraphInstanceOps is gated at DeltaRel.cpp:1355, POST
    Query::Build); it runs no DataFlow rewrite. RecognizedSubgraphs is
    populated by ApplyDemandTransform under plain `-demand` in BOTH arms
    (Query.h:1012-1023), and NO recognition metadata renders on `.df`
    (OD-R8 / C-11).
  ⇒ Every `.df` token — the 9 views, det_seq 0..8, `table=%table:15
    class=monotone` on tuple.3, c3/c8, every `=>` edge — is knob- and
    provisioning-invariant. ONE `.df` golden serves both eqgate arms.


======================================================================
§2. BLOCK (2) — NESTED `.deltarel` (FULL RE-DERIVATION, supersedes §B.4)
======================================================================

The certified nested block. Every op/effect/dep/census line is
mechanism-traced in §2.1-§2.5. IDs illustrative.

    deltarel

    instances:
      DRInstance i#0 forcing=neighborhood key=%table:8 pub=%table:4 input=%table:11 store=I#0 key_cols=[Start] row_cols=[Node]

    op.5 kIngestFold sign=+ ctx=eager stratum=0
        effects: {kCounter(%table:8, +, NonRecursive), kVecAppend(%table:8, kNetAddition)}
        spine: —
        args: table=%table:8 message=demand__neighborhood_bf/1
    op.4 kIngestFold sign=+ ctx=eager stratum=0
        effects: {kCounter(%table:11, +, NonRecursive), kVecAppend(%table:11, kNetAddition)}
        spine: —
        args: table=%table:11 message=add_edge/2
    op.0 kSubgraphInstantiate sign=+ ctx=seed stratum=1 i#0
        demand=%table:8 pub=%table:4 input=%table:11 pub_row=[ik:Start,row:Node] nested=<Node>
        reads: Present(%table:11)
        effects: {kVecDrain(%table:8, kNetAddition), kInstanceDemand(%table:8), kInstanceRebuild(%table:4, +), kStateEmit(%table:4), kStateOld(%table:4), kCounter(%table:4, +, NonRecursive)}
        spine: —
        args: demand=%table:8 pub=%table:4 input=%table:11 store=I#0
    op.2 kCommitSweep sign=· ctx=seed band=9 flavor=monotone publish_target=false
        effects: {kFlagWrite(%table:8)}
        args: table=%table:8
    op.3 kCommitSweep sign=· ctx=seed band=9 flavor=monotone publish_target=false
        effects: {kFlagWrite(%table:11)}
        args: table=%table:11
    op.1 kInstanceSeal sign=· ctx=seed band=11 i#0
        effects: {kStateFold(%table:4, sign=0)}
        args: pub=%table:4 store=I#0

    rounds:

    deps:
      op.0 -> op.1 WAR epoch
      op.0 -> op.1 WAW epoch
      op.0 -> op.3 WAR epoch
      op.4 -> op.0 RAW epoch
      op.4 -> op.3 WAW epoch
      op.5 -> op.2 WAW epoch

    census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=2 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=1 kInstanceDeath=0 kInstanceSeal=1

(The `—` on the ingest spines is U+2014, em-dash; `·` is U+00B7. Single
trailing newline after the census line, per the flat dump.)

[ADJ-G2 — ADJUDICATED UPHELD (LOW, crit-grammar-2).] The certified block
now renders the instantiate `spine: —` (the AS-LANDED D1.b mint sets no
arms, Format.cpp:482-484) — the previous `spine: kAccess(...) -> kFold(...)
    ; ⟨PIN⟩ — see §2.3` embedded a NON-BYTE doc annotation inside bytes a
byte-diff would trip on. The TARGET spine
(`kAccess(%table:11, section-walk) -> kFold(%table:4, +, NonRecursive)`)
remains the D2.b mechanism goal, discussed as an UNCERTIFIED ⟨PIN⟩ in §2.3;
either `—` or the chain is contract-legal (spine uncertified).

[ADJ-G1 — ADJUDICATED UPHELD (MED, crit-grammar-1). a2 EFFECTS FORK.] The
op.0 `effects:` line above is the a1 (demand-drain-only) set. If the ritual
adopts R-REBUILD-a2 (§0 of the design), band-(a) ALSO drains the edge
frontier and V-INST-DRAIN requires that input drain to resolve, so
InstantiateEffects (DeltaRel.cpp:774-778, the sole authority) pushes a
SECOND `kVecDrain(%table:11, kNetAddition)` onto op.0 — a REAL change to the
rendered `effects:` line. Under a2 the certified op.0 effects are:
    {kVecDrain(%table:8, kNetAddition), kVecDrain(%table:11, kNetAddition),
     kInstanceDemand(%table:8), kInstanceRebuild(%table:4, +),
     kStateEmit(%table:4), kStateOld(%table:4),
     kCounter(%table:4, +, NonRecursive)}
CENSUS (6) and DEPS (6) are UNCHANGED under a2 (a kVecDrain is an effect not
an op; the monotone edge frontier is un-minted → ResolveVecIdx ~0u → no vec
hazard). So the a1/a2 fork touches ONLY the instantiate effects multiset.
The HP-13(b) end-to-end review MUST check the real dump against whichever
branch the ritual adopts (do NOT flag the a2 kVecDrain as a mismatch, and do
NOT drop the a2 mechanism to "match" the a1 golden).

----------------------------------------------------------------------
§2.0 OP SET + MINT ORDER (the op.N labels) + LINE ORDER (render order)
----------------------------------------------------------------------

OP SET (6 ops): 2 kIngestFold, 1 kSubgraphInstantiate, 2 kCommitSweep,
1 kInstanceSeal. ZERO kInstanceDeath (R-MONO: demand %table:8 is
monotone, so the death arm at DeltaRel.cpp:976 is skipped — HP-17).

MINT ORDER (= the op.N LABELS; `os << "op." << oi` uses the flow.ops
push index, Format.cpp:650-652). In BuildDRInventory the push sequence
for this witness (no negations/products/aggregates/recursive SCCs, so no
phase ops mint — the flat dump proves flow.ops was JUST the 2 ingest
folds) is:
  1. BuildSubgraphInstanceOps (called DeltaRel.cpp:1356) pushes, in body
     order, kSubgraphInstantiate (:962-973) then — death skipped — the
     kInstanceSeal (:990-996):
        op.0 = kSubgraphInstantiate
        op.1 = kInstanceSeal
  2. COMMIT_SWEEP loop (:1931-1965) over impl->tables, monotone arm
     (:1932-1947) for each monotone table with a table_delta_vecs entry —
     %table:8 then %table:11 (id order):
        op.2 = kCommitSweep %table:8
        op.3 = kCommitSweep %table:11
  3. INGEST_FOLD loop (:2080-2118), query.IOs() order = add_edge before
     demand (the flat labels op.0=add_edge/op.1=demand prove this order):
        op.4 = kIngestFold %table:11 (add_edge)
        op.5 = kIngestFold %table:8  (demand)
NOTE the ingest folds land at op.4/op.5 (NOT op.0/op.1 as in flat) because
the instance ops + sweeps now mint BEFORE them — the flat op.0/op.1 labels
do NOT carry over. (⟨PIN⟩; the exact numbers are illustrative.)

LINE ORDER (render order = flow.pinned_order, the band-key linearization,
key_of DeltaRel.cpp:3935-3971 + key_less :3972-3979). Composite key
(lead, stratum, band, table_id=id+1, sign, ctor):
  - op.5 ingest(%table:8): lead 0, tid 9  → Key(0,0,0, 9,+1,5)   [:3945-3947]
  - op.4 ingest(%table:11): lead 0, tid 12 → Key(0,0,0,12,+1,4)  [:3945-3947]
  - op.0 instantiate: default phase arm → Key(1, stratum 1, band 0, tid 5, +1, 0)  [:3964-3970; stratum from instance_stratum[0]=1, :1008-1009 / DROpStratum :3628-3640]
  - op.2 sweep(%table:8):  lead 2, band 9, tid 9  → Key(2,max+1,9, 9,0,2)  [:3948-3950]
  - op.3 sweep(%table:11): lead 2, band 9, tid 12 → Key(2,max+1,9,12,0,3)  [:3948-3950]
  - op.1 seal: lead 2, band 11, tid 5 → Key(2,max+1,11,5,0,1)  [:3957-3962]
Sorted (lead, then stratum, band, table_id, sign, ctor):
  lead 0: op.5 (tid 9) < op.4 (tid 12)      → demand ingest, then edge ingest
  lead 1: op.0                               → instantiate
  lead 2: band 9 { op.2 (tid 9) < op.3 (tid 12) }, then band 11 { op.1 }
                                             → sweep(8), sweep(11), seal
  ⇒ pinned_order = [op.5, op.4, op.0, op.2, op.3, op.1]  (the render order above).
The demand-before-edge ingest order is the SAME table_id tie-break
(%table:8 < %table:11) that produced the flat op.1-before-op.0 order.

----------------------------------------------------------------------
§2.1 THE `instances:` SECTION  (Format.cpp:600-620; OD-I1 / C-6, real row)
----------------------------------------------------------------------

Rendered first (before ops:, after the `deltarel` header + blank), because
this witness has no vecs/branches/joins sections (p11 empty-section law,
:597-599). ONE DRInstance (one RecognizedSubgraph — the single-adornment
slice, OD-R1).

    DRInstance i#0 forcing=neighborhood key=%table:8 pub=%table:4 input=%table:11 store=I#0 key_cols=[Start] row_cols=[Node]

Field-by-field (Format.cpp:604-618):
  - `i#0` / `store=I#0`: the DRInstance index i (:605) and `store=I#`i
    (:607) — SAME id space rendered on both (C-7 lean-collapse deferred to
    the implementer, OD-I2; both = 0 here).
  - `forcing=neighborhood`: `in.forcing_name`, set at mint to
    `forcings[rs.forcing_index].query.NameAsString()` (DeltaRel.cpp:941-942).
    `query` is a ParsedQuery (Query.h:959) ⇒ NameAsString() = the bare
    query name "neighborhood". **CORRECTION vs §B.4's `neighborhood/bf`:
    NO `/bf` adornment suffix** — the code renders the declaration name only.
  - `key=%table:8`: `tid(in.demand_table)` (:606). The field is spelled
    `key=` but its VALUE is the demand table (%table:8), per the code.
  - `pub=%table:4`: `tid(in.pub_table)` (:606) — the answer INSERT target.
  - `input=%table:11`: `tid(in.input_table)` (:607) — the summarized
    monotone input (the walked-to table-bearing predecessor,
    DeltaRel.cpp:916-928).
  - `key_cols=[Start]`: rs.key_cols = the forcing's bound α positions
    (Query.h:1020) = [0] for neighborhood(bound Start,…); rendered via
    pub_col_name(in, 0) = pub_view column 0 name = "Start" (:609-612 /
    :589-595).
  - `row_cols=[Node]`: the published positions NOT in key_cols
    (DeltaRel.cpp:948-957) = [1]; pub_col_name(in,1) = "Node" (:613-617).

----------------------------------------------------------------------
§2.2 THE TWO kIngestFold OPS — BOTH gain kVecAppend (MECHANISM-NATURAL OD-4)
----------------------------------------------------------------------

Each fold is the flat op (§0) PLUS one `kVecAppend(<table>, kNetAddition)`
after the counter. Mechanism: under `-demand-instance`, the recognized
subgraph boundary makes the demanded chain a CUT SUCCESSOR of BOTH monotone
boundary inputs (OD-4 mechanism-natural — the aggregate/KV chain-breaker
analog, AnyCutSuccessorDR DeltaRel.cpp:130-137). So MonotoneIngestRoleDR
(:147-160) returns kNetAddition for %table:8 AND %table:11, and
MakeMonotoneIngestFold appends the kVecAppend when role != kEmpty
(:1083-1089). The census recount reads the SAME MonotoneIngestRoleDR
(:3071) so the ingest-key multiset stays consistent (:3176-3201).

  op.5 (demand, %table:8):
    effects: {kCounter(%table:8, +, NonRecursive), kVecAppend(%table:8, kNetAddition)}
  op.4 (edge, %table:11):
    effects: {kCounter(%table:11, +, NonRecursive), kVecAppend(%table:11, kNetAddition)}

  - kCounter render: `(tid, effect_sign(+1)="+", DerivClassName(NonRecursive))`
    (Format.cpp:442-444). klass = EmissionDerivClass (non-recursive here) —
    UNCHANGED from flat.
  - kVecAppend render: value_table non-null ⇒ `(tid, VecRoleIdent(kNetAddition)="kNetAddition")`
    (Format.cpp:430-441 / :70). NO vec-index — a monotone kNetAddition
    frontier mints NO DRVec (X-DS-1, upheld: the append parks via LOWER-time
    TableDeltaVector; MintTableVec is differential-only, :1128-1143). The
    effect renders from its own fields regardless (no vec line needed).
  - header/args/spine UNCHANGED from flat (Format.cpp:702-714):
    `sign=+ ctx=eager stratum=0`, `spine: —` (op.arms empty), and
    `args: table=… message=…/…`.
  **CORRECTION vs §B.4**: §B.4 (demand-only) gave op.1/edge fold
  effect-UNCHANGED; under the ratified mechanism-natural OD-4 the EDGE fold
  ALSO gains kVecAppend(%table:11, kNetAddition).

----------------------------------------------------------------------
§2.3 THE kSubgraphInstantiate OP  (Format.cpp:783-800; effects DeltaRel.cpp:771-838, !DIFF arm)
----------------------------------------------------------------------

Header (Format.cpp:784-786): `sign=` SignGlyph(op.table_op_sign=+1)="+"
(:275-279); `ctx=seed`; `stratum=` DROpStratum = instance_stratum[0] = 1
(1+max(ready_after(demand),ready_after(input)) = 1+max(0,0),
DeltaRel.cpp:1008-1009); `i#0`.

Second header line (Format.cpp:787-791): `demand=%table:8 pub=%table:4
input=%table:11` then emit_pub_row (:625-645):
  `pub_row=[ik:Start,row:Node] nested=<Node>` — keyset={0}; p=0 "ik:Start"
  (key), p=1 "row:Node" (row) (:634-638, HP-6 partition); nested=<row_cols>
  = <Node> (:639-644).

`reads: Present(%table:11)` (Format.cpp:792 / emit_reads :553-567): the
InstantiateEffects leaf `kFlagRead(read_table=input=%table:11, pred=kPresent)`
(DeltaRel.cpp:785-790), filtered out of `effects:` (:541-542) and rendered
on `reads:` as PredName(kPresent)="Present" (:563 / :9).

`effects:` — the !DIFF (R-MONO) multiset, in InstantiateEffects push order
(DeltaRel.cpp:774-836), kFlagRead removed:
    {kVecDrain(%table:8, kNetAddition), kInstanceDemand(%table:8),
     kInstanceRebuild(%table:4, +), kStateEmit(%table:4),
     kStateOld(%table:4), kCounter(%table:4, +, NonRecursive)}
  effect-by-effect (mint field → Format render):
  - kVecDrain: value_table=demand=%table:8, vec_role=kNetAddition
    (:774-778) → `kVecDrain(%table:8, kNetAddition)` (:430-440).
  - kInstanceDemand: read_table=demand=%table:8 (:780-783) →
    `kInstanceDemand(%table:8)` (:468-470). Frozen demand-key read, NO
    hazard (HP-8; hazard switch break :3802-3803).
  - kInstanceRebuild: value_table=pub=%table:4, sign=+1 (:792-796) →
    `kInstanceRebuild(%table:4, +)` (:465-467). A WRITE of the store
    `current` word (hazard: :3795-3801).
  - kStateEmit: read_table=pub=%table:4 (:798-801) → `kStateEmit(%table:4)`
    (:461-463). **COLLAPSED (HP-11): was kInstanceEmit.** VALUED read of
    current (hazard: :3789-3791, read).
  - kStateOld: read_table=pub=%table:4 (:803-806) → `kStateOld(%table:4)`
    (:461-463). **COLLAPSED (HP-11): was kInstanceOld.** Frozen sealed read,
    NO within-band hazard (break :3793-3794).
  - kCounter: the !DIFF single +1 counter, counter_table=pub=%table:4
    (:828-835) → `kCounter(%table:4, +, NonRecursive)`. (R-MONO: ONE
    counter, ZERO appends, NO pub queue TableVec — :828-836.)

`spine:` ⟨PIN⟩ — NOT certified (C-9; the block certifies op set/effects/
order/census/deps, NOT spine bytes). The AS-LANDED D1.b mint sets NO arms
(BuildSubgraphInstanceOps DeltaRel.cpp:962-973 leaves inst.arms empty) ⇒
emit_spine renders `—` (Format.cpp:482-484). D2.b's α-consumer wiring +
PlanTree body (the gate comment :879-882) is expected to populate the
rescan spine; the mechanism-faithful TARGET is
`kAccess(%table:11, section-walk) -> kFold(%table:4, +, NonRecursive)`
(a keyed section walk over the input keyed on the instance-key column,
LoweringName(kSectionWalk)="section-walk" :266; folding into the pub).
IF D2.b leaves the arm empty the byte is `—`; either is contract-legal
(spine uncertified).

`args:` (Format.cpp:795-798): `demand=%table:8 pub=%table:4 input=%table:11
store=I#0`.

----------------------------------------------------------------------
§2.4 THE TWO kCommitSweep OPS  (mint DeltaRel.cpp:1931-1947; render Format.cpp:769-777)
----------------------------------------------------------------------

MECHANISM-NATURAL forces TWO monotone Seal sweeps — one per boundary table
that gained a `context.table_delta_vecs` entry (%table:8 AND %table:11).
The monotone arm (:1932-1947) mints kind kCommitSweep, ctx kSeed,
sweep_flavor kMonotone, ONE kFlagWrite(table). publish_target left default
(false). Census exp_sweep independently recounts monotone tables with a
delta-vec entry (:3033-3040) ⇒ 2.

  op.2:  kCommitSweep sign=· ctx=seed band=9 flavor=monotone publish_target=false
         effects: {kFlagWrite(%table:8)}   ; write_table=%table:8, sign 0 ⇒ no sign token (:449-452)
         args: table=%table:8
  op.3:  kCommitSweep sign=· ctx=seed band=9 flavor=monotone publish_target=false
         effects: {kFlagWrite(%table:11)}
         args: table=%table:11

  header (Format.cpp:769-773): SignGlyph(0)="·"; `band=9` (a key_of-local
  constant for sweeps, not from op_band); SweepFlavorName(kMonotone)=
  "monotone" (:56); publish_target=false. NO `reads:` (monotone sweep has
  no kFlagRead; the differential arm's kInIReadFrozen is absent here).
  **CORRECTION vs §B.4**: §B.4 pinned ONE sweep (demand only,
  kCommitSweep=1). Mechanism-natural = TWO, kCommitSweep=2.

----------------------------------------------------------------------
§2.5 THE kInstanceSeal OP  (mint DeltaRel.cpp:989-996 + SealEffect :865-871; render Format.cpp:814-821)
----------------------------------------------------------------------

  op.1:  kInstanceSeal sign=· ctx=seed band=11 i#0
         effects: {kStateFold(%table:4, sign=0)}
         args: pub=%table:4 store=I#0

  - header (Format.cpp:814-816): SignGlyph(0)="·"; `band=11 i#0`. (No
    stratum on the seal — it trails, DROpStratum :3641-3642; band 11 is
    key_of-local, :3957-3962: strictly after kStateSeal band 10 and sweeps
    band 9.)
  - effects (Format.cpp:817 / emit_effects): SealEffect =
    kStateFold(value_table=pub=%table:4, sign=0) (DeltaRel.cpp:865-871) →
    `kStateFold(%table:4, sign=0)` (:458-459, effect_sign(0)="sign=0").
    **COLLAPSED (HP-11/OD-5, self-lowered per HP-1): was
    kInstanceSealSwap.** No `reads:`/`spine:` (the kInstanceSeal case emits
    effects + args only, :814-820).
  - args: `pub=%table:4 store=I#0`.
  **THE HAZARD CORRECTION**: kStateFold is a WRITE (hazard switch
  DeltaRel.cpp:3781-3788: flag_accesses{oi, value_table=%table:4, write}).
  §B.4's "NO edge touches the seal" is WRONG — the seal's write on %table:4
  produces the op.0→op.1 WAW+WAR edges below.

----------------------------------------------------------------------
§2.6 THE DEP-EDGE SET — RE-DERIVED COMPLETELY (six edges; §B.4's two are stale)
----------------------------------------------------------------------

Method: enumerate every flag/vec access (hazard switch DeltaRel.cpp:3757-3811),
group by table (by_flag :4166-4189) / by vec (by_vec :4138-4162), emit WAW
for write/write (emit_waw :4042-4058) and RAW/WAR for write/read by band-key
execution order (emit_rw_hazard :4091-4131). Dedup on the full tuple
(from,to,kind,scope,loop_carried) (Format.cpp:863-878), sort ascending
(DepKind kRAW=0/kWAR=1/kWAW=2, DeltaRel.h:641; DepScope kEpoch=0). ALL
edges are epoch-scope, non-loop-carried (no round ops; scope_pair :3997-4015).

FLAG ACCESSES (op, table, is_write):
  op.0 instantiate: read %table:11 (leaf Present); write %table:4 (rebuild);
                    read %table:4 (emit); write %table:4 (counter).
                    [kVecDrain/kInstanceDemand/kStateOld/kInIReadFrozen — no
                     flag hazard: :3759-3762 vec un-minted skip / :3802-3803
                     / :3793-3794.]
  op.1 seal:        write %table:4 (kStateFold).
  op.2 sweep(8):    write %table:8 (kFlagWrite).
  op.3 sweep(11):   write %table:11 (kFlagWrite).
  op.4 ingest(11):  write %table:11 (kCounter). [kVecAppend → un-minted
                    monotone frontier vec, ResolveVecIdx ~0u, add_vec_access
                    skips — X-DS-1 graceful, DeltaRel.cpp:3110-3127/3257-3261.]
  op.5 ingest(8):   write %table:8 (kCounter). [append skipped, as above.]
NO vec hazards exist (the only vec accesses are the two un-minted monotone
frontier appends + the instantiate's un-minted frontier drain).

BY-TABLE cross-products (writer→reader RAW if writer earlier in band key,
reader→writer WAR if reader earlier; WAW lower-key→higher-key):
  %table:4  {op.0 rebuild-W, op.0 emit-R, op.0 counter-W, op.1 seal-W}:
    op.0-W × op.1-W  → WAW, key0(lead1)<key1(lead2) → op.0 → op.1 WAW  (rebuild∧counter both hit; deduped to one)
    op.0-R × op.1-W  → reader op.0 first (lead1<lead2) → op.0 → op.1 WAR
  %table:11 {op.0 leaf-R, op.3 sweep-W, op.4 ingest-W}:
    op.0-R × op.3-W  → reader op.0 (lead1) first < writer op.3 (lead2) → op.0 → op.3 WAR
    op.0-R × op.4-W  → writer op.4 (lead0) first < reader op.0 (lead1) → op.4 → op.0 RAW
    op.3-W × op.4-W  → WAW, key4(lead0)<key3(lead2) → op.4 → op.3 WAW
  %table:8  {op.2 sweep-W, op.5 ingest-W}:
    op.2-W × op.5-W  → WAW, key5(lead0)<key2(lead2) → op.5 → op.2 WAW

DEDUPED + SORTED deps (six):
    op.0 -> op.1 WAR epoch     ; instantiate emit-read vs seal write (SEAL EDGE, new)
    op.0 -> op.1 WAW epoch     ; instantiate rebuild/counter write vs seal write (SEAL EDGE, new)
    op.0 -> op.3 WAR epoch     ; instantiate Present-read vs edge-sweep write (OD-4 blast, new)
    op.4 -> op.0 RAW epoch     ; edge-ingest write vs instantiate Present-read (the "ingest→instantiate RAW")
    op.4 -> op.3 WAW epoch     ; edge-ingest vs edge-sweep (OD-4 EXTRA WAW)
    op.5 -> op.2 WAW epoch     ; demand-ingest vs demand-sweep (the demand WAW)

V-BAND-HAZARD (DeltaRel.cpp:4203-4211) is satisfied: every edge runs
forward in the band key (op.4/op.5 lead 0 → op.0 lead 1 → op.2/op.3 lead 2
→ op.1 band 11). No abort.

MAP to the task's expected set (all present + one extra):
  ingest→instantiate RAW = op.4→op.0 ✓ ; ingest→sweep WAWs = op.5→op.2
  (demand) + op.4→op.3 (edge, the OD-4 EXTRA) ✓ ; seal edges = op.0→op.1
  WAW + op.0→op.1 WAR ✓ ; **PLUS the newly-derived op.0→op.3 WAR** (the
  instantiate's Present(%table:11) read vs the edge sweep's flag write —
  an edge that exists ONLY under mechanism-natural, since the edge sweep
  exists only under OD-4).

**CORRECTION vs §B.4**: §B.4 gave TWO edges — `op.1 -> op.3 RAW` and
`op.2 -> op.0 WAW` (its own illustrative labels). The re-derivation gives
SIX under the landed by_flag/kStateFold machinery + mechanism-natural. The
by_vec RAW §B.4 already dropped (X-DS-1) stays gone.

----------------------------------------------------------------------
§2.7 CENSUS LINE (character-derived; Format.cpp:895-918, kAllKinds enum order)
----------------------------------------------------------------------

Counts (count_kind over flow.ops): kCommitSweep=2, kIngestFold=2,
kSubgraphInstantiate=1, kInstanceSeal=1, kInstanceDeath=0, kStateSeal=0,
all others 0. census_total = 2+2+1+1 = 6 = flow.ops.size() ⇒ the
completeness abort (:914-918) stays green.

    census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=2 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=1 kInstanceDeath=0 kInstanceSeal=1

Delta vs the flat census (§0): kCommitSweep 0→2, kSubgraphInstantiate 0→1,
kInstanceSeal 0→1 (kIngestFold stays 2, kInstanceDeath stays 0).


======================================================================
§3. BLOCK (3) — NESTED `.ir` / `.h` SHAPE CONTRACT (not byte-pinned)
======================================================================

This is a D2.b codegen shape contract (the sibling writes the diff); the
bytes are not certified. Baseline = d2b/real/witness.ir (appendix-C).

----------------------------------------------------------------------
§3.1 `.ir` create-table / index lines — the exact excision rule
----------------------------------------------------------------------

HOW create lines are decided: `for (auto table : program.Tables())
DefineTable(...)` (ControlFlow/Format.cpp:1020-1022) iterates EVERY table
in the data model, unconditionally. Index lines inside a create block come
from `table.Indices()` (:51), populated lazily by GetOrCreateIndex at
region-build sites (join scans etc., e.g. Build.cpp:507/536, Join.cpp:252)
— an index exists in the dump IFF some built region needed it.

CONSEQUENCE under Alt-A (FillDataModel knob-blind ⇒ %table:15 is STILL in
program.Tables()):
  - `create %table:15[u64,u64]` with its two `%col` lines **SURVIVES**
    as bytes — it is emitted because program.Tables() is data-model-driven,
    NOT usage-driven, and Alt-A keeps %table:15 in the model. (This answers
    the task's "does %table:15 survive?": the CREATE LINE survives; the
    table is dead content.)
  - Its index lines **VANISH**. Under the nested lowering the recognized
    subgraph replaces the join.6→%table:15→join.7 plumbing with the
    SUBGRAPH_INSTANTIATE rescan (GT-5), so no built region SCANS %table:15
    (drops the `%index:38[u64,_] on %col:16` Start,_ scan index) and no
    region WRITES/membership-checks it (drops the `%index:18[u64,u64]`
    full-key index). ⇒ the `create %table:15` block loses BOTH index lines
    (columns-only create). [Fork: if D2.b keeps a residual write into
    %table:15, %index:18 could survive — flagged; the clean expectation is
    both gone.]
  - %table:4 (pub): both indices SURVIVE — `%index:7` full-key (the answer
    is still membership-published) and `%index:57[u64,_]` Start,_ scan
    (P-D2b.2 "idx_57 unchanged"). The store publishes INTO %table:4.
  - %table:11 (edge): both indices SURVIVE — `%index:14` + `%index:32`
    Start,_ (the instantiate rescan reads edge via the keyed section walk;
    P-D2b.2 "edge's idx_32 unchanged").
  - %table:8 (demand): `%index:10` SURVIVES.

The `^flow:58` procedure body (appendix-C:81-104): the two `join-tables`
blocks + the `update-count … in %table:15` / `… in %table:4` become the
SUBGRAPH_INSTANTIATE region (per-demanded-key FindOrAddInstance →
TouchCurrent → rescan edge → publish into %table:4 → Seal at commit tail).
The join.7 re-guard against %table:8 collapses into the instance store's
keyed identity. (Shape only; the exact region grammar is D2.b's.)

----------------------------------------------------------------------
§3.2 `.h` — the instance member + V-INST-FRESH guard shape
----------------------------------------------------------------------

The generated Database struct gains ONE per-recognized-subgraph member:
  - `InstanceStore<Key, RowT> instance_0;` — I#0's store
    (include/drlojekyll/Runtime/InstanceStore.h:58-59, the StateCellStore
    transpose). Constructed with the allocator AND the RAT-4 monotone bool
    = TRUE at D2 (R-MONO): `instance_0(alloc, /*monotone=*/true)` (ctor
    InstanceStore.h:66). Key = the demanded key type (u64 Start); RowT =
    the published per-key row (the (Start,Node) / Node shape — D2.b's
    choice). Member NAME `instance_<id>` is illustrative (⟨PIN⟩).
  - The store surface the instantiate codegen emits AGAINST (actual member
    names, InstanceStore.h): FindOrAddInstance(key) (:105), TouchCurrent(iid)
    (:130 — the fresh-rebuild), Current/Frozen (:138-139), Touched()
    (:144), KeyAt(iid) (:153), WorkingOccupied/SealedOccupied (:161-168),
    Seal() (:174), RecycleCurrent(iid) (:216).

V-INST-FRESH guard shape (HP-7 / HP-11 KEEP): the per-epoch rebuild
discipline. The instantiate lowering, per demanded key, does
FindOrAddInstance → TouchCurrent(iid) [ADJ-C1 — CORRECTION: TouchCurrent
(InstanceStore.h:130-133) does NOT reset current; it calls Touch and
returns *current[iid]. Freshness comes from the PRIOR Seal's Reset (:202) —
a fresh mint is empty (:120) and, under R-MONO, a key is not re-touched
within a life (demand if-crossed). The rescan therefore writes into an
already-empty current. This is exactly why crit-correctness-1's a2
INCREMENTAL add is wrong: it would add onto that empty buffer without a
full rescan, leaving current partial and tripping the Seal belt] → drain
the demand net-additions frontier → rescan input
(%table:11) → publish the (F,T) net into %table:4. At commit tail the
kInstanceSeal lowers to Seal() (InstanceStore.h:174-207), whose
monotone-gated HP-7 belt asserts `frozen ⊆ current` per iid
(InstanceStore.h:185-198) — the (T,F) drop set is PROVABLY EMPTY under
R-MONO, so a would-be retract on the monotone pub trips a LOUD assert
instead. (The belt is the RAT-4 monotone-ctor-bool gate; R-DIFF at D3.a
constructs monotone=false.)


======================================================================
§4. CORRECTIONS LEDGER — this artifact vs the stale d1-desired-states §B.4
======================================================================

  1. forcing name: `neighborhood/bf` → `neighborhood` (NameAsString on the
     ParsedQuery is the bare name; DeltaRel.cpp:941-942 / Query.h:959).
  2. effect spellings (HP-11 collapse, LANDED): kInstanceEmit→kStateEmit,
     kInstanceOld→kStateOld, kInstanceSealSwap→kStateFold
     (DeltaRel.h:83-85; InstantiateEffects/SealEffect).
  3. edge ingest fold: effect-UNCHANGED → +kVecAppend(%table:11,
     kNetAddition) (OD-4 mechanism-natural).
  4. commit sweeps: ONE (kCommitSweep=1) → TWO (kCommitSweep=2), demand +
     edge (OD-4).
  5. deps: TWO illustrative edges → SIX mechanism-derived edges, including
     the previously-DENIED seal edges (op.0→op.1 WAW+WAR — kStateFold is a
     write) and the OD-4-only op.0→op.3 WAR + op.4→op.3 WAW.
  6. op labels: instance ops + sweeps mint BEFORE the ingest folds, so the
     flat op.0/op.1 ingest labels do NOT carry over (nested op.4/op.5).
  7. census: kCommitSweep=2, kSubgraphInstantiate=1, kInstanceSeal=1;
     census_total=6.
UNCHANGED-AND-CONFIRMED from §B.4/X-DS-1: no DRVec line for the monotone
frontiers (the appends/drains park via LOWER-time TableDeltaVector; the
hazard path is graceful, no by_vec edge).

CERTIFICATION: this block certifies — for demand_neighborhood_witness
under `-demand -demand-instance`, R-MONO-a, MECHANISM-NATURAL OD-4, HP-11
collapse — the OP SET (§2.0), the EFFECT MULTISETS (§2.2-§2.5), the LINE
ORDER (§2.0 band key), the CENSUS (§2.7), and the DEP-EDGE SET (§2.6). IDs
and the instantiate SPINE are ⟨PIN⟩-at-first-emission. HP-13(b): the
end-to-end review of the REAL nested `.deltarel` runs against THIS
contract before any nested golden is blessed. [ADJ-G1] The op.0 EFFECTS
multiset is the a1 set; under a2 it gains one kVecDrain(%table:11) — see the
a2 fork note in §2. CENSUS + DEPS + .df + .ir/.h are a1/a2-invariant.

======================================================================
§5. ADJUDICATION LEDGER (XHIGH adjudicator, 2026-07-20, tip 677977f8)
======================================================================
The 9 D2.b critic findings were verified AT CODE and all UPHELD. This
ds artifact is amended in place ([ADJ-*] markers). The findings that touch
the DUMP CONTRACT (this artifact's job) are:

  ADJ-G1 (crit-grammar-1, MED) UPHELD — the certified op.0 effects are the
    a1 set; a2 adds kVecDrain(%table:11) (InstantiateEffects
    DeltaRel.cpp:774-778). a2-fork recorded in §2 + §4. Census/deps/ir/h
    invariant. This is the DUMP consequence of the design's open R-REBUILD
    decision — the ds now carries BOTH branches so HP-13(b) can review
    against whichever lands.
  ADJ-G2 (crit-grammar-2, LOW) UPHELD — the non-byte spine annotation was
    moved OUT of the certified block; the block renders `spine: —` (the
    landed no-arms byte); the target chain stays an uncertified ⟨PIN⟩ in
    §2.3. §2 fixed.
  ADJ-C1 (crit-correctness-1, HIGH) UPHELD (design-side; ds §3.2 corrected)
    — TouchCurrent does NOT reset (InstanceStore.h:130-133); the ds's
    "TouchCurrent RESETS current" parenthetical was factually wrong and is
    corrected; a2 must FULL-RESCAN not incremental-add. NO dump-byte change
    (§2 block is a1; a2 is the effects fork above).
  ADJ-C3 / ADJ-P3 (crit-correctness-3 MED / crit-pins-3 LOW) UPHELD — this
    block's MECHANISM-NATURAL certification (kVecAppend on both folds,
    kCommitSweep=2, the 6 deps) is LOAD-BEARING only under a2; under a1-only
    the provisioned edge frontier is UNDRAINED dead work AND edge-after-
    demand is silently dropped (the witness's own demand-flap REBUILD batch
    fails). The dump SHAPE (this block) is identical either way; the LOWERING
    and the WITNESS .batches differ. If a1-only lands, the witness is
    reshaped birth-only and CLAUDE.md names the gap (design ADJ-C3).
  ADJ-C2 (crit-correctness-2, HIGH), ADJ-C4 (LOW), ADJ-P1 (HIGH),
    ADJ-P2 (MED): design-side (fence narrowing / ABA D3 precondition / HP-4
    arm B / HP-6 partition guardian). No dump-block consequence — this
    witness is non-recursive, single-adornment, and its α tags are all
    correct; recorded here for cross-reference to d2b-design.md §10.

FINAL VERDICT (ds half): GO-WITH-AMENDMENTS. The re-derivation from the real
flat dumps is CODE-FAITHFUL — two independent critics re-walked the 6-edge
deps and char-exact census and confirmed them; the forcing-name, HP-11
spellings, edge-fold append, kCommitSweep=2, seal write-hazard, and op-label
shift corrections vs stale §B.4 are all sound. The amendments add the a2
effects fork (ADJ-G1), remove the in-block non-byte annotation (ADJ-G2), and
correct the TouchCurrent-resets misstatement (ADJ-C1). This artifact
SUPERSEDES d1-desired-states §B.4 in whole and is the HP-13(b) review
contract, pending the R-REBUILD branch decision at the ritual head.
