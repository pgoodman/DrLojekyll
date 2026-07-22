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
    (Build.cpp BuildEagerRegion, mint sites :1225/:1234):

      TUPLE  -> op = MakeEagerForwardOp(view, ModelTableOrNull(view))
                LowerRelStep_Forward(op, ...)       # Build.cpp:1138
      INSERT -> message = MessageOfInsertOrNull(insert)   # ONCE at the
                # mint site (:1233; helper :1113, ADJ-S13 note :1092 — E-98)
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
         folds (DeltaRel.cpp:2389-2396; ADJ-S2 BINDING — folds keep
         op.0/op.1; the walk is the reachability authority until the
         R-final direction flip).
      M5 key_of lead-0 off-lattice; ORDER LAW (op_table_id, sign, ctor):
         table-less eager ops LEAD the dump; sign-0 eager before each
         table's sign-+1 ingest fold.
      M6 census DAY ONE + the structural recount appended after the
         expect() lines (DeltaRel.cpp:3405ff): kind<->view-kind + table
         == the union-find MERGED model (DS-ADJ-7 — the RENDER AUTHORITY
         is view_to_model->FindAs, NEVER the .df per-view attribute); NO
         count oracle until R-final (ADJ-S12; the ADJ-S10 bless-time
         count read compensates).
      M7 Format: DROpKindName + kAllKinds + a DEDICATED render case
         (header + args: only — no reads/effects/spine sublines; table=
         rendered ONLY when non-null, tid() has no null guard; new
         payload spellings get their own loud-abort name table, e.g.
         EagerSinkName Format.cpp:126 — E-99).
      M8 helpers are .find()-ONLY on Context maps (ADJ-S13/S14 —
         operator[] on publish_vecs/view_to_model is FORBIDDEN in mint
         paths); identity extractions happen ONCE and feed all
         consumers.

## §4.1 R2 AS-LANDED MOLD DELTAS (2026-07-22, tip 056d2f96; §20(J);
##      contracts r2-design.md ADJ-R2-0..8 + r2-desired-states.md
##      DS-R2-1..9 — anchors read at the R2 tip by the landing session)

    The R2 dispatch cuts (Build.cpp): CMP :1227-1231 (wrapper :1157 —
    forwards NEITHER pred_view NOR last_table, the builder's own
    signature) and MAP :1211-1220 (wrapper :1166; mint in the IsPure()
    TRUE arm ONLY — impure maps reject upstream pre-walk, ADJ-R2-3).
    Ctors DeltaRel.cpp:1317/:1328; EAGER_WEB 4-way switch WITH loud-
    abort default :2425-2447; render cases Format.cpp:901/:913;
    ComparisonOperatorName :148. Mold deltas every future slice
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
          marker", shared by the A.6(c) guard (:3462) and the key_of
          lead-0 branch (:4424). A new marker kind extends the
          PREDICATE and both sites follow. The A.6(c) kind->view-kind
          dispatch is a SWITCH with a loud-abort default (a fifth kind
          that reaches it un-handled aborts honestly) — mirroring the
          EAGER_WEB default (ADJ-R2-8a). Sites a new kind still edits
          by hand: the enum tail, the ctor pair, the mint cut, the
          EAGER_WEB case, the A.6(c) case, DROpKindName, kAllKinds,
          the render case.
      M7' SECOND NAME-TABLE PRECEDENT: a new payload spelling gets its
          own loud-abort table (ComparisonOperatorName, Format.cpp:148
          — the EagerSinkName mold), TOTAL BY CONSTRUCTION + abort
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
      R3 MERGE-union (BuildEagerUnionRegion, Union.cpp — 49 lines) +
        SELECT rebind (the §2 dispatch's IsSelect block: bind INSERT
        input cols -> recurse; NO builder file). Owed decisions:
        payload (a plain union has little identity beyond the view —
        likely bare markers per M2'; SELECT's marker may record the
        unit-condition-ness); whether SELECT-rebind is even a marker
        (it mints no region of its own — the arm is pure plumbing; a
        marker models the WALK STEP, the R1 forward precedent says
        yes); M9 carrier coverage: the existing carriers HAVE merges/
        selects in their .df, but the dispatch's IsSelect arm is
        reached ONLY from unit-condition INSERTs (conditions_to_bools
        shape) — check reachability, not just view presence, before
        scoping (a .df `select` node that is a message receive is a
        walk ROOT, not this arm).
      R4 NEGATE gate (BuildEagerNegateRegion, Negate.cpp — 105 lines).
        Payload per M2': @never-ness and the negated table are
        derivable from the view (QueryNegate) — expect bare marker +
        re-derived render tokens; the PIN-3 class= refinement is a
        STANDING BLOCKER for any negate-carrying .deltarel bless
        (producer-side/table-level class refinement owed first —
        check PIN-3 §19 before scoping); M9: find/produce a monotone
        negate carrier.
      MERGE-INDUCTIVE is NOT a marker slice (the round shells are
        Authority A already — E-92); only its induction-input FEED may
        warrant a marker, decided at its own ritual.
    R-JOIN (LAST — the F17/F18 shape): the index-probe loop (Join.cpp),
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
