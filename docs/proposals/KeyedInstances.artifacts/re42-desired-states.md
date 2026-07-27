======================================================================
COMMITTED AT THE R-E42 LANDING (2026-07-27; contracts pair with
re42-design.md; KeyedInstances.md §20(U) is the landing record). This is
the BINDING R-E42 DESIRED-STATES CONTRACT: the stage-(a) consolidated
record (the census tables, the M9 answer, the id-stream contract §5, the
extension points §6) followed by the stage-(d) AUTHOR lane's dump-blind
predictions (DS-RE42 below). THE THREE-WAY CONVERGENCE RECORD (the
precedent's SIXTH slice): author hand-prediction (dump-blind; full
predicted bodies for all four quad carriers + census-append diffs for
the other seven) == blind worktree prototype (own build at c8888e44;
implemented from the ratified design with no sight of the predictions)
== pristine implementation, BYTE-IDENTICAL on ALL 11 pinned surfaces
(orchestrator-executed cmp per E-77: quad bodies exact 4/4, census-only
append exact 7/7, pristine==converged 11/11). The author lane died at
its structured-output cap and was RECOVERED FROM ITS DISK FILE (the
R-JOIN precedent, never respawned) — the file was complete. The first
prototype dispatch was REFUSED by its own lane: the auto-provisioned
worktree materialized 99 commits stale, the lane stopped at its
verify-the-tip step (the ritual working as designed) and was
re-dispatched on a hand-provisioned worktree verified at c8888e44.
Bless: pre-bless reds EXACTLY the 11 pre-registered IRGOLD-DIVERGE
(single-hash x3), 0 IRGOLD-MISSING; all 11 bless sources byte-verified
same-as-converged BEFORE bless; post-bless SUITE PASS(174) x3; the
gate-battery + Fable-review records are appended at the tail.
======================================================================

# Part 1 — THE STAGE-(a) CONSOLIDATED RECORD (binding census + id-stream)

# R-E42 STAGE-(a) — THE BINDING CONSOLIDATED RECORD

Consolidator merge of Lane A (pseudocode / diff templates) + Lane B (census).
Tip 8f8dd07d (docs-only atop 18026049; binaries == 18026049). All anchors and
counts re-adjudicated at code/dumps/lldb this session.

Lane inputs:
- Lane A: `.../re42/stage-a-ingest-pseudocode.md`
- Lane B: `.../re42/stage-a-census.md`

VERDICT: the two lanes are MUTUALLY CONSISTENT and consistent with the code and
the §20(T) seed digest. Zero material conflicts. Lane B's census RESOLVES the
one question Lane A left open (see M9). Two code HAZARDS (not lane conflicts)
carried forward to stage (b).

================================================================================
## 1. CONSOLIDATOR CROSS-CHECKS PERFORMED

### 1.1 Lane-B lldb rows re-run by the consolidator (3 rows)
Method = Lane B's: `br set -f Procedure.cpp -l {52,77,95} -G true`, run, `br list`,
take MAX per-location hit among ExtendEagerProcedure-scoped locations (the
co-located-address + ProgramProcedureImpl::Equals over-count caveat CONFIRMED
live: map_3 line-95 AGGREGATE=3 but per-ExtendEager-location=1).

| case (mode) | Lane B says | consolidator lldb | verdict |
|---|---|---|---|
| map_3 (opt) | 0/0/1 | A=0 B=0 C=1 | MATCH |
| merge_2 (opt) | 0/0/3 | A=0 B=0 C=3 | MATCH |
| negate_1 (nodf) | 0/0/2 | A=0 B=0 C=2 | MATCH (both recvs flip B→C) |
| negate_1 (opt) | 0/2/0 | A=0 B=2 C=0 | MATCH (the mode-split flip) |

### 1.2 Independent golden cross-check (all 11 pins, opt)
The committed `.deltarel.opt.golden` census satisfies, for EVERY pin,
`kIngestFold_count == 2·A_opt + B_opt` — an independent confirmation of Lane B's
A/B opt column:

| pin | golden kIngestFold | 2·A+B (Lane B opt) |
|---|---|---|
| demand_tc_witness | 2 | 2·0+2 |
| symrec_tie_1 | 1 | 2·0+1 |
| map_3 | 0 | 2·0+0 |
| merge_2 | 0 | 2·0+0 |
| booleans | 2 | 2·0+2 |
| elim-cond-cycle-simple | 0 | 2·0+0 |
| negate_1 | 2 | 2·0+2 |
| negate_6 | 2 | 2·0+2 |
| d5_recursive_negate | 4 | 2·1+2 |
| join_1 | 0 | 2·0+0 |
| optimize_2 | 1 | 2·0+1 |

d5_recursive_negate=4 directly confirms the Arm-A fold-PAIR 2× law (A=1 → 2
kIngestFold blocks) in a committed golden. The four kIngestFold=0 pins
(map_3, merge_2, elim-cond-cycle-simple, join_1) are exactly the table-less quad.

### 1.3 Lane-A diff-template claims re-opened at code (2+ claims)
- **LowerIngestFold table hard-assert** (Stratum.cpp:1930 `assert(table != nullptr)`)
  — CONFIRMED; structurally blocks Arm C. The kCounter-first assert (:1937)
  CONFIRMED, and is FALSE for a vec-only op → must gate on the new kind.
- **Render null-deref trap** — CONFIRMED. kIngestFold render (Format.cpp:751)
  prints `tid(op.ingest_table)` UNCONDITIONALLY; `tid` (:382-383) derefs `t->id`
  with NO null guard → CRASH on a table-less op. Correct precedent CONFIRMED:
  kEagerForward (:873-880) guards `if (op.table_op_table) os << " table=..."`
  with an in-code comment "tid() has no null guard, so the `if` is load-bearing".
- **Site-5 naming mismatch** — CONFIRMED. Header comment :2099 reads
  "V-PRED-XCHECK Site 5"; the abort string :2153 reads "V-INGEST-XCHECK (Site 5)".
  The DEAD table-less filter (:2130-2131, skip `!op.ingest_stage1 &&
  ingest_table==nullptr`) CONFIRMED dead (enrollment never mints such an op).
- **Sole caller** (Procedure.cpp:820) and **Arm layout** (A:50-60 @call:56/model:52,
  B:74-93 @call:77, C:94-106 @VECTORLOOP:95-96 + VAR-per-col:100-101) —
  CONFIRMED verbatim against Procedure.cpp:14-111.

================================================================================
## 2. ADJUDICATED CARRIER CENSUS (11 pins × 4 modes) — A/B/C

Binding table (Lane B, spot-verified). A = Arm-A receives (deletion-capable,
read at Procedure.cpp:52; each emits 2 kIngestFold), B = Arm-B receives
(monotone table-bearing, :77), C = Arm-C receives (table-less monotone, :95 —
THE E-42 SURFACE, one VECTORLOOP shim + arity VARs each, NO DR-IR op today).

| carrier | opt | nodf | nocf | none |
|---|---|---|---|---|
| demand_tc_witness | 0/2/0 | 0/2/0 | 0/2/0 | 0/2/0 |
| symrec_tie_1 | 0/1/0 | 0/1/0 | 0/1/0 | 0/1/0 |
| map_3 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 |
| merge_2 | 0/0/3 | 0/0/3 | 0/0/3 | 0/0/3 |
| booleans | 0/2/0 | 0/1/1 | 0/2/0 | 0/1/1 |
| elim-cond-cycle-simple | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 |
| negate_1 | 0/2/0 | 0/0/2 | 0/2/0 | 0/0/2 |
| negate_6 | 0/2/0 | 0/1/1 | 0/2/0 | 0/1/1 |
| d5_recursive_negate | 1/2/0 | 1/2/0 | 1/2/0 | 1/2/0 |
| join_1 | 0/0/2 | 0/0/2 | 0/0/2 | 0/0/2 |
| optimize_2 | 0/1/0 | 0/0/1 | 0/1/0 | 0/0/1 |

ADJUDICATED LAWS (both lanes agree; consolidator confirms):
- **nocf axis inert**: opt==nocf, nodf==none for every carrier. Arm selection is
  DATAFLOW-model-determined (table presence), settled before ControlFlow opt.
- **nodf flips B→C** for booleans/negate_1/negate_6/optimize_2: disabling
  CSE/canonicalization denies a receive its shared model table.
- **Mode-split family** (armC differs by mode, dataflow-opt axis only):
  booleans, negate_1 (both recvs flip), negate_6, optimize_2.
- **Mixed-ingest** (armC>0 AND armB>0 same mode): booleans, negate_6 under
  nodf/none only. No carrier is Arm-C-mixed under opt/nocf.

Corpus sweep (opt, Arm C only, 173 cases): Σ armC = 125 table-less receives over
80/173 cases; histogram 0→93, 1→46, 2→27, 3→5, 4→1 (cf14_6), 6→1
(cond_in_induction). This is the WIDEST existing-surface churn of the epoch;
modeled-replacement id-alloc impact = Σ over Arm-C receives of (1+arity).

================================================================================
## 3. THE M9 ANSWER (the stage-(a) deliverable)

**YES — every kIngestFold=0 quad member has ≥1 table-less monotone (Arm-C)
receive; the digest's "candidate carriers" are CONFIRMED real block-carriers.**

Triple-confirmed:
1. lldb armC opt > 0 for all four: map_3=1, merge_2=3, elim-cond-cycle-simple=1,
   join_1=2 (map_3 and merge_2 re-run by the consolidator; MATCH).
2. Model-side eqset derivation (Lane B, E-107/OD-13): each quad recv's `eqset=`
   shares NO `table=%table:N` block → table-less at the merged model. Reconciles
   EXACTLY with lldb on all six checked (quad + symrec + demand_tc).
3. Golden identity: kIngestFold=0 in the committed golden ⇒ the case's receives
   reach NEITHER Arm A (would enroll 2) NOR Arm B (would enroll 1); a
   message-driven program's receives must go somewhere, so they take Arm C.

This CLOSES Lane A's §6 "open refinement" ("has a table-less receive" vs "no
receives reach Arm C at all"): Lane B's positive armC counts + model derivation
settle it — no separate M12 probe is needed for the ruling.

demand_tc_witness (kIngestFold=2) and symrec_tie_1 (kIngestFold=1) are NOT
table-less carriers: their recvs render `class=table-less` on the recv LINE but
are model-table-BACKED via a merged eqset (Arm B) — the E-107 shape, not Arm C.

CENSUS-LINE CHURN INVENTORY (all 11 pins churn; 26→27 kinds — kEagerProduct=(25)
is the current last kind, a new kind is index 26 / the 27th):
- **Gain REAL op blocks (opt mode)**: map_3 (+1), merge_2 (+3),
  elim-cond-cycle-simple (+1), join_1 (+2). These 4 = the kIngestFold=0 quad.
- **Census-LINE churn only, new-kind count = 0 (opt)**: demand_tc_witness,
  symrec_tie_1, booleans, negate_1, negate_6, d5_recursive_negate, optimize_2 (7
  pins) — the kAllKinds list grows so the census line changes, but no new block.

================================================================================
## 4. PRE-REGISTERED PREDICTION — new-kind census count per pin @ pinned mode

Pinned mode = OPT (the `.deltarel.opt.golden` / `.irgold` pins are opt-mode).
Predicted new-kind census count = Lane B armC opt count (verified §1.2/§2).

| pin (opt) | predicted new-kind count | gains blocks? |
|---|---|---|
| demand_tc_witness | 0 | no (line churn only) |
| symrec_tie_1 | 0 | no |
| map_3 | 1 | YES |
| merge_2 | 3 | YES |
| booleans | 0 | no |
| elim-cond-cycle-simple | 1 | YES |
| negate_1 | 0 | no |
| negate_6 | 0 | no |
| d5_recursive_negate | 0 | no |
| join_1 | 2 | YES |
| optimize_2 | 0 | no |

d5_recursive_negate is the natural ZERO-MINT negate guard for this slice too
(its walk-cut recursive negate mints no gate/join marker AND its receives are
Arm A/B, never Arm C → new-kind count 0).

================================================================================
## 5. THE ID-STREAM CONTRACT (binding; Lane A §2, code-confirmed)

Arm C mints, IN ORDER: (1) VECTORLOOP `next_id++` (Procedure.cpp:96), then (2)
one VAR `next_id++` per receive column (:100-101) → TOTAL **1 + arity** ids.
LowerIngestFold's Arm-B path mints the SAME 1+arity in the SAME order
(VECTORLOOP :1941, then VAR-per-col :1951; the interposed UPDATECOUNT :1945 is
id-FREE). So a modeled Arm-C op must reproduce 1+arity ids at the ORIGINAL walk
position via the **LowerIngestFold HOLE-CONTRACT trick** (mint the loop, return
it as the descent cursor) — NOT the zero-next_id marker-ctor trick of R1..R-JOIN
(that trick does NOT transfer; M16 applies). BuildEagerInsertionRegions is
UNTOUCHED and called identically. A/B gate = per-slice BYTE-IDENTITY (Arm C's
emission is unchanged; only its provenance — a real op replaces "no op").

================================================================================
## 6. STAGE-(b) OPEN QUESTIONS (reconciled — the ritual-head rulings)

1. **OP-FAMILY (lead ruling).** (a) new INGEST-family sibling kind (e.g.
   kIngestLoop; reuse ingest_* payload, extend Site-5/census/key-multiset —
   the in-code :2366 comment already frames it as ingest-adjacent) vs (b)
   eager-marker kind (REJECTED-BY-CONSTRUCTION: markers are effect-free and
   mint ZERO next_id, but Arm C MUST allocate 1+arity — and E-42 is an INGEST
   surface, not a BuildEagerRegion walk-dispatch arm) vs (c) widen kIngestFold
   itself (overloads one kind with fold-into-table and loop-only shapes, breaks
   the clean "kIngestFold ⇒ a fold into a table" count reading, and SHIFTS the
   four kIngestFold=0 pins). Lane-A analysis favors (a); (b) is disfavored on
   both layer and id-discipline grounds.

2. **LOWERING shape.** Extend LowerIngestFold with a table==null arm (relax
   :1930, skip the UPDATECOUNT, return the VECTORLOOP) vs a sibling
   LowerIngestLoop. Either way: allocate 1+arity at the walk position, return
   the VECTORLOOP as descent cursor, replace Procedure.cpp:95-105 with one call.

3. **PAYLOAD (M2').** Store message+receive (kIngestFold precedent; receive
   gives `.Columns()` for the VARs, message for render) vs store only
   ingest_receive and re-derive the message.

4. **COUNT LAW.** PER-RECEIVE (not per-visit): count == # table-less monotone
   receives == # Arm-C fires == # VECTORLOOP shims minted at :95-96. Census
   recount adds a THIRD arm to the IOs×Receives loop (DeltaRel.cpp:3403-3436):
   `else /* table==null && !CanReceiveDeletions */ : exp_<newkind> += 1` under
   a SEPARATE counter (must NOT absorb into exp_ingest, or the kIngestFold count
   law breaks and the four kIngestFold=0 pins shift).

5. **ORDERING / key_of.** Table-less ingest op ⇒ op_table_id==0 (all table ptrs
   null) ⇒ LEAD band (lead-0, stratum-0, band-0, table_id-0), tie-broken sign
   then ctor (construction index oi = the walk's IOs×Receives order). Reuse the
   kIngestFold key_of arm or a dedicated mirror; VALIDATOR-ORDERING ONLY.

6. **RENDER (HAZARD — do NOT reuse the kIngestFold render case verbatim: it
   null-derefs on a table-less op).** Follow the kEagerForward guarded pattern
   `if (op.table_op_table) os << " table=..."` (Format.cpp:877-878) → OMIT the
   `table=` token; carry `message=<name>/<arity>` (existing spelling) or a
   dedicated receive=/loop= token. Vec-only op ⇒ `effects: {}` (empty) unless a
   new read/vec effect kind is modeled. kAllKinds (Format.cpp:1054-1068) MUST
   list the new kind or the census_total guard (:1077) aborts.

7. **GUARDS / naming.** The kCounter-first asserts (Stratum.cpp:1937/:2135,
   DeltaRel.cpp:3177) are FALSE for a vec-only op → gate on the new kind. If the
   modeled op makes the Site-5 dead table-less filter (:2130-2131) LIVE,
   reconcile the header/abort naming (":2099 V-PRED-XCHECK" vs ":2153
   V-INGEST-XCHECK"). Optional Arm-C cursor-shape guard analog: assert the
   returned cursor is a VECTORLOOP over the param vec (mirrors the Arm-B
   INGEST-CURSOR-SHAPE guard :87-93, which today asserts an UPDATECOUNT).

================================================================================
## 7. CONFLICTS

NONE (lane-vs-lane, lane-vs-code, or lane-vs-§20(T)-seed). Notes:
- Lane A left the "has-a-table-less-receive" question OPEN (a possible M12
  probe); Lane B RESOLVED it three ways (§3). Reconciliation, not conflict.
- Minor anchor freshness: seed cites Site 5 at ":2101ff"; Lane A's fresh read
  puts the block header at :2099 and abort at :2153. Lane A's tip-8f8dd07d read
  is authoritative; no material drift.
- Two CODE HAZARDS surfaced (Lane A caught, consolidator confirmed): the render
  null-deref trap and the Site-5 header/abort naming mismatch. Both are
  stage-(b) implementation notes, not lane disagreements (§6.6/§6.7).

======================================================================
# Part 2 — THE STAGE-(d) AUTHOR PREDICTIONS (dump-blind; converged)
======================================================================

# R-E42 STAGE-(d) — AUTHOR LANE (dump-blind hand-prediction)

Tip 1ccc9be3, READ-ONLY. No modified compiler built or run. Pristine tip
compiler used only to reproduce CURRENT dumps (== committed goldens) and to
recover message names / IO order (df dump + generated C++ handler order).

DERIVATION SPINE (all code-anchored):
- Format.cpp:318 `SignGlyph`: `s<0→"-"`, `s>0→"+"`, `s==0→"·"(\xC2\xB7)`.
  kIngestLoop has `ingest_sign=+1` ⇒ header renders `sign=+` (unlike every
  committed eager marker / kIngestFold-in-quad, which are `·`... the quad has
  NO kIngestFold; kIngestFold elsewhere is also `sign=+`).
- RH-7 MARKER-SHAPE render (Format.cpp new case ~:758): header line
  `sign=+ ctx=eager stratum=N` + one `args:` line, NO reads/effects/spine
  sublines. `ctx=eager` (CtxName(kEager)); `stratum` = DROpStratum default arm
  (:4233 `default: return 0u`) ⇒ **stratum=0**. `table=` OMITTED (table-less;
  the kEagerForward null-guard precedent). `message=<Name>/<Arity>` reusing the
  kIngestFold token spelling (RH-8, no E-71).
- RH-4 TAIL-APPEND enrollment: kIngestLoop ops get the HIGHEST oi (construction
  index) because nothing pushes to flow.ops after EAGER_WEB. Current
  flow.ops.size() == census_total (verified per carrier) ⇒ new oi values are
  contiguous from the current op count.
- RH-9 key_of (DeltaRel.cpp:4525 new arm): `Key{0,0,0, op_table_id(op)=0,
  ingest_sign=+1, oi}`. key_less order = lead,stratum,band,table_id,sign,ctor
  (:4575). ⇒ kIngestLoop sorts in the **lead-0 / table_id-0** band, AFTER the
  sign-0 eager markers that carry no table (table_id 0, sign 0), BEFORE any
  table-backed op (table_id ≥ 1). Multiple kIngestLoop tie-break by oi
  (= enrollment IOs()×Receives() order).
- RH-1 census (Format.cpp kAllKinds :1054-1068): `kIngestLoop` APPENDED at the
  TAIL (after kEagerProduct) ⇒ census line gains ` kIngestLoop=<N>` as the
  final token; the 26-token prefix is byte-identical. census_total guard
  (:1077) holds (kAllKinds grows by the kind; flow.ops grows by the blocks).

## ID-STREAM / BYTE-IDENTITY (why generated code is unchanged)
LowerIngestLoop is the byte-move of Arm C (VECTORLOOP `next_id++` + one VAR
`next_id++` per receive column = 1+arity, at the ORIGINAL walk position),
returned as the descent cursor (HOLE-CONTRACT). next_id stream identical;
BuildEagerInsertionRegions untouched ⇒ every generated datalog.h/.cpp byte-
identical to frozen. Only PROVENANCE changes (a real op replaces "no op").

================================================================================
## MESSAGE / IO-ORDER FACTS (recovered from pristine dumps)

| carrier | table-less receives (df `recv #message`) | IOs() order (gen C++ handler order) |
|---|---|---|
| map_3 | m/2 (select.2) | m |
| merge_2 | m1/2, m2/2, m3/2 (one each after CSE) | m1, m2, m3 |
| elim-cond-cycle-simple | cond_func/1 (select.0) | cond_func |
| join_1 | t1/2 (select.0), t2/1 (select.1) | t1, t2 |

- merge_2: original 5 receives (m1×2,m2×2,m3×1) CSE-collapse to 3 (one per
  message). IOs()=[m1,m2,m3] confirmed 3 ways: (i) first-encounter build order
  in merge_2.dr (m1 clause 1, m2 clause 2, m3 clause 4); (ii) ios.RemoveIf
  preserves build order through Optimize; (iii) generated handler functions
  emit m1_2, m2_2, m3_2 in that order. ⇒ oi(20)=m1, oi(21)=m2, oi(22)=m3.
- join_1: IOs()=[t1,t2] (t1 clause 1, t2 clause 2; handlers t1_2 then t2_1).
  ⇒ oi(18)=t1/2, oi(19)=t2/**1** (t2 is arity 1 — token `message=t2/1`).

================================================================================
## §3 — merge_2 MULTI-RECEIVE ORDER (the 3 new ops)

Enrollment loop `for io in query.IOs()` × `for receive in io.Receives()`.
IOs()=[m1,m2,m3]; each io has exactly ONE table-less receive post-opt. oi is
assigned in that nested order → **oi 20 = m1/2, oi 21 = m2/2, oi 22 = m3/2**.
All three carry table_id 0 / sign +1, so within the lead-0/table_id-0 band they
tie-break by oi and render in the same 20,21,22 order (m1, m2, m3). Confirmed
against the generated message-handler order (m1_2, m2_2, m3_2).

================================================================================
## §1 — FULL PREDICTED POST-SLICE .deltarel.opt BODIES (4 quad carriers)

Each is the committed golden with the derived kIngestLoop block(s) inserted at
the lead-0/table_id-0/sign+1 slot and the census tail extended. Byte-exact.

--------------------------------------------------------------------------------
### map_3.deltarel.opt (kIngestLoop=1; new block oi=10 inserted after op.7)
--------------------------------------------------------------------------------
```
deltarel

op.0 kEagerGenerate sign=· ctx=eager stratum=0
    args: functor=add_i32/3
op.3 kEagerGenerate sign=· ctx=eager stratum=0
    args: functor=add_i32/3
op.6 kEagerCompare sign=· ctx=eager stratum=0 cmp=eq
    args:
op.7 kEagerGenerate sign=· ctx=eager stratum=0
    args: functor=add_i32/3
op.10 kIngestLoop sign=+ ctx=eager stratum=0
    args: message=m/2
op.1 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:6
op.2 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:6
op.4 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:10
op.5 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:10
op.8 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:15
op.9 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:15

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=3 kEagerInsert=3 kEagerCompare=1 kEagerGenerate=3 kEagerUnion=0 kEagerSelect=0 kEagerJoin=0 kEagerProduct=0 kIngestLoop=1
```
(census_total 10+1 = 11 = flow.ops.size())

--------------------------------------------------------------------------------
### merge_2.deltarel.opt (kIngestLoop=3; new blocks oi=20,21,22 after op.16)
--------------------------------------------------------------------------------
```
deltarel

op.0 kEagerForward sign=· ctx=eager stratum=0
    args:
op.4 kEagerForward sign=· ctx=eager stratum=0
    args:
op.8 kEagerForward sign=· ctx=eager stratum=0
    args:
op.12 kEagerForward sign=· ctx=eager stratum=0
    args:
op.16 kEagerForward sign=· ctx=eager stratum=0
    args:
op.20 kIngestLoop sign=+ ctx=eager stratum=0
    args: message=m1/2
op.21 kIngestLoop sign=+ ctx=eager stratum=0
    args: message=m2/2
op.22 kIngestLoop sign=+ ctx=eager stratum=0
    args: message=m3/2
op.5 kEagerUnion sign=· ctx=eager stratum=0
    args: table=%table:4
op.6 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:4
op.7 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:4
op.13 kEagerUnion sign=· ctx=eager stratum=0
    args: table=%table:4
op.14 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:4
op.15 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:4
op.17 kEagerUnion sign=· ctx=eager stratum=0
    args: table=%table:4
op.18 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:4
op.19 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:4
op.1 kEagerUnion sign=· ctx=eager stratum=0
    args: table=%table:8
op.2 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:8
op.3 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:8
op.9 kEagerUnion sign=· ctx=eager stratum=0
    args: table=%table:8
op.10 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:8
op.11 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:8

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=10 kEagerInsert=5 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=5 kEagerSelect=0 kEagerJoin=0 kEagerProduct=0 kIngestLoop=3
```
(census_total 20+3 = 23 = flow.ops.size())

--------------------------------------------------------------------------------
### elim-cond-cycle-simple.deltarel.opt (kIngestLoop=1; new block oi=11 after op.8)
--------------------------------------------------------------------------------
```
deltarel

op.0 kEagerForward sign=· ctx=eager stratum=0
    args:
op.5 kEagerJoin sign=· ctx=eager stratum=0
    args:
op.6 kEagerForward sign=· ctx=eager stratum=0
    args:
op.8 kEagerJoin sign=· ctx=eager stratum=0
    args:
op.11 kIngestLoop sign=+ ctx=eager stratum=0
    args: message=cond_func/1
op.2 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:5
op.3 kEagerSelect sign=· ctx=eager stratum=0
    args: table=%table:5
op.4 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:5
op.1 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:8
op.9 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:12
op.10 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:12
op.7 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:15

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=6 kEagerInsert=2 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=1 kEagerJoin=2 kEagerProduct=0 kIngestLoop=1
```
(census_total 11+1 = 12 = flow.ops.size())

--------------------------------------------------------------------------------
### join_1.deltarel.opt (kIngestLoop=2; new blocks oi=18,19 after op.15)
--------------------------------------------------------------------------------
```
deltarel

op.2 kEagerJoin sign=· ctx=eager stratum=0
    args:
op.5 kEagerJoin sign=· ctx=eager stratum=0
    args:
op.8 kEagerJoin sign=· ctx=eager stratum=0
    args:
op.11 kEagerJoin sign=· ctx=eager stratum=0
    args:
op.14 kEagerCompare sign=· ctx=eager stratum=0 cmp=eq
    args:
op.15 kEagerCompare sign=· ctx=eager stratum=0 cmp=eq
    args:
op.18 kIngestLoop sign=+ ctx=eager stratum=0
    args: message=t1/2
op.19 kIngestLoop sign=+ ctx=eager stratum=0
    args: message=t2/1
op.12 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:6
op.13 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:6
op.16 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:9
op.17 kEagerInsert sign=· ctx=eager stratum=0 sink=relation
    args: table=%table:9
op.0 kEagerCompare sign=· ctx=eager stratum=0 cmp=eq
    args: table=%table:12
op.1 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:12
op.6 kEagerCompare sign=· ctx=eager stratum=0 cmp=eq
    args: table=%table:16
op.7 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:16
op.3 kEagerCompare sign=· ctx=eager stratum=0 cmp=eq
    args: table=%table:19
op.4 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:19
op.9 kEagerCompare sign=· ctx=eager stratum=0 cmp=eq
    args: table=%table:23
op.10 kEagerForward sign=· ctx=eager stratum=0
    args: table=%table:23

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=6 kEagerInsert=2 kEagerCompare=6 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0 kEagerJoin=4 kEagerProduct=0 kIngestLoop=2
```
(census_total 18+2 = 20 = flow.ops.size())

================================================================================
## §2 — CENSUS-ONLY DIFFS (the other 7 pins; no new ops, NO renumbering)

Under RH-4 tail-append EVERY existing op.N label is byte-stable (kIngestLoop
takes the highest oi and, for these 7, mints 0 blocks). The SOLE change is the
census line's appended ` kIngestLoop=0`. Verified nothing else can change:
kIngestFold count unchanged (separate counter), key_of adds only the tail band,
no op added ⇒ pinned_order identical.

For each: unified diff on the census line (last token was `kEagerProduct=<N>`):
```
demand_tc_witness:  ...kEagerJoin=8 kEagerProduct=0            → append " kIngestLoop=0"
symrec_tie_1:       ...kEagerJoin=4 kEagerProduct=0            → append " kIngestLoop=0"
booleans:           ...kEagerJoin=4 kEagerProduct=0            → append " kIngestLoop=0"
negate_1:           ...kEagerJoin=0 kEagerProduct=0            → append " kIngestLoop=0"
negate_6:           ...kEagerJoin=0 kEagerProduct=0            → append " kIngestLoop=0"
d5_recursive_negate:...kEagerJoin=0 kEagerProduct=0            → append " kIngestLoop=0"
optimize_2:         ...kEagerJoin=0 kEagerProduct=2            → append " kIngestLoop=0"
```
Example unified diff (demand_tc_witness, sole hunk):
```
@@ census line @@
-census: ... kEagerJoin=8 kEagerProduct=0
+census: ... kEagerJoin=8 kEagerProduct=0 kIngestLoop=0
```
(identical hunk shape for all 7; only the pre-existing count values differ.)

================================================================================
## §4 — [BYTE]/[STRUCT] PREDICTION LIST (stage (e))

### Pre-bless reds (irgold gate)
- **11 IRGOLD-DIVERGE, 0 IRGOLD-MISSING.** Every one of the 11 pins already
  carries a `.deltarel.opt.golden` + `.irgold` sidecar ⇒ re-bless only; a new
  census token diverges all 11, the 4 quad additionally diverge on the inserted
  block(s). No pin is MISSING (none is newly minted).

### [STRUCT] (irgold re-bless gated — NOT byte-identity)
- 11 `.deltarel.opt` goldens: census token appended on ALL 11.
- MARKER-SHAPE kIngestLoop blocks added on map_3 (+1, m/2), merge_2 (+3,
  m1/m2/m3), elim-cond-cycle-simple (+1, cond_func/1), join_1 (+2, t1/2,t2/1).
- The 4 quad bodies above are the exact predicted STRUCT bytes.

### [BYTE] (byte-identical to frozen e6eb2e3e/035720ac)
- All generated `datalog.h`/`.cpp` on 173 cases × 4 modes: BYTE-IDENTICAL
  (LowerIngestLoop is the byte-move of Arm C; next_id 1+arity at the original
  position; BuildEagerInsertionRegions untouched).
- The 7 named untouched golden surfaces BYTE-IDENTICAL:
  demand_tc_witness .h / .ir / .df; symrec_tie_1 .ir / .df; negate_1 .df;
  aggregate_1 .df. (Slice touches only the DeltaRel dump + census — DataFlow
  (.df), IR (.ir), and codegen (.h) are all upstream/orthogonal.)
- All 173 stdout goldens (the 4 optimization modes) BYTE-IDENTICAL (emission
  unchanged) — the suite passes without any stdout re-bless; ONLY the 11
  `.irgold` deltarel goldens re-bless.

### Cross-knob predictions — the quad (DS-ADJ-1: opt==nocf, nodf==none)
kIngestLoop census count per (carrier, mode); the quad are NOT mode-split, so
all 4 modes agree with opt:
```
carrier                 opt  nodf  nocf  none
map_3                    1    1     1     1
merge_2                  3    3     3     3
elim-cond-cycle-simple   1    1     1     1
join_1                   2    2     2     2
```
opt==nocf and nodf==none hold (nocf axis inert — arm selection is dataflow-
model-determined). The quad `.deltarel.opt` pins (opt mode) get the blocks
above; nodf/none unpinned but predicted identical block counts.

### Cross-knob predictions — the 4 mode-split carriers (nodf/none flip B→C)
These pins are OPT-mode (kIngestLoop=0 pinned) but gain kIngestLoop blocks
under nodf/none (UNPINNED — opt-only pinning discipline, matches R4):
```
carrier      opt(pinned)  nodf/none(unpinned)
booleans        0            1   (B=1/C=1 mixed-ingest)
negate_1        0            2   (both recvs flip B→C)
negate_6        0            1   (B=1/C=1 mixed-ingest)
optimize_2      0            1
```

================================================================================
## CENSUS TOKEN POSITION (summary)
`kIngestLoop=<N>` is the FINAL token on the census line, appended immediately
after `kEagerProduct=<N>` (kAllKinds tail-append; the 26-token prefix through
kEagerProduct is byte-identical on all 11 pins).

================================================================================
## UNCERTAINTIES / RESIDUAL RISK (all LOW; enumerated for the referee)
1. merge_2 3-block ORDER (m1,m2,m3): derived from ios first-encounter + gen-C++
   handler order (m1_2,m2_2,m3_2). If query.IOs() were ever re-sorted post-opt
   (it is NOT — ios.RemoveIf preserves order), the order could differ. Triple-
   corroborated ⇒ LOW risk. Same reasoning pins join_1 (t1 before t2).
2. RH-7 render shape: I predict MARKER-SHAPE (owner-ratified). If the owner had
   chosen fold-shape, each block would gain two sublines `    effects: {}` and
   `    spine: —` between header and args:. Ratified rulings say MARKER-SHAPE.
3. `stratum=0` assumes kIngestLoop hits DROpStratum's `default: return 0u`
   (:4233) — it is not in any switch case ⇒ default fires. Confirmed.
4. sign glyph `+` assumes ingest_sign is stored/rendered as +1 (ctor sets
   `ingest_sign = 1`; SignGlyph(+1)="+"). Confirmed at Format.cpp:318-321.
5. The exact new oi VALUES (10 / 20,21,22 / 11 / 18,19) assume flow.ops.size()
   == current census_total with the kIngestLoop loop being the last push. Each
   carrier's census_total was summed to match its highest op.N+1. Confirmed.
6. Non-quad pins: I assert ZERO op renumbering. Relies on nothing pushing to
   flow.ops between EAGER_WEB and the new tail loop (design D4 verified: only
   validators/DeriveDRStrata follow). LOW risk.

======================================================================
# Part 3 — STAGE-(e) GATE + FABLE-REVIEW RECORDS (final tree)
======================================================================

DS-RE42 CONVERGENCE (orchestrator-executed cmp, E-77): author ==
prototype == pristine implementation, 11/11 byte-identical (quad full
bodies 4/4; census-append 7/7; pristine==converged 11/11).

GATES (final tree; every referee named in a binding pin executed by the
orchestrator personally): suite pre-bless EXACTLY the 11 pre-registered
IRGOLD-DIVERGE (single red-list hash x3, 0 IRGOLD-MISSING,
agg_distinct_1 green) -> bless (11/11 sources byte-verified
same-as-converged BEFORE bless; git = exactly the 11 sanctioned
goldens) -> SUITE PASS(174) x3 + post-fix; A/B vs frozen c8888e44
(debug 0ed9cb3a / release e238bb17): 844 rows (696 corpus x4 + 4
nested + 144 data/) 0-DIVERGED (778 identical + 62 identical-reject +
4 identical-abort, evm_array_parse SIGABRT baseline stands) + 64-row
post-fix subset 0-diverged; ctest 5/5 debug + 5/5 ASAN; ASAN BOTH
surfaces SUITE PASS(174) zero reports (the gate lane returned a
placeholder and was ORCHESTRATOR-EXECUTED; post-fix ASAN re-run
green); config-invariance SINGLE-HASH x22 (11 carriers x opt+none,
3-run debug + release); E-62 re-grep CLEAN (live); M15 count oracle
44/44 (lldb LowerIngestLoop hit-count == census kIngestLoop per
(case, mode); cross-knob law opt==nocf / nodf==none held x11 incl.
the four predicted nodf B->C flips); Q5 progsize@128 release
SAME-SESSION INTERLEAVED ABABAB A warm {154,157,153} vs B
{155,155,153,155} ms (~0.6% median, noise; headers byte-identical;
A1 cold discarded — the ADJ-S8 MEASURED gate).

FABLE REVIEW (15-agent workflow, high): 6 findings, ZERO live
correctness — [1] the loop family lacked a §7b-sibling duplicate-
receive guard (all other checks re-derive from the same IOs×Receives;
FIXED: a new always-on enrolled-side guard); [2] kIngestLoop fell to
the per-op validation switch's default (shape invariants rested on
NDEBUG-stripped asserts; FIXED: a new always-on per-op ctor-contract
arm); [0] the EAGER_WEB ADJ-S2 tail-position comment went stale
(FIXED: re-worded — the INGEST_LOOP family is the new tail under the
same pin); [6] the INGEST_FOLD table-less comment lacked the
cross-reference (FIXED); [3] the key_of loop arm was a verbatim copy
of the fold arm (FIXED: merged, one arm both ingest kinds); [4] four
hand-packed key sites (FIXED: IngestLoopKeyOf single flow-side
authority + derivation-side citations). 2 REFUTED. Fixes proven
DUMP-NEUTRAL (18/18 pinned surfaces byte-identical post-fix) +
post-fix suite/ctest/A-B/ASAN green.
