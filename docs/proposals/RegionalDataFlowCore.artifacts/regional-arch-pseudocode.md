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
