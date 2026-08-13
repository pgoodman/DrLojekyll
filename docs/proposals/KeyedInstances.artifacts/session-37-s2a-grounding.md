<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-37 S2a grounding — the R-MONO keyed InstanceStore nested lowering, assembled design

> Assembled from the six-extractor sonnet fan-out (e1-rel-mint / e2-lowering /
> e3-codegen-runtime / e4-flag-program / e5-belts / e6-witness-eqgate, scratchpad
> copies) + orchestrator inline verification, at tip `7a9db1e4` (docs-only over
> `28f2ccc0`). Restoration source `6d6248a2` throughout. STATUS: EXECUTED —
> S2a LANDED this session (CP0-CP7, gate GREEN; §8 panel record + §9 landed
> state below). Owner ratification of the whole S2a slice PENDING (nothing
> pushed).

## §0 Verdict-shaping facts (each verified, not assumed)

1. **The Rel/DataFlow substrate is byte-compatible.** `EffKind` (incl.
   `kInstanceRebuild`/`kInstanceDemand`), `BindingSource::kInstanceKeySlot`,
   `Lowering::kSectionWalk`, `PlanNode`/`DRArm`, `DRInstance`, `DROp`'s
   nested fields, `DRFlowGraph::instances`/`instance_stratum`, and the whole
   GuardAnnotation/RecognizedSubgraph/DemandForcings front-end are LIVE at tip
   and byte-similar to pre-cut. What is DELETED (must re-add): the 3 `DROpKind`
   enum values, `SubgraphInstances()`, `ResolveLiveRecognition` + effect
   builders + `BuildSubgraphInstanceOps` + mint call site, the census
   expect/per-op arms, V-ALPHA / V-INST-DRAIN / the 5 `CheckInstance*`
   validators + the `LinearizeAndValidateDRFlow` `CheckInstanceOrder` call,
   `DROpStratum` instance arms + `key_of` `kInstanceSeal` band-11 arm,
   Format.cpp's 3 op-render cases + `emit_pub_row` + `DROpKindName` cases +
   census-array entries, and `IsCutSuccessorDR`'s guard-annotation disjunct.
2. **The flat byte target is already stable at tip.** The pre-cut witness `.dr`
   + UNMODIFIED pre-cut driver compile under tip `-demand` and the stdout is
   byte-identical to the pre-cut golden (verified by actual compile+run this
   session). Re-bless is still mandatory (rel goldens carry `resource=sr#` +
   the restored census tokens) but answer bytes are fixed.
3. **The ControlFlow region family is wholly absent** and mirrors the landed
   GroupUpdate/StateCell family 1:1: `ProgramSubgraphInstanceRegion(Impl)`
   (public + lib Program.h, Program.cpp wrappers, Operation.cpp
   Hash/Equals/IsNoOp/As*, Visitor.cpp MAKE_VISITOR),
   `ProgramOperation::kSubgraphInstance`, `ProgramInstanceStore(Info)` +
   `Program::InstanceStores()`, `ProgramImpl::instance_stores`.
4. **The lowering sites are pinned at tip**: `LowerSubgraphInstances` +
   `ClassifyVector` `kSubgraphInstance` arm + call-before-`LowerCommitSweeps`
   + V-INST-EMITTED (Procedure.cpp; tip gap identified); the
   `impl->instance_stores` descriptor loop (Stratum.cpp tip gap 2319/2321);
   the fences + RP-9 + `context.demand_instance_enabled` block (Build.cpp,
   after the @product fence; `frozen.Query()`→`frozen.DataFlowGraph()`);
   `Context` fields (Build.h, between `dr_flow`× and `frozen_census`;
   `EmittedInstanceOp` after `emitted_ingest_loops`); the BuildDRInventory
   mint call site (tip Rel.cpp:1820/1822 gap, byte-verified surround).
5. **Runtime is drop-in.** `InstanceStore.h` re-adds verbatim; its four dep
   headers are byte-identical 6d6248a2→tip; all bench-counter names still
   registered. StateCell.h has ZERO drift (SealOne predates the fork — seed
   open question 2 CLOSED). Table.h drift is one comment.
6. **Codegen is a surgical revert + 3 non-conflicts**: (a) demand-ABI
   suppression reworded only; (b) P7b V-PLAN-HONEST sits in disjoint EmitScan;
   (c) `next_ins_id` renumbering affects no existing golden (no codegen golden
   contains a SUBGRAPHINSTANCE region).
7. **THE BAND RESCAN IS A FULL SCAN** (pre-cut Database.cpp:2339-2341: "a full
   scan with a key filter — the keyed index is a deferred perf refinement").
   Combined with the guard JOINs being excised from the eager walk (the
   nested pre-cut rel golden has ZERO kEagerJoin/kJoinEmit), the nested
   compile MINTS NO JOIN PIVOT INDEXES. This refutes e5's "zero derivation
   changes" conclusion in the flat→nested direction and fixes the
   arrangements arm (§3).
8. **The nested lowering changes NO table allocation.** FillDataModel and all
   8 GetOrCreateIndex sites are demand_instance-blind at 6d6248a2;
   `BuildSubgraphInstanceOps` re-resolves already-allocated tables and mints
   none. Pub/demand/input tables all persist (nested rel golden confirms);
   the InstanceStore is a codegen member OUTSIDE the materialization model
   (the StateCellStore precedent, verbatim).

## §1 Restoration manifest (execution order; every step behind the flag)

Restore-then-adapt from `6d6248a2`, in dependency order (E2 §7):

- **CP1 — Rel-IR mint + census + validators** (`lib/Rel/Rel.h`, `Rel.cpp`,
  `Format.cpp`): re-add the 3 `DROpKind` values in pre-cut position (between
  `kStateSeal` and `kEagerForward` — keeps the census token order
  byte-compatible); `SubgraphInstances()`; `ResolveLiveRecognition` +
  `InstantiateEffects`/`DeathEffects`/`SealEffect` + `BuildSubgraphInstanceOps`
  verbatim; the gated mint call at the BuildDRInventory gap; the census
  expect arms + per-op census switch arms; V-ALPHA + V-INST-DRAIN +
  `CheckInstance*` + `CheckInstanceOrder` call; `DROpStratum`/`key_of` arms;
  Format.cpp `DROpKindName` + census array + op-render cases + `emit_pub_row`;
  `IsCutSuccessorDR` third disjunct (D1 — the chain-breaker; shared authority
  with AnyCutSuccessorDR keeps V-INGEST-XCHECK symmetric). NOTE the switch
  hazard (E1 drift 1): `DROpKindName` fails compile until extended (good);
  `ValidateOpResources`/`DROpStratum`/render/census-per-op have `default:`
  arms that compile silently — extend each MANUALLY, checklist below.
- **CP2 — ControlFlow region family** (public+lib `Program.h`, `Program.cpp`,
  `Operation.cpp`, `Visitor.cpp`): the SUBGRAPHINSTANCE class family +
  `ProgramInstanceStore(Info)` + `InstanceStores()` + 5-arg `Program::Build`
  decl (`bool demand_instance = false`).
- **CP3 — Build wiring** (`Build.h`, `Build.cpp`, `Procedure.cpp`,
  `Stratum.cpp`): `demand_instance_enabled` + `EmittedInstanceOp` fields; the
  fences + RP-9 + gate-assignment block (adapted: `frozen.DataFlowGraph()`,
  interleave with tip's `context.frozen`); `LowerSubgraphInstances` +
  ClassifyVector arm + call site + V-INST-EMITTED; the `instance_stores`
  descriptor loop.
- **CP4 — Runtime + codegen** (`Runtime/InstanceStore.h` drop-in;
  `Database.cpp` revert of the ~650-line deletion: ProcEffects.instances,
  CollectEffects, DetailStateParams/Args, EmitInstanceStructs,
  EmitDatabaseDecl member+ctor, EmitRegion dispatch, EmitSubgraphInstance,
  include guard, Run() call site). SKIP the D3.a.1 `_retract` EmitQueryFriends
  arm ONLY if it fails to compile without `-demand-retract` plumbing —
  otherwise restore verbatim (dormant: no differential demand table exists
  under the S2a flag set).
- **CP5 — CLI** (`Main.cpp`): `gDemandInstance` + `-demand-instance` argv arm
  (implies `-demand`) + the 5th `Program::Build` arg. `-demand-retract` is
  NOT restored (S2b; its DataFlow half — Query::Build's 6-arg — is already at
  tip, unexercised).
- **CP6 — belts arms** (§3) + **CP7 — tests/witness** (§5).

## §2 Activation design (resolves E1 drift 3 "design fork")

The pre-cut design restores unchanged: explicit `-demand-instance` →
`Program::Build(..., demand_instance)` → fences (strict rejects) →
`effective_demand_instance` → `context.demand_instance_enabled`. The RP-9
`@key` fallback arm restores VERBATIM but is DORMANT in the S2a corpus (it
fires only under `-demand` + an `@key`'d demanded decl; no restored case
carries both — the @key witnesses return with the RP-6/S2c arc). Rejected
alternative (deriving activation from `RecognizedSubgraphs()` non-emptiness):
that conflates `-demand` with `-demand-instance` and deletes the two-lowerings
eqgate contract.

## §3 THE BELTS-INTEGRATION ARMS (the no-precedent part, grounded per-site)

1. **CrossCheckMaterialization: NO ARM, by identity.** The nested lowering
   allocates no table and removes none (§0.8). The stored plan stays valid
   under both lowerings. The cross-check itself is the referee — if this
   claim is wrong it aborts on the witness compile (belt-verified live in
   the gate).
2. **DeriveArrangements: ONE arm.** `DeriveArrangements(query, plan,
   bool demand_instance = false)`; inside R-JOIN-UNIFORM, skip a pivot-JOIN
   whose view carries `GuardAnnotationIndex() != kNoGuardAnnotation` when
   `demand_instance` (the walk-excised guard joins emit no TABLEJOIN and the
   band rescan is index-free, §0.7; set semantics keep entries that coincide
   with R-FULL or another join's/R-QUERY's request). `Query::Build`'s call
   stays flagless (the stored plan/-materialization-out remain the flat
   derivation — honest: the QueryImpl is lowering-blind). `Program::Build`
   under the flag re-derives into a plan COPY and `CrossCheckArrangements`
   compares against THAT. R-QUERY unchanged: band-(b) publishes into the pub
   TABLE and the query cursor is the plain forced cursor (pre-cut
   EmitQueryFriends confirms; pub indices are maintained by the band's
   EmitIndexAdds ride-along).
3. **V-REL-OP-RESOURCE: extend the walk.** `ValidateOpResources` gains
   resolution of `op.demand_table` / `op.input_table` (non-null only on the
   restored kinds; `pub_table` rides `table_op_table`, already swept). All
   three resolve through `table_to_resource` since they are ordinary model
   tables (§0.8). This closes E1 drift 7's silent coverage hole and is
   exactly seed §2 item 3.
4. **Census/validator family: restored recount.** The census expect arms
   (`exp_instance`/`exp_death` from a fresh `ResolveLiveRecognition` — the
   single-authority recount), V-INST-EMITTED (ControlFlow side), and the
   V-INST-* belts return as pre-cut, now running INSIDE the larger s30-s36
   validator family (no interference found: s34/s35 stamps run in
   BuildDRInventory before the mint site; the census array regains 3 entries
   or `census_total != ops.size()` aborts — E1 drift 2).

## §4 Desired IR states (predict-then-verify targets)

1. **Flat rel goldens (witness + the 11 existing pins + demand_tc_witness):**
   tip flat dump + exactly three additive census tokens
   `kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0` between
   `kStateSeal=0` and `kEagerForward=`, restoring the pre-cut token order.
   NOTHING else moves (verified: tip flat witness dump already byte-matches
   the pre-cut golden modulo `resource=sr#` + these tokens).
2. **Nested rel dump (smoke, NOT goldened in S2a — see §6.3):** the pre-cut
   `key_neighborhood_witness.rel.opt.golden` shape (instances: header,
   kSubgraphInstantiate/kInstanceSeal ops with reads/effects/spine, commit
   sweeps for demand+input, ingest folds with `kVecAppend(kNetAddition)`,
   census `=1/=0/=1`) PLUS `resource=sr#` tokens on `args:` base-table
   tokens. The witness's kJoinEmit/kEagerJoin/kEagerInsert lines VANISH
   under the nested arm.
3. **-materialization-out:** unchanged everywhere (flag-off byte-identity;
   the stored plan is flag-blind by design §3.2).
4. **-region-out:** unchanged by the nested arm (the freeze precedes
   Program::Build and is lowering-blind; S1c's region-internal line is
   already the flat surface). No region goldens for the witness existed
   pre-cut; none added.
5. **Answer surfaces:** witness stdout/oracle/monotone/behavioral goldens
   re-blessed at tip == pre-cut bytes (§0.2 verified for stdout; the rest
   expected identical — the oracle/interp are demand-blind).

## §5 Witness + harness

- Restore verbatim: `demand_neighborhood_mono_witness.{dr,main.cpp,batches,
  drflags,eqgate,irgold,probes}` (drflags `-demand`; eqgate marker file;
  irgold `rel` ×4 modes) + `demand_cyclic_1.{dr,drflags,main.cpp}` (drflags
  `-demand -demand-instance`; no goldens — pure diagnostic).
- runall.sh: add `demand_cyclic_1` to the all-modes-diagnostic case list
  (one line; the ~90-line stale header prose gets a MINIMAL touch-up only
  where it lies about the live list).
- Goldens: fresh `--bless` at tip for the witness's 8 goldens (stdout,
  oracle, monotone, behavioral, rel×4); the 11 existing `.rel` pins +
  demand_tc_witness's rel goldens re-bless for the 3 census tokens. NEVER
  copy pre-cut golden bytes (permutation discipline; predict first per §4).
- eqgate: `run_eqgate` needs ZERO harness changes (byte-identical at tip,
  verified; `flags_of` folds `.drflags`, then `-demand-instance` appended).
- ctest: restore `tests/InstanceStore/` (CMakeLists + 464-line unit test,
  drop-in) + the `add_subdirectory` line → ctest gains a 6th target (the
  gate reads "5/5 + the restored InstanceStore = 6/6"). RelValidators tests
  stay deleted (S2b+, they exercise death/diff arms).

## §6 Scope decisions + panel questions

1. `-demand-retract` NOT restored (S2b). kInstanceDeath machinery restores
   verbatim but is UNREACHABLE (its mint gates on
   `TableIsDifferential(demand_table)`, never true without retract).
2. RP-9 restored-but-dormant (§2) — panel: keep or defer?
3. **Nested rel surface unpinned in S2a** (pre-cut policy: the pin lived on
   the flagless @key twins, deferred with RP-6). Alternative: a flags-twin
   case (`.drflags -demand -demand-instance`, stdout golden SYMLINKED to the
   mono witness's — the key_* twin mechanism works today via bless_copy).
   RECOMMENDATION: defer to S2b/S2c; the eqgate + the census recount +
   V-INST-EMITTED already referee the nested compile ×4 modes live. Panel:
   is a byte pin worth the extra case now?
4. The e5 R-QUERY ambiguity is CLOSED §3.2 (plain cursor; band maintains pub
   indices) — panel: refute against the pre-cut EmitQueryFriends verbatim.
5. Belt live-fire in the gate: corrupt the arrangements arm (drop the skip →
   CrossCheckArrangements must fire naming the guard-join index), corrupt
   V-REL-OP-RESOURCE's new resolution (unmapped table → fires), revert →
   quiescent.

## §7 Exit gate (charter, restated against this design)

eqgate GREEN ×4 (flat==nested==golden); SUITE PASS with flag-off
byte-identity (the 3 census tokens move goldens ONCE via bless, then
byte-stable; `-demand-instance` off = zero perturbation beyond that bless);
demand_tc_witness byte-identical post-bless; all four belts quiescent on the
nested compile + live-fire verified; ctest 6/6; WIP commit per CP; owner
ratifies before the final commit.

## §8 The refuter-panel record (4 opus refuters, all refuted=true; every finding folded)

- **(a) drift**: [REFUTED] CP2's file list missed `lib/ControlFlow/Format.cpp`
  (the `operator<<(ProgramSubgraphInstanceRegion)` + FormatDispatcher
  MAKE_VISITOR — the omission is SILENT via the empty `Visit` default) →
  restored in CP1b, nested `.ir` smoke added to the gate (renders
  `subgraph-instance i#0`). [REFUTED] the "DROpKindName fails compile" claim —
  no `-Werror` in the build; all FIVE DROpKind switch sites are manual-
  checklist (all five were extended in CP1). [WEAKENED] the R-QUERY story was
  pre-cut-stale: P7 (post-fork) provisions the bound-query index, so the
  nested band-(b) runs its `pub_has_indexes==true` arm on the witness —
  predicted `idx.Add` inside the band, VERIFIED in the generated `.h`.
  [REFUTED] the CP4 `_retract`-skip note — that arm was never deleted (live
  at tip); the filtered patch could not double-restore it (not in the diff).
- **(b) belts**: [REFUTED] the guard-annotation-only skip — a deletion-capable
  guard join is ALSO emitted by the delta path (indexes minted regardless) →
  the landed skip is `demand_instance && guard-annotated &&
  !CanReceiveDeletions()`. [REFUTED] the compare-against-a-copy mechanics —
  `CrossCheckArrangements` read the stored plan → the landed 4-arg
  plan-override core + 3-arg forwarder. [WEAKENED] §3.1's referee claim —
  CrossCheckMaterialization structurally cannot fire on a lowering-selector
  change (FillDataModel is upstream of the walk and flag-blind); the no-arm
  CONCLUSION stands on the allocation-site argument; it is OFF the live-fire
  list. [WEAKENED] §3.3 — the two resolve() calls are tautological at
  construction (kept as a regression fence, honest comment; the arms-spine
  sweep is an S2b candidate).
- **(c) eqgate**: [REFUTED] §6.3's "existing referees suffice" — the excision
  predicate (any live guard annotation) and the mint predicate (complete
  resolution) can diverge SILENTLY (cut-but-unminted = under-answer) → the
  NEW always-on V-INST-COHERE belt at the mint; and the flags-twin nested
  `.rel` pin landed in S2a (not deferred). All five divergence attacks
  SURVIVE: injector byte-identical pre-cut→tip; publish-vs-sweep ordering
  unchanged; P7-seek premise inverted (pre-cut already emitted the seek);
  driver honors the cursor contract; Optimize.cpp byte-identical.
- **(d) scope**: [REFUTED] WIP-greenness — CP1 landed without the 11-golden
  census bless (suite red between CP1 and CP1b; fixed same session, bless
  bound to CP1b). [REFUTED] the "dormant differential machinery" claim —
  `diff`/`input_diff` derive from pub/input tables, NOT the retract flag, so
  the diff-INPUT arms are reachable under bare `-demand` + `-demand-instance`
  → `demand_diff_neighborhood_witness` (the e5 carrier) landed IN S2a as
  their witness. [WEAKENED] the masked-check trap (unknown flag exits 1 —
  demand_cyclic_1 would "reject" for the wrong reason mid-restore) — moot
  once CP5 landed before CP7. One-session feasibility SURVIVES (the slice is
  indivisible below CP1-CP5 + the arrangements arm: IsCutSuccessorDR is the
  sole frontier-provisioning lever, so a mint-only shadow is unsound).

## §9 Landed state (gate results, session 37)

- Commits: CP0 `c52e7a12` (runtime + ctest), CP1 `7b8154e0` (Rel mint),
  CP1b `b2ca94d5` (census bless + CF Format render), CP2-CP5 `a6c9fd6d`
  (region family / lowering / fences / codegen / CLI), CP6 `fc01458b`
  (belts arms + V-INST-COHERE), CP7 `f53ad1c5` (witness corpus + flags-twin).
- Gate: OptDiff **SUITE PASS (232)** (228 + mono witness + nested twin + e5
  carrier + cyclic fence); eqgate GREEN ×4 modes on BOTH carriers
  (nested==flat==golden, byte-verified per mode); every restored answer
  golden byte-equal to its pre-cut bytes (stdout/oracle/monotone/behavioral,
  both carriers); ctest **6/6** (InstanceStore restored); belts quiescent on
  every nested compile; arrangements live-fire verified (corrupt → the exact
  predicted `sr#1 columns=(0)` abort; revert → quiescent); flag-off corpus
  byte-identical modulo the ONE predicted+blessed census-token delta;
  `demand_tc_witness` re-blessed census-only (flat arm undisturbed).
- Predict-then-verify: the nested `.rel` == pre-cut nested golden shape +
  `resource=sr#` args tokens + census `kSubgraphInstantiate=1/kInstanceSeal=1`
  (first prediction byte-exact); the flat dumps changed by census tokens
  ONLY; the nested `.h` carries the InstanceStore member + reference-param
  threading + the band-(b) `idx.Add`.
- Deliberate divergences from pre-cut (all recorded): instance-op `args:`
  render uses `tidr` (s35 resource tokens); the V-INST-COHERE belt is NEW
  (strictly stronger than pre-cut's silent `!ok` skip); tip comment wording
  kept at the two demand-ABI-suppression loops; P7b/s34/s35 surfaces all
  preserved.
- S2b residue (named): `-demand-retract` + kInstanceDeath live path + the
  remaining eqgate family + RelValidators death tests + the V-REL-OP-RESOURCE
  arms-spine sweep + the R-DIFF discriminator live-fire (differentialness-
  gated skip vs unconditional). S2c residue: multi-adornment + the @key
  twins (RP-6) which retire the flags-twin's pin role. RP-9 restored VERBATIM
  but dormant in this corpus (fires only under `-demand` + an `@key`'d
  demanded decl; its witnesses return with RP-6/S2c).
