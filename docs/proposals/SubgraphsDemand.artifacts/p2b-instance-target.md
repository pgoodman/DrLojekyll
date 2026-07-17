======================================================================
p2b-instance-target.md — LANE P2(b): KEYED NESTED INSTANCES
the R3-mold operator-family increment (the epoch's thesis)
======================================================================
Designer-lane artifact, checkpoint method, branch subgraphs-demand
(P0 census landed; ledger SubgraphsDemand.md §0-§2, errata E-27..E-34).
Written 2026-07-16. A future implementer builds FROM this; a judge will
try to falsify it. Every code claim carries a file:line anchor READ this
session.

SCOPE OF THIS LANE: stage (b) of PerfRoadmap §14.3-P2 — the
SUBGRAPH_INSTANTIATE operator family in the R3 GROUP_UPDATE mold, its
effect sub-domain, its InstanceStore (the StateCellStore's non-degenerate
parent), the demand-frontier-is-hazard-or-constant question (C-0b one
level up), the surface question (framed for owner input), one hand-written
subgraph case, the seven-point checklist mapped, and staged predictions.
This lane does NOT decide the surface (§4 frames it) and does NOT design
the demand transform (candidate (a), P3 — deferred). It assumes P0 landed
(the GROUP_UPDATE census, the template) and treats the demand frontier as
this family's INPUT (errata E-32/E-33 constrain what a demand edge is at
the dataflow level).

THE THESIS RESTATED (PerfRoadmap §14.0, AggregatingFunctors §4): an
aggregate keyed on (group ++ config) IS a keyed nested instance in
miniature. R3's StateCellStore holds ONE folded scalar per (group ++
config) key; the instance family holds a whole nested sub-relation per
(context ++ config) key. The keying, the two-word sealed/working
dichotomy, the non-aliasing invariant, the occupancy/birth-death guard,
and the dead-group compaction residue ALL transfer (StateCell.h:10-54).
Subgraphs EXTEND the R3 family the way R3 EXTENDED the §5.1 counter
family: a new effect sub-domain filled in an already-total matrix, a new
op plugged into the seven R3 GROUP_UPDATE integration points, a new peer
runtime store generalizing StateCellStore.


VERIFIED-THIS-SESSION ANCHOR TABLE (the load-bearing sites this artifact
reasons from; all read on branch subgraphs-demand HEAD):
  - BuildGroupUpdateOps ................ DR.cpp:638  (mint site; the analog
      BuildSubgraphOps shadows this — op mint :676, seal mint :748,
      effects :691-745, the acyclic-fence assert :718 klass==kNonRecursive,
      the V-AGG-SOLE non-aliasing asserts :663-665)
  - GROUP_UPDATE stratum seed ......... DR.cpp:2118-2124  (agg view's own
      Stratify stratum; "strictly above its input" comment :2119-2121)
  - GROUP_UPDATE stratum LIFT ......... DR.cpp:2249-2261  (the monotone
      fixpoint: stratum = max(view stratum, ready_after(input_table));
      own drain lifts to update stratum — E4)
  - op_stratum GROUP_UPDATE case ...... DR.cpp:3298-3309  (keys on
      group_update_stratum; NO silent default-0 — the critique's
      false-negative fix)
  - op_band GROUP_UPDATE case ......... DR.cpp:3329-3335  (band 0, with
      seeds/products, seed-before-drain into the agg queues E3)
  - STATE_SEAL band key ............... DR.cpp:3417-3422  (lead 2, band 10,
      trails commit sweeps — V-COMMIT-TRAILS analog E5)
  - V-BAND-HAZARD reservation ......... DR.cpp:3482-3488, 3541-3544  (a
      non-loop-carried edge running against the band key; is_round_carried /
      is_epoch_carried role classification 3486-3487)
  - V-READY off-lattice skip .......... DR.cpp:3906-3921  (StateSeal /
      CommitSweep / IngestFold / eager NegateGate exempt from reads-lower)
  - census recount (E-27) ............. DR.cpp:2790-2851  (exp_group_update =
      |Aggregates| + |KVIndices|, add_gu_key :2805, expect :2850-2851)
  - EffKind statecell tail ............ DR.h:73-83  (kStateFold/Emit/Old
      LIVE, kInIReadFrozen :80, kVecDrain :75)
  - DROpKind tail ..................... DR.h:113-137  (kGroupUpdate :129,
      kStateSeal :135)
  - DROp GROUP_UPDATE field block ..... DR.h:523-538  (agg_view, provenance,
      algebra, agg_table, input_view, statecell_id, group_cols, summary_cols)
  - StateCellStore ................... StateCell.h:214-531  (two-word cell,
      occupancy :329-334, Fold :306-323, Seal :353-363, Touched :377-380,
      @recompute membership pool :460-515)
  - StateCell.h header contract ...... StateCell.h:10-54  (non-aliasing,
      two-word cell, occupancy birth/death/change guard)
  - E-32 InsertSetsOverlap empty-set .. View.cpp:1478-1480  (empty group_ids
      => MERGEABLE; demand copies need a structural input edge, not group_ids)
  - E-32 group_ids seed views ......... Optimize.cpp:423  (group_ids seed
      only on Join/Aggregate/KVIndex)
  - E-33 ViewSelfReachable ............ Build.cpp:200, :1152  (the on-cycle
      product fence — a demand join must not form an unstratified cycle)


======================================================================
§1. FIRST DESIGN TASK — IS A DEMAND FRONTIER A HAZARDED WRITE OR A
    FROZEN READ?  (C-0b one level up; RECOMMENDATION: STAGED SPLIT)
======================================================================
This is the sharpest question PerfRoadmap §14.2(B) flags and does not
resolve. Worked concretely below, tested against the two demand regimes,
priced against the exact scheduler code sites, then recommended.

----------------------------------------------------------------------
§1.1 THE QUESTION, STATED IN R3 TERMS
----------------------------------------------------------------------
A DEMAND is a set of ground bindings (context ++ config keys) that some
subgoal has requested be materialized. In this family:
  - a demand is WRITTEN by the demanding subgoal (the rule body that
    references the subgraph at a bound pattern), and
  - a demand is READ by the instantiation (SUBGRAPH_INSTANTIATE reads the
    demand frontier and materializes one instance per demanded key).

The R3 precedent (C-0b, v3-spec-statecell §0:64-82): a StateCell working
word is a real read/write hazard (fold WRITES it, emit READS it, same
band => RAW); a sealed word is a frozen kInI-like constant (written only
by Seal, read by Old, never a within-band hazard). The two-word split is
what keeps the "kInI is a constant, real state is a hazard" dichotomy
intact (StateCell.h:26-36).

The demand frontier is the C-0b question raised one level: is the
DEMAND-KEY SET a hazarded working value (new demands can arrive in the
SAME band that the instantiation reads them — a RAW/WAR cycle) or a
frozen sealed read (all demands for this band are final before the
instance band runs)?

----------------------------------------------------------------------
§1.2 REGIME (i): ACYCLIC DEMAND — the frozen read works
----------------------------------------------------------------------
The demanding subgoal sits in a STRICTLY LOWER stratum than the
instantiation. All demand keys are produced by lower-stratum rules whose
frontier filters are FINAL before the instance band runs. This is
structurally identical to how GROUP_UPDATE reads its input: the aggregate
is placed strictly above its summarized input (DR.cpp:2119-2121 "Stratify
placed the aggregate STRICTLY ABOVE its input"), and the input's net
frontiers are drained as a settled vector (DR.cpp:691-703, the − and +
arms drain NetRemoval/NetAddition frontiers).

For acyclic demand the demand frontier is a FROZEN READ — the kInI/old()
side of the dichotomy. It is produced-then-consumed across a stratum
boundary; within the instance band it never changes; the scheduler treats
it as a settled input vector exactly like GROUP_UPDATE.in drains
input.NetAdditions. NO two-word cell is needed for the DEMAND ITSELF: the
demand is just a differential table (or a net-additions frontier of one)
whose net-added rows the SUBGRAPH_INSTANTIATE drains. The INSTANCE'S OWN
STATE (the materialized sub-relation) is the hazarded working value — but
the demand that keys it is frozen.

This is the DR-IR's already-proven shape. E1 (v3-spec-statecell §2.4:447)
is the exact edge: FRONTIER_FILTER writes input.NetRemovals/NetAdditions,
GROUP_UPDATE.in DRAINS them => RAW (filter-before-drain). A demand analog
is E1 verbatim with "input frontier" replaced by "demand frontier."

----------------------------------------------------------------------
§1.3 REGIME (ii): DEMAND INSIDE AN SCC — needs round-structured hazard
----------------------------------------------------------------------
A recursive rule demands instances whose OUTPUTS feed the demand. Example:
transitive reachability computed via demanded sub-queries where reaching
a new node demands a sub-instance whose published rows demand further
nodes. Now demand and instance publication are in the SAME SCC: within a
single batch, new demands ARRIVE AFTER instances materialize, and those
instances' published rows create MORE demands — a genuine RAW/WAR cycle,
the loop-carried hazard the fixpoint machinery exists to handle.

For this regime the frozen-read model is WRONG (it would freeze the
demand at batch start and miss demands generated mid-fixpoint — an
UNDER-materialization bug, silently incomplete). This needs the
round-structured hazard treatment the DR-IR already gives claimed
frontiers: the demand frontier becomes a round-Δ frontier (a loop-carried
role, DR.cpp:3486 is_round_carried_role — kNetRemoval/kNetAddition are
already classed loop-carried), read at each fixpoint round, refilled by
the round's instance publications, terminating when no new demand crosses
(the UPDATECOUNT zero-crossing termination invariant, CLAUDE.md core
differential invariants). This is exactly the claimed-frontier treatment:
the loop-carried RAW (this round's read sees the previous round's fill)
plus the intra-scope WAR (drain-before-refill), certified by V-LOOP
(DR.cpp:3529-3534).

CRITICALLY: R3 EXPLICITLY DOES NOT DO THIS. GROUP_UPDATE asserts its input
is non-recursive (DR.cpp:718 `assert(klass == DerivClass::kNonRecursive)`;
aggregates over induction-owned inputs are a clean-diagnostic reject,
CLAUDE.md). The SCC-demand regime is strictly BEYOND what R3 built — it is
the recursive analog R3 deferred, the same way StackSafeNegation took
acyclic @product before on-cycle @product (the ViewSelfReachable fence,
Build.cpp:200/:1152).

----------------------------------------------------------------------
§1.4 WHAT EACH ANSWER COSTS THE SCHEDULER (exact code sites)
----------------------------------------------------------------------
FROZEN-READ (acyclic) answer — the CHEAP answer, pure R3 replication:
  - DeriveDRStrata lift (DR.cpp:2249-2261): the instance op gets a lift
    rule IDENTICAL to GROUP_UPDATE's — stratum = max(subgraph view stratum,
    ready_after(demand_table)); the instance's own published-rows table
    drain lifts to the instance stratum (E4 analog). ONE new clause in the
    same monotone fixpoint loop. No new fixpoint structure.
  - op_stratum (DR.cpp:3298-3309): a new case keying on a
    subgraph_instance_stratum map, mirroring the GROUP_UPDATE case
    verbatim — NO silent default-0 (the critique's false-negative fix at
    :3300-3302 must be honored: a mis-stratified demand drain must fail
    V-READY, not pass on a default).
  - op_band (DR.cpp:3329-3335): band 0 (with seeds/products) if the
    instantiation seeds its published-rows queue before that table's own
    drain (E3 analog), OR a new band if publication has a distinct
    ordering. Recommendation: band 0, ride the existing V-SEED-DRAIN edge,
    exactly as GROUP_UPDATE.emit does.
  - the instance SEAL (if any — see §3): a STATE_SEAL analog at lead 2,
    band 10+ (DR.cpp:3417-3422), trailing commit sweeps.
  - V-BAND-HAZARD (DR.cpp:3482-3544): the demand frontier is registered as
    a NON-carried role (like kInI reads) — it never runs against the band
    key because it is produced strictly-lower and read strictly-higher. NO
    new hazard class; the reservation (DR.cpp:3541-3544 "asserted globally
    as V-BAND-HAZARD ... a non-loop-carried edge that RUNS AGAINST the band
    key ... cannot arise from THIS function by construction") stays intact.
  - V-READY (DR.cpp:3906-3921): the instance op is ON-lattice (unlike
    StateSeal/CommitSweep/IngestFold at :3913-3918); its demand-frontier
    RAW must satisfy reads-lower-or-same, which it does by construction
    (demand is strictly lower).

HAZARDED-WRITE (SCC) answer — the EXPENSIVE answer, new machinery:
  - the demand frontier must be enrolled as a loop-carried role
    (DR.cpp:3486 is_round_carried_role gains a kDemandFrontier role, or the
    demand reuses kNetAddition semantics); V-BAND-HAZARD's carried-role
    classification (DR.cpp:3550 `carried = is_round_carried_role(...) ||
    is_epoch_carried_role(...)`) must recognize it.
  - DeriveDRStrata must SCC-pin the demand table and the instance table at
    one shared stratum (DR.cpp:2263-2272 SCC pinning) — the mutual reads
    must not ratchet each other (ready_across exempts same-SCC reads,
    DR.cpp:2137-2139).
  - a FIXPOINT_ROUND region (DR.h:548-549 "§2.1 FIXPOINT_ROUND — a
    STRUCTURED REGION (container) per SCC × phase") must wrap the
    instantiation, with the demand read at claim-relative position and the
    publication refilling the round-Δ frontier. This is the whole
    recursive-round shell — the biggest cost.
  - V-LOOP (the loop-carried certification) must accept the demand
    frontier's carried RAW.
  - the UPDATECOUNT zero-crossing termination invariant (CLAUDE.md) must
    hold for demand generation — a demand that re-demands must fold through
    an UPDATECOUNT whose propagation is dominated by its zero crossing, or
    the fixpoint does not terminate. This is a NEW correctness obligation
    the acyclic case does not have.

----------------------------------------------------------------------
§1.5 RECOMMENDATION: STAGED SPLIT — ACYCLIC-FROZEN FIRST
----------------------------------------------------------------------
RECOMMEND: ship the FROZEN-READ (acyclic demand) model first; defer the
SCC-demand hazarded-write model to a follow-on, gated behind an explicit
ViewSelfReachable-style fence that rejects on-cycle demand with a clean
diagnostic.

Justification (the R2-acyclic-first discipline, made concrete):
  1. It is PURE R3 REPLICATION. Every scheduler cost in §1.4's frozen-read
     column is "one new clause mirroring the GROUP_UPDATE case." Zero new
     fixpoint structure, zero new hazard class, zero new carried role. The
     substrate the epoch thesis claims is "already there" IS already there
     for the acyclic case — this stage proves the thesis at minimum risk.
  2. It MATCHES the R3 fence exactly. GROUP_UPDATE asserts non-recursive
     input (DR.cpp:718) and rejects induction-owned inputs cleanly. The
     acyclic-demand instance inherits that fence verbatim: a demand
     produced strictly-lower, an instance strictly-higher. The
     SCC-demand case is the recursive analog R3 deferred — deferring it
     here is CONSISTENT, not a gap.
  3. The house precedent: StackSafeNegation landed acyclic @product before
     on-cycle @product (Build.cpp ViewSelfReachable fence, :200/:1152);
     R3 landed KV-as-degenerate-aggregate before general aggregates;
     R2 took acyclic families before recursive. Acyclic-first is the
     ratified epoch discipline.
  4. The SCC-demand case's NEW correctness obligation (§1.4: demand
     generation must fold through a zero-crossing-dominated UPDATECOUNT or
     the fixpoint diverges) is a real soundness risk that wants its OWN
     reviewed argument — exactly the R4-style gate PerfRoadmap §14.3-P3
     imposes on the demand transform. Bundling it into stage (b) would put
     an unreviewed recursion-termination argument on the critical path.

THE FENCE (concrete, mirrors the R3/product fences): at
BuildSubgraphOps-time (§3), assert the demand-producing view is NOT
self-reachable through the instance (reuse ViewSelfReachable,
Build.cpp:200 — NOT InductionGroupId, per the F22 lesson CLAUDE.md: a
fully interior join loses its group id). Reject on-cycle demand at the
Build.cpp pre-pass (the num_errors→nullopt gate, the same gate that
rejects induction-owned aggregate inputs, DR.cpp:2795-2797 comment). Its
witness is a synthetic on-cycle-demand .dr that must diagnose in all 4
modes (the agg_in_scc_1 / kv_in_scc_1 mold, CLAUDE.md).


======================================================================
§2. THE EFFECT SUB-DOMAIN — DECLARED TOTAL FROM DAY ONE (the R3 template)
======================================================================
R3 did NOT add a sixth top-level effect domain; it FILLED the reserved
`statecell: fold/emit/old` sub-domain and resolved the three conflicts
filling it exposed (v3-spec-statecell §0 C-0b/C-0c/C-0e). The instance
family follows the SAME shape: a new `instance:` sub-domain, declared
TOTAL, plus the conflict resolutions its filling exposes.

The current EffKind enum (DR.h:73-84, verified this session): kVecAppend,
kVecDrain, kVecClear, kCounter, kFlagRead, kFlagWrite, kInIReadFrozen,
kStateFold, kStateEmit, kStateOld. The instance family appends a NEW tail
of instance kinds, exactly as R3 appended kStateFold/Emit/Old.

----------------------------------------------------------------------
§2.1 THE NEW EFFECT KINDS (candidate spelling from §14.2(B))
----------------------------------------------------------------------
The seed candidate is `instance: instantiate(I) | demand(I) | publish(I)`.
Refined here against the R3 factoring (fold/emit/old split working vs
sealed), because "instantiate/demand/publish" conflates a READ (demand),
a WRITE (materialize), and a READ-of-output (publish). The precise
spelling, mapping each to its read/write word:

  EffKind::kDemandRead  (instance:demand)
      READS the demand frontier for instance I. In the acyclic-frozen
      model (§1.5) this is a FROZEN read — the kInI/kStateOld side: never a
      within-band hazard, RAW-only against the demand producer's frontier
      filter (E1 analog). It names the demand VALUE (the demand table) and
      the (context ++ config) key projection.  [read-only, frozen]

  EffKind::kInstanceFold  (instance:instantiate)
      WRITES the instance's standing state — materializes / updates the
      nested sub-relation for a demanded key. The hazarded WORKING side:
      the analog of kStateFold. A real read/write IR value (folds a
      demanded input delta into the instance's rows). NO presence crossing
      at the instance level (C-0c analog — the crossing is on the PUBLISHED
      rows' own DiffTable, later, at publish-touched).  [read/write working]

  EffKind::kInstanceEmit  (instance:publish, working read)
      READS the instance's CURRENT (post-fold) published-row set for a
      touched instance — the analog of kStateEmit. RAW after every
      kInstanceFold this epoch (hazarded). Feeds the publish-touched band
      that seeds the published-rows DiffTable.  [read working]

  EffKind::kInstanceOld  (instance:publish, sealed read)
      READS the instance's BATCH-START published-row set — the analog of
      kStateOld. A VALUED frozen sealed read; RAW-only against
      INSTANCE_SEAL; never a within-band hazard. Answers "what did this
      instance publish at batch start" so publish-touched can net one pair
      per (instance, changed row).  [read sealed, frozen]

  EffKind::kInstanceSeal  (the STATE_SEAL analog; carried on an
      INSTANCE_SEAL op, not GROUP_UPDATE — mirrors kStateSeal reusing
      kStateFold sign-0 at DR.cpp:754). global:rmw sealed := working for
      touched instances. Trailing commit band.  [global:rmw]

RATIONALE for splitting the seed's three into five: the seed's
"instantiate | demand | publish" maps 1:many onto the read/write words
the scheduler actually tracks. `demand` is one frozen read (kDemandRead).
`instantiate` is one hazarded write (kInstanceFold). `publish` is TWO
reads — a working read (kInstanceEmit, the new published set) and a sealed
read (kInstanceOld, the batch-start set) — exactly as R3's emit_touched
needs BOTH emit(g) and old(g) to net one pair (v3-spec-statecell §2.3).
Collapsing publish to one kind reintroduces the C-0b confusion (is it
hazarded or frozen?) the two-word split exists to prevent. FIVE kinds
keep the "frozen constant vs hazarded working" dichotomy total.

----------------------------------------------------------------------
§2.2 THE CONFLICTS FILLING THE DOMAIN EXPOSES (the C-0b/C-0c/C-0e analogs)
----------------------------------------------------------------------
Declared and resolved up front, so the scheduler + validators are
instance-total before the family exists (the v3-spec §2 "reserved so the
scheduler and validators are R3-total from day one" discipline).

  C-1b (state-as-hazard-vs-constant, C-0b one level up — THE §1 QUESTION):
    is the DEMAND a hazard or a constant, and is the INSTANCE STATE a
    hazard or a constant?
    RESOLUTION (from §1.5): TWO answers, split by regime.
      - the DEMAND is a FROZEN read (kInI-like) in the acyclic model —
        produced strictly-lower, read strictly-higher, never a within-band
        hazard. (In the deferred SCC model it becomes a loop-carried role;
        that is the follow-on, §1.3.)
      - the INSTANCE STATE is a real two-word cell exactly like a
        StateCell: `working` published-row set (kInstanceFold writes,
        kInstanceEmit reads — RAW) + `sealed` published-row set
        (kInstanceSeal writes, kInstanceOld reads — frozen). The same
        two-word split C-0b mandated. The demand frozen-read and the
        instance two-word state are INDEPENDENT axes: the demand keys the
        cell; the cell's contents are the hazarded state.

  C-1c (instance-fold != counter-fold != value-fold): kInstanceFold
    mutates a NESTED SUB-RELATION (a set of rows), not a scalar (kStateFold)
    and not a presence counter (kCounter). NO presence crossing at the
    instance level. The presence crossings live on the PUBLISHED-ROWS
    DiffTable, in publish-touched's ordinary UPDATECOUNTs (C-0c analog,
    verbatim: the counter± folds live ONLY in the emit/publish band's two
    ordinary UPDATECOUNTs, DR.cpp:720-738 GROUP_UPDATE's counter arms).
    RESOLUTION: kInstanceFold is a first-class effect kind, distinct from
    kStateFold and kCounter, precisely because its working value is a
    RELATION not a scalar. This is the genuinely-new axis vs R3.

  C-1e (the instantiation MODE is a lowering selector, not an ordering):
    an instance can be lowered by RE-DERIVATION (rerun the sub-query per
    demanded key — the @recompute analog, StateCell.h:159 Recompute) or by
    INCREMENTAL MAINTENANCE (fold demanded input deltas into the standing
    instance — the @invertible analog, StateCell.h:103 Invertible). This is
    a LOWERING SELECTOR (C-0e verbatim: like ACCESS's `Lowering how`,
    v3-spec.md §2:159 — picks an implementation, pins no order, no def/use
    edge). Carried as a SUBGRAPH_INSTANTIATE attribute, NOT an ordering
    fact. Stage (b) ships RE-DERIVATION first (the @recompute analog — the
    universal correct fallback, AggregatingFunctors §3 PER-GROUP RECOMPUTE
    THRESHOLD "often wins for small groups"); incremental maintenance is a
    later lowering knob (the II/mergeable-sketch analog, deferred).

  C-1a (NON-CONFLICT, recorded so it is not re-litigated): the "notional
    nested query object per instance" (AggregatingFunctors §4's
    keyed-instance thesis) is the SEMANTIC CONTRACT and the re-derivation
    physical fallback, NOT an IR value — exactly as C-0a resolved the
    per-group object. The instance's standing state is ONE new IR value:
    an `Instance` engine value whose runtime backing is the InstanceStore
    (§3). No per-instance IR object.

----------------------------------------------------------------------
§2.3 TOTALITY STATEMENT (the V-STATECELL-EFFECT analog)
----------------------------------------------------------------------
V-INSTANCE-EFFECT (R3-style totality, mirror v3-spec-statecell §2.5
V-STATECELL-EFFECT): every instance:fold/emit/old/demand effect names an
Instance value; no R1/R2/R3 op declares an instance effect
(SUBGRAPH_INSTANTIATE is the first and only). The reverse: every
SUBGRAPH_INSTANTIATE carries exactly {kDemandRead, kInstanceFold(×signs),
kInstanceEmit, kInstanceOld, + the publish-touched counter±/kInIReadFrozen/
kVecAppend triples}; every INSTANCE_SEAL carries exactly its sign-0
kInstanceSeal. Asserted structurally in ValidateDROps, the same shape as
the V-AGG-EFFECT check P0 landed (DR.cpp:2907-2914).


======================================================================
§3. THE InstanceStore vs StateCellStore (the C++ class sketch)
======================================================================
The InstanceStore is the StateCellStore's non-degenerate parent: same
(context ++ config) -> dense id keying, same non-aliasing invariant, same
occupancy/birth-death guard, same dead-instance compaction residue — but
the per-key cell holds a NESTED SUB-RELATION rather than a two-word
scalar.

----------------------------------------------------------------------
§3.1 WHAT IS THE CELL? — a nested DiffTable handle, NOT a Vec of rows
----------------------------------------------------------------------
DECISION: the per-instance payload is a HANDLE to a store-owned nested
DiffTable (the published-row set for that instance), NOT a flat Vec of
rows and NOT the raw StateCell two-word scalar.

Justification against the Runtime idiom + the compaction residue:
  1. The @recompute StateCell ALREADY holds a non-scalar per group — a
     membership multiset via a POD HANDLE {Vec<Summary>*, Vec<int32_t>*}
     into store-owned pools (StateCell.h:167-170, the
     std::is_trivially_copyable_v handle at :171). The instance cell is
     the SAME idiom scaled up: the per-instance nested relation is
     store-owned, referenced by a POD handle so `Vec<Working>` stays
     trivially-copyable (Vec<T> requires trivially-copyable T,
     StateCell.h:152-155). This is not a new pattern — it is the
     @recompute pattern with a DiffTable payload instead of value/count
     Vecs.
  2. A nested DiffTable (not a flat Vec) because the published rows are
     THEMSELVES differential: they gain and lose derivations, they need
     split signed counters (C_nr/C_r), kInI sealed reads, and net
     frontiers for the publish-touched net-pair. A flat Vec cannot express
     the "was this row published at batch start" (kInstanceOld) that the
     one-net-pair per (instance, row) contract needs. The published-row
     set IS a DiffTable; the InstanceStore owns one per live instance.
  3. THE COMPACTION RESIDUE transfers with a SHARPER edge (§14.0.1
     "instances can die; a dead-instance sweep is the InstanceStore
     analog"): each instance's nested DiffTable can compact its OWN dead
     rows independently (the existing DiffTable CompactDead machinery,
     dead>=live 4096-floor, CLAUDE.md data-structures epoch) — this rides
     for free, per-instance. The INSTANCE-ID namespace itself is
     append-only like StateCell group ids (StateCell.h:17-21 "a group id,
     once allocated, is retained for the program's life"); a dead-INSTANCE
     sweep (renumbering instance ids) is the D5-style residue, out of
     stage-(b) scope, MEASURE-FIRST on a dead-instance-accumulation COST
     witness (StateCell.h:21-24).

The sealed side: `sealed` is a SNAPSHOT of the published-row set at batch
start — for a re-derivation-lowered instance (the @recompute analog, the
stage-(b) default, §2.2 C-1e) this is the prior epoch's published DiffTable
state (a sealed row-id watermark, exactly Table::Seal's kInI watermark,
StateCell.h:29-30 "sealed ... WRITTEN only by Seal, READ by Old"). old()
of an instance answers "what rows did this instance publish at batch
start" via the nested DiffTable's own kInI watermark — the SAME id<sealed
compare a Table already does (v3-spec-statecell §1.2:186-189), lifted from
one boolean membership to a nested-relation membership.

----------------------------------------------------------------------
§3.2 THE CLASS SKETCH (repo idiom; new Runtime header, peer of StateCell.h)
----------------------------------------------------------------------
New file include/drlojekyll/Runtime/Instance.h (peer of StateCell.h and
Table.h; dependency-free hyde::rt; HYDE_RT_BENCH_COUNT seams). The store
IS a StateCellStore whose Algebra is a "nested-relation" policy — this is
the literal cash-in of "an aggregate is the degenerate instance": the
InstanceStore and StateCellStore share the keying/occupancy/touched
machinery; only the Working/Sealed types and the Fold/Emit bodies differ.

  namespace hyde::rt {

  // Standing per-instance state for ONE subgraph view. Keyed
  // (context ++ config) -> a DENSE instance id; columnar working/sealed
  // nested-relation handles indexed by that id. NON-ALIASING (mirror
  // StateCell.h:10-24): instance dense ids are a SEPARATE id space from
  // BOTH the demand table's row ids AND the published-rows DiffTables' row
  // ids. A DiffTable compaction NEVER touches this store; an instance's
  // OWN nested DiffTable may compact its rows without renumbering the
  // instance id. An instance id, once allocated, is retained for the
  // program's life even if the instance empties (append-only dense
  // namespace, the "monotone tables never compact" rationale).
  //
  // THE TWO-WORD CELL generalized (mirror StateCell.h:26-36): `working`
  // is the current-epoch nested published-row set (MUTATED by Fold =
  // materialize/maintain, READ by Emit — hazarded); `sealed` is the
  // batch-start snapshot (WRITTEN only by Seal, READ by Old — frozen,
  // kInI-like). For the re-derivation lowering, `working` is the live
  // nested DiffTable and `sealed` is its batch-start row-id watermark.
  //
  // OCCUPANCY (mirror StateCell.h:38-54): an instance is EMPTY or
  // OCCUPIED. WorkingOccupied/SealedOccupied drive the
  // occupancy-generalized publish_touched guard — birth (empty->occupied:
  // emit +new published rows only), death (occupied->empty: emit -old
  // rows only), change (occupied, published set differs: the net pair of
  // row deltas), no-op (unchanged: nothing).
  template <typename Key, typename Relation>
  class InstanceStore {
   public:
    // The nested published-row set. A store-owned differential relation
    // referenced by a POD handle (mirror Recompute::Working, StateCell.h:
    // 167-170) so Vec<Working> stays trivially-copyable.
    struct Working {
      Relation *rows{nullptr};   // store-owned nested DiffTable
    };
    static_assert(std::is_trivially_copyable_v<Working>,
                  "instance Working must be a POD handle (Vec<Working>)");
    using Sealed = uint64_t;     // batch-start row-id watermark (kInI-like)

    explicit InstanceStore(Allocator a)
        : allocator(a), keys(a), hashes(a), working(a), working_count(a),
          sealed(a), sealed_occupied(a), touched(a), touched_flag(a),
          relation_pool(a) {}
    InstanceStore(const InstanceStore &) = delete;
    InstanceStore &operator=(const InstanceStore &) = delete;
    ~InstanceStore(void);        // frees slots + the store-owned Relations

    uint32_t NumInstances(void) const noexcept { return keys.Size(); }

    // Dense instance id for `key`, allocating (identity-init = an EMPTY
    // nested relation) on first touch. Mirrors StateCellStore::
    // FindOrAddGroup (StateCell.h:273) / RowStore::FindOrAdd (Table.h:152).
    uint32_t FindOrAddInstance(const Key &key);   // wires working.rows
    uint32_t FindInstance(const Key &key) const noexcept;   // kNoInstance

    // Materialize/maintain: fold a demanded input delta into the instance's
    // nested relation (instance:instantiate = kInstanceFold). Records the
    // instance in `touched` once (mirror StateCellStore::Fold, :306-323).
    // NO instance-level presence crossing (C-1c) — the crossing is on the
    // nested Relation's own rows, surfaced at publish_touched.
    template <typename... Cols>
    void Fold(uint32_t iid, int32_t sign, Cols &&...row);

    // publish_touched reads: the CURRENT (kInstanceEmit) vs BATCH-START
    // (kInstanceOld) published-row sets, as net frontiers over the nested
    // Relation (row-id < sealed watermark = old; >= = new).
    const Relation &Emit(uint32_t iid) const { return *working[iid].rows; }
    Sealed          Old(uint32_t iid)  const { return sealed[iid]; }

    bool WorkingOccupied(uint32_t iid) const noexcept {
      return 0 < working_count[iid];   // net live rows folded this epoch
    }
    bool SealedOccupied(uint32_t iid) const noexcept {
      return 0u != sealed_occupied[iid];
    }

    // End-of-epoch: for each touched instance, sealed := the nested
    // Relation's post-publish row-id watermark; advance occupancy; clear
    // touched. Mirror StateCellStore::Seal (:353-363) / Table::Seal.
    void Seal(void);

    const Vec<uint32_t> &Touched(void);   // sort-unique (mirror :377-380)
    const Key &KeyAt(uint32_t iid) const noexcept { return keys[iid]; }
    void DebugValidate(void) const;       // §3.3 checks

   private:
    void Touch(uint32_t iid);             // append-once (mirror :451-458)
    Allocator allocator;
    Vec<Key> keys;                        // instance id -> key
    Vec<uint64_t> hashes;                 // cached key hashes
    Vec<Working> working;                 // POD handles to nested relations
    Vec<int32_t> working_count;           // net live-row count (occupancy)
    Vec<Sealed> sealed;                   // batch-start watermarks (old())
    Vec<uint8_t> sealed_occupied;         // batch-start occupancy bit
    Vec<uint32_t> touched;                // instances folded this epoch
    Vec<uint8_t> touched_flag;            // per-instance append-once bit
    Vec<Relation *> relation_pool;        // store-owned nested DiffTables
    uint32_t *slots{nullptr};             // open-addressing key -> iid
    size_t slot_capacity{0u};
  };

  }  // namespace hyde::rt

The store is LITERALLY the StateCellStore generalized: keys/hashes/
working/working_count/sealed/sealed_occupied/touched/touched_flag/slots
are the SAME fields (StateCell.h:518-530); relation_pool replaces
mem_values_pool/mem_counts_pool (StateCell.h:527-528); the only new type
is Relation (a nested DiffTable) in place of the scalar Working/Sealed.
This is the "same machinery, different payload" the thesis predicts.

----------------------------------------------------------------------
§3.3 THE OCCUPANCY/BIRTH-DEATH GUARD GENERALIZATION + NON-ALIASING
----------------------------------------------------------------------
OCCUPANCY generalizes StateCell.h:38-54 verbatim, one level up: the
StateCell guard nets ONE (group, scalar) pair; the instance guard nets
the ROW DELTAS between the batch-start and current published-row SETS.
  - birth (empty->occupied): publish every current row as +new (no phantom
    -old). Instance materialized this batch.
  - death (occupied->empty): publish every batch-start row as -old (no
    phantom +new). Instance's demand retracted this batch.
  - change (occupied, set differs): publish the symmetric difference —
    -old for rows dropped, +new for rows added (the nested Relation's own
    net frontiers ARE this delta; publish_touched drains them).
  - no-op (unchanged): publish nothing (OQ3 annihilation transfers).
The key generalization: StateCell's guard compares two SCALARS (new!=old);
the instance guard compares two RELATIONS (the nested DiffTable's net
frontiers). But the guard STRUCTURE — occupancy transitions drive which
side emits — is identical, and it composes with the nested DiffTable's own
one-net-pair-per-row machinery for free (the published rows ride the
existing claim/frontier/commit tail, exactly as GROUP_UPDATE.emit's
counter± do, DR.cpp:720-745).

NON-ALIASING INVARIANT (three-way, sharper than StateCell's two-way): the
instance-id space is disjoint from (a) the demand table's row ids, (b) the
published-rows DiffTables' row ids, and (c) EVERY OTHER instance's nested
DiffTable row ids. Asserted in debug (mirror StateCell.h:338-340
V-non-aliasing): an instance id never appears as a table.Find argument;
one instance's nested-relation row ids never leak into another's.
DebugValidate (mirror StateCell.h:534-573): seal coherence (touched empty
after Seal, no touched_flag survives), occupancy coherence
(sealed_occupied == working_count>0 at batch boundary, count >= 0), and
the non-aliasing set-intersection check.


======================================================================
§4. THE SURFACE QUESTION (owner input needed — FRAMED, NOT DECIDED)
======================================================================
With the demand transform (candidate (a)) deferred to P3, stage (b) has a
substrate but no obvious surface. WHAT creates a SUBGRAPH_INSTANTIATE op?
Three candidates, each with corpus/witness implications. This lane FRAMES
them; the owner picks.

GROUNDING FACT (verified this session): NO subgraph/demand parse surface
exists — the 18 `subgraph`/`demand` hits are incidental prose (PerfRoadmap
§14.2(C); confirmed: grep in lib/Parse finds nothing). Any surface is NEW.
The .dr `#query name(bound i32 X, free i32 Avg)` surface
(data/examples/average_weight.dr:12) and the `over(){}` aggregate body
(:24-33) are the existing keyed/nested-scope precedents.

----------------------------------------------------------------------
§4.1 CANDIDATE (i): a bound #query lowers to a demand-keyed instance
----------------------------------------------------------------------
A #query with bound columns (`#query q(bound i32 X, free i32 Y)`, the
average_weight.dr:12 shape) already IS a demand-keyed nested computation:
each distinct bound X is a demanded instance; the free Y columns are the
published rows. Under this candidate, a bound #query lowers to ONE
SUBGRAPH_INSTANTIATE keyed on the bound columns (the context) with the
free columns as the published-row set.

  PRO: exercises real corpus TODAY (every bound #query in data/). No new
    .dr syntax. The demand is the SET OF BOUND ARGUMENTS the driver passes
    to the generated query entry point (q_bf(db, x) — the cursor API,
    CLAUDE.md generated API). This is the most honest realization of "a
    subgraph is a demand-driven keyed nested query instance."
  CON: it changes the LOWERING of an existing surface — a golden-churn
    risk across the whole bound-#query corpus, in all 4 modes. The demand
    is DRIVER-SUPPLIED (per-call bound args), not a dataflow-produced
    frontier — so the "demand frontier" is the query-entry argument vector,
    not a lower-stratum table. This is a DIFFERENT demand shape than the
    §1 frozen-lower-stratum read (it is a per-call boundary input, closer
    to an ingest fold than a frontier filter). The §1 acyclic-frozen
    analysis still holds (the bound args are final before instantiation)
    but the demand EDGE is a boundary edge, not an E1 frontier RAW.

----------------------------------------------------------------------
§4.2 CANDIDATE (ii): an explicit .dr surface (a #subgraph decl)
----------------------------------------------------------------------
A new `#subgraph`-style declaration: a named nested query parameterized by
context ++ config, referenced from rule bodies at a bound pattern (the
`over(){}` block is the syntactic precedent — a lexically-scoped nested
computation, average_weight.dr:24-33). The reference site produces the
demand; the decl produces the instance.

  PRO: a CLEAN new op with a CLEAN new surface — no existing-lowering
    churn (the aggregate_1 diagnostic->golden FLIP precedent, CLAUDE.md:
    a new surface adds a golden, moves nothing). The demand is a genuine
    dataflow frontier (the reference site's bound-column projection feeds
    a demand table the instantiation drains — the §1 E1 frozen read
    EXACTLY). The census/oracle can referee it as a fresh corpus case.
  CON: new parse + dataflow-node surface (the biggest NEW code). Risks
    designing syntax the demand transform (P3) would rather SYNTHESIZE
    than have the user write — i.e. building a surface that (a) obsoletes.
    AggregatingFunctors §4 frames config columns as "the same lexical-scope
    move as SLDMagic's use_query_const" — arguing the surface should be
    INTERNAL (a transform output), not user-facing.

----------------------------------------------------------------------
§4.3 CANDIDATE (iii): internal-only, gated behind (a)
----------------------------------------------------------------------
SUBGRAPH_INSTANTIATE is produced ONLY by the demand transform (P3); stage
(b) is substrate-WITHOUT-surface, its witness SYNTHETIC (a hand-built
DR-IR fixture, no .dr, the op minted by a test harness that stands in for
the not-yet-built transform).

  PRO: builds the substrate the recorded plan says to build first
    (PerfRoadmap §14.0 "the keyed-instance substrate is the prerequisite
    the recorded plan says to build first") WITHOUT committing to a surface
    the transform might reshape. The census + validators + InstanceStore +
    effect domain all land and are TESTED against a synthetic witness.
  CON: NO oracle-refereed corpus case (there is no .dr to run). The seven-
    point checklist's oracle/monotone referees (point 4/the bin/Oracle
    path) CANNOT be exercised — the oracle needs a runnable program. The
    census + structural validators (points 4a/6/7) CAN be exercised on the
    synthetic DR-IR fixture, but the end-to-end golden gate cannot. This is
    the "substrate landed, witness synthetic" honesty cost PerfRoadmap
    §14.3-P2 flags for stage (b) if (a) is deferred.

----------------------------------------------------------------------
§4.4 WHICH CHECKLIST ITEMS EACH CANDIDATE EXERCISES
----------------------------------------------------------------------
  checklist point          (i) bound #query   (ii) #subgraph   (iii) internal
  1 df->dr construction        YES                YES               YES(synthetic)
  2 stratum lift               YES                YES               YES
  3 linearizer band key        YES                YES               YES
  4 census                     YES                YES               YES
  4-oracle referee             YES(existing q)    YES(new corpus)   NO (no .dr)
  4-monotone referee           YES                YES               NO
  5 dr->cf lowering            YES                YES               YES
  6 cf->c++ emission           YES                YES               YES(no golden)
  7 reduction-body ABI         maybe(demand pred) YES(demand pred)  N/A

RECOMMENDATION FOR OWNER (framing, not deciding): candidate (ii) exercises
the MOST checklist points including the oracle/monotone referees WITHOUT
existing-lowering churn — it is the aggregate_1-flip-shaped path (a clean
new corpus case). Candidate (i) is the most honest realization but risks
whole-corpus golden churn. Candidate (iii) is the safest substrate build
but forfeits the oracle referee (the epoch's correctness spine). If the
owner wants stage (b) to STAND on its own witness this epoch, (ii). If
stage (b) is explicitly a substrate-only down-payment on (a), (iii). The
§5 hand-written case below is written under (ii) so the target IR is
concrete regardless — under (i) the same IR is reached from a bound
#query; under (iii) from a synthetic mint.


======================================================================
§5. ONE REAL SUBGRAPH CASE (hand-written, under candidate (ii))
======================================================================
A demand-driven nested query: per demanded start node, the set of nodes
reachable within a bounded lexical scope. Acyclic demand (the §1.5
recommended regime): demand is produced strictly-lower (the reference
site), the instance strictly-higher.

----------------------------------------------------------------------
§5.1 THE .dr SOURCE (candidate (ii) surface)
----------------------------------------------------------------------
  #message add_edge(i32 From, i32 To).
  #message ask(i32 Start).                ; the demand source
  #local edge(i32 From, i32 To).
  #query neighborhood(bound i32 Start, free i32 Node).

  edge(From, To) : add_edge(From, To).

  ; A subgraph: per demanded Start, the direct out-neighborhood.
  ; `neighborhood` is the nested instance keyed on Start (the context);
  ; no config columns in this minimal case.
  #subgraph neighborhood(bound i32 Start, free i32 Node) {
    edge(Start, Node)
  }

  ; The demand: `ask(Start)` requests the instance for Start.
  demand_neighborhood(Start) : ask(Start).

Reading: `ask` seeds a demand relation `demand_neighborhood`. The
`#subgraph neighborhood` decl is instantiated once per demanded Start; its
published rows are (Start, Node) for each out-edge. The DEMAND is produced
by `demand_neighborhood` (strictly lower stratum — it depends only on the
`ask` message), the INSTANCE published rows are strictly higher. Acyclic:
no published row re-demands (the §1.5 fence holds — ViewSelfReachable of
the demand view through the instance is false).

Minimal on purpose: ONE demand key column (Start = context), zero config
columns, a monotone published set (edge is add-only here). The config
column and the deletable-published-row generalizations ride the exact
GROUP_UPDATE machinery (group_cols = context ++ config, DR.h:537; the
published DiffTable is differential like the agg table).

----------------------------------------------------------------------
§5.2 THE EXPECTED DR OPS (full effect sets, v3-spec §2.1 style)
----------------------------------------------------------------------
Mirrors BuildGroupUpdateOps' construction (DR.cpp:676-758) one level up.
ONE SUBGRAPH_INSTANTIATE + ONE INSTANCE_SEAL per subgraph view.

OP  SUBGRAPH_INSTANTIATE(subgraph neighborhood, instance INST,
        context={Start}, config={}, published={Node},
        mode=Rederive, class=NonRecursive, provenance=subgraph)
  ctx = kSeed;  instance_store_id = 0;  inst_table = neighborhood_pub

  BAND (a) demand_in — over the demand view's net frontiers.
    Structurally a THIRD frontier-vec consumer of the demand stratum,
    sibling of GROUP_UPDATE.frontier_in (DR.cpp:687-703), single input
    source, no join partner. Per SIGN arm (demand is differential):
      − arm:  vector:drain(demand_neighborhood.NetRemovals)    [kVecDrain,
                 vec_role = kNetRemoval]
              instance:demand(INST)  read the retracted demand key
                 [kDemandRead — FROZEN, §1.5 acyclic]
              instance:instantiate(INST, −1)  retract the instance's rows
                 for the un-demanded key  [kInstanceFold, sign −1, RW working]
      + arm:  vector:drain(demand_neighborhood.NetAdditions)   [kVecDrain,
                 vec_role = kNetAddition]
              instance:demand(INST)  read the new demand key   [kDemandRead]
              instance:instantiate(INST, +1)  materialize the instance
                 (mode=Rederive: rerun `edge(Start,_)` for Start)
                 [kInstanceFold, sign +1, RW working]
    The ONLY demand membership predicates consumed are kNetAdded/kNetDeleted
    via the drained frontier vecs (mirror DR.cpp:695 VecRole selection). NO
    table:counter± here (C-1c: instantiate is relation-mutation, not a
    presence crossing).

  BAND (b) publish_touched — over INST.Touched() (sort-unique, the
    V-TOUCH-SORTED analog). Per touched instance i, per changed published
    row r (the nested Relation's net frontier between sealed and working):
      read new := emit(i)  [kInstanceEmit, R working];
      read old := old(i)   [kInstanceOld, R sealed — frozen];
      occupancy-generalized guard (§3.3):
        for each row dropped (in old, not new):
          counter−(neighborhood_pub DiffTable, NonRecursive) over
            (Start ++ old-row);
            flags:read(pub, kInI)  [kInIReadFrozen];
            vector:append(pub.delQ)  [kVecAppend, kDeleteQueue]    ← UPDATECOUNT
        for each row added (in new, not old):
          counter+(neighborhood_pub DiffTable, NonRecursive) over
            (Start ++ new-row);
            flags:read(pub, kInI)  [kInIReadFrozen];
            vector:append(pub.addQ)  [kVecAppend, kAddQueue]       ← UPDATECOUNT
    The counter±/kInIReadFrozen/kVecAppend triples are the EXACT
    GROUP_UPDATE emit_touched vocab (DR.cpp:720-738) — the whole tail rides
    the existing claim/frontier/commit machinery. The generalization vs R3:
    R3 emits ONE net pair per touched group (scalar new!=old); the instance
    emits ONE net pair PER CHANGED ROW per touched instance (relation
    delta). Same effect kinds, iterated over the row delta.

OP  INSTANCE_SEAL(INST)   (the STATE_SEAL analog, DR.cpp:748-758)
  ctx = kSeed;  instance_store_id = 0;  inst_table = neighborhood_pub
  effect:  instance:seal(INST)  [kInstanceSeal — global:rmw
             sealed := working watermark for touched instances, §B-8 analog]

FULL EFFECT TABLE (SUBGRAPH_INSTANTIATE, both bands):
  op                      effects
  INSTANTIATE.in(−arm)    vector:drain(demand.NetRemovals);
                          instance:demand(INST)        [kDemandRead, frozen];
                          instance:instantiate(INST,−1)[kInstanceFold, RW]
  INSTANTIATE.in(+arm)    vector:drain(demand.NetAdditions);
                          instance:demand(INST)        [kDemandRead, frozen];
                          instance:instantiate(INST,+1)[kInstanceFold, RW]
  INSTANTIATE.publish     instance:emit(INST)  [kInstanceEmit, R working];
                          instance:old(INST)   [kInstanceOld, R sealed frozen];
                          {per dropped row} counter−(pub,NR);
                            flags:read(pub,kInI); vector:append(pub.delQ);
                          {per added row}   counter+(pub,NR);
                            flags:read(pub,kInI); vector:append(pub.addQ)
  INSTANCE_SEAL(INST)     instance:seal(INST)  [global:rmw, trailing band]

Per-arm effect sets (A-3): each arm carries its own set; the op-level
union is coarse-scheduling only (mirror DR.cpp:687-703's per-sign arms).

DEPENDENCE EDGES (derived, per v3-spec §4; NOT attributes — mirror
v3-spec-statecell §2.4 E1-E6):
  E1  demand-frontier RAW: demand_neighborhood's FRONTIER_FILTER writes
      demand.NetRemovals/NetAdditions; INSTANTIATE.in DRAINS them => RAW
      (filter-before-drain). Same shape as GROUP_UPDATE.in (DR.cpp:695) and
      CROSSOVER's drain of a negated table's frontier vecs.
  E2  instantiate-before-emit RAW: every INSTANTIATE.in fold WRITES
      working; INSTANTIATE.publish READS working (emit(i)) => RAW. The
      band-order edge (all folds before any emit), DERIVED from working's
      def/use (the E2 fold-before-emit analog, v3-spec-statecell §2.4).
  E3  publish-before-pub-drain (seed-before-drain): publish_touched's
      counter± APPEND to pub.delQ/addQ; pub's CLAIM_DRAIN drains them =>
      RAW; the pair MUST precede the drain (phantom-drop of the claim gates
      depends on seed-before-drain). Reuse V-SEED-DRAIN (DR.cpp:743-745
      registers the emit_touched counter± def-edges — the SUBGRAPH analog
      registers publish_touched's def-edges identically).
  E4  pub-drains-after-publish: the whole neighborhood_pub acyclic band
      (claim drains, frontier filters, commit) is downstream of
      publish_touched. DERIVED from E3 transitively (the E4 analog).
  E5  seal-after-publish (V-INSTANCE-SEAL): INSTANCE_SEAL reads working
      (sealed:=working watermark); MUST run AFTER publish_touched read
      working as `new` => ordering INSTANCE_SEAL after INSTANTIATE.publish;
      trailing commit band (mirror STATE_SEAL band key DR.cpp:3417-3422).
  E6  demand-stratum-final (V-INSTANCE-SEAL half 2): no old()/emit() read
      of INST before the DEMAND stratum's frontier filters are FINAL —
      DERIVED from E1 (the E6 analog).

----------------------------------------------------------------------
§5.3 THE InstanceStore CELL LAYOUT FOR THIS CASE
----------------------------------------------------------------------
  Key       = {Start:i32}                (context; config empty)
  Relation  = neighborhood_pub_nested    (a DiffTable of {Node:i32})
  working[iid].rows -> the live nested DiffTable of Node values for Start
  sealed[iid]        = batch-start row-id watermark of that nested table
  working_count[iid] = net live Node rows this epoch (occupancy)
  sealed_occupied[iid] = was this Start demanded at batch start

  For demand `ask(5)` in batch 1 (edges 5->7, 5->9 present):
    FindOrAddInstance({5}) -> iid 0; working[0].rows = {} (empty).
    Rederive fold(+1): rerun edge(5,_) -> materialize {7, 9} into
      working[0].rows.  working_count[0] = 2 (occupied, BIRTH).
    publish_touched: old={} (sealed_occupied=0), new={7,9} => birth =>
      publish +new only: counter+(pub,{5,7}), counter+(pub,{5,9}).
    INSTANCE_SEAL: sealed[0] := working[0] watermark; sealed_occupied[0]=1.
  Batch 2, ask retracted (demand_neighborhood loses {5}):
    − arm drain -> instantiate(INST,−1) -> retract Start=5's rows.
    working_count[0] -> 0 (DEATH).  publish_touched: death => publish −old
    only: counter−(pub,{5,7}), counter−(pub,{5,9}).  SEAL: sealed_occ=0.

  NON-ALIASING: iid 0 is disjoint from neighborhood_pub's row ids and from
  the nested Relation's own row ids (§3.3 three-way invariant).

----------------------------------------------------------------------
§5.4 STRATUM/BAND PLACEMENT
----------------------------------------------------------------------
STRATUM (the §14.2(C) item-2 initial-stratum rule this op gets): a NEW
initial-stratum rule mirroring GROUP_UPDATE's (DR.cpp:2118-2124). The
SUBGRAPH_INSTANTIATE sits at its subgraph VIEW's Stratify stratum, which
Stratify placed STRICTLY ABOVE its demand input (the §1.5 acyclic fence =
the aggregate-strictly-above-input rule, DR.cpp:2119-2121). The LIFT
(DR.cpp:2249-2261 analog): stratum = max(subgraph view stratum,
ready_after(demand_table)); the instance's OWN published-rows table drain
lifts to the instance stratum (E4). ONE new clause in the DeriveDRStrata
monotone fixpoint, structurally identical to the GROUP_UPDATE clause.

  op_stratum (DR.cpp:3298-3309 analog): a new case keying on a
    subgraph_instance_stratum map — NO silent default-0 (the false-negative
    fix at :3300-3302 honored: a mis-stratified demand drain must fail
    V-READY).

BAND KEY (the §14.2(C) item-3 band): band 0 (with seeds/products/
GROUP_UPDATE, DR.cpp:3329-3335) — the publish_touched counter± seed the
neighborhood_pub queues BEFORE that table's own claim drain (band 1) and
frontier filter (band 2) at the SAME stratum (E3 seed-before-drain, rides
the existing V-SEED-DRAIN edge). INSTANCE_SEAL: lead 2, band 11 (one past
STATE_SEAL's band 10, DR.cpp:3421), trailing commit sweeps and state seals
for deterministic order.

For the example: stratum 0 = {edge from add_edge, demand_neighborhood from
ask}; stratum 1 = {SUBGRAPH_INSTANTIATE + neighborhood_pub drain/filter}.
The band key composite (lead=1, stratum=1, band=0, table_id=pub, sign,
ctor) places INSTANTIATE.publish before pub's drain — exactly the
GROUP_UPDATE composite (DR.cpp:3423).


======================================================================
§6. THE SEVEN-POINT PLUG-IN CHECKLIST MAPPED (§14.2(C), anchors
    fleet-verified exact per ledger §1)
======================================================================
For each R3 GROUP_UPDATE integration point: the subgraph analog, census
from day one (P0 is the template), validators.

  1. df->dr CONSTRUCTION (BuildGroupUpdateOps DR.cpp:638; called per AGG
     :1034 / per KVINDEX :1048; mints op :676, seal :748, effects :691-745).
     ANALOG: a BuildSubgraphOps called per subgraph view, minting ONE
     kSubgraphInstantiate op with its five-kind effect set (§2.1) + ONE
     kInstanceSeal (mirror the seal at :748-758). Under candidate (ii) the
     caller iterates a new query.Subgraphs() accessor; under (i) it iterates
     bound query.IOs(); under (iii) a synthetic mint. The
     acyclic-fence assert (DR.cpp:718 klass==kNonRecursive) becomes the
     §1.5 ViewSelfReachable fence. The V-AGG-SOLE non-aliasing asserts
     (DR.cpp:663-665) become the three-way instance-id non-aliasing assert
     (§3.3).
     CENSUS FROM DAY ONE (E-25 charter — do NOT copy the gap P0 just closed):
     exp_subgraph = |query.Subgraphs()| (candidate ii) — the recount input
     is the SAME accessor the mint loop iterates (E-27 lesson: recount from
     the QUERY, never from flow.instancestores.size() which is a mint-order
     tautology). exp_instance_seal = exp_subgraph. Two new expect() entries
     (mirror DR.cpp:2850-2851). Per-op key multiset: (inst_table*,
     provenance, mode, view UniqueId) — NOT instance_store_id (mint-order
     artifact, the E-28 lesson: statecell_id deliberately unkeyed).
     VALIDATORS: V-INSTANCE-EFFECT (§2.3 totality), V-INST-SOLE (the
     subgraph is the pub table's sole deriver, mirror V-AGG-SOLE
     DR.cpp:2907-2914), V-INST-PAIR (instance_store_id bijection with
     INSTANCE_SEAL, mirror V-AGG-PAIR).

  2. STRATUM LIFT (DeriveDRStrata DR.cpp:2058; GROUP_UPDATE seed :2118-2124,
     lift :2249-2261). ANALOG: a subgraph_instance_stratum map seeded from
     the subgraph view's Stratify stratum (§5.4), lifted by
     max(view stratum, ready_after(demand_table)); the pub table drain
     lifts to the instance stratum. ONE new clause in the monotone fixpoint.
     CENSUS: n/a (strata are checked by V-READY, not counted). VALIDATOR:
     the op_stratum case with NO default-0 (DR.cpp:3300-3302 false-negative
     fix honored).

  3. LINEARIZER BAND KEY (LinearizeAndValidateDRFlow DR.cpp:2963; op_band
     :3323, key_of :3401). ANALOG: op_band returns 0 for
     kSubgraphInstantiate (with GROUP_UPDATE, DR.cpp:3335); key_of gives
     kInstanceSeal a lead-2 band-11 trailing key (mirror :3417-3422). The
     demand frontier introduces the ordering question §14.2(C) flags
     (demand satisfied BEFORE the instance materializes) — resolved by E1
     (demand strictly-lower => the RAW is key-forward by construction, NO
     band hazard). VALIDATOR: V-BAND-HAZARD stays reserved (§1.4: the
     acyclic demand read is a non-carried role that never runs against the
     band key, DR.cpp:3541-3544 reservation intact); V-READY checks the
     demand RAW reads-lower-or-same (DR.cpp:3922).

  4. CENSUS + VALIDATORS (ValidateDROps DR.cpp:2323; the derived-vs-derived
     census 2790-2851). ANALOG: the P0 GROUP_UPDATE census is the LITERAL
     TEMPLATE (E-25 charter, SubgraphsDemand.md §2). RECOUNT INPUTS: the
     same-accessor recount (§6.1 above) — exp_subgraph from
     query.Subgraphs(), NEVER from flow (E-27). Key multiset order-free,
     instance_store_id excluded (E-28). This is where the house bet lives:
     a first-time census FINDS a divergence (E-1..E-27 precedent). Ship it
     day one — the family MUST NOT copy the gap.

  5. dr->cf LOWERING (LowerGroupUpdate Stratum.cpp:1363; dispatch from
     LowerDRFlow :1596). ANALOG: a LowerSubgraphInstantiate emitting band
     (a) demand-guarded fold (the Rederive mode reruns the sub-query per
     demanded key — a nested scan) + band (b) publish_touched (the
     occupancy-generalized row-delta net pairs, §3.3), dispatched from
     LowerDRFlow. CENSUS: n/a (lowering is checked by the emitted-tree
     gate + V-PRED-XCHECK). VALIDATOR: a V-PRED-XCHECK site for
     SUBGRAPH_INSTANTIATE (the emitted CF tree ties back to the flow op —
     the F17/F18 bug-class kill for the new family, continuing the site
     numbering after ingest's §6 sites).

  6. cf->c++ EMISSION (EmitGroupUpdate Database.cpp:1970,
     ProgramGroupUpdateRegionImpl Program.h:1067; EmitStateCellStructs
     :1076, header assembly :2967). ANALOG: an EmitSubgraphInstantiate +
     an EmitInstanceStructs generalizing the StateCell blob (§3.2's
     InstanceStore<Key,Relation> template + the per-instance nested
     DiffTable structs). The Reduce_<id> policy analog is the nested
     sub-query's re-derivation body. CENSUS: n/a (emission checked by the
     golden gate). VALIDATOR: the byte-identical golden gate (under
     candidate ii, a new oracle-refereed corpus case; under iii, no golden).

  7. REDUCTION-BODY ABI (the C-5 driver-supplied free functions,
     Database.cpp:1271 decl / Sanitize call sites). ANALOG: a subgraph
     instance needs NO reduction body for the Rederive lowering (the
     sub-query IS the body, emitted inline) — but a DEMAND PREDICATE
     (which ground instances to materialize under a future demand-pred
     surface) may want the same driver-hook free-function ABI. The unified
     free-function surface P1 landed is the seam (§14.2(C) point 7).
     Stage (b) with Rederive lowering needs ZERO new driver ABI (the
     cheapest checklist point). CENSUS: n/a. VALIDATOR: the
     func.Name()-vs-Sanitize decl/call symmetry (the §13 P4 residue) — if a
     demand-pred free function lands, honor the fix.


======================================================================
§7. PRE-REGISTERED PREDICTIONS + STAGING
======================================================================
----------------------------------------------------------------------
§7.1 STAGING: (b) IS LANDABLE THIS EPOCH AFTER (c), UNDER CANDIDATE (ii),
     ACYCLIC-FROZEN ONLY
----------------------------------------------------------------------
CLAIM: stage (b) — the acyclic-frozen keyed-instance family under
candidate (ii) — is LANDABLE this epoch after stage (c) (config-column
aggregates), and is NOT design-only-until-(a). The SCC-demand regime (§1.3)
and the demand transform (a) are the deferred halves.

Rationale (the falsifiable core): every stage-(b) scheduler cost is "one
new clause mirroring the GROUP_UPDATE case" (§1.4 frozen-read column,
§6 checklist). The InstanceStore IS the StateCellStore generalized (§3.2 —
same fields, Relation payload). The only genuinely-new axis is
kInstanceFold's relation-valued working (C-1c) and its row-delta net pair
(§3.3) — and that composes with the existing nested-DiffTable machinery.
There is NO new fixpoint structure, NO new hazard class, NO new carried
role in the acyclic regime. This is the thesis's minimum-risk proof.

What makes it design-only-until-(a): NOTHING, under candidate (ii). Under
candidate (iii) it is substrate-only (no oracle referee, §4.3). Under
candidate (i) it risks whole-corpus churn (§4.1). The surface choice (§4)
is what gates "landable with a real witness" vs "substrate down-payment."

----------------------------------------------------------------------
§7.2 PRE-REGISTERED PREDICTIONS (per stage)
----------------------------------------------------------------------
STAGE (c) config-column aggregates (the prerequisite, E-31 forked):
  - the config-as-PARTITION-KEY half: SUITE grows by one oracle-refereed
    corpus case (config_agg_1.dr + .batches + oracle/monotone goldens);
    existing 164 byte-identical; the Build.cpp:1108 fence lifted;
    P0 census fires ZERO (config is already in group_cols, DR.h:537).
  - the config-DEPENDENT-reduction half (E-31): if the corpus case
    exercises a config-dependent reduction (it MUST, per E-31, or the gap
    ships un-caught), the C-5 free-function ABI extends (f_combine/
    f_uncombine/f_reduce gain the config value) — a real ABI change,
    a golden move on the new case only.

STAGE (b) keyed nested instances (candidate ii, acyclic-frozen):
  - SUITE grows past 164 by the subgraph corpus case (neighborhood.dr +
    .batches + oracle/monotone goldens) — the aggregate_1 diagnostic->golden
    FLIP precedent (CLAUDE.md). Existing 164 byte-identical (a new-feature
    path, not a rewrite of existing lowering — UNLESS candidate (i) is
    chosen, then whole-bound-#query churn).
  - THE HOUSE BET: the SUBGRAPH census FIRES at least once on a real
    miscount the first time it runs (E-1..E-27: every first-time census
    finds a divergence — the R1e ingest census did on day one). Most likely
    failure mode: the recount accessor (query.Subgraphs()) and the mint
    enrollment disagree on a corner shape (a subgraph with empty published
    columns, or a demand that reaches the instance through table-less
    plumbing — the DR.cpp:653-662 input-walk analog, which is exactly where
    a miscount hides). PREDICTION: the census pays.
  - Q5 NEUTRAL (subgraphs are a new-feature path, not a Q5-chain change;
    the flagship @128 point unmoved, per the P0 flat result).
  - ctest 3/3 unchanged; DR-IR always-on validators compile-time green
    (now including V-INSTANCE-EFFECT/SOLE/PAIR + the census).
  - a V-PRED-XCHECK site added for SUBGRAPH_INSTANTIATE (point 5) —
    FINDINGS entry iff it fires (the emitted-tree↔flow divergence, the
    F17/F18 bug class for the new family).

DEFERRED (NOT this epoch, pre-registered as deferred):
  - the SCC-demand hazarded-write regime (§1.3) — gated behind an explicit
    on-cycle-demand ViewSelfReachable reject (a clean 4-mode diagnostic,
    the agg_in_scc_1 mold), its own reviewed recursion-termination argument
    (the R4-style gate, §1.5 justification 4).
  - the demand transform (candidate a, P3) — the payoff; the instance
    family is its substrate.
  - incremental-maintenance instance lowering (the @invertible analog,
    §2.2 C-1e) — a later lowering knob; stage (b) ships Rederive only.
  - dead-INSTANCE compaction (§3.1) — D5-style, MEASURE-FIRST on a
    dead-instance-accumulation COST witness.

----------------------------------------------------------------------
§7.3 THE ONE THING A JUDGE SHOULD ATTACK FIRST
----------------------------------------------------------------------
The load-bearing claim is §3.1: that the instance cell is a nested
DiffTable HANDLE and that this "rides the existing DiffTable machinery for
free." The falsification target: does the occupancy-generalized guard
(§3.3, relation-delta net pairs) actually compose with the nested
DiffTable's OWN one-net-pair-per-row commit sweep WITHOUT double-counting
a row that is both (a) dropped by the instance guard (demand retracted)
and (b) independently retracted inside the nested relation? The R3 guard
nets ONE scalar pair and cannot hit this — the instance guard nets a SET
and must. If double-counting is possible, the "same machinery" thesis has
a real seam here, and §3.3 needs a claim-gate-order argument (the
seed-before-drain E3 phantom-drop applied to the relation delta) that this
lane asserts but does not fully discharge. That is the highest-value
adversarial probe.
