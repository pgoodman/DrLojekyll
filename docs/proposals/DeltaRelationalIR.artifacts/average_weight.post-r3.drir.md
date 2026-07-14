# average_weight.post-r3.drir — hand-written target DR-IR artifact (pre-generalization)

Written 2026-07-14 at the delta-relational-IR epoch's design stage
(ledger: docs/proposals/DeltaRelationalIR.md §5-§6). The emitted CF-IR
identity targets are the pinned program.ir files in this directory
(regenerable: `drlojekyll <case>.dr -ir-out ...`, branch delta-relational-ir).

========================================================================
POST-R3 DR-IR ARTIFACT — data/examples/average_weight.dr
(design target; there is NO emitted CF-IR for this case — the CF build
rejects AGG at Build.cpp:1024-1027 and KVINDEX at :1028-1032. This is the
DR-IR module R3's construction SHOULD produce.)
========================================================================

--- 0. DATAFLOW SHAPE THIS LOWERS FROM (as re-derived; cite Query.h) ---

Program (average_weight.dr, lines 16-31):
  M add_edge/3  --(SELECT)-->  proj(From,To)         -> edge/2      [KVINDEX-free, monotone-derived]
                --(SELECT)-->  proj(From,To,Weight)  -> edge_weight/3
  edge_weight is a QueryKVIndex (Query.h:909): key cols = (From,To),
    value col = Weight, NthValueMergeFunctor(0) = new_weight_i32
    (mutable(new_weight_i32), line 12). *** edge_weight is itself a
    degenerate aggregate (AggregatingFunctors §2 last bullet). ***
  node/1: MERGE(proj_From(edge), proj_To(edge))      -> node/1
  AGG_S: QueryAggregate (Query.h:646), functor sum_i32
     GroupColumns   = {X}                 (Query.h:655)
     ConfigColumns  = {}                   (sum_i32 has no constructor args; :656)
     SummaryColumns = {AX_Weight->Sum}     (:657)
     input view     = edge_weight/3, InputGroupColumns={To=X},
                      InputAggregatedColumns={Weight=AX_Weight} (:692-694)
  AGG_C: QueryAggregate, functor count_i32
     GroupColumns={X}, Config={}, Summary={BX_Weight->Count}
     input view     = edge_weight/3 (same source; two DISTINCT agg views)
  JOIN_XC: JOIN(AGG_S, AGG_C) on pivot X  -> (X, Sum, Count)
  MAP_DIV: MAP div_i32 (bound Sum, bound Count, free Avg) @range(.)
  INSERT  -> average_incoming_weight/2 (X, Avg)   [the query relation]

Differential status: add_edge is a #message; edge/edge_weight are
differential (deletable via netted batches, OQ3). Therefore AGG_S,
AGG_C, edge_weight-KVINDEX, JOIN_XC, MAP_DIV, and the query table are
ALL differential. Every aggregate here is over DIFFERENTIAL input. None
is on a recursive cycle (Stratify places AGG strictly above its input
stratum, AggregatingFunctors §2 STRATIFICATION) => every GROUP_UPDATE
folds class kNonRecursive, sole deriver (§2 PHASE-FINAL READS).


--- 1. DR-IR MODULE (hand-written; §5 vocabulary + new AGG family) ---

drir module average_weight {

  ; ---- TABLES (unchanged §5-era objects; DataTable / DiffTable) ----
  table edge_weight   diff  key(From,To) val(Weight)   ; KV-index-backed (see §7)
  table edge          diff  cols(From,To)
  table node          diff  cols(X)
  table agg_S         diff  cols(X, Sum)     sole_deriver
  table agg_C         diff  cols(X, Count)   sole_deriver
  table join_XC       diff  cols(X, Sum, Count)
  table avg_iw        diff  cols(X, Avg)     ; query relation

  ; ---- STATE-CELL STORES (NEW runtime object; see §6/§7) ----
  statecell S_edge_weight  key(From,To)  algebra=@recompute(new_weight_i32)   ; KV merge
  statecell S_sum          key(X)        algebra=@invertible(sum_i32)
  statecell S_count        key(X)        algebra=@invertible(count_i32)

  ; ==================================================================
  ; STRATUM 0 — ingest edge_weight (KV-index = degenerate aggregate)
  ; ==================================================================
  ; add_edge -> edge_weight upsert. Two-level group-by degenerates to
  ; one level: group = key (From,To); config = {}; summary = the single
  ; value col merged by new_weight_i32.
  GROUP_UPDATE(agg=KV new_weight_i32, table=edge_weight, state=S_edge_weight,
               group=(From,To), config=(), summary=(Weight),
               class=kNonRecursive) {
    ; INPUT FRONTIER READS — over the message batch's netted delta of the
    ; SELECT proj(add_edge). add_edge is a message => its rows arrive as
    ; explicit +/- (OQ3 SET netting). Predicate matrix rows used:
    ;   input + delta  : source proj row read kNetAdded  (matrix §4 A_s)
    ;   input - delta  : source proj row read kNetDeleted (matrix §4 D_s)
    frontier_in FROM proj(add_edge) SIGN both {
      +row -> S_edge_weight[(From,To)].fold(+1, Weight)   ; merge new/old cell
      -row -> S_edge_weight[(From,To)].fold(-1, Weight)   ; @recompute: re-merge survivors
      touch (From,To)
    }
    emit_touched {                        ; one-net-pair-per-group contract
      new := S_edge_weight[g].emit()      ; = merged Weight
      old := S_edge_weight[g].old()       ; sealed cell value (see §6)
      if new != old:
        UPDATECOUNT(-, edge_weight, (g.From,g.To, old), kNonRecursive)  ; retract old row
        UPDATECOUNT(+, edge_weight, (g.From,g.To, new), kNonRecursive)  ; assert new row
    }
  }
  ; then edge_weight's EXISTING acyclic machinery, verbatim §5 vocab:
  CLAIM_DRAIN(edge_weight, +) ; CLAIM_DRAIN(edge_weight, -)
  FRONTIER_FILTER(edge_weight, -)  ; gate kNetDeleted -> kNetRemovals (matrix §4)
  FRONTIER_FILTER(edge_weight, +)  ; gate kNetAdded   -> kNetAdditions

  ; edge / node built by ordinary §5 SEED_FOLD arms from proj(add_edge)
  ; and edge (omitted for brevity; identical to any monotone-derived
  ; differential relation — SEED_FOLD(rule, delta_pos, sign, kNonRecursive)).

  ; ==================================================================
  ; STRATUM 1 — the two aggregates (SUM, COUNT) over edge_weight
  ; ==================================================================
  ; Both read edge_weight's PHASE-FINAL frontiers (stratum 0 committed its
  ; frontier filters). AggregatingFunctors §2: "reads its input stratum
  ; after that stratum's frontiers are final."

  GROUP_UPDATE(agg=sum_i32, table=agg_S, state=S_sum,
               group=(X), config=(), summary=(AX_Weight),
               class=kNonRecursive) {
    ; INPUT FRONTIER READS — this is the §2 AGG-UPDATE loop
    ; "for {row,±} in input net frontiers". The delta ranges over
    ; edge_weight's net frontiers, PRODUCED by stratum 0's FRONTIER_FILTERs:
    ;   - arm : delta over edge_weight kNetRemovals   (matrix §4 D_s output)
    ;   + arm : delta over edge_weight kNetAdditions  (matrix §4 A_s output)
    ; The row's OTHER (non-delta) columns need no read here: the aggregate
    ; input has a SINGLE source view, so there is no join partner and thus
    ; NO position-keyed InNew/InI reads. g = (To=X) projected from the row;
    ; AX_Weight = Weight projected from the row.
    frontier_in FROM edge_weight SIGN both {
      +row(From,To,Weight) -> S_sum[(To)].fold(+1, Weight); touch (To)
      -row(From,To,Weight) -> S_sum[(To)].fold(-1, Weight); touch (To)
    }
    emit_touched {
      new := S_sum[g].emit()          ; sum_i32 @invertible: running sum
      old := S_sum[g].old()
      if new != old:
        UPDATECOUNT(-, agg_S, (g.X, old), kNonRecursive)
        UPDATECOUNT(+, agg_S, (g.X, new), kNonRecursive)
    }
  }
  CLAIM_DRAIN(agg_S, +) ; CLAIM_DRAIN(agg_S, -)
  FRONTIER_FILTER(agg_S, -) ; FRONTIER_FILTER(agg_S, +)

  GROUP_UPDATE(agg=count_i32, table=agg_C, state=S_count,
               group=(X), config=(), summary=(BX_Weight),
               class=kNonRecursive) {
    frontier_in FROM edge_weight SIGN both {
      +row(From,To,Weight) -> S_count[(To)].fold(+1, Weight); touch (To)
      -row(From,To,Weight) -> S_count[(To)].fold(-1, Weight); touch (To)
    }
    emit_touched {
      new := S_count[g].emit()        ; count_i32 @invertible: running count
      old := S_count[g].old()
      if new != old:
        UPDATECOUNT(-, agg_C, (g.X, old), kNonRecursive)
        UPDATECOUNT(+, agg_C, (g.X, new), kNonRecursive)
    }
  }
  CLAIM_DRAIN(agg_C, +) ; CLAIM_DRAIN(agg_C, -)
  FRONTIER_FILTER(agg_C, -) ; FRONTIER_FILTER(agg_C, +)

  ; NOTE — SHARED INPUT SCAN: agg_S and agg_C read the SAME edge_weight net
  ; frontiers. The two frontier_in loops MAY be fused into one pass over the
  ; edge_weight net frontier (folding both S_sum and S_count per row) — this
  ; is a lowering choice, not a vocabulary requirement. See gap G-6.

  ; ==================================================================
  ; STRATUM 2 — JOIN(agg_S, agg_C) on X, then div MAP, then query insert
  ; ==================================================================
  ; Ordinary §5 differential JOIN over two differential inputs, NO aggregate
  ; vocabulary. Both agg tables now phase-final. Pivot = X.
  SEED_FOLD(rule=avg, delta_pos=agg_S, sign=+, class=kNonRecursive) {
    ; delta over agg_S kNetAdditions; partner agg_C read InI  (matrix §4:
    ; lower j>i -> kInI; here agg_C is the same-stratum-later position);
    ; join emits (X,Sum,Count) -> MAP_DIV(Sum,Count->Avg) -> fold avg_iw
    ACCESS(agg_C, bound(X), pred=kInI) -> (Count)
    MAP div_i32(Sum,Count->Avg)
    UPDATECOUNT(+, avg_iw, (X,Avg), kNonRecursive)
  }
  SEED_FOLD(rule=avg, delta_pos=agg_S, sign=-, class=kNonRecursive) {
    ACCESS(agg_C, bound(X), pred=kInI) -> (Count)
    MAP div_i32(Sum,Count->Avg)
    UPDATECOUNT(-, avg_iw, (X,Avg), kNonRecursive)
  }
  SEED_FOLD(rule=avg, delta_pos=agg_C, sign=+, class=kNonRecursive) {
    ACCESS(agg_S, bound(X), pred=kInNew) -> (Sum)   ; lower j<i -> kInNew
    MAP div_i32(Sum,Count->Avg)
    UPDATECOUNT(+, avg_iw, (X,Avg), kNonRecursive)
  }
  SEED_FOLD(rule=avg, delta_pos=agg_C, sign=-, class=kNonRecursive) {
    ACCESS(agg_S, bound(X), pred=kInNew) -> (Sum)
    MAP div_i32(Sum,Count->Avg)
    UPDATECOUNT(-, avg_iw, (X,Avg), kNonRecursive)
  }
  CLAIM_DRAIN(avg_iw, +) ; CLAIM_DRAIN(avg_iw, -)
  FRONTIER_FILTER(avg_iw, -) ; FRONTIER_FILTER(avg_iw, +)

  ; ==================================================================
  ; COMMIT (per differential table, table order; §4 COMMIT SWEEP)
  ; ==================================================================
  COMMIT_SWEEP(edge_weight) ; COMMIT_SWEEP(edge) ; COMMIT_SWEEP(node)
  COMMIT_SWEEP(agg_S) ; COMMIT_SWEEP(agg_C)
  COMMIT_SWEEP(join_XC) ; COMMIT_SWEEP(avg_iw)
  ; *** ALSO: seal every state cell (see §6) ***
  STATE_SEAL(S_edge_weight) ; STATE_SEAL(S_sum) ; STATE_SEAL(S_count)
}


--- 2. THE KV-INDEX FORM (pairwise_average_weight.dr, instructive) ---

pairwise_average_weight.dr has NO `over (){}` aggregate — only the
mutable(new_weight_i32) on edge_weight (line 9). It is the KV-index-ONLY
program, and its body is a self-JOIN of edge_weight on the To column with
A!=B then add/div MAPs. So it exercises exactly the DEGENERATE aggregate
(GROUP_UPDATE for edge_weight, identical to stratum 0 above) feeding an
ORDINARY §5 differential self-join — NO second aggregate family needed.
This confirms KV-index = one GROUP_UPDATE with group=key, config=(),
summary=value, algebra=the merge functor. R3 does NOT get a separate
KVINDEX lowering; the KVINDEX dataflow node desugars to GROUP_UPDATE
(AggregatingFunctors §4.1 recommendation (a): keep mutable() as sugar).

  drir module pairwise_average_weight {
    table edge_weight diff key(From,To) val(Weight)
    statecell S_edge_weight key(From,To) algebra=@recompute(new_weight_i32)
    GROUP_UPDATE(KV new_weight_i32, edge_weight, S_edge_weight,
                 group=(From,To), config=(), summary=(Weight), kNonRecursive)
      { ...identical to stratum 0 above... }
    CLAIM_DRAIN/FRONTIER_FILTER(edge_weight, ±)
    ; body = ordinary §5 differential self-JOIN of edge_weight×edge_weight
    ; on To=X, with A!=B TUPLECMP, add_i32/div_i32 MAPs -> paw table.
    ; group_ids/CSE self-join caveat (R4, DeltaRelationalIR §5) applies —
    ; not an aggregate concern.
    SEED_FOLD(rule=paw, delta_pos=edge_weight, sign=±, kNonRecursive){...}
    COMMIT_SWEEP(...) ; STATE_SEAL(S_edge_weight)
  }


--- 3. NEW vs REUSED VOCABULARY (precise) ---

R3 ADDS (three new operator/object kinds):
  (1) GROUP_UPDATE(agg, table, state, group, config, summary, class)
      — one operator per QueryAggregate/QueryKVIndex view. Carries:
        - a `frontier_in` block: which input view(s), which SIGN arms, the
          per-row fold direction into the state cell, the touch of g.
        - an `emit_touched` block: the one-net-pair-per-group output
          contract (retract old, assert new) expressed as TWO ordinary
          UPDATECOUNT folds of class kNonRecursive into the agg's OWN table.
        This is the ONLY genuinely new region kind. Its emit_touched body
        is PURE reuse (UPDATECOUNT already exists, Program.h:629).
  (2) statecell declaration + STATE_SEAL operator — the per-AGG keyed
      state-cell store (§6) and its commit-time seal (mirror of Table.Seal,
      Table.h:277 / DiffTable Commit kInI advance, Table.h:514-546).
  (3) the algebra attribute on GROUP_UPDATE/statecell
      (@invertible/@commutative/@idempotent/@recompute, AggregatingFunctors
      §3) — an ATTRIBUTE that selects the fold/unfold/emit lowering; not a
      new region.

R3 REUSES verbatim (the whole tail rides existing machinery — this is the
  point of "aggregate outputs are ordinary differential rows, sole deriver,
  class kNonRecursive"):
  - UPDATECOUNT (both emit_touched folds)         Program.h:629
  - CLAIM_DRAIN / RETIRE                          DR §5 vocab
  - FRONTIER_FILTER(table, sign)  (kNetDeleted->kNetRemovals,
      kNetAdded->kNetAdditions)                   matrix §4, Program.h:620-621
  - COMMIT_SWEEP                                   §4 COMMIT SWEEP
  - CLAIM gates TryClaimDel(C_nr<=0)/TryClaimAdd(total>0)  matrix §4 (F17)
  - ACCESS / SEED_FOLD for the downstream JOIN/MAP  DR §5 vocab (no agg)
  - the ten MembershipPredicate reads              Program.h:611-622
  The agg table is a plain DiffTable; nothing about claim/frontier/commit
  changes. GROUP_UPDATE is the SOLE producer of UPDATECOUNTs into that
  table, so there is exactly one deriver, always kNonRecursive.


--- 4. WHAT THE STATE-CELL STORE NEEDS FROM THE RUNTIME (§6) ---

Beside the DiffTable, one new runtime container per AGG:

  StateCellStore<Key, Blob>:
    - Keyed(group,config)-addressed dense store. Engine lays out Blob
      columnarly by dense group id (AggregatingFunctors §3 ENGINE-OWNED
      STATE). For @recompute algebra it may instead hold the group's
      membership (ids into the input table) and re-run emit on touch.
    - fold(key, sign, summary_cols)   : @invertible -> O(1) fold/unfold;
      @recompute -> accrue membership, defer to emit.
    - emit(key) -> current summary value(s).
    - old(key)  -> the SEALED (batch-start) summary value. *** This is the
      new requirement: like kInI on a Table (Table.h:249, id<sealed), the
      cell must retain a batch-start snapshot so emit_touched can compute
      new!=old exactly. AggregatingFunctors §2 "commit-time sealing like
      kInI." ***
    - Seal()    : advance old := current for every touched cell; clear the
      touched set. Mirror of Table::Seal (Table.h:277) and DiffTable's
      Commit kInI advance (Table.h:544).
    - a touched-group set (mirror of DiffTable::touched, Table.h:313) so
      emit_touched iterates only changed groups (sort-unique — AGG-UPDATE
      §2 "for g in touched groups (sort-unique)").

  What it does NOT need: signed derivation counters, kDel/kAdd scratch
  flags, claim frontiers, indices. All of that lives on the agg's DiffTable
  and is reached through the emit_touched UPDATECOUNTs. The cell is standing
  state; presence/frontier semantics stay on the table (§2 "its own outputs
  are ordinary differential rows"). Two seal points must be kept coherent:
  STATE_SEAL(cell) and COMMIT_SWEEP(table) both run at epoch end; the cell
  seal must land AFTER emit_touched has fired all pairs but the ORDER
  relative to the table's Commit is only constrained by: old() must be read
  by NEXT epoch's emit_touched, so seal-after-emit within THIS epoch.


--- 5. WHERE §2 AGG-UPDATE PSEUDOCODE MEETS THE TEN-PREDICATE MATRIX ---

§2's AGG-UPDATE loop head:
  "for {row,±} in input net frontiers (net_removals => -, additions => +)"
maps EXACTLY onto the matrix's FRONTIER-FILTER outputs of the INPUT
stratum:
  - the + arm ranges over the input table's kNetAdditions vector, produced
    by A_s (matrix §4 FRONTIER FILTERS: "A_s gate kNetAdded -> kNetAdditions").
  - the - arm ranges over kNetRemovals, produced by D_s (gate kNetDeleted).
So the two membership predicates the agg's INPUT FRONTIER READS consume are
kNetAdded and kNetDeleted (Program.h:620-621; matrix §4 E-14 — these are
first-class BECAUSE the frontier filters and now GROUP_UPDATE read them).
This is the SAME consumption shape as a CROSSOVER's frontier vectors
(matrix §4: "- arm over negated kNetAdditions ... + arm over kNetRemovals")
and PRODUCT arms — GROUP_UPDATE is a third frontier-vector consumer of the
input stratum, structurally a sibling of CROSSOVER/PRODUCT_ARM.

CRUCIAL DIFFERENCE from CROSSOVER/PRODUCT: those have a JOIN partner and so
also do position-keyed InNew(j<i)/InI(j>i) NON-delta reads (matrix §4
PRODUCT arms). GROUP_UPDATE's frontier_in has a SINGLE source view (the
summarized relation) and hence NO position-keyed partner read at all — the
group/summary columns are projections of the delta row itself. The only
matrix predicates GROUP_UPDATE uses on INPUT are kNetAdded/kNetDeleted.

On its OWN OUTPUT side, emit_touched's two UPDATECOUNT folds then feed the
agg table's ordinary CLAIM gates (TryClaimDel C_nr<=0 / TryClaimAdd total>0,
F17) and its FRONTIER_FILTERs (kNetDeleted->kNetRemovals, kNetAdded->
kNetAdditions) — which is precisely what the DOWNSTREAM JOIN_XC then reads
(stratum 2 SEED_FOLDs above read agg_S kNetAdditions and agg_C InI/kInNew).
So the ten-predicate matrix is used TWICE per aggregate: kNetAdded/kNetDeleted
on the input (consumed by GROUP_UPDATE) and the full seed/fixpoint palette
on the output (consumed by the next stratum's SEED_FOLDs) — with the agg
table being an ordinary differential table in between.


--- 6. ONE-NET-PAIR-PER-GROUP vs CLAIM GATES (the §2 critique point) ---

emit_touched emits at most ONE (-old,+new) pair per touched group per epoch,
REGARDLESS of how many input rows folded into that group (§2 OUTPUT CONTRACT:
"k touches in a batch still emit one pair"). This is safe against the claim
gates because:
  - if new==old the group emits nothing (netting composes, §2; a group
    touched only by an annihilated adds∩removes pair, OQ3, sees no state
    change).
  - if new!=old, the -old fold hits an in-I row (C_nr was >0 -> crossing on
    C_nr<=0 -> TryClaimDel proceeds), the +new fold hits an absent/new row
    (total 0->1 -> TryClaimAdd proceeds). Standard acyclic pair; the F17
    dequeue re-test is satisfied. Since class=kNonRecursive and the agg is
    the SOLE deriver, no other UPDATECOUNT can perturb these two counters
    within the epoch, so the pair is a clean crossing pair with no phantom
    (the §5.1.1 phantom class needs two same-stratum body atoms — GROUP_UPDATE
    has one input position, so no phantoms). Emit-seed-before-drain ordering
    (the pair's folds precede CLAIM_DRAIN) matches the crossover/product
    seed-before-drain rule (matrix §4).


========================================================================
END ARTIFACT
========================================================================

## Lowering trace (DR op -> emitted region, the R2 identity contract)

There is NO emitted program.ir for average_weight.dr (the CF build rejects
AGG at lib/ControlFlow/Build/Build.cpp:1024-1027 and KVINDEX at :1028-1032;
the region-dispatch stub asserts at Build.cpp:955-956 "TODO(pag): Aggregates").
So the "identity target" is the CF-IR region set that R3's lowering must
PRODUCE, matched to existing runtime primitives. Per DR-IR op:

GROUP_UPDATE.frontier_in  -> a VECTORLOOP over the input DiffTable's
  net-frontier vector (kNetAdditions / kNetRemovals), each iteration a
  CHECKMEMBER-free scan (the vectors are already filtered). The per-row fold
  is a call into StateCellStore::fold — a NEW runtime call with no existing
  CF region; closest existing analog is UPDATECOUNT's inline fold shape
  (Program.h:629-659) but into the cell store, not a DiffTable counter. This
  is the one op needing a genuinely new lowering (VECTORLOOP body calling
  cell.fold). Input vectors are exactly what FRONTIER_FILTER of the input
  stratum produced (Stratum.cpp frontier-filter emission; matrix §4 D_s/A_s).

GROUP_UPDATE.emit_touched -> a VECTORLOOP over the cell store's touched-group
  set (sort-unique), whose body is TWO ProgramUpdateCountRegion nodes
  (Program.h:629) IsAdd=false then IsAdd=true, DerivationClass=kNonRecursive
  (Program.h:647-649), Table()=the agg DiffTable, guarded by the new!=old
  test (a TUPLECMP-shaped region, Program.h SERIES/TUPLECMP). These are
  ORDINARY existing CF regions — identity target is the standard UPDATECOUNT
  emission (lib/ControlFlow/Build seed-fold path).

statecell + STATE_SEAL -> new runtime object StateCellStore beside the
  DataTable; STATE_SEAL lowers to a Seal() call mirroring Table::Seal
  (include/drlojekyll/Runtime/Table.h:277) and DiffTable Commit's kInI
  advance (Table.h:514-546). Emitted at the commit-sweep tail alongside
  COMMIT_SWEEP (Procedure.cpp per-table commit band; §2 pipeline
  "commit sweeps emitted DOWNSTREAM of BuildStratumPhases").

CLAIM_DRAIN / RETIRE / FRONTIER_FILTER / COMMIT_SWEEP on every agg table ->
  IDENTITY to the existing acyclic-table lowering in Stratum.cpp EMISSION
  band (:1936-2327; "acyclic tables {claim drains, frontier filters}") and
  Procedure.cpp commit sweeps. The agg table is a plain DiffTable
  (Table.h:301) — no new lowering.

Stratum-2 SEED_FOLD(rule=avg, ...) + ACCESS(agg_C/agg_S, ...) + MAP div_i32
  -> IDENTITY to ordinary differential JOIN seed emission (Stratum.cpp
  DiscoverBranches -> JoinEmission -> seed arms) plus a ProgramCallRegion /
  functor MAP for div_i32 (@range(.) pure functor, existing MAP lowering).
  The four sign×position arms are the standard seed schema (StackSafeNegation
  §5.1 seed table; matrix §4 SEED context) — agg_S kNetAdditions delta with
  agg_C read kInI, and agg_C delta with agg_S read kInNew.

edge / node SEED_FOLDs -> IDENTITY to existing monotone-derived differential
  relation emission. Nothing aggregate about them.

Net: exactly ONE new emitted primitive (StateCellStore + its fold/emit/old/
Seal calls, driven by two VECTORLOOPs). Everything from emit_touched's
UPDATECOUNTs onward lowers to the EXISTING region set — which is the whole
argument for aggregates being the inaugural family (they force the state-cell
story and nothing else new).

## Self-flagged vocabulary gaps (consolidated into ledger §6 vocabulary v2)

- G-1 STATE CELL is not a §5 vocabulary object at all. §5's ACCESS/SEED_FOLD/FIXPOINT_FIRE/CROSSOVER/PRODUCT_ARM/CLAIM_DRAIN/RETIRE/REDERIVE/FRONTIER_FILTER/FIXPOINT_ROUND/COMMIT_SWEEP are ALL table-and-counter operators. GROUP_UPDATE needs standing keyed non-counter state (the cell) that §5 cannot name. This is the deliberate gap AggregatingFunctors §4 predicted ('GROUP-UPDATE is the first operator whose delta semantics is NOT expressible as the §5.1 counter schemas'). The vocabulary must be EXTENDED with statecell as a first-class object, not just an operator.
- G-2 The frontier_in fold direction is a SIGNED fold into a cell, but the cell's crossing semantics differ from DiffTable's. A DiffTable fold crosses on total 0<->1; a state cell has NO presence crossing — folding -1 then +1 on a SUM cell changes the VALUE, which only becomes a table event at emit_touched. §5's UPDATECOUNT conflates 'fold' with 'crossing body'; GROUP_UPDATE separates them (fold accrues, emit_touched decides). The vocabulary needs to express 'value-fold without immediate crossing' distinctly from UPDATECOUNT.
- G-3 old() (the sealed batch-start cell value) has no §5 analog on the READ side. The ten MembershipPredicates are all boolean (present/InI/...). A cell needs a VALUED sealed read (old summary value), not a boolean membership predicate. kInI answers 'was this row present at batch start'; old() answers 'what was this GROUP's value at batch start'. The matrix cannot express a valued frozen read.
- G-4 @recompute algebra (AggregatingFunctors §3, the opaque fallback used here for new_weight_i32 whose invertibility is undeclared) forces the cell to hold GROUP MEMBERSHIP (ids into the input table) and re-run emit over survivors on every touch, rather than fold/unfold. This is a genuinely different physical lowering with a per-group-recompute-threshold (§3 PER-GROUP RECOMPUTE THRESHOLD) — the vocabulary has no way to express 'this operator's cost model chooses between incremental fold and full recompute per group'. The algebra attribute gestures at it but the lowering fork is unspecified.
- G-5 MIN/MAX (non-invertible, non-@recompute) is NOT exercised by either corpus file but AggregatingFunctors §5 demands hand-tracing a MIN retraction before code. The DR-IR as written assumes the cell can always answer old()/emit() cheaply; a MIN cell under retraction may need to re-scan the group (OQ12 'retraction-aware per-key state'). The vocabulary gap: GROUP_UPDATE has no attribute distinguishing 'retraction needs a rescan' from 'retraction is O(1) unfold', beyond the coarse @invertible/@recompute split.
- G-6 SHARED-INPUT FUSION is expressible only as a lowering note, not in the vocabulary. agg_S and agg_C read the identical edge_weight net frontier; the two frontier_in VECTORLOOPs SHOULD fuse into one pass folding both cells. §5 has no operator to say 'these two GROUP_UPDATEs share an input scan' — it would fall out of a CSE-like pass on GROUP_UPDATE input sources, which does not exist. Left as a lowering choice risks two full scans of the input frontier per epoch.
- G-7 KV-index desugaring is stated as a construction-time rewrite (KVINDEX node -> GROUP_UPDATE with group=key/config=()/summary=value/algebra=merge) but the vocabulary does not carry the mutable() provenance. If AggregatingFunctors §4.1 decision (a) lands (keep mutable() sugar), the DR-IR needs to record that edge_weight's GROUP_UPDATE came from a KVINDEX (single value cell, upsert semantics) vs a true over(){} aggregate, because the merge functor's REQUIRED declared algebra (else reject, §4.1) is checked differently. Currently indistinguishable in the DR-IR text.
- G-8 The touched-group set's sort-unique iteration order (§2 'for g in touched groups (sort-unique)') interacts with the CURSOR CONTRACTS note (CLAUDE.md: keyed-cursor enumeration order unspecified; drivers must sort keyed drains). emit_touched must produce deterministic output ordering for goldens, but the vocabulary does not pin whether STATE_SEAL / emit_touched ordering is canonical or driver-sorted. This is a golden-determinism gap the R3 oracle referee (per-group recompute) must resolve.

## Notes

Files read (all read-only, nothing written to repo): docs/proposals/DeltaRelationalIR.md (full), data/examples/average_weight.dr, data/examples/pairwise_average_weight.dr, docs/proposals/AggregatingFunctors.md (full), docs/proposals/StackSafeNegation.md §5.1-5.6, include/drlojekyll/DataFlow/Query.h:646-710/909-944 (QueryAggregate + QueryKVIndex), include/drlojekyll/ControlFlow/Program.h:600-660 (MembershipPredicate enum + UpdateCount region), include/drlojekyll/Runtime/Table.h:220-360/514-546 (Table/DiffTable, Seal, kInI, touched), lib/ControlFlow/Build/Build.cpp:1015-1053 (AGG/KVINDEX rejection pre-pass) + :955-956 (the assert stub).

KEY LOAD-BEARING FACTS for the orchestrator:
1. average_weight.dr is a DOUBLE aggregate case AND a KV-index case simultaneously: the two over(){} aggregates (sum_i32, count_i32) read from edge_weight, which is ITSELF a mutable()/KVINDEX = degenerate aggregate (line 12). So a single file exercises GROUP_UPDATE twice-nested-in-strata: KV GROUP_UPDATE at stratum 0 feeding two summary GROUP_UPDATEs at stratum 1. This is the strongest possible R3 fixture.
2. pairwise_average_weight.dr is the PURE KV-index case (no over(){}): confirms KV-index needs ONLY the degenerate GROUP_UPDATE + an ordinary §5 differential self-join. No separate KVINDEX lowering.
3. Both aggregate inputs are DIFFERENTIAL (add_edge is a message, edges deletable via OQ3 netting), so every GROUP_UPDATE has BOTH +/- input arms and class is always kNonRecursive (Stratify forbids in-SCC aggregation; AggregatingFunctors §2).
4. The SINGLE genuinely-new runtime primitive is the StateCellStore (with old()/emit()/fold()/Seal()/touched-set). Everything downstream of emit_touched's two UPDATECOUNTs is EXISTING machinery — this is precisely why AggregatingFunctors §4 nominates aggregates as the inaugural family: they force exactly the state-cell story and nothing else.
5. Input-frontier reads use ONLY kNetAdded/kNetDeleted (Program.h:620-621, the E-14 first-class predicates); GROUP_UPDATE is structurally a third frontier-vector consumer alongside CROSSOVER and PRODUCT_ARM (matrix §4) but WITHOUT their position-keyed InNew/InI partner reads (single input source, no join partner).
6. new_weight_i32's declared algebra is NOT annotated in the .dr (only @range on div/add); under AggregatingFunctors §4.1 the merge functor MUST carry declared algebra or be rejected — I modeled it @recompute as the safe fallback (gap G-4/G-7). The owner's mutable()-syntax decision (§4.1) and the golden policy are both still-open gates R3 depends on.
