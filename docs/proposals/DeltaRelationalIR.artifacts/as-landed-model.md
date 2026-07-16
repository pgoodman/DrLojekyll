# As-landed pipeline model (2026-07-16)

ERRATA (verifier): written against ecfe674 — the V-PRED-XCHECK commit e355959 landed mid-workflow, so PART B's 'duplicated, unread' AUTHORITY annotations for the join-fire matrix, claim gates, and negate gate are STALE (those are now cross-checked at HEAD); line anchors drift ~7-150 lines low in DR.cpp/Stratum.cpp. The structural spine is verifier-confirmed accurate.

# AS-LANDED PIPELINE (delta-relational-ir @ HEAD ecfe674, COMMITTED state) — successor to ledger §2's pre-epoch model

Legend on every component:
  [AUTHORITY: DR-IR]      — the DR flow graph is the sole decider; deleting it changes emitted code
  [AUTHORITY: EMITTER]    — a surviving Emit*/hand-coded region decides; DR op is not consulted
  [AUTHORITY: DUPLICATED(F1)] — DR op carries the datum AND an Emit* re-derives the same datum from Query; only the emitter's copy is read
Validator strength on every check:
  [STRENGTH: can-fail]        — a plausible construction perturbation trips it
  [STRENGTH: self-satisfied]  — asserts the constructor stored what the constructor stores (tautology on current single writer)
  [STRENGTH: vacated-by-family-3] — its comparand (old discovery) was deleted; now recomputes against shared helpers, not an independent oracle

Cite anchors are file:line at HEAD.

═══════════════════════════════════════════════════════════════════════
PART A — Query::Build  (lib/DataFlow, unchanged structure; epoch touched only Optimize CSE + Stratify)
═══════════════════════════════════════════════════════════════════════

Query::Build(module, log, optimize):
    parse → dataflow node graph (SELECT/TUPLE/JOIN/MERGE/CMP/MAP/NEGATE/AGG/KVINDEX/INSERT)
    QueryImpl::Stratify(log)            # lib/DataFlow/Stratify.cpp:122  [AUTHORITY: EMITTER (dataflow)]
    if optimize: QueryImpl::Optimize(log)   # Optimize.cpp:747
    return Query

--- Stratify (Stratify.cpp) : Tarjan SCC → per-view stratum ; three REJECTS, all as clean diagnostics ---
    A1. unstratified NEGATION (:270-295): negate.stratum == negated_view.stratum ⇒ error
    A2. unstratified AGGREGATION (:297-332, R3b): reject_in_scc_agg(agg):
            for input_view in agg.predecessors:
                if agg.stratum == input_view.stratum: error "unstratified aggregation"  # one diag per agg
        applied to every live AGG (:327) and KVINDEX (:334) — KV is the degenerate aggregate, same rule, no monotone relaxation
    [AUTHORITY: EMITTER]  — strata feed both the aggregate reject AND (later, ported) DeriveDRStrata's initial values via view.Stratum()

--- QueryImpl::Optimize (Optimize.cpp:747): Simplify → Canonicalize fixpoint → CSE; do_cse re-runs CSE to fixpoint ---
    do_cse():  while CSE(all_views) merged something: re-canonicalize  (:711-745, :784/:790)

    CSE(impl, all_views)  (:275) — COLOR-REFINEMENT bucketing (the epoch's RQ5b change):
        cse_color::Refine(all_views, colors)          # :241 bisimulation partition refinement
            color[v] := InitColor(v)                  # :121 HashInit + SELECT stream/relation identity + INSERT decl-Id
            repeat ≤ (#views+2) rounds, stop after 2 stalled rounds (grace round) (:249-270):
                color'[v] := StepColor(colors, v)     # :141
                    fold generic input_columns + attached_columns positionally  (:157-163)
                    per-kind mix ONLY the fields that kind's Equals compares (:165-225):
                        JOIN: num_pivots, out_to_in, joined_views' colors, per-output-col in-sets
                        MERGE: merged_views colors ; NEGATE: negated_view color + is_never salt
                        MAP: functor.Id + free-params + is_positive + binding pattern
                        CMP: op ; AGG: functor.Id + group_by/config/aggregated col colors ; KV: merge_functors ids
            invariant (F6): color partition ⊇ Equals partition — a fold only refines where Equals would still split
        bucket all_views by refined color, first-seen order (:284-291)
        within each color bucket: pairwise v1.Equals(v2) → ReplaceAllUsesWith  (:307-376)

    F6 REVIEW-FIX ASSERT (ecfe674, Optimize.cpp:154-155)  [STRENGTH: can-fail]:
        assert not(v.AsJoin()||v.AsSelect()||v.AsMerge()) or (input_columns.Empty() and attached_columns.Empty())
        — defends the "these three carry empty col-lists so the generic fold is a no-op on them" precondition
        (a future pass attaching columns there would silently over-split Equals-equal pairs)
    CO-BUCKET ASSERT (:320): v1.columns.Size()==v2.columns.Size()  [STRENGTH: can-fail] — catches a color-fn bug
        that co-buckets a unit SELECT with a non-unit view before Equals runs

--- Algebra pragmas (R3c-i, 50f936b) — PARSE-AND-STORE ONLY, semantically inert ---
    Parse/Functor.cpp:315-360: @invertible/@recompute (mutually exclusive → conflict diag) + @commutative/@associative/@idempotent
        stored as tokens on ParsedFunctorImpl.{invertible,recompute,commutative,associative,idempotent}_attribute
    Parse.cpp:1211-1229: accessors IsInvertible/IsRecompute/IsCommutative/IsAssociative/IsIdempotent = attribute.IsValid()
    Format.cpp:132-144: round-trips them on print
    [AUTHORITY: EMITTER (inert)] — NO consumer in DataFlow/ControlFlow yet; witness byte-identical to a clone. Reserved for R3c-ii GROUP_UPDATE.

═══════════════════════════════════════════════════════════════════════
PART B — Program::Build  (lib/ControlFlow/Build/Build.cpp:1015) end to end
═══════════════════════════════════════════════════════════════════════

Program::Build(query, log, first_id, optimize):
    B0. FEATURE-GAP PRE-PASS (Build.cpp:1024-1053) [AUTHORITY: EMITTER]: clean diagnostics dominating internal asserts —
        Aggregates, KVIndices, impure Maps, and on-cycle differential @product
        (join: 0-pivot && CanReceiveDeletions && ViewSelfReachable — NOT InductionGroupId, per F22)
    B1. ProgramImpl(query, first_id); Context; init_proc
    B2. BuildDataModel → FillDataModel (assign persistent TABLEs to disjoint view sets)  [AUTHORITY: EMITTER]
    B3. FindMonotoneNegatedTables (net-additions frontier + Seal enrollment for crossover, before eager walk)  [AUTHORITY: EMITTER]
    B4. entry_proc = BuildEntryProcedure  (the eager bottom-up ingest walk)  [AUTHORITY: EMITTER — R2 UNTOUCHED]
    B5. BuildIOProcedure per IO ; BuildInitProcedure (constant/tuple flows)   [AUTHORITY: EMITTER — R2 UNTOUCHED]
    B6. BuildQueryEntryPoint / BuildEmptyQueryEntryPoint per query decl        [AUTHORITY: EMITTER — R2 UNTOUCHED]
    B7. ...BuildStratumPhases is invoked from inside the entry-proc build (B4/eager path); see below
    B8. per-proc return-false tails ; FixupContainingProcedure ; impl->Optimize (control-flow) ; ExtractPrimaryProcedure ; MapVariables
    return Program

───────────────────────────────────────────────────────────────────────
B.I  BuildStratumPhases (Stratum.cpp:1642) — the DR-IR orchestration spine (R2 cutover lives here)
───────────────────────────────────────────────────────────────────────
    recursive_sccs = ComputeRecursiveSCCs(impl, context)          # :1647 emitter's SCC map, PASSED INTO DR

    (1) dr_flow = BuildDRInventory(impl, context, query, recursive_sccs)      # :1674   → PART B.II
    (2) DeriveDRStrata(dr_flow, ...)                                          # :1681   → PART B.III
    (3) ValidateDRInventory(dr_flow)                                         # :1688   → B.IV.a
        ValidateDROps(dr_flow, impl, context, query, recursive_sccs)        # :1689   → B.IV.b (census)
        LinearizeAndValidateDRFlow(dr_flow, ...)                            # :1690   → B.IV.c (edges/linearizer/validators)
    (4) context.dr_flow = shared_ptr(move(dr_flow))   # :1698 ALWAYS stash before any early return (feeds commit sweeps)
    (5) early-return if no branch/join/crossover/product op AND no phase-owned differential table (:1711)
    (6) join_pivots: mint ONE VECTOR* per join view via entry_proc->VectorFor  # :1723-1731
        [AUTHORITY: EMITTER — the ONLY id-affecting call between inventory and emission; family-#1 hazard: LowerDRFlow must REUSE these]
    (7) strata set = union of branch/join/crossover/product strata + phase-owned differential drain strata  # :1741-1758
    (8) nest entry-proc body (the eager ingest) AHEAD of the phase series  # :1762-1766
    (9) for stratum in strata ascending:
            LowerDRFlow(...)     # :1799  acyclic band     → B.V.a
            LowerDRRounds(...)   # :1811  recursive band   → B.V.b

───────────────────────────────────────────────────────────────────────
B.II  BuildDRInventory (DR.cpp:536) — derives every op family INDEPENDENTLY from Query
───────────────────────────────────────────────────────────────────────
Carries a DRFlowGraph (DR.h:551): vecs, tables, ops, rounds, dep_edges, pinned_order, branches, joins, table_vecs, scc_map, *_stratum maps.

  TABLES (:548-556)  [AUTHORITY: DR-IR label / dead-but-alive]:
      DRTable{model, differential=TableIsDifferential(t), member_views=t->views (identity-distinct, V-MEMBER-ID)}
      NOTE flow.tables is WRITTEN, NEVER READ (F8e dead-but-alive)

  PER-TABLE VECS (:571-601)  [AUTHORITY: DR-IR structure; DUPLICATED(F1) for the VECTOR* identity]:
      per differential non-induction-owned table mint 6: DeleteQueue/AddQueue (kSortUniqueAtDrain),
        OverdeleteSet/AdditionSet/NetRemoval/NetAddition (kMultiset); shape=kIds
      + if SCC table: ClaimedDel/ClaimedAdd (round frontiers Δ_D/Δ_A)
      (the real VECTOR* objects are still minted by the emitter via TableDeltaVector/VectorFor; DR mints only debug-labelled DRVec models)

  CROSSOVER family (:610-674)  kCrossover  [AUTHORITY: DUPLICATED(F1)]:
      per non-@never negate: minus arm ALWAYS + plus arm iff negated_table differential
      carries negate/negate_table/negated_table/pred_table/pred_view/negated_differential/crossover_sign + CrossoverArmEffects + RuleClass
      records queue-append DEF edge (A-4 multi-def)
      → LowerCrossoverArm re-derives everything; the payload is decorative

  PRODUCT_ARM family (:683-740)  kProductArm  [AUTHORITY: DUPLICATED(F1)]:
      per acyclic 0-pivot differential join, per side×sign (+ always, − iff side differential):
      side_index, product_sign, side_tables, side_differential, arm_reads (position-keyed: j<i→kInNew, j>i→kInI, sign-independent), ProductArmEffects
      → LowerProductArm re-derives; arm_reads UNREAD

  BRANCHES/JOINS (:742-811)  [AUTHORITY: DR-IR — LowerDRFlow DOES iterate flow.branches/joins as keys]:
      MEMOIZED-WORKLIST SuffixesOf (DR.cpp:152) replaces old path-copying DFS; preserves per-path MULTISET faithfully (suffix- not outcome-memoization, F5)
      DRBranch{source, path (path[0]=member, back()=terminal), ends_at_join, target}
      DRJoin{join_view, pivot_vec (mints ONE shared kJoinPivots vec, kIdCols/sort-unique), targets=CollectSectionTargetsDR}
      → the ONE DR datum genuinely consumed downstream as a driver: which branches/joins exist and their stratum

  SEED_FOLD / CHAIN_FOLD (:855-1138)  kSeedFold/kChainFold  [AUTHORITY: DUPLICATED(F1)]:
      per branch × available sign; two DERIVED suppressions (V-SEED-SUP): same-SCC internal projection → CHAIN_FOLD (in-round both signs),
        all-same-SCC join → no seed. Each arm carries effects (drain/negate-reads/counter/kInIReadFrozen/append) + a PlanNode SPINE
        (kGate negate nodes with context-derived pred → kFold leaf). join-terminal seed: join_pivot=true, NO fold.
      → LowerDRFlow's seed loop re-derives via EmitSeedLoop; the plan spines/effects are validator-only

  FIXPOINT_FIRE (:~1030-1138)  kFixpointFire  [AUTHORITY: DUPLICATED(F1)]:
      ONE op per (join,sign) (G2); per same-SCC delta-position arm carries the claim-relative matrix predicate on its plan-spine ACCESS nodes
      (FixpointSamePred, DR.cpp:472 — the DR MODEL of the matrix), counter fold kRecursive, append to del/add queue
      → EmitJoinFire re-derives the matrix from Query (the REAL one, Stratum.cpp:793-800); reads only emission.join_view. THE MATRIX EXISTS TWICE.

  CLAIM_DRAIN/RETIRE/REDERIVE/FRONTIER_FILTER (:1140-1290)  [AUTHORITY: DUPLICATED(F1) except drain_form/deferral keys]:
      mint_claim(table,sign,form): drain(queue)+flagwrite+set-append; in-round adds frontier dual-append + B-7 queue clear
          claim_gate = del→kDelGateCnrNonPositive / add→kAddGateTotalPositive   # DEDICATED ClaimGate enum (F2 fix), NOT a Pred
      mint_filter(table,sign,deferral): drain(set)+flagread(kNetDeleted/kNetAdded)+append(net frontier)
      acyclic table: single-pass drains + immediate filters ; SCC table: in-round drains + retire×2 + rederive(del only, kRecursivelySupported gate) + deferred filters×2 (E-17)
      → LowerDRFlow/LowerRoundBody use only {kind, claim_form, table_op_sign, is_recursive, drain_stratum} as KEYS; EmitClaimDrain re-derives payload

  COMMIT_SWEEP (:1292-1338)  kCommitSweep  [AUTHORITY: DR-IR keys, DUPLICATED(F1) publish message]:
      per differential table (flavor=differential, publish_target iff backs @differential transmit); per monotone boundary table w/ delta-vec → Seal
      → LowerCommitSweeps reads {table, sweep_flavor, publish_target} as keys, RE-DERIVES the transmit message from context.commit_published_view

  PIVOT_ASSEMBLE (:1340-1391)  kPivotAssemble  [AUTHORITY: EMITTER — UNREAD]:
      per SCC join: pivot_join, pivot_vec_index, pivot_source_tables, effects (per-side both-sign frontier drains + pivot clear/append)
      → NO lowering consumes it; the emitter assembles pivots inline in LowerDRFlow's join block. Wholly validator/dead.

  eager NEGATE_GATE (:1393-1425)  kNegateGate  [AUTHORITY: DUPLICATED(F1)]:
      per negate: ctx=eager, gate_pred=NegateGatePred(eager,hint) (normal→kInI, @never→kPresent), gate_hint
      → BuildEagerNegateRegion re-derives; kIngestFold stays RESERVED (eager message→table fold has no externalized struct to mirror)

  FIXPOINT_ROUND shells (:1427-1472)  DRRound  [AUTHORITY: DR-IR — LowerDRRounds keys on these]:
      per SCC group × phase {kOverdelete,kInsert}: scc_group, phase, test_vecs (claimed-* frontier per SCC table)
      body_ops/output_ops populated LATER by linearizer; both DEAD-BUT-ALIVE (F8e, unread by LowerDRRounds)
      drain_stratum stamped by DeriveDRStrata

───────────────────────────────────────────────────────────────────────
B.III  DeriveDRStrata (DR.cpp:1653) — the PORTED integer lift (R2 family #3; B-13 old-lift seeding retired)
───────────────────────────────────────────────────────────────────────
  [AUTHORITY: DR-IR — sole writer of *_stratum maps; F5 verdict: faithful line-by-line port of the deleted lift]
    owner_stratum(t) = max over t->views of view.Stratum()          # port of TableOwnerStratum
    init drain_stratum[phase-owned diff tables] = owner_stratum
    init branch_stratum / join_stratum / crossover_stratum / product_stratum from view.Stratum()
    ready_after(T) = drain_stratum[T]+1 (0 if not phase-owned) ; ready_across(head,read)= same_scc? 0 : ready_after (SCC self-read exemption)
    negated_tables_ready(branch,head) = max ready_across over negated tables on chain
    MONOTONE FIXPOINT to convergence (:1755-1845): branch lift, join lift, crossover lift,
        product lift (STRICT ready_after, no SCC exemption — acyclic fence), SCC pinning (every SCC table drains at group-max)
    post: join-terminal branch stratum ← its join's ; stamp round.drain_stratum ; stamp FIXPOINT_FIRE.scc_group
  WART preserved (F5): drain_stratum operator[] pollution by non-phase fold targets (:1774/1789) — old behavior; consumer-side filters it (Stratum.cpp:1755)

───────────────────────────────────────────────────────────────────────
B.IV  VALIDATORS (all always-on, fprintf+abort, survive NDEBUG)
───────────────────────────────────────────────────────────────────────
B.IV.a  ValidateDRInventory (DR.cpp:1477) — INTRINSIC B-3 family (V-OLD-EQUIV legs RETIRED with the deleted discovery):
    V-XOVER-ONE (:1485-1555)  [STRENGTH: can-fail] — exactly one − arm/negate, + arm iff negated_differential, no negate_table folded by two negates, no @never
    V-PROD-MONO (:1607-1623)  [STRENGTH: can-fail] — monotone side no − arm/one + arm; differential side both signs
    V-PROD-CLASS (:1598-1604) [STRENGTH: can-fail] — every product counter fold kNonRecursive
    V-JOIN-ONE (:1625-1638)   [STRENGTH: can-fail] — one DRJoin/view, one well-typed pivot vec

B.IV.b  ValidateDROps (DR.cpp:1887) — internal §5 validators + op-inventory census:
    V-CLAIM-GATE (:1967-1977)   [STRENGTH: self-satisfied] — checks the constructor stored the ClaimGate the constructor stores (F3/F2: gate unconsumed at HEAD)
    V-QCLEAR (:1978-2017)       [STRENGTH: can-fail] — in-round drain has queue-clear + dual-append; single-pass has NO clear (positive+negative space)
    V-NEG-CTX (:2020-2035, +plan-spine :1928-1947)  [STRENGTH: can-fail] — gate pred is (ctx,hint)-derived, ABSENT polarity, never sign-keyed;
        genuinely rejects the F18 shape IN THE DR MODEL (but the emitter's real gate at Stratum.cpp:388 is a SEPARATE decision — F1)
    V-DEFER (:2051-2061)        [STRENGTH: can-fail] — SCC filter deferred, non-recursive immediate
    V-ONE-FOLD (:2063-2091)     [STRENGTH: can-fail] — exactly one Fold leaf per arm (0 for join-pivot seed); no Fold has a child
    V-RETIRE-AFTER structural (:2097-2128)  [STRENGTH: self-satisfied] — asserts retire ops EXIST per fire group per sign (both retires minted unconditionally per SCC table alongside fires; ordering leg is in B.IV.c)
    V-OLD-EQUIV CENSUS (:2130+) [STRENGTH: vacated-by-family-3 (F4)] — recomputes expected op counts by CALLING THE SAME SuffixesOf + same helpers;
        a shared-helper bug is invisible to it. The independent branch-MULTISET leg is GONE. Real net = goldens+oracle.

B.IV.c  LinearizeAndValidateDRFlow (DR.cpp:2335):
    (1) materialize vec use-edges from drain/append/clear effects (:2384-2447); build VecAccess + FlagAccess lists (kInIReadFrozen = no hazard)
    (2a) band-template KEYS per op (:2450-2624) — pure fn of op attrs + seeded strata (lead: eager/phase/commit; stratum asc; B-9 band; table-id; sign)
    (2b) DEP-EDGE derivation (:2626-2828) key-directed:
        emit_waw: symmetric write/write, always follows key  (:2681)
        emit_rw_hazard (:2730): classify by ACCESSOR KIND + KEY (EXECUTION) order, NOT construction order (F3(c) fix, :2699-2707);
            writer-before-reader → intra-scope RAW ; reader-before-writer → intra-scope WAR (+ loop-carried RAW iff carried role)
        NOTE (F3(a) fix, :2658-2673): the old SILENT emit_hazard FLIP is GONE — carried roles (Δ frontiers, net_* frontiers, del/add queues)
            are explicitly classified as loop-carried refills so V-BAND-HAZARD stays reserved for a GENUINE inversion
    V-BAND-HAZARD (:2830-2850)  [STRENGTH: self-satisfied at HEAD] — every intra-scope edge runs forward in band key.
        emit_rw_hazard/emit_waw DIRECT every edge from lower-key by construction, so this can only trip a FUTURE deriver. (F3(a): this replaces the evidence-erasing flip — genuinely stronger than before, but cannot fail on the current deriver.)
    V-LINEAR (:2975-2994)       [STRENGTH: can-fail] — pinned_order is a topo sort of intra-scope edges (Kahn w/ band-key tie-break); fails on a cycle
    V-LOOP (:2996-3045)         [STRENGTH: can-fail AT HEAD] — the DOCUMENTED witness check is NOW IMPLEMENTED (F3(b) fix):
        every loop-carried RAW must have a matching intra-scope WAR reader→writer over the same resource+scope, respected by pinned_order.
        (Big-review F3 said "cannot fail as written" — that was PRE-review code; the ecfe674 fix makes it a real drain-before-refill check.)
    V-RETIRE-AFTER ordering (:3047-3080) [STRENGTH: can-fail] — every same-group same-sign fire precedes its retire in pinned_order
    V-READY (:3082-3109)        [STRENGTH: can-fail] — every non-carried RAW writer-stratum ≤ reader-stratum (off-lattice eager gates/sweeps skipped)
    V-OLD-EQUIV(order) (:3111-3137)  [STRENGTH: self-satisfied] — pinned_order non-decreasing in band key; Kahn is key-monotonic BY CONSTRUCTION,
        so this is a standing drift-guard for a future independent deriver, not failable now. (V-OLD-EQUIV(strata) leg RETIRED — was tautological.)

───────────────────────────────────────────────────────────────────────
B.V  LOWERINGS — what each consumes; DECORATIVE payload marked (F1)
───────────────────────────────────────────────────────────────────────
B.V.a  LowerDRFlow (Stratum.cpp:1213) — ACYCLIC band, per stratum:
    RE-DERIVES all_sides_same_scc / same_scc / is_recursive locally from recursive_sccs (:1222-1242) — NOT from DR
    (1) SEEDS: iterate flow.branches at stratum; re-apply suppressions from Query; EmitSeedLoop        [reads branch_stratum,path,ends_at_join,target,source only]
    (2) JOINS: iterate flow.joins at stratum; BuildJoin + LowerSectionWalk (RuleClass re-derived)      [reads join_view, pivot from join_pivots map]
    (3) CROSSOVERS: iterate flow.Crossovers() at stratum; LowerCrossoverArm(*op, sign)                 [reads crossover_sign, negate only]
    (4) PRODUCTS: iterate flow.ProductArms() at stratum; LowerProductArm(*op, seed_vector)             [reads product_view, sign; arm_reads UNREAD]
    (5) ACYCLIC drains+filters: iterate ops keyed on single-pass del kClaimDrain; EmitClaimDrain×2 + EmitFrontierFilter×2  [reads kind,claim_form,sign as KEYS; claim_gate/gate_pred/effects UNREAD → predicate payload DECORATIVE(F1)]

B.V.b  LowerDRRounds (Stratum.cpp:1520) — RECURSIVE band, per stratum:
    walk OVERDELETE rounds whose round.drain_stratum==stratum (F3 nativized field); find INSERT sibling
    LowerRoundBody (:1428): INDUCTION whose cyclic region = { clear each Δ ; in-round EmitClaimDrain per SCC table ;
        FIXPOINT_FIRE: for each kFixpointFire op of this sign+group → EmitJoinFire(emission(op.fire_join)) — EmitJoinFire RE-DERIVES the matrix, reads ONLY join_view ;
        CHAIN_FOLD: for each kChainFold op → EmitSeedLoop(in_fixpoint) ;
        RETIRE: EmitRetireFrontier per SCC table }
    output regions: OVERDELETE→EmitRederive per table ; INSERT→EmitFrontierFilter both signs (deferred, E-17)
    [reads round.{scc_group,phase,drain_stratum,test_vecs} + op.{fire_sign,scc_group,chain_sign,chain_source} as KEYS; round.body_ops/output_ops UNREAD; all per-arm plan spines DECORATIVE(F1)]

B.V.c  LowerCommitSweeps (Stratum.cpp:1604, called from Procedure.cpp:316 PublishDifferentialMessageVectors):
    per kCommitSweep op → COMMITSWEEP{table} ; attach transmit message iff differential&&publish_target (message RE-DERIVED from context.commit_published_view)
    [reads table_op_table, sweep_flavor, publish_target as KEYS; the frozen kInI-read effect DECORATIVE(F1)]

───────────────────────────────────────────────────────────────────────
B.VI  SURVIVING Emit* TEMPLATES — what each STILL OWNS (the real predicate authority, F1)
───────────────────────────────────────────────────────────────────────
  EmitChainStep (Stratum.cpp:318)  [AUTHORITY: EMITTER] — OWNS the F18 negate-gate rule at :388: `in_fixpoint ? kInNew : kInI`.
        Duplicated in DR by NegateGatePred (DR.cpp:346) — DR copy UNREAD. If someone reintroduces the sign-keyed read HERE, every DR validator stays green (F1).
  EmitJoinFire (Stratum.cpp:761)   [AUTHORITY: EMITTER] — OWNS the claim-relative matrix (SurvivesSoFar/AliveAtClaim/InNewWithFrontier/InNewSansFrontier) at :793-800.
        Duplicated by FixpointSamePred (DR.cpp:472). THE EXACTLY-ONCE MATRIX EXISTS TWICE, no cross-check (at committed HEAD).
  EmitClaimDrain (Stratum.cpp:526)  [AUTHORITY: EMITTER] — OWNS the F17 dequeue gate (TryClaimDel C_nr≤0 / TryClaimAdd total>0) implicit in is_del.
  EmitSeedLoop / EmitHeadFold / LowerSectionWalk / EmitFrontierFilter / EmitRederive / EmitRetireFrontier / LowerCrossoverArm / LowerProductArm / BuildCheckMember / BuildJoin
        [AUTHORITY: EMITTER] — all re-derive columns/preds/RuleClass from Query; the DR op is a stratum/kind/sign KEY only.

───────────────────────────────────────────────────────────────────────
B.VII  THE REMAINING HAND-CODED WEB — everything R2 did NOT touch (precise boundary)
───────────────────────────────────────────────────────────────────────
  [AUTHORITY: EMITTER — untouched all epoch]
    - ENTRY / EAGER INGEST: BuildEntryProcedure + the recursive BuildEagerRegion walk (message→table fold sites) — kIngestFold RESERVED, never populated (DR.cpp:1393-1402)
    - BuildInitProcedure, BuildIOProcedure, BuildQueryEntryPoint/BuildEmptyQueryEntryPoint
    - INDUCTION-OWNED PATH: lib/ControlFlow/Build/Induction.cpp (994 lines) — the induction-region machinery is entirely emitter-owned;
      DR-IR explicitly EXCLUDES induction-owned tables everywhere (TableIsInductionOwnedDR filters them out of vecs, branches, drains, rounds).
      BOUNDARY: DR-IR owns the STRATUM-PHASE differential band (acyclic + SCC claim-round fixpoints over differential NON-induction-owned tables + commit sweeps).
      Induction-owned differential maintenance stays on the old web. The two are disjoint by the TableIsInductionOwned partition.
    - BuildDataModel/FillDataModel, FindMonotoneNegatedTables, control-flow Optimize (region flattening/dedup), ExtractPrimaryProcedure, MapVariables


═══════════════════════════════════════════════════════════════════════
PART C — PENDING DIFFS ALREADY BRIEFED (in-flight in the MAIN working tree; NOT in committed HEAD)
═══════════════════════════════════════════════════════════════════════
Confirmed by grep: `HEAD:Stratum.cpp` has ZERO "V-PRED-XCHECK"; the strings exist ONLY in the working-tree `M lib/ControlFlow/Build/Stratum.cpp`.
So the committed subject above has NO cross-check; the concurrent agent's diff adds it. Restated as pseudocode deltas against B.VI:

  DELTA 1 — new helpers (Stratum.cpp:28-63):
      PredMatches(Pred dr, MembershipPredicate emitted): NAME-map DR.h Pred ↔ Program.h MembershipPredicate (unmapped ⇒ mismatch)
      PredXCheck(dr, emitted, site) / PredXCheckFail(site): observation-only compile-time abort on divergence (never mutates a region ⇒ suite byte-identical)

  DELTA 2 — SITE 3, EmitClaimDrain gains `const DROp *drain_op=nullptr` (:529); at :536-542:
      if drain_op: want = is_del? kDelGateCnrNonPositive : kAddGateTotalPositive ; abort if drain_op->claim_gate != want
      (a ClaimGate check, not Pred — honors F2). WIRED at LowerRoundBody in-round call (:1599) + acyclic call (:1493/1495 via drain_op lookup :1590-1595). ⇒ V-CLAIM-GATE becomes can-fail on a real gate perturbation.

  DELTA 3 — SITE 2, EmitJoinFire gains `const DROp *fire_op=nullptr` (:764); at :880-910:
      find the arm matching (delta_pos, sign); collect stored per-table Pred from arm->body plan-spine ACCESS nodes;
      for each emitter scan_list entry PredXCheck(stored[table], emitted_pred, "EmitJoinFire matrix") — abort if side absent from spine.
      WIRED at LowerRoundBody fire call (:1619 passes op). ⇒ the twice-existing matrix (EMITTER Stratum.cpp:793 vs DR FixpointSamePred) is now CROSS-CHECKED; the F1 duplication becomes a checked equality for FIXPOINT_FIRE.

  DELTA 4 — NOT YET DONE (precise boundary of the pending diff):
      SITE 1 (EmitChainStep negate gate, Stratum.cpp:388) is NOT wired — no DROp threaded, `in_fixpoint ? kInNew : kInI` remains the emitter's SOLE authority.
      So even WITH the pending diff, the F18 negate-gate read in EmitChainStep stays unchecked against the DR NegateGate/NegateGatePred model; only the JOIN matrix (site 2) and the CLAIM gate (site 3) get cross-checks. PIVOT_ASSEMBLE, per-arm effects, pinned_order, dep_edges, DRRound.body_ops/output_ops remain UNCONSUMED under the pending diff.

## Notes

Boundary facts verified from code, not docs:

1. COMMITTED SUBJECT vs PENDING: HEAD is ecfe674 "review fixes". `git show HEAD:lib/ControlFlow/Build/Stratum.cpp | grep -c V-PRED-XCHECK` == 0; the V-PRED-XCHECK code lives ONLY in the working-tree modification (`git status`: ` M lib/ControlFlow/Build/Stratum.cpp`). So the committed model has NO cross-check; the concurrent agent's in-flight diff is exactly V-PRED-XCHECK. This matches the ledger §11 disposition (review-fixes F2/F3/F6/F7 already landed as ecfe674; the F1 minimal cross-check is the separate in-flight piece).

2. The review-fixes half (F2 ClaimGate enum, F3 emit_hazard flip removal + V-LOOP witness + RAW/WAR execution-order keying, F6 Optimize color-fold assert, F7 StateCell Touched const) IS committed at HEAD ecfe674 — I read all of it in place. This means several big-review F3 statements ("V-LOOP cannot fail", "emit_hazard silently flips") describe the PRE-review code and are NO LONGER TRUE at HEAD; I marked V-LOOP and V-BAND-HAZARD strengths at their HEAD (post-fix) state and flagged the divergence from the review's pre-fix description explicitly.

3. F1 is fully confirmed from code: LowerDRFlow/LowerDRRounds/LowerCommitSweeps consume DR ops as {kind, sign, stratum, form, group} KEYS only; EmitJoinFire reads only emission.join_view (Stratum.cpp:1483/1606-1619 comment says so and the body confirms); the matrix and negate-gate rules exist twice (DR.cpp:472 FixpointSamePred / DR.cpp:346 NegateGatePred vs Stratum.cpp:793 / :388). PIVOT_ASSEMBLE is wholly unconsumed (no lowering reads it). flow.tables, DRRound.body_ops/output_ops are written-never-read (F8e).

4. Validator strengths are grounded: V-CLAIM-GATE / V-RETIRE-AFTER(structural) / V-BAND-HAZARD / V-OLD-EQUIV(order) are self-satisfied-or-construction-guaranteed at HEAD; the census is vacated-by-family-3 (recomputes via the SAME SuffixesOf helper). Genuinely failable: V-XOVER-ONE, V-PROD-*, V-JOIN-ONE, V-QCLEAR, V-NEG-CTX, V-DEFER, V-ONE-FOLD, V-LINEAR, V-LOOP (post-fix), V-RETIRE-AFTER(ordering), V-READY, plus the two Optimize asserts.

5. Key file:line anchors used: Build.cpp:1015/1024; Stratum.cpp:1642/1674/1681/1688-1690/1698/1711/1723/1799/1811; DR.cpp:536/610/683/855/1140/1292/1340/1393/1427/1477/1653/1887/2335/2681/2730/2830/2996/3082/3111; DR.h:158/428/512-523; Optimize.cpp:141/154/241/275/747; Stratify.cpp:270/297; Parse.cpp:1211. Pending: working-tree Stratum.cpp:28-63/388/529-542/764/880-910/1599/1619.

6. Did NOT run a build (READ-ONLY, concurrent agent editing lib/). All claims are static reads of committed code + working-tree diff inspection.