======================================================================
witness-deltarel-target.md — THE HAND-WRITTEN DeltaRel-IR TARGET FOR ONE
DEMAND-KEYED-INSTANCE WITNESS (nested lowering)
======================================================================
*** SUPERSEDED-IN-PART (§19 checkpoint, 2026-07-20; erratum E-80). This
*** PAPER-ERA target predates the REAL flat dumps and the D1 pinned
*** design. Corrected claims — read d1-ground-truth-nbhd.md (GT-1..GT-6),
*** d1-design-consolidated.md, d1-desired-states.md, d1-pinned.md first:
***  (1) %table NUMBERING is the borrowed d3 map; the REAL demand-ON flat
***      map is GT-4 (%table:8=demand, %table:11=edge, %table:15=guarded
***      copy, %table:4=answer — pub/answer is 15+4, not 4).
***  (2) The fabricated demand table lowers MONOTONE as landed (GT-1);
***      the ENTIRE witness is monotone — there IS no acyclic claim/
***      frontier/commit tail to ride (GT-2), and the band-(a) minus arm's
***      $net-removal vec is never minted. Death's trigger is UNREACHABLE
***      until the staged D3.a retraction surface (-demand-retract) lands
***      (d1-pinned.md OD-1).
***  (3) Death-as-minus-arm is superseded by the §18(B) OWN-OP split as
***      pinned: kSubgraphInstantiate / kInstanceDeath / kInstanceSeal
***      with the zero-counter death signature (d1-pinned.md HP-3).
***  (4) §2.6's draft nested dump block is STALE; the amended nested
***      block is d1-desired-states.md §B.4 (no DRVec line, monotone
***      kCommitSweep on the demand table, re-derived labels/deps).
*** The H1-H11 hole list REMAINS the live index of design questions; each
*** is dispositioned in d1-design-consolidated.md §0/§A and d1-pinned.md.
======================================================================
Target-artifact lane, demand-keyed-instances / implicit-asynchrony epoch.
Branch `keyed-instances`, tip fa9a8cc2 (D0 fleet @60821adf substrate).
Written 2026-07-18, checkpoint step 3, artifact 1. PAPER-FIRST: this hand-
writes the DESIRED DeltaRel-IR (the `-deltarel-out` dump the owner directed
into being, KeyedInstances.md §0.5(2)) for ONE witness lowered the NESTED
(demand-keyed-instance) way, in the ir-dump-formats.md §2 syntax, so the
artifact doubles as the dump-format exercise. It lowers TO the R-A frozen-
pair store (d3-instance-store-target.md A0-A11). A future implementer builds
FROM this; a judge falsifies it.

INPUTS THIS ARTIFACT IS BUILT ON (all read this session):
  - d3-instance-store-target.md A0-A11 — the R-A frozen-pair emission target
    (two monotone tables/instance, swap-at-seal, whole-table old/new; F1/F2
    discharged; A5 monotone-input fence; A7 census; A8 recognizer-gate).
  - v3-spec.md §2 + §2.1 — THE STYLE MODEL (the effect domain; the §2.1
    tabular per-op effect table). Every effect set below is in §2.1 style.
  - v3-spec-statecell.md §0-§2 — the GROUP_UPDATE/StateCell precedent (the
    two-word cell, the fold≠counter C-0c split, the sealed=kInI-frozen /
    working=hazard dichotomy). SUBGRAPH_INSTANTIATE is its one-level-up peer.
  - fleet-d0/lane-drir.md — the VERIFIED GROUP_UPDATE mint/band/validator
    reality (BuildGroupUpdateOps DR.cpp:638-761; the census; the band-key
    closures; op_stratum NO-DEFAULT-0). Its vocabulary and anchors are used.
  - fleet-d0/lane-rastore.md — what the runtime ACTUALLY provides (the R-A
    store is 100% paper; three new items MakeTable/InstanceStore/Reset();
    the Arena-Free-no-op soundness gap; the frozen->Find/current->Find reads).
  - fleet-d0/lane-pipeline.md + E-46 — the THREE guard-site kinds
    (kReadAtTuple / kPushDown / kBaseAtom) the query-side annotation marks.
  - tests/OptDiff/cases/demand_tc_witness.{dr,batches,main.cpp} + goldens —
    the FLAT-lowering witness as landed (the two-lowerings equivalence base).
  - ir-dump-formats.md §2 — the `-deltarel-out` dump syntax used verbatim.

STATUS OF THE THING BEING DUMPED: the SUBGRAPH_INSTANTIATE / INSTANCE_SEAL
op family DOES NOT EXIST in code (lane-drir §"ZERO instance-op code";
lane-rastore PART 1 "DO NOT EXIST"). This is the hand-written TARGET the ops
must produce, not a dump of a live compiler.

======================================================================
§1. THE WITNESS PROGRAM — PICK, WITH THE OWNER-DECIDES RECOMMENDATION
======================================================================

RECOMMENDATION: **re-lower `demand_tc_witness` nested** (the landed flat
witness, tests/OptDiff/cases/demand_tc_witness.dr), do NOT mint a new
dedicated witness. Reasons for, and the one genuine argument against,
below — the owner decides.

The witness .dr (unchanged from the landed flat case):

    #message edge_2(u64 From, u64 To).
    #local path(u64 From, u64 To).
    path(F, T) : edge_2(F, T).
    path(F, T) : path(F, M), edge_2(M, T).
    #query reachable_from(bound u64 From, free u64 To) : path(From, To).

FOR re-lowering demand_tc_witness nested (five reasons):

  1. THE TWO-LOWERINGS EQUIVALENCE GATE IS THE POINT (PerfRoadmap §18.5(C),
     the completeness gap consolidated.md §5 flags UNVERIFIED). The whole
     epoch thesis is that flat (D4, landed) and nested (D1, this epoch) are
     TWO LOWERINGS of one object p^α that must be ANSWER-IDENTICAL under the
     same oracle. The equivalence is only exercisable if BOTH lowerings run
     the SAME .dr on the SAME batches against the SAME oracle/monotone
     goldens. demand_tc_witness ALREADY has `.batches` + `.oracle.stdout` +
     `.monotone.stdout` + `.drflags` (`-demand`) + a driver. A new witness
     would have to reproduce all of that, and — critically — would NOT be
     the flat case, so the equivalence claim would compare two different
     programs. Re-using demand_tc_witness makes the two lowerings share
     literally one .dr and one oracle. This is the strongest reason.

  2. THE ORACLE ALREADY REFEREES ANSWER-IDENTITY. The `.batches`/`.oracle`
     sidecar evaluates the PLAIN (undemanded) program definitionally
     (consolidated.md §3 "the oracle runs the PLAIN .dr with NO drflags;
     demand changes materialization, never answers"). So the oracle is
     lowering-AGNOSTIC by construction — it already certifies that whatever
     the nested lowering publishes for `reachable_from(From, To)` equals the
     full closure for each probed key, EXACTLY as it does for the flat
     lowering today. No new oracle wiring; the existing goldens are the gate.

  3. IT IS THE OWNER-BLESSED FIRST IR-GOLDEN CARRIER. KeyedInstances.md
     §0.5(5) names demand_tc_witness (with fixpoint_stress_1, reconverge_1,
     the aggregate corpus) as a natural first carrier of the new
     `.deltarelgold` sidecar, and §0.5(5) says "demand-ON IR goldens become
     valid the moment (F) lands — and ARE the restored gate." The nested
     lowering's `-deltarel-out` dump (this artifact's target) becomes that
     case's `demand_tc_witness.deltarelgold` content once emission lands.

  4. IT SATISFIES THE A5 MONOTONE-INPUT FENCE AS WRITTEN (d3 N2, :1638-1659).
     The nested R-A lowering ships only over ADD-ONLY summarized inputs. The
     witness's summarized input is `path`... which is RECURSIVE (an SCC
     table), not `edge`. See the argument AGAINST (below) — this is where
     the tension lives, and why the recommendation is CONDITIONAL.

  5. IT REUSES THE E-32 DEMAND EDGE ALREADY PROVEN STRUCTURAL. d3 §4.2 names
     the exact real equi-join demand edge for the neighborhood witness; the
     tc witness has the identical shape one adornment up (d_path ⋈ path on
     From), so the SUBGRAPH_INSTANTIATE consumes an edge the flat lowering
     already mints and CSE-distinguishes (Demand.cpp MintGuardJoin, E-32).

AGAINST (one real argument, LOUD): demand_tc_witness's demanded body is
RECURSIVE — `path(F,T) : path(F,M), edge_2(M,T)` — so the summarized input
to the demand-keyed instance is `path` ITSELF (an INDUCTION-owned SCC
table), NOT a monotone `edge`. This directly collides with:
  - d3 A5's fence: "assert every SUMMARIZED INPUT view of the subgraph is
    MONOTONE ... any deletion-capable summarized input is REJECTED." `path`
    is differential AND recursive.
  - the CLEAN-DIAGNOSTIC gap "aggregates/KV over INDUCTION-OWNED
    (recursively-derived) inputs" (CLAUDE.md) — the instance analog is a
    subgraph over a recursively-derived body, which the stage-(b) R-A
    lowering does NOT handle (A6's reserved incremental arm).
  - the ViewSelfReachable acyclic-demand fence (d3 §4.5 UNCHANGED; the
    "through the instance" predicate consolidated.md §5 flags as NEW).

  So `demand_tc_witness` AS WRITTEN is NOT inside the stage-(b) R-A fence:
  the frozen-pair whole-instance-rebuild lowering assumes a NON-recursive
  demanded body materialized by a monotone JOIN (the d3 §4.1 neighborhood
  shape: `neighborhood(Start,Node) : demand_neighborhood(Start),
  edge(Start,Node)` — one monotone hop, no self-reference).

RESOLUTION OFFERED (the owner picks A or B):

  PICK-A (RECOMMENDED): keep demand_tc_witness as the FLAT witness and the
  answer-oracle base, and add a SECOND, non-recursive sibling — call it
  `demand_neighborhood_witness` — that IS the d3 §4.1 shape (one monotone
  hop), carrying its OWN `.batches`/`.oracle`/`.monotone`/`.drflags` and,
  once emission lands, its `.deltarelgold` = this artifact's dump. The
  nested lowering de-risks on the sibling (inside the A5 fence); the
  two-lowerings equivalence is then stated for the SIBLING under both
  flat-and-nested (both are single-adornment, single-hop, add-only —
  equivalence is clean). demand_tc_witness remains the RECURSIVE flat
  witness (it is already the flat gate; the nested lowering FENCES it out
  with the clean A5 diagnostic, which is itself a witness — a `demand_
  multi_adorn_1`-style reject case proving the fence fires).

  PICK-B: re-lower demand_tc_witness nested anyway, accepting that this
  forces the A6 incremental-maintenance arm (R-B differential nested
  DiffTable) THIS epoch — a much larger scope (re-opens HOLE-B, needs the
  nested-Commit ordering, d3 A6). NOT recommended for stage (b); it makes
  the epoch the incremental-instance epoch, not the frozen-pair epoch.

THIS ARTIFACT LOWERS THE `neighborhood` SHAPE (PICK-A's sibling / d3 §4.1),
because that is the shape the R-A frozen-pair store is DESIGNED for and the
only shape the stage-(b) lowering can emit. The witness .dr it targets:

    #message add_edge(u64 From, u64 To).      ; add-only (no @differential; d3 N2)
    #local edge(u64 From, u64 To).
    #query neighborhood(bound u64 Start, free u64 Node) : edge(Start, Node).

with batches chosen to exercise BIRTH / REBUILD / DEATH (§3). One bound
query, single adornment (bf), non-recursive body, monotone input — inside
every stage-(b) fence. The recursive demand_tc_witness stays the flat gate
and the fence-reject witness; the owner may instead choose PICK-B.

RATIONALE FOR NOT INVENTING A THIRD NEW PROGRAM: the `neighborhood` shape is
already the d3-canonical witness (d3 §4.1 hand_demand.dr, compiled and
IR-anchored that session); re-using its exact shape keeps this artifact's IR
anchored to d3's %table:4/:8/:11 numbering, so the two artifacts cross-check.


======================================================================
§2. THE TWO OPS — SUBGRAPH_INSTANTIATE + INSTANCE_SEAL, FULL EFFECT SETS
    (v3-spec §2.1 tabular style; the GROUP_UPDATE peer, one level up)
======================================================================

ONE SUBGRAPH_INSTANTIATE + ONE INSTANCE_SEAL per subgraph view, exactly as
BuildGroupUpdateOps mints ONE kGroupUpdate + ONE kStateSeal per agg view
(lane-drir §1). The instance op is the GROUP_UPDATE peer at the SET level:
where GROUP_UPDATE folds a SCALAR summary per (group++config) into a two-word
StateCell, SUBGRAPH_INSTANTIATE (re)materializes a ROW SET per (context++
config) into an R-A frozen-pair (two monotone tables).

[AMENDED: F1/HIGH + F6/LOW — the family was FOUR kinds silently dropping
d3 §4.3/§5.2's MANDATORY `instance:demand`(kDemandRead); it is now FIVE,
reconciled verbatim with d3's ratified family, and the seal EffKind is
named `instance:seal`(kInstanceSeal) with `global:rmw` demoted to a hazard
CLASS (not the kind), per d3 §4.3 :772.]

NEW EffKind FAMILY (v3-spec §2 reserved-sub-domain pattern; the statecell
tail's sibling, DR.h:81-83): `instance: demand(I) | rebuild(I) | emit(I) |
old(I) | seal(I)` — FIVE kinds, EXACTLY d3 §4.3's ratified family (:734-772),
`instance:demand`(kDemandRead) the FIRST enumerated member. Justification for
a NEW family (NOT reusing statecell:*): the payload is a WHOLE-TABLE
membership read (frozen->Find / current->Find, lane-rastore A1/A2), not a
scalar value fold — the C-0c value-fold≠counter argument applies one level up
(a set-rebuild is neither a counter nor a scalar fold). The linearizer's
effect→access switch (DR.cpp:3206-3246) needs a new case per new EffKind or
hazards go unmodeled (lane-drir §4, silent-miscompile risk):
    instance:demand(I)  -> READ the demand KEY (which Start is born/killed),
                             one per band-(a) SIGN arm; FROZEN (kInI-like:
                             NO WAR/WAW, RAW-only against instance:seal —
                             the acyclic-frozen read of the drained demand
                             pivot's key, d3 §4.3 :735 "kDemandRead — FROZEN,
                             p2b §1.5 acyclic"). DISTINCT from the kVecDrain
                             of the demand ROW frontier: the drain yields the
                             rows, instance:demand is the frozen read of the
                             KEY those rows carry. Without it the linearizer's
                             dependence graph has NO node for "read the demand
                             key" — the silent-omission class the census kills.
    instance:rebuild(I) -> WRITE current (a set materialize; hazards vs emit).
                             STRUCTURAL two-arm pair, NOT a value fold: the
                             plus arm TryAdd-materializes, the minus arm
                             Recycle-to-empties (see §3 / F3 — the ± arms do
                             NOT cancel a value the way StateCell folds do).
    instance:emit(I)    -> READ current  (RAW after rebuild; hazarded)
    instance:old(I)     -> READ frozen   (FROZEN, kInI-like: NO WAR/WAW,
                             RAW-only against instance:seal — the sealed=
                             frozen dichotomy, v3-spec-statecell C-0b, lifted
                             to a whole table; frozen is written only by the
                             swap at INSTANCE_SEAL)
    instance:seal(I)    -> the pointer swap (d3 A1; NOT a Table Seal — the
                             watermark is DISSOLVED, A9). The EffKind is
                             `instance:seal`(d3's kInstanceSeal, §4.3 :772),
                             a member of THIS family; its HAZARD is a
                             store-internal read-modify-write (the "global:rmw"
                             hazard CLASS — NOT the reserved kGlobalRmw
                             engine-scalar EffKind, which does not apply here).
                             Since it trails at band 10 after all reads, its
                             linearizer case is a no-op-hazard on the store
                             (like kStateOld) — nothing reads current/frozen
                             after it within the epoch.

NOTE vs the E-50 seam precedent: the frontier vecs this op DRAINS are the
EXISTING kVecDrain sub-domain (no reservation); only the FOUR set-payload
effects above are the new family. This mirrors E-50's correction (append/
drain is existing; only the genuinely new payload op reserves).

----------------------------------------------------------------------
§2.1 OP: SUBGRAPH_INSTANTIATE — the two-band effect table (§2.1 style)
----------------------------------------------------------------------
Notation exactly as v3-spec §2.1: T = target table; V = a vec; "loop V" =
drain(V); "->V" = append(V); I = the instance store (dense (context++config)
-> iid). Reads are flags:read unless kInI-frozen or instance:old (frozen).

OP: SUBGRAPH_INSTANTIATE(subgraph neighborhood, instance-store I,
        context={Start}, config={}, published={Node}, mode=Rederive,
        class=NonRecursive, provenance=subgraph)          [d3 §4.3, A1/A2]
  ctx = kSeed;  instance_store_id = 0;  pub_table = neighborhood(%table:4)
  demand_table = demand__neighborhood_local(%table:8)  [the d_p^bf relation]
  input_table  = edge(%table:11)  [monotone; A5 fence asserts this]

  BAND (a) instance_rebuild — over the DEMAND frontiers, per SIGN arm.
    (The demand relation is differential: `ask`/the demand seed can retract.
     TWO arms, mirror GROUP_UPDATE band (a)'s 2 per-sign arms, lane-drir §1
     "for sign in {-1,+1}".)  Vec wiring IN: the demand table's net frontiers
     (drained), provisioned by demand's own FRONTIER_FILTERs (E1).
    V-INST-FRESH (d3 NF2) fires HERE — at band-(a) ENTRY, once per TOUCHED
     instance, BEFORE either arm — asserting current->NumRows()==0. It is NOT
     scoped to the plus arm: a death-only (minus-arm-only) epoch (§3.3 batch 3)
     touches the instance and so is checked too, catching a Recycle bug that
     leaves a dead instance's current dirty (the exact NF2 bug). See §3.

    minus arm (sign=-1)  [demand retracted -> instance DEATH candidate]:
      Effects:
        vector:drain(demand_table.net-removal)     [role=net-removal; E1]
        instance:demand(I)                         [read the RETRACTED Start
                                                      key — kDemandRead, FROZEN,
                                                      d3 §4.3 :735; the frozen
                                                      read of the drained key]
        instance:old(I)                            [read frozen — the set to
                                                      retract; FROZEN, no hazard]
        instance:rebuild(I, -1)                    [leave current EMPTY (d3 A3
                                                      minus-arm: NO fold); if a
                                                      same-epoch flap dirtied
                                                      current, Recycle it —
                                                      mid-band, d3 NF3]
      NO table:counter here (crossing is band (b), d3 §4.3 "NO table:counter±
        here" / GROUP_UPDATE C-0c).

    plus arm (sign=+1)   [demand added/changed -> instance BIRTH/REBUILD]:
      Effects:
        vector:drain(demand_table.net-addition)    [role=net-addition; E1]
        instance:demand(I)                         [read the NEW Start key —
                                                      kDemandRead, FROZEN,
                                                      d3 §4.3 :735]
        flags:read(input_table edge %table:11, Present, seed)
                                                    [kFlagRead — the LEAF read
                 of the Rederive rescan of edge(Start,_) via %index:27 on
                 From=Start; a monotone Present scan, add-only. This is the
                 genuine EffKind member. The Rederive rescan itself is a
                 PlanTree carried on the op BODY (v3-spec §3, §7.1
                 std::unique_ptr<PlanNode> body), NOT an effect-set member —
                 "ACCESS" is an OP/PlanNode (v3-spec §2.1), not an EffKind
                 (DR.h:73-84). It appears in the effect multiset ONLY via its
                 leaf flags:read, exactly as v3-spec lowers a leaf ACCESS.]
        instance:rebuild(I, +1)                    [TryAdd each Node into a
                 FRESH current (monotone dedup, Table.h:236); V-INST-FRESH
                 asserts current->NumRows()==0 at band-(a) entry per touched
                 instance, d3 NF2]
      NO table:counter here (same as minus arm).

  BAND (b) publish_touched — over I.Touched() (sort-unique, the StateCell
    Touched mold, lane-rastore). Per touched iid, per candidate row r in
    (frozen's rows UNION current's rows — two full RowAt scans, no union
    primitive, lane-rastore N-2). The ONE-NET-PAIR partition (d3 A2):
      Effects (the READS, once per iid):
        instance:old(I)   -> old_r := (frozen->Find(r)  != kNoRow)  [FROZEN]
        instance:emit(I)  -> new_r := (current->Find(r) != kNoRow)  [current;
                              RAW after band (a) rebuild]
      class = RuleClass(scc_map, pub_table, {input_table}) ; assert ==
        kNonRecursive (the ACYCLIC fence, mirror GROUP_UPDATE lane-drir §1
        "assert klass==kNonRecursive"; d3 §4.3 class=NonRecursive).
      Per r, the occupancy-generalized guard emits AT MOST ONE net pair
      (d3 A2 partition; the 2x2 cells partition r exactly):
        (F,T) born   -> Effects:
            table:counter+(pub_table %table:4, NonRecursive) over (Start++r)
            kInI:read-frozen(pub_table)             [the crossing predicate]
            vector:append(pub_table.add-queue)      [-> outer UPDATECOUNT]
        (T,F) dropped -> Effects:
            table:counter-(pub_table %table:4, NonRecursive) over (Start++r)
            kInI:read-frozen(pub_table)
            vector:append(pub_table.delete-queue)   [-> outer UPDATECOUNT]
        (T,T) unchanged -> NOTHING  [OQ3, the clean R-A win, d3 A2 :1276]
        (F,F) absent    -> NOTHING
    The counter±/kInI-read-frozen/vector:append TRIPLES are the EXACT
    GROUP_UPDATE emit_touched vocab (lane-drir §1 band (b); DR.cpp:707-740).
    SOLE-WRITER (d3 §2/SW-2, V-INSTANCE-SOLE): these counter± are the ONLY
    writers of %table:4's counters; the nested monotone tables NEVER fold
    into %table:4 (read-only membership oracles). HOLE-B foreclosed.

  Queue-append DEF edges (registered EXPLICITLY at mint, mirror DR.cpp:744-
    747 — NOT derived from effects): pub_table.delete-queue.defs += this op;
    pub_table.add-queue.defs += this op. So the linearizer DERIVES the E3
    seed-before-drain edge to %table:4's own claim drain.

----------------------------------------------------------------------
§2.2 OP: INSTANCE_SEAL — the trailing swap (the STATE_SEAL peer)
----------------------------------------------------------------------
OP: INSTANCE_SEAL(subgraph neighborhood, instance-store I)   [d3 A1/A9; E5]
  ctx = kSeed;  instance_store_id = 0;  pub_table = neighborhood(%table:4)
  Effects (per touched iid):
    instance:seal(I)   [the EffKind is `instance:seal` (d3's kInstanceSeal,
        §4.3 :772), a member of the instance family — its HAZARD is a
        store-internal read-modify-write (the "global:rmw" hazard CLASS, NOT
        the reserved kGlobalRmw engine-scalar EffKind). The POINTER SWAP,
        d3 A1:1223-1234: tmp=frozen; frozen=current; current=Recycle(tmp);
        sealed_occupied[iid] = (frozen->NumRows()>0). NOT a Table::Seal —
        the watermark is DISSOLVED (A9); the swap plays STATE_SEAL's role.]
  Effect TOTALITY: EXACTLY {1 sign-0 instance:seal}, mirror kStateSeal's
    EXACTLY {1 sign-0 fold} (lane-drir §2 V-AGG-EFFECT; d3 §5.2 :967-968
    "every INSTANCE_SEAL carries exactly its sign-0 kInstanceSeal"). No vec,
    no counter — the swap is store-internal (lane-rastore "needs NO runtime
    primitive").
  Emitted LAST, trailing all strata (band 10, the STATE_SEAL slot; §2.4).

----------------------------------------------------------------------
§2.3 THE VEC WIRING (which frontier feeds instantiate; what seal consumes)
----------------------------------------------------------------------
IN (band a drains):
  - $net-removal.<id>  of demand_table(%table:8)  -> minus arm drain
  - $net-addition.<id> of demand_table(%table:8)  -> plus arm drain
  Both are DEF'd by demand's own FRONTIER_FILTER ops (demand is a lower
  stratum than the subgraph, exactly as GROUP_UPDATE's input frontiers are
  DEF'd by the input stratum's FRONTIER_FILTERs, lane-drir §1). The plus
  arm ALSO reads input_table edge(%table:11) via an ACCESS (a Present scan
  keyed by %index:27 on From=Start) — a flags:read, not a vec drain.
OUT (band b appends):
  - $delete-queue.<id> of pub_table(%table:4)  <- (T,F) drop arm append
  - $add-queue.<id>    of pub_table(%table:4)  <- (F,T) born arm append
  These are the OUTER pub table's own delta queues; the E3 seed-before-drain
  edge to %table:4's CLAIM_DRAIN is derived from the explicit DEF edges.
INSTANCE_SEAL consumes NOTHING (no vec): the swap reads/writes only the
  store's internal frozen/current handles + sealed_occupied. It is ordered
  by the trailing band key, not by a vec dependence (mirror STATE_SEAL,
  lane-drir §1 "seal ... NO dedicated codegen region — FOLDS onto
  COMMITSWEEP"). See §2.4 for whether INSTANCE_SEAL rides COMMITSWEEP or
  mints its own trailing region.

There are NO per-instance frontier vecs and NO seam vec: the nested tables
are store-internal (POD handles), reached by the instance:* effects, not by
a DRVec. This is the E-50 point — no reserved seam EffKind, no cross-batch
carried VecRole for THIS op (the instance store's cross-batch state is the
frozen table, carried inside the store, not a loop_carried DRVec). The ONLY
cross-batch carry that WOULD need is_epoch_carried_role + V-LOOP is the
demand seed self-pump (the implicit-asynchrony seam, PerfRoadmap §18.5(E)) —
which is a SEPARATE op (D4 seams), NOT this instance op. Flagged as a hole
(§4) because the termination argument for that self-pump is design-only.

----------------------------------------------------------------------
§2.4 BAND PLACEMENT, STRATUM, WHAT THE KAHN LINEARIZER SEES
----------------------------------------------------------------------
STRATUM (op_stratum, DR.cpp:3275-3345, the NO-DEFAULT-0 discipline):
  SUBGRAPH_INSTANTIATE keys on the subgraph view's stratum via a NEW
  `subgraph_instance_stratum` map (the group_update_stratum peer, DR.h:684;
  DeriveDRStrata mold DR.cpp:2148-2154 + the lift block DR.cpp:2279-2291).
  Per d3 A8/F9: op_stratum keys to the EXCISED JOIN's stratum (the plain
  d_p ⋈ p JOIN the recognizer replaced), which Stratify placed STRICTLY
  ABOVE the demand input. MUST return the real stratum, NEVER default-0
  (the V-READY false-negative hole, lane-drir §3/§4; DR.cpp:3331-3333).
  INSTANCE_SEAL returns 0 like kStateSeal (DR.cpp:3341-3342) — it trails via
  key_of special-casing, not op_stratum.

BAND (op_band, DR.cpp:3354-3384):
  SUBGRAPH_INSTANTIATE lands in BAND 0 (with seeds/products/GROUP_UPDATE),
  at its lifted stratum — the exact GROUP_UPDATE placement (DR.cpp:3360-
  3366). Its band-(a) rebuild reads the demand net frontiers (produced at a
  LOWER stratum, so V-READY passes) and band-(b) publish seeds %table:4's
  queues BEFORE %table:4's own acyclic CLAIM_DRAIN (the E3 edge lands it in
  band 0 ahead of the drain, exactly as GROUP_UPDATE's emit precedes the agg
  drain).
  INSTANCE_SEAL trails at BAND 10 via key_of (DR.cpp:3432-3456, the
  STATE_SEAL special-case DR.cpp:3448-3452), one past commit sweeps (band 9)
  — mirror table_36.Seal LAST (d3 §4.3 E5, D1:942).

RELATIVE TO GROUP_UPDATE's bands (a)/(b): IDENTICAL structure — band (a) is
  the input-frontier fold (here: demand-frontier rebuild), band (b) is the
  one-net-pair emit_touched into the owned table's queues. The only shape
  difference is band (a)'s payload: GROUP_UPDATE does kStateFold (scalar
  value fold, NO ACCESS body); SUBGRAPH_INSTANTIATE does instance:rebuild
  with an ACCESS body (the Rederive re-materialize scan over edge). Both
  land in band 0 at their lifted stratum; both trail a seal at band 10.

RELATIVE TO THE EXISTING ACYCLIC CLAIM/FRONTIER/COMMIT TAIL: unchanged.
  %table:4 (the OUTER pub DiffTable) rides the EXISTING acyclic tail
  (claim-drain -> frontier-filter -> commit-sweep) verbatim — the instance
  op only SEEDS its queues (band 0, band (b)); everything downstream of the
  seed is the already-proven differential machinery (d3 §2.4 "the OUTER
  table's already-proven edge"). No new tail region.

WHAT THE KAHN LINEARIZER SEES (LinearizeAndValidateDRFlow, lane-drir §3):
  - a well-defined band key (op_stratum from subgraph_instance_stratum,
    op_band=0, op_table_id=pub_table via the DR.cpp:3387-3395 ?: chain,
    op_sign split for the two band-(a) arms via DR.cpp:3396-3411).
  - RAW edges: band (a) drains DEF'd by demand FRONTIER_FILTERs (E1);
    band (b) instance:emit reads current WRITTEN by band (a) instance:rebuild
    (E2, intra-op ordering — the rebuild-before-emit RAW).
  - the E3 seed-before-drain edge (band (b) appends -> %table:4 CLAIM_DRAIN)
    from the explicit queue DEF edges.
  - INSTANCE_SEAL's band-10 key orders it after everything (E5).
  V-BAND-HAZARD (DR.cpp:3676-3696): every intra-scope dep edge runs FORWARD
    in the band key — satisfied because rebuild(band0,armkey) < emit(band0,
    publishkey) < %table:4 drain (band>0) < seal(band10).
  V-LINEAR / V-READY: topological + no-read-from-higher-stratum — satisfied
    (all reads resolve to demand/edge producers at stratum <= the op's).
  V-LOOP: NOT engaged — this op reads no round-carried/epoch-carried
    frontier (the frozen table is store-internal, not a DRVec).

----------------------------------------------------------------------
§2.5 VALIDATORS THAT APPLY + THE NEW INSTANCE CENSUS (E-27/E-28)
----------------------------------------------------------------------
EXISTING validators and their verdict on the new op-pair (lane-drir §3):
  - V-XOVER-ONE / V-PROD-MONO / V-PROD-CLASS / V-JOIN-ONE: INVISIBLE — the
    instance op mints no crossover/product/join structure. (The EXCISED
    d_p ⋈ p JOIN is gone; V-JOIN-ONE never sees it.)
  - V-BAND-HAZARD / V-LINEAR / V-READY: APPLY, satisfied via §2.4's band key.
  - V-LOOP: APPLIES vacuously (no carried frontier read).
  - V-PRED-XCHECK (Stratum.cpp): band (b)'s reads are instance:old (frozen,
    no gate) + instance:emit (current, no gate) + kInI:read-frozen (frozen).
    Like GROUP_UPDATE (lane-drir §3 "carries NO membership-predicate choice
    on its hot path"), the instance op has NO context-sensitive gate, so NO
    new V-PRED-XCHECK site is needed — UNLESS the rebuild ACCESS's Present
    scan is deemed a gated read; it is a plain monotone Present (add-only,
    A5 fence), not a claim-relative predicate, so no site. (Hole flagged §4
    if an implementer disagrees.)
  - V-INGEST-XCHECK Site 5: N/A if emission is FULLY DR-lowered (a
    LowerSubgraphInstantiate, the LowerGroupUpdate mold, lane-drir §4). IF
    any of the rebuild ACCESS is hand-coded (the eager-web style), it needs
    a Site-5-style coverage multiset check (lane-drir §3). RECOMMENDATION:
    FULLY DR-lower it (mold LowerGroupUpdate) so NO Site-5 analog is needed —
    its totality is guaranteed by the effect-totality validator + census.

THE NEW ALWAYS-ON VALIDATORS (the V-AGG-* mold, one level up; d3 §5.2/A7):
  - V-INSTANCE-EFFECT (the V-AGG-EFFECT peer, DR.cpp:2964 mold):
    [AMENDED: F1/HIGH + F2/HIGH + F3/MED — the totality (i) silently DROPPED
     d3 §5.2's MANDATORY kDemandRead (now added, ± per sign arm), (ii) counted
     a non-EffKind "ACCESS" as an effect member (replaced with the kFlagRead
     it lowers to, the Rederive rescan demoted to a PlanTree on the op body),
     and (iii) framed instance:rebuild(±) as a value-balanced fold_signs==0
     pair (corrected to a STRUCTURAL two-arm pair — see below). Reconciled
     with d3 §5.2 :961-968 verbatim.]
    every kSubgraphInstantiate carries EXACTLY {2 instance:demand (±, the
    kDemandRead key reads — d3 §5.2 :965), 2 demand-frontier drains
    (kVecDrain, existing sub-domain), 1 flags:read over the monotone input
    (kFlagRead, plus arm only — the Rederive leaf; the rescan PlanTree on the
    op body is NOT counted), 2 instance:rebuild (±, STRUCTURAL — see below),
    1 instance:emit, 1 instance:old, 2 NonRecursive counters, 2 kInI
    crossings, 2 queue appends}; every kInstanceSeal EXACTLY {1 sign-0
    instance:seal}. All members are genuine EffKind kinds (DR.h:73-84), as
    V-AGG-EFFECT counts only closed-set members of op.effects and aborts on
    any kind outside that set (DR.cpp:2935-2937) — so no "ACCESS" token.
    THE instance:rebuild(±) PAIR IS STRUCTURAL, NOT A VALUE FOLD (F3): unlike
    V-AGG-EFFECT's `2 state folds, fold_signs==0` (DR.cpp:2951-2952 — both
    signs genuinely fold a VALUE into the working word), the minus rebuild
    arm materializes NOTHING (Recycle-to-empty, d3 A3; §3.3) and the plus arm
    TryAdd-materializes. A `fold_signs==0` value-balance has NO semantic
    content for instances (there is no value added-and-subtracted) and would
    be a fiction if asserted. V-INSTANCE-EFFECT therefore asserts
    `rebuild count==2 with exactly one -1 (Recycle) arm and one +1 (TryAdd)
    arm` — STRUCTURAL presence of both arms — explicitly NOT a value balance.
    The ± rebuilds do NOT cancel like StateCell folds.
  - V-INSTANCE-SOLE (the V-AGG-SOLE peer, DR.cpp:2983 mold; d3 §2.5): the
    pub table's member-view list holds EXACTLY the one kSubgraphInstantiate;
    input_table non-null, monotone (the A5 fence), non-aliasing the pub table.
  - V-INSTANCE-PAIR (the V-AGG-PAIR peer, DR.cpp:3010 mold; d3 §5.2): the
    instance_store_id is a bijection kSubgraphInstantiate <-> kInstanceSeal
    onto [0, |instance_stores|). E-28: instance_store_id is a MINT-ORDER
    artifact, EXCLUDED from census keys (DR.cpp:2830-2832 discipline), bijection
    checked STRUCTURALLY not by key equality.
  - V-INST-FRESH (d3 NF2, a store-side DebugValidate check): at rebuild entry
    per touched iid, assert current->NumRows()==0 (the A2 partition needs a
    clean current). Runtime-side, fprintf+abort, survives NDEBUG.

THE NEW INSTANCE CENSUS (E-27/E-28, the query-side recount; d3 A7 + NF1):
  Recount source: |query.RecognizedSubgraphs()| — the accessor the SURFACE-2
    JOIN-excision recognizer MUST populate on QueryImpl at RECOGNITION time,
    distinct from BuildSubgraphOps's mint loop (d3 A7). expect(kSubgraph
    Instantiate, want, ...) and expect(kInstanceSeal, want, ...) both ==
    |RecognizedSubgraphs()| (the GROUP_UPDATE/STATE_SEAL == exp_group_update
    pattern, DR.cpp:2881-2882).
  Per-op key MULTISET (order-free, stable identity, NEVER mint-order — E-28):
    key = (inst_table_ptr, provenance=kSubgraph, mode=Rederive,
           subgraph view.UniqueId()) — EXACTLY d3 §5.1's SubgraphKey
    (:944-948), mirroring GroupUpdateKey (DR.cpp:2834
    tuple<agg_table_ptr, provenance, algebra, view.UniqueId>) with `mode`
    (Rederive, the lowering selector) in the ALGEBRA slot. NOT a context-col
    set (variable-width, derivable from the view, not a stable scalar key
    component — the earlier draft's error). inst_table_ptr = the OUTER pub
    table %table:4 (the SOLE-WRITER table), not a nested table pointer.
    Sorted-compared off flow.ops.
  SCOPE HONESTY (d3 NF1, LOUD): this census cross-checks the MINT LOOP
    against the recognizer's own list — it catches BuildSubgraphOps skipping/
    duplicating/diverging from what the recognizer identified. It does NOT
    cross-check the RECOGNIZER against anything independent (both the recount
    and the mint read the recognizer's ONE subgraph_views list — the E-27
    tautology relocated one level up for recognizer bugs). Recognizer
    correctness rests on the ORACLE-refereed witness + the `-ir-out`
    structural gate (K10: byte-identical to plain-join IR modulo message
    name), NOT this census. If RecognizedSubgraphs() is not built this stage,
    the census DOWNGRADES to V-INSTANCE-PAIR bijection-only (the honest
    fallback, d3 A7 :1414-1418).

======================================================================
§2.6 THE `-deltarel-out` DUMP (ir-dump-formats.md §2 syntax, verbatim)
======================================================================
The hand-written target dump for the `neighborhood` witness, nested-lowered.
This IS what `demand_neighborhood_witness.deltarelgold` (PICK-A) should
contain once emission lands. Determinism: every line keys off ids (op/vec/
view/table id) or the checked linearization; no pointer values (ir-dump-
formats §2 determinism argument). Op ids are illustrative (mint-order); the
CHECKED-LINEARIZATION order is what the dump walks.

    deltarel demand_neighborhood_witness

    ;; stratum 0 = the demand relation d_p^bf (demand__neighborhood_local)
    ;;             + the monotone edge input. Standard seed/frontier machinery.
    stratum 0 band=0
      vec $net-addition.20 <u64> def=[op.5] use=[op.31]      ; d_p A\D frontier
      vec $net-removal.21  <u64> def=[op.5] use=[op.31]      ; d_p D\A frontier
      op.5  FRONTIER_FILTER sign=· pos=· claim=seed
        reads: NetAdded(%table:8), NetDeleted(%table:8)
        writes/effects: {loop $overdelete-set/$addition-set(%table:8),
                         ->$net-removal.21, ->$net-addition.20}
        spine: %table:8[u64]
        args: table=%table:8 (demand__neighborhood_local)
      ;; edge(%table:11) is monotone — its rows are read by the ACCESS below,
      ;; no frontier op (A5: monotone input, add-only).

    ;; stratum 1 = the subgraph instance (the excised d_p |x| edge JOIN's
    ;;             stratum, strictly above stratum 0; d3 A8/F9).
    stratum 1 band=0
      op.31 SUBGRAPH_INSTANTIATE sign=· pos=· claim=seed
            subgraph=neighborhood context={Start} config={} published={Node}
            mode=Rederive class=NonRecursive instance-store=0
        ;; --- BAND (a) instance_rebuild, per-sign arms ---
        ;; V-INST-FRESH fires at band-(a) ENTRY per touched instance (before
        ;; either arm): assert current->NumRows()==0. (d3 NF2.)
        arm sign=-  reads: (demand key via instance:demand; frozen via instance:old)
          writes/effects: {loop $net-removal.21, instance:demand(I0),
                    instance:old(I0), instance:rebuild(I0,-1)}  ; leave current EMPTY
        arm sign=+  reads: instance:demand(I0); Present(%table:11 edge)
          spine: %table:11[u64,u64] via %index:27[From,_]   ; edge(Start,_)
          body: PlanTree = Rederive rescan edge(Start,_)  ; NOT an effect member
          writes/effects: {loop $net-addition.20, instance:demand(I0),
                    flags:read(edge %table:11, Present, seed),
                    instance:rebuild(I0,+1)}         ; TryAdd Node -> current
        ;; --- BAND (b) publish_touched, the one-net-pair partition (d3 A2) ---
        publish over I0.Touched():
          reads: instance:old(I0)=frozen->Find, instance:emit(I0)=current->Find,
                 kInI-read-frozen(%table:4)
          (F,T) born:    counter+(%table:4, NonRecursive) over (Start++Node),
                         ->$add-queue.14(%table:4)
          (T,F) dropped: counter-(%table:4, NonRecursive) over (Start++Node),
                         ->$delete-queue.13(%table:4)
          (T,T)/(F,F):   nothing
        args: pub_table=%table:4 (neighborhood), demand=%table:8,
              input=%table:11, store=I0
      vec $delete-queue.13 <u64,u64> def=[op.31] use=[op.40]   ; pub_table delQ
      vec $add-queue.14    <u64,u64> def=[op.31] use=[op.41]   ; pub_table addQ

      ;; %table:4 (the OUTER pub DiffTable) rides the EXISTING acyclic tail:
      op.40 CLAIM_DRAIN sign=- form=single-pass claim=seed
        reads: TryClaimDel C_nr<=0 (%table:4); writes kDel|kDelNow
        writes/effects: {loop $delete-queue.13, ->$overdelete-set(%table:4)}
        args: table=%table:4
      op.41 CLAIM_DRAIN sign=+ form=single-pass claim=seed
        reads: TryClaimAdd Total>0 (%table:4); writes kAdd|kAddNow
        writes/effects: {loop $add-queue.14, ->$addition-set(%table:4)}
        args: table=%table:4
      ;; (FRONTIER_FILTER + COMMIT_SWEEP for %table:4 follow at bands 1..9,
      ;;  standard acyclic tail, elided — unchanged from the flat lowering.)

    ;; band 10 = the trailing seal (the STATE_SEAL slot).
    stratum 1 band=10
      op.55 INSTANCE_SEAL sign=· pos=· claim=seed instance-store=0
        writes/effects: {instance:seal(I0)}  ; the pointer swap; global:rmw hazard class
        args: pub_table=%table:4, store=I0

    census: subgraph_instantiate=1 instance_seal=1 (== |RecognizedSubgraphs()|)

KEY DUMP OBSERVATIONS:
  - The dump makes the NEW effect family (all FIVE kinds) VISIBLE
    (instance:demand/rebuild/emit/old/seal) — the review surface the owner
    directed (§0.5(3) end-to-end IR review reads the DeltaRel dump).
  - The EXCISED JOIN is ABSENT: no op.<n> JOIN, no %table for a guarded
    copy — SUBGRAPH_INSTANTIATE stands where `^flow:43` (d3 §4.2) was. A
    reviewer diffing against the flat dump SEES the excision (d3 NF1: the
    -ir-out structural gate is where recognizer drift shows up).
  - %table:4/:8/:11 keep d3's numbering; %index:27 is edge's From index.

======================================================================
§3. THE 3-BATCH TRACE — FROZEN-PAIR STORE CALLS + FLAT EQUIVALENCE
======================================================================
The witness with a concrete demand + edge stream. The demand seed for
`neighborhood(bound Start, free Node)` arrives via the FABRICATED demand
message `demand__neighborhood_bf` (Demand.cpp fabrication; the injector
forces it from the bound query call — lane-pipeline §10 / the force machinery).
For the trace, "demand-add Start=5" = the fabricated demand seed for key {5}
crosses +; "demand-retract Start=5" = it crosses -.

  EDGES (add-only, monotone; injected once, batch 1): 5->7, 5->9, and later
    5->8 (batch 2). No edge ever retracts (A5 fence).
  INSTANCE store I0 keyed by context {Start}. PubRow = {Node}.
  Store state written as [ current={...} | frozen={...} | wc=working_count
    | so=sealed_occupied ].

Store call vocabulary (the d3 A-section / lane-rastore PART 2 names):
  FindOrAddInstance(key)   -> iid, allocating current+frozen via MakeTable
  current->TryAdd(Node)    -> monotone-dedup materialize (band a plus arm)
  Touch(iid) / Touched()   -> per-epoch touched-set bookkeeping
  frozen->Find(r) / current->Find(r)  -> the (old_r,new_r) partition reads
  pub.AddDerivation / pub.SubDerivation  -> the outer %table:4 counter±
  Recycle(tmp)             -> teardown+reconstruct an empty table (d3 N1)
  swap (frozen<->current) + sealed_occupied := frozen->NumRows()>0  (Seal)

----------------------------------------------------------------------
§3.1 BATCH 1 — demand-add {5}; edges 5->7, 5->9 present. INSTANCE BIRTH.
----------------------------------------------------------------------
  entry state: I0 has no instance for {5}.
  BAND (a) plus arm (demand__neighborhood_bf net-addition drains {5}):
    iid = FindOrAddInstance({5})            -> iid 0; MakeTable(current),
                                               MakeTable(frozen); both EMPTY.
      [ current={} | frozen={} | wc=0 | so=0 ]
    V-INST-FRESH: assert current->NumRows()==0  (holds — fresh)
    ACCESS edge(5,_) via %index:27 -> yields Node 7, Node 9.
    current->TryAdd(7); current->TryAdd(9)     ; instance:rebuild(I0,+1)
    Touch(0); working_count[0] = 2
      [ current={7,9} | frozen={} | wc=2 | so=0 ]
  BAND (b) publish_touched, Touched()={0}, candidates r in {7,9}:
    r=7: old_r=frozen->Find(7)=absent (F); new_r=current->Find(7)=(T) => (F,T) born
         pub.AddDerivation((5,7), NonRecursive); ->add-queue    ; kInI-read-frozen
    r=9: (F,T) born => pub.AddDerivation((5,9), NonRecursive); ->add-queue
  OUTER %table:4 acyclic tail: CLAIM_DRAIN+ claims (5,7),(5,9); COMMIT_SWEEP
    publishes was!=now: +(5,7), +(5,9). kInI:=Present for both.
  INSTANCE_SEAL (band 10), touched={0}:
    swap: tmp=frozen({}); frozen=current({7,9}); current=Recycle(tmp)={}
    sealed_occupied[0] = (frozen->NumRows()=2 >0) = 1; touched cleared.
      [ current={} | frozen={7,9} | wc=2 | so=1 ]
  PUBLISHED THIS BATCH (nested): +(5,7), +(5,9).

  FLAT LOWERING on the SAME batch (demand_tc_witness-style guarded copy):
    the flat lowering (D4, landed) folds the guarded rows of `neighborhood`
    straight into %table:4 as differential rows via the d_p ⋈ edge JOIN:
    the demand seed {5} joins edge(5,_) -> (5,7),(5,9) -> two +nonrecursive
    UPDATECOUNTs into %table:4 -> COMMIT_SWEEP publishes +(5,7),+(5,9).
  NET %table:4 AFTER BATCH 1:  {(5,7),(5,9)} present in BOTH lowerings. ==

----------------------------------------------------------------------
§3.2 BATCH 2 — add edge 5->8 under STANDING demand {5}. INSTANCE REBUILD.
----------------------------------------------------------------------
  entry state: [ current={} | frozen={7,9} | wc=2 | so=1 ]; demand {5} STILL
    held (no demand delta this batch — but the INPUT edge changed).
  ** THE A5 FENCE SUBTLETY, LOUD **: under stage-(b) R-A, band (a) drains
    ONLY the DEMAND frontier, NOT the edge frontier (d3 A5 / K9). A NEW edge
    5->8 while demand persists triggers NO band-(a) rebuild arm — because
    edge is monotone and there is no input-frontier arm. So the NESTED
    lowering does NOT see 5->8 this batch: current stays {} (no rebuild
    fired), Touched() is EMPTY, publish emits nothing, seal is a no-op.
    NESTED PUBLISHED: nothing. Instance stays [current={}|frozen={7,9}].
  FLAT LOWERING on the SAME batch: the guarded copy's d_p ⋈ edge JOIN DOES
    fire on the new edge 5->8 (the JOIN re-runs on ^receive:add_edge for the
    still-demanded key {5}) -> +(5,8) into %table:4 -> published +(5,8).
    FLAT PUBLISHED: +(5,8).

  *** DIVERGENCE — the two lowerings are NOT answer-identical on batch 2 ***
  This is EXACTLY the d3 A5 stale-set divergence made concrete: "an edge
  change while demand persists triggers NO rebuild -> a STALE set, diverging
  from the eager reference." Under the stage-(b) fence this batch shape is
  supposed to be UNREACHABLE: the A5 fence asserts the summarized input is
  monotone AND the DEMAND is what drives rebuild, so a mid-stream edge add
  under standing demand is the case the fence's whole-instance-rebuild model
  does NOT maintain incrementally. Two honest options for the witness:

    OPTION-1 (KEEP BATCH 2 as a NEGATIVE/FENCE witness): batch 2 as written
      DEMONSTRATES the divergence — and therefore the witness's batch stream
      must NOT include a mid-stream edge add under standing demand if the
      goal is answer-identity, OR the batch is deliberately included as the
      COST/semantics note that stage (b) is demand-triggered-rebuild-only.
      The oracle (which recomputes the FULL closure) would show (5,8)
      reachable, so the NESTED lowering FAILS the oracle on batch 2 -> this
      batch is OUTSIDE the shipped semantics.

    OPTION-2 (RE-DEMAND to force rebuild — the TRUE stage-(b) REBUILD shape):
      make batch 2 a DEMAND-CHANGE, not an input-change: retract-and-re-add
      demand {5} in the same batch (a same-epoch flap, d3 NF3) AFTER edge
      5->8 was added. THEN band (a) fires: minus arm empties current (already
      empty), plus arm re-materializes edge(5,_) = {7,9,8} into a fresh
      current. This is the REBUILD the R-A store is designed for.

  THIS ARTIFACT ADOPTS OPTION-2 for the rebuild trace (it is the shape R-A
  lowers; Option-1's divergence is recorded as the A5 fence's raison d'etre
  and a §4 hole). LOUD (F5): Option-2's batch is CONSTRUCTED so the demand
  flap DRIVES the nested rebuild while being a NO-OP for flat — flat publishes
  +(5,8) from the edge join alone (Option-1, above), the demand retract+re-add
  it also carries nets zero for flat (a phantom pair) and does no work there.
  The two lowerings therefore agree on the ANSWER (+(5,8)) on THIS batch, but
  they do NOT process it symmetrically: the batch is shaped to the nested
  lowering's trigger, not a neutral witness batch. The "==" below is
  answer-agreement on this constructed batch, NOT evidence of general
  equivalence over arbitrary edge/demand streams (that stays UNVERIFIED,
  consolidated §5; see §3.4's caveat and §3.5). Batch 2 = "edge 5->8 added,
  THEN demand {5} re-asserted"
  (the injector re-forces the bound query for {5}, crossing demand -,+ in one
  batch — a same-epoch flap):
    BAND (a) minus arm (demand {5} net-removal): instance:old reads
      frozen={7,9}; instance:rebuild(I0,-1) leaves current EMPTY (it already
      is; if a prior plus arm had dirtied it this batch, Recycle empties it —
      d3 NF3). working_count untouched by the read.
    BAND (a) plus arm (demand {5} net-addition): V-INST-FRESH asserts
      current->NumRows()==0 (holds). ACCESS edge(5,_) now yields {7,8,9}.
      current->TryAdd(7); TryAdd(8); TryAdd(9). Touch(0). working_count[0]=3.
      [ current={7,8,9} | frozen={7,9} | wc=3 | so=1 ]
    BAND (b) publish_touched, candidates r in frozen UNION current = {7,8,9}:
      r=7: old_r=(T) [in frozen], new_r=(T) [in current] => (T,T) UNCHANGED, nothing
      r=9: (T,T) UNCHANGED, nothing
      r=8: old_r=(F), new_r=(T) => (F,T) born; pub.AddDerivation((5,8)); ->addQ
    OUTER %table:4: publishes +(5,8) only (7,9 already present, OQ3 no churn).
    INSTANCE_SEAL: swap frozen<->current; frozen={7,8,9}; current=Recycle={}.
      sealed_occupied[0]=(3>0)=1.
      [ current={} | frozen={7,8,9} | wc=3 | so=1 ]
    NESTED PUBLISHED (option-2): +(5,8).
  FLAT LOWERING (option-2 batch): the guarded copy joins the re-asserted
    demand {5} against edge = {7,8,9}; but (5,7),(5,9) are ALREADY present
    (SET semantics, OQ3 annihilation) so only +(5,8) nets. FLAT PUBLISHED:
    +(5,8).
  NET %table:4 AFTER BATCH 2 (option-2): {(5,7),(5,8),(5,9)} in BOTH. ==
  The (T,T) unchanged suppression (d3 A2 :1276) is why the nested lowering
  emits ONLY +(5,8), matching the flat lowering's OQ3 annihilation exactly —
  the two-lowerings equivalence is the (T,T)=OQ3 correspondence.

----------------------------------------------------------------------
§3.3 BATCH 3 — demand-retract {5}. INSTANCE DEATH (the -frozen arm).
----------------------------------------------------------------------
  entry state: [ current={} | frozen={7,8,9} | wc=3 | so=1 ].
  BAND (a) minus arm (demand {5} net-removal drains {5}):
    instance:old reads frozen={7,8,9} (the set to retract).
    instance:rebuild(I0,-1): current STAYS EMPTY (no fold — d3 A3 minus arm).
    Touch(0). working_count[0] -> 0 (all live rows gone).
      [ current={} | frozen={7,8,9} | wc=0 | so=1 ]
    NO plus arm this batch (demand only retracted).
  BAND (b) publish_touched, Touched()={0}, candidates r in {7,8,9}:
    r=7: old_r=(T) [frozen], new_r=(F) [current empty] => (T,F) dropped;
         pub.SubDerivation((5,7), NonRecursive); ->delete-queue
    r=8: (T,F) dropped => pub.SubDerivation((5,8)); ->delQ
    r=9: (T,F) dropped => pub.SubDerivation((5,9)); ->delQ
  OUTER %table:4 acyclic tail: CLAIM_DRAIN- claims (5,7),(5,8),(5,9);
    COMMIT_SWEEP publishes -(5,7),-(5,8),-(5,9). kInI:=absent.
  INSTANCE_SEAL: swap; tmp=frozen({7,8,9}); frozen=current({}); current=
    Recycle(tmp)={}. sealed_occupied[0]=(frozen->NumRows()=0)=0. Instance id
    0 RETAINED (append-only namespace; dead-instance sweep is D5 residue,
    lane-rastore).
      [ current={} | frozen={} | wc=0 | so=0 ]
    NESTED PUBLISHED: -(5,7), -(5,8), -(5,9).
  FLAT LOWERING on the SAME batch: the demand seed {5} retracts; the guarded
    copy's rows lose their demand support -> the d_p ⋈ edge JOIN's del arm
    fires -> -(5,7),-(5,8),-(5,9) into %table:4 -> published -(5,7),-(5,8),
    -(5,9). FLAT PUBLISHED: -(5,7),-(5,8),-(5,9).
  NET %table:4 AFTER BATCH 3: EMPTY in BOTH lowerings. ==

----------------------------------------------------------------------
§3.4 THE TWO-LOWERINGS EQUIVALENCE, STATED
----------------------------------------------------------------------
Across the three batches (option-2), the OUTER %table:4 nets IDENTICALLY:
    batch 1: +(5,7),+(5,9)          | flat: +(5,7),+(5,9)          ==
    batch 2: +(5,8)                 | flat: +(5,8)                 ==
    batch 3: -(5,7),-(5,8),-(5,9)   | flat: -(5,7),-(5,8),-(5,9)   ==
The nested lowering's EMITTED membership predicates on %table:4 are the
EXACT flat vocab: counter± (NonRecursive), kInI-read-frozen (the crossing),
vector:append(del/add queue), then the standard CLAIM_DRAIN (TryClaimDel
C_nr<=0 / TryClaimAdd Total>0, F17) + COMMIT_SWEEP (kInI:=Present, publish
was!=now). The ONLY nested-specific machinery is band (a)'s instance:rebuild
+ the instance:old/emit membership reads (frozen->Find / current->Find) that
COMPUTE the net pairs band (b) folds — and those reads touch NO counter and
publish NOTHING directly (SW-2). So the two lowerings publish the SAME
%table:4 deltas by construction: both compute the symmetric difference of
(demanded old set, demanded new set) and fold exactly that into %table:4.
  The oracle referees this: it recomputes neighborhood(5,_) = the full
  edge(5,_) closure per batch and compares the driver's demand-ON answers
  for key {5} — identical under flat and nested because both publish the
  same %table:4 (consolidated.md §3, the oracle is lowering-agnostic).
  DIVERGENCE CAVEAT (§3.2 Option-1): the equivalence holds ONLY for
  DEMAND-triggered changes. A mid-stream monotone-input change under standing
  demand (batch-2 Option-1) diverges — that is OUTSIDE stage (b) and is the
  A5 fence's job to REJECT at compile, not a runtime equivalence to claim.

  [AMENDED: F4/MED + F5/MED — the "by construction" framing over-claimed. The
   3 batches (BIRTH / rebuild-via-flap / DEATH) are the batches the store was
   DESIGNED for and cannot FALSIFY the equivalence; batch 2 is a CONSTRUCTED
   batch (§3.2, shaped so the demand flap drives the nested rebuild while being
   a no-op for flat). Two load-bearing qualifications are now stated below, and
   the adversarial batches are walked in §3.5.]

  SCOPE OF THE "==" (F5): the three-batch "==" above demonstrates
  ANSWER-AGREEMENT on THESE THREE batches (one of which, batch 2, was
  constructed to the nested trigger). It is NOT a proof of GENERAL equivalence
  over arbitrary edge/demand streams — that remains UNVERIFIED (consolidated
  §5). The witness demonstrates three points; it does not discharge the gate.

  ORDERING PRECONDITION (F4, LOAD-BEARING): the equivalence holds PROVIDED the
  minus rebuild arm is ordered BEFORE the plus rebuild arm within band (a).
  §3.5(b) (the phantom demand pair) shows the equivalence is CONDITIONAL on
  this: if the plus arm ran after the minus in a same-batch add-then-remove of
  {5}, current would end non-empty and nested would WRONGLY publish rows flat
  suppresses. H9/H10 flag this intra-band ordering as UNPINNED — it MUST be
  pinned by an explicit DR-IR edge before this equivalence is a GATE, not
  merely a claim. (This is a precondition of §3.4, cross-ref H9/H10, not a
  buried hole.)

----------------------------------------------------------------------
§3.5 ADVERSARIAL EQUIVALENCE BATCHES (the cases the 3-batch trace CANNOT
     falsify — walked by hand; F4)
----------------------------------------------------------------------
[AMENDED: F4/MED — added. The §3.1-§3.3 trace exercises only the store's
 designed BIRTH/rebuild/DEATH batches; the epoch gate (PerfRoadmap §18.5(C))
 demands the ADVERSARIAL batches. Walked here so a reviewer can check them.]

  (a) DUP DEMAND KEY, same batch: demand-add {5} TWICE.
      NESTED: FindOrAddInstance({5}) returns ONE iid; the second add is an
        idempotent Touch (no second instance). The plus arm's TryAdd(Node)
        is monotone-dedup (Table.h:236), so current={7,9} regardless of the
        dup. Publish: (F,T) born for 7,9 ONCE each.
      FLAT: demand__neighborhood is a DiffTable; the two {5} adds NET to one
        present row (SET semantics, OQ3). The d_p ⋈ edge JOIN fires once.
      BOTH publish +(5,7),+(5,9) once. EQUIVALENT — the nested store dedups
        via FindOrAdd-returns-same-iid + monotone TryAdd; the flat side dedups
        via the DiffTable's SET-net. (Stated so a reader can check it.)

  (b) PHANTOM DEMAND PAIR, same batch: demand-add {5} THEN demand-remove {5},
      net absent (the same-epoch flap mechanic, d3 NF3 / A11).
      NESTED (minus BEFORE plus, per the §3.4 ordering precondition):
        plus arm materializes current={7,9}; minus arm Recycles current->{}.
        Band (b): old_r=frozen->Find(r)={} (never sealed — birth epoch),
        new_r=current->Find(r)={} => (F,F) for all r => publish NOTHING.
        Seal: frozen stays {}.
      FLAT: demand {5}+ and {5}- annihilate (SET-net zero) => no JOIN fire =>
        nothing published.
      BOTH publish nothing. EQUIVALENT — *** BUT ONLY IF the minus rebuild arm
        is ordered before the plus arm within band (a) ***. If the plus arm
        ran AFTER the minus in this phantom case, current would end {7,9} and
        nested would WRONGLY publish +(5,7),+(5,9) while flat publishes nothing
        — a DIVERGENCE. THIS is the load-bearing ordering precondition promoted
        into §3.4; H9/H10 flag it UNPINNED and it MUST be pinned by an explicit
        DR-IR minus-before-plus edge before the equivalence is a gate.

  (c) OVERLAPPING INSTANCES sharing a Node: demand {5},{6}; edges (5,7),(6,7).
      NESTED: instance{5}'s current={7}, instance{6}'s current={7} — two
        DISTINCT iids, two distinct nested tables. Publish emits the OUTER row
        as (Start ++ Node): counter+(%table:4) over (5++7) and (6++7) — two
        distinct pub rows, NO aliasing (the nested table holds only {Node} but
        Start is RE-ATTACHED at publish, §2.1 band (b)).
      FLAT: the JOIN yields (5,7),(6,7).
      BOTH publish +(5,7),+(6,7). EQUIVALENT — confirmed the pub row carries
        Start so the shared Node 7 does not alias across instances.

  SUMMARY: (a) and (c) are UNCONDITIONALLY equivalent; (b) is equivalent ONLY
  under the minus-before-plus band-(a) ordering (§3.4 precondition, H9/H10).
  These three plus the §3.1-§3.3 designed batches still do NOT prove general
  equivalence over arbitrary streams (F5) — that stays UNVERIFIED.

======================================================================
§4. OPEN HOLES — LOUD, one line each (could not close from the inputs)
======================================================================

H1  WITNESS SHAPE UNDECIDED (owner call, §1): demand_tc_witness is RECURSIVE
    so cannot be nested-lowered under stage (b); this artifact lowers the
    non-recursive `neighborhood` sibling (PICK-A). The owner must pick A
    (add the sibling) or B (force the A6 incremental arm). All of §2/§3
    assume PICK-A's neighborhood shape.

H2  THE TWO-LOWERINGS EQUIVALENCE DIVERGES on a mid-stream monotone-input
    change under standing demand (§3.2 batch-2 Option-1): nested does NOT
    rebuild (demand-triggered only), flat DOES. Whether this batch shape is
    fenced at COMPILE (A5 monotone-input fence rejects the PROGRAM) or must
    be excluded from the WITNESS BATCH STREAM (a runtime-scope note) is
    unresolved — A5 fences deletable inputs, not add-under-standing-demand.

H3  THE JOIN-EXCISION RECOGNIZER IS UNSPECIFIED (d3 A8/F6 second half, E-53):
    this artifact hand-mints SUBGRAPH_INSTANTIATE in place of the d_p ⋈ edge
    JOIN, but WHICH pattern-match excises the plain JOIN + %table:4 and
    reaches the store from source is D1/D2 work — the store is de-risked as
    SUBSTRATE only until it exists.

H4  RecognizedSubgraphs() ACCESSOR NOT DESIGNED (d3 A7/NF1): the census
    recount source is an accessor the recognizer MUST populate on QueryImpl;
    its shape (what a QuerySubgraph-analog node holds) is unspecified. Until
    built, the census downgrades to V-INSTANCE-PAIR bijection-only.

H5  THE DEMAND-SEED SELF-PUMP / IMPLICIT-ASYNCHRONY SEAM (PerfRoadmap
    §18.5(E), consolidated.md §5) is a SEPARATE op from this instance op and
    is NOT modeled here; its finite-demand-lattice termination argument is
    design-only. §2.3 flags that the only is_epoch_carried_role/V-LOOP carry
    would live on THAT op (E-50 VecRole+loop_carried), not the instance op.

H6  Table::Reset() ARENA-SOUNDNESS (lane-rastore N-1, NOT in d3): the R-A
    Recycle default (teardown+reconstruct) LEAKS per rebuild under an Arena
    (Arena::Free is a no-op). The witness uses MallocAllocator so it PASSES,
    hiding the leak. Table::Reset() is a SOUNDNESS requirement under Arena,
    not the measure-first perf residue d3 A4 files it as.

H7  "frozen UNION current" HAS NO RUNTIME PRIMITIVE (lane-rastore N-2): band
    (b)'s candidate enumeration lowers to two full RowAt scans (frozen for
    -old, current for +new) — the natural partition, but the codegen shape
    (dedup vs two separate scans) is unstated; I assumed the two-separate-
    scans partition (iterate current for +new candidates, frozen for -old).

H8  V-PRED-XCHECK SITE FOR THE REBUILD ACCESS (§2.5): I assumed the plus
    arm's edge Present scan is a plain monotone read needing NO new
    V-PRED-XCHECK site. If an implementer treats the Rederive ACCESS as a
    context-sensitive gated read, a new Pred site is required — unresolved
    which, because GROUP_UPDATE's precedent (no hot-path pred) may not
    transfer to an ACCESS-bearing band (a).

H9  THE NEW EffKind INTEGRATION into the linearizer switch (DR.cpp:3206-3246)
    is SPECIFIED as four cases (§2, instance:rebuild=write / emit=read /
    old=frozen-no-hazard / seal=global:rmw) but the EXACT hazard classes
    (does instance:rebuild WAW against a same-band sibling arm? the ±arms
    both write current — the minus arm's Recycle vs the plus arm's TryAdd)
    are not fully worked; §3.2 Option-2's same-epoch flap orders minus-
    before-plus by the demand pivot's own delta order, but the DR-IR edge
    that PINS minus-before-plus within band (a) is unstated.

H10 op_sign FOR THE TWO BAND-(a) ARMS (§2.4): I assumed op_sign splits the
    ± rebuild arms for the band key (DR.cpp:3396-3411 mold), but GROUP_UPDATE
    band (a) also has ±arms and I did not verify whether it USES op_sign in
    the key or relies on op_band alone — the tie-break that orders the minus
    rebuild before the plus rebuild within band 0 is inherited-assumed, not
    read from code.

H11 BINDING-SOURCE RESOLUTION FOR INSTANCE-OWNED COLUMNS (owner directive,
    2026-07-19 — ledger §12): every consumer inside a nested instance that
    reads an α-column (functor bound args FIRST AMONG THEM, but also join
    keys, negate gate keys, insert projections) must resolve it to a MODELED
    binding source — `row-slot | instance-key-slot | config-slot` — carried
    as a DR-IR op attribute (access-plan-spine extension), never decided
    inside codegen. The functor ABI stays closed (plain values, the
    config-column mold — free functions gain leading key args exactly as
    config_agg_1/2 added config params; no Database/instance handle ever
    flows in). The ELISION decision and the WIRING decision are ONE
    decision: α-columns resolve to the instance key, never to row storage,
    for ALL consumers — and a validator aborts on any row-slot resolution
    of an α-column (the wrong answer silently duplicates α per row and
    forfeits the nested lowering's storage win).

======================================================================
§5. PROVENANCE
======================================================================
Every effect-set line traces to: v3-spec §2.1 (the tabular style + the
counter±/kInI-frozen/vector vocab), v3-spec-statecell §0-§2 (the two-word
sealed=frozen/working=current dichotomy, C-0c fold≠counter), d3 A0-A11 (R-A
frozen-pair, the swap, the A2 partition, A5 fence, A7 census, A8 recognizer
gate), lane-drir §1-§4 (BuildGroupUpdateOps mold, the census, band keys,
op_stratum NO-DEFAULT-0, the EffKind switch), lane-rastore PART 2 (the store
call sequence + the three new runtime items), lane-pipeline §3/E-46 (the
demand edge + the three guard-site kinds), ir-dump-formats §2 (the dump
syntax). Store state and the 3-batch BIRTH/REBUILD/DEATH walk mirror d3 §4.4
+ A2 + NF3, re-grounded on the neighborhood witness with edges 5->7/5->9/5->8.
