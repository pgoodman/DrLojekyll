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


======================================================================
## AMENDMENTS (2026-07-17, post-judge)
======================================================================
Judge-d3 returned VERDICT REVISE with F1-F9 (two CRITICAL at the rebuild
boundary). The body above (§1-§7) is LEFT INTACT as the record of the
first design; this section is the substantial redesign that supersedes
§1.4's "one monotone table, two predicates" model where the two conflict.
Every anchor here was re-read THIS session on branch demand-seeds (Table.h,
StateCell.h END TO END; the judge report; lane-substrate.md; p2b §1.5/§3/
§7.3). Each amendment is TAGGED with the judge finding(s) it discharges.

THE CORE DEFECT THE JUDGE PROVED (F1/F2, both CRITICAL). §1.4's choice to
make the nested set a MONOTONE Table closes HOLE-B structurally (SW-1: a
monotone table has no independent per-row retraction, Table.h:269-274) BUT
CONTRADICTS the §3.2.1 Rederive rebuild. A monotone Table cannot delete or
clear (Table.h:235 `TryAdd` is the SOLE mutator), so a rebuild REPLACES the
table; a fresh table has `sealed=0`, so `old()=InI(r)=id<sealed=id<0` is
FALSE for every row (F1) — batch-start is unreadable exactly when band (b)
publish_touched needs it. And the §2.3 one-net-pair partition — "old_r and
new_r read from the SAME table at the SAME instant" — is VOID once band
(a)'s reset intervenes before band (b) (F2): old_r is on the pre-reset
table, new_r on the fresh one, different tables/instants, so the partition
that gave "at most one net pair per row" no longer holds. §1.4's "one
table, two predicates" simplification is precisely what the reset breaks.

----------------------------------------------------------------------
## A0. THE THREE RESOLUTIONS WORKED, AND THE PICK
----------------------------------------------------------------------
Three candidate substrates were worked end-to-end against the code.

  R-C (ORDERING FIX — keep one monotone table, move the reset AFTER
    publish reads old): REJECTED, unsound. The reset is not an ordering
    problem, it is a CLASS problem. A monotone Table's `InNew` is
    always-true for every stored row (Table.h:255-260); under Rederive the
    new set may DROP a row the old set had, and a monotone table cannot
    make that row LEAVE (no delete/clear, Table.h:235). Append-without-
    clear makes `current` = old ∪ new and `InNew` reports the dropped row
    as present FOREVER — emit(iid) is wrong, and moving the reset later
    does not fix it (the stale row still must leave before the NEXT
    epoch's emit). Against the commit-tail order (fact 15; average_weight
    .datalog.h:877-944) the DR band structure could ORDER a
    reset-after-publish, but no ordering rescues a class that cannot drop
    a row. R-C COLLAPSES into R-A (keep a frozen copy) or R-B (make
    `current` differential). REJECT.

  R-B (DIFFERENTIAL nested DiffTable, per-row kInI — the E-35-consistent
    arm): SOUND but re-opens the seam it must re-close. Nested set = a
    DiffTable; old(iid)={r:InI(r)} via the flag form (Table.h:357-360,
    kInI rides the row under CompactDead, Table.h:584-585 — E-35's crux
    cashed in). Rederive rebuild = fold(-1) over stale rows + fold(+1)
    over the new derivation (NO table replacement, NO reset) — so F1/F2
    VANISH (old() is always readable; kInI is a persistent flag not a
    fresh-table watermark, and InI(r)/InNew(r) are two reads of ONE row's
    flag byte at ONE instant). BUT SW-1 is GONE: a nested DiffTable CAN
    independently retract a row (`NetDeleted`, Table.h:405-409), so the
    p2b §7.3 seam's arm (b) is STRUCTURALLY BACK. HOLE-B must then be
    re-closed by SW-2 (sole outer writer) PLUS a claim-gate-order argument
    on the NESTED table's own counters — the p2b §7.3 question re-asked.
    That re-close IS available (seed-before-drain / TryClaimDel-Add
    phantom-drop on the nested table's Commit, Table.h:437-497/517-551),
    but it puts BOTH the nested Commit AND the outer publish Commit in the
    commit tail with an ordering obligation (nested Commit before the
    publish reads old/new) — a heavier substrate, and it reintroduces the
    exact double-writer geometry the epoch wanted to foreclose. R-B is the
    RESERVED future incremental-maintenance lowering (see A6), not the
    stage-(b) ship.

  R-A (FROZEN-PAIR — two MONOTONE tables per instance, SWAP not copy):
    SOUND, CHOSEN. Details in A1. It keeps SW-1 (both tables monotone, no
    independent retraction) so HOLE-B stays closed, and it makes old()
    coherent across a rebuild without a watermark by holding the
    batch-start set as a WHOLE frozen table, swapped (not copied) at seal.

DECISION: **R-A**. It is the minimum change that keeps the §2 SW-1/SW-2
seam closure intact (both tables monotone) while making old() a coherent
whole-table read that no rebuild can invalidate — paying two POD handles
per instance and an O(1) pointer swap, never a row copy.

----------------------------------------------------------------------
## A1. R-A SPECIFIED — the frozen-pair store [discharges F1, F2, F7, F8]
----------------------------------------------------------------------
PER INSTANCE, TWO store-owned MONOTONE Table<PubRow> handles:
  `current` — the working published-row set this epoch (Fold materializes
    into it during the Rederive rebuild; emit(iid) reads it).
  `frozen`  — the batch-START published-row set (old(iid) reads it;
    written ONLY at INSTANCE_SEAL by the swap; never mutated mid-batch).

The `Working` handle becomes TWO raw pointers (still trivially-copyable,
K7, so Vec<Working> is fine):
    struct Working { Table<PubRow> *current{nullptr};
                     Table<PubRow> *frozen{nullptr}; };
    static_assert(std::is_trivially_copyable_v<Working>);   // C1/K7
The store's pool becomes `Vec<Table<PubRow> *> table_pool` holding BOTH
tables per instance (and any recycled spare). NO `sealed` field, NO
per-table watermark drives old() — E-35 satisfied without needing Seal at
all (the watermark question is DISSOLVED, not answered: old() is a whole
table, not an id<sealed compare).

READS (both membership tests over WHOLE coherent tables):
    old(iid) = enumerate frozen's rows   (all of them; RowStore full scan)
    new(iid) = enumerate current's rows
  per candidate row r:
    old_r := (frozen->Find(r)  != kNoRow)   (r stored in frozen;
                                              RowStore::Find, Table.h:79)
    new_r := (current->Find(r) != kNoRow)   (r stored in current)
  These are two reads of two coherent tables at ONE instant (post-rebuild,
  pre-swap) — the §2.3 partition RESTORED and now SOUND (A2).

WHY F1/F2 DISCHARGE:
  - old() is ALWAYS coherent (F1): `frozen` is written only at the swap
    (Seal), never reset mid-batch; the §3.2.1 rebuild materializes into
    `current`, leaving `frozen` intact. old() is readable before or after
    the rebuild — no reset-vs-read ordering hazard exists. This is the
    "frozen kInI-like constant, RAW-only against Seal" discipline
    (StateCell.h:30-31) lifted to the SET level: two whole tables, not one
    table + a watermark.
  - the one-net-pair partition holds (F2): old_r from `frozen`, new_r from
    `current`, both coherent, at one instant; the four cells partition r
    exactly (A2). No reset intervenes — the swap is at Seal, strictly
    AFTER band (b). §2.3's premise is TRUE under R-A.
  - Old() return signature (F7): Emit(iid) returns *current, Old(iid)
    returns *frozen — TWO DISTINCT pointers, both always coherent. The
    F7 API defect (Old==Emit, unreadable post-reset) is gone.

THE SEAL/SWAP (INSTANCE_SEAL, per touched instance) [E5 amended]:
  after band (b) has read both frozen and current, the swap is:
    tmp            = w.frozen;      // last epoch's start set, now stale
    w.frozen       = w.current;     // this epoch's published set becomes
                                    //   next epoch's batch-start set
    w.current      = Recycle(tmp);  // emptied table for next rebuild
    sealed_occupied[iid] = (w.frozen->NumRows() > 0) ? 1u : 0u;
  Swap is an O(1) pointer exchange of POD handles — ROWS ARE NEVER COPIED
  (contrast the p2b Option-B copy §1.2, which is why the two-table set is
  affordable here). `Recycle(tmp)` empties the stale table for reuse (A3).
  Ordered LAST in the trailing commit band, mirroring table_36.Seal
  (D1:942) — the swap plays STATE_SEAL's role.

DebugValidate under R-A [discharges F8]:
  - FROZEN≠CURRENT: for every occupied instance the two handles are
    distinct tables (the swap gave them distinct storage). Replaces the
    §3.3 "nested watermark <= NumRows" check, which was MEANINGLESS under
    R-A (no per-table watermark) and passed trivially on a fresh table
    (the F8 defect — it could not detect the reset drift it was
    advertised against).
  - SEALED-OCCUPANCY COHERENCE: sealed_occupied[iid] == (frozen->NumRows()
    > 0) at the batch boundary — `frozen` holds the batch-start set so its
    non-emptiness IS sealed occupancy.
  - SEAL COHERENCE: after the swap, touched empty, no touched_flag
    survives (StateCell.h:603-605 mirror).
  - NON-ALIASING (three-way, §3.3): instance ids disjoint from the outer
    pub table's row ids and from current's/frozen's own row ids.

MEMORY, PRICED HONESTLY: two monotone tables per live instance (a fixed
working set, not a per-epoch leak — the old `current` BECOMES `frozen`,
the old `frozen` is RECYCLED as the fresh `current`). Steady state is
2 tables/instance; the swap adds one recycled spare that is reused each
epoch. Rows are never duplicated (the p2b Option-B redundancy returns only
as the SWAP, O(1), not the copy §1.2 rejected).

----------------------------------------------------------------------
## A2. THE ONE-NET-PAIR PARTITION, RE-DERIVED UNDER R-A [discharges F2]
----------------------------------------------------------------------
The §2.3 argument as written is VOID (its premise — one table, one instant
— fails across the §3.2.1 reset, F2). Re-derived under R-A:
  Per touched instance iid, per candidate row r, band (b) reads:
    old_r := (frozen->Find(r)  != kNoRow)     [frozen, batch-start; A1]
    new_r := (current->Find(r) != kNoRow)     [current, this epoch; A1]
  BOTH from coherent whole tables at ONE instant (post-rebuild of current,
  pre-swap). The occupancy-generalized guard:
    (F,T) -> emit +new (Start ++ r) into the OUTER pub DiffTable  [born row]
    (T,F) -> emit -old (Start ++ r) into the OUTER pub DiffTable  [dropped]
    (T,T) -> emit NOTHING                       [unchanged — OQ3, no churn]
    (F,F) -> emit NOTHING                       [never present]
  r falls in EXACTLY ONE cell (the two booleans partition the 2x2 space) =>
  AT MOST ONE net pair per (iid, r). This is EXACT because old_r and new_r
  are two independent stored-membership reads of two tables that CANNOT
  disagree with themselves (unlike the §1.4 one-table-across-reset case).
  The (T,T) cell is the clean OQ3 win: an UNCHANGED row across a rebuild
  emits NOTHING at all — R-A never emits the -old/+new pair and relies on
  outer annihilation (contrast §3.2.1, which emitted both and leaned on the
  outer table's claim gates to annihilate). Fewer outer folds, and the
  partition is the discharge — not an assertion.

SW-1 SURVIVES (HOLE-B stays closed): `current` and `frozen` are BOTH
monotone (no NetDeleted; K1). Neither folds into the outer pub table's
counters — they are read-only membership oracles the guard consults. The
seam's arm (b) ("row independently retracted inside the nested relation")
is STRUCTURALLY ABSENT. SW-2 unchanged: the outer pub DiffTable's SOLE
writer is publish_touched (§2.2 / V-INSTANCE-SOLE). The §2 HOLE-B closure
holds verbatim under R-A — R-A changes only HOW old() is read (whole
frozen table vs a watermark), not the sole-writer geometry.

----------------------------------------------------------------------
## A3. THE §3.2.1 REBUILD MECHANIC, REDONE [discharges F1]
----------------------------------------------------------------------
§3.2.1 (reset the single monotone table) is SUPERSEDED. Under R-A the
Rederive rebuild on a demand-key change for iid is:
  band (a) minus arm (demand retracted): NO reset needed. `current` is
    already the recycled-empty table from last epoch's swap (or is emptied
    now if this is a same-epoch re-demand). The instance's death is
    signaled by leaving `current` EMPTY; publish then sees (T,F) for every
    frozen row -> -old only. `frozen` is untouched and holds the set to
    retract.
  band (a) plus arm (demand added/changed): materialize the Rederive
    result (rerun edge(Start,_), the IR:71 select on %table:11) via
    TryAdd into `current` (monotone dedup, Table.h:235). `frozen` holds
    last epoch's set (empty on birth). publish compares current vs frozen.
  Recycle(tmp) at swap empties the stale table: EITHER teardown+MakeVec a
    fresh Table through table_pool (the StateCell.h:560-566 pool idiom, no
    new primitive) OR — if the F4 COST witness shows churn dominates — an
    in-place monotone Table::Reset() (truncate rows/hashes to 0, zero
    sealed, clear hash slots; a bounded op, no per-row work). Reset() is
    MEASURE-FIRST residue (A4).
  THE OLD FROZEN TABLE IS READ FOR old() THROUGHOUT the batch (it is only
  swapped at Seal), so there is NO "read-old-before-reset" intra-op
  ordering to get wrong — the F1 hazard is dissolved, not merely reordered.

----------------------------------------------------------------------
## A4. REBUILD CHURN, PRICED WITH A MEASURE-FIRST BAR [discharges F4]
----------------------------------------------------------------------
R-A pays ONE monotone-table Recycle (teardown+reconstruct OR Reset) per
instance PER EPOCH IT REBUILDS — bounded by |touched instances|, NOT per
demand-key and NOT unbounded (the F4 "N stale tables accumulate" worry does
not arise: the two-table working set is fixed; the stale table is recycled,
not leaked). This is DISTINCT from the dead-INSTANCE sweep residue (§3.1 /
StateCell.h:21-24) — two separate residues, two separate COST triggers.
  WHAT THE COST WITNESS MEASURES (bench harness, PerfRoadmap §2; the COST
    instrument, never a correctness gate): under -DDRLOJEKYLL_BENCH_COUNTERS
    a per-instance Recycle counter (table teardown/Reset events) + allocator
    bytes churned by the table_pool recycle, over a DEMAND-FLAP workload
    (one instance repeatedly demanded/retracted), compared to the eager
    reference's join re-run cost on the same stream.
  WHEN DEAD-TABLE RECYCLING / Reset() BECOMES MANDATORY: if the witness
    shows teardown+reconstruct (drop-to-pool + MakeVec) DOMINATES — rebuild
    churn > the join re-run it replaces on a demand-flap workload — the
    in-place Reset() primitive lands to replace the teardown. Until the
    witness shows it, stage (b) ships teardown-reconstruct via the existing
    pool idiom (zero new primitive) and Reset() stays D5-style residue.
    MEASURE-FIRST, same discipline as the dead-INSTANCE sweep.

----------------------------------------------------------------------
## A5. F3 — DIFFERENTIAL/GROWABLE INPUT: FENCED [discharges F3]
----------------------------------------------------------------------
The judge proved band (a) drains ONLY demand frontiers (K9: IR:69 the
demand pivot loop); there is NO arm over the INPUT (edge) net frontier, so
an edge change while demand persists triggers NO rebuild -> a STALE set,
diverging from the eager reference (which re-runs the join on
^receive:add_edge, IR:42->49). The §1.4 pt2 / §1.3 (C-ii) "a differential
input collapses to (C-i)" claim is FALSE and is RETRACTED: it does not
collapse — a differential input needs an input-frontier rebuild arm stage
(b) does not build.
  DECISION: FENCE, not wire (the R2-acyclic-first discipline, the p2b §1.5
    fence's sibling; wiring an input-frontier arm is the incremental-
    maintenance obligation A6 defers, and it wants its own reviewed
    argument).
  THE PREDICATE THAT FENCES IT: at BuildSubgraphOps time, assert every
    SUMMARIZED INPUT view of the subgraph (the non-demand JOIN sides — for
    the witness, `edge`) is MONOTONE (its data model is a monotone Table,
    not a DiffTable — reusing the DR-IR's existing monotone-vs-differential
    model classification). Any deletion-capable summarized input (a
    retract-capable #message, or a view reachable from a differential
    producer) is REJECTED.
  THE DIAGNOSTIC (clean, 4-mode, agg_in_scc_1 mold): "subgraph instance
    over a deletion-capable input relation `<edge>` is not yet supported
    (only add-only summarized inputs; a differential input requires the
    incremental-maintenance lowering)". Emitted at the Build.cpp
    num_errors->nullopt pre-pass gate (Build.cpp:1071/1178), all 4 modes —
    the same gate the ViewSelfReachable acyclic fence uses.
  F3-WITNESS CONSEQUENCE: hand_demand.dr's `edge` is fed by the #message
    add_edge (add-capable). For the fence to PASS, the landing witness's
    summarized input must be ADD-ONLY (a monotone #local / an add-only
    input). The witness .dr is AMENDED so `edge` is add-only, keeping the
    landing case inside the fence. This is the honest scope: stage (b)
    demonstrates demand-keyed instances over MONOTONE inputs; a deletable
    input is the cleanly-diagnosed unhandled case (A6's reserved lowering).

----------------------------------------------------------------------
## A6. THE RESERVED INCREMENTAL-MAINTENANCE LOWERING (R-B) [context for F3/F4]
----------------------------------------------------------------------
The future lowering that HANDLES a differential input (and avoids the
whole-instance rebuild churn A4 prices) is R-B: the nested set is a
DiffTable, an input delta folds into the standing instance (fold(-1)/
fold(+1)) instead of a whole rebuild, old()={r:InI(r)} on the nested
DiffTable's own kInI (compaction-safe, rides the row, Table.h:584-585).
That lowering RE-OPENS the p2b §7.3 seam (a nested DiffTable CAN
independently retract) and MUST re-establish SW-2 by folding the nested
DiffTable's NetAdded/NetDeleted frontiers (Table.h:405-433) THROUGH the
publish_touched guard (never directly into the outer counters), plus a
claim-gate-order (seed-before-drain) argument on the NESTED table's Commit.
Pre-registered here (as §2.5's residual was) so it is not re-litigated: R-B
is the incremental arm, gated behind the differential-input fence A5
lifts. Stage (b) SHIPS R-A (monotone frozen-pair, add-only inputs).

----------------------------------------------------------------------
## A7. CENSUS RECOUNT INPUT UNDER SURFACE-2 [discharges F5]
----------------------------------------------------------------------
§5.1's `exp_subgraph = |query.Subgraphs()|` is UNBUILDABLE: query.Subgraphs()
does not exist (QueryImpl has Joins/Selects/Tuples/KVIndices/Aggregates/
Maps, no QuerySubgraph — judge grep of Query.h). The recount must be a
GENUINE INDEPENDENT re-derivation from a DIFFERENT object than the mint
loop (E-27; never flow.instancestores.size(), DR.cpp:2820-2823).
  THE REAL RECOUNT SOURCE (SURFACE-2): the recognizer that identifies
    guarded-copy views MUST expose its identified set as an explicit list
    on the surface's own query object — it registers each recognized
    subgraph as a QuerySubgraph-analog node (a `subgraph_views` list) on
    QueryImpl AT RECOGNITION TIME, distinct from the flow mint. The census
    recounts `|query.RecognizedSubgraphs()|` (the accessor the SURFACE-2
    recognizer MUST provide) vs the flow's minted SUBGRAPH_INSTANTIATE
    count. Recognition (populates the query list) and BuildSubgraphOps
    (consumes it, mints ops) are TWO passes over TWO objects — a genuine
    cross-check, not a tautology.
  STATED PLAINLY: a recognizer-ONLY census whose recount IS the
    recognizer's mint output (no independent query-side accessor) WOULD be
    an E-27 tautology, and is THEREFORE NOT CHARTERED. The full census is
    chartered ONLY once the recognizer provides the query-side
    RecognizedSubgraphs() accessor. If that accessor is not built this
    stage, the census DOWNGRADES to the V-INSTANCE-PAIR bijection-only
    check (SUBGRAPH_INSTANTIATE <-> INSTANCE_SEAL, a flow-internal
    structural bijection needing no query accessor) — the honest fallback
    (judge A4).

----------------------------------------------------------------------
## A8. THE RECOGNIZER DEPENDENCY, STATED PLAINLY [discharges F6, and F9]
----------------------------------------------------------------------
hand_demand.ir is byte-structurally identical to a plain 2-relation join
(K10: == plain_join.ir modulo message name). A SURFACE-2 corpus case
compiled TODAY exercises the plain JOIN + %table:4 (DiffTable) path, NOT
the InstanceStore; the oracle/monotone goldens would PASS on the plain-JOIN
emission with the store as DEAD CODE.
  WHAT THIS EPOCH's WITNESS DE-RISKS: the STORE (InstanceStore layout, the
    R-A frozen-pair), the CENSUS (recount A7 + bijection), the VALIDATORS
    (V-INSTANCE-EFFECT/SOLE/PAIR + the V-PRED-XCHECK site), and the
    SEAL/swap machinery — ALL under a HAND-BUILT op mint (BuildSubgraphOps
    invoked directly on a hand-recognized view / a minimal shape-match), so
    the substrate is unit-exercisable.
  WHAT IT DOES NOT DE-RISK: the JOIN-EXCISION RECOGNIZER (the pattern match
    that REPLACES the plain JOIN + DiffTable with a SUBGRAPH_INSTANTIATE, so
    the store is reached from source). Until that recognizer exists and
    excises the JOIN, hand_demand.dr compiles as a plain JOIN and the store
    is not reached from the .dr.
  STATE PLAINLY (judge A5): the SURFACE-2 landing witness does NOT exercise
    the store until the recognizer excises the plain JOIN; the store and
    census are NOT de-risked against a live oracle by hand_demand.dr as it
    compiles today. They are de-risked AS SUBSTRATE (hand-built op mint),
    NOT end-to-end. F9's "strictly above demand input" stratum claim is
    inherited from the excised JOIN's stratum (sound for the JOIN, not an
    independent SUBGRAPH_INSTANTIATE property) — cosmetic given this
    recognizer gate; op_stratum keys the subgraph_instance_stratum map to
    the excised JOIN's stratum, NO-DEFAULT-0 (§5.2) unchanged.

----------------------------------------------------------------------
## A9. WHAT CHANGES IN §3/§4 UNDER R-A (delta index, no body rewrite)
----------------------------------------------------------------------
The following body claims are SUPERSEDED by R-A (read them through this
index):
  - §1.4 "one monotone table, two predicates ... TIGHTER than the p2b
    two-word sketch": SUPERSEDED. R-A is TWO monotone tables (current +
    frozen) read as two whole-table memberships; the "one table" claim is
    withdrawn (F1). The type symmetry is preserved differently: emit reads
    current, old reads frozen, both whole coherent tables.
  - §3.1 Working { Table<PubRow>* rows; }: SUPERSEDED by
    Working { Table<PubRow>* current; Table<PubRow>* frozen; } (A1).
  - §3.1 Emit/Old both `return *working[iid].rows`: SUPERSEDED — Emit
    returns *current, Old returns *frozen (A1; F7).
  - §3.1 Seal "calls nested.Seal()": SUPERSEDED by the SWAP (A1) — no Seal
    on either monotone table is needed for old() (the watermark question
    is dissolved); the swap plays STATE_SEAL's role.
  - §3.2.1 rebuild-reset mechanic: SUPERSEDED by A3 (materialize into
    current, frozen intact, recycle at swap).
  - §3.3 "nested watermark <= NumRows" DebugValidate: SUPERSEDED by the
    A1 FROZEN≠CURRENT + SEALED-OCCUPANCY checks (F8).
  - §4.3 band (a) "[D3-d] instance:rebuild ... read old() then reset
    table": SUPERSEDED — materialize into current; old() reads frozen; no
    reset (A3). band (b) reads old_r from frozen, new_r from current (A2).
  - §4.3 INSTANCE_SEAL kInstanceSeal "delegates to working[iid].rows->
    Seal()": SUPERSEDED by the SWAP (A1). Effect kind kInstanceSeal now
    names the swap (global:rmw), not a monotone Seal.
  - §4.5 "differential input collapses to (C-i)": RETRACTED; fenced (A5).
UNCHANGED and still load-bearing: §2.2 SW-1/SW-2 (both tables monotone so
SW-1 holds; A2), §2.5 V-INSTANCE-SOLE, §4.2 the E-32 real equi-join demand
edge, §5.2 validator shapes + op_stratum NO-DEFAULT-0, §4.5 the
ViewSelfReachable acyclic-demand fence (distinct from the A5 input fence),
the SURFACE-1/SURFACE-2 split and carry (A8).

----------------------------------------------------------------------
## A10. FINDINGS DISCHARGE SUMMARY
----------------------------------------------------------------------
  F1 (CRITICAL): old() reads the whole `frozen` table, never reset
    mid-batch -> always coherent across a rebuild (A1/A3).
  F2 (CRITICAL): the one-net-pair partition re-derived on two coherent
    tables at one instant, pre-swap -> premise TRUE, exact (A2).
  F3 (HIGH): differential/growable input FENCED with a monotone-input
    predicate + clean 4-mode diagnostic; the "collapses" claim retracted;
    witness input amended add-only (A5).
  F4 (HIGH): rebuild churn priced — one Recycle per rebuilt instance,
    bounded; MEASURE-FIRST bar for the Reset() primitive (A4).
  F5 (MEDIUM): recount from a recognizer-populated query-side
    RecognizedSubgraphs() accessor (independent of the flow mint); a
    recognizer-only census is an E-27 tautology and is NOT chartered;
    bijection-only downgrade otherwise (A7).
  F6 (MEDIUM): stated plainly — the store is de-risked as SUBSTRATE (hand
    mint), NOT end-to-end until the JOIN-excision recognizer exists (A8).
  F7 (LOW): Old returns *frozen, Emit returns *current — distinct, both
    coherent; API defect gone (A1).
  F8 (LOW): DebugValidate replaced with FROZEN≠CURRENT + sealed-occupancy
    checks that constrain real R-A state (A1).
  F9 (NIT): strict-above stratum re-scoped as inherited from the excised
    JOIN's stratum, recognizer-gated (A8).

----------------------------------------------------------------------
### A11 — round-2 punch-list (2026-07-17, per judge-d3-round2)
----------------------------------------------------------------------
Round-2 (scratchpad/design/judge-d3-round2.md) re-judged the A0-A10
redesign: VERDICT APPROVE-WITH-NITS, both CRITICALs (F1/F2) confirmed
discharged by an independent by-hand 3-batch trace. Five residuals
(NF1/NF2/NF3 + N1/N2), discharged here in place against the sections
that named them.

  NF1 (MEDIUM, against A7) — the census-scope overclaim, corrected.
    A7's "a genuine cross-check, not a tautology" (:1409) OVERSTATES what
    the census guards. Round-2 traced it exactly: `RecognizedSubgraphs()`
    (the recount) and `BuildSubgraphOps`'s mint loop (the count-under-test)
    are TWO PASSES but share ONE COMMON CAUSE — the recognizer's own
    `subgraph_views` list. If the recognizer mis-identifies the guarded-copy
    set (misses or over-collects a view), BOTH the recount and the mint
    read the SAME wrong list, so recount==mint==wrong-N and the |list|==
    |minted| check AGREES on the wrong answer. This is the P0 E-27
    tautology relocated one level up, for recognizer bugs specifically —
    it does NOT survive for mint-LOOP bugs (a loop that skips or
    double-mints a view the recognizer DID correctly identify is still
    caught, because that divergence is not shared with the recognizer
    pass). CORRECTED CLAIM (replaces "a genuine cross-check, not a
    tautology," A7 :1408-1409): the census is a genuine cross-check of the
    MINT LOOP against the recognizer's own list — it catches BuildSubgraphOps
    skipping, duplicating, or otherwise diverging from what the recognizer
    identified. It does NOT cross-check the RECOGNIZER against anything
    independent; recognizer correctness (did it identify the right guarded-
    copy views at all) rests on the oracle-refereed witness (`bin/Oracle`,
    the definitional per-group/per-instance recompute) plus the `-ir-out`
    structural gate (K10: byte-identical to the plain-join IR modulo message
    name — any recognizer drift from the intended shape shows up as an IR
    diff a reviewer reads), NOT on this census. The bijection-only downgrade
    (A7 :1414-1418) is unaffected by this correction — it was already
    labeled the weaker fallback and inherits the same scope note.

  NF2 (LOW, against A1) — new always-on validator: V-INST-FRESH.
    The A2 partition's exactness depends on `current` being EMPTY at the
    start of every rebuild (so new()=current reflects ONLY this epoch's
    Rederive output, never a stale carry-over). R-A gets this from
    Recycle(tmp) handing back an empty table (A1 :1228, A3 :1306-1311); a
    Recycle bug that returns a non-empty table would silently corrupt the
    (T,T) suppression (a stale row reads as new, wrongly suppressing a
    genuine +new or fabricating a false unchanged). A1's DebugValidate list
    (FROZEN≠CURRENT, SEALED-OCCUPANCY COHERENCE, SEAL COHERENCE, NON-
    ALIASING — A1 :1236-1249) does not check this. Add a fourth always-on
    check to that list, named **V-INST-FRESH**: at rebuild entry, for every
    instance about to be touched this epoch, assert `current->NumRows()==0`
    before the Rederive materialize begins (equivalently: assert the
    freshly-swapped `current` handed out by last epoch's Recycle has
    NumRows()==0, checked once per touched instance at band (a) entry).
    Same convention as the other four checks: fprintf+abort, survives
    NDEBUG, placed alongside FROZEN≠CURRENT/SEALED-OCCUPANCY/SEAL-COHERENCE/
    NON-ALIASING in the DebugValidate pass. This closes the one gap in the
    F8 discharge: F8 replaced the meaningless watermark check with checks
    that constrain real R-A state, but none of them detected a Recycle that
    handed back a dirty table; V-INST-FRESH does.

  NF3 (LOW, against A3) — the intra-batch demand flap, walked through
    the F2 partition explicitly.
    A3's band (a) minus-arm text (:1298-1300) hedges with "or is emptied
    now if this is a same-epoch re-demand" without naming the mechanic.
    Scenario: within ONE batch, iid's demand is retracted THEN re-demanded
    (a same-epoch flap on one key). Walked through A2's partition:
      1. Batch-start: `current` already recycled-empty from last epoch's
         swap (A3's ordinary case); `frozen` holds the batch-start set,
         untouched all batch.
      2. RETRACT ARM fires first (net frontier orders retract before
         re-add within a batch — the demand pivot's own delta ordering,
         not a new mechanic): `current` stays/becomes EMPTY. If band (a)'s
         plus arm had already materialized rows into `current` earlier in
         THIS same batch (a flap where the plus arm ran before the minus
         arm was seen), the retract arm's teardown empties `current` via
         the SAME Recycle-shaped op A1's swap uses (destroy the dirty
         `current`, hand out a fresh empty one from table_pool) — it is
         NOT a new primitive, it is Recycle invoked mid-band instead of
         at Seal. `frozen` is NEVER touched by this — only `current` is
         ever recycled/rebuilt, `frozen` is write-once-per-epoch at the
         swap (A1 :1184).
      3. RE-DEMAND ARM fires next: sees a FRESH EMPTY `current` (from
         step 2's teardown) and materializes the Rederive result into it
         exactly as an ordinary birth (A3's plus-arm case) — no different
         from a from-scratch materialize, because `current` is already
         empty going in.
      4. Band (b) publish reads old_r=frozen->Find(r), new_r=
         current->Find(r) — SAME partition as any other epoch (A2): rows
         present in both old batch-start membership and the flap's final
         re-derived set net (T,T)=unchanged; rows dropped by the flap net
         (T,F)=-old; rows newly present net (F,T)=+new. The flap is
         invisible to band (b) except through its NET effect on `current`
         — exactly the "net per batch" behavior the rest of the DR-IR
         already assumes for message batches (OQ3, the outer CLAUDE.md
         "explicit message batches net with SET semantics" invariant).
    So: the retract arm is the one that empties `current` (via Recycle,
    same op as the Seal-time swap's third leg, just invoked mid-band); the
    re-demand arm always sees a fresh empty `current`; `frozen` is
    untouched until Seal. The rebuild the re-demand arm performs is a
    plain birth-shaped rebuild into an empty table — no new partition
    case, no new mechanic beyond "Recycle can also fire mid-band on the
    retract arm, not only at Seal." Replace A3 :1298-1300's "or is emptied
    now if this is a same-epoch re-demand" with this walk (or a pointer to
    it).

  N1 (NIT, wording — against A1, A3, A4) — "Recycle" does not mean pool
    reuse; there is no free-list.
    Round-2 (R4, StateCell.h:595-624) confirmed the StateCell pool idiom
    (MakeVec/FreeMembership) only GROWS the pool (MakeVec: allocate +
    placement-new + pool.Add) and frees ONLY at store teardown
    (FreeMembership) — there is no dequeue-a-free-slot / hand-back-a-spare
    operation today. Every "Recycle" / "drop back to table_pool [for
    reuse]" phrase in A1 (:1228, :1232, :1254), A3 (:1306-1307, "the
    StateCell.h:560-566 pool idiom"), and A4 (:1319, :1327, :1328, :1332,
    :1335-1336) is READ AS: destroy the stale Table (`~Table`, freeing its
    slots array back to the allocator) and construct a fresh empty one via
    `MakeTable(table_pool)` (allocate + placement-new + pool.Add) — i.e.
    teardown-and-reconstruct, NOT reuse-from-a-free-list. This needs no new
    Runtime primitive (today's Allocator + the existing MakeVec-shaped
    pattern suffice) but the word "Recycle" oversells it as pooled reuse.
    No section is renumbered or rewritten; going forward, read every
    "Recycle(tmp)" / "drop back to table_pool" occurrence in A1/A3/A4 as
    shorthand for teardown+reconstruct-via-MakeTable, stage (b)'s actual
    mechanism. The deferred in-place `Table::Reset()` (A3 :1309-1311, A4
    :1331-1336) is UNAFFECTED by this correction and remains what it always
    was: a genuinely new Runtime primitive, held behind the A4 MEASURE-
    FIRST bar with its own counter seam (a per-instance Recycle counter
    under `-DDRLOJEKYLL_BENCH_COUNTERS`, A4 :1327). Only stage (b)'s
    DEFAULT path (teardown+reconstruct) was mis-named; the priced,
    deferred alternative was already correctly described as a new
    primitive.

  N2 (NIT, against A5 / §4.1) — the witness's add-only input is add-only
    by construction; say so and exhibit it.
    A5 (:1367-1373) asserts the landing witness's `edge` input is amended
    to be add-only but does not show why — and §4.1's `.dr` text is
    unchanged from before A5 was written. Round-2 confirmed (its §2, "DOES
    THE AMENDED WITNESS .dr REALLY HAVE ADD-ONLY INPUT?") that NO .dr edit
    was actually needed: a plain `#message` (one with no `@differential`
    pragma) is ALREADY monotone — a `#message` receive is add-only unless
    the message is explicitly marked `@differential` (deletion-capable);
    "a message carries adds" is not the same claim as "the message's table
    is differential." §4.1's witness already reads exactly this way:
        #message add_edge(i32 From, i32 To).
        edge(From, To) : add_edge(From, To).
    `add_edge` carries no `@differential` pragma, so its receiving table's
    `TableIsDifferential` (DR.cpp:132/148, A5's fence predicate) is FALSE
    — `edge` is monotone as written, and A5's fence (:1354-1360) PASSES on
    §4.1 unmodified. State this plainly in place of A5's "the witness .dr
    is AMENDED so edge is add-only" (:1370): §4.1's `#message add_edge`
    is a plain, non-`@differential` message, hence already add-only/
    monotone by the DR-IR's own classification (TableIsDifferential==
    false) — the fence passes on the witness AS WRITTEN, with no .dr edit
    required.
