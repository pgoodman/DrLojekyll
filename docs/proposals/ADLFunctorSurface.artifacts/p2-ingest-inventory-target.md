# P2 / FAMILY #4 — the ingest-fold inventory ↔ region-tree identity target (REVISED)

Artifact for `docs/proposals/ADLFunctorSurface.artifacts/`. Author: P2 designer
lane, epoch ADL/functor-surface (branch `adl-functor-surface`, off main
88058879 + the §1 ledger). REVISION of the REVISE-verdict design; judge
findings C1/C2/C3, H1/H2, M, L resolved below. Two message-heavy corpus
witnesses, IngestFold inventory in v3-spec §2.1 style, each op mapped to the
EXACT emitted region-tree lines (quoted verbatim) it must lower to. Generated
with `build/debug/bin/drlojekyll <case>.dr -ir-out` this session against HEAD.

The single load-bearing correction versus the original: the seam is a
DELETION-CAPABLE-RECEIVES-FIRST cut (clean tree cuts, no descent), lowered
from the phase `context.dr_flow` INSIDE BuildStratumPhases at the body-nest
point — NOT a hoist to Procedure.cpp:714, and NOT the monotone/publish
receives whose fold body hosts descent output. The monotone-receive and
eager-descent surface is a LATER stage under an explicit hole contract.

---

## §0. THE SEAM BOUNDARY IS A TREE CUT ONLY FOR DELETION-CAPABLE RECEIVES (C1)

The original design drew the boundary as {seed fold externalized, descent
stays hand-coded} and claimed it a clean cut for all four IF-classes. It is
NOT. The IR proves the non-cut for monotone receives.

CASE cf16_4, entry proc `^entry:7` (verbatim, this session):

    proc ^entry:7($param:9<i32>)
      seq
        vector-loop {@A:11} over $param:9<i32>
          update-count +nonrecursive {@A:11} in %table:4[i32]     ; IF-4 ingest fold
            if-crossed
              publish out_val/1(@A:11)                             ; DESCENT output, NESTED in the fold

The `publish` (Insert.cpp:60-69, reached via Procedure.cpp:125-126 →
BuildEagerInsertionRegions → BuildEagerInsertRegion) is a CHILD of the ingest
fold's own `if-crossed` body. LowerIngestFold would own the `vector-loop /
update-count / if-crossed` subtree; the hand-coded descent emits INTO that
`if-crossed` body. There is no separable downstream region and no described
re-parent mechanism. This is precisely the "boundary leaks state" failure.

THE FIX — stage the cutover DELETION-CAPABLE-RECEIVES-FIRST (the honest
R2-acyclic-first discipline). A deletion-capable receive is a CLEAN tree cut:
Procedure.cpp:85-90 emits `build_explicit_loop(add=true)` then
`build_explicit_loop(add=false)` and `continue`s — NO
`BuildEagerInsertionRegions` call, so the fold body is EXACTLY a single
`vector-append` into the receive table's queue, with no descent nested inside
it (NLS IR §1, L44-51: the fold body is just the queue append; the descent
lives in `^flow:86`).

STAGE-1 SEAM SCOPE (what cuts over in P2 proper):

  IN  — IF-2 / IF-3: the two per-polarity `build_explicit_loop` folds of a
        DELETION-CAPABLE receive (Procedure.cpp:43-90). Clean tree cut: fold
        body = {kCounter±(explicit); kVecAppend(kAddQueue|kDeleteQueue)},
        NOTHING nested below.

  OUT — IF-1 / IF-4: the MONOTONE receive fold (Procedure.cpp:92-127). Its
        fold body hosts the descent (BuildEagerInsertionRegions at :125),
        which emits either the net-additions append AT the fold (IF-1, NLS
        L43) OR a nested publish / relation descent (IF-4, cf16_4). These
        stay HAND-CODED in stage 1.

The monotone-receive + eager-descent surface cuts over in a LATER stage
(§6, deferred), which requires LowerIngestFold to own the fold region and
return the `if-crossed` body cursor into which the still-hand-coded descent
emits — a HOLE CONTRACT with a fill-exactly-once validator. That stage is NOT
this diff; it is called out so the boundary is honest.

CONSEQUENCE for the IF-4 effect set (the judge's internal-contradiction
finding): IF-4 is OUT of the stage-1 seam, so its effect set is NOT modeled
as a lowered ingest op this diff. When it IS modeled (§6), its effect set is
`[kCounter(+1, kNonRecursive)]` ONLY — the `publish` is a WALK effect emitted
by the descent, NOT an ingest-seed effect. The original `[kCounter; kPublish]`
was wrong; a fold whose publish stays hand-coded cannot carry kPublish.

---

## §1. THE TWO WITNESSES (verbatim IR)

CASE 1 — negate_lower_strata (a `@differential` message `block` with BOTH
explicit loops; a monotone message `feed` whose eager descent raw→step→mid
feeds a CUT negate `out:mid,!blk`; the entry proc is PURELY ingest folds —
the negate gate is in `^flow:86`, the seam boundary). Tables: `%table:4[u64]`
= blk (DIFFERENTIAL); `%table:11[u64,u64]` = mid/raw/step (MONOTONE, feeds the
cut negate); `%table:7` = out. Entry proc `^entry:15` (verbatim):

    proc ^entry:15($param:17<u64,u64>, $param:22<u64>, $param:23<u64>)
      vector-define $net_additions:21<u64,u64>
      vector-define $add_queue:26<u64>
      vector-define $delete_queue:29<u64>
      seq
        par
          vector-loop {@K:19, @V:20} over $param:17<u64,u64>              ; IF-1 (OUT of stage 1)
            update-count +nonrecursive {@K:19, @V:20} in %table:11[u64,u64]
              if-crossed
                vector-append {@K:19, @V:20} into $net_additions:21       ; boundary append AT the receive fold
          vector-loop {@K:25} over $param:22<u64>                         ; IF-2 (IN — stage 1)
            update-count-explicit +nonrecursive {@K:25} in %table:4[u64]
              if-crossed
                vector-append {@K:25} into $add_queue:26                  ; clean body: append only
          vector-loop {@K:28} over $param:23<u64>                         ; IF-3 (IN — stage 1)
            update-count-explicit -nonrecursive {@K:28} in %table:4[u64]
              if-crossed
                vector-append {@K:28} into $delete_queue:29               ; clean body: append only
        vector-clear $param:17 ; vector-clear $param:22 ; vector-clear $param:23
        call ^flow:86($net_additions:21, $add_queue:26, $delete_queue:29)
        return-false

  - IF-1 (feed/2, +1, is_explicit=FALSE, target %table:11, queue_role=
    kNetAddition): OUT of the stage-1 seam. effects (when modeled, §6)
    [kCounter(+1, kNonRecursive); kVecAppend(kNetAddition)]. The net-additions
    append rides the receive fold's if-crossed HERE (NLS L43), BUT that is a
    property of NLS (feed/raw/step/mid all share %table:11, folded once at the
    receive by the already_added guard) — NOT a general guarantee (see §5 / the
    M-finding witness). Stays hand-coded stage 1.

  - IF-2 (block/1, +1, is_explicit=TRUE, target %table:4, queue_role=
    kAddQueue): IN. effects [kCounter(+1, kNonRecursive, explicit);
    kVecAppend(kAddQueue)] → the L44-47 tree (update-count-explicit
    +nonrecursive / if-crossed / vector-append into $add_queue:26). Clean cut.

  - IF-3 (block/1, -1, is_explicit=TRUE, target %table:4, queue_role=
    kDeleteQueue): IN. effects [kCounter(-1, kNonRecursive, explicit);
    kVecAppend(kDeleteQueue)] → the L48-51 tree (update-count-explicit
    -nonrecursive / if-crossed / vector-append into $delete_queue:29). Clean cut.

  NOT ingest folds (seam boundary): the param clears (L52-54), the flow call
  (L55), the negate gate in `^flow:86` (eager kInI, Negate.cpp:91-94), the
  net-batch in `^receive:block/1:78`.

CASE 2 — cf16_4 (monotone message `in_val` with descent to an INSERT STREAM
publication). `%table:4[i32]` = in_val/mid/keep model. Entry proc `^entry:7`
quoted in §0.

  - IF-4 (in_val/1, +1, is_explicit=FALSE, target %table:4, queue_role=kEmpty
    — no cut successor, not monotone-negated): OUT of the stage-1 seam.
    effects (when modeled, §6) [kCounter(+1, kNonRecursive)] ONLY — the
    `publish out_val/1` is the E-22 Insert.cpp:60-69 direct-PUBLISH branch
    (IsStream, no publish_vec), a WALK effect DOWNSTREAM of the descent,
    NESTED in IF-4's fold body. NOT an ingest-seed effect. Stays hand-coded
    stage 1; cf16_4's whole entry-proc walk is untouched by this diff (its
    sole receive is monotone-with-descent).

---

## §2. THE INGEST-FOLD INVENTORY (E-22 COMPLETE — retained, amended for scope)

`BuildIngestInventory` derives IF-1..IF-N INDEPENDENTLY from the Query (NEVER
copying ExtendEagerProcedure). Iterate `query.IOs()` (mirror Procedure.cpp:711),
skip io with empty `Receives()`, per receive:

  - target_table = `impl->view_to_model[receive]->FindAs<DataModel>()->table`
    (Procedure.cpp:50-51 / 98-99).
  - polarity / is_explicit / queue_role from `receive.CanReceiveDeletions()`:
      * DELETION-CAPABLE (STAGE-1 IN) → TWO descs:
          {sign=+1, is_explicit=TRUE, queue_role=kAddQueue,    stage1=TRUE}
          {sign=-1, is_explicit=TRUE, queue_role=kDeleteQueue, stage1=TRUE}
        Then STOP (no cut/descent — Procedure.cpp:89).
      * MONOTONE (STAGE-1 OUT) → ONE desc {sign=+1, is_explicit=FALSE,
        stage1=FALSE}; queue_role DERIVED by re-running the boundary predicate
        on the RECEIVE table (see §5 for the correctness caveat):
          kNetAddition iff (!TableIsDifferential(table) &&
                            (any_cut_succ(receive.Successors()) ||
                             context.monotone_negated_tables.count(table)));
          else kEmpty.
        any_cut_succ = ∃ succ ∈ receive.Successors():
          succ.CanReceiveDeletions() || succ.IsAggregate() || succ.IsKVIndex()
        (replicating Build.cpp:857-858). deriv_class =
        EmissionDerivClass(impl, context, receive) (kNonRecursive here).
  - publication branch (E-22, INVENTORIED for census completeness but a WALK
    effect, not a seed): if a successor reaches an INSERT view that IsStream(),
    record {stream_publish | publish_vec | commit_published} by replicating
    Insert.cpp:35-79 (context.publish_vecs / context.commit_published_view
    membership) as WALK metadata on the desc.

Every op carries a `stage1` bit. In stage 1 ONLY `stage1==TRUE` descs
(deletion-capable) become emitted kIngestFold ops that LOWER; `stage1==FALSE`
descs are still INVENTORIED and CENSUSED (so the count is complete) but their
lowering is a no-op (the hand-coded monotone walk still emits them).

§4 INVENTORY-COMPLETENESS TABLE (retained verbatim — the discovery MUST model
all of these even though the two targets don't quote them all):
monotone-no-frontier (kEmpty), monotone-net-additions (kNetAddition), diff-add
(kAddQueue), diff-remove (kDeleteQueue), INSERT direct publish (walk kPublish),
publish-vec accum (Insert.cpp:47-57, walk kVecAppend kMessageOutput),
commit-published early-return (Insert.cpp:42-44, no eager emit), eager negate
gate (Build.cpp:1048-1051 — NOT :1002, E-24a), eager MAP generate
(Build.cpp:1009-1017, post-P1 free-fn call).

BuildIngestInventory is ID-NEUTRAL (allocates no region/vector ids — a pure
derivation over query.IOs()×Receives()×view_to_model, same class as
BuildDRInventory's branch derivation). The queue-vec ids are minted at LOWER
time by TableDeltaVector, in the same (message,receive,polarity) order the
deleted build_explicit_loop minted them (§4).

---

## §3. WHERE THE FLOW IS BUILT, VALIDATED, AND LOWERED — ONE OBJECT (H2, C2)

The original design proposed hoisting BuildDRInventory to Procedure.cpp:711
(option A) and lowering ingest at :714, censusing the phase flow at :756. That
is TWO DRFlowGraph objects (the lowered ops ≠ the validated ops — H2) AND reads
an EMPTY `context.table_delta_vecs` at :711 (C2): the map is populated LAZILY
by the eager walk (TableDeltaVector, Build.cpp:644), so at :711 only
CreateDifferentialMessageVectors (:661) has run; the monotone queue-role
derivation at DR.cpp:2419-2425 / :2510 would read the empty map and mis-derive
`is_source` / `exp_sweep`. Both options are abandoned.

THE FIX — ONE flow, built once, lowered from the SAME object, at PHASE time.

Precedent (verbatim in the tree): `context.dr_flow` is built ONCE inside
BuildStratumPhases (Stratum.cpp:1924 BuildDRInventory), validated there
(:1938-1940), and stashed as `std::make_shared<DRFlowGraph>` at
Stratum.cpp:1948. `LowerCommitSweeps` then consumes THAT stashed flow,
dispatched from `PublishDifferentialMessageVectors` at Procedure.cpp:759 —
AFTER BuildStratumPhases at :756. The commit-sweep band lowers from the same
validated flow object it was censused on. Ingest follows this precedent
EXACTLY.

C2 RESOLUTION (map full at build site): BuildIngestInventory runs INSIDE
BuildStratumPhases, at Stratum.cpp:1924-ff, AFTER the eager walk (:714) has
already run and populated `context.table_delta_vecs`. The monotone queue-role
derivation reads a FULL map — the same map the census at DR.cpp:2419-2425
reads. No empty-map hazard. (For a STAGE-1 desc the queue-role is
kAddQueue/kDeleteQueue, derived purely from polarity — it does NOT read
table_delta_vecs at all; the map-read is only for the OUT monotone descs,
whose census correctness is proved in §5's R1e-phase cross-check.)

H2 RESOLUTION (one flow): the ingest ops are appended to `flow.ops` inside
BuildDRInventory (§3a), so the SAME DRFlowGraph that is censused (ValidateDROps
:1939) and stashed (:1948) carries them. LowerIngestFolds consumes
`context.dr_flow` — the validated object. Validated ops == lowered ops, by
construction, one object.

WHERE INGEST LOWERS IN THE EMITTED TREE (preserving the entry-proc order):
BuildStratumPhases nests the entry-proc body (the ingest walk) into a new
SERIES at Stratum.cpp:2035-2039, THEN appends the per-stratum phase series. The
emitted shape is `SERIES[ ingest_walk_body, stratum_0_seq, ... ]`. The stage-1
kIngestFold ops (deletion-capable) lower into a DR-owned region that takes the
place of the deleted build_explicit_loop output, emitted at the body-nest
point (§4). The still-hand-coded monotone walk (IF-1/IF-4 folds + descent)
continues to build `proc->body` at :714 as today; the DR-lowered
deletion-capable folds are spliced into the SAME `par` sibling group the
deleted build_explicit_loop populated (the `par` is created at
Procedure.cpp:712 before ExtendEagerProcedure; stage 1 removes the
deletion-capable calls from ExtendEagerProcedure and re-emits them from the DR
op — §4).

### §3a. kIngestFold CONSTRUCTION inside BuildDRInventory

Populate the reserved kIngestFold (DR.h:126), mirroring the eager NegateGate
block (DR.cpp:1650-1672 — the exact precedent: derive from the query, append
one DROp per discovery item to flow.ops). Append the ingest ops AFTER the eager
NegateGate block, BEFORE the FIXPOINT_ROUND shells (DR.cpp:1673-ff), in a
dedicated block. One DROp(kIngestFold) per IngestFoldDesc.

Fields added to DROp (DR.h:501-524 block): `ingest_message` (message identity),
`ingest_receive` (optional QueryView), `ingest_table` (TABLE*), `ingest_sign`
(int), `ingest_is_explicit` (bool), `ingest_role` (VecRole — the DR-side
spelling; see §9), `ingest_stage1` (bool).

EFFECT SETS (model DR.cpp:651-702), per op:
  - kCounter(counter_table=ingest_table, sign=ingest_sign, klass=
    kNonRecursive), is_explicit carried as the op's ingest_is_explicit
    attribute (the message-support-bit toggle, Procedure.cpp:36-42 —
    INGEST-specific, no GROUP_UPDATE analog);
  - kVecAppend(value_table=ingest_table, vec_role=ingest_role) IFF ingest_role
    != VecRole::kEmpty.

DEF-EDGES: for a deletion-capable (stage-1) op, the queue vec
(kAddQueue/kDeleteQueue) EXISTS by phase time (TableDeltaVector minted it
during the walk, and MintTableVec mints it for differential non-induction
tables — verified %table:4 in NLS owns kAddQueue/kDeleteQueue). Push op_idx
into `flow.vecs[TableVec(ingest_table, ingest_role)].defs` (A-4 multi-def,
mirror DR.cpp:707-709). For a monotone (OUT) op the kNetAddition vec is NOT
minted by the differential-only MintTableVec loop; the append parks via
TableDeltaVector at LOWER time — no DR def-edge (matches how the monotone
boundary works today, and consistent with the monotone op being lowered by the
hand-coded walk, not LowerIngestFold, in stage 1).

---

## §4. LOWERING ORDER — THE OLD WALK ORDER IS THE LOWERING DEFAULT (C3)

The original design keyed LowerIngestFolds off the linearizer Key with sign
`−before+` and table-id by TABLE* pointer. That INVERTS the emitted order and
REGROUPS by pointer. The emitted ingest order is:

  ADD(+1) BEFORE REMOVE(-1) per receive (Procedure.cpp:87-88:
    build_explicit_loop(add=true) then build_explicit_loop(add=false));
  receives in `io.Receives()` order; ios in `query.IOs()` order.

Confirmed in the NLS IR: `$add_queue` append (L47) precedes `$delete_queue`
append (L51). The DR band key (DR.cpp:2935 key_less) sorts `−before+` and by
TABLE* pointer — the OPPOSITE of the emitted order.

THE FIX — apply the R2 family-#1 lesson VERBATIM. That lesson is IN THE CODE
(Stratum.cpp:1035-1037): "Emission ORDER is the old driver's band walk realized
as a lowering default (B-9/B-14: tree-shape identity modulo an id-bijection —
NOT the raw pinned_order sign-major sort, which reorders product arms)."
LowerDRFlow iterates its OWN construction-order loops (branch order
Stratum.cpp:1438, join order :1475, Crossovers() construction order :1522),
NEVER pinned_order. The pinned Key is a VALIDATOR device (it proves the
dependence graph admits a valid linearization); it does NOT drive emission.

LowerIngestFolds ITERATION ORDER (the lowering default, stated concretely):

    for io in query.IOs():                       # query.IOs() order
      for receive in io.Receives():              # io.Receives() order
        for op in ingest_ops_of(receive)         # +1 before −1 (add-before-remove)
            where op.ingest_stage1:
          LowerIngestFold(op)                     # emit into the par sibling group

This reproduces Procedure.cpp:81-90's exact sibling order by CONSTRUCTION. It
does NOT iterate pinned_order. The lead-0 Key (below) is DEMOTED to a pure
validator-ordering device with NO emission role — exactly the status LowerDRFlow
gives the band key.

THE LEAD-0 KEY (validator-only, for V-LINEAR / V-LOOP / V-READY /
V-BAND-HAZARD to place ingest ops off-lattice):
  - op_stratum returns 0 (kIngestFold falls to `default: return 0u`,
    DR.cpp:2825-2827 — no new arm needed).
  - op_band returns 0 (default arm, DR.cpp:2863-2864).
  - op_table_id: add `op.ingest_table` to the ternary at DR.cpp:2870-2874.
  - Key: add a lead=0 arm ALONGSIDE the eager NegateGate (DR.cpp:2915-2916):
      if (op.kind==kIngestFold) return Key{0u, 0u, 0u, op_table_id(op),
                                           op.ingest_sign, oi};
    lead=0 so ingest sorts BEFORE all phase ops (lead=1) and commit (lead=2).
    The sign field carries ingest_sign; the ctor (oi) is the final tie-break.
    Because emission does NOT read this Key, its `−before+` sign order does NOT
    perturb the emitted +before− order — the two are decoupled exactly as
    LowerDRFlow decouples them for product arms. The Key is used only to check
    that ingest→drain RAW edges are key-monotone: ingest is lead-0, the drain
    lead-1, so every ingest→drain RAW is forward in the key.
  - V-READY / V-BAND-HAZARD off-lattice skip: extend the wo_offlattice /
    ro_offlattice test (DR.cpp:3415-3421) to include `op.kind==kIngestFold`,
    exactly like the eager NegateGate at :3416/:3419. A RAW ingest→phase-drain
    is a legitimate cross-band lead-0→lead-1 edge; since ingest is off-lattice
    the V-READY strictly-higher-stratum check is skipped, and the band key
    already orders lead-0 before lead-1 so V-BAND-HAZARD sees no same-scope
    inversion. seed_before_drain holds (ingest counter± seeds the queue the
    drain reads).

The queue-vec MINT ORDER (byte-identity, D1): LowerIngestFold calls
TableDeltaVector(ingest_table, kAddQueue/kDeleteQueue) in the SAME
(message,receive,polarity) order build_explicit_loop did. Because
LowerIngestFolds runs at PHASE time (§3) — AFTER the eager walk at :714 has
ALREADY minted every queue vec via TableDeltaVector's memoization
(Build.cpp:644) — the ingest queue vecs are ALREADY MINTED by the time
LowerIngestFolds runs, so LowerIngestFold REUSES the memoized vec (no new
mint), no mint-order perturbation. NB: the deletion-capable walk is DELETED in
the same diff, so its mints move to LowerIngestFold — but both mint in the
identical (message,receive,polarity) order and both run before any phase drain
reads the vec, so next_id order is preserved. VERIFY at cutover: diff next_id
assignment across the entry-proc region before/after (the single byte-identity
check, D1).

---

## §5. THE MONOTONE NET-ADDITIONS APPEND CAN MIGRATE ACROSS THE SEAM (M)

Build.cpp:868-893 documents (verbatim comment L870-872): "The append always
sits inside the table's fold crossing — either the fold THIS CALL just made, or
an ANCESTOR fold of the same table that this walk is nested under." The append
site is BuildEagerInsertionRegionsImpl (Build.cpp:888-893), reached at DESCENT,
guarded by the `already_added`/`last_table` fold-once machinery
(Build.cpp:708). Its POSITION depends on walk NESTING, not on which view is the
receive. NLS IF-1 happens to fold at the receive (feed/raw/step/mid all share
%table:11, folded once at the receive — NLS L41-43), but that is a PROPERTY OF
NLS, not a guarantee.

CONSEQUENCE: for a monotone message whose receive table is FIRST folded at an
INTERIOR view (receive-table != first-folded-model), the net-additions append
sits at a DEEPER fold, not the receive. If the seam claimed the SEED owns the
net-additions frontier in general, it would either double-append (seed emits
one AND the descent emits one) or drop it (seed emits none, descent's is
deleted with the walk).

THE FIX (two-pronged):

(1) STAGE 1 DOES NOT TOUCH THIS. Monotone receives (IF-1/IF-4) are OUT of the
    stage-1 seam (§0). The net-additions append is emitted by the STILL-
    HAND-CODED monotone walk (Build.cpp:888-893), wherever the walk nesting
    places it. Stage 1 neither claims the seed owns it nor deletes it. So the
    migration cannot cause a double-append or drop in stage 1 — the append
    stays exactly where the current code puts it.

(2) THE DEFERRED MONOTONE STAGE (§6) NEEDS A DIRECTED WITNESS before it can
    externalize IF-1. SPEC of the directed .dr shape (a monotone message whose
    receive table is first folded at an INTERIOR view):

        #message feed(u64 K, u64 V).
        #message sink(u64 K) @differential.   ; a cut/differential successor

        #local a(u64 K, u64 V).               ; interior view
        #local b(u64 K, u64 V).
        #local blk(u64 K).

        #query out(free u64 K, free u64 V).

        a(K, V) : feed(K, V).                 ; receive → a
        b(K, V) : a(K, V).                    ; a → b   (interior hop)
        blk(K)  : sink(K).
        out(K, V) : b(K, V), !blk(K).         ; the cut negate consumes b, NOT a

    Design intent: `a` and `b` on DISTINCT models, so the fold-once guard folds
    at `a`'s model at the receive, but the net-additions frontier the negate
    crossover reads is `b`'s model, folded at the INTERIOR `b` view during
    descent — moving the net-additions append off the receive. Whether the
    optimizer keeps `a`/`b` on distinct models MUST be VERIFIED by generating
    this case's IR and inspecting the `net-additions` append site (receive fold
    vs interior fold). If CSE collapses `a`/`b` to one model, strengthen the
    witness (a JOIN or projection forcing a model boundary) OR the deferred
    stage must prove absence for all reachable shapes. This witness is a
    PREREQUISITE of §6, NOT of stage 1.

    Until (2) is shown safe, §6 must NOT claim the seed owns the net-additions
    frontier in general; it must derive the append SITE from the actual
    fold-nesting (the walk's last-folded-model), or keep the net-additions
    append hand-coded even while externalizing the fold itself.

R1e-PHASE CROSS-CHECK for the OUT monotone queue-role (proving the derived
queue-role agrees with the walk's, C2 tail): the always-on validator (§7d)
recomputes, for every monotone receive, the queue_role BOTH from
BuildIngestInventory's boundary predicate AND from `context.table_delta_vecs`
membership (kNetAddition present iff the map holds a kNetAdditions vec for the
table). If they DISAGREE on any of the 164 cases → abort. This is the
independent-predicate-agrees-with-map cross-check the judge required, run at
phase time when the map is full.

---

## §6. THE DEFERRED MONOTONE / DESCENT STAGE (NOT THIS DIFF — the hole contract)

Externalizing IF-1/IF-4 (monotone receives whose fold body hosts descent)
requires LowerIngestFold to own the `vector-loop / update-count / if-crossed`
region and expose the if-crossed body as a HOLE the hand-coded descent fills:

  - LowerIngestFold emits the fold, returns the `if-crossed` body OP* cursor.
  - BuildEagerInsertionRegions (still hand-coded) is invoked with that cursor
    as `next_parent`, so the descent emits INTO the DR-owned fold body.
  - A validator asserts the hole is filled EXACTLY ONCE (no descent → the hole
    stays empty for kEmpty monotone-no-successor cases like a bare monotone
    receive; one descent subtree otherwise).
  - The net-additions append (§5) is emitted by the descent at its actual
    fold-nesting site, NOT claimed by the seed — until the M-witness proves the
    receive always owns it.

This stage rides the SAME census/validators; it flips IF-1/IF-4 descs' stage1
bit to TRUE and adds the hole contract. Called out here so the boundary is
honest and the follow-on scope pinned. NOT in P2 proper.

---

## §7. V-INGEST-EQUIV — DERIVED-vs-DERIVED CENSUS, NOT A TREE WALK (H1)

The original V-INGEST-EQUIV walked the emitted entry-proc region tree
(context.entry_proc->body) collecting 5-tuples, citing "the V-OLD-EQUIV
discipline R1 used." THERE IS NO SUCH PRECEDENT: grep of DR.cpp's validators for
entry_proc / ->body / VECTORLOOP / UPDATECOUNT / operation_regions returns
NOTHING (the sole `->body` hit is `r->body_ops`, a DR round-shell field —
DR.cpp:3281). Every existing validator (census, the retired V-OLD-EQUIV,
V-READY) is DERIVED-vs-DERIVED: ValidateDROps (DR.cpp:2408-2557) recomputes
expected op counts from impl/context/query and compares op counts + per-op keys
via `expect(kind, count)` — it NEVER touches the emitted tree.

THE FIX — rebuild V-INGEST-EQUIV as a DERIVED-vs-DERIVED census in the
ValidateDROps mold, folded into the E-25 census requirement:

  exp_ingest (recount INDEPENDENTLY, mirroring the exp_seed style at
  DR.cpp:2450-2467):

    exp_ingest = 0
    for io in query.IOs():
      for receive in io.Receives():
        if receive.CanReceiveDeletions():
          exp_ingest += 2        # +add, +delete (stage-1 IN)
        elif view_to_model[receive]->table != nullptr:
          exp_ingest += 1        # the monotone seed (stage-1 OUT, still counted)

  Then, in the expect() list (DR.cpp:2535-2542):
    expect(DROpKind::kIngestFold, exp_ingest, "ingest folds");

  PLUS per-op KEY comparison (the "per-op keys" half of the census discipline):
  recompute the MULTISET of per-op keys `(ingest_table*, ingest_sign,
  ingest_is_explicit, ingest_role, ingest_message)` from the same
  query.IOs()×Receives()×polarity derivation, and compare it order-free to the
  multiset of the SAME keys read off flow.ops' kIngestFold ops. Mismatch → the
  census-style fprintf+abort (DR.cpp:2528-2532). This is the E-22 completeness
  check (INSERT/publication sites inventoried as WALK metadata on the desc and
  compared too), done as a pure re-derivation — NEVER a tree walk.

This DROPS the region-tree-walk form entirely. It disambiguates nothing about
interior I8 folds (there is no tree to disambiguate) and is immune to the
Procedure.cpp:716-719 body-swap (it reads flow.ops, not the reparented
subtree). The E-25 structural validators stand.

STRUCTURAL VALIDATORS (E-25 — kIngestFold MUST NOT copy the GROUP_UPDATE census
gap; all fprintf+abort, survive NDEBUG):
  (a) EFFECT-SET TOTALITY — every kIngestFold has exactly {kCounter} or
      {kCounter, kVecAppend}; sign ∈ {-1, +1}; klass == kNonRecursive.
  (b) ONE-OP-PER-(message, receive, polarity) — no duplicate
      (ingest_message, ingest_receive, ingest_sign) key.
  (c) QUEUE-ROLE AGREEMENT — sign<0 ⇒ ingest_role == VecRole::kDeleteQueue;
      sign>0 ⇒ ingest_role ∈ {kAddQueue, kNetAddition, kEmpty}; AND a
      DIFFERENTIAL receive table's op parks into a Queue role
      (kAddQueue|kDeleteQueue) while a MONOTONE receive parks into
      kNetAddition|kEmpty (mirrors TableIsDifferential). Purely in DR-side
      VecRole terms — must NOT conflate VectorKind (see §9).
  (d) [§5] MONOTONE QUEUE-ROLE CROSS-CHECK — for each monotone (OUT) op, the
      boundary-predicate-derived kNetAddition/kEmpty AGREES with
      context.table_delta_vecs membership (the map holds a kNetAdditions vec for
      the table iff the predicate said kNetAddition). Both read at phase time
      (map full). Disagreement → abort.

While here, leave the missing GROUP_UPDATE/STATE_SEAL census (E-25 says that gap
is P4 residue) as a one-line TODO cite pointing at P4 — do NOT expand scope.

---

## §8. STAGE PLAN + GATES (R1/R2-precedent-cited; construct-alongside → cutover)

STAGE R1e (construct-alongside, NO emission change):
  BuildIngestInventory + kIngestFold construction in BuildDRInventory (§3a) +
  the DERIVED census + structural validators (§7) + the §5 cross-check. NOTHING
  lowers, NOTHING deletes. The ingest ops sit in flow.ops, are censused, keyed
  lead-0 off-lattice, but LowerIngestFolds is not yet dispatched. E-24 comment
  sweep lands here (below).
  GATE R1e: (1) full suite 164 PASS BYTE-IDENTICAL across all 4 modes (HARD
  ZERO-CHURN — a stdout diff is a bug, not a bless); (2) oracle + monotone
  sidecars byte-identical; (3) ctest 3/3; (4) all DR validators green INCLUDING
  the new census + structural validators + cross-check (must not abort on any
  of the 164); (5) the E-24 comment sweep landed. Reviewed as: derivation-only,
  no golden bless permitted.

STAGE CUTOVER (LowerIngestFolds dispatched at phase time + delete the
  DELETION-CAPABLE build_explicit_loop path, ATOMIC):
  - NEW LowerIngestFold + LowerIngestFolds in Stratum.cpp near LowerGroupUpdate
    (Stratum.cpp:1330). LowerIngestFolds iterates query.IOs()×Receives()×
    (+before−), lowering ONLY stage1 (deletion-capable) ops (§4), emitting the
    exact build_explicit_loop region tree (VECTORLOOP→UPDATECOUNT[explicit]→
    if-crossed→VECTORAPPEND into the reused memoized queue vec).
  - DISPATCH: called from BuildStratumPhases at the body-nest point
    (Stratum.cpp:2035-2039), from context.dr_flow, splicing into the par
    sibling group the deleted deletion-capable build_explicit_loop populated.
  - DELETE in the SAME diff: the build_explicit_loop lambda (Procedure.cpp:43-79)
    and its two deletion-capable calls (Procedure.cpp:85-90). The
    monotone-receive fold + descent (Procedure.cpp:92-127) STAYS (IF-1/IF-4 OUT
    this diff). The successor descent (BuildEagerInsertionRegions) STAYS for the
    monotone path.
  GATE CUTOVER: (1) full suite 164 PASS; goldens are the SEMANTIC net
  (permutation-only bless, permcheck.py); (2) oracle + monotone sidecars as
  correctness referees (a bless is valid ONLY if they agree — never to green a
  red case); (3) ctest 3/3; (4) DR validators green; (5) EXPECT byte-identity
  (§4) — LowerIngestFold reuses the memoized queue vec, no mint-order shift; a
  residual permutation is permcheck-blessable after review; (6) SCOPE FENCE
  reviewed: only the DELETION-CAPABLE seed folds (IF-2/IF-3) cut over; the
  monotone receives (IF-1/IF-4) + successor walk STAY hand-coded (the follow-on,
  §6, exactly as R2 took acyclic families first). VERIFY next_id-across-region
  diff before/after (D1).

STAGE iv (V-PRED-XCHECK completion, ADDITIVE — rides the seam, no emission
  change → byte-identical):
  - Site 4: EmitFrontierFilter (Stratum.cpp:705-743) currently emits a raw
    CHECKMEMBER kNetDeleted/kNetAdded with NO PredXCheck; add a PredXCheck
    against the DR kFrontierFilter op's pred (kNetDeleted/kNetAdded per sign).
    Self-join-INDEPENDENT — lands here.
  - The per-arm gate-node residual (EmitChainStep Site 1, Stratum.cpp:388-401)
    wants POSITION-keyed correlation for self-joins, but EmitJoinFire's Site 2
    correlation is TABLE-keyed (Stratum.cpp:911-921), collapsing a self-join's
    two same-table sides. An R4/self-join concern — DEFER the position-keyed fix
    to P3; land only Site 4 here.

STAGE-SPLIT (owner call): R1e and CUTOVER as two small reviewed diffs
(front-load-cheap-artifacts discipline — small stages lose little on a
stall-retry); stage iv folds into CUTOVER as additive cross-checks.

E-24 COMMENT SWEEP (lands in R1e, same diff): DR.cpp:1645 stale "Build.cpp:1002"
→ ":1048-1051"; DR.h:81-83 kStateFold/kStateEmit/kStateOld "R3 — reserved" →
"LIVE (BuildGroupUpdateOps)"; DR.h:110-112 "STILL construct-alongside: no op is
emitted" → "emitted since R2 family #3"; DR.cpp:1646-1649-adjacent kIngestFold
note "CUT in R1d / stays reserved" → "populated in P2/R1e (deletion-capable
receives; monotone deferred to §6)".

---

## §9. THE ENUM SPELLINGS ACROSS THE DR → CF BOUNDARY (L)

LowerIngestFold crosses the DR VecRole → ControlFlow VectorKind boundary. The
spellings DIFFER and must be used EXACTLY on each side (verified this session):

  DR side (DR.h:52-67, `VecRole`, net-* SINGULAR):
    VecRole::kNetAddition, VecRole::kNetRemoval, VecRole::kAddQueue,
    VecRole::kDeleteQueue, VecRole::kEmpty.

  CF side (include/drlojekyll/ControlFlow/Program.h:248-279, `VectorKind`, net-*
  PLURAL):
    VectorKind::kNetAdditions, VectorKind::kNetRemovals, VectorKind::kAddQueue,
    VectorKind::kDeleteQueue, VectorKind::kEmpty.

The IngestFoldDesc / DROp carries the DR-side `VecRole` (ingest_role).
LowerIngestFold MAPS it to the CF-side `VectorKind` at the TableDeltaVector call
(the only crossing): kNetAddition→kNetAdditions, kAddQueue→kAddQueue,
kDeleteQueue→kDeleteQueue. The §7(c) QUEUE-ROLE AGREEMENT validator operates
purely in DR-side VecRole terms — it must NOT conflate the two enums (the
original stage-(ii) validator did). The E-24 sweep targets (DR.cpp:1645,
DR.h:81-83/110-112) are all verified accurate.

---

## §10. WHAT IS RETAINED AS SOUND (judge-confirmed, unchanged)

- The E-22 discovery inventory is complete; the §4 completeness table correctly
  enumerates the walk effects.
- BuildIngestInventory id-neutrality is verified (a pure derivation).
- The E-24 comment-sweep targets are all accurate and correctly scoped.
- The two witnesses (negate_lower_strata, cf16_4) are in-corpus → suite stays
  164; no new case needed for stage 1.
- MintTableVec monotone-coverage reasoning (%table:4 owns queue vecs;
  %table:11 monotone owns no minted DRVec, the kNetAddition append is a
  lower-time-only effect) is correct.
- The census-mold and the lead-0 off-lattice V-READY/V-BAND-HAZARD skip are the
  right shapes (§4/§7), now with the ordering fixed (§4) and the census form
  corrected to DERIVED-vs-DERIVED (§7).
- The staging discipline (construct-alongside byte-identical HARD GATE, then
  delete-with-cutover under permcheck + oracle referees) is correct and matches
  R1/R2. The byte-identity prediction is defensible now that C1/C2/C3 are
  resolved (§4/§9).
---

## §11. RE-JUDGE RECORD (binding; 2026-07-16)

The revision was re-judged. Every original finding (C1/C2/C3, H1/H2, M, L)
was RE-DERIVED FROM CODE and confirmed RESOLVED. Verdict: REVISE, confined
to ONE new HIGH on the CUTOVER stage only — **R1e IS LANDABLE AS-IS** ("it
emits nothing, deletes nothing, and is byte-identical by construction;
land it first").

THE BINDING CUTOVER AMENDMENT (must be specified + verified before the
cutover diff is authorized):

SPECIFY THE CUTOVER EMISSION TARGET. Every existing DR lowering
(LowerDRFlow/LowerGroupUpdate/LowerDRRounds) emits into the per-stratum
`stratum_seq`; ingest folds must instead emit into the per-io `par`
created locally at Procedure.cpp:712 — which is NOT tracked on Context
(no handle exists at the :2035 body-nest point), and which is EMPTY at
construction for a message with only deletion-capable receives (nls's
`block/1`) once Procedure.cpp:85-90 is deleted. Options to pick and
justify: (a) track the per-io/per-receive par on Context; (b) keep an
empty-shell region in ExtendEagerProcedure that LowerIngestFold fills;
(c) prove sibling-emission under proc_par flattens to the identical tree
(worked tree-shape argument on nls, not a next_id count). AND: replace
the next_id-diff-only byte-identity check with a REGION-TREE STRUCTURAL
COMPARISON on nls before/after — a wrong-parent splice is a tree-SHAPE
change that permcheck cannot bless and stdout may not catch. Until shown,
byte-identity is the TARGET guarded by a hard structural gate, not "the
realistic expectation".

Also carried: the §6 monotone/descent stage remains OUT (hole contract +
the interior-fold net-additions witness are its prerequisites); the LOW
finding stands (the §6 witness's .dr shape may CSE-collapse — IR-verify
before §6 proceeds).

---


## §12. CUTOVER EMISSION TARGET + THE STRUCTURAL GATE (the §11 binding amendment, resolved)

This section discharges the §11 re-judge HIGH: it specifies the CUTOVER
emission target / splice mechanism and replaces the next_id-diff-only
byte-identity check with a region-tree STRUCTURAL COMPARISON. All facts
below are re-derived from HEAD (Procedure.cpp / Stratum.cpp / Optimize.cpp /
Build.cpp this session); nothing is run.

### §12.0. THE TREE AS BUILT (the target the cutover must reproduce)

The entry-proc tree at the moment BuildStratumPhases runs (Procedure.cpp
`BuildEntryProcedure`, verbatim construction order):

  - `proc_par = parallel_regions.Create(proc)` is minted at :659, BEFORE the
    per-io loop, but is NOT yet `proc->body`.
  - Per-io loop (:711-720), once per `query.IOs()` in IOs() order:
      * `par = parallel_regions.Create(proc)` (:712) — a FRESH PARALLEL per io.
      * `proc->body.Emplace(proc, par)` (:713) — par is transiently proc->body
        so ExtendEagerProcedure's `parent` arg IS this `par`.
      * `ExtendEagerProcedure(impl, io, context, proc, par)` (:714) — fills
        `par` with one VECTORLOOP child per receive×polarity (the deletion-
        capable folds AddRegion directly onto `parent==par`, :47; the monotone
        fold likewise, :94).
      * `curr_body = proc->body.get()` (== this `par`), re-parented as a CHILD
        of `proc_par` (:716-719).
  - `proc->body.Emplace(proc, proc_par)` (:748) → `proc->body == proc_par`,
    whose children are `[test_and_set, io0_par, io1_par, ...]`.
  - BuildStratumPhases :2035-2039 wraps that into a SERIES:
    `seq = SERIES[ proc_par, stratum_seq_0, stratum_seq_1, ... ]`,
    `proc->body = seq`.

So for nls: `io_block_par` (the `block/1` io's PARALLEL) is a CHILD of
`proc_par`, and its two VECTORLOOP children (add fold, delete fold) are the
exact siblings the cutover must reproduce IN THAT PARALLEL. The §11 problem
is real: at the :2035 body-nest point BuildStratumPhases holds `proc` and
`seq`, but has NO handle on any per-io `par` — those pointers died with the
:711 loop iteration. And once Procedure.cpp:85-90 is deleted, `io_block_par`
is EMPTY at construction for nls (block/1 has only a deletion-capable
receive; ExtendEagerProcedure `continue`s past it emitting nothing).

CRITICAL SECOND FACT (Build.cpp:1259-1269): in OPTIMIZING modes
`impl->Optimize()` runs AFTER BuildEntryProcedure and BEFORE
ExtractPrimaryProcedure, and AGAIN after. In `-disable-controlflow-opt`
modes (`nodf`/`none` w.r.t. CF) `optimize` is false and the RAW tree
survives unflattened all the way to `-ir-out`. This decides both the option
choice AND the gate: a wrong-parent splice is only observable in the
un-flattened tree (flattening erases exactly the PARALLEL nesting a
misparent would perturb — Optimize.cpp:64/72/88).

### §12.1. THE THREE §11 OPTIONS, EVALUATED AGAINST THE CODE

**(a) Track the per-io/per-receive `par` on Context.**
FIELD: `std::unordered_map<ParsedMessage, PARALLEL *> ingest_par;` (or keyed
by QueryIO — but ParsedMessage is already the Context key convention,
messsage_handler/publish_vecs/commit_published_view). WRITER: the :711 loop,
`context.ingest_par.emplace(message, par)` right after :712 (the `message`
is already in scope in ExtendEagerProcedure; hoist it or pass io through).
LIFETIME: written during BuildEntryProcedure's :711 loop, read at phase time
by LowerIngestFolds (`context.ingest_par.at(op.ingest_message)`), same
build-pass lifetime as `context.dr_flow` — no dangling (the PARALLELs are
owned by `impl->parallel_regions`, live for the whole build).
VERDICT: CORRECT AND MINIMAL. It reproduces the exact parent by
construction: LowerIngestFold AddRegion's the VECTORLOOP onto the SAME
`par` the deleted build_explicit_loop used (:47 `parent->AddRegion(loop)`
where `parent == par`). No tree-shape risk: the parent pointer IS the
original parent. Survives all 4 modes because it does not RELY on
flattening. Cost: one field, one emplace, one map-read. This is the exact
`context.dr_flow` precedent (§3): a build-time handle stashed on Context so
a phase-time lowering can target a construction-time region.

**(b) Empty-shell region in ExtendEagerProcedure that LowerIngestFold fills.**
SHAPE: ExtendEagerProcedure, for a deletion-capable receive, would AddRegion
an empty PARALLEL (or LET) placeholder onto `par` and stash its pointer;
LowerIngestFold emits the two VECTORLOOPs INTO that shell. This is strictly
option (a) with an EXTRA indirection node — and that extra node is FATAL to
byte-identity: an empty PARALLEL is `IsNoOp()` (Parallel.cpp:131, vacuously
true on empty `regions`), so in optimizing modes Optimize.cpp:88-101 ERASES
it as a no-op child, and a single-child shell is ELEVATED away (:64) — but
in the NON-optimizing modes the empty/extra shell SURVIVES as a distinct
tree node the pre-cutover tree never had. Result: byte-identical in opt
modes, tree-SHAPE DIVERGENT in nodf/none modes — exactly the wrong-parent
class the gate must reject. If the shell is filled before Optimize runs it
degenerates to option (a) with a redundant wrapper. VERDICT: REJECTED — it
either equals (a) plus churn, or breaks the un-optimized golden.

**(c) Emit all deletion-capable folds as siblings under `proc_par` (or the
:2036 SERIES) and prove flattening yields the identical tree.**
This drops the per-io grouping and puts every deletion-capable fold directly
under `proc_par` (or the new `seq`). WORKED ARGUMENT on nls:

  TODAY (pre-cutover), un-flattened:
    proc.body = SERIES
      ├─ proc_par : PARALLEL
      │    ├─ test_and_set : TESTANDSET
      │    ├─ io_feed_par  : PARALLEL[ VECTORLOOP(feed, IF-1 monotone fold) ]
      │    └─ io_block_par : PARALLEL[ VECTORLOOP(+add fold),
      │                                VECTORLOOP(-del fold) ]
      └─ stratum_seq_0, ...

  OPTION (c) EMITTED (siblings under proc_par), un-flattened:
    proc.body = SERIES
      ├─ proc_par : PARALLEL
      │    ├─ test_and_set
      │    ├─ io_feed_par : PARALLEL[ VECTORLOOP(feed monotone) ]  (still hand-coded)
      │    ├─ VECTORLOOP(+add fold)     ← spliced sibling, NO io_block_par wrapper
      │    └─ VECTORLOOP(-del fold)     ← spliced sibling
      └─ stratum_seq_0, ...

The `io_block_par` PARALLEL is GONE (block/1's ExtendEagerProcedure emits
nothing now, so no empty par is even created under option (c) — unlike (b)).
Node-by-node the two trees DIFFER: today has a PARALLEL(2 loops) child;
option (c) has two loose VECTORLOOP children. Now FLATTEN:
Optimize.cpp:72-82 donates a PAR-in-PAR's children to the parent — so
today's `io_block_par` (PARALLEL child of proc_par) has its two loops LIFTED
into proc_par, yielding EXACTLY option (c)'s post-flatten shape. So in
OPTIMIZING modes (c) IS tree-identical to today.

BUT: in nodf/none modes `optimize==false`, flattening NEVER runs, and the
two trees above stay DISTINCT (PARALLEL-wrapped vs loose siblings). The
golden for those modes is byte-compared against the SAME committed golden
(CLAUDE.md: every mode's stdout vs one golden). If `-ir-out` (or any
shape-sensitive downstream, e.g. ExtractPrimaryProcedure's per-region
walk :488-522, which iterates `entry_par` membership) differs, the
un-optimized golden breaks. VERDICT: REJECTED for the same reason as (b) —
it relies on a flattening that does not run in 2 of the 4 modes, so it is
NOT byte-identical there. The proof holds ONLY under opt; the amendment
demands identity in all modes.

### §12.2. DECISION: OPTION (a)

Track the per-io PARALLEL on Context and re-target it at phase time. It is
the only option that reproduces the ORIGINAL PARENT POINTER, hence the only
one byte-identical in ALL FOUR MODES without leaning on region-flattening.
It is the `context.dr_flow` precedent applied verbatim. (b) adds a shell
node that survives un-optimized; (c) drops the io PARALLEL wrapper that
survives un-optimized — both are wrong-parent/wrong-shape in nodf/none.

### §12.3. THE ATOMIC CUTOVER DIFF (exact shape + order)

Build.h (Context):
  - ADD field: `std::unordered_map<ParsedMessage, PARALLEL *> ingest_par;`
    documented as "the per-io ingest PARALLEL (Procedure.cpp:712), stashed so
    LowerIngestFolds can splice the deletion-capable folds into the SAME
    sibling group the deleted build_explicit_loop populated — the
    construction-time handle unreachable from BuildStratumPhases, exactly the
    `dr_flow` precedent."

Procedure.cpp (ExtendEagerProcedure + the :711 loop):
  1. In the :711 loop (or inside ExtendEagerProcedure), after `par` is
     created and the message is known, `context.ingest_par.emplace(message,
     par);`. (ParsedMessage is derived at ExtendEagerProcedure:19; either
     pass `par`'s message key out, or emplace in the caller where both `io`
     and `par` are in scope — `ParsedMessage::From(io.Declaration())`.)
  2. DELETE the `build_explicit_loop` lambda (Procedure.cpp:43-79) and its
     two calls (:85-90). The `if (receive.CanReceiveDeletions()) { ...
     continue; }` block becomes just the emplace-and-skip: the deletion-
     capable receive now contributes NOTHING to `par` at build time (the
     folds come from LowerIngestFolds). Keep the `removal_vec` discovery
     (:26-34) — LowerIngestFold reuses those parameter vectors via the
     memoized TableDeltaVector, and the message handler still nets them.
  3. The monotone path (:92-127) STAYS unchanged (IF-1/IF-4 OUT, §0).

Stratum.cpp:
  4. ADD `LowerIngestFold` + `LowerIngestFolds` near LowerGroupUpdate
     (~:1330). LowerIngestFolds iterates `query.IOs()` × `io.Receives()` ×
     (+1 before -1), and for each stage1 (deletion-capable) op:
       * `PARALLEL *par = context.ingest_par.at(op.ingest_message);`
       * emit VECTORLOOP over the add/removal parameter vector, AddRegion
         onto `par` (reproducing Procedure.cpp:45-47);
       * UPDATECOUNT(is_add, kNonRecursive, is_explicit=TRUE) with the table;
       * TableDeltaVector(table, kAddQueue|kDeleteQueue) — REUSES the memoized
         vec minted during the eager walk (Build.cpp:644), so no new id;
       * VECTORAPPEND into that queue (reproducing :68-78).
     Map the DR VecRole→CF VectorKind at the TableDeltaVector call only (§9).
  5. DISPATCH: at the :2035-2039 body-nest point, AFTER `seq`/`proc_par` are
     established, call `LowerIngestFolds(impl, context, query)` reading
     `context.dr_flow` (the same validated object, §3). Because each op
     targets `context.ingest_par.at(message)` — a PARALLEL that is a live
     child of `proc_par` which is a child of `seq` — the splice lands in the
     exact pre-cutover parent. Ordering within a par: LowerIngestFolds' own
     +before- / IOs()×Receives() construction order (§4), NOT pinned_order.

DIFF ORDER within the atomic commit (so no intermediate build is broken):
  (i) Build.h field; (ii) Procedure.cpp emplace; (iii) Stratum.cpp
  LowerIngestFold/LowerIngestFolds definitions + the :2035 dispatch;
  (iv) LAST, delete Procedure.cpp:43-90's build_explicit_loop + calls. Steps
  (i)-(iii) are additive (emit the folds twice would be wrong, so (iv) must
  land in the SAME commit — this is why it is ATOMIC). The R1e ops already
  sit in flow.ops and are censused; only the LOWER + DELETE flip here.

### §12.4. THE STRUCTURAL GATE (replaces the next_id-diff-only check)

The gate is a REGION-TREE STRUCTURAL COMPARISON of the entry procedure's
`-ir-out` text, pre-cutover vs post-cutover, on named witnesses, run IN A
MODE WHERE THE TREE IS NOT FLATTENED so a wrong-parent splice is visible.

MECHANISM (no new tooling; `-ir-out` exists, Main.cpp:267, and the artifact
already quotes its verbatim proc dumps):
  1. Checkout the R1e parent (pre-cutover) commit; for each witness `W`:
     `DR=<pre>  drlojekyll W.dr -ir-out pre/W.ir`
     AND with CF-opt DISABLED: `drlojekyll W.dr -disable-controlflow-opt
     -ir-out pre/W.nocf.ir`.
  2. Same on the cutover commit → `post/W.ir`, `post/W.nocf.ir`.
  3. Extract the entry proc region subtree (`proc ^entry:` … through its
     `return`) from each and `diff` pre vs post.

WITNESSES (real corpus, verified this session):
  - `negate_lower_strata` (nls) — 1 differential message `block/1` (both
    explicit loops) + 1 monotone `feed/2`. The primary: block/1's io par
    goes EMPTY at construction under the cutover, the exact §11 hazard.
  - `cf13_6` — 2 messages, BOTH @differential (two deletion-capable ios, four
    explicit loops across two per-io PARALLELs). Exercises the per-message
    `ingest_par` map key + the IOs()-order splice across MULTIPLE ios.
  - `cf14_3` — 2 @differential + 1 monotone message (a MIXED case): proves
    the monotone io par (IF-1, still hand-coded) and the differential io pars
    (spliced) coexist under proc_par with the pre-cutover sibling order
    intact. (Alternatives with more ios: `merge_5` / `cond_both_polarities`,
    4 messages 2+2; `cf14_3` is the minimal mixed witness.)

PASS CRITERIA:
  - PRIMARY (opt modes): the extracted entry-proc subtree is BYTE-IDENTICAL
    pre vs post for `W.ir` on all three witnesses. Under option (a) this is
    identity by construction (same parent, same construction order, reused
    memoized queue vec → same ids).
  - STRUCTURAL (nocf modes): `W.nocf.ir` entry-proc subtree is
    BYTE-IDENTICAL pre vs post. THIS is the wrong-parent detector: option (a)
    passes (parent pointer preserved); options (b)/(c) would FAIL here (extra
    shell / dropped io-par wrapper visible only un-flattened). A diff in the
    nocf tree that is NOT a pure id-renumber is a REJECT (not blessable).
  - A residual pure id-BIJECTION (every id shifted by a consistent
    renumbering, tree shape identical) is permcheck-reviewable per the
    delta-relational golden policy; a NODE-COUNT or PARENT-EDGE change is
    NOT — it is a bug. State this explicitly: PASS = (structural tree shape
    identical in ALL 4 modes) ∧ (opt-mode text byte-identical) ∧ (suite 164
    stdout + oracle + monotone sidecars byte-identical, GATE CUTOVER §8).

WHY nocf mode is load-bearing: Build.cpp:1259-1269 runs `impl->Optimize()`
(PARALLEL flattening, Optimize.cpp:64/72/88) only when `optimize`; the
nodf/none CF-opt-off runs preserve the raw PARALLEL nesting. A misparent
that flattening would erase in opt mode is caught here. This is precisely
why the next_id-count check the §11 amendment rejected is insufficient: a
count can match while the parent edge is wrong; the structural nocf diff
cannot.

### §12.5. RESIDUAL RISKS + THEIR CHECKS

  R-ID (id-allocation order): LowerIngestFold must mint NO new ids — it
    REUSES the memoized queue vec (TableDeltaVector, already minted by the
    eager walk at Build.cpp:644 before phase time, §4) and the VECTORLOOP /
    VAR ids are minted in the SAME +before-, IOs()×Receives() order the
    deleted build_explicit_loop used. CHECK: the opt-mode byte-identity of
    §12.4 (any id shift shows as a text diff on the entry-proc dump); plus
    the D1 next_id-across-entry-proc diff retained as a SECONDARY witness
    (necessary-not-sufficient, now subordinate to the structural gate).

  R-CLASSIFY (ClassifyVector reading the vecs): ExtractPrimaryProcedure
    (:530-ff) classifies each vector read/written by the entry vs primary
    proc by WALKING the entry-proc region tree (ClassifyVector,
    Procedure.cpp:138). If the folds move parent or the queue append lands
    under a different region, ClassifyVector could re-partition the queue vec
    between entry/primary, changing the primary-proc argument list — a
    SILENT ABI shift stdout might not catch. Under option (a) the append is
    the SAME VECTORAPPEND under the SAME `par` under `proc_par`, so the
    read/write classification is unchanged by construction. CHECK: the
    structural nocf diff (R-CLASSIFY manifests as a changed `call
    ^flow:...(...)` argument tuple or a moved vector-define in the entry-proc
    dump — both are entry-proc-subtree text, covered by §12.4); ADD the
    primary-proc `call` signature line to the extracted comparison window so
    an arg-list shift is in-scope.

  R-ORDER (par sibling order vs io.Receives() order): the deleted
    build_explicit_loop emitted +add THEN -del per receive, receives in
    io.Receives() order (Procedure.cpp:81-90); LowerIngestFolds MUST iterate
    the identical order (§4: IOs()→Receives()→+before-), NOT the DR band Key
    (which sorts -before+ by TABLE*, the OPPOSITE — §4). CHECK: the opt-mode
    byte-identity catches any reorder as a swapped VECTORLOOP pair in the
    dump; the §7(b) ONE-OP-PER-(message,receive,polarity) validator catches a
    duplicate/missing op; and the nls witness pins +add(L44-47) before
    -del(L48-51) explicitly (a swap is a visible text diff).

  R-EMPTY-PAR (nls's block/1 io par empty at construction): once :85-90 is
    deleted, block/1 contributes nothing to its `par` at build time; the par
    is filled ONLY by LowerIngestFolds at phase time. Between those points
    (during `CompleteProcedure` :750 and the FixupContainingProcedure at
    Build.cpp:1259 if it ran pre-phase — it does NOT; phases run inside
    BuildEntryProcedure at Procedure.cpp:756, before Build.cpp:1259) the par
    is transiently empty. RISK: an intermediate Optimize pass could erase an
    empty par before LowerIngestFolds fills it. CHECK: Optimize runs only at
    Build.cpp:1261, AFTER BuildEntryProcedure returns (which includes
    BuildStratumPhases:756 → LowerIngestFolds), so the par is ALREADY FILLED
    before any Optimize sees it — no empty-par erasure window exists. Assert
    this ordering in a one-line comment at the dispatch site and confirm via
    the nls opt-mode structural PASS (an erased-then-refilled par would
    renumber or reparent, failing §12.4).

---

## §13. R1e AS-LANDED DEVIATIONS (2026-07-16; ledger §3 has the full record)

1. §2's monotone queue-role rule (receive-site successor test) was WRONG
   in-corpus — deep_chain_retract's monotone receive reaches its cut
   JOIN through a non-cut same-table interior TUPLE; the §7d cross-check
   aborted (all 4 modes) exactly as designed. LANDED RULE (table-level):
   kNetAddition iff `!TableIsDifferential(T) && (monotone_negated(T) ||
   ∃ member view of T with a cut successor)`; every member view of a
   monotone table is itself monotone plumbing, hence eager-walked. Still
   independent of context.table_delta_vecs — §7d remains a live
   cross-check (and now passes 656/656 compiles).
2. §7(c) scoped to the sound directions (explicit ⇒ queue ∧ differential;
   monotone ⇒ {kNetAddition, kEmpty} ∧ (kNetAddition ⇒ monotone table)) —
   the literal table-keyed clause contradicted §2's own kEmpty derivation
   for a monotone receive over a shared differential table.
3. No publication walk-metadata DROp field (binding §3a omits it; the
   census key is the 5-tuple; publish stays a walk effect until §6).
