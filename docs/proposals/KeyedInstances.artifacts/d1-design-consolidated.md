======================================================================
COMMITTED AT THE §19 CHECKPOINT (2026-07-20, tip 1aaca896). This is the
D1 design fleet's binding consolidated record, verbatim from the session
scratchpad (fleet-d1/d1-design-consolidated.md). WHERE THIS RECORD AND
d1-pinned.md DISAGREE, d1-pinned.md WINS — its preamble enumerates the
superseded clauses (§A.7/decision-9's permcheck claim, §A.3.4's seal
wiring, §A.2.1's "compile-time-absent", §A.2.3's "always" drain framing,
the §B "OD-6" labels, §B-D3.a's churn line). Sibling references resolve:
GROUND-TRUTH.md → d1-ground-truth-nbhd.md; fleet-ds/ds-consolidated.md →
d1-desired-states.md; probes/diff_demand_analog.* + da.deltarel → the
appendices of d1-ground-truth-nbhd.md.
======================================================================

# D1 DESIGN — CONSOLIDATED RECORD (fleet-d1, binding)

Consolidator: XHIGH, 2026-07-19. Tip 1aaca896 (frozen; NO D1/D2 code exists).
Inputs: lane-{annot,ops,store,eqgate}.md, critique-{annot,ops,store,eqgate}.md,
critique-cross.md, GROUND-TRUTH.md (GT-1..GT-6), the lane-ops probe
diff_demand_analog.* (R-DIFF donor), witness-deltarel-target.md (H1-H11),
epoch-diffs §D1/§D2 (PRE-PICK-A; counts stale), KeyedInstances §18/§12/§13/
§0.6/§17, tc-four-adornment-target §3-§5, demand_neighborhood_witness.dr.
EVERY critique finding below was re-adjudicated at the code by this
consolidator; verification cites are mine, at tip.

VERDICT: READY-FOR-DESIRED-STATES — with the adjudicated amendments folded in
(§A) and the diff sequence re-cut (§B). The fleet converges on one
architecture; the three cross-lane collisions (X-1/X-2/X-3) are resolved
below by consolidation, not left to implementers. One consolidator-minted
finding (X-9, §0.3) corrects a contradiction the cross critic missed.

======================================================================
§0. ADJUDICATION LEDGER — every critique finding, accepted/rejected AT CODE
======================================================================

Notation: ACCEPT = folded into §A; ACCEPT-AMENDED = folded with a change;
REJECT = not folded (reason given). All verified at tip 1aaca896.

§0.1 critique-annot (lane verdict UNSOUND-reducible — UPHELD; reduced in §A.1)
- F1 [HIGH] ACCEPT. VERIFIED: `UniqueId()` = `reinterpret_cast<uintptr_t>(impl)`
  (include/drlojekyll/Util/Node.h:31-33); QueryViewImpl has NO id member (the
  `unsigned id` at lib/DataFlow/Query.h:115 is QueryColumnImpl's; the only view
  ordinal is the transient `det_seq{~0u}` at :472). The lane's id-keyed,
  id-sorted side table is pointer-ordered — an (F) breach as specified. FOLD:
  §A.1 de-pointers identity (opaque-pointer map for lookup/equality ONLY, never
  ordered; all ordered consumption driven by deterministic walk order).
- F2 [HIGH] ACCEPT-AMENDED. VERIFIED: `QueryViewImpl::ReplaceAllUsesWith`
  (lib/DataFlow/View.cpp:602-603) funnels into `SubstituteAllUsesWith` (:575)
  which calls `CopyDifferentialAndGroupIdsTo(that)` (:588) — and the Join
  self-canon RAUW sites (Join.cpp:158 `ReplaceAllUsesWith(tuple)`, :424
  `SubstituteAllUsesWith(tuple)`) BOTH route through that funnel. So the
  amendment is STRONGER than the critique states: ONE transfer hook inside
  `CopyDifferentialAndGroupIdsTo` covers CSE (Optimize.cpp:397 routes through
  RAUW), both Join self-canon sites, every other RAUW route, AND the ~12
  explicit mint-and-replace call sites that already call
  CopyDifferentialAndGroupIdsTo by hand (Join.cpp:279, Merge.cpp:647/865/888,
  Link.cpp:70/98/163/219, Connect.cpp:38/143, Induction.cpp:444). The
  annotation migrates exactly where group_ids migrate — the landed metadata
  precedent. FOLD: §A.1. Census abort softened per the amendment: hard-abort
  only where the reader consumes (under `-demand-instance`); flat compiles
  never abort on annotation loss (§A.1.4).
- F3 [MED] ACCEPT. VERIFIED: the CSE merge guard `if (v1 != v2 && v1->IsUsed()
  && ... Equals ...)` wraps the RAUW (Optimize.cpp:390-397). Moot in the main:
  with the F2 choke-point hook there is NO CSE-site hook; the transfer fires
  inside RAUW, which only runs post-guard. The already-annotated-survivor merge
  branch remains and needs D3 live coverage (folded into §A.1.3).
- F4 [MED] ACCEPT. VERIFIED: PlanNode's `bound_cols` is the ACCESS/GATE field;
  the FOLD leaf carries only fold_table/fold_sign/fold_class
  (lib/DeltaRel/DeltaRel.h:333-360). The insert-projection α-check as designed
  is a no-op. FOLD: §A.4 gives the fold's α-key a REAL representation
  (op-level `context_col_sources` parallel to context_cols) and moves the
  check there.
- F5 [LOW-MED] ACCEPT. Follows from F1. FOLD: census is order-free multiset
  counting only (the DeltaRel.cpp:2818-2851 E-27/E-28 discipline, verified).
- F6 [LOW] ACCEPT-AMENDED. The neighborhood witness populates nothing — TRUE —
  but the critique (and the lane) both miss that demand_tc_witness (COMMITTED,
  in-suite, recursive body under -demand) carries kReadAtTuple guard sites, so
  the D1.a stamp + census get LIVE non-trivial coverage from the existing
  suite with zero new cases. The α-validator's ABORT paths still first fire at
  D2.b (nested spines) — deferred there, per the critique's own alternative.
- F7 [LOW] ACCEPT. E-62 tripwire + PassPolicy orthogonality statements folded
  into every diff checklist (§B). The stamp is demand-mode-gated, not a pass;
  `-demand-instance` stays OFF the PassPolicy registry (§A.6.1).

§0.2 critique-ops (lane verdict SOUND-WITH-AMENDMENTS — UPHELD)
- F-OPS-1 [HIGH] ACCEPT. VERIFIED: emission is per-kind loops with no default
  (Stratum.cpp:1594-1614: the `dr_flow.GroupUpdates()` loop, the
  `op.kind != kClaimDrain → continue` filter loops). An op kind with no loop
  emits NOTHING and no validator objects. FOLD: §A.5 adds
  LowerSubgraphInstance + the eager-web hole-fill contract + V-INST-EMITTED
  (V-INGEST-XCHECK Site-5 multiset mold) as first-class D2.b deliverables.
- F-OPS-2 [HIGH] ACCEPT. VERIFIED: `DRFlowGraph::TableVec` asserts the vec
  exists (DeltaRel.cpp:595-601, two bare asserts); queue vecs are minted ONLY
  for `TableIsDifferential(table) && !induction-owned` (DeltaRel.cpp:878-893).
  The lane's unconditional queue-DEF registration SIGABRTs under its own
  recommended R-MONO-a slice. FOLD: §A.2.3 regime-splits the mint AND
  V-INST-EFFECT on `TableIsDifferential(pub_table)`.
- F-OPS-3 [MED] ACCEPT-AMENDED. VERIFIED: all four dep_edges.push_back sites
  live in the linearizer (DeltaRel.cpp:3555/3605/3615/3626); emit_waw forces
  edge direction to follow the band key (:3540-3556); the backward-edge
  catcher is V-LINEAR (:3845-3856), not V-BAND-HAZARD. FOLD: the mint-time
  DRDep is DROPPED; validator attribution corrected. AMENDMENT (see §0.4
  store-F-1 adjudication): the op_sign band-key ordering is RETAINED — it is
  sound under the joint condition the ops lane itself specifies (§1.8:
  op_table_id resolves to pub_table for BOTH ops, so key_less reaches the
  sign comparison, verified :3470-3477 `table_id` then `sign, − before +`).
  V-INST-ORDER is the enforcement; the key is the mechanism.
- F-OPS-4 [MED] ACCEPT-AMENDED. VERIFIED: `RecognizedSubgraphs` has ZERO
  matches in the tree; the live census recount is from query.Aggregates()/
  KVIndices() with the E-27 self-comparison prohibition in its own comment
  (DeltaRel.cpp:2818-2851). AMENDMENT (consolidator, X-9 §0.3): the recount
  source becomes `query.DemandForcings()` — which EXISTS
  (include/drlojekyll/DataFlow/Query.h:995, storage :1133, reachable in
  Program::Build at Build.cpp:1281) and is populated by the DataFlow demand
  pass, NOT by the DR mint loop — exactly the Aggregates() independence
  pattern. The census has real teeth on the FIRST landing; no downgrade.
- F-OPS-5 [MED] ACCEPT. VERIFIED: the linearizer effect switch ends
  `default: break;` (DeltaRel.cpp:3318-3320). FOLD: D1.b turns the default
  into ValidatorFail.
- F-OPS-6 [LOW] ACCEPT. VERIFIED: `TableIsMonotone` absent; use
  `!TableIsDifferential(t)`. H3/H4 hard-blocker status resolved by X-9
  (recognition = per-DemandForcing; excision owned by D2.b).
- F-OPS-7 [LOW] ACCEPT. The `return 0u` fallback is a known mold hole; §A.2.4
  upgrades the new kinds' case to ValidatorFail-on-missing-map-entry (cheap,
  and the mold's own comment begs for it).
- F-OPS-8 [LOW] ACCEPT. E-62 re-grep in every DeltaRel-touching diff (§B).

§0.3 X-9 (CONSOLIDATOR-MINTED, adjudicating a contradiction the cross critic
missed): lane-annot §4 populates RecognizedSubgraphs ONLY from
is_instance_key sites and states the neighborhood witness's list is EMPTY —
"the nested lowering does nothing on it." That contradicts ops/store/eqgate
AND the witness-target itself, all of which lower the neighborhood witness's
guarded subgraph as THE instance (store Key={Start}, RowT={Node}, replacing
flow_58). RESOLUTION (binding): the RECOGNITION UNIT is the DemandForcing —
one RecognizedSubgraph per entry of `query.DemandForcings()`
(BindingPattern-keyed, Query.h:957), key_cols = the forcing's bound α
positions, regardless of whether the demanded body is recursive.
is_instance_key sites mark recursive-subgoal boundaries (a D3 concern), not
the top-level instance. Consequences: (i) the neighborhood witness has
exactly ONE recognized subgraph (the D2.c gate is non-vacuous); (ii) the
census recount source is a real, independent, ALREADY-LANDED accessor
(discharges F-OPS-4/X-1's accessor half); (iii) annot's RecognizedSubgraph
struct survives as the per-forcing record, populated in ApplyDemandTransform.

§0.4 critique-store (lane verdict SOUND-WITH-AMENDMENTS — UPHELD)
- F-1 [MED] ACCEPT-THE-EVIDENCE, REJECT-THE-AMENDMENT. VERIFIED: the sign
  tie-break fires only when table_id is already equal (DeltaRel.cpp:3474-3475)
  and GROUP_UPDATE is one op. But the amendment ("drop the split, order ± inside
  one op") CONTRADICTS the binding §18(B) mandate (instance death is its OWN
  op) and ignores that ops §1.8 pins op_table_id = pub_table for BOTH ops —
  under which the sign tie-break DOES fire (the live op.3→op.4 claim-drain
  precedent in the probe). FOLD: §A.2.4 states the pin as the JOINT condition
  (same table_id) + table_op_sign ∓1 + V-INST-ORDER. The store-side halves the
  critique endorses (unconditional idempotent Recycle, intra-op two-scan
  order) are kept as the belt.
- F-2 [HIGH] ACCEPT. The birth-only regime vs the 2-arm V-INST-EFFECT totality
  is a real cross-lane SIGABRT. FOLD: §A.2.3 regime-split is BINDING, not
  optional; same fold as F-OPS-2.
- F-3 [MED] ACCEPT. FOLD: §A.3.5 pins "K adornments ⇒ K independent
  InstanceStores (per-forcing, mapping 1:1 to the BindingPattern-keyed
  registry)"; the single-Key store is explicitly the single-adornment
  specialization. Consistent with X-9 (per-forcing everything).
- F-4 [LOW] ACCEPT. VERIFIED: emit_seal is gated on region.SealStateCellId()
  (Database.cpp:2326-2327) and called at THREE exits (:2378, :2418, :2445).
  FOLD: §A.3.4 — new `SealInstanceStoreId()` region attribute +
  `emit_inst_seal()` lambda at all three sites.
- F-5 [LOW] ACCEPT. VERIFIED: rows/hashes/slots/slot_capacity are PRIVATE to
  RowStore (Table.h:169-216). FOLD: R-2 is `RowStore::Reset()` (Table inherits).

§0.5 critique-eqgate (lane verdict SOUND-WITH-AMENDMENTS — UPHELD)
- F1 [MED] ACCEPT. VERIFIED: the oracle golden is the full-relation dump
  (`reachable_from<TAB>1 1` …, goldens/demand_tc_witness.oracle.stdout) —
  format-disjoint from the driver stdout (`from 1: 2 3 4`). The flat==nested
  referee is the direct stdout cmp; the oracle is the orthogonal
  closure cross-check on the flat answer. §A.6.2 states it that way.
- F2 [MED] ACCEPT. The DEATH answer (empty after unask) is invisible to the
  closure oracle (edges survive). DEATH's only correctness anchor is the
  blessed .stdout + flat==nested mutual consistency. Stated LOUD in §A.6.4.
- F3 [HIGH] ACCEPT-AMENDED. VERIFIED: demand_tc_witness carries a T3 .irgold
  with four committed surfaces, and its .deltarel golden shows the demand
  message lowering MONOTONE (kIngestFold=2, census line 16). An @differential
  mint firing under bare `-demand` churns all four — confirmed hazard. BUT the
  critique's amendment (gate on `-demand-instance`) is ITSELF incomplete: the
  eqgate design shares ONE .main.cpp across both arms, and an `unask` entry
  point that exists only in the nested header fails to compile on the flat
  arm — the DEATH batch needs the retraction surface on BOTH arms.
  CONSOLIDATION (§A.6.3): the @differential fabrication + unask forcer are
  gated on a NEW opt-in flag `-demand-retract` (implies -demand), carried in
  the WITNESS's `.drflags` — so BOTH eqgate arms and all four golden modes of
  the new case see the same differential demand + unask ABI, and every
  existing case (incl. demand_tc_witness, plain `-demand`) is byte-untouched.
- F4 [MED] ACCEPT. Fence (i) restated per-forcing over DemandForcings()
  (§A.6.5) — consistent with X-9's per-forcing recognition unit.
- F5 [MED] ACCEPT. E-62 + 3-run determinism + debug==release as gates in §B.
- F6 [LOW] ACCEPT. VERIFIED: the all-modes-diagnostic list is a hardcoded
  alternation at runall.sh:296. §B/D2.c includes the mechanical edit + the
  CLAUDE.md prose update.
- F7 [LOW] ACCEPT. Flat==nested equivalence is the gate's ACCEPTANCE
  CRITERION, not a prediction (§B/D2.c states it as such).

§0.6 critique-cross (all upheld; disposition)
- X-1 [BLOCKER] RESOLVED by X-9 (accessor = DemandForcings-derived, landed
  pass populates it) + §A.5/§B-D2.b (excision explicitly owned).
- X-2 [BLOCKER] RESOLVED by the staged scope in §A.6.3/§C-1: D2 ships
  MONOTONE-FIRST (R-MONO-a, regime-split so nothing SIGABRTs), D3.a ships
  R-DIFF via `-demand-retract`+unask. The eqgate lane's a.ii is the ADOPTED
  stage-2, not dropped — its dissent (ship differential THIS epoch) recorded
  in §C-1.
- X-3 [HIGH] RESOLVED per §0.4 F-1 / §0.2 F-OPS-3: split kept (mandate),
  op_sign works under the same-table_id joint condition, mint DRDep dropped,
  V-INST-ORDER is the enforcement, V-LINEAR is the derived catcher's name.
- X-4 [MED] ACCEPT: ONE α-validator, name **V-ALPHA** (owned with the
  BindingSource tag, §A.4); V-ALPHA-RESOLUTION/-SOURCE/-KEYSLOT retired.
- X-5 [MED] ACCEPT: the dump-grammar extension is an explicit D1.b
  deliverable (spelling-table rows for 3 DROpKinds + 5 EffKinds, census
  counters, the instance op's p-rule) — WITH the honest consequence the
  cross critic did not draw: the census line renders every kind
  unconditionally (VERIFIED: `kGroupUpdate=0 kStateSeal=0` present at
  goldens/demand_tc_witness.deltarel.opt.golden:16), so adding enum members
  CHURNS that one committed golden line. §B/D1.b pre-registers this as a
  MECHANICAL single-line re-bless under the permcheck referee — "irgold
  zero-churn" is amended to "irgold zero-churn EXCEPT the census line of
  deltarel carriers, permcheck-refereed".
- X-6 [MED] ACCEPT: §A.5 (emission) + E-62 checklist rows in §B.
- X-7 [MED] ACCEPT: single-adornment specializations marked in §A.3.5/§A.6.5.
- X-8 [LOW] ACCEPT: same fold as annot F1/F2.

======================================================================
§A. THE ADJUDICATED DESIGN (pseudocode; every fold marked [FOLD:<finding>])
======================================================================

----------------------------------------------------------------------
§A.1 Annotation + survival (lane-annot, REBUILT per F1/F2/F3/F5/X-8/X-9)
----------------------------------------------------------------------
A1.1 The struct (unchanged from lane-annot §1.1 — fields ratified):
  struct GuardAnnotation {
    enum Kind : uint8_t { kReadAtTuple, kPushDown, kBaseAtom };
    enum DemandSide : uint8_t { kDReader, kRawSeed };
    enum Role : uint8_t { kBody, kQueryProjection };
    Kind kind; DemandSide demand_side; Role role;
    bool is_instance_key;                // marks RECURSIVE-subgoal sites (D3
                                         //   boundary), NOT the top instance [FOLD:X-9]
    std::vector<unsigned> instance_key;  // pivot positions within the guarded read
    QueryView guarded_read;              // OPAQUE handle; equality/lookup ONLY,
    QueryView demanded_view;             //   NEVER enters any order [FOLD:F1-annot]
    unsigned forcing_index;              // index into query.DemandForcings() —
                                         //   the per-forcing recognition link [FOLD:X-9]
  };
  Stamped at the TWO call sites (Demand.cpp step 7 :960 body/d_reader; step 8
  :1002 query-projection/raw_seed) — ratified; GT-3 (raw_seed CSE-folded into
  d_reader, live on the real witness) is WHY demand_side must be recorded at
  stamp time, pre-CSE. This survives all amendments.

A1.2 Storage [FOLD:F1-annot,F5-annot]: on QueryImpl, beside demand_forcings:
    std::vector<GuardAnnotation> guard_annotations;   // append order = the
        // deterministic site-loop order of ApplyDemandTransform; NEVER re-sorted.
    std::unordered_map<QueryViewImpl*, unsigned> guard_annotation_of;  // view ->
        // index into guard_annotations. LOOKUP/EQUALITY ONLY. Never iterated
        // into any ordered or emission-visible consumption. Empty flag-off.
  Ordered consumption (the D2.b spine builder, the census) walks
  DemandForcings()/the deterministic DefList view walk and LOOKS UP per view —
  the order comes from the walk, never from the map. No pointer-derived key is
  ever sorted. (F)-law satisfied by construction.

A1.3 Survival = ONE hook at the metadata choke point [FOLD:F2-annot,F3-annot]:
    // lib/DataFlow/View.cpp, inside CopyDifferentialAndGroupIdsTo(that) (:557):
    impl->TransferGuardAnnotation(this /*loser*/, that /*survivor*/);
  Coverage = exactly group_ids coverage: every RAUW route
  (ReplaceAllUsesWith :602 → SubstituteAllUsesWith :575 → :588), both JOIN
  self-canon sites (Join.cpp:158/424), CSE (Optimize.cpp:397, post-guard by
  construction), and the ~12 explicit mint-and-replace sites that already call
  CopyDifferentialAndGroupIdsTo by hand. Transfer semantics:
    loser unannotated               -> no-op.
    survivor unannotated            -> re-point guard_annotation_of[survivor]
                                       at the loser's entry; erase loser key.
    both annotated (guards folded)  -> assert compatible (kind, is_instance_key,
                                       instance_key, demanded_view,
                                       forcing_index; demand_side equal IN THE
                                       SLICE — the OD-annot-3 D3 relaxation
                                       point stands); count ++folded; keep
                                       survivor's entry. Incompatible -> LOUD
                                       abort (a demand bug folded two
                                       semantically distinct guards).
  The already-annotated branch first gets live coverage at D3 (multi-guard
  folds); until then the compatibility abort is its guard [FOLD:F3-annot].

A1.4 Census [FOLD:F5-annot,F2-annot]: order-free counts only —
  n_stamped == live(found via lookup during the deterministic inventory walk)
  + folded, and orphaned == 0. ABORT SEVERITY IS REGIME-SPLIT: under
  `-demand-instance` (a reader exists, a lost annotation = a silently skipped
  instance) the census is fprintf+abort always-on; under flat `-demand` (no
  reader) it is a debug-only assert — a legitimate flat compile is never
  killed by annotation loss the flat lowering doesn't consume.

A1.5 Recognition registry [FOLD:X-9, replaces lane-annot §4's population rule]:
    struct RecognizedSubgraph {          // ONE PER DemandForcing entry
      unsigned forcing_index;            // -> query.DemandForcings()[i]
      QueryView demanded_view;           // p_merge (opaque handle)
      std::vector<unsigned> key_cols;    // the forcing's bound α positions
      QueryView pub_view;                // the answer view (insert target)
      std::vector<unsigned> guard_annotation_indices;  // its guards
    };
    std::vector<RecognizedSubgraph> recognized_subgraphs;  // on QueryImpl;
        // populated in ApplyDemandTransform, append order = forcing order.
    Query::RecognizedSubgraphs() accessor mirrors DemandForcings() (:995).
  |recognized_subgraphs| == |DemandForcings()| on the slice (one adornment per
  name). The neighborhood witness: ONE entry, key {Start}. demand_tc_witness:
  ONE entry — live D1.a coverage from the existing suite [FOLD:F6-annot].
  Census independence: the recount source is populated by the DataFlow demand
  pass; the DR mint loop only reads it — the Aggregates() pattern
  (DeltaRel.cpp:2818-2851), NOT the E-27 tautology [FOLD:F-OPS-4].

----------------------------------------------------------------------
§A.2 The op family (lane-ops, amended per F-OPS-2/3/5/7, store-F-1/F-2)
----------------------------------------------------------------------
A2.1 THREE DROpKinds appended after kStateSeal (DeltaRel.h:139):
  kSubgraphInstantiate  — BIRTH/REBUILD (band-(a) plus arm) + band-(b)
                          publish_touched. Sole deriver of the pub table.
  kInstanceDeath        — whole-instance DEATH, its OWN op (§18(B) mandate):
                          drain demand net-removal, read retracted key, read
                          frozen, Recycle current→empty. NO fold, NO counter —
                          the zero-counter totality is the mandate's teeth.
                          MINTED ONLY when TableIsDifferential(demand_table)
                          (R-DIFF); compile-time-absent under R-MONO [FOLD:F-OPS-2].
  kInstanceSeal         — trailing pointer swap (kStateSeal peer, band 11).
  Publish lives on kSubgraphInstantiate (needs frozen AND current; a
  pure-death epoch still runs instantiate band-(b)-only) — ops OD-2 ratified.

A2.2 FIVE EffKinds appended after kStateOld (DeltaRel.h:85):
  kInstanceRebuild (writes store `current`; hazard keyed on pub_table; carries
  sign ±1, STRUCTURAL), kInstanceEmit (reads current, RAW after rebuild),
  kInstanceOld (reads frozen, no hazard), kInstanceDemand (frozen read of the
  drained demand KEY — distinct from the kVecDrain row read; a fold into
  kInIReadFrozen would blind the census to the F1 silent-drop), kInstanceSealSwap
  (sign 0, trails). Linearizer switch gains five cases AND the `default:`
  becomes ValidatorFail("unhandled EffKind") [FOLD:F-OPS-5].

A2.3 EFFECT MULTISETS — REGIME-SPLIT (binding; the X-2 SIGABRT fix)
  [FOLD:F-OPS-2,store-F-2]. Let DIFF := TableIsDifferential(pub_table).
  kSubgraphInstantiate:
    always:  1 kVecDrain(demand_table, kNetAddition)   // requires OD-7 frontier
             1 kInstanceDemand
             1 kFlagRead(input_table, kPresent, kSeed)  // Rederive leaf; the
                                                        //   rescan is the op BODY (PlanTree)
             1 kInstanceRebuild(+1)
             1 kInstanceEmit, 1 kInstanceOld            // band (b) partition reads
    DIFF:    2 kCounter(pub_table, ±, NonRecursive), 2 kInIReadFrozen,
             2 kVecAppend(pub_table, {kDeleteQueue,kAddQueue})
             + queue DEF-edge registration (the DR.cpp:744-747 mold)
    !DIFF:   1 kCounter(pub_table, +, NonRecursive); ZERO appends; NO TableVec
             call on pub queues (TableVec ASSERTS — DeltaRel.cpp:595-601).
  kInstanceDeath (R-DIFF only): 1 kVecDrain(demand_table, kNetRemoval),
    1 kInstanceDemand, 1 kInstanceOld, 1 kInstanceRebuild(-1); EXACTLY ZERO
    {emit, counter, crossing, append}.
  kInstanceSeal: exactly 1 kInstanceSealSwap (sign 0).
  V-INST-EFFECT asserts the regime-matched totality (both regimes first-class,
  never "downgrade"); a stray kCounter on a death op aborts.

A2.4 THE MINUS-BEFORE-PLUS PIN (H9/H10) — resolved [FOLD:F-OPS-3,store-F-1,X-3]:
  (i) death and instantiate are SEPARATE ops (mandate) BOTH keyed
      op_table_id = pub_table (op_table_id gains op.pub_table in its ?: chain)
      with death.table_op_sign=-1, instantiate.table_op_sign=+1: the band key
      {lead,stratum,band,table_id,sign,ctor} (DeltaRel.cpp:3470-3477) reaches
      the sign comparison BECAUSE table_id is equal, and sorts death first —
      the live op.3→op.4 claim-drain mechanism, probe-confirmed.
  (ii) NO mint-time DRDep (unprecedented; emit_waw direction is key-forced,
      :3540-3556, so "explicit agrees with derived" was circular). The derived
      WAW from the two kInstanceRebuild write hazards on pub_table exists but
      cannot catch a wrong key (V-LINEAR :3845-3856 checks derived edges
      against the same key that oriented them).
  (iii) V-INST-ORDER (NEW, always-on): per instance store id, pinned_order
      index of kInstanceDeath < kSubgraphInstantiate. THIS is the enforcement;
      it is mechanism-independent. Vacuous under R-MONO (no death op).
  (iv) belt at the store: minus-arm Recycle UNCONDITIONAL + idempotent; band-(b)
      two-scan order intra-op (store §1.4/§1.5) — the GROUP_UPDATE-style
      intra-op half the store critic endorsed.
  DROpStratum cases for the two live kinds ValidatorFail on a missing
  instance_stratum entry instead of `return 0u` [FOLD:F-OPS-7]. Bands: both
  live ops band 0; kInstanceSeal trails band 11 via key_of (after kStateSeal's
  10). Stratum lift = the GROUP_UPDATE seed+lift mold with
  ready_after(demand_table) and ready_after(input_table), strict (A5).

A2.5 Census + validators: recount `expect(kind, |query.RecognizedSubgraphs()|)`
  for instantiate and seal (and death under R-DIFF; 0 under R-MONO), key
  multiset (pub_table_ptr, provenance=kSubgraph, mode=Rederive, view id) —
  order-free, store id excluded (E-28). V-INST-SOLE (pub deriver = the one
  instantiate; input monotone = !TableIsDifferential, non-aliasing);
  V-INST-PAIR (2-way instantiate↔seal under R-MONO, 3-way under R-DIFF —
  regime is a per-flow fact); V-INST-ORDER (A2.4); V-INST-EMITTED (§A.5);
  V-INST-FRESH (runtime, §A.3.3). Scope honesty stands: the census checks the
  mint loop against the recognizer's list; recognizer correctness rests on
  the oracle-refereed witness + the -ir-out structural gate.

----------------------------------------------------------------------
§A.3 The store + Runtime items (lane-store, amended per store-F-3/4/5)
----------------------------------------------------------------------
A3.1 R-1: include/drlojekyll/Runtime/InstanceStore.h —
  `template<typename Key, typename RowT> class InstanceStore`, the
  StateCellStore transpose exactly as lane-store §1.1-§1.2 (dense iid
  namespace monotone-forever; keys/hashes/frozen/current/sealed_occupied/
  touched/touched_flag Vecs over one Allocator; open-addressing slots;
  FindOrAddInstance = FindOrAddGroup mold with MakeTable pair; Touch/Touched/
  KeyAt molds; RecycleCurrent; Seal = per-touched pointer swap + Reset +
  sealed_occupied snapshot; DebugValidate). working_count DROPPED
  (WorkingOccupied == current->NumRows()>0; band-(a) plus is monotone TryAdd).
  NESTED TABLES ARE INDEX-FREE (Find/TryAdd by full row; flat's idx_38 on the
  guarded copy DISAPPEARS — a real storage/index win). Counter seam: reuse
  StateCell's counter names under HYDE_RT_BENCH_COUNT; off-build byte-neutral.
A3.2 R-2: `RowStore<Row>::Reset()` [FOLD:store-F-5 — members are PRIVATE to
  RowStore, Table.h:169-216, so it is a RowStore member; Table inherits]:
  rows.Truncate(0); hashes.Truncate(0); slots[i]=kNoRow in place; sealed=0 at
  the Table layer (dissolved watermark). Calls NO allocator entry point —
  Arena-safe (Allocator::Free is a no-op; the slot loop is CompactRowsInPlace's
  own tail). DISCHARGES H6. Byte-neutral to the suite while uncalled.
A3.3 V-INST-FRESH: codegen-emitted inline guard at band-(a) entry per touched
  iid, BEFORE either arm, `current->NumRows()==0` else fprintf+abort;
  survives NDEBUG; unconditional (covers death-only epochs). Matched pair
  with the unconditional minus-arm Recycle.
A3.4 Seal wiring [FOLD:store-F-4]: NEW region attribute
  `SealInstanceStoreId()` + `emit_inst_seal()` lambda called at ALL THREE
  EmitCommitSweep exits (Database.cpp:2378, :2418, :2445) — a parallel of
  emit_seal/SealStateCellId (:2326-2327), NOT a reuse. Rides the monotone
  exit under R-MONO (pub monotone), moves to the differential exits
  automatically under R-DIFF.
A3.5 Band (b) = the pinned two-scan partition (H7, ratified): scan current
  (Find in frozen) for born {(F,T)}, scan frozen (Find in current) for dropped
  {(T,F)}; (T,T) emits nothing; no double-count; O(|frozen|+|current|).
  pub_row = concat(KeyAt(iid) at α positions, r at row positions).
  D3 NOTE [FOLD:store-F-3,X-7]: a relation demanded under K adornments gets K
  INDEPENDENT InstanceStores, one per forcing (per-forcing everything, X-9);
  the single-Key store is the single-adornment specialization, stated as such.

----------------------------------------------------------------------
§A.4 Binding-source + THE ONE α-validator V-ALPHA [FOLD:X-4,F4-annot]
----------------------------------------------------------------------
  PlanNode (ACCESS/GATE) gains `std::vector<BindingSource> bound_col_sources`
  parallel to bound_cols (DeltaRel.h:342), BindingSource ∈ {kRowSlot,
  kInstanceKeySlot, kConfigSlot}; defaults all-kRowSlot (all 169 corpus cases
  unaffected; validator short-circuits). The kSubgraphInstantiate op
  ADDITIONALLY carries `std::vector<BindingSource> context_col_sources`
  parallel to context_cols — the FOLD-side α representation the fold leaf
  lacks (bound_cols is ACCESS/GATE-only, verified DeltaRel.h:333-360)
  [FOLD:F4-annot]. codegen selector: kInstanceKeySlot → store.KeyAt(iid).c<k>;
  kRowSlot → r.<field>; kConfigSlot → the config_agg leading-param mold.
  Functor ABI stays closed (leading plain values; no handle ever passes).
  V-ALPHA (single, always-on, owned beside the tag; replaces
  V-ALPHA-RESOLUTION/-SOURCE/-KEYSLOT):
    arm A (ACCESS/GATE): a kInstanceKeySlot-tagged bound col must resolve as a
      kPointTest against the instance store, never a row-table scan/gate —
      covers join keys, negate gates (D3+), functor bound args.
    arm B (FOLD/publish): every context col of an instantiate op must have
      context_col_sources[k]==kInstanceKeySlot, i.e. the published α value is
      sourced from KeyAt(iid), never row-projected from the rescan — the
      elision and the wiring are ONE decision (§12 directive 2).
  Store cross-pin (P-4 store): Key_<id> column list == the forcing's key_cols
  == the annotation's instance_key positions; mismatch = V-ALPHA abort.

----------------------------------------------------------------------
§A.5 THE EMISSION PATH (new; the X-6/F-OPS-1 fill — no lane owned it)
----------------------------------------------------------------------
  (a) LowerSubgraphInstance in Stratum.cpp: a per-stratum dispatch loop over
      the instance ops (the `dr_flow.GroupUpdates()` loop mold,
      Stratum.cpp:1594-1603), emitting: the band-(a) drain of the demand
      net-addition frontier → FindOrAddInstance → V-INST-FRESH guard →
      (R-DIFF: death arm first, per the A2.4 order) → the Rederive rescan of
      the monotone input from the op BODY PlanTree → TryAdd into current →
      band (b) two-scan publish into the pub table (regime-split counters/
      queues per A2.3) — then the seal rides the commit-sweep tail (A3.4).
  (b) The eager-web hole contract: the recognizer's chain-breaker registration
      (SuffixesOf/CollectSectionTargetsDR + the Build.cpp eager walk) STOPS
      the eager descent at the recognized subgraph view (the GROUP_UPDATE
      precedent), and the demand receive's ingest fold cursor gets the
      instance machinery as its successor instead of the flat guard joins —
      the flat join-tables regions (flow_58) are NOT emitted under
      `-demand-instance`. This is GT-5 honored: SUBGRAPH_INSTANTIATE replaces
      EAGER-WEB emission, not DR ops.
  (c) V-INST-EMITTED (always-on): multiset-compare every emitted instance
      region against the flow's kSubgraphInstantiate/kInstanceDeath
      enrollment (the V-INGEST-XCHECK Site-5 mold) — a kind with no lowering
      loop ABORTS instead of silently dropping the answer derivation
      [FOLD:F-OPS-1]. This is the F17/F18-class guard for the new family.

----------------------------------------------------------------------
§A.6 Equivalence gate, retraction surface, fences (lane-eqgate, amended)
----------------------------------------------------------------------
A6.1 Knobs: `-demand-instance` (implies -demand; lowering selector; OFF the
  PassPolicy registry, mirroring the P1 "demand is semantics" pin) — ratified.
  NEW [FOLD:eqgate-F3 amended]: `-demand-retract` (implies -demand;
  orthogonal to -demand-instance): gates the @differential fabrication at the
  Parse/Demand.cpp mint (:185-204, today plain kMessage — the verified root
  lever) AND the `unask_<q>_<α>` forcer-symmetric public entry point (the
  suppressed-message-preserving a.ii shape; the demand message itself STAYS
  suppressed, IsDemandMessage discipline intact). Carried by a case's
  .drflags, so BOTH eqgate arms and all four golden modes of a retract-bearing
  case share one retraction surface; every existing case is byte-untouched.
A6.2 The gate (run_eqgate in runall.sh's --one worker, sidecar
  `cases/<name>.eqgate`, E-58/E-65 layout, tokens FLAT-NESTED-DIVERGE /
  *-GOLDEN-DIVERGE / EQGATE-*-FAIL matching the :350 grep): flat arm =
  .drflags as-is; nested arm = + `-demand-instance`. REFEREE = direct driver
  stdout cmp: flat == nested == goldens/<name>.stdout [FOLD:eqgate-F1]. The
  oracle (never runs demand, Oracle Main.cpp:740) is the ORTHOGONAL
  closure cross-check on the flat answer via the standing run_oracle arm.
  Answer-identity, never generated-code bytes. Flat==nested is the gate's
  ACCEPTANCE CRITERION (what D2.b must achieve), not a prediction
  [FOLD:eqgate-F7].
A6.3 STAGING (the X-2 resolution, binding):
  STAGE 1 (D2): R-MONO-a. Witness .drflags = `-demand`. Gate covers BIRTH +
    REBUILD-via-demand-flap (the driver re-probes after an edge add). Death
    op not minted; the .deltarel census shows kSubgraphInstantiate=1
    kInstanceSeal=1 kInstanceDeath=0. Requires OD-7 (net-additions frontier
    provisioning on the monotone demand table — the GROUP_UPDATE
    cut-successor precedent) or the birth arm has no drain vec.
  STAGE 2 (D3.a): R-DIFF. Witness .drflags gains `-demand-retract`; driver
    gains the unask/DEATH batch; TableIsDifferential gains the
    recognized-subgraph arm; the death op mints and fires; the phantom-pair /
    same-epoch-flap adversarials enter ONLY here (they are blocked on the
    A2.4 pin being live and V-INST-ORDER green). DEATH's correctness anchor =
    blessed .stdout + flat==nested mutual consistency; the closure oracle
    CANNOT see demand scoping [FOLD:eqgate-F2] — stated loud, forever.
A6.4 The witness plan: demand_neighborhood_witness enters cases/ at D2.c with
  .batches (add_edge only — demand is driver-injected; the suppressed message
  is unreachable from batches by design), .stdout/.oracle.stdout/
  .monotone.stdout goldens (blessed, never on red), .eqgate sidecar, driver in
  the demand_tc_witness mold (probe = query call = demand inject; sorted
  keyed drains).
A6.5 The three fences (all in Program::Build's pre-pass, the Build.cpp:1256
  mold, iterating PER FORCING over query.DemandForcings() [FOLD:eqgate-F4]):
  (i) cyclic-demand: DemandSelfReachableThroughInstance per forcing's demand
      relation (ViewSelfReachable restricted through the instance boundary;
      keyed by the forcing's bound columns, NEVER the CSE-folded d_reader
      identity). Allows recursive CONTENT (bf-tc). Witness demand_cyclic_1.
  (ii) mid-stream monotone-input-add under standing demand (§0.6.6): ii-strict
      compile fence + the witness driver uses the demand-flap rebuild so a
      legal program never trips it. NOT a deletion fence — the hazard axis is
      structural (demand-triggered-only rebuild + live-updatable input); §13
      honored. Witness demand_midstream_edge_1.
  (iii) differential-summarized-input (A5): CanReceiveDeletions() on the
      summarized input → reject. Witness demand_diff_input_1.
  All three witnesses: all-4-modes-diagnostic under `-demand -demand-instance`
  via .drflags; REGISTERED in the runall.sh:296 alternation + CLAUDE.md's
  authoritative list [FOLD:eqgate-F6]. The three fences stay SEPARATE (only
  iii is deletion-keyed); collapsing them is the C-4 over-broadness.

----------------------------------------------------------------------
§A.7 Dump-grammar extension (the X-5 deliverable)
----------------------------------------------------------------------
  Spelling-table rows for the 3 DROpKinds + 5 EffKinds (loud-abort fallback
  discipline, T2b law); census counters kSubgraphInstantiate= kInstanceDeath=
  kInstanceSeal= appended to the census line; a p-rule for the instance op's
  rendered spine incl. the binding-source tags (`pub_row=[ik:Start,row:Node]`,
  `nested=<Node>`) and the DRInstance descriptor block. KNOWN CONSEQUENCE
  (pre-registered): the census line of goldens/demand_tc_witness.deltarel.opt.
  golden (line 16, renders every kind unconditionally — verified) churns by
  exactly the three appended `=0` counters at D1.b. This is a MECHANICAL
  single-line re-bless under the permcheck referee; every other golden byte
  is unchanged. "irgold zero-churn" is amended accordingly for D1.b only.

======================================================================
§B. THE D1→D2 DIFF SEQUENCE (numbered; each with files, PRE-REGISTERED
    predictions, standing-gate checklist)
======================================================================
Standing-gate vocabulary per diff: [G1] SUITE: PASS (169/…) with all sidecar
arms live; [G2] 676-row corpus A/B byte-identical vs frozen e6264b54 knob-off;
[G3] irgold byte-compare on both carriers; [G4] Q5 ABABAB bench neutral;
[G5] 3-run determinism + debug==release on all four surfaces of every
demand-ON carrier (config-invariance; mandatory on dump-touching diffs);
[G6] E-62 tripwire re-grep (body_ops/output_ops/pinned_order consumers) on
any DeltaRel-touching diff; [G7] data/ examples clean; ctest 3/3.

--- D1.a — ANNOTATION + RECOGNITION REGISTRY (DataFlow-side, inert) ---
Files: include/drlojekyll/DataFlow/Query.h (GuardAnnotation,
  RecognizedSubgraph, storage + accessors), lib/DataFlow/Demand.cpp (stamp at
  :960/:1002; populate recognized_subgraphs per forcing), lib/DataFlow/View.cpp
  (TransferGuardAnnotation inside CopyDifferentialAndGroupIdsTo :557),
  lib/DataFlow/Query.cpp (accessor).
Predictions (pre-registered):
  P-D1a.1 Flag-off: guard_annotations/recognized_subgraphs EMPTY; [G2] holds
    byte-for-byte; suite 169 [G1].
  P-D1a.2 demand_tc_witness live: n_stamped>0 incl. kReadAtTuple sites;
    |recognized_subgraphs|==|DemandForcings()|==1; zero orphans; the census
    (debug-severity under flat, §A.1.4) is green on a real recursive compile.
  P-D1a.3 demand_neighborhood_witness (uncommitted; probe-compile): 2 stamps
    (join.6 kBaseAtom body, join.7 query-projection), both survive distinct
    (GT-3 folds only the demand TUPLE), 1 recognized subgraph, key {Start}.
  P-D1a.4 [G3] zero-churn EXACT (no dump touched); [G5] 1-hash; [G4] neutral.
Checklist: [G1][G2][G3][G4][G5][G7]; PassPolicy untouched (stamp is
  demand-mode-gated, not a pass); no Runtime edit.

--- D1.b — DR-IR OP FAMILY + EFFKINDS + VALIDATORS + DUMP GRAMMAR (DeltaRel-
    side; enums/validators land, MINT GATED OFF until D2.b's knob) ---
Files: lib/DeltaRel/DeltaRel.h (3 DROpKinds, 5 EffKinds, DROp fields,
  DRInstance, instance_stratum, accessors), lib/DeltaRel/DeltaRel.cpp
  (BuildSubgraphInstanceOps behind the -demand-instance gate; linearizer
  cases + default→ValidatorFail; DROpStratum/op_band/op_table_id/key_of
  cases; DeriveDRStrata seed+lift; census recount from
  query.RecognizedSubgraphs(); V-INST-EFFECT/SOLE/PAIR/ORDER),
  lib/DeltaRel/Format.cpp (spelling rows, census counters, instance p-rule).
Predictions:
  P-D1b.1 Knob-off (i.e. everywhere): zero instance ops minted; suite 169
    [G1]; [G2] byte-identical.
  P-D1b.2 THE ONE EXPECTED CHURN: demand_tc_witness.deltarel.opt.golden's
    census line gains ` kSubgraphInstantiate=0 kInstanceDeath=0
    kInstanceSeal=0` — a single-line mechanical bless, permcheck-refereed;
    NO other golden byte changes (§A.7). [G3] amended exactly so.
  P-D1b.3 The linearizer default-abort trips nothing on the corpus (all
    existing EffKinds have cases).
Checklist: [G1][G2][G3-amended][G4][G5 on both irgold carriers][G6 — E-62
  re-grep MANDATORY: new kinds flow through pinned_order][G7]; permcheck run
  on the blessed census line; config-invariance direct (debug==release dumps).

--- D2.a — RUNTIME STORE (inert until D2.b) ---
Files: include/drlojekyll/Runtime/InstanceStore.h (NEW, R-1; Peter Goodman
  copyright line only), include/drlojekyll/Runtime/Table.h (R-2
  RowStore::Reset per §A.3.2).
Predictions:
  P-D2a.1 Suite 169 green with both items UNREFERENCED; zero generated-code
    byte change [G2]; bench-counter seam off-build byte-neutral (no new
    counter names).
  P-D2a.2 A DrTest unit (ctest) exercises FindOrAddInstance/Touch/Seal/Reset
    incl. the Arena allocator (H6 regression) and the same-epoch flap
    (Recycle idempotence) — the store's death half gets its ONLY pre-D3
    execution here; ship the test with the header.
Checklist: [G1][G2][G4][G7]; no DeltaRel touch (no E-62); ctest grows by one.

--- D2.b — RECOGNIZER EXCISION + LOWERING + CODEGEN (the knob goes live) ---
Files: bin/drlojekyll/Main.cpp (-demand-instance), lib/DataFlow/Build.cpp /
  Demand.cpp (knob threading), lib/DeltaRel/DeltaRel.cpp (mint un-gated under
  knob; bound_col_sources/context_col_sources tagging; V-ALPHA),
  lib/ControlFlow/Build/Stratum.cpp (LowerSubgraphInstance + dispatch loop;
  V-INST-EMITTED), lib/ControlFlow/Build/Build.cpp (chain-breaker stop; the
  eager-web hole contract; fences i/ii/iii per §A.6.5; OD-7 net-additions
  provisioning for the monotone demand table), lib/CodeGen/CPlusPlus/
  Database.cpp (instance_<id> member/ctor/ref-params; SealInstanceStoreId +
  emit_inst_seal at :2378/:2418/:2445; V-INST-FRESH inline guard),
  lib/ControlFlow/Program.h (region attr).
Predictions:
  P-D2b.1 Knob-off: [G2] byte-identical everywhere; suite (169+3 fence
    witnesses if OD-6 lands here) green; [G3] zero-churn (D1.b's line already
    blessed).
  P-D2b.2 Knob-on (neighborhood witness, probe): flat vs nested generated
    code — %table:15 (Row15) + idx_38 (Key38) VANISH from the touched proc
    signatures; instance_<id> appears exactly where flow_58 lived; idx_57 +
    edge's idx_32 unchanged (store P-2/P-5, ratified).
  P-D2b.3 Nested .deltarel census: kSubgraphInstantiate=1 kInstanceSeal=1
    kInstanceDeath=0 (R-MONO-a); V-INST-* and V-ALPHA all green; V-INST-EMITTED
    proves consumption.
  P-D2b.4 ACCEPTANCE CRITERION (not prediction): nested driver stdout ==
    flat driver stdout on BIRTH/REBUILD probes.
Checklist: [G1][G2][G3][G4][G5 on the nested witness's four surfaces —
  BEFORE any nested golden is blessed][G6][G7]; fences' diagnostics fire on
  their witnesses in all 4 modes.

--- D2.c — WITNESS + EQUIVALENCE GATE ENTERS THE SUITE ---
Files: tests/OptDiff/cases/demand_neighborhood_witness.{dr,main.cpp,batches,
  drflags(-demand),eqgate} (+ the three fence cases if OD-6), tests/OptDiff/
  goldens/ (+3: .stdout/.oracle.stdout/.monotone.stdout, blessed from green),
  tests/OptDiff/runall.sh (run_eqgate in --one; :296 alternation gains the
  fence names), CLAUDE.md (case count, diagnostic list, eqgate arm).
Predictions:
  P-D2c.1 Suite 169→170 (→173 with fences); all four golden modes of the new
    case byte-agree (the standing cross-mode law).
  P-D2c.2 run_eqgate STAGE-1 green: flat==nested==.stdout on BIRTH +
    flap-REBUILD; oracle golden = full-closure dump, one golden for both arms.
  P-D2c.3 [G2] untouched (new case only); [G3] zero-churn.
Checklist: [G1 at the new count][G2][G3][G4][G7]; bless ritual (never on red);
  cursor-contract review on the new driver (sorted keyed drains).

--- D3.a — R-DIFF: THE RETRACTION SURFACE + DEATH GOES LIVE (stage 2; next
    epoch, pre-registered here so nothing forecloses it) ---
Files: lib/Parse/Demand.cpp (@differential fabrication under -demand-retract),
  bin/drlojekyll/Main.cpp (-demand-retract), lib/DataFlow/Demand.cpp (unask
  forcer half of QueryDemandForcing), lib/ControlFlow/Build/Build.cpp
  (TableIsDifferential recognized-subgraph arm), lib/CodeGen (unask entry,
  kMessageHandler suppression intact), witness .drflags/-driver (DEATH batch),
  goldens (re-blessed: the flat witness flips differential too — its four
  golden modes re-bless together; demand_tc_witness UNTOUCHED, plain -demand).
Predictions: the death op mints (census kInstanceDeath=1), V-INST-ORDER live,
  net-removal frontier exists (the probe's op.5 shape), unask(5) publishes
  −(5,·) on BOTH arms, %table:4 ends empty on both; phantom-pair adversarial
  enters the gate ONLY here. demand_tc_witness byte-stable [G3].

======================================================================
§C. OPEN OWNER DECISIONS (numbered; fleet recommendation + dissent)
======================================================================
1. THE RETRACTION SURFACE (GT-1; decision #1). RECOMMENDATION (consolidated,
   = ops OD-1(a)+eqgate a.ii STAGED): D2 ships R-MONO-a birth/rebuild-only
   (death op designed, minted only when demand is differential — never fires
   this epoch); D3.a ships R-DIFF via the NEW `-demand-retract` opt-in flag
   (@differential fabrication + suppressed-message-preserving unask ABI)
   carried by the witness's .drflags so BOTH gate arms share one retraction
   surface and nothing else churns. DISSENT (eqgate lane): ship a.ii THIS
   epoch so the DEATH arm is real immediately; cost = witness flips fully
   differential now and D2 scope grows. REJECTED options: un-suppressing the
   demand message (breaks the no-driver-callable-seam invariant); a test-only
   unsuppression flag (env-gated-scaffolding anti-pattern; oracle can't see it).
2. Recognition unit = per-DemandForcing (X-9): RecognizedSubgraphs derived
   1:1 from the landed BindingPattern-keyed registry; is_instance_key marks
   recursive-subgoal (D3) boundaries only. RECOMMEND ratify — it un-blocks
   the census recount and makes the gate non-vacuous. (No dissent; it
   supersedes lane-annot §4's is_instance_key-only population, which would
   have made the nested lowering a no-op on the PICK-A witness.)
3. Knob spelling: `-demand-instance` (implies -demand), OFF the PassPolicy
   registry; `-demand-retract` likewise (both are semantics/lowering
   selectors, the P1 ruling's shape). RECOMMEND yes. (Name of the retract
   flag is the owner's to bikeshed.)
4. Three-op family with publish-on-instantiate (ops OD-2) + the five-member
   instance:* EffKind family (ops OD-4). RECOMMEND confirm both.
5. Minus-before-plus pin = same-table_id band key (table_op_sign ∓1) +
   V-INST-ORDER; NO mint-time DRDep (X-3 resolution). RECOMMEND confirm.
6. OD-7 (was ops OD-5): provision the monotone demand table a net-additions
   frontier (the GROUP_UPDATE cut-successor precedent) — REQUIRED for the
   birth arm to see new keys under R-MONO-a. RECOMMEND yes; without it D2.b
   cannot function.
7. Fence (ii) axis: ii-strict compile fence (per §0.6.6) + demand-flap driver
   in the witness (witness-target option-2); they compose. RECOMMEND yes.
8. Fence witnesses land WITH D2.c (suite →173) vs follow-on. RECOMMEND with
   D2.c — a fence with no red witness is untested.
9. D1.b census-line churn: bless the mechanical single-line change on
   demand_tc_witness.deltarel.opt.golden under permcheck (§A.7). RECOMMEND
   yes (the alternative — conditional census rendering — breaks the census-
   sum cross-check discipline).
10. Nested .deltarel/.irgold pin for the witness: as a D2.c follow-on once
   the nested lowering is stable (eqgate OD-5). RECOMMEND yes, follow-on.

======================================================================
§D. RESIDUAL HOLES (LOUD, one line each)
======================================================================
- H5 (D4 demand-seed self-pump / implicit-asynchrony death seam): UNTOUCHED by
  the whole fleet; D3.a's unask is the manual surface, the self-pump is not designed.
- The eager-web hole-fill contract (§A.5b) is designed at CONTRACT level only —
  the exact Build.cpp descent cut + successor wiring (the E-42 class) has no
  pseudocode anywhere; first real risk in D2.b.
- OD-7 net-additions provisioning for a NON-chain-breaker-input monotone
  receive is asserted by precedent (GROUP_UPDATE) but unverified for the
  demand table's ingest-fold shape — probe it before D2.b lands.
- V-ALPHA arm B rests on the NEW context_col_sources field (§A.4) — invented
  here to fix F4-annot; no live precedent; needs its own dump rendering row.
- The unask forcer / @differential fabrication (D3.a) is outline-only: the
  QueryDemandForcing unask half, the parse-level attribute injection, and the
  flat differential demand lowering have no committed precedent (the probe
  covers the message-level shape only).
- D3 multi-adornment: K stores per relation, per-forcing fences, demand_side
  fold relaxation — all NOTED as specializations, none designed (tc-four-
  adornment-target §3-§5 interactions preserved, not closed).
- DEATH has NO independent oracle forever (closure oracle can't see demand
  scoping, eqgate-F2) — permanent scope note on the equivalence gate.
- The census recount is independent-of-mint but NOT independent-of-recognizer
  (E-27 honesty, one level up): recognizer correctness rests on the oracle +
  -ir-out structural gate, as pinned.
- The already-annotated-survivor transfer branch (§A.1.3) has zero live
  coverage until D3 multi-guard folds; guarded by the compatibility abort.
- witness-deltarel-target.md §2.6's draft nested dump + its %table numbering
  are STALE vs GT-4 and vs this record's regime split; it needs a
  SUPERSEDED-IN-PART banner when this record is adopted.


