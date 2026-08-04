# The demand/keyed-instance architecture as pseudocode — and RegionalDataFlowCore as diffs

Verified 2026-08-02 at tip f0c913e0 (branch keyed-instances) by a
nine-extractor fleet. SINGLE-PASS RULE (house precedent,
rel-arch-pseudocode.md): the next session's fleet re-verifies every anchor
in THIS document against code before building on it — line numbers drift;
the structure should not.

Scope: this document covers the layer RegionalDataFlowCore.md replaces
— the demand transform, the recognition records, the instance
recovery/lowering, and the runtime store — plus enough of the pipeline
to place them. The Rel differential machinery (ingest folds, eager
markers, claim gates, commit sweeps, join emission, the 29-kind census)
is NOT re-derived here: KeyedInstances.artifacts/rel-arch-pseudocode.md
§7 is the as-landed authority for that layer, and it SURVIVES the
regional replacement as the per-region local-graph lowering (the
proposal is greenfield at the demand layer only). §4b (runtime epoch
path) and §4c (generated cursor lifetime) are NEW this pass — they place
the demand layer inside the general epoch and expose the two-lifetimes
gap (finding F3) at emission grain.

## §1. The pipeline, with the demand slots marked

    main(argv):                          # bin/drlojekyll/Main.cpp
      # ALTITUDE NOTE: the two Build calls, SetRelDumpStream, and all
      # dump/codegen emission actually live in CompileModule (:62-136),
      # reached from main() (:268-593) only via ProcessModule (:139-191).
      # "main(argv)" here means "the main-driven compile path", not main()'s
      # own body.
      gDemand (:48), gDemandInstance (:49, implies gDemand),
      gDemandRetract (:50)               # three globals, ad-hoc double-set
                                         # (:471-491); no single "X implies Y"
                                         # source of truth — a Stage-C flag
                                         # rework decides whether to centralize
      gPassPolicy.bisect_counter = 0     # :67 — the ONE cross-Build mutable
                                         # state threaded Query->Program via the
                                         # shared gPassPolicy global
      query   = Query::Build(module, log, gPassPolicy,
                             gDemand, gDemandRetract)          # :69-70
      SetRelDumpStream(gRelStream)       # :79 — MUST precede Program::Build:
                                         # Rel is built+drained INSIDE it
      program = Program::Build(query, log, gFirstId,
                               gPassPolicy, gDemandInstance)   # :81-83
      if gIRStream: dump program            # :90-93  AFTER Program::Build
      if gCxxOutDir: GenerateDatabaseCode   # :115-116 (Database.cpp:3720)
      if gDOTStream: dump query (DOT)        # :122-125 AFTER — TableId() only
      if gDFStream:  dump QueryDF{query}     # :130-133 populated post-Program
      # DUMP-TIMING SPLIT (matters to §6 Stage B's regional dump surface):
      #   -rel-out sink installed BEFORE Program::Build (built inside);
      #   -ir-out/-df-out/-dot-out drained AFTER it returns (need TableId()).
      #   A regional dump added in Stage B picks one of these two shapes.
      # (gDRStream/-dr-out is the amalgamated DATALOG-SOURCE echo, NOT the
      #  Rel/DR-IR dump gRelStream/-rel-out — do not conflate by gDR* name.)

    Query::Build                         # lib/DataFlow/Build.cpp:2518
      TrackDifferentialUpdates(log)                  # :2555
      Simplify            [gate df.simplify]         # :2563-2564
      ConnectInsertsToSelects                        # :2570
      ApplyDemandTransform(module, log,              # :2587-2589
                           demand_mode, demand_retract)
        # DELIBERATELY UN-GATED: "df.demand" is in NEITHER kDataFlowBody NOR
        # kAllPasses (PassPolicy.cpp:33-43) — no glob can name it (contrast
        # df.simplify, which IS in kAllPasses and is individually gateable but
        # excluded from the -disable-dataflow-opt wholesale skip). Slotted
        # after ConnectInsertsToSelects, before Optimize — the fabricated
        # graph is optimized AS PEER of the ordinary graph (finding F1's
        # second half: CSE must preserve the annotations).
      Optimize            [gate AnyBodyOptionalEnabled(kDataFlow)] # :2606-2607
        # kDataFlowBody = {df.cse, df.canon, df.dfe, df.sink, df.ident_join}
        # 2026-08-02 (F26): the df.dfe gate picks WHICH dead-flow pass runs,
        #   never WHETHER one runs (Optimize.cpp:885-894, hygiene/optimization
        #   split): EliminateDeadFlows (the full taint-based OPTIMIZATION, also
        #   removes acyclic dead arms) when the gate is ON; CollectDeadCycles
        #   (source-less forwarding cycles + their dependents + trivial cycles
        #   ONLY, by well-foundedness over the shared TaintDerivedFromInput
        #   fixpoint; never consults the policy) when OFF. Dead-cycle collection
        #   is REQUIRED graph hygiene like RemoveUnusedViews — it establishes
        #   Stratify's V-SCC-SEAM precondition (canon demolishes a dead cycle's
        #   MERGE/io-seam structure; the collection is the always-run janitor).
      ConvertConstantInputsToTuples → RemoveUnusedViews →
      ProxyInsertsWithTuples → LinkViews → RemoveUnusedViews →
      IdentifyInductions → FinalizeDepths → FinalizeColumnIDs  # :2613-2624
      # ---- TAIL THE SEED OMITTED (past :2613; nothing was named past
      #      FinalizeColumnIDs before) ----
      TrackDifferentialUpdates(log, /*force=*/true)  # :2625  SECOND call
      TrackConstAfterInit()                          # :2629
      BuildEquivalenceSets(impl)                     # :2631
      Stratify(log)                                  # :2632
        # Stratify runs AFTER ApplyDemandTransform/Optimize, so a demand-
        # FABRICATED relation is subject to the SAME stratification check as
        # ordinary views — it is the pass rejecting unstratified aggregation/
        # negation (kv_in_scc_1/agg_in_scc_1/evm_func_parse). A Stage B/C
        # regional rewrite that moves the demand mint point relative to
        # Stratify must preserve this ordering.
        # 2026-08-02 (F26): Stratify carries the ALWAYS-ON V-SCC-SEAM validator
        #   (Stratify.cpp:346-381, fprintf+abort, survives NDEBUG — the
        #   dataflow-layer peer of the Rel V-* belts, PROMOTED from the
        #   debug-only assert this fix implicated): a multi-view SCC lacking
        #   both an inductive MERGE and an io seam aborts. Its precondition is
        #   unconditionally established by the required dead-cycle hygiene above
        #   (the df.dfe split), so a firing is a real compiler bug, never a
        #   config — a source-less forwarding cycle that survived to Stratify.
      return Query(impl)                             # :2637

    Program::Build                       # lib/ControlFlow/Build/Build.cpp:1308-1311
      # C-2 V-ALGEBRA + feature-gap pre-pass (demand-independent):     :1332-1411
      #   induction-owned agg/KV, undeclared-algebra KV, impure MAP,
      #   0-pivot differential @product in a self-reachable cycle
      if demand_instance:                            # :1428  <- gate
        # Bucket LIVE guard JOINs by forcing_index via GuardAnnotationIndex()
        # (:1429-1438), then per forcing check TWO independent fences:
        #   cyclic_demand: ViewSelfReachable(joined[0]) — reject :1467-1469
        #                  (demand_cyclic_1; compiles under plain -demand)
        #   recursive_content: a kBody guard's input is induction-owned or
        #                  self-reachable — reject :1470-1473 (ADJ-C2, a
        #                  keyed-instance-ONLY gap over guard-JOIN shapes)
        # cyclic_demand takes priority in the if/elif. NOTE these are BOTH
        # local to this block; the plain-`-demand` recursive-content reject
        # (demand_recursive_content_1) is a SEPARATE, earlier body-walk check.
        # FENCE(iii) differential-summarized-input is named in the block's
        # comment but is NO LONGER a live reject (lifted by R-a2 band-(a2)).
      if num_errors != log.Size(): return nullopt    # :1478-1480
      context.demand_forcings = &query.DemandForcings()      # :1491
      context.demand_instance_enabled = demand_instance      # :1497
      BuildDataModel(query, program)                          # :1499
      ... the §7 rel-arch pipeline: THE WALK + THE FLOW,
          BuildDRInventory → validators → Lower* ...

## §2. The demand transform (the top-down authority)

    # lib/DataFlow/Demand.cpp:385  QueryImpl::ApplyDemandTransform
    # (NOTE: TWO files named Demand.cpp — this is lib/DataFlow/Demand.cpp,
    #  the SIP walk + guard mint + record construction. lib/Parse/Demand.cpp
    #  is the leaf FABRICATION file, §2's "Records" callouts below; it has
    #  zero GuardSite awareness. Never conflate "Demand.cpp:145".)
    ApplyDemandTransform(module, log, demand_mode, demand_retract):
      if not demand_mode: return true                # :393 mode gate (total
                                                     #   no-op; id-stream
                                                     #   untouched)
      if module.DemandMessagesFabricated(): reject   # :399 re-entry guard (G2)

      # Step 1 (:417-451): find THE bound #query relation
      #   rejects: >1 bound query (:435); ≠1 materialization (:446)
      q_rel, q_insert = locate()

      # ---- Loop 1 / PHASE 1 (:477-795): trace, never mint ----
      plan : vector<PerAdornment>                    # :456-465
      known_consumers, seen_variants = {}, {}        # :467-469
      for redecl in q_decl.UniqueRedeclarations():   # :477 seen_variants dedup
        reject all-free sibling adornment            # :497
        trace projection chain q_read → read-of-p    # :501-601 (Step 2)
        # Step 3, THE SIP WALK (:602-783): per p_merge member, per bound
        # column, classify the guard site:
        #   kReadAtTuple (:696) | kBaseAtom (:705) | kPushDown (:751)
        # collecting GuardSite{kind, consumer, read, pivot_pos}.
        # GuardSite (pass-internal, :126-140) is DISTINCT from the public
        # GuardAnnotation (Query.h:988); the two are related ONLY by a
        # value-locked enum static_assert at Demand.cpp:145 (against
        # GuardAnnotation::Kind, Query.h:993) — Step 7's static_cast rides it.
        plan.append(PerAdornment{...})

      # Step 4 (:797-826): stray-consumer union, ONCE between the loops.
      #   (The seed's ":809-841" WRONGLY annexed the PendingRewire struct
      #    declaration at :837-846 — that is R-DUP prep for Step 11, NOT part
      #    of Step 4. Real Step 4 = comment :797-808 + code :809-826.)
      known_consumers = CollectColUsers(p_merge)     # every full-width reader's
                                                     # own users must be known
      # PendingRewire{consumer, read, out_for_pos, guard, restore, kind}
      # + `pending` vector declared :837-846 — deferred-rewire scaffolding
      # consumed only by Step 11.

      # ---- Loop 2 / PHASE 2 (:849-1193): mint per adornment ----
      for pa in plan:
        name = "demand__<q>_<adorn>"                 # :872 reserved prefix
                                                     #   ("demand__" not
                                                     #   "__demand_": a leading
                                                     #   underscore lexes as a
                                                     #   VARIABLE, not an atom)
        reject DemandFabricationWouldCollide         # :886 (BEFORE any mint)
        msg   = FabricateDemandMessage(name, types, demand_retract)
                                                     # :895 → Parse/Demand.cpp:163
                # real ParsedMessageImpl via CreateDerived; differential ⇒
                # SYNTHETIC @differential pragma stamped (Parse/Demand.cpp
                # :200-210) so IsDifferential() holds via ordinary machinery
        local = FabricateDemandLocal(name, types)    # :906 → Parse/Demand.cpp:219
                # real ParsedLocalImpl; NO differential concept (a #local is
                # the demand RELATION, never a message — no pragma to stamp)
        mint QueryIO + receive SELECT (the demand seed)   # :916-1041
        forcing_index = |demand_forcings|            # :1048
        # Step 7 (:1052-1084): guard every body site
        for site in pa.sites:
          J = MintGuardJoin(d_reader, site.read, ...)  # :162 "DEMAND-GUARD"
          J.guard_annotation_index = |guard_annotations|; J.query = this  # :1063-1065
          guard_annotations.push(GuardAnnotation{      # :1066-1071
            cast<Kind>(site.kind), kDReader, kBody,
            is_instance_key=false, site.pivot_pos,
            QueryView(site.read), QueryView(d_reader), forcing_index})
          restore = (site.kind != kReadAtTuple)
                    ? MintRestoringTuple(...) : null   # :212 "DEMAND-RESTORE"
          pending.push(PendingRewire{...})             # :1082 (rewire DEFERRED)
        # Step 8 (:1086-1127): the query-projection guard vs a FRESH raw-seed
        # receive projection (:1093-1101 "DEMAND-RAW-SEED") — THE double join.
        # demand_side recorded PRE-CSE (kRawSeed); on the supported monotone
        # slice the identity-join recognizer (df.ident_join, §20(AV)) later
        # folds raw_seed into d_reader, after which the graph alone cannot
        # tell the two sides apart.
        guard_annotations.push(GuardAnnotation{
          kReadAtTuple, kRawSeed, kQueryProjection, ...})  # :1116-1120
        recognized_subgraphs.push(RecognizedSubgraph{       # :1141-1143
          forcing_index, QueryView(p_merge), p_bound,
          QueryView(q_insert), guard_annotation_indices})
        # Step 9 (:1146-1182): TRIPWIRE — root-seed reachability, ALWAYS-ON
        #   (fprintf+abort, survives NDEBUG).
        demand_forcings.push(QueryDemandForcing{            # :1190-1191
          ParsedQuery::From(redecl), d_msg, bound_indices})

      # Step 11 (:1203-1264): apply the deferred rewires, grouped by
      #   (consumer, read):
      #   SINGLETON group → RewireConsumer direct (:232) — the always-true
      #     case at |plan|==1, byte-identical to the pre-R-DUP direct rewire;
      #   MULTI-guard group (N>=2 adornments) → mint MERGE union of the
      #     guards' restored reads ("DEMAND-GUARD-UNION", :1246) then rewire
      #     the shared consumer onto it (R-DUP). Distinct fabricated demand
      #     relations feed distinct joined_views[0], so the union never CSE-
      #     folds.
      # Step 11b / OWN-3 census (:1266-1332) — ALWAYS-ON, PRE-Optimize only:
      #   n_stamped + guard_annotation_folded_count == |guard_annotations|
      #     (guard_annotation_folded_count is OWNED ELSEWHERE — the CSE/fold
      #      machinery, View.cpp — this pass depends on it but never maintains
      #      it);
      #   |recognized_subgraphs| == |demand_forcings|;
      #   g8 belt: every forcing with a stamped guard has >=1 role==kBody guard.
      module.MarkDemandFabricated()                  # :1334

    # TWO failure classes in this file (a regional rewrite must not swap one
    # for the other): every reject(...) is a CLEAN, non-aborting diagnostic
    # (returns false up through Query::Build); the Step-9 tripwire and the
    # OWN-3 census are fprintf+abort internal-invariant belts.

    # THE READ SEAM — the only surface other passes (Rel.cpp, Build.cpp,
    # codegen) consume this pass's output through (NOT impl-> field access):
    Query::DemandForcings()       # Demand.cpp:295 / Query.h:1059
    Query::GuardAnnotations()     # Demand.cpp:301 / Query.h:1067
    Query::RecognizedSubgraphs()  # Demand.cpp:307 / Query.h:1068
    Query::IsDemandMessage(m)     # Demand.cpp:313 / Query.h:1074 — the SOLE
                                  #   codegen message-entry suppression hook;
                                  #   Stage C needs an equivalent predicate

    Records (include/drlojekyll/DataFlow/Query.h — the PUBLIC header):
      QueryDemandForcing  {query, message, bound_params}       # :966
      GuardAnnotation     {Kind(:993), DemandSide(:996),        # :988
                           Role(:998), is_instance_key(:1006),
                           instance_key(:1010), guarded_read(:1014),
                           demanded_view(:1015), forcing_index(:1018)}
        # guarded_read/demanded_view are OPAQUE handles (pointer-derived
        # UniqueId) — equality/lookup ONLY, never entered into any order;
        # ordered consumption must re-derive order from a DefList VIEW WALK
        # (HP-9), never from these fields or the vectors' append order.
      RecognizedSubgraph  {forcing_index, demanded_view,       # :1026
                           key_cols, pub_view, guard_annotation_indices}
      carrier: the CSE-SURVIVING stamp is `QueryViewImpl::guard_annotation_index`
        (`unsigned {~0u}`) at lib/DataFlow/Query.h:479 — the PRIVATE compiler-
        internal header, class body at :262. (The seed's "~:438 on
        QueryViewImpl" was a ~41-line drift AND wrong-header: the PUBLIC
        include/drlojekyll/DataFlow/Query.h only forward-declares
        QueryViewImpl (:308) and exposes the accessor
        QueryView::GuardAnnotationIndex() (:444) + kNoGuardAnnotation (:443).)
        Everything downstream re-walks LIVE views through the stamp because
        the stored QueryView handles go stale; the stamp migrates through CSE
        with group_ids (View.cpp:684-724).

## §3. Recognition recovery + instance ops (the bottom-up authority)

    # lib/Rel/Rel.cpp — the DR-IR side. Op vocabulary in Rel.h:
    #   DROpKind += kSubgraphInstantiate(:148) kInstanceDeath(:153)
    #               kInstanceSeal(:159)
    #   EffKind  += kInstanceRebuild(:86) kInstanceDemand(:90)
    #   BindingSource::kInstanceKeySlot(:478) ties α-fold cols to store keys

    ResolveLiveRecognition(impl, query):             # Rel.cpp:934
      # ABA-SAFE RE-RESOLUTION — the fix for stale RecognizedSubgraph
      # handles: bucket LIVE guard JOINs by forcing_index via each
      # view's GuardAnnotationIndex() (:948-955); per forcing derive
      #   demand_table = joined[0].model                            (:972-974)
      #   input_table/view/key_cols = the role==kBody guard's joined[1]
      #                               + its instance_key               (:977-984)
      #   pub_table/view = live INSERT matching forcing.query.Id()      (:990-1013)
      #                    (FULL parse identity — name+arity — never name-only)
      # No stored QueryView handle from RecognizedSubgraph is ever dereferenced.
      # Exactly TWO call sites, each a fresh invocation (deliberate A.1.5
      # cross-check independence): the mint (:1050) and the census recount
      # (:4006, inside ValidateDROps).

    BuildSubgraphInstanceOps(flow, impl, context, query, scc_map):   # Rel.cpp:1036
      (void) context; (void) scc_map;   # :1039-1040 — plumbing PRESENT but
                                        #   INERT today (relevant if Stage D
                                        #   routes RegionId through here)
      lr = ResolveLiveRecognition(impl, query)                       # :1050
      for rs in query.RecognizedSubgraphs():                         # :1052
        skip if forcing dead / !ok in lr.by_forcing                 # :1053-1056
        HP-4 refusal belt: input view is Map/Negate/Agg/KV → fail    # :1062-1069
        diff       = TableIsDifferential(pub_table)      # P-STORE   # :1072
        input_diff = input_table && TableIsDifferential(input_table) # :1077
        # (NO third hoisted `death_gate` variable — the P-DEATH check
        #  `demand_table && TableIsDifferential(demand_table)` is evaluated
        #  INLINE at the kInstanceDeath mint guard, :1163. P-STORE and
        #  P-DEATH diverge by design — the e5 carrier. NOTE: DRInstance::
        #  differential is FALSE program-wide in today's corpus (Rel.h:881-886);
        #  the `if (diff)` R-DIFF pub arm is code-complete but UNEXERCISED.)
        flow.instances.push(DRInstance{...})                        # :1080-1107
        # ---- kSubgraphInstantiate ----                            # :1110-1159
        inst = DROp(kSubgraphInstantiate)                            # ctor :1110
        inst.effects = InstantiateEffects(diff, input_diff, ...)     # :782
          # demand net-add drain; input net-add drain (band-a2); input net-
          # removal drain iff input_diff (band-a2'); kInstanceDemand frozen
          # read; kFlagRead rederive leaf; kInstanceRebuild +1; kStateEmit;
          # kStateOld; then IF diff: ±(kCounter/kInIReadFrozen/kVecAppend);
          # ELSE (R-MONO): ONE counter, zero appends, no pub-queue TableVec.
        rescan spine = kAccess walk keyed on input_key_cols → kFold(pub)
        flow.ops.push(inst)                                          # :1159
        if demand_table && TableIsDifferential(demand_table):        # :1163 P-DEATH
          mint kInstanceDeath (DeathEffects, :878) — zero-counter signature
        mint kInstanceSeal always (SealEffect, :903)                 # :1177
        flow.instance_stratum[sid] = 1 + max(ready_after(demand),
                                             ready_after(input))     # :1186-1196

    BuildDRInventory(...):                           # Rel.cpp:1803
      tables → vecs → BuildGroupUpdateOps (agg/KV StateCells, :649)
      → for agg/kv: BuildGroupUpdateOps (:2041/:2055)
      → if context.demand_instance_enabled:          # :2065  THE MODE FORK
          BuildSubgraphInstanceOps(...)              # :2066 (flag-off: never
          # runs; RecognizedSubgraphs() never even iterated — sidesteps the
          # dangling-QueryView hazard on every plain-mode corpus flow)
      → branches/joins/eager derivation (rel-arch §7(B))

    Validation — CORRECTED ATTRIBUTION (the seed's one real §3 defect):
      ValidateDRInventory(flow)                      # Rel.cpp:2917-3092
        # ONLY the four intrinsic B-3 validators (V-XOVER-ONE / V-PROD-MONO /
        # V-PROD-CLASS / V-JOIN-ONE). Contains ZERO reference to
        # RecognizedSubgraphs / demand_instance_enabled / ResolveLiveRecognition.
      ValidateDROps(flow, impl, context, query, scc_map)  # Rel.cpp:3358-4790
        # THIS is where the keyed-instance machinery is validated:
        #   *** KEYED-INSTANCE CENSUS RECOUNT ***                    # :3999-4021
        #     (the seed WRONGLY placed this in ValidateDRInventory at
        #      ":4007-4021" — retarget any edit here, ValidateDROps, a DIFFERENT
        #      function with a (flow,impl,context,query,scc_map) signature.)
        #     if demand_instance_enabled: lr = ResolveLiveRecognition(...);
        #       per RecognizedSubgraph ++exp_instance (+ ++exp_death if P-DEATH);
        #       expect kSubgraphInstantiate/kInstanceSeal == exp_instance,
        #       kInstanceDeath == exp_death
        #   V-INST-EFFECT / V-INST-SOLE / V-INST-PAIR                  # ~:4280-4392
        #     CheckInstantiateEffects(op,diff,input_diff)              # :4301
        #     CheckInstanceSolePub — re-keyed (pub_table, forcing_index) since
        #       D3.a.3 (N adornments of one query name SHARE the pub table) # :4384
        #   CheckInstanceDeathFrontier / CheckInstanceInputArm         # :4551-4552
        #   V-ALPHA arms (kInstanceKeySlot placement)                  # ~:4411-4488
      CheckInstanceOrder(flow)                        # Rel.cpp:4993, impl of the
        # V-INST-ORDER decl — called from a THIRD site,
        # LinearizeAndValidateDRFlow (:5023), at :5887, AFTER linearization
        # (needs flow.pinned_order). A Stage-C rewrite retiring the instance
        # op kinds must delete validator calls in BOTH ValidateDROps AND here.

## §4. Lowering + runtime (the exceptional path)

    # Query entry injection — lib/ControlFlow/Build/Build.cpp
    BuildQueryEntryPointImpl:                        # :509-543
      forcer_proc  = BuildQueryInjectorProcedure(is_retract=false)  # :526-527
      retract_proc = BuildQueryInjectorProcedure(is_retract=true)   # :531-532
        # retract built AFTER forcer so the flag-off id stream stays byte-
        # identical to tip.
        # :483 dispatcher: match context.demand_forcings on
        # (query, BindingPattern) — the BindingPattern conjunct (:489-490) is
        # REQUIRED because ParsedQuery::operator== compares (name,arity) only,
        # so two adornments of one name would else cross-wire; retract
        # additionally gates on message.IsDifferential() (:491); fallback =
        # parse-level query.ForcingMessage() (:499, the @first body-forcing
        # surface, force-only).
      # :389 FromRegistry: kQueryMessageInjector proc — one var per bound
      # param, add_vec/del_vec (add ALWAYS first, byte-identical id stream),
      # VECTORAPPEND(payload_vec), CALL handler, RETURN. ALWAYS-ON handler-miss
      # fence at :404-409 (fprintf+abort).
      queries.emplace_back(query, table, scanned_index,
                           forcer_proc, retract_proc)  # :541-542
      # (BuildEmptyQueryEntryPointImpl :551-573 handles a #query whose INSERT
      #  was optimized away — fresh empty table, forcer/retract both nullopt.)

    LowerSubgraphInstances:                          # Procedure.cpp:279
      death_by_sid = OpsOfKind(kInstanceDeath)        # :282-287 (V-INST-PAIR:
                                                      #   <=1 death per sid)
      for op in flow.SubgraphInstances():   # ONE SUBGRAPHINSTANCE region
        # DOC-COMMENT RUNTIME band order (:269-278, "textual and unreorderable"):
        #   a0 death → a1 birth → a2 rebuild → a2' → b publish → Seal.
        #   That is the RENDERED/executed order (CodeGen renders it — NOT
        #   reviewed here); this BUILDER assigns si's fields in a different,
        #   data-dependency order (a0/death is wired textually AFTER band-b).
        V-INST-DIFF-COHERENCE: inst.differential == TableIsDifferential(pub)  # :300
        band a1: birth — orphan-mint fence on demand kNetAdditions front # :309-327
        band a2: rebuild — memoized input net-additions frontier          # :328-352
        band a2': rebuild — input net-removals iff diff input            # :353-366
        V-INST-INPUT-COHERENCE: (input_removal_frontier != null)==input_diff # :374
        band b : signed publish — del_queue/add_queue (differential only) # :386-408
                 si->demand_table set ONLY here (differential), for the
                 band-a2 demand-liveness Present-probe (E8d)             # :413
                 # under R-MONO (monotone pub) si->demand_table stays null
        band a0: death drain (demand kNetRemovals) iff sid in death_by_sid # :416-454
                 V-INST-DEATH-COHERENCE: death op's tables agree w/ inst  # :428
        si->input_key_cols = op.arms[0].body->bound_cols  # the rescan-spine
                             kAccess bound_cols, guarded by the kAccess-kind
                             check at :462-463; ASSIGNED at :464 (seed's ":462"
                             is the guard line, off by two)
        arity belt: |input_key_cols| == |inst.key_cols|                  # :473
        si->input_row_cols = input columns MINUS input_key_cols          # :483-491
                             (the published row payload — unnamed in the seed)
        enroll {instantiate, seal} in context.emitted_instance_ops       # :498-501
          # (death, if any, was enrolled at :452-453 inside the death band)
          # — the Site-5 cross-check inventory consumed at Procedure.cpp:598.

    # Runtime — include/drlojekyll/Runtime/InstanceStore.h:62
    InstanceStore<Key, RowT>:      # double-buffered nested relation
      slots: open-addressing Key → iid   (APPEND-ONLY: kNoInstance=~0u;
                                          KEY EXISTENCE ≠ LIVENESS — F3:
                                          a minted iid persists forever;
                                          WorkingOccupied (current->NumRows()>0)
                                          is the SEPARATE liveness signal, and
                                          is FALSE immediately after
                                          FindOrAddInstance until a band-(a)
                                          TouchCurrent+rescan runs)
      ctor(allocator, monotone=true)     # :70-79 — `monotone` gates ONLY the
                                          #   HP-7 Seal belt; R-DIFF lowering +
                                          #   the death-half unit test pass false
      FindOrAddInstance(:109) TouchCurrent(:134) Current(:142) Frozen(:143)
      Touched(:148) WorkingOccupied(:167) SealedOccupied(:173)
      Seal(:180 swap + CONDITIONAL HP-7 belt) RecycleCurrent(:222)
      DebugValidate(:228 — occupancy coherence + non-aliasing, NDEBUG-gated)
      # Content maintenance = FULL RESCAN of the instance on touch (the
      # band-a1/a2/a2' Present-filtered rescan mold) — the physical strategy
      # embedded in the exceptional lowering. Because `current` only grows
      # (TryAdd) or resets wholesale (never a per-row shrink), NumRows() as
      # occupancy is provably exact (N-1 CLOSED) — hence no working_count field
      # (contrast StateCellStore, whose signed Fold needs one).

## §4b. The runtime epoch path (NEW — where the demand layer sits in the epoch)

    # The GENERAL differential-maintenance epoch every program runs; the
    # demand/instance bands (§3/§4) are CONSUMERS of these primitives, not a
    # variant of them. rel-arch-pseudocode.md §7(A)/(B) is the authority for
    # how the DR-IR flow graph is DERIVED and LOWERED (BuildDRInventory /
    # DeriveDRStrata / Linearize / the 29-kind census); this section maps what
    # that lowering PRODUCES — the emitted region shape — and how it runs
    # against Runtime/Table.h. NOT re-derived here.

    Step 1 — ENTRY: driver call → message handler → data-flow proc.
      <message>_<arity>(db, log, functors, adds[, removes])  # Database.cpp:1507-1541
        asserts db.initialized_; calls <message>_<arity>_detail(...).
      kMessageHandler proc (BuildIOProcedure)                # Procedure.cpp:690-765
        if differential: NETBATCH(adds, removes)  # ::hyde::rt::NetBatch —
          dedup each side + annihilate rows in both (SET semantics: one
          received batch IS one epoch)                        # :727-730
        CALL <entry/primary data-flow proc>; RETURN true      # :733-739

    Step 2 — WHOLE-EPOCH TREE: BuildEntryProcedure.            # Procedure.cpp:931-1048
      CreateDifferentialMessageVectors (publish plumbing)      # :950 → :231-267
      init-guard TESTANDSET + all-constant TUPLE roots          # :953-995
      per IO: ExtendEagerProcedure (Step 3)                     # :997-1006
      CompleteProcedure (drain work_list: joins/products)       # :1036
      BuildStratumPhases (Step 5)                               # :1042
      PublishDifferentialMessageVectors (Step 6)                # :1045

    Step 3 — INGEST FOLDS + EAGER WEB: ExtendEagerProcedure.    # Procedure.cpp:14-124
      per receive, three shapes → the descent's entry cursor:
        deletion-capable: MakeStageOneIngestFolds → LowerIngestFold per
          polarity (UPDATECOUNT parks each row; no eager descent)  # :50-60
        monotone + table:   MakeMonotoneIngestFold → LowerIngestFold
          (returns the UPDATECOUNT "hole"; INGEST-CURSOR-SHAPE guard) # :72-95
        monotone table-less: MakeIngestLoopOp → LowerIngestLoop
          (VECTORLOOP + one VAR/col; INGEST-LOOP-SHAPE guard)         # :96-119
      BuildEagerInsertionRegions fills the hole (STILL the one hand-coded
        descent): TUPLE-forward / CMP / MAP / MERGE / SELECT / NEGATE-gate /
        pivot-JOIN / @product, cut at IsCutSuccessorDR (rel-arch §7(A))  # :121-122

    Step 4 — FOLD CODEGEN: EmitUpdateCount.                     # Database.cpp:2048-2128
      monotone: member.TryAdd(row) (+ index-add / body if `.added`)
      differential: Add/SubDerivation (or Add/SubExplicit); if `.crossed`,
        emit the fold body — 1:1 with Table.h Fold/FoldAt crossing contract.

    Step 5 — DIFFERENTIAL MACHINERY: BuildStratumPhases.        # Stratum.cpp:2117-2508
      BuildDRInventory (rel-arch §7(B))                         # :2149
      DeriveDRStrata                                            # :2156
      delta JOIN_EMIT enrollment (post-Derive, pre-validate)    # :2169-2178
      ValidateDRInventory / ValidateDROps / LinearizeAndValidateDRFlow # :2185-2187
      per stratum (ascending): seed vectors → LowerDRFlow (acyclic band:
        seeds / join section walks / crossovers / product arms / GROUP_UPDATEs
        / claim drains + frontier filters) → LowerDRRounds (recursive SCC band:
        OVERDELETE induction + REDERIVE + INSERT induction, semi-naive
        per-round frontiers via TryClaimDel/TryClaimAdd).        # :2446-2490

    Step 6 — PUBLICATION TAIL: PublishDifferentialMessageVectors. # Procedure.cpp:505-626
      monotone-flow messages: VECTORUNIQUE → VECTORLOOP → PUBLISH → VECTORCLEAR
      if dr_flow:
        LowerSubgraphInstances (§4)   # :588 — the keyed-instance layer runs
          # ONCE, OUTSIDE all strata, at the epoch tail (ASYMMETRY: GROUP_UPDATE
          # is stratified inside LowerDRFlow; SUBGRAPHINSTANCE is NOT — it
          # always runs after every ordinary stratum quiesces, reading frontier
          # vectors those strata finalized). An instance's band-(b) rides the
          # ORDINARY pub-table kDeleteQueue/kAddQueue vectors + commit sweep —
          # confirmed: it emplaces the SAME TableDeltaVectors EmitClaimDrain
          # drains.
        LowerCommitSweeps (Step 7)     # :589
      RETURN true                       # :624-625

    Step 7 — COMMIT + COMPACTION: LowerCommitSweeps / EmitCommitSweep. # Stratum.cpp:2067 / Database.cpp:2890-3027
      monotone: member.Seal() (watermark advance, no renumber)
      differential: member.Commit(publish `was!=now` deltas); DebugValidateCounts;
        then member.CompactDead() — renumbers live-row ids AND rebuilds every
        live index under its key projection (this is WHY the cursor contract
        holds — §4c).

    Step 8 — CLAIM GATES / RETIRE / CHECKMEMBER codegen.        # Database.cpp:3029-3076, 2799-2888
      EmitClaim: id=Find(row); if TryClaimDel/Add(id): body. TryClaim* re-tests
        the stale-entry gate at DEQUEUE (C_nr<=0 / Total>0), not fold time.
      EmitCheckMember maps each MembershipPredicate to its Table.h method — the
        ONLY forms a generated join may read a differential table through.

    Step 9 — POST-HOC PROCEDURE SPLIT: ExtractPrimaryProcedure. # Procedure.cpp:777-928
      Runs after the WHOLE tree above is built + ControlFlow Optimize. Splits
      it into TWO C++ procedures: a thin kEntryDataFlowFunc wrapper (the
      message-vector-adjacent eager work) that tail-CALLs a separate
      kPrimaryDataFlowFunc holding the rest (Steps 5-8). A driver's actual call
      chain is detail-handler → entry_proc → primary_proc. NOT a semantic
      reorder — but any successor doc that says "the entry procedure" as ONE
      C++ function must account for this two-procedure shape (relevant to Stage
      B's FrozenRegionalProgram, which must preserve or fold it away).

## §4c. Generated cursor lifetime (NEW — the F3 two-lifetimes evidence)

    # lib/CodeGen/CPlusPlus/Database.cpp. A #query's whole driver surface.
    # Companion descriptor: ProgramQuery (Program.h:1362-1397) — its `table`
    # is the ORDINARY pub Table<RowT>/DiffTable<RowT>, NEVER an InstanceStore
    # or StateCellStore (those are private members no query cursor references).

    EmitDatabaseDecl:                                # :1436-1605
      ctor (one arg per table/index/statecell/instance-store; InstanceStore
        arg#2 = monotone, `false` iff store.IsDifferential())  # :1465-1473
      init(db, log, functors)  epoch-0 entry, once                 # :1477-1499
      message-entry loop over kMessageHandler procedures:          # :1507-1541
        SUPPRESSION SITE: if IsDemandMessage(*proc.Message()) continue  # :1511-1513
          # a fabricated demand-seed message gets NO public hidden-friend entry;
          # its `_detail` twin (:214-219) is reached ONLY via a query's
          # forcing_function — never a driver-callable seam.
        (SECOND, likely-implicit exclusion — INFERRED, not verified against
         Parse/Demand.cpp: EmitLogDecl's IsPublished() filter :1418-1421 drops
         the received-only demand message from log hooks.)
      query-entry loop: EmitQueryFriends(spec) per ProgramQuery     # :1546-1548

    EmitQueryFriends(spec):                          # :1610-1820
      emit_forcing_call(args):                                       # :1662-1672
        if spec.forcing_function:
          DetailName(*forcing_function)(state..., args...);
        # THE forcing call into the injector proc (kQueryMessageInjector,
        # "inject_<id>"). There is NO separate "run the epoch" step — this
        # call IS the epoch run, transitively, through ordinary VECTORAPPEND +
        # CALL region emission (append bound values to the demand message's
        # add-vector, CALL the handler).
      if spec.retract_function:                                      # :1679-1704
        emit `void name_retract(db, ..., bound params)` — asserts initialized_,
        calls DetailName(*retract_function). VOID / fire-and-forget; emitted for
        BOTH query shapes (existence-check AND cursor), BEFORE the !has_free
        early return; shares ZERO synchronization with any live cursor object.
      if !has_free:  existence check (Find/Present), NO cursor struct  # :1706-1724
      else: nested `struct name_cursor { Database&; bound fields; uint32_t pos;
        bool next(free out-refs) {...} }`                             # :1730-1796
        via_index branch walks an Index First()/Next() — enumeration order
        UNSPECIFIED (open-addressing); unkeyed scan re-checks every bound col.
      factory: emit_forcing_call(bound); return { db, bound..., First/0 }  # :1798-1819

    # ---- THE F3 GAP (two lifetimes, no shared token) ----
    # "Demand lifetime" (a demanded row's PRESENCE) is administered by the
    #   InstanceStore touch/rebuild bands (EmitSubgraphInstance) + the
    #   fabricated demand message's own row presence.
    # "Cursor lifetime" (the drain contract) is a driver-held raw `uint32_t
    #   pos`/`id` into db.<pub member>.
    # These are STRUCTURALLY UNCONNECTED APIs. All three of {inject demand,
    # run epoch, construct cursor} happen inside ONE call — the cursor is only
    # guaranteed fresh at the MOMENT of the forcing call. Every next()
    # thereafter re-reads live mutable state with no snapshot/versioning; any
    # intervening entry-point call (another message, a re-force, or
    # name_retract) can run a COMMITSWEEP whose CompactDead() renumbers the very
    # rows pos indexes — with NO generated guard. The CLAUDE.md cursor contract
    # ("drain fully before the next entry-point call") rules this out by
    # CONVENTION, never by type. Stage C's move-only RootRequestLease + one
    # lifetime API is the type-level fix (§6 Stage C).

## §5. The identity story today (what Stage A replaces)

    member identity  = equality of SURVIVING PHYSICAL ROW FIELDS
      (optimizer liveness picks the fields; runtime hashes the tuple)
    aggregate input  = distinct over()-PROJECTION tuples (columns, not
      rows); the count(*) idiom needs a carried key; a dropped body
      column silently dedups — guarded only by the ADVISORY lint
      LintAggregateProjection (lib/Parse/Aggregate.cpp; agg_distinct_1)
    demand ownership = presence of a fabricated demand-message row
      (multiple requesters collapse; no per-owner retraction — F4)
    demand support   = the row's derivation counters (a count standing
      in for an owner set)
    # RUNTIME ECHO (§4b Step 8): the TryClaimDel/TryClaimAdd stale-entry
    # re-test and the NetAdded/NetDeleted asymmetric !kInI guard
    # (Table.h:465-525 / :433-457) are exactly the hand-written phantom-pair
    # reasoning Stage A's typed DerivationSupportCount / InferConservativeRow-
    # Contracts (proposal §4.4) would formalize — a concrete transfer-rule
    # source, currently living as code comments, not types.

## §6. THE PATH FORWARD AS DIFFS on §1–§5
##     (RegionalDataFlowCore.md §13, amended per fable-review-2026-08-01;
##      ranking: A → I0 → B → C → D; each stage names its exit gate)
##     NOTE 2026-08-02: the five stage docs (stage-a-diff.md,
##     stage-i0-interpreter.md, stage-b-diff.md, stage-c-diff.md,
##     stage-d-diff.md) are now the AUTHORITATIVE hunk-level diffs,
##     critiqued in phase3-critique-report.md (verdicts: B sound-with-
##     amendments; A/I0/C/D each BLOCKED-ON named findings). §6 below is
##     the compact map, corrected where the docs diverged from the seed.

### Stage A — make identity explicit (diff on §5; no pipeline change)

    + typed ids: FieldId, SemanticMemberKey, DerivationSupportCount,
      DemandSupportCount, DeltaSign, ... (proposal §4.1) — no free
      arithmetic/comparison across domains
    + normalization marks MemberProjection | DistinctProjection —
      realized (stage-a-diff.md; O-A1 OWNER-GATED) as an identity-folded
      ProjectionRole discriminant on QueryTupleImpl, with role part of
      node Hash/Equals so CSE structurally cannot merge across roles
      (the F1-proof rule; distinct node CLASSES were this seed's sketch
      and remain the laid-out alternative); optimizer liveness may never
      convert one role to the other (V-NO-COLLAPSE, V-PROJ-ROLE-STABLE)
    + InferConservativeRowContracts over the existing QueryView graph
      (transfer rules = proposal §4.4; §4b Step 8 names two concrete
       runtime-comment sources for the phantom-pair transfer rules)
    + aggregates consume an EXPLICIT input member key
    - LintAggregateProjection (advisory)             → precise contract
      validation (compile error on unproven collapse)
    EXIT GATE (per stage-a-diff.md): ALL 180 goldens AND all dump
      goldens byte-identical (contracts go to a separate opt-in
      -contract-out sink, .df unchanged); agg_distinct_1 loses its 5
      advisory warnings (validated-safe) + is re-pinned; new negative
      witness member_collapse_1 (all-4-modes-diagnostic).
      BLOCKED-ON (phase3): T-conf-1/T-conf-2 — contract inference must
      be an SCC fixpoint (or on-cycle AllFields), not one depth pass.

### I0 — the reference relational interpreter (INSERTED; review C1)

    + tests/RefInterp (or bin/RefInterp): a small definitional
      evaluator (naive/semi-naive over parsed clauses + set semantics
      + batch netting) — NO Rel, NO codegen
    + oracle harness: per corpus case with .batches, compare final
      membership + sorted published deltas vs the CURRENT compiler,
      all 4 modes
    EXIT GATE: interpreter agrees with the tip compiler on the full
      corpus BEFORE any Stage B/C change; disagreement = a finding
      (either a compiler bug or an interpreter bug — adjudicate,
      never tolerance-fudge).

### Stage B — the regional representation becomes canonical (diff on §1)

    Query::Build tail:
    -  ... FinalizeColumnIDs → TrackDifferentialUpdates(force) →
    -      TrackConstAfterInit → BuildEquivalenceSets → Stratify → return query
    +  ... FinalizeColumnIDs → TrackDifferentialUpdates(force) →
    +      TrackConstAfterInit → BuildEquivalenceSets → Stratify
    +  planning = BuildPlanningRegionalProgram(query)
    +      # whole program = ProgramRoot + observation-root region
    +      # templates; NO child extraction yet
    +  frozen = FreezeAndValidate(planning)   # distinct TYPE; no
    +      # mutator from frozen back to open
    +  return frozen
       # (NOTE: the real tail runs Stratify LAST before return — the demand-
       #  fabricated relations are stratified alongside ordinary views; the
       #  planning build must slot AFTER that, so a demanded subgraph's
       #  stratum is settled before it is frozen.)
    Program::Build:
    -  consumes Query                 → +  consumes FrozenRegionalProgram
       (Rel's BuildDRInventory reads through the frozen region's local
        graph; the §7 rel-arch pipeline is UNCHANGED inside — INCLUDING
        the ExtractPrimaryProcedure two-procedure split of §4b Step 9,
        which Stage B must preserve or deliberately fold away)
    + regional dump surface (new; grammar decision owed — T2-style; picks
      the -rel-out-BEFORE-Program::Build shape or the -ir-out-AFTER shape
      of §1's dump-timing split)
    EXIT GATE: all 180 goldens byte-identical (pure representation
      refactor); deterministic regional dump pinned for the witness
      set. The shared gPassPolicy bisect_counter (§1) must still thread
      cleanly across the Query/Program boundary.

### Stage C — request edges replace forcing (diff on §2, §3, §4; the cutover)

    - ApplyDemandTransform (DataFlow/Demand.cpp:385, ALL of it) + the two
      failure classes (clean rejects vs the tripwire/OWN-3 aborts)
    - FabricateDemandMessage/Local (Parse/Demand.cpp:163/:219) + fabrication
      registry + kMessageHandler suppression (Database.cpp:1511-1513) + the
      IsDemandMessage predicate (Query.h:1074)
    - GuardAnnotation / RecognizedSubgraph / QueryDemandForcing
      (Query.h:966-1032) + the guard_annotation_index carrier (private
      lib/DataFlow/Query.h:480) + the DemandForcings/GuardAnnotations/
      RecognizedSubgraphs read seam
    - ResolveLiveRecognition + BuildSubgraphInstanceOps (Rel.cpp:934,
      :1036) + the demand_instance_enabled fork (:2065) + the census recount
      in ValidateDROps (:3999-4021) + CheckInstance* validators (BOTH the
      ValidateDROps site and CheckInstanceOrder at LinearizeAndValidateDRFlow)
    - BuildQueryInjector* (Build.cpp:389-506) + retract_proc + the
      parse-level ForcingMessage fallback
    - flags: -demand, -demand-instance, -demand-retract
    + planner: ExtractPureChild via FirstStableAdmissibleChild
      (proposal §8; ONE level in this stage)
      + ExtractionPolicy(candidate) -> bool as a NAMED SEAM
        (provisionally always-true; the cost model meets it here —
        review C3)
    + RequestEdgeRelation {owner: RootLease|PermanentRoot|
        RegionalMember, call_site, child} — THE demand authority;
      ActiveInstance = DistinctProjection(edges.child);
      DemandSupportCount demoted to derived summary
    + ChildResultRelation; RoutedResult = edges ⋈ child results
      (FIRST PHYSICAL REALIZATION: results stored once, per-owner
       routing/retraction derived from the edge relation — the
       D3.a.3 refcounted-union-pub shape generalized; review C4;
       NEVER a literal per-edge materialization)
      # ADJUDICATION FLAG: this realization generalizes the R-DIFF pub arm
      # (differential published answer under deletion), which DRInstance::
      # differential==false makes CORPUS-UNEXERCISED today (§3). Stage C's
      # tagged-binary behavioral goldens will have NO pre-cutover
      # differential-pub demand-instance witness to compare against — I0
      # must carry that case, or one must be authored.
    + move-only RootRequestLease owned by the generated cursor;
      destructor enqueues the exact edge removal; next entry nets
      pending removals first (replaces §4/§4c's injector/retract pair —
      ONE lifetime API, resolving F3's demand-vs-cursor split)
    + lifecycle lowering through Rel: request_edge_add/remove,
      input_delta, local_fixpoint, child_result_add/remove,
      routed_result_add/remove, retire_inactive, seal_epoch
      (replaces the a0/a1/a2/a2'/b band vocabulary of §4;
       kSubgraphInstantiate/kInstanceDeath/kInstanceSeal retire;
       new lifecycle census + V-* validators, proposal §11)
    ! INADMISSIBLE EXTRACTION ⇒ full materialization in the
      observation root (never a reject) — OWNER DECISION OWED on the
      fate of today's clean diagnostics (demand_cyclic_1,
      demand_recursive_content_1, demand_multi_adorn_allfree_1 —
      review C2)
    EXIT GATE: I0 interpreter agreement on the rewritten witness
      corpus + behavioral goldens from the PRE-CUTOVER TAGGED BINARY
      (final membership + sorted deltas; the eqgate family retires
      only after both referees pass); grep gates: no demand__ prefix,
      no GuardAnnotation, no forcing registry in the tree.

### Stage D — deepen the forest, qualify local recursion (diff on Stage C)

    + nested deterministic child extraction; ports solved in ownership
      postorder; parents reoptimized after child freeze
    + local Rel tables/state/rounds qualified by (RegionId, InstanceId)
      — the induction machinery of rel-arch §7(B) runs PER INSTANCE
      # ADJUDICATION FLAG: today keyed instances run OUTSIDE all strata,
      # never inductive (§4b Step 6: SUBGRAPHINSTANCE is unstratified, lowered
      # once at the epoch tail; the recursive-content fences ADJ-C2/cyclic-
      # demand at Build.cpp:1470-1473/:1467-1469 forbid induction-owned
      # demanded content entirely). Stage D's "induction machinery PER
      # INSTANCE" is the direct inverse of that fence — the reconciliation
      # (lift the fences AND stratify per instance) is the load-bearing
      # design step, not a mechanical port.
    + full witness matrix: nesting depth 2 with lexical key aliases,
      local linear/nonlinear/mutual recursion, multi-owner
      attach/detach permutations, request/data order flaps
    EXIT GATE: semantic property tests (I0 + permutation invariance
      over same-epoch request/input orderings) across the matrix.

## §7. Standing open decisions (owner)
##     SUPERSEDED 2026-08-02: the consolidated, deduplicated decision
##     queue is owner-adjudication-brief.md (23 items, TIER 1-3, with
##     alternatives + evidence + panel recommendations). The items below
##     are the historical seed list; every one appears in the brief
##     (1→D1.1+D2.x R-DIFF; 2→D1.5/D1.6 the MASTER decision; 3/4→TIER 2;
##     5→ledger-entry-AW-draft.md; 6→Stage B resolved-preserve /
##     Stage D E1 V-PI-vs-V-CW). Adjudicate from the brief, not from here.

    1. Ratify I0-before-Stage-C + the tagged-binary oracle. (Now sharpened:
       the R-DIFF pub arm is corpus-unexercised — I0 or a new witness must
       cover differential-pub demand-instance before Stage C's goldens can
       reference it.)
    2. Inadmissible-extraction semantics + fate of today's rejects.
    3. ExtractionPolicy named as provisional seam (cost work meets it).
    4. RoutedResult first realization = shared-pub sentence in the doc.
    5. Ledger: §20(AW) regional-epoch-open entry (not yet written —
       the ledger stops at (AV)/cost-simulator, which this turn
       supersedes).
    6. (NEW) Stage B must decide the fate of ExtractPrimaryProcedure's
       two-procedure split (§4b Step 9) under FrozenRegionalProgram, and
       Stage D must reconcile the unstratified/non-inductive keyed-instance
       reality (§4b Step 6 + the §1 fences) with "induction PER INSTANCE".

---

# Part R (2026-08-03, session 3) — the region-model current-architecture pseudocode, fleet-verified at tip

This part SUPERSEDES `region-model-pseudocode-seed.md` Part 1. A verification
+ extraction fleet re-anchored the seed's four Part-1 sections against the
branch tip and deep-dived the three thin areas the seed only glossed. Every
pseudocode block below is the fleet's CORRECTED text — drifted and broken
claims from the seed are fixed in place; the DRIFT LEDGER at the end records
each finding as `claim -> reality -> fresh anchor`. Line anchors are at the
`keyed-instances` tip as of 2026-08-03; re-verify before building, per the
standing rule.

Fleet tally: **anchors verified 36** (1.1: 15, 1.2: 12 exact incl. 5
InstanceStore API points, 1.3: 6, 1.4: 6 — plus the 3 extraction areas'
anchor tables, ~60 more file:line points, all confirmed-live), **drifted 8**
(1.1: 4, 1.2: 5, 1.3: 1, 1.4: 1 — one 1.2 item is a confirm-unchanged, net
7 real drifts), **broken 1** (1.2 kInstanceDeath gate). No section was found
structurally wrong; the seed's shape held, the drift was in enclosing-function
names, gate conditions, and data-model framing.

## R.1.1 The demand transform (flat lowering) — `lib/DataFlow/Demand.cpp`

`QueryImpl::ApplyDemandTransform` (Demand.cpp:385), run at the post-
`ConnectInsertsToSelects` slot in `Build.cpp` (:2576 then :2593). MODE-GATED:
returns a no-op when `demand_mode==false` (:393-395). The forcer-proc
injection that ultimately consumes this pass's output registry lives in a
DIFFERENT file/pass — `lib/ControlFlow/Build/Build.cpp` — not here (see tail,
and R.1.5).

```
ApplyDemandTransform(module, log, demand_mode, demand_retract):        # :385
  if not demand_mode: return true                     # :393, orthogonal to 4 golden modes
  if module.DemandMessagesFabricated(): reject("re-entry")             # G2, :399
  bound_queries := [ rel for rel in relations                          # :417-429
                     if rel.decl.IsQuery() and rel.decl.Arity()>0
                     and any(param.Binding()==kBound for param in decl.Parameters()) ]
  if bound_queries empty: return true                                  # :431-433
  if |bound_queries| > 1: reject("multiple bound queries")             # C14 fence, :435-439
  q_rel, q_decl := bound_queries[0], q_rel.declaration
  q_insert := q_rel.inserts[0]           # exactly one materialization required, :446-451

  # ---- Loop 1 (Phase 1), :477-795: per adornment, LOCATE + CHECK, NO mint ----
  seen_variants := {}                    # dedup, mirrors Build.cpp's own UniqueRedeclarations idiom
  plan := []
  known_consumers := {}                  # pass-level union, populated across every adornment
  for redecl in q_decl.UniqueRedeclarations():                         # :477
    binding := redecl.BindingPattern()
    if binding in seen_variants: continue                              # :479
    seen_variants.add(binding)
    bound_indices := [param.Index() for param in redecl.Parameters() if Binding()==kBound]
    if bound_indices empty:                                            # :489-499
      reject("a demanded query name with an all-free sibling adornment")
      # (fires on THIS redecl being the all-free one, not a conjunction test)

    # Step 2 (:507-600): trace each bound column of q_insert back through
    # forwarding TUPLEs / the relation's own post-Connect MERGE to a single
    # full-width reader TUPLE `q_read` over p's MERGE `p_merge`; record
    # `q_consumer` (q_read's sole consumer) and `p_bound` (bound positions in p).
    # All bound columns must land on the SAME (p_merge, q_read, q_consumer);
    # multi-clause queries / mismatched targets -> reject.

    # Step 3 (:607-783): per member (rule body) of p_merge.merged_views:
    #   - walk the body tree; reject on NEGATE/AGG (demand sink, :648-651),
    #     a second read of p (self-join, :656-660), or any un-witnessed view.
    #   - for each bound position, trace its source and classify ONE of three
    #     GuardSite kinds (must agree across all bound positions of one member):
    #       kReadAtTuple  (:684-701): member reads p_merge directly (no join),
    #                      value already sits at its own adornment position.
    #       kBaseAtom     (:702-709): a base rule — the value's source is a
    #                      TUPLE reading a message-receive SELECT (a leaf atom).
    #       kPushDown     (:721-755): a recursive rule — the value flows out of
    #                      a JOIN whose input traces to a read of p_merge (the
    #                      demanding subgoal); JOIN-18 push-down site.
    #     A value NOT at its own adornment position anywhere -> "sideways
    #     (non-From-preserving)" reject (:693, :748) — the left-linear /
    #     second-adornment case.
    known_consumers.add(q_consumer); known_consumers.add(site.consumer for site in sites)
    plan.append(PerAdornment{redecl, bound_indices, p_bound, q_read, q_consumer,
                             p_merge, sites, pushdown_reads})            # :791-794

  # Step 4 (ONCE, between loops, on the PRE-MINT graph), :797-826:
  for reader in CollectColUsers(p_merge):        # every consumer of p
    if reader is not a full-width TUPLE over p_merge: reject(...)
    for ruser in CollectColUsers(reader):
      if ruser not in known_consumers: reject("untraced consumer of the demanded relation")

  # ---- Loop 2 (Phase 2), :848-1193: per adornment, MINT (Steps 5-10) ----
  pending := []               # deferred (consumer, read) -> guard/restore, for R-DUP
  for a in plan:
    # Step 5 (:858-914): fabricate demand__<q>_<adorn> message + "_local" decl
    #   under the reserved lexable `demand__` prefix (G3 collision-checked
    #   BEFORE fabrication); FabricateDemandMessage/FabricateDemandLocal live
    #   in lib/Parse/Demand.cpp (:163-217 / :219-254) — real ParsedMessageImpl/
    #   ParsedLocalImpl minted via a synthetic display-buffer lex (A7/G1).
    d_msg, d_local := module.FabricateDemandMessage(...), module.FabricateDemandLocal(...)

    # Step 6 (:916-1041): mint the IO+receive SELECT for d_msg (the root seed),
    # the demand relation's MERGE (d_merge, ALWAYS a MERGE — even one-member,
    # matching Connect's real shape), a root member (TUPLE chain over the
    # receive) and one propagation member per distinct demanding-subgoal read
    # (a TUPLE projecting the adornment's columns off that read), and a shared
    # derived-d_p reader TUPLE `d_reader` over d_merge (recipe N2/N4).

    # Step 7 (:1052-1084): for each body GuardSite in a.sites:
    #   mint guard JOIN  d_reader ⋈ site.read  pivoting site.pivot_pos     # MintGuardJoin, :162-208
    #   stamp a GuardAnnotation{kind, side=kDReader, role=kBody, ...}
    #   if site.kind != kReadAtTuple: mint a restoring TUPLE (re-establish
    #     site.read's original column order)                              # :212-226
    #   pending.append(PendingRewire{site.consumer, site.read, guard, restore, kind})

    # Step 8 (:1086-1127): the QUERY-PROJECTION GUARD — a SEPARATE, once-per-
    #   adornment guard (NOT a body GuardSite): mint a fresh `raw_seed` TUPLE
    #   over the same receive, then mint guard JOIN  q_read ⋈ raw_seed  pivoting
    #   p_bound (JOIN-7/TABLE-23, the "raw seed" site — distinct from d_reader).
    #   stamp GuardAnnotation{kReadAtTuple, side=kRawSeed, role=kQueryProjection}
    #   pending.append(PendingRewire{q_consumer, q_read, guard, restore=None, kReadAtTuple})

    # Step 8b (:1129-1144): register RecognizedSubgraph{forcing_index, p_merge,
    #   p_bound, q_insert, guard_indices} — the keyed-instance census's authority.

    # Step 9 (:1146-1182), ALWAYS-ON TRIPWIRE: assert d_merge has >=1 member
    # whose TUPLE chain traces to the fabricated receive; else fprintf+abort.

    # Step 10 (:1184-1191): demand_forcings.append(
    #   QueryDemandForcing{query=a.redecl, message=d_msg, bound_params=bound_indices})
    #   — REGISTERS ONLY. The forcer proc itself is NOT built here (see tail).

  # ---- R-DUP grouped rewire, ONCE, AFTER Loop 2 (not per-adornment!), :1195-1264 ----
  group pending by (consumer, read):
    if group.size()==1:                          # SINGLETON: today's exact direct rewire
      RewireConsumer(consumer, read, guard-or-restore-output-cols, replacement=guard-or-restore)
    else:                                         # MULTI (R-DUP): >=2 adornments share a site
      members := [ (p.restore or freshly-minted restore over p.guard) for p in group ]
      um := MERGE(members)                        # union of restored read-schema outputs
      RewireConsumer(consumer, read, um's cols, replacement=um)

  # Step 11 (:1266-1332), ALWAYS-ON census: guard_annotations count reconciles
  # with live-stamped-view count + folded count (OWN-3); recognized_subgraphs
  # count == demand_forcings count; every forcing with >=1 guard has >=1 kBody
  # guard (g8 belt — ResolveLiveRecognition/Rel.cpp needs a kBody stamp).

  module.MarkDemandFabricated()                   # :1334, closes the single-shot window
  return true
```

Key property (unchanged): the demand relation is JUST ANOTHER RELATION; the
region call is an IMPLICIT guard-JOIN; the key stays a COLUMN. No explicit
call/region node.

TAIL, in a DIFFERENT file/pass (not part of this DataFlow transform): the
forcer proc that actually SEEDS the demand relation at runtime is built later,
during ControlFlow `Program::Build`, from the `demand_forcings` registry this
pass populated — `BuildQueryInjectorFromRegistry` (lib/ControlFlow/Build/
Build.cpp:413-490; one input var per bound param, a VECTORAPPEND onto the
message's payload vector, a CALL into the fabricated message's handler
procedure), dispatched per query by `BuildQueryInjectorProcedure` (:507-520,
matched on `(query, BindingPattern)` — the D3.a.3 multi-adornment belt that
keeps two adornments of one name from cross-wiring). Full trace: R.1.5.

## R.1.2 The keyed-instance (nested) lowering — `lib/Rel/Rel.cpp`

`-demand-instance` (implies `-demand`; a lowering selector, not a pass). Gated
on `context.demand_instance_enabled`. Flag-off: zero instance ops minted
(the stored `RecognizedSubgraph` handles are never dereferenced).

```
# lib/ControlFlow/Build/Stratum.cpp:2117
BuildStratumPhases(impl, context, query):
  ...
  dr_flow := BuildDRInventory(impl, context, query, recursive_sccs)  # :2149
  ...

# lib/Rel/Rel.cpp:1803 — NOT BuildStratumPhases itself; BuildStratumPhases's
# callee. Builds the whole DR-IR op inventory (crossovers, product arms,
# group updates, THEN keyed instances, THEN branches/joins).
BuildDRInventory(impl, context, query, ...):
  ... crossovers, product arms, group updates (aggregates/KV) ...
  if context.demand_instance_enabled:                        # :2065
    BuildSubgraphInstanceOps(flow, impl, context, query, scc_map)   # :2066
  ... branch/join inventory ...

# lib/Rel/Rel.cpp:1036 (anchor UNCHANGED — E-142 deliberately pins it)
BuildSubgraphInstanceOps(flow, impl, context, query, scc_map):
  # §2.1 ABA-SAFE re-resolve: NEVER deref a stored RecognizedSubgraph
  # QueryView (they dangle post-Optimize). Re-derives demand/input/pub
  # tables from LIVE GuardAnnotationIndex-stamped views + parse identity.
  lr := ResolveLiveRecognition(impl, query)     # :934-1023

  for rs in query.RecognizedSubgraphs():        # :1052, one per DemandForcing
    ri := lr.by_forcing[rs.forcing_index]
    if ri missing or !ri.ok: continue            # fully-dead forcing, ABA-safe skip
    pub, demand, input := ri.pub_table, ri.demand_table, ri.input_table

    # HP-4 recognizer-refusal belt: input MUST be a plain table-bearing view
    # (never MAP/NEGATE/AGG/KVIndex) — a belt, not the induction-owned fence.
    assert not (ri.input_view.{IsMap,IsNegate,IsAggregate,IsKVIndex}())  # :1066-1070
    # NOTE: the induction-owned / cyclic-demand fences are NOT here. They are
    # an EARLIER, separate pre-pass in Program::Build (Build.cpp:1451-1499),
    # gated on demand_instance, walking LIVE guard JOINs (grouped by forcing,
    # not via RecognizedSubgraphs()) BEFORE ProgramImpl/the DR-IR even exist:
    #   cyclic_demand      := ViewSelfReachable(guard.joined[0])   -> reject
    #   recursive_content  := guard.joined[1].InductionGroupId() or
    #                         ViewSelfReachable(joined[1]) or
    #                         any predecessor.InductionGroupId()   -> reject
    # (a THIRD historical fence, differential-summarized-input, was LIFTED at
    # D3.a.2 — R-DIFF handles it via input_diff below, no reject remains.)

    diff       := TableIsDifferential(pub)         # P-STORE
    input_diff := input and TableIsDifferential(input)   # separate axis (D3.a.2 e3)
    sid := flow.instances.size()
    flow.instances.push_back(DRInstance{demanded_view, pub_view, diff, ...})

    # ---- kSubgraphInstantiate: birth/rebuild + band-(b) publish ----
    inst := DROp(kSubgraphInstantiate, ctx=kSeed, table_op_table=pub, sign=+1,
                 demand_table=demand, input_table=input,
                 instance_store_id=sid, forcing_index=rs.forcing_index)
    inst.effects := InstantiateEffects(diff, input_diff, pub, demand, input)  # :782-876
    #   band-(a1) birth:        kVecDrain(demand, NetAddition)          # :786-790
    #   band-(a2) edge-add:     kVecDrain(input,  NetAddition)          # :792-801
    #   band-(a2') edge-remove: kVecDrain(input,  NetRemoval)           # :803-816
    #     present ONLY if input_diff — TWO DRAINS, NO RECYCLE (R-A2-TRIGGER)
    #   [shared] kInstanceDemand read(demand); ONE Present-filtered leaf
    #     kFlagRead(input, pred=kPresent) — the rescan mold ALL THREE bands
    #     above funnel into (never 3 separate rescans)                  # :823-828
    #   band-(b) emit_touched (ONE-NET-PAIR):
    #     kInstanceRebuild(pub,+1); kStateEmit(pub); kStateOld(pub)      # :830-844
    #     if diff:   for sign in {+1,-1}: kCounter + kInIReadFrozen +
    #                kVecAppend(add/delete queue)                        # :846-865
    #     else:      ONE kCounter(+1); ZERO appends (R-MONO)             # :866-874
    # rederive body: ONE arm, kAccess(input, pred=kPresent,
    #   lowering=kSectionWalk, bound=instance-key cols) -> kFold(+1) into pub
    flow.ops.push_back(inst)                                             # :1159

    # ---- kInstanceDeath (R-DIFF, D3.a.1) ----
    # Gate is the DEMAND table's differentiality (== demand_retract, since the
    # fabricated demand message goes @differential iff -demand-retract is on)
    # — NOT "input differential AND demand_retract"; input_diff is unrelated.
    if demand and TableIsDifferential(demand):                           # :1163
      death := DROp(kInstanceDeath, ctx=kSeed, table_op_table=pub, sign=-1,
                    demand_table=demand, instance_store_id=sid,
                    forcing_index=rs.forcing_index)
      death.effects := DeathEffects(pub, demand)   # :878-900, zero-counter signature:
        # kVecDrain(demand, NetRemoval); kInstanceDemand read(demand);
        # kStateOld(pub); kInstanceRebuild(pub,-1) — EXACTLY ZERO of
        # {kStateEmit, kCounter, kInIReadFrozen, kVecAppend} (§18(B) teeth)
      flow.ops.push_back(death)

    # ---- kInstanceSeal (always minted; self-lowered) ----
    flow.ops.push_back(DROp(kInstanceSeal, table_op_table=pub,
                            instance_store_id=sid, effects=[SealEffect(pub)]))

    flow.instance_stratum[sid] := 1 + max(ready_after(demand), ready_after(input))
```

Runtime state: `InstanceStore<Key,RowT>`
(`include/drlojekyll/Runtime/InstanceStore.h`) — the keyed-instances TRANSPOSE
of `StateCellStore` (StateCell.h): a two-WORD cell per group there becomes a
two-BUFFER (double-buffered nested `Table<RowT>`) relation per dense instance
id (iid) here — never conflate the two shapes (the seed's "two-word cells"
framing borrowed the wrong sibling class):

```
InstanceStore<Key,RowT>:
  FindInstance(key) -> iid | kNoInstance          # :103, memo lookup, no mint
  FindOrAddInstance(key) -> iid                   # :109, mints empty current+frozen
  TouchCurrent(iid) -> Table&                     # :134, PUBLIC band-(a) entry:
                                                   #   Touch(iid) [private, :304] + return current buffer for the TryAdd rescan
  Current(iid) -> Table&                          # :142, this epoch's rebuilt content
  Frozen(iid) -> const Table&                     # :143, last epoch's sealed snapshot
  WorkingOccupied(iid) -> bool                    # :167, current[iid].NumRows()>0
  SealedOccupied(iid) -> bool                     # :173, batch-start occupancy bit
  Touched() -> sorted-unique Vec<iid>              # :148, band-(b) iteration order
  TouchedFlag(iid) -> bool                        # :153
  KeyAt(iid) -> const Key&                        # :157
  Seal()                                          # :180, per touched iid: pointer-swap
                                                   #   current<->frozen, Reset new current,
                                                   #   update sealed_occupied; R-MONO-only
                                                   #   debug belt asserts frozen⊆current
  RecycleCurrent(iid)                             # :222, Touch(iid) + current.Reset() —
                                                   #   death arm + same-epoch rebuild; idempotent
  DebugValidate()                                 # :228, seal/occupancy/non-aliasing asserts
  # private: dense append-only iid space (NumInstances monotone-forever);
  # open-addressing slots[] keyed by hash, Rehash() at 8/9 load — :244-300
```

So the InstanceStore ALREADY IS the memo table of activations (rows = keys);
the recognition is subgraph-shaped (SIP-reachable), not SCC-shaped.

## R.1.3 The recursion / fixpoint lowering — `lib/ControlFlow/Build/` + `Stratum.cpp` + `lib/Rel/Rel.cpp`

```
# --- Stage A: dataflow-IR stratification (runs once, at Query::Build tail) ---
Stratify(log):                                          # QueryImpl::Stratify, Stratify.cpp:124
  sources(v) := predecessors(v) u {negated_view(v)}
              u {INSERT i : (i,v) is an INSERT->SELECT decl seam}   # :138-172
  run iterative Tarjan over sources; SCC pop order = stratum id     # :174-232, num_strata :235
  view->stratum := popped SCC id                                    # :237-240
  model->stratum := max stratum over the model's member views       # :242-267
  reject unstratified negation (negate->stratum == negated_view->stratum)   # :272-297
  reject unstratified aggregation/KV (agg->stratum == input_view->stratum) # :299-344
  V-SCC-SEAM (always-on, no NDEBUG guard; F26):                     # :346-381
    every multi-view stratum must have an inductive MERGE (AsMerge()
    && induction_info) OR be closed by an IO seam (message pub/recv);
    else fprintf+abort("source-less forwarding cycle survived to Stratify")
    # precondition: canonicalization always followed by dead-cycle
    # collection (EliminateDeadFlows under df.dfe, else CollectDeadCycles)
  #ifndef NDEBUG: cross-check SCC condensation vs IdentifyInductions   # :383-440

# --- Stage B: the EAGER-INSERTION descent's own induction (separate, earlier) ---
# For a view that actually gets a ProgramInductionRegion built (narrower than
# "carries an InductionGroupId" -- Stratum.cpp:119-125), the descent
# (Join.cpp/Product.cpp/Induction.cpp, still hand-coded) builds ITS OWN
# INDUCTION, keyed per-view in context.view_to_induction:
GetOrInitInduction(view, parent):                        # Induction.cpp:625
  if context.view_to_induction[view] exists: return it
  induction := impl->induction_regions.Create(parent)     # :659
  register a ContinueInductionWorkItem                    # :674
  view_to_swap_vec[view] := a fresh loop-carried swap VECTOR  # Induction.cpp/Join.cpp/Product.cpp
  # BuildFixpointLoop (Induction.cpp:138) feeds NEW rows in via
  # AppendToInductionInputVectors, iterates to a monotone fixpoint,
  # and fills the induction's NET-ADDITIONS output vector
  # (AppendToInductionOutputVectors) for non-inductive successors.
  # NEVER does OVERDELETE/REDERIVE/INSERT -- monotone-only.
  # (This is the mechanism the seed's 1.3 MIS-attributed to LowerDRRounds:
  #  view_to_swap_vec + inductive-MERGE-as-loop-header is THIS, not the
  #  stratum-phase round shells below.)

# --- Stage C: the DR-IR strata authority (per-batch entry-procedure build) ---
BuildStratumPhases(query):                                # Rel.cpp, called from Procedure.cpp
  dr_flow := BuildDRInventory(...)                        # branch/join/crossover/product/ingest inventory
  DeriveDRStrata(dr_flow, ...):                            # Rel.cpp:3093 -- MONOTONE INTEGER LIFT
    owner_stratum(table) := max spec stratum over table's views
    init: branch/join/crossover/product/group_update strata from view->stratum
    drain_stratum[table] := owner_stratum(table)
      for every differential table NOT induction-owned      # Rel.cpp:3119: TableIsDifferential
                                                             #   && !TableIsInductionOwnedDR
    fixpoint: lift(slot, value) { if slot < value: slot = value; changed = true }  # :3192-3197
      repeat branch/join lifts (ready_after/ready_across rules) until !changed
  ValidateDRInventory / ValidateDROps / LinearizeAndValidateDRFlow(dr_flow):
    Kahn-linearize the op dependence graph, band-key tie-break         # Rel.cpp:5023+
    V-LINEAR (pinned order is a topo sort) / V-LOOP (loop-carried RAW
    has a matching WAR, drain-before-refill) / V-READY (no read of a
    strictly-higher stratum) / V-BAND-HAZARD (intra-scope edges never
    run backward against band-key order)

  for stratum in 0..num_strata ascending:                  # Stratum.cpp:2446-2490
    LowerDRFlow(...)          # ACYCLIC band: seeds, join section walks,
                               #   crossovers, product arms, single-pass
                               #   claim drains + immediate frontier filters
    LowerDRRounds(dr_flow, stratum):                        # Stratum.cpp:1799
      for each (OVERDELETE, INSERT) round-shell pair whose
          drain_stratum == this stratum:                    # :1817-1827
        scc_tables := RoundTables(dr_flow, round)            # per-table Delta frontiers, :1675-1681
        del_loop := LowerRoundBody(round=OVERDELETE):         # :1691-1792, a FRESH INDUCTION
          loop->vectors := {TableDeltaVector(t, ClaimedDeleteFrontier) : t in scc_tables}
                                                              # loop-carried state = per-TABLE
                                                              #   Delta vectors, NOT view_to_swap_vec
          body (ACYCLIC, straight-line SERIES):
            VECTORCLEAR each round Delta                     # :1712-1722
            EmitClaimDrain(table, is_del) per scc_table        # split ClaimGate:
                                                              #   kDelGateCnrNonPositive (C_nr<=0)
            EmitJoinFire per kFixpointFire op of this sign/group
            EmitSeedLoop (CHAIN_FOLD) per kChainFold op
            EmitRetireFrontier per scc_table                  # :1786-1790
        EmitRederive per scc_table into del_loop's OUTPUT region   # :1841-1843 (REDERIVE)
        add_loop := LowerRoundBody(round=INSERT):              # mirror; ClaimGate kAddGateTotalPositive
        EmitFrontierFilter (del then add) into add_loop's OUTPUT  # :1861-1870, DEFERRED (E-17/V-DEFER):
                                                              #   both signed net frontiers built only
                                                              #   AFTER INSERT quiesces (spec S5.0)
    # DIVISION OF LABOR: a recursive SCC WITH a ProgramInductionRegion is
    # fed/drained by Stage B's induction and left ALONE here; a recursive SCC
    # WITHOUT one (the doc's example: JOIN-terminal linear recursion of
    # transitive closure) gets its OVERDELETE/REDERIVE/INSERT from THIS
    # machinery (drain_stratum populated only for non-induction-owned tables).

  LowerCommitSweeps(dr_flow, seq):                           # Stratum.cpp:2067
    # called ONCE, from Procedure.cpp:589, AFTER the whole ascending
    # stratum loop -- the epoch-boundary SOLE publish point (full trace R.1.6)
    for each kCommitSweep DR op:
      emit COMMITSWEEP(table)
      if differential + has a publish target: attach the @differential
         transmit message (net presence changes)                # :2094-2099
      if this table backs an R3 StateCell: attach seal_statecell_id
         (sealed := working, AFTER emit_touched read working as "new")  # :2100-2105
```

## R.1.4 The Stage-A identity/contract layer (LANDED; verified against tip)

```
Query::Build tail (Build.cpp:2637-2649, after impl->Stratify(log) at :2638):
  impl->row_contracts := InferConservativeRowContracts(impl.get())  # RowContract.cpp:365
    # PRECONDITION (RowContract.h:57-59): post-Stratify (view->stratum set by
    # every live view, Stratify.cpp:238), post-FinalizeColumnIDs, post-
    # TrackConstAfterInit. A PURE, RECOMPUTABLE fn of the FINAL graph — never
    # materialized during Optimize (RowContract.h:14-18, the F1 lesson: no
    # satellite annotation for CSE/canonicalization to migrate).

    views_by_depth := stable_sort(live views, by view->depth)  # :368-382
      # READ-ONLY read of the frozen `depth` FinalizeDepths already set; must
      # NOT call ForEachViewInDepthOrder (that RESETS depth as a side effect
      # and would silently re-order downstream .rel/.ir/codegen).

    # Phase 0 (:384-390) — per-stratum view histogram (deterministic):
    stratum_size[s] := |{ v in views_by_depth : v.stratum == s }|

    # Phase 1 (:392-400) — cyclic rule: every view on a MULTI-VIEW stratum
    # (stratum_size>1, i.e. a recursive SCC) gets the conservative key
    # directly from SCC structure, dissolving the would-be contract fixpoint:
    for v in views_by_depth:
      if v.stratum.has_value() and stratum_size[v.stratum] > 1:
        out[v] = { visible_fields = AllFields(v), member_key = AllFields(v) }

    # Phase 2 (:402-408) — acyclic per-operator transfer, ONE pass in
    # topological (frozen-depth) order over the remaining single-view strata
    # (cross-SCC edges go low->high depth, so depth order is topological):
    for v in views_by_depth:
      if v not in out:
        out[v] = TransferContract(v, out)          # :158-248, flat-key,
                                                     # value-id intersection
          # TUPLE[kDistinct]         -> key = AllFields(v)          (:182-183)
          # TUPLE[kMember]/CMP/
          #   NEGATE/INSERT          -> key = passthrough(sole producer)
          #                                    (:184-198)
          # JOIN                    -> key = union of per-input mapped keys
          #                                    (:200-215)
          # AGGREGATE               -> key = group_by++config OUTPUT PREFIX,
          #                             positional not id-matched (:217-230)
          # SELECT/MERGE/MAP/
          #   KVINDEX/other         -> key = AllFields(v)   (sound fallback,
          #                                    :232-238)
          # empty mapped key        -> falls back to AllFields(v); a member
          #                            key is NEVER empty (:242-244)

  ValidateRowContracts(impl.get(), log)     # RowContract.cpp:413-419, H-A7
    # All four run UNCONDITIONALLY (no feature gate); "belt-only" below is an
    # epistemic distinction (tautological on a correct pipeline), not a
    # gating difference:
    CheckContractCensus(impl)      # :254-272  ALWAYS-ON:
                                    #   |row_contracts| == |live views|,
                                    #   exactly one per live view
    CheckMemberKeyRealized(impl)   # :276-296  ALWAYS-ON: every live view's
                                    #   key non-empty (when it has columns)
                                    #   and every FieldId resolves live
    CheckNoCollapse(impl)          # :318-336  BELT-ONLY (E-A2, :298-317):
                                    #   the real "unproven collapse" reject
                                    #   is UNSOUND at Stage A's conservative-
                                    #   AllFields-producer-key model (every
                                    #   dropped column looks like a dropped
                                    #   key column even for a benign
                                    #   projection) — defers to Stage B
                                    #   Minimize; only the sound residual
                                    #   (mapped key nonempty, already implied
                                    #   by V-MEMBERKEY-REALIZED) is checked
    CheckAggInputKey(impl)         # :340-361  ALWAYS-ON: every aggregate's
                                    #   summarized-input view has a realized
                                    #   member key

ProjectionRole (QueryTupleImpl::ProjectionRole {kMember, kDistinct},
  Query.h:718-731, field at :765; set once at mint, never mutated) folded
  into QueryTupleImpl::Equals ONLY (Tuple.cpp:308-310 — a role mismatch is
  an unconditional CSE refusal branch, so a member-preserving projection can
  never be folded into a set-collapsing one and vice versa). Deliberately
  NOT folded into QueryTupleImpl::Hash (Tuple.cpp:25-58, NOTE(H-A2) at
  :35-49): Hash is not on the CSE bucketing path (CSE buckets by
  cse_color::Refine, decides membership via Equals — Optimize.cpp), and
  folding role into Hash would perturb order-sensitive tie-breaks that DO
  read Hash (Merge.cpp canonicalization sort, ControlFlow Program.h .Hash()
  ordering), flipping goldens for no correctness benefit.

-contract-out dump (Format.cpp:1516-1727; wired at
  bin/drlojekyll/Main.cpp:154-160, drained alongside -df-out): flat-key
  contracts ONLY (member_key; no support value, no candidate-key antichain —
  Stage-A scope, RowContract.h:20-23), one block per live view in
  KIND-TAGGED DET_SEQ order — the SAME per-kind DefList traversal QueryDF
  (-df-out) uses, NOT a raw column/view id sort (Format.h:26-27;
  Format.cpp:1535-1567, which also re-runs a det_seq-bijection V-CONTRACT-
  CENSUS over the dump traversal itself). The number after the kind tag
  (e.g. "^tuple.7") IS v.DeterministicOrder()==det_seq (Query.cpp:437-438),
  NOT QueryColumnImpl::id/FieldId — those are the separate id space inside
  the rendered key= tuples:
    "<kind> ^<kind>.<det_seq> (typed visible fields)"
    "  role=<member|distinct|n/a> key=(member_key, by column name)"
    "  input_key=(...)"                       # aggregate views only, H-A5
    ...
    "census: views=<V> contracts=<V> role{distinct=<d> member=<m> na=<n>}"
    "        agg_input_key_ok=<a> collapse_error=0"
  Pure byte-compare, no order-free field, OPT-MODE-only pinning.

-dot-out DOT twin (Format.cpp:43-141; wired at Main.cpp:141-144):
  one "subgraph cluster_stratum_<n>" per MULTI-VIEW stratum (:56-74;
  singleton strata stay top-level; advisory, never golden-pinned — the
  Stage-B regional dump's DOT twin will cluster by RegionId the same way);
  each view's node label additionally carries "ROLE <member|distinct>"
  (TUPLEs only) and "KEY (...)" resolved from row_contracts by column
  name/variable, falling back to "f<id>" when unresolved (:100-141).
```

## R.1.5 The forcer / injector path (bound `#query` demand-seed injection at runtime)

Registry (DataFlow) -> ControlFlow injector proc -> generated C++ forcing
surface -> one synchronous epoch -> keyed-instance tail. THE KEY PROPERTY the
seed's one-liner ("inject the demand SEED via a forcer proc") left implicit:
a forced query call runs a FULL EPOCH, SYNCHRONOUSLY, INLINE, before the query
reads anything — not a deferred/async injection; there is no "pending demand"
state visible to the driver between inject and read.

```
## A. Registry population — lib/DataFlow/Demand.cpp, Loop 2 / Phase 2
    # struct QueryDemandForcing { ParsedQuery query; ParsedMessage message;
    #   std::vector<unsigned> bound_params; }  — Query.h:967-979 (PUBLIC record)
    # impl->demand_forcings : std::vector<QueryDemandForcing> — lib/DataFlow/Query.h:1199
    ApplyDemandTransform(...):                          # Demand.cpp:385
      ... Phase 1 locate+check ...
      for pa in plan:                                    # Phase 2, PER ADORNMENT
        d_msg  = FabricateDemandMessage(name, bound_types, demand_retract)  # :896
        forcing_index = |demand_forcings|                # :1048, BEFORE guard mint
        ... mint seed / guards / RecognizedSubgraph / TRIPWIRE ...
        demand_forcings.emplace_back(                     # :1190-1191, STEP 10 registry write
          QueryDemandForcing{ParsedQuery::From(pa.redecl), d_msg, bound_indices})
    # READ SEAM (Demand.cpp:295-317):
    #   Query::DemandForcings() -> const ref (no copy)
    #   Query::IsDemandMessage(m) -> any(f.message==m) — sole codegen suppression predicate

## B. ControlFlow: forcer proc construction — lib/ControlFlow/Build/Build.cpp
    # Context wiring (Build.cpp:1517-1523), at Program::Build entry:
    context.demand_forcings = &query.DemandForcings()    # empty unless -demand
    context.demand_instance_enabled = demand_instance
    # messsage_handler[ParsedMessage]->PROC* populated EARLIER in BuildIOProcedure
    # (Procedure.cpp:690-762), BEFORE BuildQueryEntryPoint runs — injector target
    # always already exists. entry_proc is the SAME PROC* for every message.

    BuildQueryInjectorFromRegistry(impl, context, query, entry, is_retract):  # :413
      assert entry.message.IsReceived()
      assert !is_retract || entry.message.IsDifferential()
      if !messsage_handler.count(entry.message): fprintf+abort   # ADV-8 handler-miss fence
      proc = Create(kQueryMessageInjector)
      proc.input_vars = [one kParameter VAR per entry.bound_params]
      # add_vec ALWAYS created before del_vec (byte-identical id stream):
      if is_retract: add_vec=kEmpty; del_vec=kParameter
      else:          add_vec=kParameter; del_vec=kEmpty-if-differential
      payload_vec = is_retract ? del_vec : add_vec
      body = SERIES[ VECTORAPPEND(payload_vec, proc.input_vars),
                     CALL(messsage_handler[entry.message]),   # arg_vecs: add_vec[,del_vec]
                     RETURN(true) ]

    BuildQueryInjectorProcedure(impl, context, query, is_retract):            # :507
      for entry in *context.demand_forcings:
        if entry.query == query                            # name+arity (operator==)
           AND Decl(entry.query).BindingPattern() == Decl(query).BindingPattern()  # D3.a.3 belt:
                                                            #   LOAD-BEARING — name+arity alone
                                                            #   would cross-wire two adornments
           AND (!is_retract || entry.message.IsDifferential()):
          return BuildQueryInjectorFromRegistry(...)
      if !is_retract and (pred = query.ForcingMessage()):   # legacy @first non-demand path
        return BuildQueryForceProcedureImpl(...)
      return nullopt

    BuildQueryEntryPointImpl(...):                          # :533
      forcer_proc  = BuildQueryInjectorProcedure(is_retract=false)
      retract_proc = BuildQueryInjectorProcedure(is_retract=true)  # AFTER forcer (id stream)
      impl->queries.push(ProgramQuery{query, table, scanned_index,
                                      forcer_proc, retract_proc})
      # Driver (:623-638): one BuildQueryEntryPointImpl per UniqueRedeclaration
      #   -> N adornments => N ProgramQuery entries, each its OWN forcer/retract.

## C. Generated C++ — lib/CodeGen/CPlusPlus/Database.cpp EmitQueryFriends (:1610)
    # forced form deduces Log/Functors: friend q_<pattern>(db, log, functors, bound...)
    # existence body (:1706-1723): assert(initialized_); emit_forcing_call(params) <<< INJECT
    #   then Find()/Present(); cursor body (:1799-1819): inject BEFORE index.First(key).
    # emit_forcing_call = inject_<id>_detail(state.., args) — DIRECT SYNCHRONOUS call.
    # demand-message public entry SUPPRESSED (:1500-1541, IsDemandMessage) — only the
    #   _detail twin exists, reachable ONLY via the injector's CALL.

## D. RUNTIME PATH — one synchronous call chain, one epoch
    driver: q_<pattern>(db, log, functors, K)
      inject_<id>_detail(state.., K)                       # kQueryMessageInjector body
        VECTORAPPEND(add_vec, [K]); <dmsg>_<arity>_detail(state.., add_vec, [del_vec])
          if differential: NETBATCH(add_vec, del_vec)      # dedup+annihilate
          CALL(entry_proc):                                # the §4b Step-2 whole-epoch TREE
            ExtendEagerProcedure (ingest folds + eager web) — the demand receive is
              JUST ANOTHER RECEIVE (MakeStageOneIngestFolds / MakeMonotoneIngestFold)
            BuildStratumPhases: per stratum LowerDRFlow/LowerDRRounds — the demand
              relation's guard JOINs fire in ORDINARY strata (this is how FLAT
              -demand re-derives the demanded subset WITHIN THIS EPOCH)
            PublishDifferentialMessageVectors:
              LowerSubgraphInstances  # ONLY if -demand-instance; band a1/a2/a2'/b
              LowerCommitSweeps        # commit + Seal — table now durably updated
      # ONLY NOW does the friend read Find()/Present() or build the cursor.

## E. V-INST-SOLE keying — N forcings, one pub (D3.a.3), at THREE layers
    # 1. DataFlow: N RecognizedSubgraph, same pub_view, distinct forcing_index/demanded_view
    # 2. DR-IR: N DRInstance (same pub_table ptr), N kSubgraphInstantiate ops
    # 3. Codegen: N InstanceStore<Key_i,Row_i> members (distinct types), one Table<Row>
    CheckInstanceSolePub(flow):                            # Rel.cpp:5009, V-INST-SOLE
      inst_per_pub : map<(pub_table_ptr, forcing_index), count>   # RE-KEYED at D3.a.3
      for op kind==kSubgraphInstantiate: ++inst_per_pub[{op.table_op_table, op.forcing_index}]
      assert every count == 1   # forcing_index is the distinguishing half that LETS
                                 #   N adornments share one pub without tripping the belt
```

## R.1.6 The commit-sweep / Seal epoch boundary — `LowerCommitSweeps` + `LowerSubgraphInstances`

SCOPE: one received message batch == one epoch. From "all ordinary per-stratum
DR machinery has quiesced" through "driver log callbacks fire and every store's
double-buffer/watermark advances." The whole assignment lives inside §4b Steps
6-8; it is NOT invoked from inside BuildStratumPhases/LowerDRFlow/LowerDRRounds.

```
BuildEntryProcedure(...):                                 # Procedure.cpp:931-1048
  ... Steps 1-4 ingest folds / eager web / CompleteProcedure ...
  BuildStratumPhases(...)                                  # :1042 = Rel-IR Step 5
    # GROUP_UPDATE (agg/KV) IS emitted HERE, band 0, at the agg view's OWN lifted
    # stratum (op_band(kGroupUpdate)=0u) — the ASYMMETRY: GROUP_UPDATE is
    # stratified INSIDE the per-stratum walk; SUBGRAPHINSTANCE is NOT.
  PublishDifferentialMessageVectors(...)                   # :1045 = Step 6, the epoch boundary

PublishDifferentialMessageVectors(impl, proc, context):   # Procedure.cpp:505-626
  seq := new SERIES wrapping proc->body
  # 6a. pure-MONOTONE published messages (independent of differential machinery):
  for (message, vec) in context.publish_vecs:
    VECTORUNIQUE(vec); VECTORLOOP(vec){ PUBLISH(message, row, added=true) }; VECTORCLEAR(vec)
  # 6b/6c only if the program has ANY differential machinery:
  if context.dr_flow != null:
    LowerSubgraphInstances(impl, context, *dr_flow, seq)   # :588 — EXACTLY ONCE, after all strata
    LowerCommitSweeps(impl, context, *dr_flow, seq)        # :589 — AFTER (band-b publish
                                                            #   precedes the store Seal)
    # V-INST-EMITTED (:591-620): multiset(emitted {store_id,kind}) ==
    #   multiset(dr_flow.ops of {Instantiate,Death,Seal}) — else abort.

# --- 6b. ONE SUBGRAPHINSTANCE region per store; band order UNREORDERABLE ---
EmitSubgraphInstance(si):                                  # Database.cpp:2341-2779
  # band a0 — DEATH (only if kInstanceDeath minted, i.e. demand @differential / -demand-retract)
  for d in RemovalFrontier(): iid=FindInstance(Key{d})     # NON-adding
    if iid!=kNoInstance: RecycleCurrent(iid)               # Touch + current.Reset() -> EMPTY;
                                                            #   band-b drop-scan retracts the whole frozen set
  # band a1 — BIRTH (demand net-additions)
  for k in DemandFrontier(): iid=FindOrAddInstance(Key{k})
    if !TouchedFlag(iid): emit_instance_rescan(k)
  # band a2 — REBUILD on input net-ADDITIONS (edge-after-demand)
  for e in InputFrontier(): iid=FindInstance(Key{e.key})   # NON-adding
    if iid!=kNoInstance:
      if diff: gate on demand_table.Present(demand.Find(key))  # E8d demand-liveness gate
      if !TouchedFlag(iid): emit_instance_rescan(e.key)
  # band a2' — REBUILD on input net-REMOVALS (iff input @differential; R-A2-TRIGGER)
  if InputRemovalFrontier(): same emitter as a2, third drain into the ONE shared mold
  # -- the ONE shared rescan mold (a1/a2/a2' all call this) --
  emit_instance_rescan(keyexprs):
    assert !WorkingOccupied(iid)                            # V-INST-FRESH
    cur := TouchCurrent(iid)
    for s in input_member.rows:
      if row.key_cols==keyexprs and (!input_diff or input_member.Present(s)):
        cur.TryAdd(row.remaining_cols)
  # band b — PUBLISH, once per Touched(iid) [sort-uniqued], OVERDELETE-FIRST:
  for iid in Touched():
    cur=Current(iid); frz=Frozen(iid)
    if diff: for drow in frz: if cur.Find(drow)==kNoRow:    # dropped = frozen \ current
               pub.SubDerivation(drow,kNonRecursive); DelQueue.Add(drow)   # else ++carried
    for row in cur: if frz.Find(row)==kNoRow:               # born = current \ frozen
      if !diff: pub.TryAdd(row) else: pub.AddDerivation(row,kNonRecursive); AddQueue.Add(row)
    if diff: assert V-INST-PARTITION (born+carried==cur.rows && dropped+carried==frz.rows)
    # SubDerivation/AddDerivation Touch pub's OWN DiffTable — band-b output RIDES the
    # SAME `touched` set the later COMMITSWEEP's member.Commit() drains for pub.
  # self-lowered SEAL (HP-1/OD-5: no separate DR op — folded into this region's tail):
  instance_<sid>.Seal()   # per touched iid: pointer-swap current<->frozen, Reset current,
                          #   sealed_occupied := frozen.NumRows()>0; R-MONO belt asserts frozen⊆current

# --- 6c. ONE COMMITSWEEP region per table, ASCENDING TABLE-ID order, AFTER all instances ---
EmitCommitSweep(region):                                   # Database.cpp:2890-3027
  # MONOTONE table: member.Seal() (append-only high-watermark advance, NEVER renumbers),
  #   then emit_seal(); return.  (assert !region.Message())
  # DIFFERENTIAL table:
  member.Commit(sink):
    for id in touched:
      (nr,r)=counts[id]; assert nr>=0 && r>=0               # per-CLASS non-negativity, asserted
                                                            #   HERE not per-fold (phantom dips legal mid-batch)
      was := kInI; now := (nr+r)>0
      if was != now: sink(RowAt(id), now)                   # <<< THE publish point: if publish_target,
                                                            #   sink = log.<message>_<arity>(fields..., added=now)
                     num_live += now?+1:-1
      set/clear kInI := now
    touched.Clear()
    # sink is a no-op lambda when op.publish_target false — Commit still runs for
    #   counter/flag/num_live side effects on EVERY differential table.
  #ifndef NDEBUG member.DebugValidateCounts() #endif        # re-walks EVERY row
  if member.CompactDead():                                  # AFTER the validator (validator vs PRE-compact ids)
    # NeedsCompaction: dead!=0 AND ( (dead>=num_live && dead>=4096)          # arm (a), documented
    #   OR (NumRows()>=SlotCapacity()*7/8 && dead>=NumRows()/2) )            # arm (b), UNDOC in CLAUDE.md:
    #     a near-Rehash mostly-dead table compacts+reslots in place instead of doubling a dead-heavy slots array
    # CompactRowsInPlace: drop dead, renumber survivors densely [0,num_live); truncate counts/flags.
    for index in table.live_indices: index.Clear()          # rebuild every index over NEW ids
    for cid in [0,NumRows()): for index: index.Add(key_projection(RowAt(cid),index), cid)
  emit_seal()   # STATE_SEAL tail: if region.SealStateCellId(): statecell_<id>.Seal()/SealOne per touched gid
                #   (STATE_SEAL is NOT a separate region kind — an optional field on the COMMITSWEEP region)
```

BAND-KEY ASYMMETRY (validation key vs literal codegen placement — new from
this trace, silent in the seed): the flow-graph's Kahn linearizer (Rel.cpp
`key_of`, used ONLY for dep-edge derivation / V-READY / dump order — never read
by codegen) puts kCommitSweep/kStateSeal/kInstanceSeal in a trailing sentinel
band `Key{lead=2, stratum=max_stratum+1, band=9/10/11}`, but
kSubgraphInstantiate/kInstanceDeath fall to the DEFAULT lead=1 arm with
`stratum = instance_stratum[sid] = 1 + max(drain_stratum(demand),
drain_stratum(input))` — an ordinary mid-stream number that can be well below
`max_stratum+1`. So in the VALIDATED order Instantiate/Death sort as mid-stream
ops (a minimum-readiness bound for hazard-edge validation), while only their
kInstanceSeal is truly pushed to the tail; the LITERAL codegen placement is
fixed unconditionally at the tail by LowerSubgraphInstances regardless. A reader
who assumes "DR-op stratum == where it executes in the region tree" for
Instantiate/Death specifically would be misled.

## R.1.7 The hand-coded eager-web reachability (`BuildEagerInsertionRegions` / `BuildEagerRegion` / `ExtendEagerProcedure`)

TWO INDEPENDENT AUTHORITIES compute the SAME reachable-view set and are
cross-checked (never merged): (I) THE WALK — hand-coded, EMITS the region tree,
temporally FIRST (BuildEntryProcedure Step 2/3); (II) THE DERIVATION —
`BuildDREagerInventory` (Rel.cpp), graph-only, runs LATER (BuildStratumPhases
Step 5), re-floods successors from the QUERY GRAPH alone using the SAME
single-view cut predicate `IsCutSuccessorDR`. Agreement is asserted (SD-4),
never assumed — SD-4 is a GATE (fprintf+abort), so a region-model rewrite must
reproduce authority (I)'s COVERAGE exactly.

```
## THE CUT PREDICATE — IsCutSuccessorDR (Rel.cpp:1567-1573), the single-view authority
IsCutSuccessorDR(context, succ) -> bool:
  if succ.CanReceiveDeletions():  return true   # -> succ's OWN differential stratum phases
                                                 #    (even when inductive — an eager induction
                                                 #    would wrongly mark succ's head induction-owned)
  if succ.IsAggregate() or succ.IsKVIndex(): return true   # -> GROUP_UPDATE folds input into a StateCell
  if context.demand_instance_enabled and succ.GuardAnnotationIndex()!=kNoGuardAnnotation:
    return true                                  # -> (GT-5/OD-4) fed by SUBGRAPH_INSTANTIATE, not the flat web
  return false                                   # walk continues: dispatch via BuildEagerRegion
# NOTE: an INDUCTIVE (InductionGroupId) non-deletion MERGE is NOT cut — reachability is
# TRANSPARENT through it — but BuildEagerInsertionRegions does not recurse into it directly:
# its arm hands off to the fixpoint machinery (Authority A), which re-invokes
# BuildEagerInsertionRegions from INSIDE the cycle/output regions. "cut" and
# "walk-continues-elsewhere-then-rejoins" are DISTINCT buckets.

## WALK ROOTS — BuildEntryProcedure (Procedure.cpp:931-1006)
#   Root 1: all-constant TUPLEs (IsAllConstantTupleDR, :977) under a TESTANDSET init-guard —
#     the ONLY other view dispatchable with NO predecessor in scope; DIRECT BuildEagerRegion(:993).
#   Root 2: every RECEIVE's non-cut Successors() via ExtendEagerProcedure (:1000).
#     The RECEIVE ITSELF IS NEVER DISPATCHED — a pure propagation root.

## PER RECEIVE — ExtendEagerProcedure (Procedure.cpp:14-124)
#   deletion-capable receive: NO eager descent for the fold — both polarities park via
#     UPDATECOUNT (kIngestFold pair from MakeStageOneIngestFolds); `continue` — ZERO dispatch.
#   monotone table-bearing receive: MakeMonotoneIngestFold -> LowerIngestFold; next_parent is
#     the UPDATECOUNT cursor (INGEST-CURSOR-SHAPE assert). >>> THE HOLE <<<
#   monotone table-less receive (R-E42): MakeIngestLoopOp -> LowerIngestLoop; next_parent is a
#     VECTORLOOP cursor (INGEST-LOOP-SHAPE assert). >>> THE HOLE <<<
#   then: BuildEagerInsertionRegions(receive, next_parent, receive.Successors(), table)

## THE DESCENT — BuildEagerInsertionRegionsImpl (Build.cpp:901-1063)
BuildEagerInsertionRegionsImpl(view, parent_, successors, last_table_):
  (parent, table, last_table) = InTryInsert(view, parent_, last_table_)   # Build.cpp:862-897:
    # if view has a model table != already_added, emit UPDATECOUNT fold; successors run only
    # past the FOLD'S ZERO CROSSING — the physical "a row propagates once per batch."
  par := PARALLEL(parent)              # ALWAYS created (CSE dedups repeats)
  # boundary append #1 (D2): deletion-capable, just-folded, non-induction-owned view seeds
  #   its OWN add-queue frontier here.
  any_cut_succ := false
  for succ in successors:
    if IsCutSuccessorDR(context, succ): any_cut_succ=true; continue   # DO NOT recurse
    BuildEagerRegion(pred_view=view, succ, LET(par), last_table)
  # boundary append #2 (D2'/R7): if any_cut_succ OR (monotone table is a non-@never NEGATE's
  #   negated view): seed net-additions frontier — the cut consumer's / crossover arm's seed source.

## THE DISPATCH — BuildEagerRegion (Build.cpp:1208-1323)
# Every (kind,view) dispatch wrapped in CensusEagerMarkerAndBuild (census[{kind,view}]+=1 THEN
#   build_fn) — census == emission multiplicity BY CONSTRUCTION.
#   JOIN(pivots>0): kEagerJoin  -> BuildEagerJoinRegion (DEFERRED, §JOIN)
#   JOIN(0-pivot @product): kEagerProduct -> BuildEagerProductRegion
#   MERGE inductive: BuildEagerInductiveRegion (NO census — marker-silent)
#   MERGE non-inductive: kEagerUnion -> InTryInsert + BuildEagerInsertionRegions(Successors)
#   AGGREGATE/KVINDEX: return (chain-BREAKER)
#   MAP pure: kEagerGenerate -> GENERATOR + recurse ; MAP impure: assert(false) (rejected upstream)
#   COMPARE: kEagerCompare -> TUPLECMP + recurse per branch
#   SELECT: kEagerSelect (reached ONLY bottom-up from an INSERT into its relation) + recurse
#   TUPLE: kEagerForward -> pure forward recurse
#   INSERT: kEagerInsert -> UPDATECOUNT fold; stream=TERMINAL (publish/append/nothing),
#           relation=recurse into its SELECTs
#   NEGATE: kNegateGate -> BuildEagerNegateRegion (CHECKMEMBER gate; only !CanReceiveDeletions
#           negates reach here — recursive negates cut upstream)

## JOIN is DEFERRED (Join.cpp:547-781), NOT immediate recursion:
#   BuildEagerJoinRegion enqueues a ContinueJoinWorkItem (context.work_list), drained by
#   CompleteProcedure. ContinueJoinWorkItem::Run fires ONCE per join_view (not per dispatch edge):
#   VECTORAPPEND/VECTORUNIQUE the merged pivot vec, then MakeJoinEmitOp -> LowerJoinEmit — the
#   ONCE-PER-JOIN TABLEJOIN emission (kJoinEmit DR op, distinct from the per-visit kEagerJoin
#   markers — the 4-vs-2 asymmetry), then CHECKMEMBER gates + InTryInsert + recurse.
#   (Join.cpp:772-780 direct nested-loop branch is DEAD CODE — `true ||` at :742 + assert(false).)

## THE DR-IR MIRROR — BuildDREagerInventory (Rel.cpp:1681-1801)
#   worklist floods from the SAME roots, cut by the SAME IsCutSuccessorDR, propagating straight
#   THROUGH inductive merges (no special case). marker_views := dispatched \ inductive-merges,
#   sorted by (Depth, DeterministicOrder). Per v: n = census[{MarkerKindOfDR(v), v}] copies.
#   SD-4 ORACLE: assert derived_keys == walk_keys (order-free, always-on) — covers the 9
#     DISPATCHED marker kinds (8 IsEagerMarkerKind + kNegateGate; kNegateGate is EXCLUDED from
#     IsEagerMarkerKind but INCLUDED in SD-4).
## V-INGEST-XCHECK Site 5 (Stratum.cpp:2189-2282): the SIBLING oracle for the 2 ENTRY kinds
#   (kIngestFold/kIngestLoop) — never dispatched views, outside BuildDREagerInventory's walk;
#   coverage+payload multiset compare, always-on. Together SD-4 + Site-5 fence all 11 eager kinds.
```

---

## DRIFT LEDGER (Part R) — every fleet finding, `claim -> reality -> anchor`

Recorded per the standing rule: existing sections above (Parts 1-D, §1-§7 of
this file and the seed's Part 1) are NOT edited; contradictions are logged here.

### Broken (1)

- **B1 (1.2) — kInstanceDeath gate.** CLAIM: "if input differential AND
  demand_retract: mint kInstanceDeath." REALITY: the mint gate is
  `if (demand_table && TableIsDifferential(demand_table))` — it tests the
  DEMAND (fabricated forcing-message) table's differentiality, equivalent to
  `demand_retract` ALONE. `input_diff` is a completely separate axis gating
  only band-(a2') inside InstantiateEffects; the code comment explicitly warns
  against this conflation ("NEVER folded into `diff` (P-STORE) or P-DEATH").
  A monotone-input + demand_retract-ON program DOES mint kInstanceDeath (seed
  says it wouldn't); a diff-input + demand_retract-OFF program does NOT (seed
  implies gate is about input). ANCHOR: lib/Rel/Rel.cpp:1161-1174 (gate),
  :1072-1077 (anti-conflation comment).

### Drifted (7 real + 1 confirm-unchanged)

- **D1 (1.1) — all-free sibling reject.** CLAIM: "if redecl has BOUND and an
  all-free sibling: reject." REALITY: the check is per-redecl, not a
  conjunction; it fires on the iteration where THIS redecl's own bound_indices
  is empty. An all-free-ONLY name never reaches the loop (excluded by
  bound_queries). ANCHOR: lib/DataFlow/Demand.cpp:483-499.
- **D2 (1.1) — guard-site taxonomy.** CLAIM: base rule "guard JOINs at the
  raw-seed site"; two site kinds. REALITY: (a) the base-rule body site joins at
  the body's own bound-column SOURCE ATOM (`kBaseAtom`), NOT a raw-seed;
  "raw_seed" is a SEPARATE once-per-adornment query-projection guard (Step 8).
  (b) A third kind `kReadAtTuple` (direct non-join read of p_merge) was
  omitted. ANCHOR: Demand.cpp:126-140, 663-775, 1086-1127.
- **D3 (1.1) — R-DUP nesting.** CLAIM: the R-DUP MERGE-union decision runs
  per-adornment inside Loop 2. REALITY: it is DEFERRED and runs exactly ONCE
  after Loop 2, grouping `pending` rewires across ALL adornments by
  (consumer, read) — a MULTI-guard group only exists once ≥2 adornments
  contributed. ANCHOR: Demand.cpp:848-1193 (Loop 2) vs 1195-1264 (R-DUP).
- **D4 (1.1) — forcer injection locus.** CLAIM: "inject the demand SEED via a
  forcer proc" is the tail action of ApplyDemandTransform. REALITY: this pass
  only REGISTERS the forcing entry; the forcer-proc build + call-site injection
  is a separate mechanism in lib/ControlFlow/Build/Build.cpp during ControlFlow
  Program::Build. ANCHOR: Demand.cpp:1190-1191 vs Build.cpp:413-490, 507-520.
- **D5 (1.2) — enclosing function name.** CLAIM: `BuildStratumPhases` calls
  BuildSubgraphInstanceOps at Rel.cpp:2065. REALITY: the :2065 gate+call is
  inside `BuildDRInventory` (Rel.cpp:1803); BuildStratumPhases is one layer up
  in Stratum.cpp:2117 and calls BuildDRInventory at :2149. Line anchor correct,
  enclosing-function name wrong. ANCHOR: Rel.cpp:1803/2065-2067,
  Stratum.cpp:2117/2149.
- **D6 (1.2) — induction-owned fence locus.** CLAIM: the per-`rs` loop "checks
  rs.input not induction-owned." REALITY: no such check in the loop; what IS
  there is the HP-4 recognizer-refusal belt (MAP/NEGATE/AGG/KVIndex reject,
  Rel.cpp:1062-1070). The real induction-owned/cyclic-demand fences are an
  EARLIER separate pre-pass in Program::Build (Build.cpp:1451-1499), walking
  live guard JOINs before the DR-IR exists. ANCHOR: Build.cpp:1451-1499;
  Rel.cpp:1062-1070.
- **D7 (1.2) — InstanceStore data model.** CLAIM: "two-word sealed/working
  cells; Seal()." REALITY: InstanceStore is NOT a two-word cell store (that is
  StateCell.h). Its surface is `Current(iid)`/`Frozen(iid)` (two whole nested
  `Table<RowT>` double-buffers per iid), `WorkingOccupied`/`SealedOccupied`;
  Seal() is correctly named. The "two-word cells" framing borrowed the wrong
  sibling class. ANCHOR: InstanceStore.h:3-11 (disclaimer), :142-143, :167-175.
- **D8 (1.2) — Touch API.** CLAIM: `Touch(iid); TouchedFlag(iid)` implying
  Touch is directly callable. REALITY: `Touch` is PRIVATE (InstanceStore.h:
  304-311); the public band-(a) entry is `TouchCurrent(iid)->Table&` (:134),
  which calls Touch internally. ANCHOR: InstanceStore.h:134-137, :304-311.
- **D9 (1.3) — LowerDRRounds loop-carried state.** CLAIM: in LowerDRRounds the
  inductive MERGE is the loop-header join point and `view_to_swap_vec` is the
  loop-carried (semi-naive) variables. REALITY: that describes the SEPARATE,
  earlier eager-descent induction (Induction.cpp GetOrInitInduction/
  BuildFixpointLoop, monotone-only, built BEFORE BuildStratumPhases).
  LowerDRRounds creates its OWN fresh INDUCTION per round whose loop-carried
  state is per-TABLE differential Delta frontiers (TableDeltaVector), and it
  applies ONLY to differential tables NOT induction-owned (drain_stratum gated
  by `!TableIsInductionOwnedDR`). ANCHOR: Induction.cpp:625/138;
  Stratum.cpp:111-133/1699-1722; Rel.cpp:3119; Program.h:1915.
- **D10 (1.4) — "id-ordered" dump.** CLAIM: the -contract-out dump is
  "id-ordered." REALITY: it is ordered by kind-tagged det_seq (the same
  per-kind DefList traversal -df-out uses), NOT raw column/view value id. The
  number after the kind tag is `DeterministicOrder()`==det_seq, not
  QueryColumnImpl::id/FieldId (a separate id space inside the key= tuples).
  ANCHOR: Format.cpp:1535-1567; Format.h:26-27; Query.cpp:437-438.
- **D-confirm (1.2, not a drift) — BuildSubgraphInstanceOps anchor UNCHANGED.**
  Verified still at Rel.cpp:1036 (E-142 deliberately pins its line count so
  downstream D3.a anchors stay valid). Recorded so the ledger's "the line
  anchors moved" reflex does not wrongly re-attribute this one.

### Extraction-only precision notes (no seed claim contradicted)

- **X1 (R.1.5) — synchronicity.** The forced-query call is one synchronous call
  stack ending only after the WHOLE epoch (incl. LowerSubgraphInstances under
  -demand-instance) completes, before the friend reads the table. The baseline
  DIFF-R1's "REQUEST edge lowers to FindOrAddInstance + the activation's
  fixpoint" must preserve or deliberately break this. No existing doc states it.
- **X2 (R.1.6) — CompactDead second arm.** CLAUDE.md's "dead ≥ live, 4096
  floor" is arm (a) only; NeedsCompaction OR's a second undocumented near-Rehash
  arm `NumRows()>=SlotCapacity()*7/8 && dead>=NumRows()/2`. Table.h:541-653.
- **X3 (R.1.7) — dead JOIN branch.** Join.cpp:742 `true ||` makes the direct
  nested-loop-join at :772-780 (`assert(false && "Disabled")`) unreachable; a
  region-model translation of "how BuildEagerRegion reaches a JOIN's successors"
  must model ONLY the WorkItem-deferred ContinueJoinWorkItem::Run path.

---


## Part B (2026-08-03, session 4) — the Stage-B-grain pipeline pseudocode, fleet-verified at tip 8a4520d9

This part SUPERSEDES `stage-b-seed.md` Part 1 at finer grain, and re-anchors
the Stage-B freeze-point pseudocode against the `keyed-instances` tip
**8a4520d9**. A four-agent verification fleet (Query::Build tail/ctor/callers;
Main.cpp wiring; Program::Build/ExtractPrimaryProcedure/ValidateDROps; the
downstream read surface) walked the pipeline mechanically — not against an
older doc's claim list but from the code — so every anchor below is live at
8a4520d9. Per the standing SINGLE-PASS RULE, the next Stage-B implementer
re-verifies before building; line numbers drift, the structure should not.

Earlier sections of this file (Parts 1–D, §1–§7, and Part R) are NOT edited.
Where Part R §1 (verified at the older f0c913e0 tip) and `stage-b-diff.md`
(authored at f0c913e0, pre-Stage-A) carry anchors that have since drifted,
the contradiction is logged in the **DRIFT LEDGER (Part B)** at the end, never
patched in place.

Fleet tally at 8a4520d9: **Query::Build tail/signature/ctor/callers — all
VERIFIED** (tail substance byte-identical to the seed paraphrase, two
interleaved `num_errors` guards the seed collapsed now made explicit);
**Main.cpp dump-drain block anchors — 2 DRIFTED (+1 line each), 3 VERIFIED**;
**Program::Build entry — signature + pre-pass + BuildDataModel DRIFTED (+~23
lines, file grew above), demand_instance fences / context wiring / census site
VERIFIED**; **read surface — fresh mechanical extraction, clean for a future
`FrozenRegionalProgram::Query()` accessor**. No section was structurally
wrong; drift is line-shift and (in Program::Build) the whole body sliding down
~23 lines because code above it grew between the two tips.

---

### B.1 The whole Main.cpp pipeline (bin/drlojekyll/Main.cpp, 621 lines)

The two `Build` calls, `SetRelDumpStream`, all dump drains, and codegen live
in `CompileModule` (:63–145), reached from `main()` (:278–621) only via
`ProcessModule` (:148–200). Globals (namespace `hyde`, anon namespace, :47–61;
`gOut` is file-scope at :34, wired to `std::cout` in `main` at :291–292):

```
# bin/drlojekyll/Main.cpp:47-61 — the driver globals
gFirstId=0u :47   gDemand=false :48   gDemandInstance=false :49
gDemandRetract=false :50   gPassPolicy :51 (ONE instance; threaded, by identity,
                                            into BOTH Build calls)
gDatabaseName="datalog" :52   gHasDatabaseName=false :53   gCxxOutDir=nullptr :54
gDOTStream :56  gDFStream :57  gContractStream :58  gRelStream :59
gDRStream :60   gIRStream :61     # all OutputStream*, nullptr until an argv case arms them
```

```
CompileModule(parser, display_manager, error_log, module):        # :63-145
  gPassPolicy.bisect_counter = 0u                                  # :68 — reset per module;
                                                                    #   the ONE cross-Build mutable index
  query_opt = Query::Build(module, error_log, gPassPolicy,
                           gDemand, gDemandRetract)                 # :70-71 (call expr on :71)
  if not query_opt: return EXIT_FAILURE                            # :72-74
  # <<< THIRD-SLOT INSERTION POINT >>> a post-Query::Build / pre-Program::Build
  #   dump gated purely on *query_opt (a hypothetical -region-out / -query-out)
  #   inserts HERE — after :74's closing brace, before :76's comment. Nothing
  #   else touches *query_opt in this gap; TableId() is NOT yet annotated (that
  #   is a Program::Build side effect), so this slot sees the PURE DataFlow graph.
  #   Follows the ir-out pattern: a gStream global near :56-61, an argv case in
  #   the :315-410 band, and `if (gStream){ (*gStream)<<...; gStream->Flush(); }` here.
  # T2b: install the -rel-out sink BEFORE Program::Build — Rel is built AND
  #   drained INSIDE it; there is NO top-level `if (gRelStream)` drain anywhere.
  SetRelDumpStream(gRelStream)                                     # call at :80 (comment :76-79)
                                                                    #   null-safe: unset => guarded no-op
  program_opt = Program::Build(*query_opt, error_log, gFirstId,
                               gPassPolicy, gDemandInstance)        # :82-84 (call expr on :83)
  if not program_opt: return EXIT_FAILURE                          # :85-87
  ret = EXIT_SUCCESS                                               # :89
  if gIRStream:    (*gIRStream) << *program_opt; Flush()           # :91-94  -ir-out (drains PROGRAM)
  if gCxxOutDir:   create_dirs; open <name>.h/.cpp;                # :96-118 -cpp-out (drains PROGRAM,
                   cxx::GenerateDatabaseCode(*program_opt,...)      #   SITS BETWEEN ir-out and dot-out;
                                                                    #   NOT a "dump slot" but a real
                                                                    #   *program_opt consumer)
  # NOTE(pag) :120-122: the next three drain AFTER Program::Build because it
  #   back-annotates TableId() onto the DataFlow views the DOT/DF dumps render.
  if gDOTStream:   (*gDOTStream) << *query_opt; Flush()            # :123-126 -dot-out (drains QUERY)
  if gDFStream:    (*gDFStream) << QueryDF{*query_opt}; Flush()    # :131-134 -df-out  (drains QUERY via tag)
  if gContractStream: (*gContractStream)                          # :139-142 -contract-out (drains QUERY
                       << QueryContracts{*query_opt}; Flush()       #   via tag; Stage A H-A8)
  return ret                                                       # :144
```

```
ProcessModule(...):                                               # :148-200
  resolve database name from module.DatabaseName()                # :152-155
  if gDRStream: (*gDRStream) << module                            # :158-162 -dr-out: the AMALGAMATED
                                                                    #   datalog-SOURCE echo (raw module),
                                                                    #   PRE-Query::Build — NOT the Rel/DR-IR
                                                                    #   dump; do not conflate by gDR* name.
  #ifndef NDEBUG: parse->print->reparse x3, assert ss2==ss3       # :165-197 round-trip self-test
  return CompileModule(parser, display_manager, error_log, module) # :199
```

```
main(argc, argv):                                                 # :278-621
  setup: display_manager, error_log, parser, gOut=&os (:291-292),
         unique_ptr<FileStream> RAII owners for each -*-out file  # :280-299
  for i in 1..argc:                                               # argv loop :302-576
    -cpp-out :305-313    -ir-out :315-327    -dr-out :331-344
    -dot-out :347-360    -df-out :363-376    -contract-out :379-393
    -rel-out :396-410    -first-id :414-424 (gFirstId=strtoul)
    -disable-dataflow-opt    :429-433  # appends DisableDataFlowOpt().disabled_globs
    -disable-controlflow-opt :436-441  # appends DisableControlFlowOpt().disabled_globs
    -opt-disable=<glob>[,..] :445-464  # IsValidGlob+MatchesAnyKnownPass, => disabled_globs
    -opt-only=<glob>[,..]    :465-484  # same validation, => only_globs
    -opt-bisect-limit=<N>    :485-495  # sets gPassPolicy.bisect_limit (NOT the counter).
                                       #   THE ONLY bisect CLI surface — there is NO "-bisect" flag.
    -demand :499-501 (gDemand=true)
    -demand-instance :506-510 (gDemand=true; gDemandInstance=true — implies -demand)
    -demand-retract  :516-519 (gDemand=true; gDemandRetract=true — implies -demand)
    -M <path> :522-531   --help/-h :534-536   --version/-v :539-541
    unrecognized "-..." :544-548 (error, continue)
    input file path :551-575 (escape, append `#import "path".` to linked_module)
  dispatch :578-611:
    error_log non-empty        -> skip                            # :581-582
    0 input paths              -> "No input files" error          # :583-584
    1 path AND !gHasDatabaseName-> ParsePath -> ProcessModule     # :587-597 single-module
    else                        -> ParseStream(linked_module) ->  # :601-610 amalgamation
                                   ProcessModule
  render :613-618 (Render on error else RenderWarnings); return code # :620

# bisect / PassPolicy thread:
#   PassPolicy::bisect_counter is `mutable uint64_t{0}` at Util/PassPolicy.h:53,
#   on the SAME class as disabled_globs/only_globs/bisect_limit (:47-53). The
#   single gPassPolicy (:51) is passed by identity to Query::Build (:71, arg 3)
#   AND Program::Build (:83, arg 4) — both IR levels tick ONE monotone index.
#   Reset once/module at CompileModule:68; incremented at Util/PassPolicy.cpp:85
#   (`bisect_counter++`) inside PassPolicy::Gate (decl PassPolicy.h:68) — one
#   tick per gateable application. No CLI setter for the counter itself.
# SetRelDumpStream: decl ControlFlow/Format.h:17; def Rel/Format.cpp:1160
#   (also decl Rel/Rel.h:1257); called exactly once, at :80.
```

**Dump-timing split (binds any Stage-B `-region-out`):** `-rel-out`'s sink is
armed BEFORE Program::Build (built+drained inside it); `-ir-out`/`-dot-out`/
`-df-out`/`-contract-out` drain AFTER it returns (they need `TableId()`). A
Stage-B regional dump picks ONE of these two shapes. Per `stage-b-seed.md`
DELTA-3, the desired-states G1 predicted bytes assume the third-slot
(pre-Program::Build) shape for a `*query_opt`-rooted region dump; the exact
insertion point is the `<<< THIRD-SLOT >>>` marker above (:74→:76 gap).

---

### B.2 The Query::Build tail (lib/DataFlow/Build.cpp)

Function spans `std::optional<Query> Query::Build(...)` :2524–2652. `impl` is
minted at :2529 (`std::shared_ptr<QueryImpl> impl(new QueryImpl(module));`).
The full error-check ladder has SEVEN guard points; the last two belong to
"the tail" as the seed scoped it. Two of those seven are interleaved through
the tail itself and were collapsed by the seed's one-line paraphrase:

```
# lib/DataFlow/Build.cpp — the Query::Build TAIL, live at 8a4520d9
2629  impl->FinalizeDepths();
2630  impl->FinalizeColumnIDs();
2631  impl->TrackDifferentialUpdates(log, /*force=*/true);   # SECOND, forced call
2632  if (num_errors != log.Size()) return std::nullopt;      # :2632-2634 guard
2635  impl->TrackConstAfterInit();
2637  BuildEquivalenceSets(impl.get());                       # identity-erasing model sharing
2638  impl->Stratify(log);                                    # Tarjan; view->stratum; V-SCC-SEAM
2639  if (num_errors != log.Size()) return std::nullopt;      # :2639-2641 guard
      # Stage A (H-A4): identity now provable over the FINAL graph —
      #   view->stratum set, column ids final, const facts ready.
2646  impl->row_contracts = InferConservativeRowContracts(impl.get());   # STAGE A, pure, post-Stratify
2647  if (!ValidateRowContracts(impl.get(), log))             # :2647-2649 — BOOLEAN guard, NOT the
2648    return std::nullopt;                                   #   `num_errors != log.Size()` idiom used
2649                                                           #   everywhere else in this function (a real,
                                                              #   verified distinction — H-A7 validators)
2651  return Query(std::move(impl));                          # sole return of a live Query
2652  }  # end Query::Build

# Earlier guards (context for where the tail begins):
#   :2562-2564 (post RemoveUnusedViews/ClearGroupIDs/1-arg TrackDifferentialUpdates)
#   :2571-2573 (post df.simplify Gate)   :2596-2598 (post ApplyDemandTransform)
#   :2614-2616 (post Optimize; gated `policy.AnyBodyOptionalEnabled(PassLevel::kDataFlow)`)
#   :2625-2627 (post IdentifyInductions)
```

Signature (header `include/drlojekyll/DataFlow/Query.h:1049-1053`; definition
`Build.cpp:2524-2527`), verbatim:

```
static std::optional<Query> Build(const ParsedModule &module,
                                  const ErrorLog &log,
                                  const PassPolicy &policy,
                                  bool demand_mode = false,
                                  bool demand_retract = false);
```

**DIFF (DELTA-1 / stage-b-diff.md H1 — the freeze point, NOT current state):**
`BuildPlanningRegionalProgram` + `Freeze`/`FreezeAndValidate` slot AFTER
`ValidateRowContracts` and BEFORE `return Query(...)` — contracts are planning
input, so the freeze MUST follow the contract pass:

```
2646  impl->row_contracts = InferConservativeRowContracts(impl.get());
2647  if (!ValidateRowContracts(impl.get(), log)) return std::nullopt;
+     planning = BuildPlanningRegionalProgram(impl.get());   # DIFF (Stage B) — degenerate:
+         #   ONE ProgramRoot + ONE observation-root template; consumes contracts,
+         #   equivalence models, strata, guard_annotations, RecognizedSubgraphs(),
+         #   DemandForcings() — every §1.2-seed input.
+     frozen = FreezeAndValidate(planning, log);             # DIFF — distinct TYPE; H9 freeze validators;
+         #   no extraction, no request-edge mint at Stage B (that is Stage C).
+     return Query(std::move(impl), frozen);                 # H1 Variant 1 — return-type ripple
2651  return Query(std::move(impl));                         # CURRENT STATE (Variant-1 replaces this line)
```
Owner-gated placement (ESC-4): H1 Variant 1 changes `Query::Build`'s return
shape (ripples every caller — see B.3); H1-ALT hoists the two NEW calls to a
`main`-level step between `Query::Build` and `Program::Build` in
`CompileModule` (the `<<< THIRD-SLOT >>>` gap of B.1), leaving `Query::Build`'s
signature untouched. Both are DIFF, not current state.

---

### B.3 The Query object — ctor surface, ownership, callers

`class Query` at `Query.h:1036-1158` is a PIMPL, shared-ownership handle:

```
# include/drlojekyll/DataFlow/Query.h
:1157  std::shared_ptr<QueryImpl> impl;               # SHARED (not unique_ptr); matches Build.cpp:2529
                                                       #   `shared_ptr<QueryImpl>` + :2651 std::move
:1155  inline explicit Query(std::shared_ptr<QueryImpl> impl_) : impl(impl_) {}
                                                       # the ONLY constructing ctor; PRIVATE; reachable
                                                       #   only from Build() (static member access) and
                                                       #   the two friend operator<< overloads below
:1149  friend operator<<(OutputStream&, QueryContracts);   # -contract-out emitter (Format.cpp)
:1153  friend operator<<(OutputStream&, Query);            # -dot-out DOT dump  (Format.cpp)
:1141-1144  Query(const Query&)=default; Query(Query&&) noexcept=default;
            operator= x2 =default;                     # cheap-to-copy shared handle — callers pass by
                                                       #   value / deref *query_opt repeatedly, no rebuild
:1055  ~Query(void);                                   # out-of-line dtor (QueryImpl incomplete here — PIMPL)
:1049-1053  static std::optional<Query> Build(...);    # the SOLE public factory
```

**Complete caller list of `Query::Build` (repo-wide, live tree — excludes
docs/ and stale `.claude/worktrees/wf_*` snapshots): exactly TWO.**

```
# (1) bin/drlojekyll/Main.cpp:71 — the CLI compile path (full policy + demand flags)
query_opt = Query::Build(module, error_log, gPassPolicy, gDemand, gDemandRetract)
  reads of *query_opt, ALL inside CompileModule:
    :83  Program::Build(*query_opt, ...)          # control-flow-IR input
    :123 (*gDOTStream) << *query_opt              # DOT (gated gDOTStream)
    :131 (*gDFStream)  << QueryDF{*query_opt}     # .df  (gated gDFStream)
    :139 (*gContractStream) << QueryContracts{*query_opt}   # -contract-out (gated gContractStream)
  # gDemandInstance is NOT passed here — it reaches Program::Build only.

# (2) bin/Oracle/Main.cpp:745-746 — the reference oracle (dataflow-opt DISABLED, demand OFF)
query = hyde::Query::Build(*module_opt, error_log, hyde::PassPolicy::DisableDataFlowOpt())
  # 3-arg overload (demand_mode/demand_retract default false); member
  #   `std::optional<hyde::Query> query;` at :600 (persists across the Oracle's life).
  # Comment :743-744: "the oracle interprets the graph the aggressive optimization
  #   pass never touched." Reads the whole Query PUBLIC surface read-only:
  #   NumStrata() :766 (ONLY caller repo-wide), ForEachView :800, IOs :804, Tuples :966,
  #   Compares :985, Maps :1021, Merges :1086, Joins :1102, Negations :1178, Inserts :1206,
  #   Selects :1231, Aggregates :1288, KVIndices :1357, Relations :2337/:2369.
  # Never calls Program::Build (grep-confirmed) — stays at the DataFlow layer.

# NON-callers, confirmed by grep + their own header comments (design-intentional):
#   bin/RefInterp/Main.cpp:5  "(no ... Query::Build, no Rel, no codegen)" — OG1-parsed I0 evaluator
#   bin/RefHarness/Main.cpp:13 "hyde::Parser ONLY (no Query::Build, no DataFlow)" — OG2 harness
#   No tests/*.cpp calls it — end-to-end tests drive the drlojekyll binary or Oracle/RefInterp/RefHarness.
```

**Stage-B ripple (H1 Variant 1):** changing `Query::Build`'s return to
`optional<pair<Query, FrozenRegionalProgram>>` (or a two-field return) touches
BOTH live callers — `Main.cpp:71` and `Oracle/Main.cpp:745`. The Oracle reads
only the Query half, so a Variant-1 return must keep `.first`/`.query` a plain
`Query`. H1-ALT touches neither caller's `Build` call (the freeze becomes a
`main`-level step) — its only edit is in `CompileModule`.

---

### B.4 The downstream read surface (the FrozenRegionalProgram::Query() budget)

Definitive `Query`-public-method → consumer table at 8a4520d9 (representative
anchor each; Oracle listed parenthetically as a non-lib bonus consumer that
reaches nearly the whole surface for its independent re-derivation):

```
Build (factory)     Main only                         Main.cpp:71
DemandForcings      ControlFlow, Rel                  CF Build.cpp:1514 / Rel Rel.cpp:937 (def Demand.cpp:295)
GuardAnnotations    ControlFlow, Rel                  CF Build.cpp:1452 / Rel Rel.cpp:936 (def Demand.cpp:300)
RecognizedSubgraphs Rel only                          Rel Rel.cpp:1052, :4007 (def Demand.cpp:305; NO CF caller)
IsDemandMessage     CodeGen only (via Program::Query())  CodeGen Database.cpp:1522, :3692 (def Demand.cpp:310)
ParsedModule        ControlFlow (Main indirect)       CF Program.cpp:266 (impl->query.ParsedModule())
NumStrata           Oracle only — NO lib consumer     Oracle Main.cpp:766 (def Query.cpp:1816)
Joins               ControlFlow, Rel, Format          CF Build.cpp:83 / Rel Rel.cpp:1950 / Format
Selects             Format only (+Oracle)             Format.cpp:266
Tuples              ControlFlow, Format (+Oracle)     CF Procedure.cpp:971 / Format
KVIndices           ControlFlow, Rel, Format          CF Build.cpp:178 / Rel Rel.cpp:2045
Relations           Format only (+Oracle)             Format.cpp:183
Inserts             ControlFlow, Format               CF Build.cpp:61
Negations           ControlFlow, Rel, Format          CF Build.cpp:136 / Rel Rel.cpp:1877
Maps                ControlFlow, Format               CF Build.cpp:147
Aggregates          ControlFlow, Rel, Format          CF Build.cpp:175 / Rel Rel.cpp:2017
Merges              ControlFlow, Format               CF Build.cpp:77
Compares            ControlFlow, Format               CF Build.cpp:154
IOs                 ControlFlow, Rel                  CF Procedure.cpp:233 / Rel Rel.cpp:2758
Constants           ControlFlow only                  CF Build.cpp:1527
Tags                ControlFlow only                  CF Build.cpp:1539
ForEachView         ControlFlow, Format               CF Build.cpp:39 / Format
```

**How each downstream library GETS a Query:**
- **ControlFlow** receives it exactly once, as `Program::Build`'s first param
  (`Build.cpp:1331`), stores it verbatim into `ProgramImpl::query` (a
  `const Query query;` at `Program.h:1998`) via `make_shared<ProgramImpl>(query,
  first_id)` (`Build.cpp:1505`), and threads it by value/const-ref to every
  callee (FillDataModel/BuildDataModel/BuildEntryProcedure/BuildIOProcedure/
  BuildInitProcedure/FindMonotoneNegatedTables).
- **Rel** gets `Query` ONLY as a by-value param forwarded from ControlFlow's
  `Program::Build` query through `Stratum.cpp` (e.g. `DeriveDRStrata(...)` at
  `Stratum.cpp:2156`, `LinearizeAndValidateDRFlow(...)` at `:2187`). It has no
  other source. Signature anchors: `Rel.h:992/1023/1069`.
- **CodeGen** reaches Query ONLY via `Program::Query()` (decl
  `Program.h:1489`, def `Program.cpp:260-262` `return impl->query;`), and calls
  exactly ONE method: `program.Query().IsDemandMessage(*m)` (`Database.cpp:1522,
  :3692`). Everything else CodeGen needs comes off Program/ProgramImpl. It never
  holds a Query independent of Program.

**What a thin `FrozenRegionalProgram::Query()` accessor MUST preserve:** the
entire read surface above is already clean for a frozen wrapper — no
downstream library names `QueryImpl` in code (one stray comment at
`Build.cpp:187`) or includes the PRIVATE `lib/DataFlow/Query.h`. So the
accessor need only return the same `Query` value handle that ControlFlow/Rel
thread today and that `Program::Query()` hands CodeGen. The single seam that
must survive verbatim is `Program::Query()` (`Program.cpp:260-262`) — it is the
ONE gateway by which anything outside ControlFlow (i.e. CodeGen) recovers a
Query after Program::Build. Preserve `DemandForcings`/`GuardAnnotations`/
`RecognizedSubgraphs`/`IsDemandMessage` as pass-throughs (the demand satellites
downstream reads by name) and the 15 `DefinedNodeRange` view accessors +
`ForEachView` used by FillDataModel/BuildDataModel and the Format dumps.

**Direct-QueryImpl bypasses (the only genuinely private leaks, both confined to
lib/DataFlow itself — NOT downstream):**
- `row_contracts` is exposed through NO public Query method. `-contract-out`
  reads it ONLY via the friend `operator<<(OutputStream&, QueryContracts)` at
  `Format.cpp:1531-1533`, doing `qc.query.impl->row_contracts` — a direct
  `.impl->` into the private QueryImpl (legal: Format.cpp is a declared friend
  `Query.h:1149` AND `#include`s the private `Query.h` at `Format.cpp:19`).
  Driven entirely from `Main.cpp:139`. A `FrozenRegionalProgram::Query()` need
  NOT carry `row_contracts` unless the frozen dump replaces `-contract-out`.
- The three DataFlow dump operators (`operator<<(…, Query)` DOT `Format.cpp:43`,
  decl `Format.h:12`; `QueryDF` `Format.cpp:813`, struct `Format.h:18-20`;
  `QueryContracts` `Format.cpp:1531`, struct `Format.h:31-33`) all take the
  `Query` WRAPPER by value, but Format.cpp additionally reads
  `QueryViewImpl::det_seq`/`is_dead` directly (bypassing `Query::ForEachView`,
  which the comments at `Format.cpp:811/1536` forbid here — it walks JOINs first
  and would break the required ascending-`det_seq`/kind-tagged order). This is a
  DataFlow-internal bypass, not a downstream one.
- `lib/Rel/Format.cpp` (`-rel-out`) takes NO `Query` — it dumps `DRFlowGraph`
  (built by `BuildDRInventory` from `impl->tables`/DROps), armed by
  `SetRelDumpStream`. It `#include`s only the PUBLIC `Query.h` for QueryView
  types embedded in DROp payloads.

Consequence for Stage B: the current read surface is ALREADY a clean substrate
for the frozen accessor — ControlFlow/Rel take `Query`/`const Query&` (never
`QueryImpl*`), CodeGen reaches Query only through `Program::Query()`, and the
one private leak (QueryContracts→row_contracts) is Format-local.

---

### B.5 Program::Build entry (lib/ControlFlow/Build/Build.cpp)

```
1331  std::optional<Program> Program::Build(const ::hyde::Query &query,
1332                                        const ErrorLog &log, unsigned first_id,
1333                                        const PassPolicy &policy,
1334                                        bool demand_instance) {
1340    num_errors = log.Size();          # only statement before the pre-pass, plus the lambda below
        # C-2 V-ALGEBRA + feature-gap pre-pass (demand-INDEPENDENT), header :1342-1362,
        #   `has_induction_owned_input` lambda :1355-1362:
1364    for agg in query.Aggregates():                                     # :1364-1392
1368      if has_induction_owned_input(agg): diagnostic "not yet supported"; continue
          # (NO over()-undeclared reject here — an undeclared over() defaults to @recompute)
1394    for kv in query.KVIndices():                                       # :1394-1417
1397      if has_induction_owned_input(kv): diagnostic; continue
1409      if !merge.IsInvertible() && !merge.IsRecompute(): V-ALGEBRA diagnostic; continue
1418    for map in query.Maps():                                           # :1418-1423
1419      if !map.Functor().IsPure(): diagnostic "Impure functors..."
1424    for join in query.Joins():                                         # :1424-1435
1425      if 0 pivot cols && CanReceiveDeletions() && ViewSelfReachable(join):
            diagnostic "Cross-products ... recursive cycles ..."

        # Keyed-instance feature-gap fences — GATED (comment :1436-1450):
1451    if (demand_instance) {                                             # :1451-1499
1452      annots = query.GuardAnnotations()
1455      query.ForEachView(...) -> bucket guard-annotated views by forcing_index into fguards
1462      for each forcing-index bucket fe:
            recursive_content=false, cyclic_demand=false
            for (v, ai) in fe.second:
              if !v.IsJoin(): continue
              collect JoinedViews jl; if jl.size()<2: continue
              if annots[ai].role == kBody:
                in = jl[1]
                if in.InductionGroupId() || ViewSelfReachable(in): recursive_content=true
                for p in in.Predecessors(): if p.InductionGroupId(): recursive_content=true
              if ViewSelfReachable(jl[0]): cyclic_demand=true
            if cyclic_demand:    diagnostic "Recursive demand ... not yet supported under -demand-instance"
            elif recursive_content: diagnostic "recursive (induction-owned) content ... feature gap"
1499    }  # cyclic_demand wins the if/elif; BOTH are local to this block. FENCE(iii)
          #   differential-summarized-input is named in the comment but is NO LONGER a
          #   live reject (lifted by R-a2 band-(a2)).

1501    if (num_errors != log.Size()) return std::nullopt;                 # :1501-1503

1505    impl    = make_shared<ProgramImpl>(query, first_id);   program = impl.get();
1508    Context context;
1509    context.init_proc = impl->procedure_regions.Create(next_id++, kInitializer);   # :1509-1510
1514    context.demand_forcings = &query.DemandForcings();                 # (comment :1512-1513)
1520    context.demand_instance_enabled = demand_instance;
1522    BuildDataModel(query, program);                                    # <- DRIFTED anchor (was :1499)
1527    for const_val in query.Constants(): mint const_to_var              # special-case no-literal/non-tag
                                                                            #   -> impl->true_
1539    for const_val in query.Tags():      mint const_to_var (kConstantTag)
        # (DiscoverInductions is commented-out dead code :1546-1552)
1556    FillDataModel(query, program, context);
1561    FindMonotoneNegatedTables(program, context, query);
1564    entry_proc = BuildEntryProcedure(program, context, query);
1566    for io in query.IOs(): BuildIOProcedure(impl.get(), query, io, context, entry_proc);
1572    BuildInitProcedure(program, context, query);
1574    for insert in query.Inserts():
          if insert.IsRelation() && insert.Relation().Declaration().IsQuery():
            BuildQueryEntryPoint(program, context, decl, insert)
1587    queries_with_entry_points = { spec.query.Id() for spec in impl->queries }
1591    for sub_module in ParsedModuleIterator(query.ParsedModule()):
          for parsed_query in sub_module.Queries():
            if not in queries_with_entry_points: BuildEmptyQueryEntryPoint(...)
1601    for proc in impl->procedure_regions:
          if !EndsWithReturn(proc): append BuildStateCheckCaseReturnFalse
1608    FixupContainingProcedure(impl.get());
1611    if policy.AnyBodyOptionalEnabled(kControlFlow): impl->Optimize(policy);   # 1st CF Optimize :1611-1613
1615    ExtractPrimaryProcedure(impl.get(), entry_proc, context);                 # SANDWICHED
1617    FixupContainingProcedure(impl.get());
1620    if policy.AnyBodyOptionalEnabled(kControlFlow): impl->Optimize(policy);   # 2nd CF Optimize :1620-1622
1629/1638  for proc in impl->procedure_regions: MapVariables(proc);   # DUPLICATED loop (:1629-1631, :1638-1640)
1657    return Program(std::move(impl));
1660  }  # namespace hyde
```

**Direct `query.<method>()` sites in Program::Build's own body (12):**
Aggregates :1364, KVIndices :1394, Maps :1418, Joins :1424, GuardAnnotations
:1452, ForEachView :1455, DemandForcings :1514, Constants :1527, Tags :1539,
IOs :1566, Inserts :1574, ParsedModule :1592. Separately the whole `query` is
forwarded to 7 callees: ProgramImpl ctor :1505, BuildDataModel :1522,
FillDataModel :1556, FindMonotoneNegatedTables :1561, BuildEntryProcedure
:1564, BuildIOProcedure :1567 (per IO), BuildInitProcedure :1572.

**ExtractPrimaryProcedure call chain** (decl `Build.h:653`; doc `Procedure.cpp:
767-776`; body `:777-928`; sole call site `Build.cpp:1615`, SANDWICHED between
the two `impl->Optimize(policy)` rounds — after one CF-optimize, immediately
before another, NOT unconditionally "after Optimize"):

```
ExtractPrimaryProcedure(impl, entry_proc, context):     # Procedure.cpp:777-928
  # splits entry_proc into (1) a simplified entry proc that reads only message
  #   vectors, does joins, appends to induction/output vectors, and (2) a new
  #   primary_proc (ProcedureKind::kPrimaryDataFlowFunc) that takes induction
  #   vectors and does the rest of the flow.
  mint primary_proc (kPrimaryDataFlowFunc)                              # :778-779
  walk entry_proc->input_vecs uses (ForEachUse) -> regions_to_extract   # :787-793
  build entry_seq/entry_par SERIES/PARALLEL shell                       # :797-799
  re-parent each extracted region under entry_par via LET replacement   # :806-811
  swap entry_proc->body into primary_proc; entry_proc->body = entry_seq  # :815-817
  ClassifyVector each entry vec as read/written by entry vs primary      # :823-843
  primary_params = written_by_entry ∩ read_by_primary                    # :845-850
  create replacement vecs on primary_proc for params/read/written        # :854-876
  rewrite primary-scoped uses to the new vecs                            # :878-882
  GC unused entry vecs                                                    # :885
  emit VECTORCLEAR for unneeded entry vecs                               # :892-911
  emit CALL entry_seq -> primary_proc(primary_params)                    # :914-921
  terminate entry_proc with kReturnFalseFromProcedure                    # :924-925
  FixupContainingProcedure(impl)                                         # :927
```

**ValidateDROps tail — the keyed-instance census recount (ESC-3 candidate for
V-REGION-CENSUS-IDENTITY).** `ValidateDROps` (`Rel.cpp:3358`, decl `Rel.h:1022`,
sole call `Stratum.cpp:2186`); op-inventory `count_kind`/`expect` lambdas
`:3959-3976`, the `expect(...)` run `:3977-3990`, then:

```
# lib/Rel/Rel.cpp:3999-4021 — instance-op census recount
3999  unsigned exp_instance = 0u, exp_death = 0u;
4000  if (context.demand_instance_enabled) {
4006    LiveRecognition lr = ResolveLiveRecognition(impl, query);       # SAME helper the mint uses,
                                                                         #   but driven off DataFlow's
                                                                         #   RecognizedSubgraphs(), NEVER
                                                                         #   the mint's own output (A.1.5
                                                                         #   independence)
4007    for (const RecognizedSubgraph &rs : query.RecognizedSubgraphs()):
4008      git = lr.by_forcing.find(rs.forcing_index)
4008      if (git == end || !git->second.ok) continue
4012      ++exp_instance
4013      if (git->second.demand_table && TableIsDifferential(git->second.demand_table))
4015        ++exp_death
4018  }
4019  expect(kSubgraphInstantiate, exp_instance, "subgraph instantiates");
4020  expect(kInstanceDeath,       exp_death,    "instance deaths");
4021  expect(kInstanceSeal,        exp_instance, "instance seals");   # 1:1 with instantiate
```
Flag-off it never dereferences `RecognizedSubgraphs()` and expects 0 of each
instance-kind op. Flag-on it independently re-derives the counts — exactly the
V-REGION-CENSUS-IDENTITY-style cross-check the Stage-C region census wants (this
is ESC-3's ValidateDROps-tail home; the alternate home is a post-Program `main`
check). Note the `kInstanceDeath` gate keys on the DEMAND table's
differentiality (`demand_retract`), NOT the input's — see Part R ledger B1; this
recount mirrors that gate exactly.

---

### B.6 DRIFT LEDGER (Part B) — `claim -> reality -> fresh anchor`

Recorded per the standing rule: Parts 1–D, §1–§7, and Part R above are NOT
edited; the following corrects `stage-b-diff.md` (authored at f0c913e0),
`stage-b-seed.md` Part 1, and this file's Part R §1 (verified at the older
f0c913e0 tip) against tip 8a4520d9.

#### Against stage-b-diff.md (f0c913e0-era anchors)

- **DB-1 — Query::Build return line.** CLAIM (H1, `stage-b-diff.md:63`):
  `return Query(std::move(impl))` at `:2637`. REALITY: at `:2651`
  (`Build.cpp:2652` closes the function). The `BuildPlanningRegionalProgram`/
  `FreezeAndValidate` DIFF-hunk therefore inserts at :2647→:2651, not
  :2637. ANCHOR: `lib/DataFlow/Build.cpp:2646-2651`.
- **DB-2 — Program::Build signature range.** CLAIM (`stage-b-diff.md:204`,
  ":1308-1311"): VERIFIED text, DRIFTED range — the file grew ~23 lines above.
  REALITY: `:1331-1334`. ANCHOR: `lib/ControlFlow/Build/Build.cpp:1331-1334`.
- **DB-3 — V-ALGEBRA pre-pass range.** CLAIM (`stage-b-diff.md:225`,
  ":1332-1411"): REALITY: `:1342-1435` — header+lambda :1342-1362, Aggregates
  :1364-1392, KV :1394-1417, Maps :1418-1423, Joins :1424-1435 (runs THROUGH
  the Joins loop at 1435, not ending at 1411). ANCHOR: `Build.cpp:1342-1435`.
- **DB-4 — demand_instance guard fences range.** CLAIM (`stage-b-diff.md:226`,
  ":1428-1473"): DRIFTED/stale. REALITY: the `if (demand_instance){...}` block
  is `:1451-1499` (opens :1451, closes :1499 right before the num_errors gate
  at :1501). This RESOLVES the H4-vs-Part-R disagreement: Part R's
  :1451-1499 is current; H4's :1428-1473 no longer matches. ANCHOR:
  `Build.cpp:1451-1499`.
- **DB-5 — BuildDataModel call.** CLAIM (`stage-b-diff.md:226`, ":1499"):
  REALITY: `BuildDataModel(query, program);` is at `:1522` (after the
  num_errors gate :1501-1503 and Context/impl setup :1505-1520). ANCHOR:
  `Build.cpp:1522`.
- **DB-6 — Main.cpp Query::Build call.** CLAIM (`stage-b-diff.md:83`,
  ":69-70"): REALITY: the call expression is on `:71` (the `auto query_opt =`
  LHS is :70); null-check :72-74. ANCHOR: `bin/drlojekyll/Main.cpp:70-74`.
- **DB-7 — Main.cpp SetRelDumpStream / Program::Build calls.** CLAIM
  (`stage-b-diff.md:290`, `:79` for SetRelDumpStream; `:81-83` for
  Program::Build). REALITY: `SetRelDumpStream(gRelStream)` call is on `:80`
  (comment block :76-79); Program::Build call expression is on `:83` (statement
  :82-84). ANCHOR: `Main.cpp:76-84`.

#### Against stage-b-seed.md Part 1

- **DB-8 — Query::Build tail range.** CLAIM (`stage-b-seed.md:26`,
  ":2629-2650"): REALITY: `:2629-2651`, with TWO `num_errors != log.Size()`
  early-return guards interleaved (`:2632-2634`, `:2639-2641`) that the seed's
  one-line paraphrase collapsed, and a distinct BOOLEAN `!ValidateRowContracts`
  guard at `:2647-2649`. ANCHOR: `Build.cpp:2629-2651`.
- **DB-9 — dump-drain block ranges.** CLAIM (`stage-b-seed.md:36`,
  "Main.cpp:120-141" as one span for -dot/-df/-contract): REALITY: the drains
  are separate `if` blocks at `-dot-out :123-126`, `-df-out :131-134`,
  `-contract-out :139-142` (with the -cpp-out codegen at :96-118 sitting BEFORE
  the DOT drain, between it and -ir-out :91-94). ANCHOR: `Main.cpp:91-142`.
- **DB-10 — SetRelDumpStream slot.** CLAIM (`stage-b-seed.md:37`,
  "Main.cpp:76"): the comment starts :76 but the CALL is :80. VERIFIED as
  BEFORE Program::Build. ANCHOR: `Main.cpp:80`.

#### Against Part R §1 (this file, verified at the older f0c913e0 tip)

- **DB-11 — Query::Build head + return.** Part R §1 (`:55`, `:103`) placed the
  function head at `Build.cpp:2518` and `return Query(impl)` at `:2637`.
  REALITY at 8a4520d9: definition `:2524-2527`, return `:2651`. The named body
  passes (TrackDifferentialUpdates :2555→`:2631` forced call, BuildEquivalenceSets
  :2631→`:2637`, Stratify :2632→`:2638`) all shifted; substance unchanged.
  ANCHOR: `Build.cpp:2524-2652`.
- **DB-12 — Main.cpp drain + bisect-reset lines.** Part R §1 (`:35`, `:38-53`)
  used bisect-reset `:67`, Query::Build `:69-70`, SetRelDumpStream `:79`,
  Program::Build `:81-83`, -ir-out `:90-93`, -dot-out `:122-125`, -df-out
  `:130-133`. REALITY at 8a4520d9: reset `:68`, Query::Build call `:71`,
  SetRelDumpStream `:80`, Program::Build call `:83`, -ir-out `:91-94`, -dot-out
  `:123-126`, -df-out `:131-134` (all +1 from the -ir-out/-df-out drains
  because `auto ret = EXIT_SUCCESS;` at :89 shifts the post-Program blocks down
  by one; -dot-out and -contract-out net to their listed lines). VERIFIED that
  the timing SPLIT (rel-out sink pre-Build, ir/df/dot/contract drains
  post-Build) holds unchanged. ANCHOR: `Main.cpp:68, 71, 80, 83, 91-142`.
- **DB-13 — Program::Build pre-pass + context anchors.** Part R §1 (`:105-125`)
  used head `:1308-1311`, pre-pass `:1332-1411`, demand_instance gate `:1428`
  + fences `:1467-1473`, num_errors `:1478-1480`, demand_forcings `:1491`,
  demand_instance_enabled `:1497`, BuildDataModel `:1499`. REALITY at 8a4520d9:
  head `:1331-1334`, pre-pass `:1342-1435`, demand_instance block `:1451-1499`,
  num_errors gate `:1501-1503`, demand_forcings `:1514`, demand_instance_enabled
  `:1520`, BuildDataModel `:1522` — the whole body slid ~+23 lines
  (code above grew). VERIFIED unchanged: `ValidateDROps` census recount
  `Rel.cpp:3999-4021`, `ExtractPrimaryProcedure` body `Procedure.cpp:777-928`,
  `BuildSubgraphInstanceOps` `Rel.cpp:1036` (E-142-pinned). ANCHOR:
  `Build.cpp:1331-1522`.

#### Inter-bundle agreements + the one framing note

- No bundle DISAGREED on substance; the only cross-bundle tension is the head
  anchor of `Query::Build` — Part R §1's `:2518` vs Bundle 1's measured
  `:2524-2527` — resolved in DB-11 as pure line-drift at the newer tip.
- **Framing (not a code drift):** `stage-b-seed.md:63-68` asserts "there is NO
  planning object; Rel/ControlFlow read QueryImpl directly." The read-surface
  fleet REFINES this: downstream libraries read the `Query` WRAPPER (by
  value/const-ref), never `QueryImpl*` — the only `.impl->` reach is
  Format-local (`QueryContracts`→`row_contracts`, `Format.cpp:1531`). The
  Stage-B thesis (one frozen `PlanningRegionalProgram`) therefore starts from
  an ALREADY-CLEAN wrapper boundary; the frozen accessor's budget is B.4's
  table, and `Program::Query()` (`Program.cpp:260-262`) is the one seam that
  must survive verbatim for CodeGen.


---


# Part R3 (2026-08-03, session 5) — the DIFF-R3 grain, fleet-verified at tip

DIFF-R3 adds a USER-DECLARED region-key surface (bracket syntax on `#local`/
`#export`), forks the demand pass between KEY INFERENCE and a V-DECLARED-KEY
check, and (DIFF-NEXT-3 Tier 1) makes the demanded INTERIOR relation NAMEABLE
in the Regional dump by carrying its `ParsedDeclaration` from mint to freeze.
Four surfaces, at Part-R grain, fleet-corrected at the `keyed-instances` tip.
Earlier sections are NOT edited beyond the in-place SHIFTED-anchor fixes logged
in the DRIFT LEDGER (Part R3).

## R3.1 The bracket surface in the CURRENT lexer/parser

**Lex layer — TWO enumerators + TWO char-dispatch arms, ZERO spelling edits.**
`Token.h:224-252` is the `kPunc*` enum block (`kPuncOpenParen`=224,
`kPuncCloseParen`=225, `kPuncOpenBrace`=227, ...); **no `kPuncOpenBracket`/
`kPuncCloseBracket` exists anywhere in Token.h** (grep-confirmed). A bracket
surface adds two enumerators beside 224-225.

`Lexer.cpp:105` opens `switch (ch) {`; the `'('` arm at `Lexer.cpp:138-143` is
the verbatim idiom every new single-char punctuation arm mirrors — and **no
`'['`/`']'` case exists anywhere in that switch today**:

```
case '(': {                                   # Lexer.cpp:138-143
  auto &basic = ret.As<lex::BasicToken>();
  basic.Store<Lexeme>(Lexeme::kPuncOpenParen);
  basic.Store<lex::SpellingWidth>(1);
  return true;
}
```

`As<lex::BasicToken>()` + `Store<Lexeme>` + `Store<SpellingWidth>(1)` is the
ENTIRE per-char contract. There is **NO Lexeme->string spelling table** (the
seed's "punctuation-spelling table" framing was WRONG — see DRIFT LEDGER):
`Token.cpp` has only `Token::SpellingRange()` (`Token.cpp:70`, recomputes a
`DisplayRange` from stored `Position`/`SpellingWidth`), and rendering is the
single default branch of `operator<<(OutputStream&, Token)` at
`Format.cpp:9-16` (`default: os << tok.SpellingRange();` at `:13`). So a
future `kPuncOpenBracket` renders correctly with **ZERO edits to Token.cpp /
Format.cpp** — only the two Token.h enumerators + the two Lexer.cpp arms.

**Parser state machine.** `ParserImpl::ParseLocalExport` (`Parser.cpp:354`) is
a template `<NodeTypeImpl, kDeclKind, kIntroducerLexeme>` with two instantiation
call sites — `Parser.cpp:1174` (`ParsedExportImpl`) and `:1183`
(`ParsedLocalImpl`) — so **a state-1a amendment written ONCE here covers both
`#local` and `#export` for free**. Idiom: `int state = 0;` (`:374`) drives a
`switch (state)` (`:406`) inside the sub-token loop `for (next_pos =
tok.NextPosition(); ReadNextSubToken(tok); ...)` (`:396`); each branch does
`state = N; ...; continue;`. The `(name-atom)->kPuncOpenParen` transition is
`case 1` at `Parser.cpp:421-433`:

```
case 1:                                       # Parser.cpp:421-433
  if (Lexeme::kPuncOpenParen == lexeme) {
    state = 2;
    clause_toks.push_back(tok);
    continue;
  } else {
    context->error_log.Append(scope_range, tok_range)
        << "Expected opening parenthesis here to begin parameter list of "
        << introducer_tok << " '" << name << "', but got '" << tok
        << "' instead";
    return;
  }
```

The reject idiom is uniform (`case 0` at `:414-419`): `error_log.Append(
scope_range, tok_range) << "Expected ..." << ... << "but got '" << tok <<
"' instead"; return;`. **EOF/termination**: the `switch` has NO per-state EOF
branch; the loop simply exits when `ReadNextSubToken(tok)` returns false, and
the external gate at `Parser.cpp:856-869` is the real terminator — `state 9`
(reached only via the terminating-period transition in `case 8`, `:786-794`,
target `:840`) is the SOLE accept state; **any other state at loop-exit
(including a mid-parse EOF) hits `RemoveDecl(local)`**:

```
if (state != 9) {                             # Parser.cpp:856-869
  err << (local ? "...must end with a period"
                : "Incomplete declaration; ...must end with a period");
  RemoveDecl(local);
} else { ...FinalizeDeclAndCheckConsistency(local)... }
```

A new bracket-bearing `state 1a` (after the name atom, before the `(`) must
mirror this: its own in-switch reject branch for a malformed bracket body, and
reliance on the external `state != 9` truncation gate for a bracket left open
at EOF (it never advances to a period, so RemoveDecl fires — no bespoke EOF
handling needed inside the switch).

**Accessor home.** `#local`/`#export` are DISTINCT public wrappers —
`ParsedLocal` (`Parse.h:606-632`), `ParsedExport` (`Parse.h:567-594`), both
convertible to the shared `ParsedDeclaration` handle (`Parse.h:405-498`), NOT
unified. Region-key is `#local`/`#export`-only, so the accessor pair (e.g.
`HasRegionKey()`/`RegionKey()`) is scoped to `ParsedLocal`/`ParsedExport`, a
sibling of `HasMutableParameter` (`Parse.h:439`) / `IsInline` (`:468`). Storage
lives on the shared impl `ParsedDeclarationImpl` (`lib/Parse/Parse.h:341`, a
plain-field aggregate — `std::vector<Token> parsed_tokens;` at `:379`): a
`std::vector<unsigned> region_key_param_indices` field fits as a sibling of
`parsed_tokens`, left empty for non-local/export decls (as
`has_mutable_parameter` does). Accessors are out-of-line in
`lib/Parse/Parse.cpp` (idiom: `HasMutableParameter` `:847`, `IsInline` `:959`),
forwarding through `impl->`.

## R3.2 The inference-vs-check fork — Step 2 -> [Step 2b V-DECLARED-KEY] -> Step 3

The demand pass currently INFERS the key (`p_bound`) purely from graph shape;
the bracket surface adds a CHECK arm that validates the inferred key against
the user's declaration. The fork slots BETWEEN the two existing steps in the
per-adornment Loop-1 body:

```
Step 2 (Demand.cpp:507-600):  infer p_bound from the projection chain
Step 2b [V-DECLARED-KEY, NEW]: reconcile p_bound vs p's declared region key
Step 3 (Demand.cpp:607-783):  locate every guard site (uses the checked key)
```

`p_bound` is declared `std::vector<unsigned> p_bound;` at `Demand.cpp:510`,
filled `p_bound.push_back(in_col->Index());` at `:576`, and stashed
per-adornment at `:792-794` (`plan.push_back(PerAdornment{redecl,
std::move(bound_indices), std::move(p_bound), ...})`).

**THE SCOPING PROBLEM (load-bearing).** The bracket is ONE relation-scoped
declaration (a property of `p`), but the inference runs PER ADORNMENT inside
the Loop-1 head:

```
for (ParsedDeclaration redecl : q_decl.UniqueRedeclarations()) {   # Demand.cpp:477
  ...
  std::vector<unsigned> p_bound;                                    # :510, FRESH per iteration
  ...                                                               # filled at :576
}
```

`p_bound` is a fresh local per adornment; two adornments of one query name
(`q(bound,free)`, `q(free,bound)`) produce DIFFERENT `p_bound` over the SAME
relation `p`. V-DECLARED-KEY must NOT re-derive the declared key per adornment
— it checks each adornment's inferred `p_bound` (equivalently the transferred
member key) for CONSISTENCY with the single relation-scoped bracket, rejecting
adornments whose demanded key disagrees with what the user declared.

**Clean-reject idiom (TWO tiers).** In-pass, every fence returns the local
`reject` lambda (`Demand.cpp:407-411`):

```
const auto reject = [&](const char *what) -> bool {                # Demand.cpp:407-411
  log.Append(module.SpellingRange())
      << what << "; recompile without -demand";
  return false;
};
```

so V-DECLARED-KEY is `return reject("declared region key disagrees with the
demanded binding pattern");`. The outer gate is at the CALL SITE
(`Build.cpp:2593-2598`): `if (!impl->ApplyDemandTransform(...)) return
std::nullopt;` followed by `if (num_errors != log.Size()) return
std::nullopt;`. NOTE: V-DECLARED-KEY needs `p`'s `ParsedDeclaration` to read
the bracket — which is **NOT in scope here today** (see R3.3(i)); the check
depends on the same new plumbing as the naming lift.

## R3.3 The Tier-1 naming lift (DIFF-NEXT-3 Tier 1)

**(i) Mint-time snapshot — and its missing variable.** The recognition is
minted at `Demand.cpp:1141-1143` (a 3-line push, NOT a bare `:1142`):

```
recognized_subgraphs.push_back(                                    # Demand.cpp:1141-1143
    RecognizedSubgraph{forcing_index, QueryView(p_merge), p_bound,
                       QueryView(q_insert), std::move(guard_indices)});
```

`RecognizedSubgraph` (`Query.h:1028-1034`) has NO `ParsedDeclaration` field;
Tier 1 adds one (a `pub_decl`/`demanded_decl` twin beside the existing
`pub_view`/`demanded_view` QueryView identities). **But `p`'s ParsedDeclaration
is NOT reachable at the mint site.** The only ParsedDeclaration-typed locals in
the whole function are for the QUERY (`q_decl` at `:442`, `redecl` at
`:457`/`:477`) and the two FABRICATED demand decls (`d_msg_decl` `:923`,
`d_local_decl` `:944`). `p` is identified PURELY STRUCTURALLY as `p_merge` (a
`MERGE*`), landed by the Step-2 graph descent at `Demand.cpp:573`
(`p_merge = m;`) — no `REL*`/`ParsedDeclaration` for `p` is ever obtained.
`QueryRelationImpl` (`Query.h:130-146`) DOES carry `const ParsedDeclaration
declaration;`, but nothing walks `impl->relations` to map `p_merge` back to its
REL — that reverse lookup **does not exist anywhere today**. The declaration
lives transiently in Connect's per-relation loop as `rel->declaration` at the
moment `insert_proxy` (== `p_merge`) is minted:

```
for (REL *rel : relations) {                                       # Connect.cpp:229
  ...
  VIEW *const insert_proxy = CreateProxyForMutableParams(           # Connect.cpp:260-261
      this, CreateProxyOfInserts(this, rel->inserts), rel->declaration);
  ...                                                               # rel->declaration then DISCARDED
}
```

So Tier 1 needs NEW plumbing: **(a)** stash a `ParsedDeclaration` back-pointer
on the created `insert_proxy` view at `Connect.cpp:260-261` (single-writer,
cleanest) threaded to Demand; OR **(b)** a `relations`-list correlation pass in
Demand.cpp mapping `p_merge`->REL. The variable that reaches `p`'s decl at its
ONLY live site is `rel->declaration` (`Connect.cpp:260-261`), NOT anything at
`:1141-1143`.

**(ii) Freeze-time resolve — a ProgramImpl-FREE clone of
`ResolveLiveRecognition`.** The Rel.cpp original is at `Rel.cpp:936-1025`. Its
ONLY ProgramImpl dependency is one lambda: `model_table = [&](QueryView v) ->
TABLE* { auto it = impl->view_to_model.find(v); return ... it->second->
FindAs<DataModel>()->table; }` and its three call sites `model_table(jl[0])`,
`model_table(jl[1])`, `model_table(iv)`. **CARRIES OVER verbatim to a Regional
clone in `lib/Regional/Planning.cpp` (pure `Query`/`QueryView` graph API, no
ProgramImpl):**
  - `query.GuardAnnotations()` / `query.DemandForcings()`;
  - the bucketing walk `query.ForEachView([&](QueryView v){ ai =
    v.GuardAnnotationIndex(); if (ai == QueryView::kNoGuardAnnotation) return;
    guards[annots[ai].forcing_index].emplace_back(v, ai); })` — pure view-flag
    read (`kNoGuardAnnotation = ~0u`, `Query.h:445-446`);
  - the per-forcing JOIN decomposition `v.IsJoin()`,
    `QueryJoin::From(v).JoinedViews()` -> `jl[0]`=demand side, `jl[1]`=body
    guard's summarized monotone input; the `annots[ai].role ==
    GuardAnnotation::kBody` filter; `instance_key = annots[ai].instance_key`;
  - the pub-resolution walk: `ParsedDeclaration q_decl(forcings[fidx].query)`,
    iterate `query.Inserts()`, match `ins.Declaration().Id() == q_decl.Id()`,
    `QueryView::From(ins)`, walk `iv.Predecessors()` for the name-view /
    column names.
**DOES NOT EXIST at freeze time (ProgramImpl-only):** the `model_table` lambda
and any `TABLE*` — `impl->view_to_model` / the DataModel tables are built inside
`Program::Build`, AFTER the Regional freeze. So the clone resolves the live
INTERIOR relation as a `QueryView` (the guard JOIN `v` == `demanded_view`, or
`jl[1]` the summarized input) but CANNOT turn it into a `TABLE*` storage
identity (`demand_table`/`input_table`/`pub_table`). Portable anchor:
`GuardAnnotationIndex` stamp + DefList-ordered `ForEachView` (HP-9); Tier-1
names `p` from parse identity (the new `Connect`-stashed field, or re-derived
from the resolved QueryView), never via a table.

**(iii) Contract emission + census arm — R-STORE loop determinism.** A new
contract for the interior relation slots into the SAME single deterministic
walk `CollectContractInserts` (`Planning.cpp:178-196`) drives. Its stated rule
(doc comment `:171-177`): distinct non-demand relation-insert declarations,
`demand__`-prefixed skipped (`starts_with("demand__")`), FIRST-encounter order
over the `query.Inserts()` DefList range — **a source-position sort is
deliberately NOT used** ("the first-encounter order is already a pure function
of the final graph"). The render loop is `Planning.cpp:390-432` (member-key
positional render `:403-423`), `edge_index` numbering `unsigned edge = 0u;`
incremented per entry (`:389`). A SECOND contract source (the interior `path`/
`rel`) must obey the SAME discipline: derived from a deterministic, graph-pure
traversal (NOT a side registry iterated in pointer/hash order), with its own
DISJOINT `edge_index` numbering or merged into the same `E0,E1,...` sequence.
`DeriveRegionalCensus` (`Planning.cpp:200-217`) is the single census authority,
re-derived independently and cross-checked at `:434-461` (fprintf+abort,
V-REGION-CENSUS) — a new arm must bump `census.row_contracts` in BOTH the
stored count and the independent recount. Emitters: the row-contract line at
`Format.cpp:133-138` (`row-contract  E<i>  rel=...  member-key=(...)
support=<monotone|differential>`) and the census line at `Format.cpp:145-151`.

## R3.4 The verified witness ground truth

In BOTH current goldens the lone `row-contract` names the PUBLISHED
query-answer relation, NOT the interior demanded relation — R-STORE walks
`query.Inserts()` for relation-backed INSERTs, and under `-demand` the interior
view is guard-materialized/inlined (no standalone relation-insert), so only the
answer relation surfaces today. Tier 1 makes the interior relation nameable (a
second `row-contract`, `row-contracts` census += 1).

**`demand_tc_witness.dr`** — `#message edge_2(u64 From, u64 To).` +
`#local path(u64 From, u64 To).` (two clauses) + `#query
reachable_from(bound u64 From, free u64 To) : path(From, To).`. Interior
relation nameable under Tier 1 = **`path`**. Current
`demand_tc_witness.region.opt.golden` (14 lines):

```
region-program
program-root {
  input-abi   edge_2/2(From:u64, To:u64)                   -> R0 via P1
  query-abi   reachable_from(From:bound u64, To:free u64)  -> R0 via P0
  output-abi  <none>
}
region R0  owner=program-root  parents=()  children=() {
  request-port     P0  query=reachable_from  fields=(From)
  input-port       P1  message=edge_2/2      fields=(From, To)
  region-internal  demand__reachable_from_bf/1(p0:u64)  [fabricated, driver-suppressed]
  row-contract     E0  rel=reachable_from  member-key=(From, To)  support=monotone
}
census: regions=1 child-calls=0 program-roots=1 request-ports=1 input-ports=1 result-ports=0 row-contracts=1
```

**`demand_multi_adorn_witness.dr`** — `#message edge_2(u64 A, u64 B).` +
`#local rel(u64 A, u64 B).` + `rel(A, B) : edge_2(A, B).` + `#query
q(bound u64 A, free u64 B).` + `#query q(free u64 A, bound u64 B).` +
`q(A, B) : rel(A, B).`. Interior relation nameable = **`rel`** (note the
collision with the dump's own `rel=` token — a Tier-1 emitter test must not
confuse the relation named `rel` with the `rel=` field). Current
`demand_multi_adorn_witness.region.opt.golden` (16 lines):

```
region-program
program-root {
  input-abi   edge_2/2(A:u64, B:u64)                -> R0 via P2
  query-abi   q(A:bound u64, B:free u64)  adorn=bf  -> R0 via P0
  query-abi   q(A:free u64, B:bound u64)  adorn=fb  -> R0 via P1
  output-abi  <none>
}
region R0  owner=program-root  parents=()  children=() {
  request-port     P0  query=q  adorn=bf  fields=(A)
  request-port     P1  query=q  adorn=fb  fields=(B)
  input-port       P2  message=edge_2/2   fields=(A, B)
  region-internal  demand__q_bf/1(p0:u64)  [fabricated, driver-suppressed]
  region-internal  demand__q_fb/1(p0:u64)  [fabricated, driver-suppressed]
  row-contract     E0  rel=q  member-key=(A, B)  support=monotone
}
census: regions=1 child-calls=0 program-roots=1 request-ports=2 input-ports=1 result-ports=0 row-contracts=1
```

## DRIFT LEDGER (Part R3)

- **SHIFTED anchors fixed in place this session** (Part R / R.1.x, edited per
  the task's explicit override of the append-only convention):
  `Build.cpp` `412-494`->`413-490` (BuildQueryInjectorFromRegistry, at the
  R.1.1 tail, the `# :412`->`# :413` op-line, and D4),
  `506-519`->`507-520` (BuildQueryInjectorProcedure, R.1.1 tail + `# :506`->
  `# :507` + D4), `532`->`533` (BuildQueryEntryPointImpl),
  `622-637`->`623-638` (BuildQueryEntryPoint driver),
  `1508-1514`->`1517-1523` (Context wiring); `Main.cpp` `123-126`->`141-144`
  (-dot-out wire, R.1.4), `136-142`->`154-160` (-contract-out wire, R.1.4);
  `Rel.cpp` `4977`->`5009` (CheckInstanceSolePub / V-INST-SOLE, +32 file-wide).
- **Part B B.1/B.5 are STALE-BY-CONSTRUCTION, not merely shifted** (NOT patched
  in place — logged here). B.1's title "bin/drlojekyll/Main.cpp, 621 lines" is
  now WRONG: Main.cpp is **677 lines**, and `CompileModule` now really runs
  `auto frozen_opt = FrozenRegionalProgram::Build(*query_opt, error_log);`
  (`Main.cpp:81`) between `Query::Build` and `Program::Build`, drains
  `-region-out`/`-region-dot-out` (`:85-92`), and calls `Program::Build(
  *frozen_opt, ...)` (`:100-102`). B.5's cited `Program::Build(const
  ::hyde::Query &query, ...)` signature is WRONG: the parameter is now `const
  FrozenRegionalProgram &frozen` (`Build.cpp:1333`) with `const ::hyde::Query
  &query = frozen.Query();` as the new first statement (`:1337`) — this is
  H1-ALT / H4 (Query::Build's own signature UNCHANGED, confirmed
  `return Query(std::move(impl));` at `Build.cpp:2638`ff), the LANDED Stage-B
  freeze the earlier Part B treated as a hypothetical future hunk. The whole
  B.1/B.5 line tables need fresh grep-based re-derivation before use (new lines
  were INSERTED, not flat-shifted): e.g. `num_errors` is now `Build.cpp:1343`.
- **Corrected seed framing (lex-parse item 3): there is NO punctuation-spelling
  table.** Any plan that edits a "Lexeme->string table" for brackets is chasing
  a non-existent artifact — bracket spellings render free via
  `Token::SpellingRange()` (`Token.cpp:70`) + the `Format.cpp:9-16` default
  branch. Bracket lex needs ONLY Token.h enumerators + Lexer.cpp arms.
- **Corrected mint anchor:** the RecognizedSubgraph mint is `Demand.cpp:
  1141-1143` (a 3-line push), not a bare `:1142`.