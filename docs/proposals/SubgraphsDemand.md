# Subgraphs / demand epoch — design ledger

Status: OPEN. Opened 2026-07-16 on branch `subgraphs-demand` off main
0a4a9225 (the ADL/functor-surface epoch merged + its §13 deviations
ratified). This file is the epoch's working ledger, in the
ADLFunctorSurface.md mold: the PerfRoadmap §14.2/§14.3 seed's
re-verification record (errata continue at E-27), the diff plan as
amended, the hand-written target artifacts index, and — as they land —
the per-diff records. The landing record goes to PerfRoadmap §15 at
close. Owner theme: PerfRoadmap §12.0(b) SUBGRAPHS/DEMAND, seeded as
§14 (recommended cut (c) config-column aggregates → (b) keyed nested
instances → (a) demand transform, with P0 = the GROUP_UPDATE census gap
and the §6 monotone/descent stage as early work — owner ratifies the
cut at the design checkpoint).

## 0. Epoch-start baseline (2026-07-16, branch tip == main 0a4a9225)

- debug + release builds green (cmake presets, C++23).
- ctest 3/3 (runtime_test included; 59.8s wall).
- FULL SUITE: PASS (164 cases), zero churn — the epoch-start baseline.
- Q5 spot @128 (3-rep, idle machine): release/opt 0.11-0.12s, debug/opt
  0.94s — byte-flat against the run-9 close numbers (0.11-0.12 /
  0.87-0.95). Regressions >10% stop the line; re-spotted per-diff.
- Witness artifacts generated at baseline (session scratchpad
  witness/): average_weight datalog.h (945 lines) + aw.ir (344),
  cf16_4.ir (39) + cf16_4.nocf.ir (50) — the checkpoint-1 fleet's
  emitted-artifact inputs.

## 1. Seed re-verification record (§14.2/§14.3 vs HEAD; 2026-07-16)

Fleet: 5 opus derivation lanes (groupupdate-statecell / ingest / census
/ dataflow / emitted-artifacts), each re-deriving pseudocode FROM CODE
(anchors read this session, seed unread) before an adversarial opus
verifier checked every §14.0/§14.0.2/§14.2/§14.3 claim against BOTH the
lane report and the code (10 agents, ~848k tokens; 96 claims checked,
~74 CONFIRMED). Lane reports: session scratchpad fleet2-lane-*.txt.
THE PRECEDENT HELD A SEVENTH TIME — the P0 diff sketch itself contains
a real defect (E-27), and the verify pass additionally caught one lane
report's own error (the artifacts lane misread the deep_chain_retract
interior-fold witness as falsified; the verifier re-derived the IR and
proved the SEED right — the net-additions append does sit at the
interior %table:7 TUPLE fold; 'interior/ancestor fold' means a
non-receive-VIEW fold, textually still inside the entry proc).

### Seed errata (E-27.. continuing the house numbering)

- E-27 (REAL-DEFECT, the P0 sketch): §14.3-P0's
  `exp_state_seal = count of statecells` is a flow-derived TAUTOLOGY,
  not the independent recount the census contract demands
  (DR.cpp:2646-2652). flow.statecells is pushed exactly once per
  BuildGroupUpdateOps call (DR.cpp:667/674, sole push site) in lockstep
  with the kGroupUpdate/kStateSeal mints — expect(kStateSeal,
  flow.statecells.size()) compares flow against itself and can never
  fire. BOTH recounts must derive from the QUERY:
  exp_group_update = exp_state_seal = |query.Aggregates()| +
  |query.KVIndices()| (the mint loop's own accessors, DR.cpp:1013/1038).
  PROVABLY TIGHT: the mint loops have no skip/continue (only aborting
  asserts), BuildGroupUpdateOps' only caller is DR.cpp:1034/1048, and
  the Build.cpp:1162-1164 num_errors→nullopt fence runs BEFORE impl
  exists, so no unsupported agg/kv (induction-owned :1099/:1123, config
  :1108, undeclared-KV-algebra :1135) coexists with a running census.
- E-28 (REAL-DEFECT, the P0 key): §14.3-P0's per-op key multiset lists
  `statecell_id` as "recomputed from the Query" — it is a MINT-ORDER
  artifact (DR.cpp:667 = flow.statecells.size() at mint time), not
  independently recomputable; including it breaks the order-free
  multiset discipline the IngestKey precedent (DR.cpp:2764-2786) sets.
  Sound independent key: (agg_table*, provenance, algebra, agg-view
  identity). statecell_id consistency (bijection with kStateSeal) is a
  flow-internal STRUCTURAL validator, not census.
- E-29 (STALE-ANCHOR, ×2 sites, all five verifiers): "a plain assert
  DR.cpp:629" (§14.0.1 + §14.3-P0 "promote-the-assert-DR.cpp:629") —
  :629 is a blank line. The family's real asserts: DR.cpp:663-665
  (agg_table/input_table non-null; agg_table != input_table =
  V-AGG-SOLE non-aliasing) and :718 (klass == kNonRecursive acyclic
  fence) — several, all plain assert() (NDEBUG-stripped). P0's
  promotion targets that cluster.
- E-30 (STALE/INCOMPLETE anchors): the census expect() list is NINE
  kinds spanning DR.cpp:2810-2818 (seed lists five at "2810-2813",
  omitting kRetire/kRederive/kFrontierFilter/kCommitSweep; kIngestFold
  is at :2818); the P0 diff anchor "2646-2813" extends to ~2850 (key
  multisets); the kStateSeal OP mints at DR.cpp:748 (:754 is its
  kStateFold effect).
- E-31 (REAL-DEFECT, P2 stage (c) UNDER-SCOPED): the (group ++ config)
  KEY projection is genuinely threaded end-to-end (DR.cpp:1019-1025 →
  cell.key_cols → Key_<id> Database.cpp:1084-1090 → agg-row :2029-2031)
  — config-as-PARTITION-KEY needs only the Build.cpp:1108 fence lifted
  + a corpus case. BUT the REDUCTION ABI never receives config:
  EmitGroupUpdate folds summary positions only (Database.cpp:2007-2015);
  StateCell Fold/Combine/ReduceLive (StateCell.h:117-119/181-198) see
  only Summary. A config-DEPENDENT reduction (the natural case — a
  bound Threshold gating the fold) requires extending the C-5 free-
  function ABI (f_combine/f_uncombine/f_reduce gain the config value).
  Stage (c) scope must fork: (i) config-as-key = fence + corpus;
  (ii) config-dependent reduction = ABI extension; the corpus case MUST
  exercise a config-dependent reduction or the gap ships un-caught.
  (Also: the in-code comment Build.cpp:1106-1107 "not yet threaded
  through the state-cell key projection" is itself stale — fix when the
  fence lifts.) Verified empirically: a config aggregate parses +
  passes dataflow; only the ControlFlow :1108 reject fires.
- E-32 (REAL-DEFECT, P3 obligation (b) mis-framed): group_ids CANNOT
  keep demand-guarded copies distinct — InsertSetsOverlap returns
  MERGEABLE when either set is empty (View.cpp:1478-1480) and group_ids
  seed only on JOIN/AGG/KVINDEX views (Optimize.cpp:423), so a demand
  copy feeding none of those has empty group_ids and CSE WOULD merge it
  with its unguarded twin. The transform must make copies STRUCTURALLY
  distinct via a demand/magic INPUT EDGE (the ApplyPositiveConditionTest
  ⊥c pivot precedent, Build.cpp:1450-1512, keep-last-edge-protected at
  Join.cpp:314-322) — Equals() fails on arity/children before group_ids
  logic runs. Relying on group_ids for demand-copy distinctness is
  UNSOUND; obligation (b) restated accordingly.
- E-33 (NUANCE, P3 obligations incomplete): add to (a)-(d):
  (e) no zero-pivot JOIN except under @product (a demand join on no
  shared columns violates a hard structural invariant); (f) every
  introduced demand relation has a real producing source (else
  dead-flow elimination collapses it / a source-less cycle forms); and
  sharpen (a): the IdentifyInductions/merge_set cross-check
  (Stratify.cpp:387-419) ABORTS in debug if a demand copy's SCC
  induction structure diverges from the union-find partition.
- E-34 (NUANCE, P1 diff under-specified): the §6 hole-return shape is
  implementable but is NOT a bare LowerIngestFold signature change.
  Today LowerIngestFold is invoked ONLY on deletion-capable ops
  (Procedure.cpp:52, then `continue` :55 — no descent) whose fold body
  the VECTORAPPEND already fills (Stratum.cpp:1919-1926), and it
  ASSERTS op.ingest_stage1 && op.ingest_is_explicit (Stratum.cpp:1887);
  monotone ops are stage1=false/is_explicit=false (DR.cpp:1808-1809)
  and routed through the hand-coded arm (Procedure.cpp:58-92). P1 must
  (i) relax the asserts, (ii) move the monotone routing into
  LowerIngestFold, (iii) return the UPDATECOUNT OP* as next_parent —
  exactly the pattern the hand-coded arm already uses (Procedure.cpp:
  77/91: next_parent=insert → BuildEagerInsertionRegions).

### Verified facts the epoch stands on (fleet-confirmed)

- The §14.2(C) seven-point plug-in checklist anchors are ALL exact
  (BuildGroupUpdateOps :638, calls :1034/:1048, mint :676, effects
  :699/:709/:713, seal :748; DeriveDRStrata :2058 + :2090-2094;
  LinearizeAndValidateDRFlow :2963 + :3543-3555; LowerGroupUpdate
  Stratum.cpp:1363 + dispatch :1588-1596; EmitGroupUpdate
  Database.cpp:1970 / EmitStateCellStructs :1076 / header :2967;
  ProgramGroupUpdateRegionImpl Program.h:1067; C-5 decl :1271).
- StateCell.h structure as seeded: two-word cell (:26-36), occupancy
  bit + working member count (:38-54, the batch-1 abort fix, birth/
  death/change guard :50-54), non-aliasing (:10-24), FindOrAddGroup
  mirrors RowStore::FindOrAdd, kNoGroup=~0u.
- group_cols = InputGroupColumns ++ InputConfigurationColumns is LIVE
  (DR.cpp:1019-1025; DR.h:537 comment verbatim).
- The no-operator-hook grep (18 incidental prose hits) reproduces.
- deep_chain_retract IS a valid interior-fold witness for §6 (the
  verifier re-derived it against a lane error; append at the interior
  same-table TUPLE fold, claimed by the DESCENT, inside ^entry).
- average_weight emits exactly 3 StateCellStores (Invertible<Reduce_0/
  Reduce_1>, Recompute<Reduce_2>), matching the emitted-artifact lane.

## 2. Implementation record (one diff at a time)

### P0 — GROUP_UPDATE/STATE_SEAL census LANDED (2026-07-16)

The E-25 carried gap is CLOSED, as corrected by the §1 errata. DR.cpp
(+139/−2), ValidateDROps only — observation-only, zero emission change:

- CENSUS (E-27 honored): exp_group_update = exp_state_seal =
  |query.Aggregates()| + |query.KVIndices()| — the mint loop's own
  discovery inputs, recounted independently, NEVER flow-derived (the
  seed's `count of statecells` tautology rejected). Two new expect()
  entries after kIngestFold.
- KEY MULTISET (E-28 honored): per-op (agg_table*, provenance, algebra,
  view UniqueId) recomputed from the Query (algebra via the same
  SelectAlgebra the mint uses), compared order-free; statecell_id
  deliberately NOT keyed (mint-order artifact).
- STRUCTURAL VALIDATORS (the E-29 assert promotion, all ValidatorFail
  = fprintf+abort, survive NDEBUG): V-AGG-EFFECT (spec §2.3 effect-set
  totality per kGroupUpdate — 2 drains, ± folds summing 0, emit+old,
  2 NonRecursive counters, 2 kInI crossings, 2 queue appends; kStateSeal
  = the single sign-0 fold); V-AGG-SOLE (agg table present; summarized
  input table present and never aliasing it — the promoted DR.cpp:665
  assert); V-AGG-PAIR (statecell ids a GROUP_UPDATE↔STATE_SEAL bijection
  onto [0, |statecells|)). The stale TODO(P4) at DR.cpp:2761 deleted.

GATES (all green on the exact committed tree): FULL SUITE PASS (164),
zero golden churn; generated datalog.h + -ir-out byte-identical on the
average_weight and cf16_4 witnesses; ctest 3/3; debug + release builds
green (validators always-on under NDEBUG — the release binary runs the
census on average_weight); Q5 spot @128 release 0.11-0.13s / debug
0.93s (flat). FIRE-TESTS (scaffolding deleted pre-commit, per house
rule): a provenance-flip perturbation aborted on the key multiset; an
exp+1 perturbation aborted with "derived 3, emitter-expected 4" on
average_weight (2 aggs + 1 KV — the recount exact); the build-time
compile_datalog() step aborting the ninja build was a third,
independent confirmation. THE HOUSE BET NOT CASHED: the census found
no live miscount (unlike R1e's day-one catch) — expected, since the
recount and the mint iterate the same unconditional accessors; the
census's value is against FUTURE drift (skip paths, wrong enrollment,
wrong algebra/provenance selection), and it is the TEMPLATE the P2
subgraph family's census copies (E-25 charter).

PRE-REGISTERED PREDICTIONS (§14.3-P0): ALL HIT — zero emission change,
suite 164 byte-identical, validators green corpus-wide; the "census may
fire on a real miscount" house bet correctly did NOT pay (see above).

## 3. The diff plan as designed and judged (checkpoint 2+3; 2026-07-16)

Workflow: 4 opus designers (P1-§6 / P2c-config / P2b-instances /
P3-demand), each delivering a diff plan + a hand-written binding
artifact (SubgraphsDemand.artifacts/) + pre-registered predictions;
4 adversarial opus judges (8 agents, ~825k tokens). EVERY judge round
found something real (the house precedent, an eighth time). Verdicts:
ALL FOUR REVISE — P1 and P2c with local, targeted fixes (landable after
amendment); P2b substrate-sound but its two concrete deliverables holed;
P3 with a CRITICAL premise-level hole (the R4-precedent shape).

### P1 — §6 monotone/descent stage (p1-cf16-hole-target.md): REVISE,
### core refactor RATIFIED-QUALITY

The judge independently verified the byte-identity argument on real IR: only VECTORLOOP
+ per-column VARs mint ids (UPDATECOUNT/PARALLEL id-free ctors); same
walk position + same order ⇒ id-stream identity (the §12.6 discipline);
the UPDATECOUNT construction is byte-equal across both paths; the
net-additions append is confirmably the descent's sole job (Build.cpp:
886-893) and stays put; ExtendEagerProcedure is the sole receive path —
scope complete; zero-golden-churn prediction SAFE. Findings to amend:
(1) MEDIUM — the hole-filled-exactly-once validator AS DRAFTED can never
fire (Build.cpp:758-759 unconditionally emplaces the descent par;
Emplace itself aborts on double-fill): redesign to assert
next_parent->AsUpdateCount() non-null with the expected table (catches
the real R-CURSOR risk pre-abort) or drop it from the gate; (2) MEDIUM —
Site 5 is a fold-op COVERAGE check, not an emitted-tree cross-check
(blind to the descent's net-additions append); downgrade the claim,
delegate tree shape to the -ir-out structural gate (already in the
design); (3) LOW — one LowerIngestFold call site, not two.

### P2c — config-column aggregates (p2c-config-agg-target.md): REVISE,
### premise VERIFIED, three edits then GO for config_agg_1

Both E-31 facts re-confirmed empirically by the judge (config_agg_1.dr
reaches exactly the ControlFlow fence; the reduction ABI is config-
blind). Findings to amend: (1) MEDIUM — the store-Fold rewrite must SAY
"drop the Summary(...) wrapper at StateCell.h:315, forward the raw
pack" (as written it won't compile on the mandatory case); (2) MEDIUM —
@recompute must SUPPRESS config at the Fold arm and route it through
Emit/ReduceLive (the artifact's §4.3 unconditional Fold-split would
corrupt the recompute multiset); (3) LOW — the kHasConfig compile-time
flag on the emitted Reduce policy is a fifth threading site (DebugValidate
calls Fold config-free), count it or drop DebugValidate for config
cells; (4) LOW — disambiguate lib/DataFlow/Build.cpp vs
lib/ControlFlow/Build/Build.cpp anchors.

### P2b — keyed nested instances (p2b-instance-target.md): REVISE —
### SUBSTRATE RATIFIED-QUALITY, implementation NOT authorized yet

The scheduler-cost pricing, effect-domain totality, and the seven-point
plug-in mapping all verified exact; the C-0b-one-level-up answer
(ACYCLIC-FROZEN-FIRST: demand as frozen read under an acyclic fence =
ViewSelfReachable, the R2-acyclic-first discipline; recursive demand =
round-carried, priced as claimed-frontier-like) is sound. HIGH holes in
the concrete deliverables: (1) the §5 witness has NO demand→instance
edge — the demand relation is dead-flow-collapsible and the strict-above
stratum claim baseless (exactly the E-32/E-33 trap); wiring demand into
the instance body is genuinely (a)-transform territory, so (b)-with-a-
real-witness is MORE COUPLED to (a) than the (c)→(b)→(a) ordering
assumed; (2) the InstanceStore sealed word as a row-id watermark into a
compactable nested DiffTable BREAKS the non-aliasing property it claims
to transfer (CompactDead renumbers) — redesign around the nested
table's OWN kInI watermark; (3) MEDIUM — the two-differential-tables
double-count seam (nested table sweep vs outer publish) undischarged.

### P3 — demand transform (p3-demand-argument.md): REVISE with a
### CRITICAL premise hole — DESIGN-ONLY this epoch confirmed, for a
### DEEPER reason than the artifact gave

THE HOLE (judge, verified): the demand SEED DOES NOT EXIST in the
eager/push model. A bound #query is built like any relation
(lib/DataFlow/Build.cpp:239-246, no adornment in the Query IR) and its
bound columns surface only at ControlFlow as a READ-TIME index probe
over the already-eagerly-materialized table (lib/ControlFlow/Build/
Build.cpp:394-408); the runtime bound argument arrives AFTER eager
computation. A pure Query-IR rewrite (the design's own anti-evaluator
constraint) has no injection point — d_p is source-less and dead-flow
collapses it, the exact obligation-(f) failure. The one query-driven
re-injection surface today is the FORCING-MESSAGE path (ControlFlow
Build.cpp:246-290); generalizing it is the candidate seed mechanism and
the next design task. Also: per-adornment magic splits unspecified
(transitive_closure.dr needs THREE adornments of tc); the all-free-
forces-full interaction makes demand inert on the multi-query-sharing
corpus subset; E-32 discharge covers p' but not the d_p projections.
CHURN FACT (verified twice, independently): 37-38 of 164 corpus cases
carry bound #query columns — an unconditional transform rewrites ~23%
of the corpus; any in-scope P3 must be mode-gated off by default.

## 3.1 Owner decisions (2026-07-16, ratified at the design checkpoint)

1. EPOCH CUT: P1 (§6 monotone/descent, as amended per its judge) +
   P2(c) (config-column aggregates, as amended per its judge) are the
   epoch's EMISSION diffs. P2(b) keyed instances is PAPER-ONLY this
   epoch: the substrate design (effect domain, scheduler pricing,
   seven-point mapping, acyclic-frozen-first) is ratified as the
   standing record; the InstanceStore sealed-word redesign + the
   demand-wired witness are next-epoch design obligations, no emission.
2. §6 GOLDEN POLICY: BYTE-IDENTITY is the hard target, guarded by the
   §12.4-precedent structural gate — full -ir-out + datalog.h byte-
   compared pre/post in opt AND nocf on cf16_4 / negate_lower_strata /
   deep_chain_retract / cf14_3 (mixed) / average_weight; suite 164 zero
   stdout churn; oracle + monotone sidecars byte-identical; permcheck
   permutation-bless ONLY as a reviewed fallback, never to green red.
3. P3/DEMAND: DESIGN-ONLY this epoch, re-chartered on the SEED
   MECHANISM (the judge's CRITICAL: no demand-seed injection point
   exists in the eager/push model; generalizing the forcing-message
   surface is the candidate; per-adornment splits + the all-free-
   forces-full interaction must be specified). The amended invariant
   argument is the standing gate; any live transform is mode-gated off
   by default (37-38/164 corpus cases carry bound queries).
