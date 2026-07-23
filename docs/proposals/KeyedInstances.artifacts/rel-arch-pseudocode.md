======================================================================
COMMITTED AT THE §20 EPOCH-CLOSE (2026-07-21, tip 64bcbd16+). The
whole-program architecture of THE TWO-AUTHORITY SEAM as pseudocode,
derived from the REAL code at tip by the closing session's orchestrator
(key anchors personally read: Build.cpp:841-1010 BuildEagerInsertionRegionsImpl,
:1084-1170 BuildEagerRegion dispatch, Procedure.cpp:14-60
ExtendEagerProcedure, Stratum.cpp:1909 LowerIngestFold), with the Rel
path expressed as DIFFS on it. SINGLE-PASS: the §9-Rel epoch's opening
fleet re-verifies this against code (per the house precedent) before
building on it — it is the epoch's seed, not its ground truth.
RE-VERIFIED 2026-07-21 at tip 5813ab8a by the Rel epoch-open fleet
(3 seed-unread derivation lanes + 4 seed-read verifiers + xhigh
consolidator; KeyedInstances.md §20(E)): SOUND-WITH-ERRATA, corrections
E-87/E-88/E-89/E-90/E-91/E-92/E-95/E-96 applied IN PLACE below (each
marked at its site). The §2 seam pseudocode, the four seam artifacts,
the cut-successor/AnyCutSuccessorDR lock-step, and the §3 Rel-diff
structure all HELD; the errata were navigation/attribution/figure
drift only.
AMENDED 2026-07-22 at tip 8fa156bc (post R-a2 + R1): §4 = the AS-LANDED
marker-op mold M1-M8 (orchestrator-read anchors), §5 = the path forward
as diffs ON THE MOLD (supersedes §3's R-a2/R1 blocks, which are DONE;
§3's R-final block stands). SINGLE-PASS: the next session's fleet
re-verifies §4/§5 against code before R2.
RE-AMENDED 2026-07-22 at tip 056d2f96 (post R2, §20(J)): §4.1 = the R2
AS-LANDED MOLD DELTAS M2'/M6'/M7'/M9 (orchestrator-read anchors at the
R2 tip — the same session that landed R2 wrote this); §5's R2 block
DONE; §5's R3..Rk re-expressed as per-slice diff blocks ON THE AMENDED
MOLD with the E-101 carrier-coverage requirement made standing.
SINGLE-PASS: the next session's fleet re-verifies §4/§4.1/§5 against
code before R3.
RE-VERIFIED 2026-07-22 at tip b8314dc0 by the R3-open fleet (3 seed-
unread derivation lanes + 3 seed-read verifiers + 2 mechanical lanes +
xhigh consolidator; KeyedInstances.md §20(K)): SOUND-WITH-ERRATA, zero
code or design defects; anchor corrections E-102..E-105 applied IN
PLACE below (each marked at its site — all four are §4-R1-body line
drift caused by R2's dispatch-arm insertions; §4.1's own anchors were
stamped at the R2 tip and HELD). The M9 carrier-coverage/reachability
sweep result is recorded in §20(K) and noted at §5's R3/R4 blocks.
RE-AMENDED 2026-07-23 at tip d4678109 (post R3, §20(L)): §4.2 = the R3
AS-LANDED MOLD DELTAS M10-M13 (orchestrator-read anchors at the R3
tip — the same session that landed R3 wrote this; NOTE the §4/§4.1
R1/R2-body anchors have shifted AGAIN by R3's insertions and are NOT
re-based here — §4.2's fresh anchors + the fleet's next pass are the
live authority); §5's R3 block DONE; §5's R4..Rk re-expressed with
the M12 model-layer caveat. SINGLE-PASS: the next session's fleet
re-verifies §4/§4.1/§4.2/§5 against code before R4.
RE-VERIFIED 2026-07-23 at tip 1492adbf by the R4-open fleet (3 seed-
unread derivation lanes + 3 seed-read verifiers + 1 mechanical lane +
1 worktree-isolated R4-input probe lane + xhigh consolidator;
KeyedInstances.md §20(M)): SOUND-WITH-ERRATA, zero code or design
defects — the M1-M13 mold holds exactly. Errata applied IN PLACE
below (each marked at its site): E-108 MED (the §5 R4 block's
STARTING-STATE CAVEAT — an eager kNegateGate op ALREADY exists,
effect-carrying and walk-over-enumerating; R4 is a diff/re-source,
not a greenfield marker), E-109 LOW (sole @never-NEGATE witness
precision), E-110 COSM (Induction.cpp:996), E-111 LOW (the §4/§4.1
R1/R2-body anchor re-base at tip 1492adbf; definition anchors
:1138/:1146/:1157/:1166/:1113/:1123/:1306/:1279/:1290 HELD exactly).
======================================================================

# The two-authority seam, as pseudocode — and "DeltaRel → Rel" as diffs

## §1. The compile pipeline (context; thrice-fleet-verified in §18(A),
##     amended by this epoch's landings)

    main(argv):                                # bin/drlojekyll/Main.cpp
      flags -> streams, gDemand, gDemandInstance (D2.b), gFirstId,
        gPassPolicy
      query   = Query::Build(module, log, policy, demand_mode)
                # ... ApplyDemandTransform (stamps GuardAnnotations +
                #     RecognizedSubgraphs pre-CSE — D1.a) -> Optimize ->
                #     IdentifyInductions (det_seq) -> ... -> Stratify
      program = Program::Build(query, log, first_id, policy,
                               demand_instance)          # E-91
                # fences pre-pass (cyclic/recursive-content/diff-input
                #     under -demand-instance — D2.b)
                # BuildStratumPhases:                    AUTHORITY A
                #   BuildDRInventory -> DeriveDRStrata -> validators ->
                #   LinearizeAndValidateDRFlow (band key; V-INST-ORDER)
                #   -> V-PRED-XCHECK / V-INGEST-XCHECK    <- seam check
                #   -> Lower{DRFlow,DRRounds,CommitSweeps,GroupUpdate,
                #      SubgraphInstance}                 # emission
                # BuildEntryProcedure (Procedure.cpp:725): AUTHORITY B
                #   per IO: ExtendEagerProcedure -> the eager descent
                #   (E-87: was misnamed "BuildEagerProcedures")
      dumps; codegen.

## §2. AUTHORITY B — the hand-coded eager web (what Rel dissolves)

    ExtendEagerProcedure(impl, io, context, proc, par):   # Procedure.cpp:14
      vec = param vector for io's receives (added_message)
      if any receive CanReceiveDeletions: removal_vec (removed_message)
      for receive in io.Receives():
        if receive.CanReceiveDeletions():
          # THE HOLE CONTRACT, deletion side: the fold ops ARE DR-IR
          # (MakeStageOneIngestFolds' kIngestFold pair — enrolled in the
          # flow, censused, V-INGEST-XCHECK'd), but they are LOWERED HERE,
          # at the original walk position, for id-stream identity.
          for op in MakeStageOneIngestFolds(message, receive, table):
            LowerIngestFold(...)     # Stratum.cpp:1909 — E-88: on THIS
                                     # side the fold body IS the queue
                                     # VECTORAPPEND (:1962-1975) and the
                                     # returned cursor is DISCARDED
                                     # (Procedure.cpp:54-59) — no guard
          continue                   # consumers run in stratum phases (A)
        if receive is table-backed monotone:
          op = MakeMonotoneIngestFold(...)   # single payload authority
          cursor = LowerIngestFold(op, vec)  # E-88: HERE the cursor body
              # is EMPTY (the HOLE) and INGEST-CURSOR-SHAPE guards it
              # (Procedure.cpp:79-93) — monotone-only properties
          BuildEagerInsertionRegions(..., cursor)   # THE HOLE FILLED by B
        else:  # table-less monotone receive
          # E-42: a VECTORLOOP shim minted from NO DR-IR op — the one
          # emission surface with zero model representation.
          loop = hand-mint VECTORLOOP over vec
          BuildEagerInsertionRegions(..., loop)

    BuildEagerInsertionRegionsImpl(impl, view, ctx, parent, succs,
                                   last_table):           # Build.cpp:841
      bind view's columns
      (parent, table, last_table) = InTryInsert(...)  # the fold: an
          # UPDATECOUNT crossing on view's table, if table-backed
      par = new PARALLEL under parent
      if a fold happened this call (parent != parent_)
         && table differential && !induction-owned:         # E-89:
        par += AppendViewTupleToVector(table's kAddQueue)   # park for (A)
            # full predicate Build.cpp:867-868
      any_cut = false
      for succ in succs:                       # THE CUT-SUCCESSOR TEST
        # REPLICATED on the DR side as AnyCutSuccessorDR /
        # MonotoneIngestRoleDR; the §7d role/walk cross-check ABORTS on
        # divergence. Four arms at tip:
        if succ.CanReceiveDeletions()          # differential consumer
           or succ.IsAggregate() or succ.IsKVIndex()   # GROUP_UPDATE
           or (demand_instance_enabled &&
               succ.GuardAnnotationIndex() != kNone):  # D2.b: recognized
          any_cut = true; continue             # (A) owns the successor
        BuildEagerRegion(impl, view, succ, ctx, LET under par, last_table)
      if table monotone && (any_cut || monotone_negated):
        par += append to table's kNetAdditions frontier   # OD-4
            # mechanism-natural: the boundary provisioning that forces
            # the monotone commit sweep (X-DS-2)

    BuildEagerRegion(impl, pred, view, ctx, parent, last_table):  # :1084
      MapVariablesInEagerRegion(...)
      dispatch on view kind:                   # the SYNTAX-DIRECTED WALK
        JOIN w/ pivots  -> BuildEagerJoinRegion      # index-probe loop
        JOIN w/o pivots -> BuildEagerProductRegion
        MERGE inductive -> BuildEagerInductiveRegion # feeds induction
                           # input vecs (round shells are Authority A —
                           # LowerDRRounds; E-92)
        MERGE plain     -> BuildEagerUnionRegion
        AGG | KVINDEX   -> return                    # chain-breaker: (A)
        MAP pure        -> BuildEagerGenerateRegion  # functor call
        CMP             -> BuildEagerCompareRegions
        SELECT          -> rebind + recurse (unit-condition plumbing)
        TUPLE           -> BuildEagerTupleRegion     # forward
        INSERT          -> BuildEagerInsertRegion    # terminal
        NEGATE          -> BuildEagerNegateRegion    # gate
      # None of these interior steps exist as ops: the plan is only its
      # execution trace. The T2b dump has a hole here BY CONSTRUCTION —
      # a monotone program's .deltarel is 12-16 lines vs 419-427
      # differential (§19(H)'s motivating measurement).

    THE FOUR SEAM ARTIFACTS (the §19(H) acceptance list — all become
    internal invariants of ONE mint+lower path when Rel lands):
      S1 the ingest-fold HOLE CONTRACT (empty cursor, filled by B);
      S2 the replicated cut-successor predicates + the §7d role/walk
         divergence-abort (two copies of one decision);
      S3 V-INGEST-XCHECK Site 5 (multiset boundary check that B's folds
         match A's enrollment);
      S4 the E-42 VECTORLOOP shim (emission from no op).
    Plus the D2.b twin of S2: is_recog_guard is replicated symmetric
    with AnyCutSuccessorDR (kept lock-step by the same §7d cross-check).

## §3. "DeltaRel → Rel" AS DIFFS on §2 (the owner re-ranks slices at
##     epoch open; per-slice byte-identity A/B, structural gates for
##     one-time shape changes; the rename ritual last)

    R-a2 (FIRST DELIVERABLE, from the D2.b RAT-6 residual): the
      input-frontier drain for SUBGRAPH_INSTANTIATE — band-(a2) full-
      rescan keyed on the edge frontier (the ADJ-C1 collapse), giving the
      mechanism-natural edge frontier its first consumer and UN-GAPPING
      edge-after-demand. Un-retires the witness REBUILD batch; fence-ii
      question dissolves. (This is Rel-epoch work per OD-3's re-homing.)

    R1..Rk (ONE STEP KIND AT A TIME, the strangler-fig ritual): mint a
      Rel op kind for each §2 dispatch arm (TUPLE forward, INSERT
      terminal, CMP filter, MAP call, JOIN index-probe, MERGE union,
      NEGATE gate, SELECT rebind), each op carrying the walk position so
      LowerRelStep emits AT the original position (id-stream identity —
      the hole contract's own trick, generalized). Per step: census
      extended day one; byte-identity A/B across the 676-row corpus
      (the FROZEN 169×4 baseline — E-90; never a live-count recompute);
      the retained N-run sweep on substrate change; the dump grows the
      op kinds (wholesale .deltarel churn is EXPECTED and is WHY OD-10
      deferred the witness irgold).
      + first slice REC (rel-epoch-open-brief §5): the witness's own
        two-join monotone web — TUPLE/INSERT before JOIN.

    R-final (the seam deletion — §19(H) acceptance):
      - S1 retires: LowerIngestFold's cursor body is minted ops; the
        INGEST-CURSOR-SHAPE guard becomes an internal invariant.
      - S2 retires: ONE cut/boundary decision on the Rel graph; the §7d
        cross-check deletes (nothing to diverge).
      - S3 becomes an interior validator of the one path.
      - S4 retires: the table-less receive minted as a real op.
      - differentialness = op ATTRIBUTE/regime (the direction D1.b's
        regime-split effect multisets already took); eager-per-row vs
        frontier-batch = a LOWERING CHOICE on modeled monotone ops
        (opens the access-plan/WCOJ consumer — the spine's section-walk
        target vs full-scan lowering from §19(N) is the same seam).
      - every program gets a PROPORTIONAL dump + whole-program census.
      - THEN: DeltaRel → Rel rename (the lib/DR→lib/DeltaRel ritual).

    CARRIED PINS THAT BIND Rel WORK: E-62 tripwire at every DeltaRel
    diff; the (F) law (no pointer-ordered emission — det_seq/mint-order
    only); the T2b dump law (stored fields, loud-abort spellings);
    HP-17's death-op residual EXTENDS through Rel (D3.a retires it);
    OWN-3 (the View.cpp record-comparing fold diagnostic) is a HARD
    precondition of D3 recursive/multi-guard admission (both halves,
    per d2b-design §OWN-3 — E-96), not of Rel.

## §4. AS-LANDED AMENDMENT (2026-07-22, tip 8fa156bc; orchestrator-read
##     anchors — R-a2 landed §20(G), R1 landed §20(H); SINGLE-PASS: the
##     next session's fleet re-verifies THIS section before R2)

    R-a2 (§20(G); contract ra2-design.md): §2's OD-4 frontier append now
    HAS its consumer — the SUBGRAPH_INSTANTIATE lowering's band-(a2)
    drains the edge kNetAdditions frontier and full-rescans live-demanded
    keys (FindInstance skip + !TouchedFlag dedup, ONE shared
    emit_instance_rescan emitter). E-100 attribution: the drain CODEGEN
    (FindInstance/skip/dedup/rescan + the emit_instance_rescan lambda)
    lives in Generator::EmitSubgraphInstance (Database.cpp:2369-2403 /
    :2308-2346); LowerSubgraphInstances (Procedure.cpp:252-320) wires the
    memoized input-frontier flow-proc param (id-order) + carries the
    key-arity belt (:292-298). Knob-on only. EDGE-AFTER-DEMAND is
    CLOSED; §3's R-a2 block is DONE.

    R1 (§20(H); contract r1-design.md): §2's dispatch table is AMENDED —
    the TUPLE and INSERT arms are now MODELED OPS. The dispatch
    (Build.cpp BuildEagerRegion, mint sites :1294/:1303 — E-103/E-111:
    R2 then R3 inserted arms ABOVE these; re-based at tip 1492adbf):

      TUPLE  -> op = MakeEagerForwardOp(view, ModelTableOrNull(view))
                LowerRelStep_Forward(op, ...)       # Build.cpp:1138
      INSERT -> message = MessageOfInsertOrNull(insert)   # ONCE at the
                # mint site (:1302 — E-103/E-111; helper :1113, ADJ-S13
                # note :1092 — E-98; both HELD through R3)
                op = MakeEagerInsertOp(view, ModelTableOrNull(view),
                       ClassifyEagerSink(ctx, insert, message), message)
                LowerRelStep_Insert(op, ...)        # Build.cpp:1146
      all other arms -> unchanged hand-coded builders (§2 stands)

    THE MARKER-OP MOLD (established once by R1; every R2..Rk slice is a
    diff on THIS mold, not a fresh design):
      M1 op kinds at the ENUM TAIL, EFFECT-FREE — zero dep edges, so
         V-LINEAR/V-READY/V-BAND-HAZARD never see them (the kIngestFold
         exclusion, generalized).
      M2 ONE single-authority ctor (DeltaRel.cpp:1279/:1290) invoked
         from BOTH the walk mint and inventory enrollment.
      M3 walk-time: mint -> RecordEagerDispatch (Context::
         emitted_eager_ops, walk/DFS order — Build.cpp:1123) -> CALL the
         UNTOUCHED region builder at the original site; id-stream
         identity is MECHANICAL (same args, same walk moment).
      M4 inventory enrollment: BuildDRInventory re-invokes the ctor from
         the recorded stream, TAIL-APPENDED strictly after the ingest
         folds (DeltaRel.cpp:2437-2478 — E-102/E-111 re-based at tip
         1492adbf; the pre-R2 ":2389-2396"
         now lands in the UNRELATED DRRound test_vec build; ADJ-S2
         BINDING — folds keep
         op.0/op.1; the walk is the reachability authority until the
         R-final direction flip).
      M5 key_of lead-0 off-lattice; ORDER LAW (op_table_id, sign, ctor):
         table-less eager ops LEAD the dump; sign-0 eager before each
         table's sign-+1 ingest fold.
      M6 census DAY ONE + the structural recount appended after the
         expect() lines (base batch DeltaRel.cpp:3443-3453; A.6(c)
         recount :3486-3560, guard :3494, view-kind switch :3505,
         table-match :3556 — E-104/E-111 re-based at tip 1492adbf;
         ":3405" sits INSIDE the count-expect() lambda):
         kind<->view-kind + table
         == the union-find MERGED model (DS-ADJ-7 — the RENDER AUTHORITY
         is view_to_model->FindAs, NEVER the .df per-view attribute); NO
         count oracle until R-final (ADJ-S12; the ADJ-S10 bless-time
         count read compensates).
      M7 Format: DROpKindName + kAllKinds + a DEDICATED render case
         (header + args: only — no reads/effects/spine sublines; table=
         rendered ONLY when non-null, tid() has no null guard; new
         payload spellings get their own loud-abort name table, e.g.
         EagerSinkName Format.cpp:130 — E-99/E-105/E-111: the def
         re-settled again post-R3; :127-129 are its doc-comment).
      M8 helpers are .find()-ONLY on Context maps (ADJ-S13/S14 —
         operator[] on publish_vecs/view_to_model is FORBIDDEN in mint
         paths); identity extractions happen ONCE and feed all
         consumers.

## §4.1 R2 AS-LANDED MOLD DELTAS (2026-07-22, tip 056d2f96; §20(J);
##      contracts r2-design.md ADJ-R2-0..8 + r2-desired-states.md
##      DS-R2-1..9 — anchors read at the R2 tip by the landing session)

    The R2 dispatch cuts (Build.cpp): CMP :1276-1281 (wrapper :1157 —
    forwards NEITHER pred_view NOR last_table, the builder's own
    signature) and MAP :1260-1274 (wrapper :1166; mint in the IsPure()
    TRUE arm ONLY — impure maps reject upstream pre-walk, ADJ-R2-3).
    Ctors DeltaRel.cpp:1319/:1330; EAGER_WEB switch (6-way at tip
    after R3's union/select arms; was 4-way at the R2 tip) WITH loud-
    abort default :2450-2478; render cases Format.cpp:903/:915;
    ComparisonOperatorName :150. [E-111 re-based at tip 1492adbf.]
    Mold deltas every future slice
    inherits:

      M2' NO-PAYLOAD-FIELD RULE (the R2 headline, ADJ-R2-1/2): when an
          op's render identity is a PURE FUNCTION of the stored
          eager_view (CMP operator via QueryCompare::Operator(); MAP
          functor via QueryMap::Functor()), it is RE-DERIVED at Format
          time (the agg_name precedent) — never stored. A field is
          stored ONLY when derivation needs context unavailable at
          Format (the kEagerInsert sink/message precedent). This keeps
          EmittedEagerOp closed and the round-trip lossless by
          construction.
      M6' ONE MEMBERSHIP PREDICATE: IsEagerMarkerKind
          (DeltaRel.cpp:1306) is the SOLE spelling of "is an eager
          marker", shared by the A.6(c) guard (:3494) and the key_of
          lead-0 branch (:4475). [E-111 re-based; :1306 HELD.] A new marker kind extends the
          PREDICATE and both sites follow. The A.6(c) kind->view-kind
          dispatch is a SWITCH with a loud-abort default (a fifth kind
          that reaches it un-handled aborts honestly) — mirroring the
          EAGER_WEB default (ADJ-R2-8a). Sites a new kind still edits
          by hand: the enum tail, the ctor pair, the mint cut, the
          EAGER_WEB case, the A.6(c) case, DROpKindName, kAllKinds,
          the render case.
      M7' SECOND NAME-TABLE PRECEDENT: a new payload spelling gets its
          own loud-abort table (ComparisonOperatorName, Format.cpp:150
          — E-111 — the EagerSinkName mold), TOTAL BY CONSTRUCTION + abort
          tail (-Wswitch is WARNING-ONLY in the presets: no -Werror;
          the tail is the enforcement, d2-grammar finding). Reuse the
          .df house spelling for cross-surface consistency — but NOTE
          the accepted-deferred residual: eq/neq/lt/gt now has THREE
          hand copies (two inline in lib/DataFlow/Format.cpp);
          unification is a dedicated hygiene diff (touches the .df
          emitter, never mid-slice).
      M9  CARRIER COVERAGE IS A PER-SLICE DESIGN INPUT (the E-101
          lesson, now STANDING): before scoping a slice, CHECK which
          committed carriers contain the target view kinds (grep the
          .df goldens' node-prefix histogram — the spelling is
          `compare `/`map `, NOT `cmp`). If none, the slice needs a
          new/extended carrier (RAT-8 seeding, sidecar on an
          all-4-modes-clean golden case) or its blocks land corpus-
          unwitnessed (the ADJ-S5 residual class — declined for R2).
          Census renders zero-count kinds, so kind-set growth churns
          EVERY carrier's census line regardless.

## §4.2 R3 AS-LANDED MOLD DELTAS (2026-07-23, tip d4678109; §20(L);
##      contracts r3-design.md ADJ-R3-1..10 + r3-desired-states.md
##      DS-R3-1..9 — anchors read at the R3 tip by the landing session)

    The R3 dispatch cuts (Build.cpp): UNION mint :1243-1245 (inside
    the IsMerge arm's not-owning-an-InductionGroupId else-leg; the
    owning leg is Authority A, mint-free) and SELECT mint :1286-1288.
    Wrappers :1200 (LowerRelStep_Union — forwards the builder's full
    6-arg signature; pred_view is never read by the builder, forwarded
    for uniformity) and :1209 (LowerRelStep_Select). The extracted
    builder BuildEagerSelectRegion :1183 (the inline rebind block
    moved VERBATIM — assert(pred_view.IsInsert()) + the col rebind
    loop + the BuildEagerInsertionRegions recursion; a byte-move
    minting zero impl->next_id). Ctors DeltaRel.cpp:1342/:1354;
    EAGER_WEB cases :2466-2471 (6-way + loud-abort); A.6(c) arms
    :3526 (union, STRENGTHENED) / :3540 (select, strict);
    IsEagerMarkerKind :1306 (6-way; callers :3494 + :4475); render
    Format.cpp:935/:945 (the kEagerForward shape exactly, no extra
    token); DROpKindName :120-121; kAllKinds :1040 (24); enum
    DeltaRel.h:190/:201. Mold deltas every future slice inherits:

      M10 STRENGTHENED-RECOUNT PRECEDENT (ADJ-R3-2): when the mint
          predicate is STRICTER than the view-kind (union: IsMerge &&
          !InductionGroupId — the view-kind alone is ambiguous), the
          A.6(c) arm re-checks the FULL mint predicate, not just the
          kind. The R1/R2 "recount checks only the unambiguous
          view-kind" uniformity is a coincidence of those arms, not a
          law. An arm whose strengthen could FALSE-abort on a
          reachable shape stays strict until the reachable set is
          verified (the select arm's declined condition-relation
          strengthen, ADJ-R3-3).
      M11 EXTRACT-AND-WRAP PRECEDENT (ADJ-R3-5): a dispatch arm with
          NO builder file (inline body) is extracted VERBATIM into a
          builder so the LowerRelStep_* family stays uniform and the
          id-stream argument is MECHANICAL (a byte-move), never
          argued per-case. Applies to any future inline arm.
      M12 MODEL-TABLE-NOT-DF-CLASS (the E-106/E-107 lesson, STANDING;
          supersedes the naive half of M9): `.df class=` and `.df`
          TableId are DATAFLOW-layer attributes; the render `table=`,
          InTryInsert's fold-vs-passthrough, and the A.6(c) table
          match all read ModelTableOrNull — the ControlFlow DataModel
          EQUIVALENCE-SET (a `.df class=table-less` view is typically
          model-table-BACKED via sharing). M9 carrier sweeps must
          classify reachability at the WALK layer and table-ness at
          the MODEL layer; when a prediction hinges on model tables,
          PROBE (the stage-(d) blind lane's fprintf-ModelTableOrNull
          instrumentation is the sanctioned precedent — build, probe,
          revert, rebuild, verify pristine vs a blessed golden).
      M13 SECOND-CALLER COVERAGE (Fable-review R3 [1]): before
          minting for an arm, ENUMERATE ALL CALLERS of the target
          builder (grep, not just the dispatch) — a non-dispatch
          caller is emission outside the marker model. If dead,
          label it LOUDLY at the call site (Induction.cpp:996-1005
          — E-110: the loud comment opens at :996, matching §5,
          the dead !NeedsInductionCycleVector fallback — a
          strengthened A.6(c) arm FORBIDS modeling it, so relaxing
          the guarding TODO requires modeling first; re-visit at
          R-final). If live, it is in-scope for the slice.

## §5. THE PATH FORWARD AS DIFFS ON THE MOLD (§3's R1..Rk, updated)

    R2 — DONE (2026-07-22, §20(J); contracts r2-design.md ADJ-R2-0..8 +
      r2-desired-states.md DS-R2-1..9): kEagerCompare(20)/
      kEagerGenerate(21) on the mold; NO new payload field (cmp=/functor=
      re-derive from eager_view); MAP mint pure-arm-only; census 22;
      map_3 the third carrier (RAT-8, deltarel-opt-only pin);
      IsEagerMarkerKind the ONE membership predicate. [Original owed
      decisions, all ruled: payload = re-derived view operator /
      functor name+arity; grammar E-71 pre-code adjudicated; impure
      disposition = no mint, upstream reject.] The original expectation
      text (retained for lineage): Expect
      the two-carrier .deltarel golden churn — E-101: CENSUS-LINE ONLY
      on the existing carriers (census renders zero-count kinds, so
      20->22 churns both; but NEITHER carrier contains a CMP or MAP
      view — verified in both .df goldens — so no new BLOCKS appear
      there). Witnessing R2 blocks needs a THIRD carrier (candidates:
      map_3, acyclic monotone MAP×3+CMP; fibonacci_iterative) seeded
      via RAT-8, or the blocks land corpus-unwitnessed (the ADJ-S5
      residual class). Same bless ritual (ADJ-S7 referee / ADJ-S10
      count read).
    R3..Rk (each a diff on the AMENDED mold M1-M9; owner re-ranks
      scope/pairing at each ritual head):
      R3 — DONE (2026-07-22, §20(L); contracts r3-design.md
        ADJ-R3-1..10 + r3-desired-states.md DS-R3-1..9):
        kEagerUnion(22)/kEagerSelect(23) on the mold; NO payload
        field (M2'); the union mint ONLY on the not-owning-an-
        InductionGroupId leg; SELECT lowered via the extracted
        BuildEagerSelectRegion + LowerRelStep_Select (owner-ruled
        extract-and-wrap); A.6(c) union arm = the mold's FIRST
        STRENGTHENED arm (re-checks !InductionGroupId), select
        strict; census 24; carriers merge_2 + booleans +
        elim-cond-cycle-simple (RAT-8, deltarel-opt pins — six
        .deltarel goldens total now). [All originally-owed decisions
        ruled: bare markers per M2'; SELECT DOES mint (the R1
        forward precedent); E-106/E-107 carrier story recorded in
        §20(K)/(L) — the witnessed kEagerUnion arm is TABLE-BACKED
        (`.df class=table-less` ≠ model-table-null); the table-less
        union render is the opt-unwitnessed residual; the
        Induction.cpp:996 dead second BuildEagerUnionRegion caller
        is a LABELED coverage hole outside the marker model (Fable
        review [1], re-visit at R-final).]
      R4 NEGATE gate (a diff on the AMENDED mold M1-M13). Dispatch
        arm Build.cpp:1310-1312 (IsNegate -> BuildEagerNegateRegion,
        Negate.cpp — 105 lines, a real builder file: NO M11
        extraction needed, the R1-shape thin wrapper suffices).
        STARTING-STATE CAVEAT (E-108, R4-open fleet, verified at tip
        1492adbf): NEGATE is NOT un-modeled like union/select were —
        an EAGER kNegateGate op ALREADY exists, minted per-negate
        from query.Negations() (DeltaRel.cpp:2299-2321, ctx=kEager),
        carrying a REAL kFlagRead effect (NOT effect-free — the
        R1/R2/R3 markers were). It is inventory-only (no
        LowerNegateGate; Negate.cpp BuildEagerNegateRegion is the
        sole CHECKMEMBER emitter) and OVER-ENUMERATES the walk on 9
        corpus cases (the population-2 differential/walk-cut negates:
        cond_both_polarities 2, cond_diff_flipflop 1,
        d5_recursive_negate 1, disassemble +1, merge_5 3,
        negate_cobatch_diff 1, negate_lower_strata 1,
        negate_multiplicity 1, negation_flap 1). The R4 ritual head
        must rule: (A) DIFF/RE-SOURCE — move the mint from
        query.Negations() to the walk dispatch (Build.cpp:1310 via
        MakeEagerNegateOp + LowerRelStep_Negate), dropping the 9-case
        surplus and making the mint comment's "every negate is
        eager-walk-reached" premise true; or (B) FOLD/KEEP — leave
        the all-negations gate as a dataflow census and add a
        SEPARATE effect-free population-1 marker (two ops per
        walk-reached negate). The effect asymmetry must be accounted
        for either way. V-NEG-CTX (DeltaRel.cpp:3057) pins
        @never->ctx=eager and must survive any re-source (all @never
        negates are population 1). The code anticipates the reframe:
        DeltaRel.cpp:417-422 "a standalone form is expected for the
        EAGER context when the eager walk is inventoried, R1d+".
        Payload per M2': @never-ness (HasNeverHint) and the negated
        table are derivable from the view (QueryNegate) — expect a
        bare marker + re-derived render tokens (every new spelling
        E-71-adjudicated pre-code; a positivity/@never token was
        DECLINED unwitnessed at R2 — negate_6 is the sole
        @never-NEGATE witness (E-109: map_5's `@never is_even` is
        @never-on-a-#functor -> a negated MAP, kNegateGate=0, not a
        QueryNegate; negate_cobatch_mono / negate_downstream_diff
        @never hits are COMMENT text), so the token question re-opens
        WITH that carrier or stays declined). M10: the walk-cut
        discriminator is `!v.CanReceiveDeletions()` (the exact
        Build.cpp:970 cut criterion; NON-DEAD via
        d5_recursive_negate; NOT InductionGroupId() — every
        reached-or-recursive negate has group_id=none, the F22
        lesson) — the A.6(c) arm strengthens to match the
        walk-authoritative mint (E-108). M13: DISCHARGED by the
        R4-open fleet — BuildEagerNegateRegion has exactly ONE live
        caller (Build.cpp:1312; def Negate.cpp:9, decl Build.h:533);
        no Induction.cpp-style dead second caller.
        THE PIN-3 STANDING BLOCKER (KeyedInstances.md:431-437, T2b
        review): per-view class= mislabels a non-@never negate's OWN
        table (deletion-capable via its crossover while the negate
        view is not) — producer-side/table-level refinement owed
        BEFORE any negate-carrying .deltarel bless; NOTE the
        refinement itself churns existing class=-bearing dumps (its
        own structural-gate + re-bless cycle) — rule at the R4
        ritual head whether it lands as a PRE-diff or folds in.
        [M9 SWEEP 2026-07-22, §20(K), WITH THE M12 CAVEAT: the 17
        "class=monotone" negate hits across ~20 green cases
        (negate_1..6 et al.) are .df-LAYER readings — R4's stage-(a)
        must re-derive reachability at the walk layer and table-ness
        at the model layer (probe per M12) before trusting them;
        @never THIN — sole @never-NEGATE witness negate_6 ^negate.8
        (E-109), rendered as
        the prose "; never negates <target>", NOT an @never token.
        THE R4-OPEN FLEET RE-DERIVED THIS AT THE RIGHT LAYERS
        (§20(M)): walk-cut = CanReceiveDeletions (Build.cpp:970);
        ~18 fires/opt, 22/none; every reached negate group_id=none;
        PIN-3 MANIFESTS at the model layer (negate_1 ^negate.5 .df
        class=monotone but model %table:4 DIFFERENTIAL,
        CanProduceDeletions=1); negate_6's @never leg MODE-SPLITS
        (%table:7 opt / null none — E-107 shape); recommended golden
        trio negate_6 + negate_1 + d5_recursive_negate (the
        zero-mint NEGATIVE guard, R3-elim analog).]
      MERGE-INDUCTIVE is NOT a marker slice (the round shells are
        Authority A already — E-92); only its induction-input FEED may
        warrant a marker, decided at its own ritual.
    R-JOIN (LAST — the F17/F18 shape; after R4 the remaining
      hand-coded arms are JOIN-with-pivots, JOIN-product,
      MERGE-inductive FEED, and the E-42 shim): the index-probe loop (Join.cpp),
      CARRYING the NOT-RULED pivot-equality-belt fold candidate (brief
      §5: the monotone body TUPLECMP + delta side_key_eqs re-check what
      the exact Index::First/Next probe guarantees — owner rules at the
      slice head; if adopted, ONE bless cycle and probe-exactness
      becomes a documented Runtime contract).
    R-E42: the table-less receive's VECTORLOOP shim minted from an op
      (ExtendEagerProcedure, Procedure.cpp) — S4 retires.
    R-final (the §19(H) acceptance, unchanged from §3): the DIRECTION
      FLIP — inventory becomes the reachability authority and the walk
      CONSUMES ops instead of recording them; then S1 (hole contract) /
      S2 (cut predicates + §7d cross-check) / S3 (V-INGEST-XCHECK) /
      S4 retire as interior invariants; the eager COUNT ORACLE lands
      (owed, ADJ-S12); the ClassifyEagerSink replica retires (ADJ-S4);
      eager-vs-frontier becomes a lowering choice; THEN the DeltaRel->
      Rel rename ritual. R1 RESIDUALS CARRIED LOUD: publish-* sink
      spellings + the stream message= arm are corpus-UNWITNESSED
      (ADJ-S5 — a publishing-demanded-insert corpus case is the cheap
      witness); DS-ADJ-1: census counts are mode-stable ONLY across the
      controlflow axis (df-axis growth is EXPECTED).
