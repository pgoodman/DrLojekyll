# RegionalDataFlowCore.artifacts — directory map

## Current resumption authority

Start with **`session-23-prompt.md`** (the current next-session charter — now a **P7
charter**) and its grounded START-HERE **`p7-grounding-seed.md`** (POST-P6.2,
current-code-grounded: the as-is pseudocode, the P4-substrate-vs-s15 reconciliation
table, the six P7 hunk diffs, IR desired-states on `key_partial_1`, seven open Qs).
The **`session-23-whole-program-seed.md`** backbone (**P1 + P2 + P3 + P4 + P5 + P6.1
+ P6.2 LANDED**; P6 compile-time first cut COMPLETE) RANKS the fork **P7 #1** over the
runtime cut P6.3–P6.6 (§3, dependency-ordering clincher); confirm the ranking at the
[OWNER STOP]. The s14-15 `keyed-rewrite-p7p9-diffs.md`/`-critique.md` are PARTLY STALE
(predate P4/P5/P6) — trust the p7-grounding-seed reconciliation over their raw anchors. The prior charter was **`session-22-prompt.md`** +
**`session-22-whole-program-seed.md`** (tip `734712c0`, next actionable was P6.2 —
now LANDED).
**P6.2 LANDED (session 22, 2026-08-10)**: typed edge-local routing +
`SymbolicFieldId` promotion (compile-time, codegen BYTE-UNCHANGED) —
`RegionTemplate.rules`/`inherited_symbolic_fields` populated CLAUSE-SOURCE (the
post-Optimize DataFlow graph loses per-relation field identity via CSE — the same
lesson that flipped P6.1's insert-arm); `AssignSymbolicFields` /
`BuildRuleRoutingProjections` / `PromoteSharedSymbolicField` (a `while(changed)`
union-find FIXPOINT, directional per-head-field, F16 co-occurrence trap closed) in
Planning.cpp + the new `symbolic_field_table` interner; gated own-width
`rule`/`shared-field` `-region-out` blocks (NO census count). NEW carriers
`key_corecursion_1` (F16 arm) + `fixpoint_force` (F28 referee); 30 region goldens
gained a pure suffix. A 4-refuter EMPIRICAL opus panel (0 blocking; C1 fixed the
carrier's vacuous copy-cycle to a JOIN-in-cycle, C2 added the F28 referee). Record:
**`p6.2-grounding.md`** (+ `p6.2-design.md` full hunks, `p6.2-synthesis.md` panel
verdict + IR states). SUITE PASS 226, ctest 5/5, codegen byte-stable.
**P6.1 LANDED (session 21, 2026-08-09)**: query-independent recursive components
(compile-time, codegen BYTE-UNCHANGED) — `RegionTemplate.recursive_components`
populated by `ComputeRecursiveComponents` (Planning.cpp) projecting the DataFlow
multi-view-stratum SCC condensation (`QueryView::Stratum()`, message-seam-closing)
onto frozen relations via `OriginDecls`; gated own-width `-region-out`
`recursive-component` block (NO census count); MODE-FAITHFUL; carriers
`corecursion_1`/`two_inductions`/`recursion`/`tc_nonlinear_diff`; a 3-refuter opus
panel EMPIRICALLY refuted an insert-arm formulation → the origin projection is the
survivor. Record: **`p6-grounding.md`** (§6 = the shipped v2 + panel verdict).
SUITE PASS 224, ctest 5/5.
**P5 LANDED (session 20, 2026-08-09)**: the partial-binding DAG — the ORDERED
`DeclaredAccessPath` authority + the per-relation order-free binding-schema DAG
(`RelSchemaLocalId`/`MaterializePrefixChain`, lazy) interned at freeze from
`decl.InstanceKeys()`, the `HasInstanceKey()`-tied V-PREFIX-CHAIN belt, a golden-
pinned `-region-out` `declared-key` line (F21), and the first positive `@key`
carrier `key_partial_1`; model+render only (no codegen movement), OptDiff SUITE:
PASS 223, ctest 5/5. Full record: **`p5-grounding.md`** (the layer-site inventory,
the refuter-panel survivors A1–A7 §8, the IR states §9, the drift table §7). **`p4-grounding.md`** is the P4 record (the
`AccessPlan` fourth authority selected-at-freeze / read-at-codegen; the §8 critique
survivors + certifications; the §9 IR states; the freeze-store→codegen-read + negative-
witness method template). NOTE for P5: `@key` survived P1 as INERT parsed metadata
(`InstanceKeys()`/`HasInstanceKey()`); there is NO positive `@key` corpus carrier
post-P1 (the semantic `key_*_witness` cases were deleted with demand) — P5 must author
a new one. The prior charter was **`session-19-prompt.md`** + **`session-19-whole-program-seed.md`**
(tip `63573a67`, **P1 + P2 + P3 LANDED**; next actionable was **P4**, honest complete-path
specialization / FullScanFilter — now LANDED at `c546a6a4`). The session-19 seed §1 is the POST-P3 whole-program pseudocode
(the freeze now builds the P3 request/derivation model via `BuildRequestPorts`; a
bound `#query` renders a `-> request-port`). **`p3-grounding.md`** is the P3 record
(the model P4 extends — `RegionInstanceRelations`, `AddDerivation` via
`member_key_positions`, the §8 critique survivors, the §9 IR states). The prior
charter was **`session-18-prompt.md`** + **`session-18-whole-program-seed.md`** (tip
`ae207c36`, **P1 + P2 LANDED** — P1 deleted the -demand/keyed-instance authority wholesale; **P2 made
`FrozenRegionalProgram` the TYPED semantic owner** (ONE typed `RegionTemplate`
replaces the 5 render-string shells; `member_key_positions` positional masks,
`DataFlowGraph()` accessor); compile-clean + OptDiff SUITE: PASS 222 + ctest 4/4;
`@key` still inert; the next actionable step is **P3**, the RequestEdge /
FactDerivation acyclic slice). **`p3-grounding.md`** is the session-18 grounding
output — the code-verified P3 execution-readiness record (layer-site inventory, typed
`RegionInstance.h` ids, edge-op pseudocode, the M3 compile-time-model resolution, the
drift-correction table vs `reconstruction-diffs.md` §3-P3, the adversarial critique
survivors §8, and the `-region-out` desired-state §9); it supersedes the stale P3
anchors in `reconstruction-diffs.md` §3-P3. The session-18 seed §1 is the POST-P2 whole-program
pseudocode grounded in real code; the session-17 seed §1.3 (pre-P2 render-string
shell) and every "current pipeline" doc before it are now HISTORICAL for the
Regional layer. **`p2-typed-owner-grounding.md`** is the P2 record (compile-clean
inventory + typed-record design + the 4-lens refuter panel; note
`RelationSchema.member_key_positions` is the H3 positional member key P3 reuses).
The session-16/17 prompt/seeds were the P1/P2 grounding backbones. Then
**`next-session-prompt.md`**: it contains the current code-grounded
review and roadmap for relation-local `@key` semantics, ordered access paths,
canonical relation facts, exact request ownership, rooted lifecycle for cyclic
binding-state dependencies, partial-binding agreement, and lazy trie/COLT work.
It supersedes older session charters and the assumption that `@key` is owned by
query-demand adornments. Re-verify its code anchors against the branch tip
before implementation.

Historical session provenance: DESIGN-GROUNDING session 2026-08-02 at tip
f0c913e0 (branch `keyed-instances`), phases 1–6. The target at that time was
`../RegionalDataFlowCore.md`. The current resumption prompt is the semantic and
execution-roadmap authority for keyed-instance continuation. The older proposal
still supplies the retained invariants named below; it is not an independent
authority to use when the two documents disagree. The remaining artifacts
preserve grounding, staged-cutover, and critique evidence.

READ ORDER for keyed-instance continuation: this INDEX →
`session-15-prompt.md` (the CURRENT next-session charter) →
`next-session-prompt.md` (semantic + roadmap authority) →
**`session-15-whole-program-seed.md`** (START HERE: the post-s14 CONSOLIDATED
whole-program pseudocode — its §1 is the first to include the PHYSICAL layer
(ControlFlow lowering → CodeGen EmitScan → DataIndex → runtime Table/Index),
target model §2, path forward as P0–P9 diffs with the s14 corrections folded into
the diffs §3, and "what session 15 must do" §4) → `keyed-rewrite-p7p9-diffs.md`
(P7–P9 operational diffs + §7 s14 amendments) → `keyed-rewrite-p7p9-critique.md`
(the s14 physical-layer critique: 3 blocking + 5 high + 18 certifications) →
`session-14-whole-program-seed.md` (the prior backbone; §1 lacks the physical layer) →
`keyed-rewrite-reconstruction-diffs.md` (the actionable P1 compile-clean inventory +
D1–D4 resolutions + deepened P2–P6 operational diffs + §5 re-critique amendments) →
`keyed-rewrite-reconstruction-critique.md` (the s13 re-critique: 23 survivors +
20 certifications) → `keyed-rewrite-ir-desired-states.md` (predict-then-verify IR +
the 2 no-baseline carriers) → `keyed-rewrite-pseudocode-seed.md` (§2/§3 target
algebra) → `keyed-rewrite-whole-program.md` (the s12 backbone; §4 = s13 status) →
the rest of the grounding set (below) → `../RegionalDataFlowCore.md` → older phase
reports only when a current prompt references them. `keyed-rewrite-whole-program.md`
§1 and `keyed-rewrite-phase-diffs.md` are s12-vintage — the session-14 seed +
reconstruction-diffs SUPERSEDE them for the current state. The historical
adjudication artifacts remain evidence, not current semantic authority.

### Session 12 (2026-08-06) grounding set — precedes the owner-gated Phase-1 cut

- **`keyed-rewrite-whole-program.md`** — the single consolidated backbone. §1
  whole-program current pseudocode, §2 four-authority target + EvaluateEpoch, §3 the
  AMENDED path forward (the s12 critique folded in: the CORRECTED P1 deletion
  inventory, the P2–P5 amendments, the open decisions D1–D4), §4 the immediate next
  step. Supersedes the phase-diffs P1 inventory where they disagree.

The rigorous, critiqued design grounding the owner needs to green-light Phase 1+.
All read-only (no production code touched; suite stays at the 251 PASS baseline;
nothing blessed). Post-cut IR states are PREDICTIONS to verify when each phase lands.

- **`keyed-rewrite-current-pseudocode.md`** — the implementer-grain deepening of the
  seed §1: 8 per-subsystem pseudocode blocks, a fully RE-VERIFIED anchor table (§1.0,
  supersedes seed §5) and a 10-item drift-correction ledger (§1.1). Anchors verified
  @ tip 46a404d4 + the uncommitted Phase-0 worktree.
- **`keyed-rewrite-phase-diffs.md`** — P1–P9 as hunk-grained diffs (P1 decomposed into
  7 atomic sub-cuts P1.1–P1.7, P6 into P6.1–P6.6), each with invariant / exit gate /
  design-goals-resolved / deletion + reintroduction obligations / anchors. Resolves the
  seven design goals (four-authority separation, order-significant paths w/ order-free
  binding identity, RequestEdge vs RuleActivationEdge, rooted liveness, honest
  FullScanFilter, partial-binding DAG, co-recursive key flow).
- **`keyed-rewrite-critique.md`** — adversarial refute panel (5 per-cluster refuters +
  a cross-phase critic) over the diffs, verified against real code + retained
  invariants. 33 surviving findings (6 blocking, 12 high) + 7 REFUTED (diffs that held).
  HEADLINE: the P1 deletion inventory as authored is INCOMPLETE (would not compile —
  GuardAnnotation CSE-migration in View/Join/IdentityJoin/Link.cpp; kSectionWalk's live
  join consumer at Rel.cpp:2433; ControlFlow DR-vocab consumers), plus two blocking
  soundness/reintro gaps (P4 re-provides FullScanFilter emission only in prose; P6.5
  lacks OVERDELETE→REDERIVE). Every finding carries a concrete `Fix:` amendment; none
  invalidate the direction.
- **`keyed-rewrite-ir-desired-states.md`** — desired post-cut IR output states for the
  `key_neighborhood_witness` / `key_tc_witness` / `key_multi_adorn_witness` carriers,
  phase-staged across .df/.contract/-region-out/.rel/header/C++, with a self-critique
  closing the deepen→diff→critique loop. Baselines empirically verified; deltas predicted.

### Session 13 (2026-08-06) — corrected diff set + deepened P2–P6 + re-critique + 2 new carriers

Grounding round (docs only; suite 251 PASS, nothing blessed, ZERO production code touched).
Read AFTER `keyed-rewrite-whole-program.md` §3/§4:

- **`keyed-rewrite-reconstruction-diffs.md`** — the CURRENT actionable authority for P1 +
  P2–P6: the compile-clean P1 deletion inventory (folds every s12 + s13 missed-deletion),
  the D1–D4 resolutions, the deepened P2–P6 operational diffs at implementer grain, the
  design-goal diffs, and §5 (the s13 re-critique amendments folded). SUPERSEDES the s12
  `keyed-rewrite-phase-diffs.md` P1 inventory + P2–P6 exit gates where they disagree.
- **`keyed-rewrite-reconstruction-critique.md`** — the s13 independent opus re-critique of
  the corrected diffs: 23 survivors (5 blocking, 6 high, 8 medium, 4 low) + 1
  refuter-claim-refuted + 20 certifications. The 5 blocking (B1–B5) gate the P1 green-light.
- **`keyed-rewrite-ir-desired-states.md` §6/§7/§8** (extended s13) — the two no-baseline
  carriers (co-recursive `p@key(K)/q@key(X)` for P6; `@key(A)@key(A,B)` +
  `@key(A,B)@key(B,A)` for P5), each grounded by its VERIFIED CURRENT REJECT (reject→compile
  is the s13 predict-then-verify contract), plus the P4-emission revision (ProgramTableScanRegion
  reuse, certified) and the Phase-0-item-4 parser-flip prerequisite (empirically confirmed).

### Session 14 (2026-08-06) — P1 gate re-verified NOT-clean + P7–P9 physical layer deepened

Grounding round (docs only; suite 251 PASS by construction — production untouched, nothing blessed).
Read AFTER the session-13 set:

- **`keyed-rewrite-reconstruction-diffs.md` §6/§6.1`** (extended s14) — the P1 cut is STILL NOT
  compile-clean: a whole-tree re-grep found **7 MORE un-enumerated consumers** (verified at tip;
  the Rel-IR dump emitter, Rel.cpp DROpStratum/key_of arms, the ControlFlow class-definition sibling
  TUs, the retained→deleted live call at Planning.cpp:304, and 6 unit-test TUs). §6.1 CERTIFIES B2–B5
  internally consistent with the seed §2/§3 algebra. The durable fix is a symbol-driven `git grep -l`
  P1 pre-commit acceptance gate, not range-anchored review.
- **`keyed-rewrite-p7p9-diffs.md`** — the PHYSICAL-layer sibling of reconstruction-diffs: P7 (access
  planning), P8 (lazy tries/COLT/Free Join), P9 (inference) deepened into operational diffs at hunk
  grain + design-goal diffs + DISCRIMINATING (structural, never answer) exit gates. §7 folds the s14
  critique amendments. Headline: at P7 the AccessPlan→region-kind map STOPS being injective (D4
  Option-1→Option-2 transition), so V-PLAN-HONEST moves to the EmitScan emission site.
- **`keyed-rewrite-p7p9-critique.md`** — the s14 opus refuter panel over the P7–P9 diffs: 15 survivors
  (3 blocking, 5 high, 5 medium, 2 low) + 18 certifications. The 3 blocking each force one real
  correction (thread `plan_kind` at every scan mint; intern trie nodes on `BindingStateId` not the
  value-free schema; re-source the inferred-path order from a DataFlow signal, NEVER the file-static
  ControlFlow `SortedPredecessors`). All folded into p7p9-diffs §7.
- **`keyed-rewrite-ir-desired-states.md` §9/§10/§11/§12** (extended s14) — the P7–P9 desired IR states
  with the critique corrections: the `.rel` AccessPlan render + cursor-shape (`s<id>` vs `pos`) belt
  (P7, verified in the real generated header); the trie `-region-out` block reusing the P5
  binding-schema spine (P8); the SPLIT `declared-key`/`inferred-key` render RE-TARGETED DataFlow→Regional
  (P9). Structural pins throughout.

### Session 16 (2026-08-07) — P1 DRY re-grep found 2 MORE consumers + P2 §5.3 folded

Grounding round (docs only; owner chose "stay in grounding" — P1 cut NOT green-lit; suite 252 PASS by
construction, production untouched, nothing blessed). Read AFTER the session-14 set:

- **`keyed-rewrite-reconstruction-diffs.md` §6.0/§6.0.1`** (extended s16) — the symbol-driven acceptance-gate
  DRY at tip `6d6248a2`, comment-stripped, found the `lib`/`include`/`tests` inventory NOW EXHAUSTIVE but
  **TWO MORE un-enumerated compile-breaking consumers in `bin/`** (the 8th & 9th): **§6-7** `bin/drlojekyll/Main.cpp`
  (the `gDemand*` flag globals + arg-parse arms + the demand args at both `Build` calls — the "delete the flags"
  prose named no file) and **§6-8** `bin/Oracle/Main.cpp:749-753` (a DISJOINT binary calling `Query::Build` with
  3 demand args incl. `suppress_demand=true`, whose sole purpose was to defeat flagless `@key` so the oracle
  referees the full closure — behavior-preserving to drop post-cut). `Query::Build` reverts 6-arg→3-arg
  (Query.h:1081-1086); its complete non-comment caller set is exactly {def, drlojekyll Main, Oracle Main}.
  §6.0.1: 7 comment-only stale refs (Prov/Regional.h/Optimize/IdentityJoin/Link/Table.h/Connect) → the gate
  needs a code-vs-comment strip + a retained-symbol/test-data allowlist. New 3rd structural blind spot:
  libraried-vs-disjoint-binary.
- **`keyed-rewrite-reconstruction-diffs.md` §3 P2`** (amended s16) — §5.3 (H1/H2/M1) FOLDED into the P2 primary
  text so it reads consistently: keep the friend-class `query.impl->row_contracts` access (M1 — a public accessor
  WIDENS the leak); the anti-hollow-`R` belt is the region goldens + the in-`Build` recount RE-POINTED at typed
  `R` (H2 — V-REGION-CENSUS is tautological); `frozen_census` KEPT `const RegionalCensus *` (H1). All P2
  Planning.cpp anchors re-verified at tip (:439/:574/:591-608/:663-687). **False-start CERTIFICATION:** P2's
  typed-owner move does NOT re-introduce "DataFlow mutation + post-Optimize recognition" — freeze is a pure
  post-Optimize read; the recognition authority was deleted in P1.
- **`session-16-whole-program-seed.md` §3 P1`** — updated: the §6 inventory is now NINE consumers (§6-1..§6-8),
  the acceptance gate is comment-stripped + allowlisted. P1 remains the next actionable step, owner-gated.

Session 11 (2026-08-06) status: Phase 0 partially landed (reject `@key` on
`#query`; full-context redeclaration consistency; suite 251 PASS). Owner chose
"safe Phase 0 only" — the destructive Phase 1 demand-deletion cut is
owner-gated and NOT started.

## Supersession matrix

| Topic in `RegionalDataFlowCore.md` | Status for keyed-instance continuation |
| --- | --- |
| Semantic member identity, explicit projection, and derivation support | RETAIN. Canonical regional facts and `SemanticMemberKey` remain the logical truth authority. |
| Exact `RequestEdgeId`, multiple owners, late attachment, caller-qualified results, and drain-before-retire | RETAIN. Counts and runtime handles remain derived implementation aids. |
| Pure-region/effect boundary and epoch results independent of queue order | RETAIN. |
| Region ownership/call forest is acyclic | RETAIN for lexical parent/child region ownership and external request routing. |
| No cyclic or key-changing regional request graph | REFINE. Region ownership stays acyclic, but typed intra-region `RuleActivationEdge` dependencies between binding states may cycle and use rooted SCC liveness. They are not request-owner edges. |
| Recursive evaluation remains inside one instance | SUPERSEDE. Prefix-preserving recursion may stay in one binding state; key-changing recursion may run a joint fixpoint over several binding states. |
| Regional demand has no source annotation | SUPERSEDE. `@key` is a source-level ordered specialization-path contract, not a request, member key, query adornment, or physical-layout promise. |
| Physical layout research excluded | RETAIN AS SEQUENCING. Hash/trie/COLT/Free Join planning follows semantic and lifecycle cutover; it does not define the semantic model. |
| Original Stage A–D implementation order | SUPERSEDE. Use the phases in `next-session-prompt.md`. Historical diffs remain evidence for retained invariants and deletion obligations. |

## Historical session deliverables

- **owner-adjudication-brief.md** — the historical consolidated,
  deduplicated DECISION QUEUE in three tiers (T1 blocks the stage sequence, T2
  blocks a stage's exit gate, T3 standalone pre-Stage-A), each item mapped across
  its source labels, with the panel's recommendation where one exists, a fast-path
  summary, and six session errata.
- **ledger-entry-AW-draft.md** — DRAFT of KeyedInstances.md §20(AW) (the regional
  epoch open). The owner decides whether/where it lands; KeyedInstances.md is
  untouched (its ledger stops at (AV)).
- **next-session-prompt.md** — the current keyed-instance review and execution
  roadmap. It supersedes the session-8/K5 successor charter formerly stored at
  this path.
- **INDEX.md** — this file.

## Historical architecture pseudocode

- **regional-arch-pseudocode.md** — whole-program pseudocode of the CURRENT
  demand/keyed-instance architecture (the layer the proposal replaces) with Stages
  A–D as diffs. Fleet-verified 2026-08-02; §4b (runtime epoch path) and §4c
  (generated cursor lifetime — the F3 two-lifetimes gap) are new this pass. §6's
  Stage-A hunk was amended IN PLACE 2026-08-02 to the enum-in-identity realization
  (brief Errata-3 RESOLVED; O-A1 stays owner-gated) and §6/§7 now point at the
  stage docs / the brief as the authorities.

## The five stage diffs (Phase 2)

- **stage-a-diff.md** — typed identity + explicit projections + row contracts;
  lint→contract-validation. Verdict: BLOCKED-ON T-conf-1/T-conf-2 (cyclic-graph
  contract soundness). O-A1/O-A2/O-A3, E-A2/E-A3 escalations live here.
- **stage-i0-interpreter.md** — the reference relational interpreter (review's
  inserted step, ranked #2). Verdict: BLOCKED-ON A-corr-1 (bound-query probe-
  enumeration contract). OG1-4 engine/emitter decisions; H8/OG3 = the R-DIFF
  witness.
- **stage-b-diff.md** — the planning regional program becomes canonical (pure
  refactor, 180 goldens pin it). Verdict: SOUND-WITH-AMENDMENTS (the only clean
  stage). ADJ-2/ADJ-3 carried-open; the `-region-out` grammar is left open.
- **stage-c-diff.md** — request edges replace forcing (THE cutover). Verdict:
  BLOCKED-ON corr-1 (force.dr `@first`); corr-3 (permanent-root) required. H-J
  Variant A/B = the Concern-2 decision; E1–E6 escalations.
- **stage-d-diff.md** — deep forest + instance-qualified local recursion. Verdict:
  BLOCKED-ON T-oracle-4 (reject-vs-silent-full-materialize, = Concern 2 at Stage
  D); A-corr-3 (recursion×detach) required. V-PI vs V-CW realization choice.

## The four desired-state docs (Phase 4)

- **df-stage-a-desired-states.md** — the `.df`/`.contract` dump after Stage A.
  SOUND-WITH-AMENDMENTS. §4.2 census line corrected 2026-08-02 per RDA-C1
  (`views=20 contracts=20 na=9`; brief Errata-4 RESOLVED). AI-1 = in-`.df` vs
  separate-sink decision.
- **regional-dump-stage-b-desired-states.md** — the new `-region-out` dump
  (grammars G1/G2/G3 all rendered on join_1). SOUND-WITH-AMENDMENTS (the row-
  contract set/count needs a specified oracle). AI-2 = the grammar decision.
- **rel-stage-c-desired-states.md** — the `.rel` dump after Stage C (request-edge/
  lifecycle ops). SOUND-WITH-AMENDMENTS on the body; BLOCKED-ON the tc `.rel` pin
  (C2/AI-4 = Concern 2). N1 double-write is a required fix; AI-5 = the
  `differential=` flag.
- **header-stage-c-desired-states.md** — the generated header after Stage C
  (RegionalCursor, move-only lease, deleted retract). SOUND-WITH-AMENDMENTS. AI-6 =
  the pivotal owner-identity-vs-refcount schema decision; AI-7 = A2–A5.

## The critique + audit reports (Phase 3)

- **phase3-critique-report.md** — per-stage findings (59 surviving / 23 refuted),
  refuter-adjusted severities, amendments per finding, and the cross-stage roll-up
  (5 blocking findings; Stage B the only clean stage).
- **phase3-coverage-audit.md** — the completeness ledger: §11 validators (5
  ORPHAN), §15 invariants (3 caveated), §12.3 witnesses (2 UNHOMED), §14 deletions
  (0 ordering violations), X1–X4 cross-stage inconsistencies.
- **necessity-audit.md** — the complexity/necessity lens: mechanism verdicts + 5
  ranked simplification candidates. Headline: NO legacy-two-authority or
  unnecessary mechanism survived; every candidate folds into a stage diff (smaller-
  not-larger holds).

## The desired-state critique (Phase 4)

- **phase4-critique-report.md** — refute-verified critique of the four desired-
  state docs, with the collected owner-facing ADJUDICATION INPUTS AI-1..AI-8.

## The test matrix (Phase 5)

- **test-matrix-proposal.md** — the gate×mode covering array (13 configs, complete
  strength-2 CA over 8 PassPolicy gates) + the feature-mixing directed corpus (14
  crossings / 15 cases), EMPIRICALLY verified at tip. Carve-out A (kvindex_1/canon-
  off), carve-out B (df.dfe SIGABRT, PREDICTION-FAILED), PF-1..PF-4, the 9-case
  pre-Stage-A landing set.

## Persisted evidence (promoted from session scratch, 2026-08-02)

- **passpolicy-gates.md** — the authoritative PassPolicy gate table (8 gates:
  registry name, gated site, what disabling skips, default, mode composition;
  df.sink vestigial; ApplyDemandTransform deliberately un-gated).
- **covering-array-verified.md** — the full covering-array design WITH per-claim
  empirical verification (12 VERIFIED / 2 PREDICTION-FAILED / 6 unverifiable-
  today), incl. the df.dfe SIGABRT evidence and the kvindex_1 canon-off
  uniqueness sweep. `test-matrix-proposal.md` is the synthesized deliverable;
  this is its evidence base.
- **feature-mixing-verified.md** — the 15-case feature-mixing design with
  per-case compile/reject verification and the reject-site corrections (PF-3/
  PF-4 evidence).
- **probes-feature-mixing/** — the 15 verified `.dr` probe programs exactly as
  compiled during verification; the starting point for the pre-Stage-A landing
  set (brief D3.3).

## The standing review (Phase 0 / prior session)

- **fable-review-2026-08-01.md** — the first review: the four concerns
  (interpreter-before-cutover, tagged-binary oracle, inadmissible-extraction
  semantics, extract-always-is-a-cost-policy, routed-result realization) + the
  recommended ranking. The seed the whole session grounds.

## Session 3 (2026-08-03) — region-model grounding + I0 landed

- **regional-arch-pseudocode.md Part R** — the fleet-verified region-model
  current-architecture pseudocode (R.1.1-R.1.7 + the drift ledger: 1 broken,
  10 drifts; the kInstanceDeath gate, LowerDRRounds loop-carried state, and
  InstanceStore two-buffer model corrections are load-bearing). Supersedes
  region-model-pseudocode-seed.md Part 1.
- **region-model-diffs.md** — DIFF-R1..R6 formalized (implementer-grade, on
  Part R) + per-diff 4-lens panel records with code-refuted verdicts and
  normative amendments. Supersedes the seed's Part 2. Panel survival rate
  flagged un-triaged.
- **region-model-desired-states.md** — the desired region-model IR output
  states (.rel request-edge family, G1 -region-out blocks, DOT cluster_region
  twins, D2.6-paused header stub) authored from fresh carrier dumps, with the
  determinism-critique ledger applied. The Stage-C predict-then-verify targets.
- **stage-i0-interpreter.md §7/§8** — the dated amendments (OG1-parsed /
  OG2-tool / OG4-carve ratified; D1.3 .probes contract; the plain-compile CBF
  adjudication) and the EXIT-GATE RECORD: I0 LANDED, 63/63, suite 190 green
  with run_refinterp live, F29 promoted+fixed.
- **owner-adjudication-record.md** (appended) — the adornment-fuzzing owner
  direction (placement-enumeration harness + bracket parser obligations).

## Later landed-state and implementation evidence (2026-08-03–05)

These files describe intermediate compiler states and the tests that landed
with them. They are useful for locating current code and deciding which tests
to rewrite, but `next-session-prompt.md` supersedes their key/adornment,
fallback, and stage-sequencing semantics.

- **stage-b-seed.md** and **stage-b-landed-seed.md** — pre-landing and landed
  whole-pipeline views of the degenerate `FrozenRegionalProgram` layer.
- **key-pragma-landed-seed.md** — landed architecture after `@demand` became
  `@key` and pragma-selected nested lowering existed.
- **k1-multikey.md** — grounded implementation record for repeated `@key`
  pragmas, unordered set-of-sets validation, multi-adornment demand coupling,
  and its golden corpus. Its parser/code census remains evidence; its semantic
  coupling is superseded.
- **k6-riders.md** and **k6-landed-seed.md** — redeclaration, diagnostic-range,
  DOT, and referee riders plus the post-K6 pipeline. The immediately-previous-
  redeclaration implementation documented here is the source of the remaining
  keyed/unkeyed/keyed consistency gap.
- **k5-provenance.md** and **k5-landed-seed.md** — Tier-2 origin-declaration
  provenance design, landed evidence, and the post-K5 pipeline.
- **s9-landed-seed.md** and **s9-mint-sloc-diag-formulation.md** — post-session-9
  compiler/referee architecture and deferred diagnostic-source-location work.
- **s5-mint-tags.md**, **s5-mint-tags-table.md**, and
  **s5-mint-tags-desired-states.md** — the stable DataFlow mint-tag design,
  complete site table, and predicted dump deltas. Mint tags remain debugging
  metadata, never semantic identity.
- **running-example-disassembler.md** — recursive-disassembler design terrain,
  including pivot splits and activation-call-graph examples. Re-evaluate its
  old bracket/request terminology through the supersession matrix.
- **region-model-pseudocode-seed.md** — superseded seed for the region-model
  architecture. `regional-arch-pseudocode.md` is the later executable census;
  neither overrides the current target semantics.
