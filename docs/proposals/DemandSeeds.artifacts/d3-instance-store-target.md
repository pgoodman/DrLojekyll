======================================================================
d3-instance-store-target.md — LANE D3: THE InstanceStore REDESIGN
+ THE DEMAND-WIRED WITNESS
the two p2b-judge HIGH holes closed on E-35-corrected substrate facts
======================================================================
Designer-lane artifact, demand-seeds / keyed-instances epoch. Branch
`demand-seeds` (tip 84bb39f1 = main ca569dd8 + the D0 ledger commit).
Written 2026-07-17. PAPER-FIRST: this artifact fully specifies the store
(§1-§3), the demand-wired witness (§4), and the census/validators (§5)
BEFORE any emission code. A future implementer builds FROM this; a judge
will try to falsify it. Every code claim carries a file:line anchor READ
THIS session on THIS branch.

SCOPE: D3 of the epoch diff plan (DemandSeeds.md §2). It closes the two
HIGH-severity holes the p2b judge left open (SubgraphsDemand.md §3, the
p2b judge findings; p2b-instance-target.md §7.3 the double-count probe,
§3.1 the sealed-side claim), USING the E-35-corrected substrate facts
(the D0 consolidated record's 19 verified facts + errata E-35). It does
NOT re-open the acyclic-frozen-first regime decision (p2b §1.5 RATIFIED)
and does NOT re-design the demand transform (D1's lane). It CONSUMES D1's
mechanism AS JUDGED (verdict REVISE — the demand message is a fabricated
`ParsedMessage` at DataFlow-build time, judge-d1 F1; a codegen entry-point
suppression, F2) to wire the §4 witness's demand edge.

THE TWO HOLES D3 CLOSES, NAMED:
  HOLE-A (p2b §3.1 / E-35): the p2b sketch says the instance sealed side
    rides "the nested DiffTable's OWN kInI watermark (Table.h Seal/
    sealed)". E-35 is the REAL-DEFECT: a DiffTable has NO Seal()/sealed
    (those are monotone-Table-only, Table.h:277-282); a store-held row-id
    watermark is unsound under CompactDead (Table.h:584-585). §1 redesigns
    old(iid) in terms the runtime actually has.
  HOLE-B (p2b §7.3): the two-table double-count seam — can a published row
    be counted twice when the instance guard drops it (demand retracted)
    AND it is independently retracted inside the nested relation the SAME
    batch? p2b §3.3 asserts a claim-gate-order argument but "does not fully
    discharge" it. §2 answers it head-on, redesigning to a SOLE-WRITER
    invariant so the seam cannot arise.


======================================================================
§0. VERIFIED-THIS-SESSION ANCHOR TABLE
======================================================================
The load-bearing runtime sites this artifact reasons from; all read THIS
session on branch demand-seeds (== main ca569dd8 substrate).

RUNTIME — Table.h (include/drlojekyll/Runtime/Table.h):
  A1  :36-44     RowFlags: kInI=1<<0 (present at batch start, "frozen I"),
                   kDel/kAdd/kDelNow/kAddNow scratch, kExplicit, kTouched
  A2  :117-118   RowStore::CompactRowsInPlace — STABLE densify, moved(old,
                   new) cb, RENUMBERS ids (the caller owns rebuilding every
                   id-keyed structure)
  A3  :224-283   MONOTONE Table<Row>: Seal() :277-278 (sealed:=NumRows()),
                   sealed field :282, InI :249-252 (id<sealed), NetAdded
                   :263-266 (id>=sealed) — the id-order watermark
  A4  :300-604   DiffTable<Row>: NO Seal/sealed. InI :357-360 (flags[id]&
                   kInI), Commit :517-551 (kInI:=(counts>0), publishes
                   was!=now via sink), NetAdded :429-433 (kAdd&&!kDel&&
                   !kInI), NetDeleted :405-409 (kDel&&!kAdd)
  A5  :517-551   DiffTable::Commit — the per-touched-row sweep; sink(row,
                   now) fires ONLY on was!=now; num_live accounting :532-540
  A6  :565-604   NeedsCompaction :565-575 (dead>=live, 4096 floor);
                   CompactDead :586-604 (renumbers; caller Clear()s +
                   rebuilds indexes)
  A7  :584-585   verbatim: "kInI/kExplicit travel with the row; no watermark
                   exists on a differential table, so nothing else remaps"
                   — the E-35 crux (why a store-held row-id is unsound)
  A8  :610-620   DiffTable::DebugValidateCounts — non-neg per class, no
                   scratch flag survives, kInI==(Total>0)

RUNTIME — StateCell.h (include/drlojekyll/Runtime/StateCell.h):
  B1  :10-24     non-aliasing header contract: dense group ids a SEPARATE
                   id space from the agg DiffTable row ids; CompactDead
                   never touches the store; append-only dense namespace
  B2  :26-36     the TWO-WORD cell: working (Fold writes / Emit reads —
                   hazard) + sealed (Seal writes / Old reads — frozen)
  B3  :38-54     OCCUPANCY: working member count (net signed, occupied<=>
                   >0, algebra-independent) + sealed_occupied bit; the
                   birth/death/change/no-op guard
  B4  :322-350   FindOrAddGroup — dense-id alloc, identity-init working+
                   sealed, working_count.Add(0), sealed_occupied.Add(0)
  B5  :366-383   Fold(gid, sign, args...) — Touch once, read/fold/write-back
                   working, working_count += sign
  B6  :389-394   WorkingOccupied (count>0) / SealedOccupied (bit)
  B7  :401-411   Emit(gid,cfg...) working read / Old(gid) sealed read
  B8  :417-427   Seal — per touched: sealed:=SealFrom(working),
                   sealed_occupied:=(count>0), clear touched_flag
  B9  :441-444   Touched() — SortAndUnique, const result
  B10 :524-566   @recompute membership POOL idiom: Working is a POD handle
                   {Vec<Summary>*,Vec<int32_t>*} into store-owned pools;
                   kHasMembership requires-clause; InitMembership/MakeVec/
                   FreeMembership; MakeVec :560-566 (allocator.Allocate +
                   placement-new + pool.Add)
  B11 :581-595   store fields: keys/hashes/working/working_count/sealed/
                   sealed_occupied/touched/touched_flag/mem_*_pool/slots
  B12 :597-652   DebugValidate — seal coherence (touched empty, no flag),
                   occupancy coherence (sealed_occupied==(count>0), count>=0)

RUNTIME — Vec.h (include/drlojekyll/Runtime/Vec.h):
  C1  :28-29     Vec<T> static_asserts trivially_copyable + trivially_
                   destructible T — a per-instance nested DiffTable CANNOT
                   be Vec-stored by value (fact 17); POD handle mandatory

WITNESS — average_weight.datalog.h (scratchpad/witness):
  D1  :877-943   the commit-sweep TAIL ORDER (fact 15): per feeder table
                   Commit -> DebugValidateCounts -> CompactDead(+index
                   rebuild) -> statecell_N.Seal() -> DebugValidate; monotone
                   table_36.Seal() LAST (:942); return true :944

D0 CONSOLIDATED FACTS this artifact stands on (fleet-d0/consolidated.md §3):
  fact 5/6   DiffTable has no Seal/sealed; store-held row-id unsound under
               CompactDead (E-35)
  fact 15    commit-tail order (D1 above)
  fact 16    StateCell dense-id never-compact namespace — the D3 precedent
  fact 17    POD-handle-into-store-pool idiom mandatory (Vec<T> POD)
  fact 18    a demand relation needs a REAL structural input edge (E-32)

D1 MECHANISM AS JUDGED (design/judge-d1.md, verdict REVISE) — the witness
input D3 wires against:
  J-F1 (CRITICAL) the demand message is NOT a pure DataFlow object; to
    reach messsage_handler (keyed by ParsedMessage, Build.h:106) the demand
    pass MUST fabricate a real ParsedMessage at DataFlow-build time
    (mode-gated), reusing BuildIOProcedure/BuildPredicate.
  J-F2 (HIGH) that fabricated message leaks a PUBLIC entry point
    (Database.cpp:1426, gated only on kMessageHandler) — D4 must suppress
    it. D3's witness must NOT model a driver calling the demand message.
  The DataFlow-graph half of D1 (demand relation d_p^α + ⊥c-pivot guarded
    copy p'^α + CSE discharge + inertness rule) SURVIVED the judge cleanly
    — that is the demand EDGE D3's §4 witness consumes (E-32 satisfied).


======================================================================
§1. THE SEALED SIDE, REDESIGNED (the E-35-honoring answer)
======================================================================
old(iid) must answer "what rows did instance iid publish at BATCH START"
in terms the runtime actually has. The p2b sketch (§3.1:451-459) answers
"via the nested DiffTable's own kInI watermark ... the SAME id<sealed
compare a Table already does" — E-35 REAL-DEFECT: a DiffTable has NO
`sealed` field and NO `Seal()` (A4); `id<sealed` is a MONOTONE-Table
primitive (A3). And a store-held row-id watermark is unsound because
CompactDead RENUMBERS (A2/A6/A7). So the p2b answer is a dead end. Below
the three candidate substrates are priced; §1.4 DECIDES.

----------------------------------------------------------------------
§1.1 OPTION (A): old = the nested DiffTable's OWN per-row kInI flags
----------------------------------------------------------------------
The correct E-35 reading of the p2b intent. old(iid) is NOT a scalar
watermark — it is the SET { r in working[iid].rows : rows.InI(r) }, read
row-by-row via `DiffTable::InI` (A4:357-360, `flags[id]&kInI`). The nested
table advances its OWN kInI by its OWN Commit (A5): Commit sets
kInI:=(counts>0) per touched row. "old" is thus not a snapshot the STORE
holds at all — it is a MEMBERSHIP PREDICATE on the nested table's rows,
exactly as `emit`/`new` is the predicate { r : rows.InNew(r) }.

WHERE the nested Commit runs (fact 15 / D1 tail order): each instance's
nested DiffTable is a FEEDER of the outer published-rows DiffTable (§2),
so its Commit must run in the outer commit tail at the SAME relative
position StateCell.Seal occupies for the agg case — AFTER the feeder-table
Commit+CompactDead, BEFORE the outer publish table's own Commit. But there
is a subtlety Option (A) exposes and the p2b sketch missed: the nested
tables and the outer publish table are the SAME differential rows viewed
twice (§2 resolves this — they are NOT two tables under the sole-writer
redesign). Under (A)-as-literally-stated there would be N+1 differential
tables (N nested + 1 outer) whose Commits must be ordered — the seam
HOLE-B lives (§2).

WHO calls the nested Commit: NOT a store `Seal()`. The nested table is a
DiffTable; the emitted commit tail calls `.Commit(sink)` on it directly
(the D1:877 shape), per instance. The store's role shrinks to owning the
handle pool and the occupancy/touched bookkeeping — it does NOT re-seal a
watermark, because there is no watermark.

PER-INSTANCE CompactDead — ALLOWED, and old() stays coherent: because
kInI TRAVELS WITH THE ROW (A7 verbatim), CompactDead's moved() cb
(A6:594-598) drags `flags` (hence kInI) alongside `counts`. After a
renumber, `rows.InI(new_id)` reads the SAME kInI bit the row had at
old_id. So old() (a kInI membership predicate) is compaction-COHERENT for
free — this is precisely the E-35 design intent (A7) cashed in: the
frozen-I rides the row, so a per-instance nested compaction NEVER
invalidates old(). This is the property a store-held row-id watermark
would have BROKEN.
  COST: N+1 differential tables and their Commit ordering — the HOLE-B
  seam (§2). Space: one kInI bit per published row (free — it is already
  the DiffTable flag byte).
  TYPE SYMMETRY (the §1 requirement): emit(iid) = {r:InNew(r)} and
  old(iid) = {r:InI(r)} are BOTH set-valued predicates over the SAME row
  universe (the nested table's rows). SYMMETRIC by construction. ✓

----------------------------------------------------------------------
§1.2 OPTION (B): a sealed SNAPSHOT copy per instance
----------------------------------------------------------------------
Hold, per instance, a second frozen Vec<Row> copy of the published-row
set as of batch start; old(iid) reads that copy, emit(iid) reads the live
working relation.
  PRO: old() is a literal set, trivially coherent; no cross-table Commit
    ordering.
  CON: SPACE — a full duplicate of every instance's published rows, per
    epoch (the p2b §3.1 "space cost" flag). It also DUPLICATES the state
    the nested DiffTable's kInI ALREADY encodes for free (A7) — the kInI
    flag IS the batch-start snapshot; a copy is redundant with it. And it
    re-introduces a maintenance obligation (keep the copy in sync at Seal)
    the DiffTable machinery does not need. REJECTED as redundant with (A):
    it pays space to re-derive what kInI already gives.
  TYPE SYMMETRY: emit set-valued (live), old set-valued (copy) — symmetric
    but over DIFFERENT physical row universes (live table vs frozen copy),
    which is the redundancy.

----------------------------------------------------------------------
§1.3 OPTION (C): MONOTONE nested tables (no deletions inside an instance)
----------------------------------------------------------------------
Make each instance's nested published-row set a MONOTONE `Table` (A3), not
a DiffTable. Then old(iid) IS the p2b sketch's literal `id<sealed`
watermark — because a MONOTONE Table DOES have Seal/sealed (A3:277-282),
and its ids never renumber (no CompactDead path, B1 rationale). The p2b
"kInI watermark" answer becomes CORRECT — just on the wrong class. The
question §1 must answer: does the p2b §5 witness NEED differential nested
rows?

ANALYSIS against the p2b §5 / D3 §4 witness. Under Rederive lowering (the
stage-(b) default, p2b §2.2 C-1e), an instance's published set is REBUILT
per demand change: when Start=5's demand is retracted, the WHOLE instance
{7,9} is retracted (p2b §5.3 batch-2 DEATH). The published rows never
"gain and lose derivations" INDEPENDENTLY of the demand — a published row
exists iff (Start demanded) AND (edge(Start,Node) holds). Two sub-cases:

  (C-i) MONOTONE input, demand-keyed lifetime: if the summarized relation
    (edge) is add-only, a published row's ONLY death cause is demand
    retraction — which is a WHOLE-instance event (retract every row). A
    monotone nested Table + a FULL-RETRACT-ON-DEATH (publish -old for
    every batch-start row, then drop the instance) is SOUND: the instance
    is REBUILT, not incrementally maintained, so its nested table is
    write-once-per-life. old() = the monotone `id<sealed` watermark; on
    demand retraction the guard publishes -old for {r:id<sealed} and the
    instance is retired. NO differential nested rows needed. This is the
    p2b sketch's answer, VALID for (C-i).

  (C-ii) DIFFERENTIAL input (edge deletable): a published row can die
    because edge(Start,Node) is retracted while Start STAYS demanded — a
    PARTIAL instance change, not a whole-instance death. A monotone nested
    table cannot express "row 7 left but 9 stays" (no NetDeleted on a
    monotone Table, A3:269-274). (C-ii) NEEDS a differential nested table
    (Option A) OR a full-rebuild-on-any-input-change (recompute the whole
    instance whenever ANY demanded edge changes — the @recompute analog,
    which is EXACTLY Rederive: on any input delta touching a demanded key,
    retract the whole old instance and re-materialize). Under strict
    Rederive, (C-ii) collapses to (C-i): a differential input still drives
    a WHOLE-instance rebuild, so the nested table is still write-once-per-
    rebuild and can be MONOTONE with full-retract-on-rebuild.

----------------------------------------------------------------------
§1.4 DECISION — OPTION (C-i) MONOTONE NESTED + FULL-RETRACT-ON-REBUILD,
     for the Rederive stage-(b) default; Option (A) reserved for a future
     incremental-maintenance lowering
----------------------------------------------------------------------
DECIDE: for the RATIFIED stage-(b) Rederive lowering (p2b §1.5 acyclic-
frozen, §2.2 C-1e Rederive-first), an instance's published-row set is a
MONOTONE `Table<PubRow>` with a per-instance Seal watermark (A3). old(iid)
= { r in table[iid] : table[iid].InI(r) } via the monotone `id<sealed`
compare (A3:249-252) — the p2b sketch's watermark answer, now on the
CORRECT class (monotone Table HAS Seal/sealed; E-35 satisfied because we
are NOT on a DiffTable). emit(iid) = { r : table[iid].InNew(r) } (A3:255,
"final so far", all stored rows). A demand change (birth/death/rebuild)
publishes the net pair of the WHOLE instance against its watermark and
advances the watermark at INSTANCE_SEAL.

WHY (C-i) over (A) for stage (b):
  1. E-35 CLEANLY SATISFIED. The p2b watermark is real ON A MONOTONE
     TABLE. We do not need a DiffTable's absent Seal; we use the monotone
     Table's present one (A3:277-278). No new primitive.
  2. IT MATCHES THE Rederive SEMANTICS. Rederive rebuilds the instance
     whole-hog on demand change (p2b §5.3). A rebuilt set is write-once-
     per-life — a monotone table with full-retract-on-rebuild expresses
     exactly that. Differential nested rows (A) would be machinery the
     Rederive lowering never exercises (it never does partial maintenance).
  3. IT COLLAPSES HOLE-B. A monotone nested table has NO independent
     per-row retraction (no NetDeleted, A3:269-274) — so the "row
     independently retracted inside the nested relation" arm of the p2b
     §7.3 double-count seam CANNOT ARISE (§2). The seam is a DiffTable-
     nested-only hazard; a monotone nested table structurally forecloses
     it. This is the decisive win.
  4. FACT-16 PRECEDENT. A monotone table "never compacts — no deaths, and
     sealed is an id-order watermark" (CLAUDE.md data-structures epoch).
     The per-instance nested monotone table inherits that verbatim; its
     rows are demand-lifetime, retracted only whole-instance at the OUTER
     publish table (§2), never inside the nested table.

WHAT (A) IS RESERVED FOR: the incremental-maintenance instance lowering
(p2b §2.2 C-1e, the @invertible analog, DEFERRED). When an instance is
maintained incrementally (fold input deltas into a standing instance
rather than rebuilding), a published row CAN die independently (C-ii) and
the nested table MUST be differential (Option A), and HOLE-B's seam
re-arises and MUST be discharged by §2's sole-writer argument on the
nested DiffTable's kInI. D3 SPECIFIES (A)'s coherence (§1.1: kInI rides
the row, per-instance CompactDead is old()-coherent) so the future lowering
inherits a designed substrate — but stage (b) SHIPS (C-i).

THE EMIT/OLD TYPE SYMMETRY, honored under (C-i): both emit(iid) and
old(iid) are SET-VALUED membership predicates over the SAME row universe
(instance iid's monotone nested Table rows): emit = {r:InNew(r)} (all
stored, A3:255), old = {r:InI(r)} (id<sealed, A3:249). Same table, same
row ids, two frozen-vs-current predicates — the StateCell two-word split
(B2) generalized from a scalar working/sealed to a set-valued current/
frozen over ONE monotone table. This is TIGHTER than the p2b two-word
sketch: the "working" and "sealed" are not two blobs, they are one
monotone table read through two membership predicates keyed on the
watermark, exactly how a monotone Table already answers frozen-vs-current
at a delta-join read position (A3:220-223).


======================================================================
§2. THE TWO-TABLE DOUBLE-COUNT SEAM, SPECIFIED BEFORE EMISSION
    (the p2b §7.3 probe answered head-on)
======================================================================
p2b §7.3's highest-value adversarial probe: the instance guard nets a SET
(not a scalar); can ONE published row be double-counted when (a) the
instance guard drops it (demand retracted) AND (b) it is independently
retracted inside the nested relation the SAME batch? p2b §3.3 asserts a
seed-before-drain (E3 phantom-drop) argument but "does not fully discharge"
it. D3 RE-DESIGNS so the seam CANNOT ARISE, then states the invariant and
the validator.

----------------------------------------------------------------------
§2.1 THE SEAM, RESTATED PRECISELY (why it is a real hazard for a
     DiffTable-nested design)
----------------------------------------------------------------------
The p2b sketch (§3.1) had TWO differential row universes:
  (1) each instance's NESTED DiffTable (working[iid].rows), with its own
      counters/kInI/Commit; and
  (2) the OUTER published-rows DiffTable (neighborhood_pub), with ITS own
      counters/kInI/Commit, into which publish_touched folds the net pairs.
A published row r=(Start,Node) then has TWO counter homes: the nested
table's row for Node, and the outer table's row for (Start,Node). If
r dies both ways in one batch — the nested table retracts Node (input
change) AND the instance guard emits -old for r (demand retracted) — the
OUTER table receives the -1 TWICE (once from the guard's death arm, once
from the nested-death propagating to the outer fold), unless something
nets them. That is the p2b §7.3 seam: two writers into the outer table's
counter for one row.

----------------------------------------------------------------------
§2.2 THE REDESIGN — ONE WRITER: the outer publish table's counters fold
     ONLY from publish_touched's net pairs
----------------------------------------------------------------------
D3 forecloses the seam by SOLE-WRITER, not by ordering. TWO structural
commitments:

  SW-1 (from §1.4): the nested per-instance table is MONOTONE (C-i). A
    monotone Table has NO per-row retraction (no NetDeleted, A3:269-274)
    and NO independent counter that could propagate a -1. A published
    row's ONLY death is a WHOLE-INSTANCE event (demand retracted → rebuild
    → the whole batch-start set is published -old). So arm (b) of the seam
    — "independently retracted inside the nested relation" — is
    STRUCTURALLY ABSENT: a monotone nested table cannot independently
    retract a row. There is exactly ONE death cause (whole-instance), read
    by exactly ONE reader (the publish_touched guard).

  SW-2 (the sole-writer invariant): the OUTER published-rows DiffTable's
    counters are folded from EXACTLY ONE source — publish_touched's net
    pairs (§4 band (b)). The nested monotone table NEVER folds into the
    outer table's counters directly; it is a READ-ONLY membership oracle
    (emit/old predicates, §1.4) the guard consults to COMPUTE the net
    pairs. The outer table's counter for (Start,Node) moves iff
    publish_touched emits a pair for it. ONE writer, by construction.

Under SW-1 + SW-2 the double-count is IMPOSSIBLE:
  - arm (b) is structurally absent (SW-1: monotone nested → no independent
    retraction);
  - the outer table has ONE writer (SW-2: publish_touched only), so even
    across birth/death/change transitions each (Start,Node) row receives
    at most one +1 and at most one -1 per batch from the SINGLE guard —
    and the guard's occupancy-generalized birth/death/change logic (§3,
    generalizing B3) emits AT MOST ONE net pair per (instance,row), exactly
    as the StateCell guard emits at most one net pair per (group,scalar).

----------------------------------------------------------------------
§2.3 THE GUARD-INTERNAL ONE-NET-PAIR ARGUMENT (the relation-delta
     generalization of B3, discharged not asserted)
----------------------------------------------------------------------
p2b §3.3 GENERALIZED StateCell's scalar guard (new!=old) to a relation
delta (symmetric difference of published sets) but left the "at most one
pair per row" as an assertion. D3 discharges it via the monotone-table
membership predicates (§1.4). Per touched instance iid, per candidate row
r, the guard reads:
    old_r := table[iid].InI(r)     (id<sealed; A3:249 — frozen)
    new_r := table[iid].InNew(r)   (all stored; A3:255 — current)
and applies the occupancy-generalized ONE-NET-PAIR guard (§3):
    (old_r, new_r) = (F,T) → emit +1 (Start,r)   [born row]
    (old_r, new_r) = (T,F) → emit -1 (Start,r)    [dropped row]
    (old_r, new_r) = (T,T) → emit nothing          [unchanged]
    (old_r, new_r) = (F,F) → emit nothing          [never present]
Because old_r and new_r are BOTH read from the SAME monotone table at the
SAME instant (post-fold, pre-Seal), each row falls in EXACTLY ONE of the
four cells → AT MOST ONE net pair per (iid, r). No row can be both born
and dropped in one guard pass (the cells partition). This is the scalar
B3 guard, lifted row-wise, and it is EXACT because a monotone table's
InI/InNew are two reads of one id against one watermark — they cannot
disagree with themselves. The p2b assertion is now a partition argument.

----------------------------------------------------------------------
§2.4 SEED-BEFORE-DRAIN STILL HOLDS FOR THE OUTER TABLE (E3, unchanged)
----------------------------------------------------------------------
The outer publish table IS differential (its rows gain/lose presence
across batches as demands come and go). publish_touched's net pairs are
folded into the outer table's counters + del/add queues via the EXACT
GROUP_UPDATE emit_touched vocab (counter±/kInIReadFrozen/kVecAppend, p2b
§5.2 band b). The E3 seed-before-drain invariant (the phantom-drop of the
claim gates, Table.h TryClaimDel/TryClaimAdd A4) applies to the OUTER
table UNCHANGED — publish_touched seeds the outer queues BEFORE the outer
table's own claim drain, exactly as GROUP_UPDATE.emit seeds the agg queues
before the agg drain (p2b §5.4 band 0, E3). The seam that p2b §7.3 feared
was BETWEEN the nested and outer tables; SW-1 removes the nested table's
counters entirely, so the only counter machinery is the outer table's, and
its seed-before-drain is the ALREADY-PROVEN GROUP_UPDATE edge. No new
ordering obligation.

----------------------------------------------------------------------
§2.5 THE INVARIANT + ITS VALIDATOR
----------------------------------------------------------------------
INVARIANT V-INSTANCE-SOLE-WRITER: the outer published-rows DiffTable of a
subgraph view has EXACTLY ONE deriver — the SUBGRAPH_INSTANTIATE op's
publish_touched band. No nested per-instance table folds into it; no other
view inserts into it. (This is the V-AGG-SOLE analog, p2b §6.1 / DR.cpp:
663-665 — the agg table's sole deriver is its GROUP_UPDATE.)

VALIDATOR (structural, always-on, survives NDEBUG — the DR-IR house
convention): at BuildSubgraphOps time, assert the pub table's member-view
list holds exactly the one SUBGRAPH_INSTANTIATE (mirror V-AGG-SOLE's
member-view check). AND a V-PRED-XCHECK site (§5): the emitted CF tree for
publish_touched must be the ONLY writer of the pub table's counter± — a
reintroduced second writer (e.g. a future incremental lowering wiring the
nested DiffTable into the outer counters, the (A) path) aborts on compile.

RESIDUAL, honestly flagged: this invariant HOLDS for the stage-(b)
Rederive lowering because SW-1 makes the nested table monotone. The FUTURE
incremental-maintenance lowering (Option A, §1.4) puts a DiffTable back in
the nested position and RE-OPENS the seam. D3 does not discharge that case
— it RESERVES it: the incremental lowering must re-establish SW-2 by
folding the nested DiffTable's NetAdded/NetDeleted frontiers (A4:405-433)
through the SAME publish_touched guard (never directly into the outer
counters), so the outer table keeps its single writer. That is the
follow-on's obligation, pre-registered here so it is not re-litigated.


======================================================================
§3. THE STORE, SKETCHED TO THE FIELD (p2b §3.2 revised)
======================================================================
The §1.4 decision (monotone nested + full-retract-on-rebuild) SIMPLIFIES
the p2b §3.2 sketch: the per-instance payload is a store-owned MONOTONE
`Table<PubRow>` (not a DiffTable), and old() is that table's OWN sealed
watermark (not a store-held uint64 — the p2b sketch's `Sealed=uint64_t`
field at :508 was the E-35 defect and is DELETED). Every field delta below
is tagged with its driving erratum/fact.

----------------------------------------------------------------------
§3.1 THE CLASS SKETCH (new Runtime header, peer of StateCell.h)
----------------------------------------------------------------------
New file include/drlojekyll/Runtime/Instance.h (peer of StateCell.h and
Table.h; dependency-free hyde::rt; HYDE_RT_BENCH_COUNT seams). Keying/
occupancy/touched machinery is LITERALLY StateCellStore's (B4-B12); only
the per-instance payload type and the Fold/Emit/Old bodies differ.

  namespace hyde::rt {

  // Standing per-instance state for ONE subgraph view. Keyed
  // (context ++ config) -> a DENSE instance id; per-id a store-owned
  // MONOTONE nested Table of published rows. NON-ALIASING (mirror
  // StateCell.h:10-24, B1): instance dense ids are a SEPARATE id space
  // from the demand table's row ids, the OUTER published DiffTable's row
  // ids, and every nested Table's own row ids. A DiffTable compaction
  // (CompactDead) never touches this store; the nested monotone tables
  // never compact (fact 16: no deaths inside — a published row dies only
  // as a WHOLE-INSTANCE rebuild, at the outer table, §2). An instance id,
  // once allocated, is retained for the program's life (append-only dense
  // namespace, the "monotone tables never compact" rationale, B1).
  //
  // THE TWO-WORD CELL generalized to a SET, over ONE monotone table
  // (§1.4, generalizing StateCell.h:26-36, B2): `working` = the current
  // published-row set = { r : nested.InNew(r) } (all stored, A3:255);
  // `sealed` = the batch-start set = { r : nested.InI(r) } (id<sealed,
  // A3:249). NOT two blobs — ONE monotone Table read through two
  // frozen-vs-current membership predicates keyed on its own watermark.
  // (DELTA vs p2b §3.2: the p2b `Sealed=uint64_t sealed[]` field is
  // DELETED — E-35: there is no store-held watermark; the watermark lives
  // INSIDE the nested monotone Table, advanced by nested.Seal(). old() is
  // a predicate on the nested table, not a store field read.)
  //
  // OCCUPANCY (mirror StateCell.h:38-54, B3): an instance is EMPTY or
  // OCCUPIED. working_count / sealed_occupied drive the occupancy-
  // generalized publish guard (§3.2): birth (+new only), death (-old
  // only), rebuild/change (the row-delta net pairs, §2.3), no-op.
  template <typename Key, typename PubRow>
  class InstanceStore {
   public:
    // The nested published-row set: a store-owned MONOTONE Table
    // referenced by a POD handle (fact 17 / C1; mirror Recompute::Working,
    // StateCell.h:167-170, B10) so Vec<Working> stays trivially-copyable.
    // (DELTA vs p2b: the payload is a MONOTONE Table, not a DiffTable —
    // §1.4/SW-1, foreclosing HOLE-B.)
    struct Working {
      Table<PubRow> *rows{nullptr};   // store-owned nested MONOTONE table
    };
    static_assert(std::is_trivially_copyable_v<Working>,   // C1
                  "instance Working must be a POD handle (Vec<Working>)");
    // (DELTA vs p2b: NO `using Sealed = uint64_t;` — E-35, the watermark
    // is inside Working.rows, not a store field.)

    explicit InstanceStore(Allocator a)
        : allocator(a), keys(a), hashes(a), working(a), working_count(a),
          sealed_occupied(a), touched(a), touched_flag(a),
          table_pool(a) {}
    InstanceStore(const InstanceStore &) = delete;
    InstanceStore &operator=(const InstanceStore &) = delete;
    ~InstanceStore(void);   // ~Table each pooled table, then free the slot

    uint32_t NumInstances(void) const noexcept { return keys.Size(); }

    // Dense instance id for `key`, allocating an EMPTY nested monotone
    // table on first touch. Mirror StateCellStore::FindOrAddGroup (B4).
    uint32_t FindOrAddInstance(const Key &key);   // wires working.rows via
                                                  // MakeTable(table_pool)
    uint32_t FindInstance(const Key &key) const noexcept;  // kNoInstance

    // Rederive materialize: (re)build the instance's nested set. Under the
    // stage-(b) Rederive lowering the caller CLEARS-and-rebuilds on any
    // demand-key change (whole-instance, §1.4): TryAdd each rederived row
    // into working[iid].rows (monotone dedup, A3:235). Records the
    // instance in `touched` once (mirror StateCellStore::Fold, B5). NO
    // instance-level presence crossing (p2b C-1c) — the crossing is on the
    // OUTER pub DiffTable, at publish (§2/SW-2).
    template <typename... Cols>
    void Fold(uint32_t iid, int32_t sign, Cols &&...row);

    // publish reads (§1.4/§2.3): both set-valued predicates over the SAME
    // nested monotone table. emit(iid) enumerates {r:InNew(r)} (current);
    // old(iid) enumerates {r:InI(r)} (batch-start, id<sealed). SYMMETRIC.
    const Table<PubRow> &Emit(uint32_t iid) const { return *working[iid].rows; }
    const Table<PubRow> &Old(uint32_t iid)  const { return *working[iid].rows; }
    // ^ Emit and Old return the SAME table (the row universe); the CALLER
    //   distinguishes current vs frozen by InNew(r) vs InI(r) on it. This
    //   is the type symmetry made literal: one table, two predicates.
    //   (DELTA vs p2b: p2b's Old returned a scalar `Sealed` watermark;
    //   here Old returns the table and the watermark is read via InI —
    //   E-35.)

    bool WorkingOccupied(uint32_t iid) const noexcept {  // B6 mirror
      return 0 < working_count[iid];
    }
    bool SealedOccupied(uint32_t iid) const noexcept {   // B6 mirror
      return 0u != sealed_occupied[iid];
    }

    // End-of-epoch: for each touched instance, advance the nested table's
    // OWN watermark (nested.Seal(), A3:277 — sealed:=NumRows()); advance
    // sealed_occupied; clear touched. Mirror StateCellStore::Seal (B8).
    // (DELTA vs p2b: no store-held watermark to set — Seal delegates to
    // the nested monotone Table's Seal, E-35.)
    void Seal(void);

    const Vec<uint32_t> &Touched(void);   // sort-unique (B9)
    const Key &KeyAt(uint32_t iid) const noexcept { return keys[iid]; }
    void DebugValidate(void) const;       // §3.3

   private:
    void Touch(uint32_t iid);             // append-once (B mirror)
    template <typename T> T *MakeTable(Vec<T *> &pool);  // B10 MakeVec twin
    Allocator allocator;
    Vec<Key> keys;                        // instance id -> key       (B11)
    Vec<uint64_t> hashes;                 // cached key hashes         (B11)
    Vec<Working> working;                 // POD handles to nested tables
    Vec<int32_t> working_count;           // net live-row count (occupancy)
    Vec<uint8_t> sealed_occupied;         // batch-start occupancy bit
    Vec<uint32_t> touched;                // instances (re)built this epoch
    Vec<uint8_t> touched_flag;            // per-instance append-once bit
    Vec<Table<PubRow> *> table_pool;      // store-owned nested MONOTONE
                                          // tables (replaces p2b's
                                          // relation_pool + the deleted
                                          // Vec<Sealed> sealed field)
    uint32_t *slots{nullptr};             // open-addressing key -> iid
    size_t slot_capacity{0u};
  };

  }  // namespace hyde::rt

FIELD-DELTA TABLE vs the p2b §3.2 sketch (each tagged):
  - Working.rows : DiffTable* -> Table<PubRow>*        [§1.4/SW-1; HOLE-B]
  - `using Sealed = uint64_t` field DELETED             [E-35; fact 5/6]
  - `Vec<Sealed> sealed` field DELETED                  [E-35]
  - `relation_pool` -> `table_pool` (monotone)          [§1.4]
  - Old(iid): returns Sealed scalar -> returns the table [E-35; §1.4]
  - Seal(): sets store watermark -> calls nested.Seal()  [E-35; fact 15]
  - kept IDENTICAL (fact 16 precedent, B11): keys, hashes, working,
    working_count, sealed_occupied, touched, touched_flag, slots,
    slot_capacity — the StateCellStore field set, verbatim.

----------------------------------------------------------------------
§3.2 THE OCCUPANCY / BIRTH-DEATH GUARD GENERALIZATION
----------------------------------------------------------------------
Generalizes B3 one level up: the StateCell guard nets ONE (group, scalar)
pair; the instance guard nets the ROW DELTAS between the batch-start and
current published sets, read via the nested monotone table's InI/InNew
(§2.3). The four occupancy transitions drive which side emits:
  - birth (empty->occupied): every current row is +new (InNew && !InI);
    no phantom -old. Instance materialized this batch.
  - death (occupied->empty): every batch-start row is -old (InI && !InNew
    is IMPOSSIBLE on a monotone table since InNew is always true for stored
    rows — so death is signaled by the OUTER retract: the whole instance's
    InI rows are published -old and the instance is retired at the outer
    table; the nested monotone table is dropped/reset on rebuild). See
    §3.2.1 for the Rederive rebuild mechanics.
  - change/rebuild (occupied, set differs): the symmetric difference —
    -old for InI-and-not-in-new-set, +new for in-new-set-and-not-InI. Under
    Rederive this is a CLEAR-and-refill of the nested table (§3.2.1).
  - no-op (unchanged): publish nothing (OQ3 annihilation).
The guard STRUCTURE (occupancy transitions decide which side emits) is
identical to B3; the payload is a row set instead of a scalar. The
partition argument (§2.3) makes it EXACTLY one net pair per (iid, row).

§3.2.1 THE Rederive REBUILD MECHANIC (the monotone-table subtlety). A
monotone Table cannot delete rows, but Rederive must REPLACE an instance's
set on demand-key change. Resolution: on a demand-key change for iid, the
guard FIRST reads old() = {r:InI(r)} from the CURRENT nested table (the
frozen batch-start set), publishes those as -old candidates, THEN the
store RESETS working[iid].rows to a FRESH empty nested table (drop the old
one back to table_pool / re-init) and Fold re-materializes the new set,
whose rows publish as +new candidates. The -old/+new candidates net at the
OUTER DiffTable (§2/SW-2): a row present in BOTH old and new annihilates
(OQ3, the outer table's own claim gates), so an UNCHANGED row across a
rebuild publishes nothing net. This keeps the monotone-nested invariant
(each nested table is write-once-per-life) AND the one-net-pair-per-row
result (the outer table nets the rebuild's old/new). THE OLD NESTED TABLE
IS READ FOR old() BEFORE THE RESET — the reset is ordered after the guard's
old-read, an intra-op ordering (E5 analog, INSTANCE_SEAL after publish).

----------------------------------------------------------------------
§3.3 NON-ALIASING (three-way) + DebugValidate
----------------------------------------------------------------------
NON-ALIASING INVARIANT (three-way, sharper than StateCell's two-way, B1):
the instance-id space is disjoint from (a) the demand table's row ids,
(b) the OUTER published DiffTable's row ids, and (c) every nested monotone
table's own row ids. Asserted in debug (mirror StateCell.h:10-24 + the
V-AGG-SOLE non-aliasing check DR.cpp:663-665): an instance id never
appears as a table.Find argument; one nested table's row ids never leak
into another's or into the outer table.

DebugValidate (mirror StateCellStore::DebugValidate, B12):
  - SEAL COHERENCE: after Seal(), touched empty, no touched_flag survives
    (B12:603-605 mirror).
  - OCCUPANCY COHERENCE: sealed_occupied[iid] == (working_count[iid] > 0)
    at batch boundary; working_count >= 0 at boundary (may dip mid-epoch,
    B12:609-610 mirror).
  - NESTED-TABLE WATERMARK COHERENCE (the E-35-specific check, NEW vs
    B12): for each occupied instance, working[iid].rows->NumRows() >=
    (the row count implied by sealed_occupied) and the nested table's
    sealed watermark <= NumRows() (a monotone Seal only advances; A3:277).
    This is the check that would have caught a store-held watermark drift
    — it lives on the nested table now (E-35).
  - NON-ALIASING set-intersection check (three-way, above).


======================================================================
§4. THE DEMAND-WIRED WITNESS
======================================================================
ONE subgraph case wired through the D1 mechanism AS JUDGED, with its REAL
structural demand edge (E-32) named against a compiled IR witness. I
generated the hand-authored demand-guarded graph (the D1 §5 checkpoint-3
spike) to GROUND every edge name; the artifact is
`scratchpad/design-work/hand_demand.{dr,ir}` (compiles clean this session,
77-line IR). Every op/edge below is anchored to that IR.

----------------------------------------------------------------------
§4.1 THE .dr (the D1 §5 primary witness, single-adornment bf)
----------------------------------------------------------------------
hand_demand.dr (compiled this session):

  #message add_edge(i32 From, i32 To).
  #message ask(i32 Start).                  ; the demand source
  #local edge(i32 From, i32 To).
  #local demand_neighborhood(i32 Start).    ; the demand relation d_p^bf
  #query neighborhood(bound i32 Start, free i32 Node).

  edge(From, To) : add_edge(From, To).
  demand_neighborhood(Start) : ask(Start).
  neighborhood(Start, Node) : demand_neighborhood(Start), edge(Start, Node).

This IS the graph D1's Option-D-as-judged (judge-d1 F1) FABRICATES at
DataFlow-build time when the user writes only the bound `#query
neighborhood(bound Start, free Node)` and the demand pass synthesizes
`demand_neighborhood` + the guarded copy. Hand-authoring it (checkpoint-3)
lets D3 name the target IR against real compiler output.

----------------------------------------------------------------------
§4.2 THE REAL STRUCTURAL DEMAND EDGE (E-32) — named against the IR
----------------------------------------------------------------------
The compiled IR (hand_demand.ir) shows the demand edge is a REAL JOIN, not
a group_ids artifact (E-32 / fact 18 satisfied):
  - %table:8[i32] (Start)  = demand_neighborhood; fed by
    `^receive:ask/1` (IR:58-62, the demand SOURCE; the injector-analog for
    the synthesized case).
  - %table:11[i32,i32]     = edge (Start,Node); fed by `^receive:add_edge`.
  - %table:4[i32,i32]      = neighborhood (the QUERY-PUBLISHED rows), with
    the bound-query read index %index:42[i32,_] on Start (IR:9-14).
  - THE DEMAND EDGE: `^flow:43` (IR:64-76) is the JOIN; pivots {Start}
    drive `select from %table:8 (demand) JOIN select from %table:11 (edge)`
    on the Start pivot (IR:68-72, `if-compare {Start,Start}={Start,Start}`)
    then `update-count +nonrecursive {Start,Node} in %table:4` (IR:74).
    THIS JOIN on the Start column IS the c-pivot demand edge (p2b E-32,
    D1 §1.1 A16): |x|=1 real column edge from demand_neighborhood into the
    neighborhood-producing view. NO group_ids involved — a structural
    equi-join. This is the exact edge D1 §3(b) proves CSE-distinct.

So the demand edge D3's SUBGRAPH_INSTANTIATE consumes is: the net-additions
frontier of %table:8 (demand_neighborhood), joined on Start against edge,
producing the instance's published rows. In the D1-independent fallback
(§4.6) this stays a plain JOIN into a plain DiffTable (%table:4). In the
KEYED-INSTANCE lowering (§4.3) the JOIN + %table:4 are REPLACED by a
SUBGRAPH_INSTANTIATE op keyed on Start, whose InstanceStore holds the
per-Start nested published set.

----------------------------------------------------------------------
§4.3 THE TARGET DR-IR OPS (v3-spec §2.1 style; STARTS FROM p2b §5.2,
     AMENDED per §1-§3; every changed line tagged [D3-d])
----------------------------------------------------------------------
ONE SUBGRAPH_INSTANTIATE + ONE INSTANCE_SEAL per subgraph view (mirrors
BuildGroupUpdateOps one level up, p2b §6.1). Amendments vs p2b §5.2 tagged.

OP  SUBGRAPH_INSTANTIATE(subgraph neighborhood, instance INST,
        context={Start}, config={}, published={Node},
        mode=Rederive, class=NonRecursive, provenance=subgraph)
  ctx = kSeed;  instance_store_id = 0;  inst_table = neighborhood(%table:4)

  BAND (a) demand_in — over demand_neighborhood's net frontiers
    (%table:8's NetAdditions; the IR:69 pivot loop is this drain). Per SIGN
    arm (demand is differential; ask can retract):
      minus arm: vector:drain(demand.NetRemovals)  [kVecDrain,kNetRemoval]
              instance:demand(INST)  read the retracted Start key
                 [kDemandRead — FROZEN, p2b §1.5 acyclic]
              [D3-d] instance:rebuild(INST,-1): read old()={r:InI(r)} from
                 the CURRENT nested monotone table, publish -old candidates,
                 THEN reset working[iid].rows (§3.2.1 rebuild mechanic).
                 On a MONOTONE nested table this is a WHOLE-INSTANCE retract,
                 NOT a per-row DiffTable fold; the p2b "RW working" becomes
                 "read-old-then-reset-table".
      plus arm:  vector:drain(demand.NetAdditions) [kVecDrain,kNetAddition]
              instance:demand(INST)  read the new Start key  [kDemandRead]
              [D3-d] instance:rebuild(INST,+1): materialize (Rederive:
                 rerun edge(Start,_) — the IR:71 select on %table:11 with
                 %index:27 on Start) into a FRESH nested monotone table;
                 TryAdd each Node (A3:235, monotone dedup).
    Demand membership predicates consumed: kNetAdded/kNetDeleted via the
    drained frontier vecs (IR:69 loop). NO table:counter± here (p2b C-1c).

  BAND (b) publish_touched — over INST.Touched() (sort-unique, B9). Per
    touched instance iid, per candidate row r (§2.3 partition):
      new_r := table[iid].InNew(r)  [kInstanceEmit, R working — A3:255];
      old_r := table[iid].InI(r)    [kInstanceOld,  R sealed — A3:249];
      [D3-d] BOTH reads on the SAME monotone table (§1.4 type symmetry),
        NOT a store scalar (E-35). Occupancy-generalized guard (§3.2):
        (T,F)-> drop: counter-(neighborhood %table:4,NonRec) over (Start++r);
           flags:read(%table:4,kInI) [kInIReadFrozen];
           vector:append(%table:4.delQ) [kVecAppend] <- outer UPDATECOUNT
        (F,T)-> add:  counter+(neighborhood %table:4,NonRec) over (Start++r);
           flags:read(%table:4,kInI) [kInIReadFrozen];
           vector:append(%table:4.addQ) [kVecAppend] <- outer UPDATECOUNT
        (T,T)/(F,F) -> nothing (§2.3 partition).
    The counter±/kInIReadFrozen/kVecAppend triples are the EXACT
    GROUP_UPDATE emit_touched vocab (p2b §5.2, DR.cpp:720-738).
    [D3-d SOLE-WRITER, §2/SW-2]: these counter± are the ONLY writers of
    %table:4's counters. The nested monotone tables NEVER fold into
    %table:4 — they are read-only membership oracles. HOLE-B foreclosed.

OP  INSTANCE_SEAL(INST)   (the STATE_SEAL analog, p2b §5.2)
  ctx = kSeed;  instance_store_id = 0;  inst_table = neighborhood(%table:4)
  [D3-d E-35]: effect instance:seal(INST) [kInstanceSeal — global:rmw]
    delegates to, per touched instance, working[iid].rows->Seal() (advance
    the NESTED MONOTONE table's OWN watermark, A3:277) + advance
    sealed_occupied. NOT a store-held watermark write (the p2b `sealed :=
    working watermark` scalar is GONE; the watermark is inside the nested
    table). Trailing commit band.

DEPENDENCE EDGES (derived; the p2b E1-E6 amended):
  E1  demand-frontier RAW: demand's FRONTIER_FILTER writes its net
      frontiers; INSTANTIATE.in DRAINS them (IR:69). UNCHANGED from p2b.
  E2  rebuild-before-emit RAW: INSTANTIATE.in rebuild WRITES the nested
      table; INSTANTIATE.publish READS it (InNew/InI). [D3-d]: the read is
      a membership predicate on the monotone table, and the §3.2.1 reset is
      ordered AFTER old() is read (intra-op).
  E3  publish-before-outer-drain (seed-before-drain): publish_touched's
      counter± APPEND to %table:4's del/add queues; %table:4's CLAIM_DRAIN
      drains them; the pair MUST precede the drain (E3 phantom-drop).
      UNCHANGED — the OUTER table's already-proven edge (§2.4). [D3-d]: it
      is the ONLY seed-before-drain edge now (no nested-table drain, SW-1).
  E4  outer-drain-after-publish: %table:4's acyclic band downstream of
      publish_touched. DERIVED from E3. UNCHANGED.
  E5  seal-after-publish: INSTANCE_SEAL advances the nested watermark AFTER
      publish read InI/InNew; trailing commit band. [D3-d]: seals the
      MONOTONE nested table (A3:277), ordered LAST (like table_36.Seal,
      D1:942).
  E6  demand-stratum-final: no old()/emit() read before demand frontier
      final. DERIVED from E1. UNCHANGED.

----------------------------------------------------------------------
§4.4 THE InstanceStore CELL LAYOUT FOR THIS CASE (p2b §5.3, amended)
----------------------------------------------------------------------
  Key       = {Start:i32}                (context; config empty)
  PubRow    = {Node:i32}                  (the nested published row)
  working[iid].rows -> a MONOTONE Table<PubRow> of Node values for Start
                        [D3-d: monotone Table, not DiffTable, §1.4/SW-1]
  working_count[iid]   = net live Node rows this epoch (occupancy)
  sealed_occupied[iid] = was this Start demanded at batch start
  [D3-d E-35: NO sealed[iid] watermark field — old() reads
   working[iid].rows->InI(r) via the nested table's own watermark]

  For demand ask(5) in batch 1 (edges 5->7, 5->9 present):
    FindOrAddInstance({5}) -> iid 0; working[0].rows = empty monotone Table.
    Rederive rebuild(+1): rerun edge(5,_) (IR:71 select) -> TryAdd 7, TryAdd
      9 into working[0].rows. working_count[0]=2 (BIRTH). Nested watermark
      still 0 (not sealed) so InI(7)=InI(9)=false (id<0), InNew=true.
    publish_touched: for r in {7,9}: (old_r,new_r)=(F,T)=birth => +new:
      counter+(neighborhood {5,7}), counter+(neighborhood {5,9}) into
      %table:4 [outer crossing, published via its Commit sink].
    INSTANCE_SEAL: working[0].rows->Seal() (watermark:=2); sealed_occupied
      [0]=1.
  Batch 2, ask(5) retracted (demand_neighborhood loses {5}):
    minus arm: read old()={7,9} (InI: id<2, both true), publish -old
      candidates; reset working[0].rows (§3.2.1). working_count[0]->0
      (DEATH). publish_touched: death => -old only: counter-(neighborhood
      {5,7}), counter-(neighborhood {5,9}) into %table:4. SEAL:
      sealed_occupied[0]=0.

  NON-ALIASING (§3.3): iid 0 disjoint from %table:4's row ids and from
    working[0].rows's own Node row ids (three-way).

----------------------------------------------------------------------
§4.5 STRATUM / BAND PLACEMENT + THE ACYCLIC-FROZEN FENCE (p2b §5.4)
----------------------------------------------------------------------
STRATUM: SUBGRAPH_INSTANTIATE sits at its subgraph view's Stratify stratum,
STRICTLY ABOVE its demand input (p2b §1.5 acyclic fence). For the witness:
stratum 0 = {edge from add_edge, demand_neighborhood from ask}; stratum 1
= {SUBGRAPH_INSTANTIATE + neighborhood(%table:4) drain/filter}. Lift =
max(view stratum, ready_after(demand_table)); the pub table drain lifts to
the instance stratum (E4). ONE new clause in DeriveDRStrata, mirroring
GROUP_UPDATE. op_stratum: a new case keying a subgraph_instance_stratum map
— NO silent default-0 (§5).

BAND: band 0 (with seeds/products/GROUP_UPDATE) — publish_touched's
counter± seed %table:4's queues BEFORE %table:4's own claim drain (band 1)
and frontier filter (band 2) at the SAME stratum (E3 seed-before-drain,
rides the existing V-SEED-DRAIN edge). INSTANCE_SEAL: lead 2, trailing band
past STATE_SEAL, mirroring the commit-tail order (D1:942 — the monotone
nested Seal LAST, like table_36.Seal).

THE ACYCLIC-FROZEN FENCE (ViewSelfReachable): at BuildSubgraphOps time,
assert the demand-producing view (demand_neighborhood) is NOT self-
reachable THROUGH the instance (reuse ViewSelfReachable, Build.cpp:200 —
NOT InductionGroupId, the F22 lesson). For the witness: demand_neighborhood
depends only on `ask` (a message), and neighborhood's published rows do NOT
feed back into demand_neighborhood (no published row re-demands) =>
ViewSelfReachable(demand_neighborhood, through INST) = false => ACYCLIC,
lowers. An on-cycle demand is rejected with a clean 4-mode diagnostic (the
agg_in_scc_1 mold) — the SCC-demand regime (p2b §1.3) stays deferred.

----------------------------------------------------------------------
§4.6 THE D1-DEPENDENT SURFACE vs THE D1-INDEPENDENT FALLBACK — which the
     owner picks under decisions (a)/(d)
----------------------------------------------------------------------
The D1 judge returned REVISE (judge-d1 F1/F2): D1's Option-D collapses into
a FABRICATED-ParsedMessage-at-DataFlow-time mechanism plus a codegen
entry-point suppression. This CONSTRAINS D3's wiring. D3 designs the
witness under BOTH surfaces (p2b §4's candidates), so the owner can pick:

  SURFACE-1 (D1-DEPENDENT, candidate (i)-via-D1): the user writes ONLY the
    bound `#query neighborhood(bound Start, free Node)`; D1's demand pass
    SYNTHESIZES demand_neighborhood + the guarded copy (fabricating a
    ParsedMessage for the demand, judge-d1 F1) + suppresses its public
    entry (F2); D3's SUBGRAPH_INSTANTIATE then lowers the guarded copy as a
    keyed instance. PRO: no new .dr syntax; the demand is genuinely
    compiler-driven (the epoch payoff). CON: gated on D1's REVISE being
    resolved (the fabricated-message + entry-suppression mechanism), which
    the D1 judge says is a DIFFERENT, larger design than ratified — so
    SURFACE-1 cannot land until D1 re-lands. The §4.4 witness IR (%table:4/
    :8/:11) is what D1 must synthesize.

  SURFACE-2 (D1-INDEPENDENT, candidate (ii) explicit): the user WRITES the
    hand_demand.dr of §4.1 verbatim — demand_neighborhood is a real #local
    fed by a real #message `ask`, the guarded copy a hand-written clause.
    NO demand pass, NO fabricated ParsedMessage, NO F1/F2. D3's
    SUBGRAPH_INSTANTIATE lowers the already-present guarded copy as a keyed
    instance (recognized by a pattern: a bound #query whose relation is
    produced by a single JOIN against a demand-shaped relation). PRO: lands
    THIS epoch INDEPENDENT of D1's REVISE — the substrate (store + census +
    validators) is exercisable against a real oracle-refereed corpus case
    with ZERO dependence on the demand transform. This is p2b §4.3's
    candidate (iii)-with-a-real-.dr: substrate landed, witness REAL (not
    synthetic), oracle referee LIVE. CON: the demand is user-written, not
    compiler-synthesized — the substrate down-payment, not the payoff.

  RECOMMENDATION for owner decisions (a) surface / (d) D4-scope: pick
    SURFACE-2 for D3's LANDING WITNESS this epoch (it de-risks the store +
    census + validators against a real oracle without waiting on D1's
    re-land), and keep SURFACE-1 as the D4/D1-follow-on target (the same
    SUBGRAPH_INSTANTIATE lowering, reached from a synthesized guarded copy
    once D1 re-lands its fabricated-message mechanism). The lowering is
    IDENTICAL under both surfaces (the guarded copy's shape is the same,
    §4.2 IR); only WHO writes the demand relation differs. So D3's store and
    §5 census are surface-INDEPENDENT — they land under SURFACE-2 and carry
    to SURFACE-1 unchanged.

  HONEST DEPENDENCY: even SURFACE-2 needs a recognizer that turns the
    hand-written guarded copy into a SUBGRAPH_INSTANTIATE op (else it lowers
    as an ordinary JOIN+DiffTable, %table:4, and the InstanceStore is never
    exercised). That recognizer is the SURFACE-2 build cost (a pattern match
    on the guarded-copy shape, minting BuildSubgraphOps). It is SMALLER than
    D1's demand pass (no SIP walk, no message fabrication) — a bounded
    pattern recognizer over an already-built graph. WITHOUT it, hand_demand.
    dr compiles TODAY (it did this session) but as a plain JOIN — the store
    is dead code. So SURFACE-2's minimal cost is the recognizer + the store
    + the census; SURFACE-1 adds D1's re-landed pass on top.


======================================================================
§5. CENSUS + VALIDATORS FROM DAY ONE (P0 template, E-27/E-28 house law)
======================================================================
The P0 GROUP_UPDATE census (DR.cpp:2819-2851, fact 12) is the LITERAL
template. D3's family MUST NOT copy the gap P0 closed. Anchored to the
verbatim recount code read this session (DR.cpp:2819-2851).

----------------------------------------------------------------------
§5.1 THE RECOUNT — from the Query's OWN accessors, no mint-order ids
----------------------------------------------------------------------
Mirror `exp_group_update` (DR.cpp:2835) exactly. The recount input is the
SAME accessor the mint loop iterates (E-27: NEVER flow.instancestores.
size(), a mint-order tautology — the exact trap DR.cpp:2820-2823 warns
against):

  exp_subgraph = |query.Subgraphs()|         (SURFACE-1: the synthesized
                   subgraph views) OR the recognizer's identified guarded-
                   copy views (SURFACE-2). The recount iterates the SAME
                   set BuildSubgraphOps mints from — no skip.
  exp_instance_seal = exp_subgraph           (one INSTANCE_SEAL per view,
                   the V-INST-PAIR bijection, mirror V-AGG-PAIR).

  Two new expect() entries, mirroring DR.cpp:2850-2851 (the derived-op
  inventory count_kind for kSubgraphInstantiate / kInstanceSeal).

THE PER-OP KEY MULTISET (order-free, E-28), mirroring `GroupUpdateKey`
(DR.cpp:2836 = tuple<agg_table_ptr, provenance, algebra, view.UniqueId>):

  SubgraphKey = tuple<uintptr_t inst_table_ptr, uint8_t provenance,
                      uint8_t mode, uint64_t view.UniqueId()>
    - inst_table_ptr = the OUTER pub table (%table:4) via
      impl->view_to_model[view]->FindAs<DataModel>()->table (the DR.cpp:2839
      idiom) — the SOLE-WRITER table (§2.5), NOT a nested table pointer.
    - provenance = kSubgraph (a new AggProvenance-analog enum value).
    - mode = Rederive (the lowering selector, p2b §2.2 C-1e).
    - NOT instance_store_id — the mint-order artifact (E-28, exactly as
      statecell_id is deliberately unkeyed, DR.cpp:2843-2846); its bijection
      with INSTANCE_SEAL is the V-INST-PAIR structural check.
  Compared order-free (multiset), sorted both sides — same as the P0
  GroupUpdateKey comparison.

----------------------------------------------------------------------
§5.2 THE VALIDATORS (always-on, survive NDEBUG — DR-IR house convention)
----------------------------------------------------------------------
  V-INSTANCE-EFFECT (§3 totality, mirror V-AGG-EFFECT DR.cpp:2907-2914):
    every instance:demand/rebuild/emit/old effect names an Instance value;
    every SUBGRAPH_INSTANTIATE carries exactly {kDemandRead,
    kInstanceFold(×signs), kInstanceEmit, kInstanceOld, + publish_touched's
    counter±/kInIReadFrozen/kVecAppend triples}; every INSTANCE_SEAL carries
    exactly its sign-0 kInstanceSeal. No R1/R2/R3/GROUP_UPDATE op declares
    an instance effect. Asserted structurally in ValidateDROps.

  V-INSTANCE-SOLE (§2.5, mirror V-AGG-SOLE DR.cpp:663-665): the OUTER pub
    table (%table:4) has EXACTLY ONE deriver — the SUBGRAPH_INSTANTIATE's
    publish_touched. The SOLE-WRITER invariant (§2.5) on the member-view
    list. A second writer (e.g. the future incremental lowering wiring a
    nested DiffTable into the outer counters) aborts.

  V-INSTANCE-PAIR (mirror V-AGG-PAIR DR.cpp:3002-3012): instance_store_id
    bijects with INSTANCE_SEAL — every SUBGRAPH_INSTANTIATE's store id has
    exactly one INSTANCE_SEAL and vice versa. This gives the unkeyed
    instance_store_id its guarantee (E-28), replacing the census key it is
    deliberately NOT in.

  V-PRED-XCHECK site (§2.5, the F17/F18 bug-class kill for the new family):
    a site for SUBGRAPH_INSTANTIATE tying the emitted CF tree back to the
    flow op — specifically that publish_touched is the ONLY emitted writer
    of %table:4's counter± (the SOLE-WRITER cross-check). Continues the site
    numbering after ingest's Site 5 (DR.cpp:2113-2158 analog).

  op_stratum NO-DEFAULT-0 (fact 12 / p2b §5.4): the op_stratum case for
    kSubgraphInstantiate keys on the subgraph_instance_stratum map with NO
    silent default-0 — a mis-stratified demand drain must FAIL V-READY, not
    pass on a default (the DR.cpp:3300-3302 false-negative fix honored).

----------------------------------------------------------------------
§5.3 THE HOUSE BET REGISTERED (where the census fires first)
----------------------------------------------------------------------
Per E-1..E-45 (every first-time census finds a divergence), the SUBGRAPH
census WILL fire on first run. §6 P6 registers WHERE.


======================================================================
§6. PRE-REGISTERED PREDICTIONS
======================================================================
  P1 SUITE GROWTH: 165 -> 166 by the demand witness (neighborhood /
     hand_demand.dr under SURFACE-2 + .main.cpp driver + .batches +
     oracle/monotone goldens), oracle-blessed — the aggregate_1 diagnostic-
     to-golden FLIP precedent (a new-feature path adds a golden, moves
     nothing). If the SCC-demand reject witness lands too: 166 -> 167.

  P2 EXISTING-165 ZERO CHURN: all 165, all 4 golden modes, BYTE-IDENTICAL.
     The InstanceStore + BuildSubgraphOps are a NEW-feature path reached
     only by a subgraph view; no existing case has one (SURFACE-2's
     recognizer fires only on the guarded-copy shape, absent from the 165).
     Under SURFACE-2 there is NO mode flag and NO demand pass — the 165 are
     structurally untouched. (Contrast D1 SURFACE-1's mode-gate, a D1/D4
     concern, not D3's.)

  P3 DRIVER ABI — Rederive = ZERO new driver functions (p2b §6.7). The
     Rederive lowering reruns the sub-query inline (the IR:71 select) — the
     sub-query IS the reduction body, emitted inline, NOT a driver-supplied
     free function. Contrast the R3 aggregate ABI (f_combine/f_reduce free
     functions): a subgraph instance under Rederive needs NONE. The witness
     driver observes the bound #query API `neighborhood_bf(db, Start)` — a
     read-time cursor probe (fact 3, transitive_closure shape), NO forcer
     under SURFACE-2 (no #query ForcingMessage). [D3-Δ vs D1: D1's SURFACE-1
     gains (log,functors) via the synthesized forcer, E-44; D3's SURFACE-2
     does NOT — the demand is user-driven via the `ask` message entry point,
     an ordinary message batch, so the query stays a pure read probe.] So
     the SURFACE-2 driver has ZERO new ABI beyond the existing bound-query
     cursor + the ordinary `ask` message send.

  P4 ctest ADDITIVE: 3/3 unchanged; the DR-IR always-on validators compile-
     time green (now including V-INSTANCE-EFFECT/SOLE/PAIR + the census +
     the V-PRED-XCHECK site).

  P5 Q5 NEUTRAL: subgraphs are a new-feature path, not a Q5-chain change;
     the flagship @128 point unmoved (the P0 flat result carries).

  P6 THE HOUSE BET — the census FIRES at least once on first run, MOST
     LIKELY at ONE of:
       (bet A, primary) THE RECOUNT-ACCESSOR vs MINT DISAGREEMENT on a
         corner shape: a subgraph view whose OUTER pub table is SHARED with
         another view (CSE-merged pub tables), so |query.Subgraphs()| counts
         the view but the pub-table-keyed multiset collides — the exact
         "recount from the query, mint from the flow, they disagree on a
         table-sharing corner" trap the P0 census closed for aggregates
         (DR.cpp:2820-2823). SURFACE-2's recognizer identifying the guarded-
         copy view is the least-pinned input.
       (bet B) THE SOLE-WRITER CHECK (V-INSTANCE-SOLE) fires because the
         recognizer leaves the ORIGINAL plain-JOIN writer of %table:4 in
         place ALONGSIDE the SUBGRAPH_INSTANTIATE — two writers, the double-
         count seam re-surfacing at BUILD time (the recognizer must REPLACE
         the JOIN, not add beside it). This is the SURFACE-2-specific hazard:
         hand_demand.dr TODAY has the plain JOIN (IR:74); the recognizer
         must excise it.
       (bet C) THE NESTED-WATERMARK COHERENCE check (§3.3, the E-35-specific
         DebugValidate) fires because the §3.2.1 rebuild reset advances the
         monotone watermark without re-Sealing coherently across a rebuild
         (the reset-then-refill order vs the Seal order).
     PRE-REGISTERED: a FINDINGS entry iff the census/validator fires on a
     REAL miscount (not a fix-up), or the oracle disagrees on the witness's
     birth/death/rebuild answer.

  P7 THE p2b §7.3 SEAM PROBE — DISCHARGED, pre-registered as closed: the
     double-count seam CANNOT arise under stage (b) (either surface) because
     SW-1 (monotone nested) removes arm (b) and SW-2 (sole outer writer)
     removes the second counter home (§2). The oracle (definitional per-
     Start recompute) is the referee: if a witness batch ever double-counts
     a (Start,Node) row, the oracle-vs-emitted diff fires. Prediction: it
     does NOT fire under Rederive; RESERVED to re-open only under the future
     incremental-maintenance lowering (§1.4, §2.5 residual).


======================================================================
§7. SUMMARY OF THE TWO HOLES CLOSED
======================================================================
  HOLE-A (p2b §3.1 sealed side / E-35): CLOSED by §1.4 — the nested set is
    a MONOTONE Table whose OWN Seal/sealed watermark (A3:277-282) answers
    old(iid)={r:InI(r)} via the real `id<sealed` compare. The p2b DiffTable-
    Seal dead end (E-35) is avoided by using the monotone class that HAS
    Seal; the deleted store-held watermark field (E-35, unsound under
    CompactDead, A7) is replaced by a per-instance nested-table watermark
    that rides the row. emit/old type symmetry honored (both membership
    predicates over one table, §1.4).

  HOLE-B (p2b §7.3 double-count seam): CLOSED by §2 — the SOLE-WRITER
    redesign. SW-1 (monotone nested → no independent per-row retraction)
    removes the seam's arm (b); SW-2 (publish_touched the sole outer-table
    writer) removes the second counter home. The seam cannot arise for the
    stage-(b) Rederive lowering. V-INSTANCE-SOLE + a V-PRED-XCHECK site
    guard it. The future incremental lowering re-opens it and inherits
    §2.5's obligation.

  Both closures are SUBSTRATE-FIRST and surface-INDEPENDENT: the store,
  census, and validators land under SURFACE-2 (D1-independent, this epoch)
  and carry to SURFACE-1 (D1-dependent, follow-on) unchanged.
