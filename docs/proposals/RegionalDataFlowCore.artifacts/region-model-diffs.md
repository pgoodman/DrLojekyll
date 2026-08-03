# Region-model diffs — DIFF-R1..R6 FORMALIZED (2026-08-03, session 3)

The rigorous formalization of `region-model-pseudocode-seed.md` Part 2, authored
against the fleet-verified Part R of `regional-arch-pseudocode.md` under the
standing method: opus authorship -> 4-lens adversarial critique (correctness/
lifecycle, termination/confluence, testability/oracle, necessity) -> code-
grounded refutation. Each section below is the AUTHORED diff followed by its
PANEL RECORD (only CONFIRMED/SOFTENED findings survive; their final amendments
are normative and MUST be applied before any implementation of that diff — the
standing amendment rule). Where a STOP marker names an owner decision,
authoring of that hunk pauses there. NOTE on panel yield: the survival rate
(~95%) is far above Stage A's panel (~22%) — the refuters confirmed liberally;
treat CONFIRMED severities as un-triaged and re-triage at implementation time.

Supersedes the seed's Part 2 as the diff authority. DIFF-R5 was re-authored
after a structured-output loss on the first pass; its panel record is from the
re-run.


---

## DIFF-R1 — The explicit REQUEST-EDGE / region-call node (formalized 2026-08-03)

Authored against `regional-arch-pseudocode.md` **Part R** (fleet-verified, `keyed-instances` tip 2026-08-03). Expands `region-model-pseudocode-seed.md` Part 2 DIFF-R1 into an implementer-grade diff. House style: hunks quote the exact Part-R lines they modify; anchors verified against the tree this pass; the single-pass rule applies (re-verify before building).

**Goal.** Make the region call a FIRST-CLASS DataFlow-IR node instead of (a) an implicit guard-JOIN over a `demand__` relation (Part R R.1.1 Step 8) plus (b) a `RecognizedSubgraph` satellite record (`include/drlojekyll/DataFlow/Query.h:1027-1033`). Notation (owner 2026-08-03): `rel[Bound...](Free...)` — bracket = instance key (the matrix row index / InstanceStore key), parens = answer schema.

**Minimal-cut thesis (complexity is guilty).** The seed reads as a wholesale replacement of *every* guard JOIN. That over-specifies. Part R shows the demand pass mints **two structurally distinct** guard families per adornment: the **body** GuardSites (Step 7, three kinds, against `d_reader`) and the **one** query-projection guard (Step 8, against `raw_seed`). Only the query-projection guard is the wrapper→region **call**; the body guards are the region's **interior** demand propagation. DIFF-R1's smallest sound cut is therefore: **promote the query-projection guard (Step 8) + its `RecognizedSubgraph` satellite to ONE `QueryRequestImpl` node; leave the body GuardSites (Step 7) untouched as region interior** (their reclassification into self-feedback vs cross-region call is DIFF-R2). The node is a pure re-homing of two already-landed lowerings (flat guard-JOIN, nested InstanceStore) from a satellite trigger onto a graph node — provably answer-identical, hence I0/eqgate-checkable rather than a semantic change.

### Diff map

| Hunk | Part-R anchor | Real site | Kind |
| --- | --- | --- | --- |
| R1-a | R.1.4 node family / §5 | `lib/DataFlow/Query.h:253-263, 362-365` (+ new `QueryRequestImpl`) | + node kind |
| R1-b | R.1.1 Step 8 | `lib/DataFlow/Demand.cpp:1086-1127` (raw_seed mint) | mint-site replace |
| R1-c | R.1.1 Step 8b / R.1.5 §A | `include/.../Query.h:1027-1033` (`RecognizedSubgraph`) | node subsumes satellite |
| R1-d | R.1.2 (BuildSubgraphInstanceOps) | `lib/Rel/Rel.cpp:1036,1052,1161` | lowering re-parents onto node |
| R1-e | R.1.7 cut predicate | `lib/Rel/Rel.cpp:1571-1572` | cut-successor keys on node kind |
| R1-f | R.1.4 dumps | `-region-out` (new), `-contract`/`-rel` census | dump surfaces (phase d) |

---

### R1-a — the node kind (diff on R.1.4 node family, realizing §5 first-class-ness)

New `final` subclass in the `QueryViewImpl` family, forward-declared alongside the other node kinds (`lib/DataFlow/Query.h:254-263`) with an `AsRequest()` virtual mirroring `AsSelect`/`AsTuple`/`AsJoin` (`:362-365`).

```diff
  class QueryTupleImpl;                                   // Query.h:254
  class QuerySelectImpl;
+ class QueryRequestImpl;                                 // NEW: the region call
  ...
  class QueryViewImpl : public Def<QueryViewImpl>, public User {  // :266
    virtual QuerySelectImpl *AsSelect(void) noexcept;     // :362
+   virtual QueryRequestImpl *AsRequest(void) noexcept;   // returns null by default
  };
+
+ class QueryRequestImpl final : public QueryViewImpl {   // sits by QuerySelectImpl:672
+   // rel[key_cols](answer): a keyed read of `target`'s pub, activating demand.
+   QueryRelationImpl *target;              // the region's pub relation, by
+                                           //   INTENSIONAL reference (the
+                                           //   QuerySelectImpl->relation precedent,
+                                           //   Query.h:672ff); NOT a column edge.
+   // input_columns  = the ORDERED bracket key (= p_bound positions, adornment
+   //                  order; the InstanceStore key tuple / trie order, DIFF-R5).
+   // columns        = the answer schema (Free...); output column edges into
+   //                  the requester (former q_consumer).
+   enum class EdgeKind : uint8_t { kLazy, kForceComplete } edge_kind;  // build-
+                                           //   stamped, immutable; see below.
+   unsigned forcing_index;                 // -> Query::DemandForcings()[i]
+ };
```

**Why a node, not an edge-object (the invariant argument).** The core dataflow invariant (CLAUDE.md) is *"`QueryImpl` owns no conditions … and every inter-view dependency is a column edge."* An edge-object class (a satellite vector on `QueryImpl` keyed by view, the `guard_annotations` shape at `Query.h:1206`) would encode an inter-view dependency that is **not** a column edge — the exact thing the invariant forbids, and the exact F1 shape: `guard_annotation_index` (`Query.h:478-483`) is the satellite that `View.cpp` *must teach CSE to migrate* (the design-question-c cost). A node satisfies the invariant structurally: the key enters as `input_columns` (column edges from the requester's bound columns), the answer leaves as `columns` (column edges into the requester). The **target-rel reference is intensional-by-name** — the `QuerySelectImpl → QueryRelationImpl` precedent — and its dataflow realization is the existing INSERT→SELECT/pub decl seam that `Stratify` already threads (`Stratify.cpp` sources include `{INSERT i : (i,v) is an INSERT→SELECT decl seam}`, Part R R.1.3); no new edge kind is introduced. **REJECT the edge-object alternative** (invariant + F1).

**Why a new subclass, not a keyed `QuerySelectImpl`.** A `REQUEST` is *nearly* "a SELECT with a key," but a SELECT is a **leaf** (no `input_columns`); the optimizer relies on that leaf property broadly. A request consumes bound key columns as inputs (the flat lowering is literally `q_read ⋈ raw_seed`, a JOIN; the nested is `FindOrAddInstance(key)`, a keyed lookup — both take key columns *in*). Forcing input columns onto `QuerySelectImpl` breaks the leaf invariant. **REJECT the keyed-SELECT alternative** (leaf-invariant). *OWNER-GATE R1-1 (mirrors Stage-A O-A1):* Stage A recorded node-minting REJECTED — but that ruling bound a *pure identity refactor with no pipeline change*; Stage C is the cutover whose thesis IS first-class nodes, so O-A1 does not bind here. Confirm the new-subclass realization at Stage-C authoring.

**`edge_kind` identity discipline (the F1-safe half).** `edge_kind` is set once at mint, never mutated, and folded into `QueryRequestImpl::Equals` ONLY — the landed `ProjectionRole` precedent (`Tuple.cpp:308-310` role-mismatch → unconditional CSE refusal; deliberately NOT in `Hash`, per the 2026-08-03 adjudication #1: a Hash fold perturbs order-sensitive tie-breaks for zero CSE benefit since CSE buckets by `cse_color` and decides membership via `Equals`). Consequence: CSE literally cannot fold a `kLazy` request into a `kForceComplete` one; there is no annotation to migrate. **Shrink-the-seed note:** the seed lists `edge_kind ∈ {LAZY, FORCE_COMPLETE}` as if both are minted now. For DIFF-R1's slice `kForceComplete` is **unreachable** — `ApplyDemandTransform` still rejects NEGATE/AGG in a demanded body (Part R R.1.1 Step 3, `Demand.cpp:648-651`), so every minted request is `kLazy`. Declare the domain, inert until DIFF-R4 (the Stage-A `DemandSupportCount` "reserve-early, no producer" pattern).

---

### R1-b — the mint site (diff on R.1.1 Step 8; disposition of all four guard kinds)

Replace the query-projection guard mint. Quoting Part R R.1.1 verbatim:

```diff
-   # Step 8 (:1086-1127): the QUERY-PROJECTION GUARD — a SEPARATE, once-per-
-   #   adornment guard (NOT a body GuardSite): mint a fresh `raw_seed` TUPLE
-   #   over the same receive, then mint guard JOIN  q_read ⋈ raw_seed  pivoting
-   #   p_bound (JOIN-7/TABLE-23, the "raw seed" site — distinct from d_reader).
-   #   stamp GuardAnnotation{kReadAtTuple, side=kRawSeed, role=kQueryProjection}
-   #   pending.append(PendingRewire{q_consumer, q_read, guard, restore=None, kReadAtTuple})
+   # Step 8 (R1): mint ONE region-call node in place of the raw_seed guard:
+   #   req := QueryRequestImpl{ target = pub relation (q_insert's relation),
+   #                           input_columns = q_read cols at p_bound (ordered),
+   #                           columns = q_read's answer schema,
+   #                           edge_kind = InferEdgeKind(q_consumer),  # kLazy in
+   #                                        this slice (NEGATE/AGG still fenced)
+   #                           forcing_index }
+   #   pending.append(PendingRewire{q_consumer, q_read, req.columns, replacement=req})
+   #   (subsumes: the raw_seed TUPLE :1093, the q_read⋈raw_seed JOIN :1105,
+   #    the kRawSeed/kQueryProjection GuardAnnotation :1116, and RecognizedSubgraph
+   #    :1129 — see R1-c. NO raw_seed, NO kRawSeed annotation, NO separate guard.)
```

**Disposition of EACH guard family (the charge's four):**

- **query-projection guard (raw_seed, Step 8, `Demand.cpp:1093/1105/1116`)** → **becomes the REQUEST node.** This is the wrapper→region call. Its `GuardAnnotation{kReadAtTuple, kRawSeed, kQueryProjection}` and the `raw_seed` TUPLE are struck; the node carries their content structurally.
- **body `kReadAtTuple` (Step 7, `Demand.cpp:696`)** → **UNCHANGED region interior.** Member reads pub directly at its own adornment position; stays a `d_reader` guard. DIFF-R2 reclassifies it as same-key feedback iff it reads the recursive pub (X=F).
- **body `kBaseAtom` (Step 7, `:705`)** → **UNCHANGED region interior.** A base rule seeding the region; stays a `d_reader` guard against the message-receive leaf.
- **body `kPushDown` (Step 7, `:751`)** → **UNCHANGED region interior.** The recursive-subgoal push-down JOIN; DIFF-R2 reclassifies it as the cross-region call (X≠F) vs feedback (X=F) off bracket-var identity.

The R-DUP grouped rewire (Part R R.1.1 `:1195-1264`) is **unchanged in mechanism**: a singleton `(consumer, read)` group now rewires the consumer onto `req.columns`; a MULTI group (≥2 adornments sharing a query consumer, D3.a.3) mints a MERGE UNION of the participating request nodes' answer schemas (the flat-arm realization of the reference-counted-union pub). `InferEdgeKind(consumer)` reads consumer monotonicity; in this slice it is total-`kLazy` (a `constexpr` until DIFF-R4).

### R1-c — the node subsumes `RecognizedSubgraph` (diff on R.1.1 Step 8b / R.1.5 §A)

`RecognizedSubgraph` (`include/.../Query.h:1027-1033`) is the satellite record of the region call — `{forcing_index, demanded_view, key_cols, pub_view, guard_annotation_indices}`. Every field is derivable from the node: `forcing_index`/`key_cols`(=`input_columns`)/`pub_view`(=`target`) are node fields; `demanded_view` and `guard_annotation_indices` are the node's transitive body-guard set (found by walk, the way `ResolveLiveRecognition` already re-derives them ABA-safely at `Rel.cpp:934-1023`). DIFF-R1 keeps `RecognizedSubgraph` as a *derived view* of the node initially (the ABA-safe re-resolve is unchanged) and deletes the parallel `recognized_subgraphs` vector only once the DR lowering (R1-d) reads the node directly — a follow-on cleanup, not a DIFF-R1 blocker.

---

### R1-d — the lowering (diff on R.1.2; two-buffer InstanceStore, TouchCurrent, B1 death gate)

The node has **two lowerings**, both already landed; DIFF-R1 re-parents their trigger from `RecognizedSubgraph` onto the node.

**Flat (`-demand`, no `-demand-instance`).** The node lowers back to the guard JOIN it replaced: `req` → `q_read ⋈ raw_seed` pivoting `input_columns` (Part R R.1.5 §D: the demand relation's guard JOINs fire in ordinary strata within the epoch). This is a **pure re-description** — byte-identical to today — and is the primary soundness lever (I0/eqgate can prove flat-REQUEST == flat-guard-JOIN == golden).

**Nested (`-demand-instance`).** `BuildSubgraphInstanceOps` (`Rel.cpp:1036`) iterates the node set instead of `RecognizedSubgraphs()`:

```diff
- for rs in query.RecognizedSubgraphs():        # Rel.cpp:1052, one per DemandForcing
-   ri := lr.by_forcing[rs.forcing_index]
+ for req in query.RequestNodes():              # one QueryRequestImpl per call
+   ri := lr.by_forcing[req.forcing_index]      # ResolveLiveRecognition unchanged
    pub, demand, input := ri.pub_table, ri.demand_table, ri.input_table
    ...
    # kSubgraphInstantiate: birth/rebuild + band-(b) publish (UNCHANGED body)
```

The runtime realization is Part R's corrected **two-BUFFER** `InstanceStore` (double-buffered nested `Table<RowT>` per dense iid — NOT the seed's "two-word cells," which borrowed `StateCellStore`), driven per activation:

- **band a1 birth** — `FindOrAddInstance(Key{k})` (`InstanceStore.h:109`) over the demand net-additions frontier; `if !TouchedFlag(iid): emit_instance_rescan(k)` (Part R R.1.6).
- **the rescan mold** — `cur := TouchCurrent(iid)` (`:134`, the PUBLIC band-(a) entry = private `Touch` + return the current buffer), then `for s in input_member.rows: if row.key==keyexprs and (!input_diff or input_member.Present(s)): cur.TryAdd(...)`.
- **band a2 / a2'** — `FindInstance` (`:103`, NON-adding) rebuild on input net-adds / net-removals; a2' present iff `input_diff` (TWO DRAINS, NO RECYCLE, R-A2-TRIGGER).
- **band b publish** — over `Touched()` (`:148`, sort-uniqued), OVERDELETE-first, `Current(iid)` vs `Frozen(iid)` (`:142/143`) diff into pub's counters/queues.
- **`kInstanceDeath` — the B1-corrected gate.** Mint iff `demand && TableIsDifferential(demand)` (`Rel.cpp:1161-1174`) — i.e. `-demand-retract` ALONE. **`input_diff` is a SEPARATE axis** gating only band-a2' inside `InstantiateEffects`; the seed's "input differential AND demand_retract" is Part R's single BROKEN claim (B1). The death arm's runtime form is `RecycleCurrent(iid)` (`:222`, `Touch` + `current.Reset()`) over the demand removal frontier; band-b then retracts the whole frozen set.
- **`kInstanceSeal`** — always minted, self-lowered at the region tail (`Seal()`, `:180`, pointer-swap current↔frozen; R-MONO belt asserts `frozen ⊆ current`).

**STOP — the per-activation FIXPOINT is NOT DIFF-R1.** The seed says the node "lowers to `FindOrAddInstance(key)` + the activation's fixpoint (LowerDRRounds, per instance)." Part R corrects two things: (1) `LowerDRRounds` is the **stratum** machinery, not per-instance; the per-view induction (`view_to_swap_vec`) is Stage-B's eager descent (R.1.3 Stage B), not this. (2) **No recursive region is nested-lowered today** — `demand_cyclic_1` / recursive-content are fenced at the `Program::Build` pre-pass (`Build.cpp:1451-1499`). So DIFF-R1's nested lowering covers only the **acyclic** recognized subgraph (the landed `demand_neighborhood` family). The interior per-activation fixpoint (X=F feedback / X≠F cross-region descent) is **DIFF-R2**, gated on D2.12 (V-CW coupled-vs-disjoint). Do not author it here.

### R1-e — cut-successor keys on the node kind (diff on R.1.7)

`IsCutSuccessorDR` (`Rel.cpp:1571-1572`) today cuts the eager walk at `context.demand_instance_enabled && succ.GuardAnnotationIndex() != kNoGuardAnnotation`. Under the node, the cut keys on the node kind directly: `succ.AsRequest() != nullptr` (a request node is fed by `SUBGRAPH_INSTANTIATE`, not the flat web). Coverage must be identical — SD-4 (Part R R.1.7, the always-on walk-vs-derivation oracle, `fprintf+abort`) is the gate: the region-model walk must reproduce the guard-annotation walk's reachable-view set exactly.

### R1-f — dump / DOT surfaces (names only; desired states DEFER to phase d)

Name the surfaces the node must render on; the exact tokens/goldens are phase (d):
- **`-region-out`** (D2.2 G1: indented region/port/contract block) — the request node renders as `rel[Bound...](Free...)` with an inferred `edge_kind`; the `request-edges{…}` multiset may earn permcheck order-free diffability at Stage C.
- **`-region-out` DOT twin** (owner directive 2026-08-03) — request edges as **inter-cluster** edges between `subgraph cluster_region_<id>` boxes; advisory, never byte-goldened (the G1 text stays the referee).
- **`-rel-out` / `-contract-out` census** — the whole-corpus fixed-width `.rel` census re-bless (D2.4: 29→36 kinds) picks up the request-edge lifecycle kind, paired with `V-REGION`/`V-LIFECYCLE-CENSUS` recounts.

---

### SOUNDNESS OBLIGATIONS

1. **Answer-identity (the master obligation).** flat-REQUEST == today's flat guard-JOIN == nested InstanceStore == golden, for every bound-query corpus case and every eqgate witness. Referee: `runall.sh` byte-compare (4 modes) + the `.eqgate` family + I0 (D1.1). A request-node lowering that diverges is a bug, not a feature.
2. **V-EDGE-BALANCE (D2.1, RATIFIED).** Every `kRequestEdgeAdd` (demand seed) is balanced by a matching removal (`kInstanceDeath`). **Permanent-root carve-out (corr-3):** `PermanentRoot` is a `kRequestEdgeAdd` source; add-only permanent edges (all-free `rel[](Free)` / non-demanded relations, ADJ-3) are carved OUT of the balance check. Authored with the §12.3 permanent-root witness.
3. **`kInstanceDeath` gate (B1).** Death is gated on `TableIsDifferential(demand)` (== `-demand-retract`), never on `input_diff`. Belt: the anti-conflation comment at `Rel.cpp:1072-1077` stays; `demand_diff_pub_1` / `demand_diff_input_1` pin the split.
4. **V-PORT-AGREE (§11 line 5, D2.1).** The node's answer schema (`columns`) agrees with the target region's pub schema (parent/child frozen-port agreement).
5. **V-PURE-REGION (§11 line 9, D2.1).** The reached region is pure (no symbolic-parameter escape / sealed-ABI mutation). **Softening (lines 3/6, D2.1):** those two escape classes *decline extraction into full materialization* — no hard-fail validator; the request simply routes to §6 full-materialize.
6. **F1 / identity.** `edge_kind` folded into `Equals` only (not `Hash`); the node is a graph node, not a satellite — zero CSE-migration code (design-question-c).
7. **V-INST-SOLE re-key (D3.a.3).** N request nodes over one shared pub key on `(pub_table, forcing_index)` (`Rel.cpp:4977`), so multi-adornment shares one pub without tripping the belt.
8. **SD-4 coverage.** The node's cut-successor status (R1-e) reproduces the guard-annotation walk's reachable set exactly (always-on `fprintf+abort`).

### OWNER DEPENDENCIES

- **D2.6 — DEFERRED. → STOP (the reader-handle schema).** The node's answer-read row schema — whether a request edge carries an owner id (`RowReq{owner,key}`, individually retractable leases) or a refcount (anonymous handle count) — is the arrangement reader-handle question. DIFF-R1 can mint the node and lower it flat (no lease schema) and nested for the **single-reader** case (today's `InstanceStore`, refcount-free). **STOP:** the MULTI-READER shared-arrangement lowering (concurrent same-key requests outliving an epoch; the hoist-vs-nest shared block-area) cannot be authored until D2.6 answers the concurrency requirement. Stage-C header authoring pauses here per the charter; the flat + single-reader nested slice proceeds.
- **§6-vs-§11 routing rule — OPEN (D1.5/D1.6, Variant B). → STOP (per bound demanded query).** For every bound `#query`, whether its request edge routes to a full materialized relation (§6) or a region (§11) must be defined at Stage-C authoring. Recursive demanded content stays an expected-diagnostic reject (Variant B; `demand_cyclic_1` / `demand_recursive_content_1`). **STOP:** the `target`/`edge_kind` of a request whose demanded content is recursive is undefined until the routing rule lands; DIFF-R1 mints request nodes only for the acyclic-admissible set the demand pass already recognizes.
- **D2.12 — V-CW, RATIFIED (V-CW-first, owner-gated).** The coupled-vs-disjoint per-activation fixpoint. **Handoff, not STOP:** DIFF-R1's acyclic slice needs no interior fixpoint; the recursive lowering is DIFF-R2, which discharges V-CW via the nonlinear-TC witness (independent→disjoint-union / coupled→widened; X≠F cross-instance-read is the discriminator).
- **D2.1 — RATIFIED.** V-EDGE-BALANCE + permanent-root carve-out + V-PORT-AGREE + V-PURE-REGION land as the Stage-C H-I validator set (obligations 2/4/5).
- **D2.2 / D2.4 / D2.9 — RATIFIED.** G1 `-region-out` (R1-f); the 36-kind census recount (R1-f); the enum-in-`Equals`-identity precedent for `edge_kind` (R1-a).
- **DIFF-R3 / DIFF-R4 — complementary, non-blocking.** Declared brackets (region checking vs inference) and lazy/force-complete typing (the NEGATE/AGG fence lift) build on this node; DIFF-R1 reserves `edge_kind` and the `rel[Bound](Free)` shape for them.

### TEST / WITNESS plan (golden-master terms)

- **Flat re-description (the big net).** Every bound-query corpus case (~49 of 181, Errata-2 — re-measure, never propagate) must be byte-identical flat across all 4 modes with the node in place: `demand_tc_witness`, `demand_multi_adorn_witness`, plus the whole `.drflags`-`-demand` subset. Referee: `runall.sh` byte-compare. A single diff = the re-description is not pure = bug.
- **Nested eqgate family (flat == nested == golden).** All standing `.eqgate` carriers must stay green with the node driving `BuildSubgraphInstanceOps`: `demand_neighborhood_witness`, `demand_neighborhood_mono_witness`, `demand_diff_pub_1`, `demand_diff_neighborhood_witness`, `demand_diff_input_1`, `demand_multi_adorn_witness`. Referee: `runall.sh --one` re-compile-nested + byte-compare (the eqgate mechanism), plus sorted published-delta identity through the `@differential` tap.
- **B1 death-gate split.** `demand_diff_pub_1` (diff-pub × diff-demand) and `demand_diff_input_1` (diff-input × diff-demand) pin that death mints on demand-differentiality, not input; `demand_diff_neighborhood_witness` (diff-input × MONO-demand, `kInstanceDeath=0` beside `kSubgraphInstantiate=1`) pins the P-STORE∧¬P-DEATH divergence. Referee: byte-compare + `bin/Oracle` (`.batches`/`.oracle.stdout`).
- **I0 answer-identity (D1.1).** The reference interpreter adjudicates each bound `#query`'s answer set (probe-restricted, `.probes` sidecar) — flat == nested == (declared, later). The request node's flat and nested lowerings must both match I0. This is the referee that makes the "pure re-description" claim checkable rather than asserted.
- **`-region-out` surface pin (phase d).** One `.region` golden per witness (byte-compare, opt-mode-only, the `.contract`/`.irgold` sidecar policy); DOT twin advisory. Defer the exact shape to phase (d); name `demand_tc_witness` + `join_1` (a `kEagerJoin` carrier for a request over a pivot key, DIFF-R5-adjacent) as the first two carriers.
- **Adornment-fuzzing hook (owner 2026-08-03; DIFF-R3-owned, named here).** The placement-enumeration harness (covering-array machinery) exercises `InferEdgeKind` and the node's mint across bracket placements: every placement compiles answer-identical (I0/eqgate) OR draws a clean diagnostic — never a miscompile, never an assert. DIFF-R1's node is a *subject* of that generator; its edge_kind inference is the first thing the fuzzer stresses once DIFF-R3 lands the surface.


### OPEN OWNER ITEMS (DIFF-R1)

- D2.6 (reader-handle schema: owner-bearing RowReq{owner,key} vs refcount) — STOP for the multi-reader shared-arrangement lowering of the request node's answer-read row schema; flat + single-reader nested proceed
- §6-vs-§11 routing rule (D1.5/D1.6, Variant B) — STOP for defining target/edge_kind of any bound query whose demanded content is recursive; must be authored per bound #query at Stage-C
- OWNER-GATE R1-1: confirm QueryRequestImpl as a NEW QueryViewImpl subclass (Stage-A O-A1 node-minting rejection does not bind the Stage-C cutover) vs the rejected keyed-SELCT / satellite-edge alternatives
- D2.12 (V-CW coupled-vs-disjoint) — handoff to DIFF-R2 for the recursive per-activation fixpoint; DIFF-R1 covers only the acyclic slice
- Stage-B re-brief carryover: should proxy mints INHERIT edge_kind/role so set-boundary provenance survives onto the final graph (2026-08-03 adjudication #4)

### PANEL RECORD (DIFF-R1) — 15 findings, 15 survived refutation

- **DIFF-R1-correctness-lifecycle-1** [CONFIRMED; sev BLOCKING] The QueryRequestImpl node has no defined lifecycle through the DataFlow optimization/finalization pipeline it is minted into, so its birth-to-consumption story is unowned; the 'pure re-description / byte-identical flat' framing is unfounded.
  - EVIDENCE: Verified pass order at lib/DataFlow/Build.cpp:2593 (ApplyDemandTransform) -> :2613 Optimize -> ... -> :2638 Stratify -> :2648 InferConservativeRowContracts. R1-b's PendingRewire installs `req` as the surviving replacement view (mirrors the real raw_seed guard install at Demand.cpp:1121-1127), so the node PERSISTS into Optimize/finalization; it is not desugared. The diff supplies only Equals (R1-a) — no Canonicalize/CSE-color/DFE-is_dead/RowContract-transfer/Stratify handling for the new kind. Optimize's per-view Canonicalize fixpoint and RowContract.cpp's TransferContract switch both dispatch per node kind, so an unhandled QueryRequestImpl falls to a default arm of unknown behavior. The 'pure re-description / byte-identical flat' claim (obligation 1) is asserted, not mechanized.
  - AMENDMENT (normative): Before asserting byte-identity, DIFF-R1 must pick and specify the node's pipeline treatment: either (preferred) mint-then-immediately-desugar into today's raw_seed TUPLE + q_read⋈raw_seed JOIN so Optimize/finalization/eager-web see the identical graph and re-recognize post-Optimize (byte-identity by construction), OR fully specify the surviving node's Canonicalize (identity, never drops input_columns, honors keep-last-edge), CSE bucketing, DFE is_dead semantics, RowContract transfer, and FinalizeColumnIDs/Depths behavior, plus a flat byte-identity witness proving parity BEFORE the re-description claim stands.
- **DIFF-R1-correctness-lifecycle-2** [SOFTENED; sev BLOCKING] The node's `target` intensional-by-name pub reference is a real inter-view data dependency (request depends on the pub being materialized) that is invisible to Stratify's column-edge sources() and to DeadFlowElimination reachability; nothing owns the pub-before-request ordering or the pub's liveness.
  - EVIDENCE: Stratify sources (Stratify.cpp:138-172, Part R R.1.3) = predecessors + negated_view + INSERT->SELECT decl seam, so the intensional `target` is indeed not a Stratify source. BUT the request's input_columns trace through q_read over p_merge (Demand.cpp Step 2), a real column-edge predecessor chain to the pub content — so the pub dependency is NOT invisible and NOT rooted solely by `target`; DFE roots it via those edges. The nested ordering is re-derived independently at DR-lowering (Rel.cpp: instance_stratum := 1 + max(ready_after(demand), ready_after(input))), and the flat path re-expands to the guard JOIN restoring the raw_seed edge. Sibling finding termination-confluence-2 concedes 'this is not a live miscompile.' The BLOCKING 'mis-stratified request reads a not-yet-populated pub' hazard is therefore not live.
  - AMENDMENT (normative): Downgrade to a documentation/invariant obligation: state that the pub<->request ordering is re-materialized at DR-lowering (nested via instance_stratum keyed on forcing_index; flat via the re-expanded q_read⋈raw_seed) and that the request's input_columns already carry the pub dependency as column edges, then assert explicitly that no DataFlow-level Optimize/Stratify decision reads the request node's own view->stratum. Not a blocking correctness bug.
- **DIFF-R1-correctness-lifecycle-3** [CONFIRMED; sev BLOCKING] Flat mode (`-demand` without `-demand-instance`) has no lowering path for the request node: R1-e's cut change plus the demand_instance gating leaves the flat eager web either crashing on an unhandled node kind or silently under-producing answers.
  - EVIDENCE: Verified IsCutSuccessorDR (Rel.cpp:1567-1572): in flat mode context.demand_instance_enabled is false, so it returns false and the eager walk descends THROUGH guard JOINs. BuildSubgraphInstanceOps is gated on demand_instance_enabled (Rel.cpp head, Part R R.1.2), so it never runs flat. R1-e rewrites the cut to succ.AsRequest(); the eager dispatch BuildEagerRegion (Build.cpp:1208-1323, Part R R.1.7) has arms for SELECT/TUPLE/JOIN/MERGE/MAP/CMP/NEGATE/INSERT but NONE for a request kind. A persistent request node in flat mode therefore either is not cut and hits no dispatch arm (crash), or is cut with no kSubgraphInstantiate feeder (under-answer). The diff names no pass performing the asserted flat re-expansion to the guard JOIN.
  - AMENDMENT (normative): Explicitly define the flat path. Preferred: adopt lifecycle-1's pre-Optimize desugar so no request node reaches the flat pipeline. Otherwise (1) add a request dispatch arm to BuildEagerRegion + BuildDREagerInventory emitting the q_read⋈raw_seed guard JOIN, (2) keep IsCutSuccessorDR NOT cutting request nodes when demand_instance_enabled is false, and (3) add a flat-mode (.drflags -demand) byte-identity witness with the node in place before claiming purity.
- **DIFF-R1-correctness-lifecycle-4** [CONFIRMED; sev MAJOR] R1-c's 'the node subsumes RecognizedSubgraph' is only half-realized: the nested lowering's actual authority remains the live kBody guard JOINs via ResolveLiveRecognition, which R1-b explicitly leaves intact, creating a dual source of truth with no consistency owner.
  - EVIDENCE: Verified ResolveLiveRecognition (Rel.cpp:934-1023): demand_table from any guard's joined[0] (:972), input_table/input_view/input_key_cols/demanded_view ONLY from a role==kBody guard's joined[1] (:977-984), pub_table from query.Inserts() matched on forcings[fidx].query Id() (:990-1012). None read the query-projection (raw_seed) guard R1-b replaces. R1-b explicitly leaves the kBody GuardSites untouched. In BuildSubgraphInstanceOps (Rel.cpp:1052) the RecognizedSubgraph rs is used ONLY as a forcing_index driver; every table/key comes from ri (the kBody-guard re-resolution). So the node contributes only forcing_index and does NOT subsume recognition — the kBody guards remain the authority, a dual source of truth the diff gives no validator for.
  - AMENDMENT (normative): Either make the request node the SOLE nested-recognition authority (rewrite ResolveLiveRecognition to read demand/input/pub from req.target + req.input_columns + req.forcing_index, dropping the kBody-guard dependency), or document that kBody guards stay authoritative and add an always-on V-REQ-XCHECK asserting a live request node exists iff a kBody-derived ResolvedInstance exists for that forcing_index (with agreeing key_cols/pub). Also specify that R1-d's `for req in query.RequestNodes()` skips is_dead request nodes (matching the ABA-safe fully-dead-forcing skip at Rel.cpp:1054).
- **DIFF-R1-termination-confluence-1** [CONFIRMED; sev MINOR] The diff's central termination thesis — "DIFF-R1's acyclic slice needs no interior fixpoint" (STOP block, and R2-handoff on D2.12) — is load-bearing on the Program::Build pre-pass fence at lib/ControlFlow/Build/Build.cpp:1451-1499, yet that fence appears in no SOUNDNESS OBLIGATION or coverage entry; R1-e's only walk-coverage obligation is IsCutSuccessorDR/SD-4, which guards eager-walk reachability, NOT finiteness/anti-coupling. R1-b simultaneously removes a view (the query-projection guard) that this very fence enumerates.
  - EVIDENCE: Verified the finiteness/anti-coupling fence at Build.cpp:1451-1499: groups views by GuardAnnotationIndex per forcing_index, rejects cyclic_demand := ViewSelfReachable(jl[0]) (:1487) and recursive_content (role==kBody gated, :1476-1485). R1-b removes a GuardAnnotationIndex-carrying view (the query-projection guard). The finding correctly verifies the removal is inert: MintGuardJoin adds demand_side first (Demand.cpp:167, joined_views.AddUse(demand_side) before read), so the query-projection guard's jl[0]==raw_seed (a fresh TUPLE over the demand receive, never self-reachable) and its role==kQueryProjection (Demand.cpp:1116) != kBody. So no live defect — but this load-bearing fence appears in no obligation, and its inertness rests on two unstated invariants.
  - AMENDMENT (normative): Add an obligation naming Build.cpp:1451-1499 as the finiteness/anti-coupling authority DIFF-R1 relies on, and record the inertness invariant: the removed view is role==kQueryProjection (never kBody) with jl[0]==raw_seed (non-self-reachable, guaranteed by MintGuardJoin's demand-side-first operand order at Demand.cpp:167), hence inert for both fence arms. Cite demand_cyclic_1 / demand_recursive_content_1 as the standing witnesses the fence must keep rejecting after the node lands.
- **DIFF-R1-termination-confluence-2** [CONFIRMED; sev MINOR] R1-b's node drops the raw_seed->demand-receive predecessor column edge that the query-projection guard JOIN carried into Stratify, but R1-a's stratification-preservation argument only covers the target/pub INSERT->SELECT decl seam and never addresses the lost demand-seed->region ordering edge that fed the DataFlow-level Stratify SCC / view->stratum and, transitively, DeriveDRStrata's V-READY schedule.
  - EVIDENCE: Confirmed the query-projection guard JOIN carries raw_seed (a read of the fabricated demand receive) as joined[0] (Demand.cpp:1104-1105, MintGuardJoin demand_side-first), a real DataFlow predecessor giving Stratify a demand-relation->region edge. The request node's DataFlow predecessors are only input_columns (q_read over p_merge/pub) plus the intensional target (R1-a: 'NOT a column edge'); nothing ties the node to the demand receive at the DataFlow level. R1-a's stratification argument addresses only the target/pub side. As the finding states, the ordering is recovered at DR-lowering (nested instance_stratum keyed on forcing_index; flat re-expansion restores raw_seed) so it is not a live miscompile, but the diff never discharges the dropped demand-seed edge.
  - AMENDMENT (normative): Extend R1-a's stratification argument to explicitly discharge the demand-seed edge: state that the demand-relation->region ordering is preserved not as a DataFlow column edge but re-materialized at DR-lowering (flat via the re-expanded q_read⋈raw_seed, nested via instance_stratum := 1 + max(ready_after(demand), ...)), and assert no DataFlow Stratify/Optimize decision reads the request's view->stratum in a way the dropped edge would change. Pin with the V-READY/V-LINEAR schedule cross-check on demand_tc_witness (flat) and demand_neighborhood_witness (nested).
- **DIFF-R1-testability-oracle-1** [CONFIRMED; sev MAJOR] DIFF-R1's slice is answer-identical by design AND defers every structural dump surface (`-region-out`/`.region`, the 29→36 census recount) to phase (d), so no referee in the slice's own committed test plan can distinguish 'QueryRequestImpl minted and driving the lowering' from 'node minted-but-dead, old guard-JOIN / RecognizedSubgraph path still carrying the answer'.
  - EVIDENCE: Verified all named referees (runall.sh byte-compare, .eqgate flat==nested==golden, bin/Oracle, I0) observe ANSWERS. R1-c keeps RecognizedSubgraph as a parallel derived view and defers deleting recognized_subgraphs. The independent census cross-check (Rel.cpp:3994-4009) re-derives from query.RecognizedSubgraphs() via ResolveLiveRecognition, NOT from the node. necessity-5 shows the node contributes only forcing_index — identical to the satellite — so node-driving and satellite-driving are answer-equivalent by construction. Confirmed no kRequestEdge* DR op kind exists (Rel.h enum: kSubgraphInstantiate=15, kInstanceDeath=16), so the .rel census (29 kinds) carries zero request kinds and R1-f defers the 36-kind recount to phase d. No in-slice referee positively shows the node is live.
  - AMENDMENT (normative): Pull ONE positive-presence referee into DIFF-R1 rather than deferring all to phase d: either (a) emit the request-edge census kind and re-bless demand_tc_witness's .rel goldens as part of R1-b/R1-d, or (b) add an always-on fprintf+abort cross-check reconciling RequestNodes().size() with the demand-forcing count AND asserting the DR mint loop consumed the node set (mirroring the recognized_subgraphs==demand_forcings census at Demand.cpp Step 11). Without one, 'green' cannot mean 'the node works.'
- **DIFF-R1-testability-oracle-2** [CONFIRMED; sev MAJOR] The B1 death-gate split / P-STORE∧¬P-DEATH divergence that obligation 3 and the test plan lean on ('demand_diff_neighborhood_witness pins kInstanceDeath=0 beside kSubgraphInstantiate=1') is not pinned by any committed golden, and its named referee (stdout byte-compare + oracle) is answer-level and structurally blind to whether kInstanceDeath was minted.
  - EVIDENCE: Verified via directory listing: demand_diff_neighborhood_witness, demand_diff_pub_1, demand_diff_input_1 carry batches/drflags/eqgate/cpp/probes but NO .irgold sidecar, so they never enter the .rel census golden path; demand_neighborhood_witness likewise has no irgold. Only demand_tc_witness has .irgold and it compiles flat (kSubgraphInstantiate=0). The death gate is confirmed at Rel.cpp:1163 (demand && TableIsDifferential(demand)). For the MONO-demand witness kInstanceDeath=0 regardless of whether the store built, so stdout+oracle cannot observe whether a re-parent preserved the death-mint gate structurally.
  - AMENDMENT (normative): Add a nested-arm structural referee for the three diff cases: either an .irgold census golden compiled under -demand-instance (extend census-golden compilation to the nested arm), or explicitly name the C++ RelValidators (DeathFrontierTest/InstanceEffectsTest) as the death-gate referee and add the three witnesses to their inputs. State in obligation 3 that stdout+oracle do NOT discriminate the mono kInstanceDeath=0 case.
- **DIFF-R1-testability-oracle-3** [CONFIRMED; sev MAJOR] Obligation 2 (V-EDGE-BALANCE) and its permanent-root carve-out have no constructible subject or positive witness in DIFF-R1's flat+single-reader slice: the `kRequestEdgeAdd` op it validates does not exist in the tree, and the permanent-root witness the carve-out is 'authored with' is UNHOMED with its natural all-free carrier being a hard diagnostic reject (H-A9 unconstructible-witness trap).
  - EVIDENCE: Verified no kRequestEdge* DR op kind in Rel.h (enum tops out at kInstanceDeath=16); the demand seed is minted at RUNTIME by the forcer proc (BuildQueryInjectorFromRegistry, Part R R.1.5), not as a compile-time DR op, so a compile-time V-EDGE-BALANCE has no subjects in this slice. owner-adjudication-brief.md:208-217 records §12.3 permanent-root as UNHOMED (row 11) and corr-3 as still OWING the witness ('author the §12.3 permanent-root witness'). The natural all-free carrier demand_multi_adorn_allfree_1 is an all-4-modes diagnostic reject (runall.sh:497). So obligation 2's 'V-EDGE-BALANCE ... RATIFIED, Authored with the §12.3 permanent-root witness' asserts a witness that does not exist and whose carrier rejects.
  - AMENDMENT (normative): Remove obligation 2 from DIFF-R1's discharge set (it belongs with the phase-d census kinds / DIFF-R4) OR, if it must stay, name a concrete COMPILING program producing a permanent-root request edge plus a referee observing the carve-out branch. Do not list V-EDGE-BALANCE as 'RATIFIED, authored with the §12.3 permanent-root witness' while owner-adjudication-brief.md still records that witness UNHOMED and corr-3 owing it.
- **DIFF-R1-testability-oracle-4** [CONFIRMED; sev MAJOR] Obligation 8 mis-attributes a cross-version coverage-equivalence guarantee to SD-4. SD-4 cross-checks the two authorities that BOTH read the post-change cut predicate, so it cannot detect a divergence between the new `succ.AsRequest()`-keyed cut (R1-e) and the old `GuardAnnotationIndex()`-keyed cut — it only guarantees internal walk==derivation consistency under whichever predicate is current.
  - EVIDENCE: Verified IsCutSuccessorDR (Rel.cpp:1567-1572) is the single cut authority; its doc-comment (Rel.cpp:1544-1556) confirms both authorities (the walk and BuildDREagerInventory) read this same predicate, and Part R R.1.7 states SD-4 asserts derived_keys==walk_keys — internal agreement under the CURRENT predicate. R1-e deletes the GuardAnnotationIndex-keyed cut and replaces it with AsRequest, so SD-4 cannot compare new coverage against the deleted old coverage; it only guarantees walk==derivation under whichever predicate is live. An under/over-cut that keeps answers correct leaves SD-4 green, and because R1-e's change is demand_instance-gated it shows in no flat .rel census golden while the nested census is goldened nowhere (oracle-2). Obligation 8's 'reproduces the guard-annotation walk's reachable set exactly' overstates SD-4.
  - AMENDMENT (normative): Reword obligation 8 to state SD-4 provides internal walk==derivation consistency only, then name the real cross-version referee: land a temporary old-vs-new cut cross-check alongside R1-e (assert succ.AsRequest()!=nullptr iff succ.GuardAnnotationIndex()!=kNoGuardAnnotation over the walked set, before the satellite retires), or add a nested-arm .rel census golden on the demand_neighborhood witnesses under -demand-instance so a coverage delta is observable structurally.
- **DIFF-R1-necessity-1** [CONFIRMED; sev BLOCKING] The flat 'pure re-description, byte-identical' claim is unsound under the diff's own mint/lower sequence: minting the request node BEFORE Optimize (deleting the raw_seed TUPLE + q_read⋈raw_seed JOIN at R1-b) and reconstructing that JOIN only at DR/codegen time forfeits the very CSE fold today's flat graph depends on.
  - EVIDENCE: Verified: ApplyDemandTransform (Build.cpp:2593) runs before Optimize (:2613), placed there per the comment at :2580-2583 'BEFORE Optimize (so demand relations are folded by the SAME CSE/canonicalize fixpoint)'. The GT-3 fold is documented at Demand.cpp:1112-1114: 'on non-recursive witnesses CSE folds raw_seed into d_reader (GT-3) and the graph alone can no longer tell the two sides apart.' If QueryRequestImpl subsumes raw_seed pre-Optimize and re-expands only at DR-lowering (flat never enters BuildSubgraphInstanceOps, gated on demand_instance_enabled), the GT-3 fold cannot fire and the flat graph diverges from today's, violating obligation 1. The diff never states when the flat expansion happens, leaving both readings (persist-and-diverge / expand-early-and-be-inert) damaging.
  - AMENDMENT (normative): State the flat expansion point explicitly. If the node must survive to be first-class, prove byte-identity by re-running Optimize's CSE on the reconstructed guard JOIN (or expand pre-Optimize and accept the node is flat-ephemeral, rendering target/edge_kind dead flat). Additionally specify that a surviving QueryRequestImpl flows through Optimize/Canonicalize, DeadFlowElimination, and Stratify (all per-kind dispatch); the diff currently supplies only Equals.
- **DIFF-R1-necessity-2** [SOFTENED; sev MAJOR] edge_kind is an inert single-valued field in DIFF-R1 (every minted request is kLazy; kForceComplete has no producer) folded into Equals only — it is consumed by nothing this slice and should be deferred wholesale to DIFF-R4.
  - EVIDENCE: The observation is correct and the diff concedes it: kForceComplete is unreachable because ApplyDemandTransform still fences NEGATE/AGG (Demand.cpp:648-651), so every minted request is kLazy and edge_kind (folded into Equals only) changes zero CSE decisions this slice. But the recommendation to drop it wholesale overshoots: the diff's OWNER DEPENDENCIES record 'D2.9 — RATIFIED. ... the enum-in-Equals-identity precedent for edge_kind', and the Stage-A DemandSupportCount 'reserve-early, no producer' pattern is an already-adjudicated precedent for exactly this inert-domain declaration. Dropping edge_kind contradicts a landed ruling.
  - AMENDMENT (normative): Keep the declared edge_kind domain (owner-ratified reserve-early, D2.9) but right-size the mechanism: since InferEdgeKind is a constexpr-kLazy no-op this slice, inline it to `edge_kind = kLazy` rather than introducing an inference helper, and mark kForceComplete explicitly unreachable-until-DIFF-R4. Do not defer the field itself.
- **DIFF-R1-necessity-3** [CONFIRMED; sev MAJOR] R1-e replaces the GuardAnnotationIndex cut authority with AsRequest(), but that authority still governs the BODY guards R1-b explicitly leaves untouched — so the replacement under-cuts the interior demand reads.
  - EVIDENCE: Verified: R1-b converts ONLY the query-projection guard to a node and keeps the body kReadAtTuple/kBaseAtom/kPushDown guards as GuardAnnotationIndex-carrying JOINs. The current cut (Rel.cpp:1571-1572) fires on GuardAnnotationIndex, which is stamped on BOTH body and query-projection guards, and its doc-comment (Rel.cpp:1547-1553) states a recognized-subgraph guard JOIN successor 'is fed by its SUBGRAPH_INSTANTIATE op, never the eager walk — treat it as a cut successor.' R1-e's 'the cut keys on the node kind directly: succ.AsRequest() != nullptr' reads as a replacement, which would strip cut status from the still-present body guards and walk their inputs eagerly — an internal R1-b<->R1-e contradiction.
  - AMENDMENT (normative): Make R1-e ADDITIVE, not a replacement: cut when `succ.AsRequest() != nullptr || (context.demand_instance_enabled && succ.GuardAnnotationIndex() != kNoGuardAnnotation)`. The GuardAnnotationIndex arm remains the cut authority for the body guards until DIFF-R2 reclassifies them.
- **DIFF-R1-necessity-4** [SOFTENED; sev MAJOR] The node's target (QueryRelationImpl*) duplicates forcing_index: the pub is already derived purely from forcing_index by ResolveLiveRecognition, and the nested lowering never consults target.
  - EVIDENCE: Verified the nested path is forcing_index-keyed: ResolveLiveRecognition resolves pub_table from forcings[fidx].query matched on parse Id() (Rel.cpp:990-1012) and R1-d reads pub off ri := lr.by_forcing[req.forcing_index], never req.target — so target is unused by the nested lowering. BUT the finding's dangling-handle half is misplaced: R1-a types target as QueryRelationImpl* (a RELATION), grounded in the QuerySelectImpl->QueryRelationImpl precedent; relations are Optimize-stable and do NOT dangle the way the stored QueryView handles at Rel.cpp:911-914 do (that hazard is view-specific). And target is the intensional pub identity the flat lowering and the rel[Bound](Free) rendering (DIFF-R5) need. So target is neither purely redundant nor a dangling hazard — a false dilemma.
  - AMENDMENT (normative): Do not remove target on dangling-hazard grounds (relation refs are Optimize-stable, unlike view handles). Instead document that target is the flat-lowering + render intensional identity while forcing_index is the sole nested key, and drop target only if the flat/render paths can also key off forcings[forcing_index].query.
- **DIFF-R1-necessity-5** [SOFTENED; sev MAJOR] R1-d's swap of RecognizedSubgraphs() → RequestNodes() as the nested-lowering iterator delivers zero functional change while adding a parallel enumeration authority alongside the retained satellite — net-additive to authority count, the opposite of the claimed subsumption.
  - EVIDENCE: Verified the iterator swap is answer-inert: in BuildSubgraphInstanceOps (Rel.cpp:1052) the RecognizedSubgraph is used only as a forcing_index driver; all tables/keys come from ResolveLiveRecognition (Rel.cpp:1044), so iterating RequestNodes() vs RecognizedSubgraphs() changes only the loop variable. The census cross-check retains RecognizedSubgraphs() (Rel.cpp:4003-4009), so node and satellite coexist in lockstep. But framing this as 'delivers zero functional change' mischaracterizes the diff's stated goal: answer-inertness IS the eqgate/I0 lever ('pure re-description'), the intended property, not a defect. The genuine residual is the dual-authority lockstep, which overlaps lifecycle-4 and oracle-1.
  - AMENDMENT (normative): Reframe from 'the swap buys nothing' to the real residual (dual-authority lockstep, shared with lifecycle-4/oracle-1): either delete recognized_subgraphs in the same diff and re-home the A.1.5 census cross-check onto an independent re-walk so exactly one enumeration authority exists, or keep the satellite and add the V-REQ-XCHECK consistency validator (lifecycle-4). Do not defer the swap solely on parsimony grounds — the answer-inert re-homing is the intended eqgate-checkable slice.

---

## DIFF-R2 — The pivot split as SYNTACTIC self-vs-call; activation-SCC stratification (formalized 2026-08-03)

Authored 2026-08-03 (session 3) against `keyed-instances` tip, over Part R of
`regional-arch-pseudocode.md` (R.1.2 keyed-instance lowering, R.1.3
recursion/fixpoint lowering). Model document: `stage-a-diff.md`. Single-pass
rule applies — re-verify anchors before building. This diff formalizes the
seed's DIFF-R2 (`region-model-pseudocode-seed.md` Part 2, lines 166-184) and
**shrinks it**: the seed's "build activation-call graph; Tarjan SCC" as a NEW
correctness pass is over-specified. The relation-level `Stratify` SCC
condensation already IS the activation-call-graph condensation projected onto
the template, and the coupled fixpoint is already emitted. What is genuinely
new is (a) the fence-lift, (b) the memo-self-hit realization of X=F, and (c) an
OPTIONAL provable-acyclicity descent specialization deferred to cost tier.

Charge (seed §DIFF-R2, running-example §pivot-split / §matrix-frame / §peel):
read self-recursion (X=F, same-activation feedback) vs cross-region call (X≠F,
a REQUEST to a sibling activation) and stratify the activation graph into
DOWN (memoized descent) vs ACROSS (coupled fixpoint). It sharpens Stage D
(D2.12 / V-CW) and lifts the `demand_cyclic_1` nested fence.

---

### 0. Diff map

| Hunk | Part-R anchor | Real site | Kind |
| --- | --- | --- | --- |
| R2-1 | R.1.3 Stage B/C division of labor (pseudo 1104-1121, 1165-1169) | `Induction.cpp:625` / `Stratum.cpp:1799` | annotate: name the real dispatch sites (correct the seed's `view_to_swap_vec` misattribution) |
| R2-2 | R.1.2 fence note (pseudo 992-1001) + R.1.3 Stratify (pseudo 1088-1102) | `Stratify.cpp:124` / `Build.cpp:1486` | + activation-SCC as a PROJECTION of the relation SCC (no new pass); coupled default |
| R2-3 | R.1.2 fence note (pseudo 992-1001) | `Build.cpp:1451-1499` | − the `cyclic_demand -> reject` nested fence; + couple per-instance fixpoint |
| R2-4 | R.1.2 InstanceStore memo (pseudo 1058-1082) | `InstanceStore.h:103-109` | + X=F peel-depth-0 data-structure dispatch (memo self-hit) |

No hunk mints a `REQUEST` node — that is DIFF-R1. DIFF-R2 is the *dispatch
reading* over DIFF-R1's edge plus the fence-lift; where the REQUEST op surface
is load-bearing, a STOP marker cites DIFF-R1 / D2.6.

---

### R2-1 — Name the real recursive-body dispatch sites (diff on R.1.3)

The seed (`region-model-pseudocode-seed.md:114-116`) attributes
`view_to_swap_vec` + "inductive MERGE as loop header" to `LowerDRRounds`. Part
R already corrected this (pseudo 1119-1121); DIFF-R2 must lower **onto the two
real sites**, not the seed's conflated one. There are two independent recursive
regimes and the pivot split reads differently onto each:

- **Stage B — monotone eager-descent induction** (`Induction.cpp:625`
  `GetOrInitInduction`; the loop-carried swap vector `view_to_swap_vec` is
  minted here and in `Join.cpp`/`Product.cpp`; `BuildFixpointLoop`
  `Induction.cpp:138`). Keyed per-view in `context.view_to_induction`
  (`Stratum.cpp:113-138`, `IsInductionOwned`). NEVER does
  OVERDELETE/REDERIVE/INSERT — monotone-only.
- **Stage C — per-TABLE differential Delta round shells** (`Stratum.cpp:1799`
  `LowerDRRounds`; `LowerRoundBody:1691`; `RoundTables:1675`). Loop-carried
  state = per-TABLE `TableDeltaVector`s, **not** `view_to_swap_vec`. This is
  where OVERDELETE/REDERIVE/INSERT lives, for a recursive SCC that did NOT get
  a `ProgramInductionRegion`.

```diff
  # R.1.3 pseudo 1165-1169 — DIVISION OF LABOR (kept, ANNOTATED with the pivot reading):
    # DIVISION OF LABOR: a recursive SCC WITH a ProgramInductionRegion is
    # fed/drained by Stage B's induction and left ALONE here; a recursive SCC
    # WITHOUT one (the doc's example: JOIN-terminal linear recursion of
    # transitive closure) gets its OVERDELETE/REDERIVE/INSERT from THIS
    # machinery (drain_stratum populated only for non-induction-owned tables).
+   # PIVOT READING (DIFF-R2): a recursive body atom rel[K](...) is classified
+   # by BRACKET-ARG IDENTITY against the head key var:
+   #   K == head.key_var  -> X=F FEEDBACK. Same-activation self-reference; it
+   #     IS the loop. Realized by the EXISTING loop-carried vector: Stage B's
+   #     view_to_swap_vec (monotone) OR Stage C's per-table TableDeltaVector
+   #     (differential). No REQUEST edge — feedback is not externalizable
+   #     (running-example §pivot-split bullet X=F).
+   #   K != head.key_var  -> X!=F cross-instance REQUEST. A read of a DIFFERENT
+   #     activation of the same template; the InstanceStore memo (R.1.2)
+   #     collapses it (R2-4). This is the ONLY arm DIFF-R1 externalizes.
+   # Linear recursion (path(F,M),edge(M,T)) is X=F-ONLY (source key threads
+   #   unchanged) = one activation, real pruning = demand_tc_witness. Non-linear
+   #   (tc(F,X),tc(X,T)) carries BOTH a left X=F feedback arm AND a right X!=F
+   #   request arm = the coupled case (A-corr-4).
```

> **SHRINK note.** The pivot reading is a *comment layer* on the existing
> lowering, not new lowering code. The FEEDBACK arm already lowers (both
> regimes above ship today); the classification is a re-description of the
> `source-position` fact `Stratify` already encodes. The only place the split
> forces new behavior is the X≠F arm under a *nested* lowering (R2-3).

**"SYNTACTIC" is scoped.** The seed title says "SYNTACTIC self-vs-call". Per
the owner peel analysis (running-example §peel, lines 292-324): the pivot X is
the loop-carried FRONTIER (loop-**variant**), F is the region parameter
(invariant), so the split is peel-depth ZERO — a DATA-STRUCTURE DISPATCH, not
specialized code. A literally *syntactic* `K == head.key_var` test requires the
bracket surface `rel[K](...)` where K is a named bracket var — i.e. it is
realizable only atop **DIFF-R3's declared-region surface** (owner notation
proposal, running-example:243-245: "whether the call's bracket arg [X] is the
SAME variable as the head bracket [F]"). On the SIP-inferred (`-demand`) path
there is no bracket var; the classification is recovered from
source-column-position provenance (the `GuardSite.kind` the demand pass already
computes — `Demand.cpp` `kReadAtTuple`/`kBaseAtom`/`kPushDown`, R.1.1 pseudo
860-871), and X=F remains a **runtime** memo dispatch. So: syntactic where
DIFF-R3 brackets exist, position-inferred + runtime-dispatched otherwise.
`STOP-R2-A`.

---

### R2-2 — The activation-SCC "pass" is the relation SCC projected onto the template (diff on R.1.2 fence note + R.1.3 Stratify)

The seed (seed:176-179) proposes a NEW pass: "build activation-call graph
(which key requests which); Tarjan SCC". **This is guilty-until-proven.** Two
facts collapse it:

1. The activation call graph is **runtime-valued** (X is data — running-example
   §matrix-frame:199-202: "data-graph cyclicity is a RUNTIME fact"). A
   compile-time pass cannot build it; it can only work over the **template**
   call structure (self-bracket vs other-bracket atoms) and *conservatively
   prove acyclicity* in restricted cases.
2. The template's self-vs-other-bracket cyclicity is **already computed**: a
   template call graph has a cycle iff the demanded relation's own view SCC is
   nontrivial, which is exactly `ViewSelfReachable(guard.joined[0])`
   (`Build.cpp:1486`) and the relation `Stratify` SCC condensation
   (`Stratify.cpp:124-235`, `view->stratum`). The activation SCC is the
   relation SCC lifted to keys; the InstanceStore memo (R.1.2) collapses the
   per-key activations onto that one condensation.

```diff
  # R.1.3 pseudo 1088-1102 — Stratify (kept; REINTERPRETED, no code change):
    Stratify(log):                                          # Stratify.cpp:124
      ... Tarjan over sources; SCC pop order = stratum id ...
      view->stratum := popped SCC id                        # :237-240
+     # DIFF-R2 REINTERPRETATION: for a demanded relation p, p's view SCC IS the
+     # template activation-call condensation. A MULTI-view stratum containing p
+     # <=> the activation graph has a cycle <=> ACROSS (coupled). A singleton
+     # stratum <=> the template is call-acyclic <=> DOWN is PROVABLE (descent
+     # licensed). No separate activation-SCC pass; the projection is free.
```

```diff
  # R.1.2 pseudo 992-1001 — the fence note (the seed's activation-graph SCC pass
  # dissolves into this existing recognition):
    # NOTE: the induction-owned / cyclic-demand fences are NOT here. They are
    # an EARLIER, separate pre-pass in Program::Build (Build.cpp:1451-1499) ...
    #   cyclic_demand      := ViewSelfReachable(guard.joined[0])   -> reject
+   # DIFF-R2: `ViewSelfReachable(guard.joined[0])` IS the "any ACROSS edge?"
+   #   test at the KEY level. Its two outcomes are the down/across dichotomy:
+   #     false -> DOWN-only: activation graph provably acyclic -> memoized
+   #              descent is SOUND and terminating with NO iteration (cost-tier
+   #              optimization; the coupled default is also correct here).
+   #     true  -> ACROSS present: emit the COUPLED FIXPOINT (existing Stage-C
+   #              LowerDRRounds round shells, keyed per instance) -- NOT a reject
+   #              (R2-3 lifts the reject). Default when acyclicity is unprovable.
```

> **The smaller formalization.** DIFF-R2 adds **no `TarjanActivationSCC`
> function**. DOWN/ACROSS = the boolean `ViewSelfReachable(joined[0])` already
> computed at `Build.cpp:1486`. The COUPLED default is the existing
> `LowerDRRounds` machinery (which is why plain `-demand` already compiles
> recursive demand — `demand_tc_witness`, `demand_cyclic_1`). The DESCENT
> specialization (compute each row once from successors, no iteration) is a
> **cost-model optimization** licensed by provable acyclicity, and is DEFERRED
> to the D5/matrix tier (DIFF-R6-adjacent) — it is not required for
> correctness or for the fence lift. Anything the seed spends on a bespoke
> activation-SCC pass is spent twice.

---

### R2-3 — Lift the `demand_cyclic_1` nested fence: couple the per-instance fixpoint (diff on R.1.2)

Today `-demand-instance` rejects cyclic demand at `Build.cpp:1451-1499`
(`cyclic_demand := ViewSelfReachable(jl[0])` → `Build.cpp:1490-1492`). The
program COMPILES under plain `-demand` (the flat path's `LowerDRRounds` already
runs the coupled fixpoint) — the fence is nested-lowering-specific
(`demand_cyclic_1.dr` header; CLAUDE.md D2.b). DIFF-R2 lifts it by letting a
recursive demanded subgraph's InstanceStore participate in the Stage-C round
shells instead of standing outside them.

```diff
  # Build.cpp:1490-1492 (the reject the fence draws):
-     if (cyclic_demand) {
-       log.Append() << "Recursive demand relations are not yet supported "
-                       "under -demand-instance";
-     } else if (recursive_content) { ... }
+     # DIFF-R2: cyclic_demand no longer rejects. The demanded relation p is on
+     # a MULTI-view stratum (R2-2); its InstanceStore-keyed rows join the
+     # EXISTING coupled fixpoint over p's table:
+     #   - band-(a)/band-(b) birth/rebuild (R.1.2 kSubgraphInstantiate) seeds
+     #     the activation and its X=F feedback swap vector as today;
+     #   - the X!=F cross-instance arm reads a SIBLING iid via the memo (R2-4);
+     #     both arms are loop-carried in ONE keyed table -> V-CW-widened
+     #     coupled iteration (NOT a disjoint union -- A-corr-4).
+     # recursive_content (induction-owned body, Build.cpp:1477) STAYS fenced
+     # this slice: it needs Stage-B/Stage-C coupling not yet designed. STOP-R2-C.
```

> **STOP-R2-B (D2.12 / V-CW restatement — load-bearing, re-brief OPEN).** The
> coupled iteration above is sound only under the **restated** V-CW lemma:
> `independent -> disjoint-union` / `coupled -> widened-coupled-fixpoint`, with
> the cross-instance-read test (`X != F` reachable?) as the discriminator
> (owner refinement 2026-08-03, record lines 318-335; A-corr-4). D2.12 is
> RATIFIED "V-CW first, owner-gated" (record:87-89) but the disjoint-union→coupled
> **restatement is an OPEN re-brief action**. The coupled-fixpoint lowering
> here MUST NOT land before that restatement is pinned, or V-CW would falsely
> certify a cross-instance read as a disjoint union. Do not guess the restated
> predicate — cite D2.12 re-brief.

> **STOP-R2-D (DIFF-R1 / D2.6).** The X≠F arm is a `REQUEST` edge. Its op
> surface + row schema ride DIFF-R1 (the request-edge node, the Stage-C
> cutover) and **D2.6 is DEFERRED** (edge row schema, record:60-66). Until
> DIFF-R1 lands, R2-3 lowers the X≠F arm as the *implicit* guard-JOIN +
> InstanceStore lookup the flat path already uses (R.1.1) — i.e. the fence lift
> is realizable on the CURRENT implicit-edge lowering without waiting for
> DIFF-R1; DIFF-R1 only makes the edge first-class.

---

### R2-4 — X=F as a memo self-hit: peel-depth-zero data-structure dispatch (diff on R.1.2 InstanceStore)

The owner peel analysis (running-example:305-310) forces the realization: X=F
is a loop-VARIANT split → peel depth ZERO → a data-structure dispatch, never
specialized code. The dispatch IS the InstanceStore memo lookup already present
in R.1.2.

```diff
  # R.1.2 pseudo 1058-1082 — InstanceStore API (kept; ANNOTATE the memo semantics):
    InstanceStore<Key,RowT>:
      FindInstance(key) -> iid | kNoInstance          # :103, memo lookup, no mint
      FindOrAddInstance(key) -> iid                   # :109, mints empty current+frozen
+     # DIFF-R2 memo-dispatch reading of the pivot at a body atom rel[K](...):
+     #   FindInstance(K) hits an EXISTING iid  <=> X=F on-cycle (self-hit) ->
+     #     take the FEEDBACK path (fold into the SAME activation's swap vector /
+     #     TableDeltaVector); NO fresh descent, NO infinite recursion.
+     #   FindInstance(K) miss / a DIFFERENT iid <=> X!=F -> the cross-region
+     #     REQUEST (R2-3); FindOrAddInstance mints/reuses the sibling row.
+     # The self-partition {K : K==F} is a SINGLETON (present iff F on a cycle),
+     # so there is nothing to specialize -- the memo self-hit IS the whole
+     # realization (running-example §peel: "memo self-hit forced by invariance").
+     # This is what keeps NON-LINEAR demand QUADRATIC not exponential: each
+     # interior activation computed once, shared via the arrangement
+     # (running-example §endpoint-pivot Payoff 2).
```

No InstanceStore API change — the memo already exposes `FindInstance` (no-mint
lookup) vs `FindOrAddInstance` (mint). DIFF-R2 names which pivot outcome takes
which; the fixpoint termination rests on the finite-activation argument below.

---

### SOUNDNESS OBLIGATIONS

1. **Termination (finite-activation).** Every activation key is a tuple over
   already-derived values drawn from a FINITE derived domain (the magic/demand
   relation seeds `{F}` then fills with a subset of the finite reachable set —
   running-example:121-126). Finitely many keys ⇒ finitely many activation
   nodes ⇒ the coupled fixpoint over one keyed table is a monotone, bounded
   lattice iteration ⇒ converges. This is the argument the `demand_cyclic_1`
   fence stands in for; R2-3 discharges it to lift the fence. (Ownership: the
   flat path already terminates on `demand_tc_witness`/`demand_cyclic_1` — the
   obligation is that the nested per-instance coupling inherits it.)
2. **Coupled ≠ disjoint (A-corr-4).** A cross-instance read (X≠F) makes the
   key-widened fixpoint a COUPLED fixpoint in one keyed table, NOT a disjoint
   union of per-instance fixpoints. V-CW must be restated before it certifies
   this (STOP-R2-B). This is the **priority-scrutiny axis**: termination alone
   does NOT discharge it — a nonlinear (cross-instance-read) witness is
   mandatory.
3. **Answer-identity across lowerings.** flat `-demand` == nested
   `-demand-instance` == oracle, for every recursive demanded case the fence
   lift newly admits (I0/eqgate discipline; the over-materialization-aborts
   contract, CLAUDE.md HP-5).
4. **Memo self-hit soundness.** `FindInstance(K)` returning an existing iid for
   X=F must fold into the SAME activation's loop-carried vector (feedback), not
   a fresh sub-activation — else a cyclic key descends infinitely. The
   singleton self-partition (present iff on a cycle) is the guard.
5. **Fence residue (recursive_content stays).** Lifting `cyclic_demand` MUST
   NOT silently lift `recursive_content` (induction-owned body,
   `Build.cpp:1477`); that arm needs Stage-B/Stage-C coupling not designed here
   (STOP-R2-C). The reject must remain for that arm only.

### OWNER DEPENDENCIES

- **D2.12 (V-CW) — RATIFIED "V-CW first, owner-gated" (record:87-89); the
  disjoint-union→coupled RESTATEMENT is an OPEN re-brief action (record:318-335,
  A-corr-4).** Load-bearing for R2-3; `STOP-R2-B`. Do not land coupled lowering
  before the restatement is pinned.
- **DIFF-R1 (request-edge node) — the Stage-C cutover (seed §DIFF-R1); D2.6
  (edge row schema) DEFERRED (record:60-66).** The X≠F REQUEST op surface rides
  it; `STOP-R2-D`. R2-3 is realizable on the current implicit-edge lowering
  without it.
- **DIFF-R3 (declared regions) — OPEN candidate (hint-vs-mandate, sequence
  slot; record:351-383).** Gates the *syntactic* `K == head.key_var` bracket
  test; `STOP-R2-A`. Without it, the split is position-inferred + runtime
  memo-dispatched.
- **D1.5/D1.6 (Variant B, recursive-demanded content) — RATIFIED
  (record:29-37).** Recursive demanded content is a fireable reject
  (`demand_cyclic_1`/`demand_recursive_content_1`); R2-3 lifts only the
  `cyclic_demand` half under `-demand-instance`, consistent with Variant B
  keeping `recursive_content` fenced.

### TEST / WITNESS PLAN (golden-master terms)

- **Existing carriers, re-refereed unchanged.** `demand_tc_witness` (right-LINEAR
  = X=F-only feedback; the linear/one-activation carrier) and
  `demand_neighborhood_witness`/`demand_neighborhood_mono_witness` (the
  flat==nested eqgate carriers) must stay byte-identical: DIFF-R2 changes no
  lowering for cases already admitted. Referee: `runall.sh` byte-compare +
  `.eqgate` run_eqgate + `bin/Oracle`.
- **NEW mandatory nonlinear witness — `demand_tc_nonlinear_witness` (name
  owner-adjustable).** A bound `#query` over NON-LINEAR TC (`tc(F,X),tc(X,T)`)
  — the corpus has `tc_nonlinear_diff` (non-linear TC) but it is a FREE query,
  and `demand_cyclic_1` is LINEAR; neither exercises the X≠F cross-instance
  read. This witness carries BOTH the left X=F feedback arm and the right X≠F
  request arm = the COUPLED case A-corr-4 demands. It is the discharge of
  Obligation 2 and the V-CW-coupled (not disjoint-union) witness D2.12's
  re-brief makes mandatory. Referee: I0/eqgate (flat==nested==oracle) +
  `bin/Oracle` per-key answer-identity + sorted published-delta identity if
  `@differential`.
- **Fence-lift flip.** `demand_cyclic_1` flips from all-4-modes-diagnostic
  (under `-demand-instance`) to a compiling nested case ONLY after R2-3 lands
  AND STOP-R2-B clears; until then it STAYS diagnostic. Encode the flip in
  `runall.sh` in lock-step with the coupled lowering, never ahead of it.
- **Fence-residue negative.** `demand_recursive_content_1` STAYS
  all-4-modes-diagnostic (Obligation 5 / STOP-R2-C); assert the reject message
  still fires (the `recursive_content` arm at `Build.cpp:1493-1497`).
- **Adornment-placement fuzzing (owner direction 2026-08-03, record:463-487).**
  The nonlinear witness is a curated case; the covering-array/placement
  generator (DIFF-R3 test matrix) must additionally sample bracket placements
  over non-linear recursive bodies and require every placement either compiles
  answer-identical (I0 + flat-demand oracle) or draws a CLEAN diagnostic —
  never a miscompile, never an assert (evil-monkey rule). DIFF-R2 contributes
  the non-linear/cyclic axis of that generator; the generator itself lands with
  DIFF-R3.


### OPEN OWNER ITEMS (DIFF-R2)

- STOP-R2-B / D2.12 (V-CW): the disjoint-union->coupled RESTATEMENT is an OPEN re-brief action (record:318-335, A-corr-4). Coupled per-instance fixpoint lowering (R2-3) MUST NOT land before V-CW is restated as independent->disjoint-union / coupled->widened-coupled-fixpoint with the X!=F cross-instance-read discriminator.
- STOP-R2-D / DIFF-R1 + D2.6 (DEFERRED, record:60-66): the X!=F REQUEST op surface and edge row schema ride DIFF-R1 (the request-edge Stage-C cutover). R2-3 is realizable on the current implicit guard-JOIN + InstanceStore lowering meanwhile; DIFF-R1 only makes the edge first-class.
- STOP-R2-A / DIFF-R3 (OPEN candidate, record:351-383): the literally-SYNTACTIC K==head.key_var bracket test needs the declared-region bracket surface. Without DIFF-R3 the pivot split is position-inferred (GuardSite provenance) + runtime memo-dispatched, not syntactic.
- STOP-R2-C: lifting cyclic_demand must NOT lift recursive_content (induction-owned demanded body, Build.cpp:1477/1493) - that arm needs Stage-B/Stage-C coupling not designed in DIFF-R2 and stays fenced (consistent with Variant B, D1.5/D1.6).
- NEW witness gap: no corpus case exercises a NON-LINEAR DEMANDED TC (X!=F cross-instance read). demand_tc_witness is linear (X=F-only), demand_cyclic_1 is linear-cyclic, tc_nonlinear_diff is a FREE query. A demand_tc_nonlinear_witness must be authored to discharge the A-corr-4 coupled-fixpoint obligation the V-CW re-brief makes mandatory.

### PANEL RECORD (DIFF-R2) — 12 findings, 12 survived refutation

- **DIFF-R2-correctness-lifecycle-1** [CONFIRMED; sev BLOCKING] R2-3 lifts the `cyclic_demand` fence, but the nested (`-demand-instance`) lowering builds per-instance content with a SINGLE-PASS rescan that cannot close a recursively-derived (transitive-closure) relation. The claimed 'existing coupled fixpoint over p's table' does not exist in the nested lowering; there is no fixpoint mechanism for the instance, so the admitted programs under-materialize.
  - EVIDENCE: Part-R pseudocode 1442-1447: `emit_instance_rescan` is a single straight-line scan `for s in input_member.rows: if row.key_cols==keyexprs ...: cur.TryAdd(...)` guarded by `assert !WorkingOccupied(iid)` (V-INST-FRESH) — no iteration, folds the INPUT table once into pub. The in-code recursive_content fence (Build.cpp:1444) names the identical defect verbatim: 'single-scan cannot close a TC'. demand_cyclic_1.dr is linear TC (`conn(F,T):conn(F,M),link(M,T)`) whose content is still a transitive closure. R2-3's 'the demanded relation p is on a MULTI-view stratum ... its InstanceStore-keyed rows join the EXISTING coupled fixpoint over p's table' names a fixpoint the nested lowering does not have: pub is filled by the acyclic band-(b) kInstanceRebuild (pseudo 1020-1027, 1453-1454), not by round shells.
  - AMENDMENT (normative): Do NOT present R2-3 as a comment-layer reinterpretation of an existing fixpoint. State explicitly that lifting `cyclic_demand` REQUIRES a per-instance closure mechanism the nested lowering lacks today — either (a) iterate `emit_instance_rescan` to a per-instance monotone fixpoint over the instance's own current buffer (dropping the V-INST-FRESH single-pass invariant under a new explicit invariant), or (b) route p's recursion back through Stage-C round shells with the InstanceStore only guarding/keying. Add a soundness obligation making the per-instance closure explicit, and keep demand_cyclic_1 diagnostic until that mechanism is designed and witnessed. Downgrade R2-3 to a design sketch of that mechanism.
- **DIFF-R2-correctness-lifecycle-2** [CONFIRMED; sev BLOCKING] The X!=F 'cross-instance REQUEST' arm — the whole coupled-fixpoint apparatus (Obligation 2 / STOP-R2-B / the mandatory `demand_tc_nonlinear_witness`) — is a SECOND READ OF p in one body, which is rejected UPSTREAM at Demand.cpp:656-660 under plain `-demand`, a reject DIFF-R2 never lifts. R2-3 lifts only the downstream nested-only `cyclic_demand` fence (Build.cpp:1490), which admits only LINEAR self-recursion — the case the diff itself says already compiles.
  - EVIDENCE: Demand.cpp:656-660 rejects `if (p_reads > 1u)` ('a self-join ... not yet supported under -demand') inside ApplyDemandTransform, under plain -demand, upstream of the nested Build.cpp:1490 fence R2-3 touches. The diff's own R2-1 comment says non-linear `tc(F,X),tc(X,T)` 'carries BOTH a left X=F feedback arm AND a right X!=F request arm' — that right arm is a second read of tc (p_reads=2) whose bound From=X is also non-From-preserving (rejected at Demand.cpp:746-750). Corpus confirms: tc_nonlinear_diff.dr is a FREE query (never bound); demand_cyclic_1.dr is LINEAR (one conn read, From-preserving), which is exactly why it passes -demand and rejects only at the nested fence. So `cyclic_demand` (Build.cpp:1486, ViewSelfReachable(jl[0])) admits only linear self-recursion — the case the diff itself says already compiles.
  - AMENDMENT (normative): Separate the two fences in the diff map: `cyclic_demand` (Build.cpp:1490, linear self-recursion through the instance boundary) versus the nonlinear/cross-instance self-join reject (Demand.cpp:656) and the sideways reject (Demand.cpp:746). State that the coupled/A-corr-4 case additionally requires lifting Demand.cpp:656-660 and 744-750 — a new hunk with its own second-read-of-p demand-propagation rule. Drop `demand_tc_nonlinear_witness` from the mandatory-witness list (Obligation 2 is not dischargeable this slice), or re-scope the entire coupled/V-CW/A-corr-4 section as a follow-on diff that first lifts the flow-layer rejects.
- **DIFF-R2-correctness-lifecycle-3** [SOFTENED; sev MAJOR] If cross-instance requests were ever admitted, mid-fixpoint instance BIRTH and DEATH have no owner. kSubgraphInstantiate is a `ctx=kSeed` acyclic op over the batch-entry demand frontier; a demand key derived at round N has no birth op inside round N, and kInstanceDeath cannot retract transitively-derived demand keys. Obligation 1 ('inherits termination from the flat path') ignores that the nested instance-birth loop is a new iteration axis absent from the flat path.
  - EVIDENCE: The mechanism is real: kSubgraphInstantiate is ctx=kSeed (Rel.cpp:1111), lowered ONCE after all strata (Procedure.cpp:588), its birth band draining DemandFrontier() at batch entry (pseudo 1431-1433) — no birth op inside a round shell. BUT this bites only the CROSS-INSTANCE (non-linear) regime, where the demand relation grows new keys mid-fixpoint. The ACTUALLY-lifted linear cyclic_demand (demand_cyclic_1: `conn(F,M),link(M,T)`, From-preserving) has a demand set fixed to the queried keys — demand__conn(F) demands demand__conn(F), same key, no mid-fixpoint growth — so the batch-entry DemandFrontier/RemovalFrontier is complete and birth/death have their owner. The finding overshoots by implying the birth-owner gap bites the case R2-3 reaches; there the defect is Finding 1's content-fixpoint, not birth ownership.
  - AMENDMENT (normative): Scope the obligation to the cross-instance (non-linear) demand arm only: for demand relations whose key set grows mid-fixpoint, instance birth (kSubgraphInstantiate/band-a1) must move INSIDE the Stage-C round shell (or the demand table must be fully populated before the acyclic seed band), and kInstanceDeath must drain transitively-derived demand net-removals. Note this arm is already unreachable this slice (Finding 2), so the obligation rides with the deferred coupled realization; the linear cyclic_demand case R2-3 reaches has a From-preserving, batch-complete demand frontier and needs no mid-fixpoint birth owner.
- **DIFF-R2-correctness-lifecycle-4** [CONFIRMED; sev MAJOR] STOP-R2-D miscites the flat path: it claims R2-3 can lower the X!=F arm 'as the implicit guard-JOIN + InstanceStore lookup the flat path already uses (R.1.1)'. The flat `-demand` path does NOT use InstanceStore — InstanceStore exists only in the nested `-demand-instance` lowering (R.1.2). The stated fallback is incoherent.
  - EVIDENCE: Part-R separates R.1.1 'The demand transform (flat lowering)' (pseudo 816) from R.1.2 'The keyed-instance (nested) lowering' (pseudo 955), which is where `InstanceStore<Key,RowT>` is introduced (pseudo 1050-1082) as 'the keyed-instances TRANSPOSE of StateCellStore' — nested-only runtime state. The flat path lowers p to an ordinary table + guard-JOIN web and never touches InstanceStore. STOP-R2-D's phrase 'the implicit guard-JOIN + InstanceStore lookup the flat path already uses (R.1.1)' fuses two mutually exclusive lowerings and miscites R.1.1 for an InstanceStore operation that lives only in R.1.2.
  - AMENDMENT (normative): Correct STOP-R2-D: under -demand-instance the cross-instance read has NO flat-path fallback — p IS the InstanceStore, so there is no ordinary guard-JOIN over p to reuse; the read is an InstanceStore FindInstance/FindOrAddInstance against a sibling key with the mid-fixpoint birth semantics of Finding 3. Remove the assertion that R2-3's cross-instance arm is 'realizable on the CURRENT implicit-edge lowering without DIFF-R1'; it is genuinely blocked on DIFF-R1's request-edge node / D2.6 row schema, not merely 'made first-class' by it.
- **DIFF-R2-termination-confluence-1** [CONFIRMED; sev BLOCKING] The finite-activation termination argument (Obligation 1) does not discharge convergence of the realization R2-3 actually names. The claim that the coupled per-instance fixpoint is 'the EXISTING coupled fixpoint (existing Stage-C LowerDRRounds round shells, keyed per instance)' is false: the InstanceStore rebuild is acyclic single-pass and is never iterated by the round shells.
  - EVIDENCE: The SHRINK premise 'the coupled fixpoint is already emitted' is false for InstanceStore-keyed rows. kSubgraphInstantiate is ctx=Ctx::kSeed (Rel.cpp:1111); LowerSubgraphInstances is emitted at Procedure.cpp:588 BEFORE LowerCommitSweeps:589, riding the acyclic seed/commit tail, EXACTLY ONCE after the whole ascending stratum loop (pseudo 1418-1421). LowerDRRounds (Stratum.cpp:1799, pseudo 1145-1164) iterates per-TABLE TableDeltaVectors, never InstanceStores. band-(a) is one keyed section-walk run once per epoch (pseudo 1442-1447). So in the coupled case nothing re-runs instance A's rescan when sibling B grows in a later round; the finite-key lattice bound is a semantic object the emitted single-pass machinery does not iterate to.
  - AMENDMENT (normative): Drop the claim that termination is discharged by the 'existing Stage-C LowerDRRounds round shells, keyed per instance' — LowerSubgraphInstances provably does not iterate. Either add a hunk moving the recursive InstanceStore rebuild INTO the round shells as loop-carried state (re-running band-(a) each round until the keyed table reaches a joint fixpoint over both the instantiated-key set and per-instance content, with a finiteness argument over the two monotone dimensions), or keep cyclic_demand fenced this slice and defer the coupled realization. Do not assert termination is inherited from the flat path's machinery.
- **DIFF-R2-termination-confluence-2** [CONFIRMED; sev MAJOR] Confluence under batch permutation is asserted nowhere, and the X!=F cross-instance read never specifies which InstanceStore buffer it reads — a per-key-vs-joint-quiescence gap that decides whether a single batch's answer is order-independent.
  - EVIDENCE: InstanceStore is an explicit double buffer: Current(iid) = 'this epoch's rebuilt content' (:142), Frozen(iid) = 'last epoch's sealed snapshot' (:143), Seal() pointer-swaps them (:180-208). R2-4 says the X!=F arm reads 'a SIBLING iid via the memo' but never pins Current vs Frozen. Reading Frozen makes coupled activations a cross-EPOCH Jacobi iteration (stale sibling values, converges only across epochs) — flat != nested for a single batch, breaking Obligation 3; reading Current makes the result depend on Seal order across the coupled activation-SCC. Obligations 1-5 enumerate only per-key concerns; no joint-quiescence obligation is stated.
  - AMENDMENT (normative): Add Obligation 6 (joint quiescence + confluence): the coupled activation-SCC must seal JOINTLY at joint quiescence (no instance sealed or read cross-instance before the whole SCC quiesces); pin which buffer the X!=F arm reads and prove it yields the within-batch LFP independent of instance visitation order. Add a batch-permutation confluence witness (same nonlinear program, two batch orderings, byte-identical published deltas) to the TEST/WITNESS plan. Note this rides the deferred coupled arm (Findings 1-2).
- **DIFF-R2-termination-confluence-3** [SOFTENED; sev MAJOR] Obligation 1's termination argument ('monotone, bounded lattice iteration') covers only the monotone regime, but R2-3 lifts cyclic_demand with no scope restriction, admitting differential-coupled recursive demand whose termination rests on a DIFFERENT (unstated) argument and which trips the InstanceStore Seal's R-MONO assertion.
  - EVIDENCE: The 'fires the R-MONO assert' mechanism is REFUTED: the Seal belt is `monotone`-gated (`if (monotone)`, InstanceStore.h:191), and an R-DIFF/@differential store is constructed with `monotone=false` (:67-70, 'R-DIFF (D3.a) ... legitimately shrink current, so the belt is R-MONO-only'), so a differential pub SKIPS the frozen⊆current assert — no spurious abort. However the finding's core stands: Obligation 1 argues termination only via 'monotone, bounded lattice iteration', and R2-3 lifts cyclic_demand with no scope restriction, so it admits differential-coupled recursive demand (D3.a diff-input composes with -demand-retract) whose termination rests on the distinct signed-counter zero-crossing UPDATECOUNT invariant the diff never invokes.
  - AMENDMENT (normative): Scope the fence lift to MONOTONE recursive demand this slice: add a differential-coupled reject sibling (a coupled cyclic_demand whose pub or demand table is differential stays fenced) with a fence-residue negative witness. Drop the 'fires the R-MONO assert / lift the belt' claim — the belt already correctly skips differential stores (InstanceStore.h:191, monotone=false). If differential-coupled is later brought in scope, replace Obligation 1's monotone-lattice argument with the signed-counter zero-crossing UPDATECOUNT invariant rather than modifying the Seal belt.
- **DIFF-R2-termination-confluence-4** [CONFIRMED; sev MAJOR] R2-4 presents the runtime memo hit-vs-miss as the X=F/X!=F discriminator ('FindInstance(K) hits => X=F feedback; miss => X!=F REQUEST'), which is materialization-order-dependent and misclassifies the FIRST self-reference of a cyclic key, contradicting R2-1/R2-2's compile-time discriminator.
  - EVIDENCE: FindInstance is a no-mint lookup (InstanceStore.h:103, 'Dense iid for key, or kNoInstance (no allocation)'), so on the first demand of a cyclic self-key K, FindInstance(K) MISSES before FindOrAddInstance (:109) mints it — under R2-4's stated '⟺', that first self-reference is misclassified as an X!=F request, outcome-dependent on mint-vs-body ordering. Meanwhile R2-1/R2-2 locate the discriminator at compile time (bracket-arg identity K==head.key_var; GuardSite.kind; ViewSelfReachable(jl[0]) at Build.cpp:1486). A split cannot have both a compile-time classifier and a runtime memo-hit classifier stated with '⟺'; the runtime one is materialization-order-dependent and unsound for confluence.
  - AMENDMENT (normative): Rewrite R2-4 so the memo lookup REALIZES an arm already classified at compile time (per STOP-R2-A: GuardSite.kind / bracket-arg identity), not the classifier — replace the '⟺' with 'realizes the already-classified X=F feedback arm'. Add to Obligation 4 the tabling precondition: the self-instance must be minted (FindOrAddInstance) BEFORE its recursive body executes, so a compile-time-classified X=F feedback always hits its own iid regardless of batch order; state this as a lowering invariant, not an emergent runtime property.
- **DIFF-R2-testability-oracle-1** [CONFIRMED; sev BLOCKING] The diff's mandatory discriminating witness `demand_tc_nonlinear_witness` (a bound #query over non-linear TC `tc(F,X),tc(X,T)`), declared the sole discharge of Obligation 2 (coupled != disjoint) and the V-CW-coupled referee, is UNCONSTRUCTIBLE under the current `-demand` slice; the X!=F coupled behavior R2-3 lowers therefore has no constructible referee — the H-A9 unconstructible-witness trap.
  - EVIDENCE: The mandatory witness `demand_tc_nonlinear_witness` (bound #query over `tc(F,X),tc(X,T)`) reads tc twice, hitting Demand.cpp:656-660's `p_reads > 1u` reject two layers upstream of the Build.cpp nested fence R2-3 touches; the right arm's From=X bind additionally hits the sideways reject (Demand.cpp:746-750). Verified in corpus: tc_nonlinear_diff.dr is a FREE query only; demand_cyclic_1.dr is LINEAR (one conn read, From-preserving). No bound non-linear demand can pass -demand, so there is no flat baseline for the eqgate and bin/Oracle has nothing to compare — the H-A9 unconstructible-witness trap. R2-3's 'realizable on the CURRENT implicit-edge lowering' is moot: the shape is rejected before reaching that lowering.
  - AMENDMENT (normative): Reconcile scope: either (a) explicitly widen DIFF-R2 to also lift the -demand self-join (Demand.cpp:656) and sideways (Demand.cpp:744/766) rejects — a larger slice with its own obligations — before claiming a constructible nonlinear witness; or (b) downgrade R2-3 to admit ONLY linear (X=F-only) recursive demand this slice, delete the coupled/X!=F cross-instance lowering as unreachable-and-untestable, and move the coupled-fixpoint design plus its mandatory nonlinear witness to a follow-on diff. Do not present Obligation 2 as dischargeable in this slice.
- **DIFF-R2-testability-oracle-2** [CONFIRMED; sev MAJOR] The only constructible fence-lift flip in the test plan — `demand_cyclic_1` flipping from all-4-modes-diagnostic to compiling nested — does not discriminate the load-bearing new behavior: it is a LINEAR (X=F-only) program that already compiles under flat `-demand`, and today it is a diagnostic case with no driver, `.eqgate`, or `.probes`; the flip as specified proves only that it COMPILES, not per-key answer-identity nor the HP-5 over-materialization-aborts contract that is the point of nested per-instance lowering.
  - EVIDENCE: Verified: demand_cyclic_1 carries only .dr/.drflags/.main.cpp, and its main.cpp is an inert stub ('this driver is inert and never compiled ... return 0'). It is left-linear, so its nested lowering needs only the pre-existing X=F feedback vector — a degenerate single-shared-instance (over-materialized) lowering would still yield the same UNION answer if the driver probes one key or never asserts per-key isolation, so a compile-only flip cannot distinguish correct per-instance keying from over-materialization (the HP-5 contract). The TEST PLAN says only 'encode the flip in runall.sh in lock-step with the coupled lowering' and never specifies a new answer-bearing driver, .eqgate, or .probes sidecars.
  - AMENDMENT (normative): Specify concretely what demand_cyclic_1's flip must ship: a real answer-bearing .main.cpp probing >=2 distinct From keys with disjoint reachable sets, asserting each probe returns EXACTLY its own set (HP-5); plus an .eqgate sidecar (-demand -demand-instance) driving run_eqgate's LIVE flat==nested byte-compare, and .probes/.batches so bin/Oracle referees per-key flat==nested==oracle. State explicitly that this flip covers only the X=F/linear arm and is NOT a coupled-behavior referee (per testability-oracle-1).
- **DIFF-R2-necessity-1** [SOFTENED; sev MAJOR] The SYNTACTIC self-vs-call classification (R2-1) is a redundant second authority for a split whose only forced realization is the runtime memo dispatch (R2-4); for the actual deliverable (the R2-3 fence lift) the classification is inert and can be deleted from this slice.
  - EVIDENCE: The core is supported and partly self-admitted by the diff: R2-1's SHRINK note already calls the pivot reading 'a comment layer ... not new lowering code', R2-2 dissolves the activation-SCC pass, and the owner peel analysis makes X=F a peel-depth-0 data-structure dispatch (the runtime memo), not specialized code. Combined with Findings 1-2 (coupled/X!=F arm unreachable; even linear needs a real fixpoint the classification does not provide), the compile-time X=F/X!=F label is indeed not the operative fence-lift mechanism. BUT 'delete R2-1 from this slice' overshoots: R2-1's value is correcting the seed's view_to_swap_vec misattribution and naming the two real dispatch sites (Induction.cpp:625 Stage-B / Stratum.cpp:1799 Stage-C), which is genuine documentation independent of the pivot reading.
  - AMENDMENT (normative): Demote the OPERATIVE X=F/X!=F pivot reading (not R2-1 wholesale) to a one-line note: 'the self-vs-call distinction has no operative role in the fence-lift slice; realization is the peel-depth-0 runtime memo dispatch (R2-4).' Keep R2-1's site-naming/seed-correction annotation. Rewrite R2-3 to say the demanded relation's InstanceStore rows simply participate in a per-instance fixpoint over p (no per-atom classifier, no two routing arms), consistent with Findings 1-2. Bundle the SYNTACTIC bracket test (DIFF-R3) and the DOWN/ACROSS descent specialization into one explicitly-deferred cost-tier bucket, since they share the single consumer and neither is required for the fence lift.
- **DIFF-R2-necessity-2** [CONFIRMED; sev MINOR] The one mechanism that survives as load-bearing — the memo dispatch predicate in R2-4 — is stated with a self-contradictory discriminator ('hits an EXISTING iid <=> X=F' vs 'a DIFFERENT iid <=> X!=F'), so as written it does not define a usable test.
  - EVIDENCE: R2-4's predicate is logically self-contradictory as written: 'FindInstance(K) hits an EXISTING iid <=> X=F' versus 'FindInstance(K) miss / a DIFFERENT iid <=> X!=F' — but 'a DIFFERENT iid' is itself a hit of an existing iid, so 'hits an existing iid' cannot discriminate X=F from X!=F. Per the owner peel analysis the real discriminator is iid EQUALITY WITH THE CURRENT ACTIVATION, not hit-vs-miss. Since Finding 1/termination-confluence-4 make the memo the sole realization authority, an implementer reading the '⟺' literally could fold a sibling activation's rows into the current instance's swap vector — a correctness hazard in exactly the nonlinear case R2-4 aims to keep quadratic.
  - AMENDMENT (normative): Replace R2-4's iff clauses with: 'the feedback path is taken iff FindInstance(K) returns the CURRENT activation's iid (self-hit — prevents infinite descent on a cyclic key); ANY other outcome (miss or a sibling iid) is a cross-instance read served by the memo/FindOrAddInstance lookup.' Drop the X=F/X!=F labels from the runtime predicate (per termination-confluence-4 they are computed at compile time, not here) and state it purely as current-iid-equality.

---

## DIFF-R3 — User-DECLARED regions (brackets on internal relations) (formalized 2026-08-03)

Formalized against `regional-arch-pseudocode.md` **Part R** (fleet-verified,
`keyed-instances` tip). Charge: turn the region-model surface `rel[Bound…](Free…)`
inward — let an internal relation carry an explicit key spec — and turn the
demand transform's region INFERENCE into region CHECKING, *complementing* the SIP
walk, never replacing it. Anchors re-verified this pass; the SINGLE-PASS RULE
applies (re-verify line numbers before building).

**Scope discipline (complexity is guilty).** The seed (`region-model-pseudocode-seed.md`
Part 2, DIFF-R3) reads "`if rel has a DECLARED bracket: SKIP SIP inference; CHECK
the declared key (contract validates it is a real key)`." That over-specifies on
two counts, and this formalization SHRINKS both:

1. **"SKIP SIP inference" is too strong.** Part R R.1.1 shows the walk does two
   jobs at the demanded relation `p`: Step 2 (Demand.cpp:507-600) *derives* the
   bound column set `p_bound`; Step 3 (:607-783) *classifies the guard sites* and
   runs every fence (NEGATE/AGG sink, sideways/left-linear, second-read, stray
   consumer). A declared bracket replaces ONLY Step 2's key-derivation; Step 3 and
   all fences run UNCHANGED. Declaring a key does not lift a fence — that is
   DIFF-R4's job.
2. **"contract validates it is a real key" (a functional-key proof)** needs
   `Minimize`/`DeterminedBy`, which Part R R.1.4 records as DEFERRED to Stage B
   (`CheckNoCollapse` is belt-only at :318-336, unsound at the conservative
   flat-key model). The LANDABLE near-term check is a pure *structural
   set-reconciliation* — declared key == inferred `p_bound` — which needs no
   `Minimize`. The functional-key proof rides Stage B / DIFF-R5.

Hence DIFF-R3 lands in two tiers. **R3a** (parser + a checking overlay, landable
after I0 + Stage B, needs no request-edge node): brackets are parsed, well-formed-key
validated, and reconciled against the inferred key; when brackets are absent the
compiler is byte-identical to today, and the overlay changes NO lowering, so
answer-identity is trivial. **R3b** (the declared bracket DRIVES the lowering,
bypassing the transform): needs the DIFF-R1 request-edge node + D2.6 edge schema
and rides the hint-vs-mandate owner call — a **STOP** below, not formalized here.

### Diff map

| Hunk | Part-R anchor | Real site | Tier | Kind |
| --- | --- | --- | --- | --- |
| R3-P1 | (new surface) | `lib/Lex/Lexer.cpp:138`, `include/drlojekyll/Lex/Token.h:224`, `lib/Parse/Parser.cpp:354` (`ParseLocalExport`), `include/drlojekyll/Parse/Parse.h` (`ParsedRelation`/`ParsedDeclaration`) | R3a | + lexeme, + parser state, + optional key spec |
| R3-P2 | R.1.1 Loop 1, Step 2/3 | `lib/DataFlow/Demand.cpp` :507-600 / :607-783 | R3a | inference → checking fork |
| R3-P3 | R.1.4 contract | `lib/DataFlow/RowContract.cpp:365`, dump `Format.cpp` | R3a | + reconciliation check, + dump field |
| R3-STOP | — | — | R3b | owner gate (hint-vs-mandate) |

---

### R3-P1 — the bracket surface: lexer + parser + `ParsedRelation` key spec

No `[`/`]` lexeme exists today: `Lexer.cpp` dispatches `(` `)` `{` `}` `<` `>` etc.
(`:138`ff) but not brackets, and `Token.h`'s `kPunc*` enum (`:224-252`) has no
bracket entries. So the surface is a lexer + token addition, not a pure parser
tweak (complexity note: this is real, small, and self-contained).

```diff
  # include/drlojekyll/Lex/Token.h, Lexeme enum (:224-252)
+ kPuncOpenBracket,     // `[`  — region/instance key spec opener
+ kPuncCloseBracket,    // `]`
  # lib/Lex/Lexer.cpp char dispatch (mirrors the `(` arm at :138)
+ case '[': ... Store<Lexeme>(kPuncOpenBracket); ...
+ case ']': ... Store<Lexeme>(kPuncCloseBracket); ...
  # lib/Lex/Token.cpp spelling: add both spellings.
```

The bracket rides on INTERNAL-relation declarations only — `#local` / `#export`,
parsed by `ParseLocalExport` (`Parser.cpp:354`). It is NOT on `#functor` (owns
`bound/free/aggregate/summary`), NOT on `#message` (I/O), and the `#query` head
keeps its positional `bound/free` adornment (`Query.cpp:9`) — the query WRAPS a
region (owner: "region calls as body atoms; query wraps region"), it does not
grow a bracket. The bracket slots between the name (state 1) and the parameter
list's `(` (the `case 1` → `kPuncOpenParen` transition at `Parser.cpp:421-433`):

```diff
  # ParseLocalExport state machine (Parser.cpp:406ff), NEW state between 1 and 2:
  case 1:  # after the name atom
-   if kPuncOpenParen: state = 2 ...        # begin parameter list
+   if kPuncOpenBracket: state = 1a ...     # begin OPTIONAL key spec
+   else if kPuncOpenParen: state = 2 ...   # no bracket: today's path, byte-identical
+ case 1a:  # inside [ … ] — an ORDERED, duplicate-free list of NAMED vars
+   collect kIdentifierVariable tokens separated by kPuncComma;
+   PARSER OBLIGATION 1 (owner, all-4-modes reject): reject kIdentifierUnnamedVariable
+     (`_`) and any wildcard/anonymous — "region key columns must be named";
+   PARSER OBLIGATION 2 (owner, all-4-modes reject): reject a repeated var —
+     "a region key is a duplicate-free ordered column set";
+   on kPuncCloseBracket: stash the ordered key-var list; state = 2.
+   # RESOLVE-time (after parameter names bind): reject any bracket var that is
+   # not one of this relation's declared parameters — "unknown key column".
```

```diff
  # include/drlojekyll/Parse/Parse.h — ParsedRelation gains an OPTIONAL key spec:
+ // Empty unless the decl was written `rel[K…](…)`. Ordered (physical/arrangement
+ // hint — DIFF-R5); the SET is the logical region key. Each entry is a
+ // parameter index into Parameters(), named + repetition-free by construction.
+ bool HasDeclaredRegionKey(void) const noexcept;
+ std::vector<unsigned> DeclaredRegionKey(void) const noexcept;   // parameter indices
```

Obligations 1 & 2 and the resolve check are `-demand`-INDEPENDENT (they fire in
every mode on the malformed decl before any transform runs), so each is a genuine
all-4-modes directed witness. The ORDER is captured but is a DIFF-R5 physical
hint (see R3-P3) — R3a reconciles the SET only.

---

### R3-P2 — the inference→checking fork in `ApplyDemandTransform`

Part R R.1.1 Loop 1: Step 2 derives `p_bound` from the query adornment by tracing
back through forwarding TUPLEs to `p`'s MERGE; Step 3 classifies guard sites and
runs the fences. The fork inserts a CHECK between them — it does not skip the walk.

```diff
    # Part R R.1.1, Loop 1 (Demand.cpp), between Step 2 (:507-600) and Step 3 (:607-783):

    # Step 2 (:507-600): trace each bound column of q_insert back through
    # forwarding TUPLEs / the relation's own post-Connect MERGE to a single
    # full-width reader TUPLE `q_read` over p's MERGE `p_merge`; record
    # `q_consumer` (q_read's sole consumer) and `p_bound` (bound positions in p).

+   # Step 2b (NEW, R3a — inference→checking): if p's relation carries a declared
+   #   region key (ParsedRelation::HasDeclaredRegionKey):
+   #     declared := set(p_relation.DeclaredRegionKey())         # parameter indices
+   #     if set(p_bound) != declared:                            # V-DECLARED-KEY
+   #       reject("declared region key disagrees with the demanded binding")
+   #         # R3a is set-reconciliation ONLY (no Minimize; that is Stage B).
+   #         # MANDATE-arm alternative (declared OVERRIDES p_bound) is R3-STOP.
+   #   else: p_bound stays inferred (today's behavior — COMPLEMENT, not replace).

    # Step 3 (:607-783): per member (rule body) of p_merge.merged_views: …   # UNCHANGED
    #   (all fences — NEGATE/AGG sink :648-651, sideways :693/:748, second-read
    #    :656-660, and Step 4's stray-consumer union — run regardless of the bracket;
    #    a declared key does NOT lift a fence — that is DIFF-R4.)
```

Because Step 2b is a pure equality check inserted after the existing derivation,
and it changes no mint (Loop 2 / Phase 2 is untouched), a program whose declared
key MATCHES the inferred one lowers byte-identically to the un-bracketed program.
That is the answer-identity property, discharged by construction rather than by
re-derivation. A MISMATCH is a clean diagnostic (the `num_errors != log.Size()`
idiom already threaded through the pass), never a miscompile.

---

### R3-P3 — contract reconciliation + dump; the logical/physical split

The Stage-A flat-key contract (Part R R.1.4, `RowContract.cpp:365`) is the
CHECKER's authority for "is this a real column set." R3a adds a reconciliation and
a dump field; it does NOT change `TransferContract` (the demanded relation `p`
keeps `AllFields`/passthrough as today — the declared key is a DEMAND key, a
subset of the row's `member_key`, not the row identity).

```diff
  # ValidateRowContracts (RowContract.cpp:413-419) gains one always-on arm:
+ CheckDeclaredRegionKey(impl)   # for every relation with HasDeclaredRegionKey:
+   #   declared ⊆ visible_fields of the relation's contract   (well-formedness,
+   #     redundant with R3-P1 resolve but belt-checked over the FINAL graph);
+   #   record the declared ORDER as an inert DIFF-R5 physical-arrangement hint —
+   #     NOT reconciled in R3a (owner logical/physical split: canonicalize the
+   #     SET for equivalence, defer the sort ORDER to the D5 arrangement knob).
  # -contract-out dump (Format.cpp R.1.4 block): per view, add when present:
+   "  declared_key=(K…, by column name)  order=<positional-list>"
```

**Logical vs physical (owner refinement, feeds DIFF-R5).** `rel[A,C]` and
`rel[C,A]` are the SAME region logically (the reconciled SET) but DIFFERENT
arrangements physically (the order). R3a reconciles the SET against `p_bound` and
`visible_fields`; the ORDER is captured and dumped as inert. The `-demand-instance`
InstanceStore key stays positionally derived (Part R R.1.5 §E) in R3a — honoring a
non-positional declared order (re-keying the store) is DIFF-R5 physical-arrangement
work, gated on D5. Stating this here keeps R3a from silently re-ordering a store.

---

### SOUNDNESS OBLIGATIONS

- **O-R3.1 Answer-identity (I0 referee).** flat == declared == nested. Discharged
  BY CONSTRUCTION in R3a: the bracket adds only a CHECK (Step 2b) + validators;
  it mints no node and changes no lowering, so a matching bracket is byte-identical
  to the un-bracketed program and a mismatch rejects. R3b (declared drives
  lowering) must re-discharge this against I0 — deferred with the STOP.
- **O-R3.2 Orthogonality / complement-not-replace.** With no declared bracket the
  entire corpus is byte-identical in all 4 modes AND under `-demand`/`-demand-instance`
  (the fork's `else` arm is today's path verbatim). The parser obligations are the
  only always-on additions and fire only on malformed decls.
- **O-R3.3 Well-formed key.** Bracket vars are NAMED (reject `_`/anonymous),
  repetition-free, and resolve to actual parameters — enforced at parse+resolve
  (R3-P1) and belt-checked over the final graph (`CheckDeclaredRegionKey`).
- **O-R3.4 Fence preservation.** A declared key does not lift any demand fence.
  A bracket on a relation with a NEGATE/AGG sink, a sideways/left-linear body, a
  second self-read, or a stray consumer still rejects via the unchanged Step 3 /
  Step 4 (Demand.cpp :648-651 / :693/:748 / :656-660 / :797-826). Lifting these is
  DIFF-R4, not DIFF-R3.
- **O-R3.5 Functional-key validity deferred honestly.** R3a proves only the
  STRUCTURAL property (declared == inferred set; declared ⊆ visible). The stronger
  "the declared columns are a real functional key of the region answer" needs Stage
  B `Minimize`/`DeterminedBy` and is a DIFF-R5 obligation — not claimed by R3a.
- **O-R3.6 Determinism.** The checker is a pure function of the final graph +
  parse; no visitation-order dependence (mirrors the Stage-A contract discipline).

### OWNER DEPENDENCIES

- **STOP — hint-vs-mandate (OPEN; owner-adjudication-record "USER-DECLARED REGIONS"
  entry, TENSION (2); running-example thread).** LOAD-BEARING for R3b and for
  Step 2b's mismatch outcome. HINT: the declared bracket is planner input, a
  mismatch is a warning (or silently overridden by the cost-ranked factoring),
  answer-identity preserved by the planner. MANDATE: the declared bracket
  CONSTRAINS the lowering, a mismatch is a hard error, and the region is emitted as
  declared even against the transform's inference (needs the DIFF-R1 request-edge
  node to lower a declared region independent of the SIP walk). R3a formalizes the
  mismatch as a hard error under the *checking* reading (safe under either call);
  the *driving* reading is R3b and does not land until this is ratified. **Do not
  guess** — pick at DIFF-R3 authoring.
- **Sequence slot (owner RECOMMENDATION, D1.1 RATIFIED overall order).** DIFF-R3
  slots AFTER I0 + Stage B (owner-adjudication-record "USER-DECLARED REGIONS"
  RECOMMENDATION): I0 makes O-R3.1 checkable and Stage B gives C/D user-written
  test input independent of the demand transform. The exact slot within A→I0→B→C→D
  and "does it need its own stage doc" remain OPEN owner calls.
- **D3.4 candidate-3 (RATIFIED FLAT-KEY).** The Stage-A `member_key` IS the logical
  key the checker reconciles against; `Minimize`/antichain defer to Stage B —
  bounding R3a to set-reconciliation (O-R3.5).
- **Logical/physical split (owner refinement, RATIFIED framing; feeds DIFF-R5).**
  R3a reconciles the SET; the bracket ORDER is an inert DIFF-R5 arrangement hint.
- **D3.5 (RATIFIED) — PF-3 diagnostic vocabulary.** The fuzzing referee classes
  rejects by PF-3 vocabulary (demand-sink / R-MAT / R-BODYWALK, plus the new
  V-DECLARED-KEY and parser-obligation classes).
- **D2.6 (DEFERRED) — edge row schema.** Not needed by R3a (a surface + checker
  upstream of the request-edge node). R3b's declared-driven lowering IS D2.6-blocked
  — a reason to keep DIFF-R3's landable slice to the DECLARATION surface.
- **Parser obligations (owner 2026-08-03 session 3, RATIFIED as directed
  all-4-modes witnesses).** Named + repetition-free rejects (R3-P1).

### TEST / WITNESS PLAN

**Directed reject witnesses (all-4-modes, `-demand`-independent; encode in
`runall.sh` alongside the other expected-diagnostic cases).**
- `region_key_wildcard_1` — `foo[_](…)` → "region key columns must be named."
- `region_key_anon_1` — `foo[_A](…)` → same class (anonymous/wildcard var).
- `region_key_dup_1` — `foo[A,A](…)` → "duplicate-free ordered column set."
- `region_key_unknown_1` — a bracket var absent from `foo`'s parameters → resolve
  reject ("unknown key column").
- `region_key_mismatch_1` — a declared bracket whose set ≠ the SIP-inferred
  `p_bound` (`.drflags` = `-demand`) → V-DECLARED-KEY reject under the *checking*
  reading; its OUTCOME under the *driving* reading is STOP-gated (hint-vs-mandate).

**Positive witnesses (eqgate; referee = I0 / `run_eqgate`, flat == declared ==
nested + sorted published-delta identity).**
- `region_declared_tc_witness` — the `demand_tc_witness` graph re-authored with the
  bracket on the recursive relation declaring exactly the SIP-inferred key.
  `.drflags` `-demand`, `.eqgate` `-demand -demand-instance`. Asserts BYTE-identical
  stdout to `demand_tc_witness` across all 4 modes (the O-R3.1/O-R3.2
  "bracket is a no-op overlay when it matches" witness).
- Re-use `demand_neighborhood_witness` / `demand_neighborhood_mono_witness` and
  `demand_multi_adorn_witness` as bracket-annotated variants — the declared key
  over the multi-adornment pub exercises the logical-key reconciliation (SET) with
  the physical-order hint inert.

**Adornment-placement FUZZING (owner 2026-08-03 session 3; generator-based, the
evil-monkey rule applied to the declared surface — raises the matrix from
witness-based to generator-based).**
- **Generator.** Over a small set of base `.dr` programs — `demand_tc_witness`,
  `tc_nonlinear_diff` (the non-linear TC carrier), `demand_neighborhood_witness`,
  and the disassembler skeleton `tests/MiniDisassembler/database.dr` — mechanically
  enumerate/sample bracket placements over each internal relation: every key SUBSET
  × a sample of ORDERS × per-redeclaration, plus multi-adornment mixes and
  nested/composite non-prefix keys. **Complexity bound (guilty until inherent):**
  full enumeration is exponential in arity; cap it with a strength-2 COVERING ARRAY
  over (relation × key-subset × order) and a per-relation arity ceiling (fall back
  to sampling above it) — this slots BESIDE the existing covering-array machinery,
  not as a bespoke exhaustive loop.
- **Oracle + verdict (per placement).** Compile the base program flat (no bracket)
  under `-demand`, capture the I0 answer; compile the bracketed variant under
  `-demand` and `-demand-instance`; the eqgate referee requires flat == declared ==
  nested (answer-identity). Every placement MUST either (a) compile answer-identical,
  or (b) draw a CLEAN diagnostic classified by PF-3 vocabulary (D3.5) — and NEVER
  SIGABRT (exit 134) or stack-overflow (exit 139). The harness asserts the exit
  code is 0-with-identical-answers or a clean non-abort reject; an abort/miscompile
  is a hard failure (the F-record path).
- **Referee summary.** I0 + the flat-`-demand` compiler are the answer oracle;
  PF-3 is the diagnostic-class oracle; the covering array is the placement
  generator. No new golden is blessed for the fuzz arm — identity falls out
  transitively from the flat diffrun check, exactly as the `demand_neighborhood`
  eqgate does today.


### OPEN OWNER ITEMS (DIFF-R3)

- STOP: hint-vs-mandate (OPEN) — decides R3b (declared-driven lowering) and the Step-2b mismatch outcome; R3a lands under the safe checking reading, R3b waits on ratification + DIFF-R1 request-edge node + D2.6
- OPEN: exact DIFF-R3 slot within A→I0→B→C→D (owner recommends after I0 + Stage B) and whether DIFF-R3 needs its own stage doc
- DIFF-R5 dependency: honoring a non-positional declared bracket ORDER (re-keying the InstanceStore) is physical-arrangement work gated on D5; R3a keeps the order inert
- O-R3.5: the functional-key validity proof (declared columns are a real key) rides Stage B Minimize/DeterminedBy — not claimed by R3a's structural set-reconciliation

### PANEL RECORD (DIFF-R3) — 14 findings, 14 survived refutation

- **DIFF-R3-correctness-lifecycle-1** [CONFIRMED; sev MAJOR] Step 2b reconciles a SINGLE per-relation declared bracket against a PER-ADORNMENT `p_bound`, so any multi-adornment demanded relation whose adornments have differing bound sets is forced to reject on all but one adornment — directly contradicting the test plan's claim that `demand_multi_adorn_witness` can be reused as an answer-identical bracket-annotated variant.
  - EVIDENCE: Demand.cpp:477 `for redecl in q_decl.UniqueRedeclarations()` is the per-adornment loop; `p_bound` is built fresh inside it (declared :510, filled `p_bound.push_back(in_col->Index())` :576) and stashed per-adornment at :792-794 (PerAdornment). Pseudocode R.1.1 confirms Step 2 AND Step 3 sit inside Loop 1. R3-P2 inserts Step 2b `if set(p_bound)!=declared: reject` between :507-600 and :607-783, i.e. inside that loop, so the ONE relation-scoped `ParsedRelation::DeclaredRegionKey` is compared against EACH adornment's p_bound. demand_multi_adorn_witness.dr (`#local rel(u64 A,u64 B)` demanded under `q(bound A,free B)`→p_bound={0} and `q(free A,bound B)`→p_bound={1}) has DISJOINT bound sets, so any single bracket matches at most one adornment and hard-rejects the other. The TEST/WITNESS PLAN's positive-eqgate reuse of demand_multi_adorn_witness is therefore unbuildable. The two statements cannot both hold.
  - AMENDMENT (normative): Make the region key ADORNMENT-scoped, not relation-scoped, in the model. Adopt option (a): scope R3a declared brackets to single-adornment demanded relations, add an all-4-modes reject witness `region_key_multi_adorn_1` (bracket on a name carrying >1 binding pattern), and DELETE the `demand_multi_adorn_witness` positive-reuse bullet from the TEST/WITNESS PLAN. If instead option (b) is chosen (accept iff declared == some adornment's p_bound), the diff must additionally state which of the N disjoint keyed stores the bracket/order then constrains. Until adjudicated, strike the false answer-identity claim for demand_multi_adorn_witness.
- **DIFF-R3-correctness-lifecycle-2** [SOFTENED; sev MAJOR] R3-P3's `CheckDeclaredRegionKey` reads 'visible_fields of the relation's contract', but no such singular object exists: row_contracts are keyed per-LIVE-VIEW (a relation owns a MERGE + SELECT + INSERT views), and a bracketed relation whose views were dead-flow-eliminated has ZERO contract entries — the iteration over parse-relations has no defined lookup and no teardown/skip story.
  - EVIDENCE: row_contracts is keyed per-VIEW: RowContract.cpp accesses `impl->row_contracts.at(v)`/`.count(v)` (:258,:278,:328,:352), and V-CONTRACT-CENSUS (:254-272) asserts exactly one contract per LIVE view and NONE for a dead view. Pseudocode R.1.4:1206 is `out[v] = {...}` per view. A relation maps to multiple views (p_merge + reader TUPLE + SELECTs + INSERTs), so R3-P3's `visible_fields of the relation's contract` has no single referent, and a dead-flow-eliminated bracketed #local has ZERO contract entries. The gap is real. BUT the 'deref/UB' framing overshoots: the codebase's `.at()` idiom aborts/throws rather than UB, R3-P1 parse/resolve already discharges soundness (the finding concedes 'not a soundness hole'), so this is a spec-clarity gap, not MAJOR.
  - AMENDMENT (normative): Specify the canonical contract view CheckDeclaredRegionKey consults (state 'the demanded relation's MERGE view p_merge, else the full-width reader TUPLE'), and add an explicit early-skip: if the bracketed relation has no live view in row_contracts (dead-flow-eliminated), skip the belt-check — well-formedness is already discharged by the R3-P1 parse/resolve obligations. Add a directed case region_key_dead_relation_1 (bracket on an unused #local eliminated by dead-flow) proving the checker neither crashes nor rejects. Treat as a spec-clarity fix, not a soundness fix.
- **DIFF-R3-correctness-lifecycle-3** [CONFIRMED; sev MINOR] `region_key_mismatch_1` is scheduled as a blessed all-4-modes REJECT golden in runall.sh, which commits the diagnostic surface to the un-ratified 'checking' reading of the STOP; if the owner later picks HINT, mismatch must flip from hard-reject to warning-and-accept, forcing a re-bless of that witness.
  - EVIDENCE: The R3-STOP row and OWNER DEPENDENCIES both mark hint-vs-mandate OPEN and state region_key_mismatch_1's 'OUTCOME under the driving reading is STOP-gated', yet the TEST/WITNESS PLAN lists it under 'Directed reject witnesses (all-4-modes ... encode in runall.sh alongside the other expected-diagnostic cases).' Blessing a program as a hard reject commits the diagnostic surface; under a later HINT resolution the same program must accept-with-warning, forcing a re-bless. The finding correctly scopes this as MINOR (the reject is always sound) and correctly isolates the mismatch case (the four well-formedness rejects are STOP-independent).
  - AMENDMENT (normative): Hold region_key_mismatch_1 out of the committed runall.sh diagnostic list until hint-vs-mandate is ratified, OR encode it with an explicit case comment: 'checking-reading golden — RE-BLESS if STOP resolves to HINT (mismatch becomes a warning-and-accept)'. Land the four well-formedness rejects (wildcard/anon/dup/unknown) now; gate only the mismatch witness.
- **DIFF-R3-termination-confluence-1** [CONFIRMED; sev MAJOR] The Step-2b reconciliation `set(p_bound) != declared` is under-specified (not well-defined / not confluent) for a relation demanded under multiple adornments: there is no single `p_bound` to reconcile a single declared bracket against, and the diff's own positive eqgate witness (`demand_multi_adorn_witness`) is unbuildable under the rule as written.
  - EVIDENCE: Same code as finding-1: Demand.cpp:477 per-adornment loop, p_bound fresh at :510/:576, stashed :793; demand_multi_adorn_witness has disjoint p_bound {0}/{1}; Step 2b (R3-P2) inside the loop compares one relation-scoped bracket against each. The substantive defect — a single-multiplicity bracket surface over an N-multiplicity demanded-region model, making the listed POSITIVE eqgate witness unbuildable and defeating O-R3.1/O-R3.2 for any bracketed multi-adornment relation — is fully code-supported. (Framing nit: the ACCEPT/REJECT verdict is actually deterministic, so 'not confluent' is imprecise; the real defect is 'no single referent for the reconciliation', which the amendment addresses correctly.)
  - AMENDMENT (normative): Define the multi-adornment reconciliation semantics explicitly before landing (this duplicates finding-1's fix): either (a) scope R3a brackets to single-adornment demanded relations + add the multi-adorn reject witness and remove demand_multi_adorn_witness from the positive list, or (b) reconcile against the disjunction (accept iff declared == some adornment's p_bound) and state which region the bracket/order then constrains, or (c) move the bracket onto the per-adornment #query surface. Pick one and update O-R3.1/O-R3.2 and the TEST/WITNESS PLAN. Sharpen the claim wording from 'not confluent' to 'no single referent for reconciliation.'
- **DIFF-R3-termination-confluence-2** [CONFIRMED; sev MINOR] The new parser state `1a` (bracket key-var collection loop) is introduced with no stated terminating/error transition for the missing-`]` (EOF or unexpected-token) case; the diff gives no finiteness argument for the one loop it adds to the parser.
  - EVIDENCE: R3-P1's state 1a enumerates only the kIdentifierVariable / kPuncComma / kPuncCloseBracket transitions ('on kPuncCloseBracket: state=2') and gives no edge for EOF or an unexpected token. The grep confirms no bracket lexeme/state exists today (Lexer.cpp/Token.h have none), so state 1a is genuinely NEW parser code with no inherited exit. A literal `while(peek!=kPuncCloseBracket)consume()` transcription runs off the token buffer on a missing `]`. Finiteness holds only because the token stream is finite; the transition table as written is incomplete for the introduced loop.
  - AMENDMENT (normative): Add to state 1a the explicit terminating transitions: on EOF or any token other than kIdentifierVariable/kPuncComma/kPuncCloseBracket, emit a clean diagnostic ('expected `]` to close region key spec' / 'unexpected token in region key') and abort the decl parse via the existing malformed-decl reject idiom, so the loop provably exits on every input.
- **DIFF-R3-testability-oracle-1** [SOFTENED; sev MAJOR] The central R3a property — 'the overlay mints no node and changes no lowering' (O-R3.1/O-R3.2) — is witnessed only by stdout identity (region_declared_tc_witness asserts 'BYTE-identical stdout to demand_tc_witness'), never by IR-dump identity, even though demand_tc_witness already carries .df.opt/.rel.opt/.contract.opt/.ir.opt/.h.opt goldens. Stdout identity is necessary but not sufficient: a checker that accidentally mutated the graph (e.g. a side-effecting CheckDeclaredRegionKey, or Step 2b perturbing p_bound) can leave stdout answer-identical while the lowering diverges. The test as written does not discriminate 'no lowering change' from 'same answer'.
  - EVIDENCE: Confirmed demand_tc_witness carries committed .df.opt/.rel.opt/.contract.opt/.ir.opt/.h.opt goldens (ls in tests/OptDiff/goldens). The core is right: O-R3.1's 'mints no node, changes no lowering' is unwitnessed by stdout alone — a side-effecting checker could leave stdout answer-identical while perturbing the graph, and the epoch's predict-then-verify-IR discipline exists to catch exactly that. BUT the amendment's byte-EQUAL set overshoots by including -contract: R3-P3 INTENTIONALLY adds a `declared_key=`/`order=` line to the bracketed witness's contract dump, so region_declared_tc_witness's -contract output will NOT be byte-equal to demand_tc_witness's contract golden.
  - AMENDMENT (normative): For region_declared_tc_witness, require its -df/-rel/-ir/-h dumps in all 4 modes to be BYTE-EQUAL to demand_tc_witness's existing *.opt.golden files (a same-file cross-compare, no new golden blessed) — that referee witnesses 'changes no lowering'. EXCLUDE -contract from the strict byte-equal set: its dump must equal demand_tc_witness's modulo exactly the added declared_key/order line (pinned separately per oracle-4). Keep the stdout eqgate as the answer check on top.
- **DIFF-R3-testability-oracle-2** [CONFIRMED; sev MAJOR] O-R3.4 (a declared key does NOT lift any demand fence) has no directed witness. Every fence-reject witness in the corpus is bracket-free, so none discriminates 'bracket present, fence still fires'. Worse, Step 2b (V-DECLARED-KEY) is inserted BEFORE Step 3's fences, so a MISMATCHED bracket masks the fence entirely — the only way to actually reach and test fence preservation is a MATCHING bracket on a fenced body, and no such witness is listed.
  - EVIDENCE: Step 2b is inserted between Step 2 (:507-600) and Step 3 (:607-783); the fences all live in Step 3: NEGATE/AGG demand-sink at Demand.cpp:648-651, self-join second-read at :656-660, sideways/left-linear at :693 and :748. So V-DECLARED-KEY fires BEFORE any fence — a mismatched bracket on a fenced body rejects on V-DECLARED-KEY, proving nothing about fence preservation. Every existing fence-reject witness is bracket-free, so none discriminates 'bracket present, fence still fires'. O-R3.4 is therefore unrefereed; the only construction that reaches a fence past Step 2b is a MATCHING bracket, and no such witness is listed.
  - AMENDMENT (normative): Add a directed all-4-modes reject witness region_declared_fenced_1: a relation carrying a bracket that MATCHES the inferred p_bound (so Step 2b passes) AND a NEGATE/AGG or second-self-read body, asserting the reject is classified as the FENCE class (PF-3 demand-sink / R-BODYWALK), NOT V-DECLARED-KEY. This is the only construction that discriminates 'the bracket did not lift the fence.'
- **DIFF-R3-testability-oracle-3** [CONFIRMED; sev MAJOR] The adornment-placement fuzz generator enumerates '(relation × key-subset × ORDER)' as a strength-2 covering array with the eqgate answer oracle as verdict, but in R3a the ORDER is explicitly inert: R3a reconciles the SET only, the -demand-instance store 'stays positionally derived' (R3-P3). Two placements differing only in bracket order (rel[A,C] vs rel[C,A]) therefore lower and answer identically, so the ORDER dimension discriminates zero R3a behavior against the eqgate oracle — it burns covering-array budget on a dimension whose sole observable (the `order=` dump field) the fuzz oracle never inspects.
  - EVIDENCE: R3-P3 states the store key 'stays positionally derived (Part R R.1.5 §E) in R3a — honoring a non-positional declared order ... is DIFF-R5', and 'rel[A,C] and rel[C,A] are the SAME region logically ... the ORDER is captured and dumped as inert.' The fuzz oracle is 'flat == declared == nested' (answer identity). Answer identity is invariant under bracket-order permutation in R3a by that design, so the ORDER axis of the strength-2 covering array (relation × key-subset × order) produces no distinguishable eqgate verdict — only the un-inspected `-contract-out` `order=` field varies. Confirmed budget waste on a non-observable dimension.
  - AMENDMENT (normative): Drop ORDER from the R3a covering array (defer it to the DIFF-R5 fuzz matrix where the store re-keys). If order-capture must be exercised in R3a, add a SEPARATE referee that byte-compares the `-contract-out` `order=` field against the declared spelling for a handful of ordered brackets — the answer oracle cannot see it.
- **DIFF-R3-testability-oracle-4** [CONFIRMED; sev MAJOR] R3-P3 adds two `-contract-out` dump fields — `declared_key=(K…, by column name)` and `order=<positional-list>` — but the TEST/WITNESS plan lists no contract-dump golden pinning them. The new dump surface has no referee, so a regression in how the declared key/order is rendered (or a failure to emit it at all) is invisible.
  - EVIDENCE: The contract-golden machinery is live and demand_tc_witness carries demand_tc_witness.contract.opt.golden (confirmed via ls). R3-P3 adds two rendered fields to -contract-out (`declared_key=(K…, by column name)` and `order=<positional-list>`), but the TEST/WITNESS PLAN's positive witnesses assert stdout only and the reject witnesses exercise malformed brackets — nothing pins the RENDERED declared_key/order fields of a WELL-FORMED accepted bracket. The IR-observability directive requires new dump surfaces be pinned; this one is unrefereed, so a render regression (or a failure to emit) is invisible.
  - AMENDMENT (normative): Add a contract-dump golden for at least one accepted bracketed witness (e.g. region_declared_tc_witness.contract.opt.golden) exhibiting a non-empty `declared_key=` line, byte-pinning the emitter — the standing pattern for every other dump field in the epoch. (Coordinate with necessity-4/oracle-3: if the inert `order=` token is deferred to DIFF-R5, pin only declared_key= now.)
- **DIFF-R3-testability-oracle-5** [SOFTENED; sev MINOR] The INFERRED p_bound is not dumped anywhere; only the DECLARED key is added to -contract-out 'when present'. Constructing the match witness (region_declared_tc_witness 'declaring exactly the SIP-inferred key') and locating the match/mismatch boundary for region_key_mismatch_1 therefore requires the test author to reverse-engineer the SIP result by guessing, with the only feedback being pass/reject. The match boundary — the exact behavior R3a checks — is itself unobservable.
  - EVIDENCE: Confirmed: R3-P3 dumps `declared_key` only 'when present', and Step 2's `p_bound` (Demand.cpp:510) is an internal derivation with no emission. The observability gap is real — a witness author cannot read off the SIP-inferred key they must declare to make region_declared_tc_witness a no-op, and the match/mismatch boundary is unobservable. BUT the amendment 'emit ... for every demanded relation, bracketed or not' overshoots: existing demand cases run under -demand and DO carry contract goldens (demand_tc_witness.contract.opt.golden), so an unconditional inferred-key line would churn every existing demand contract golden.
  - AMENDMENT (normative): Emit the inferred demand key (p_bound, by column name) into -contract-out, but SCOPE it to the reconciliation context (only when a bracket is present on that relation, or accept a one-time re-bless of the existing -demand contract goldens rather than churning them silently). This makes the match/mismatch boundary observable and lets region_declared_tc_witness be constructed without guessing, without an unbudgeted golden churn across the whole demand corpus.
- **DIFF-R3-necessity-1** [CONFIRMED; sev MAJOR] R3a is a capability-free overlay — under set-reconciliation the declared bracket carries ZERO information the compiler doesn't already derive, so the entire R3a surface (lexeme + token + parser state 1a + two ParsedRelation accessors + contract arm + dump field + fuzz harness) adds no behavior and could be deferred whole into R3b, or shrunk to a lint.
  - EVIDENCE: The formalization's own SHRINK point 1 keeps Step 2 (Demand.cpp:507-600) deriving p_bound UNCHANGED and inserts Step 2b as a pure equality check; O-R3.1 states R3a 'mints no node and changes no lowering'; the R3-STOP row concedes the bracket only DRIVES lowering in R3b. So under set-reconciliation the declared bracket carries zero information the compiler doesn't already derive — a matching bracket is byte-identical, a mismatch rejects — its sole effect is a lint asserting 'inferred key == what I wrote.' Building a lexeme + token + parser state 1a + two ParsedRelation accessors + an always-on contract arm + a dump token + five reject witnesses + two eqgate witnesses + a covering-array fuzz arm to ship a no-op overlay is mechanism far exceeding the delivered (lint) capability.
  - AMENDMENT (normative): Prefer option (b): SHRINK R3a to exactly the lint — the parser surface + Step 2b equality reject — and DROP the contract-layer CheckDeclaredRegionKey arm (see necessity-2), the order= capture/dump (necessity-4), and the covering-array fuzz arm (necessity-3), none of which a no-op overlay justifies. Note in the diff that the parser+AST surface is worth landing early only insofar as it de-risks R3b (surface-laying value); state explicitly what R3a buys over a deferred R3b so the sequencing is a necessity argument, not a process one. If even the lint value is judged thin, defer the whole surface to R3b where it buys a real capability and answer-identity becomes a non-trivial obligation.
- **DIFF-R3-necessity-2** [SOFTENED; sev MAJOR] R3-P3's CheckDeclaredRegionKey duplicates the R3-P1 resolve-time check (the two-authorities disease) — its 'declared ⊆ visible_fields' arm is provably vacuous for the R3a scope, and its reconciliation overlaps Step 2b.
  - EVIDENCE: Partly right: R3-P1 resolve proves declared ⊆ Parameters(); for a DEMANDED relation p keeping AllFields/passthrough (R3-P3), visible_fields == params, so `declared ⊆ visible_fields` is vacuous there and the content lives in Step 2b's `declared == p_bound`. BUT 'provably vacuous for the R3a scope' overshoots: brackets ride on ANY internal #local/#export, not only demanded ones, and the parse-resolve check runs PRE-optimization. A NON-demanded bracketed relation whose declared-key column is dropped by canonicalization survives resolve but is absent from the FINAL graph — a mismatch only a final-graph belt (CheckDeclaredRegionKey ⊆ visible_fields) can catch. The finding's own fallback concedes exactly this scenario, so 'delete outright' is wrong.
  - AMENDMENT (normative): Do not delete CheckDeclaredRegionKey. Scope it to the one non-redundant property: 'every declared-key column of a bracketed relation still resolves to a live visible column in the FINAL graph' (a column-survival-over-optimization check that pre-optimization parse-resolve cannot provide). Drop the redundant framing that overlaps Step 2b's `declared == p_bound`. State the canonicalization-column-drop scenario in the diff as its justification (this also composes with the correctness-lifecycle-2 dead-view early-skip).
- **DIFF-R3-necessity-3** [SOFTENED; sev MAJOR] The covering-array adornment-placement fuzz arm is disproportionate to R3a: its answer-identity oracle tests a property that holds BY CONSTRUCTION, leaving only parser-robustness as the real signal.
  - EVIDENCE: The ORDER dimension is genuinely inert in R3a (agrees with oracle-3: store stays positionally derived, answer-identity invariant under order permutation) — that part is right. BUT 'answer-identity holds by construction, leaving only parser-robustness' overshoots on the key-SUBSET dimension: for a relation with p_bound={0,2}, the covering array systematically exercises V-DECLARED-KEY's boundary — the exact-match subset must accept-byte-identical while every other subset must draw a CLEAN reject and NEVER SIGABRT. That is real, non-tautological signal (it verifies the by-construction claim empirically AND that the checker never aborts across placements), not merely parser robustness.
  - AMENDMENT (normative): For R3a, DROP the ORDER dimension from the covering array (inert), but KEEP the key-SUBSET × redeclaration covering array: it exercises V-DECLARED-KEY's accept-exact-match / clean-reject-otherwise boundary and abort-safety across placements. Do not collapse the whole arm to a parser-token fuzz. Move the ORDER-sensitive fuzz to DIFF-R5 where the store re-keys and order becomes observable.
- **DIFF-R3-necessity-4** [SOFTENED; sev MINOR] The declared ORDER capture into the contract layer and the `-contract-out` `order=<positional-list>` dump field are inert in R3a — mechanism with no R3a consumer.
  - EVIDENCE: The inert-order concern is legitimate and code-consistent: R3-P3 states the order is 'NOT reconciled in R3a', 'inert', the store 'stays positionally derived (Part R R.1.5 §E)', and honoring it is 'DIFF-R5 ... gated on D5' — so recording it in the contract layer and emitting an `order=` golden token adds a golden-visible field nothing in R3a consumes. BUT this DIRECTLY CONFLICTS with oracle-4 (which wants order= pinned), and the second half — collapse HasDeclaredRegionKey() into !DeclaredRegionKey().empty() — is a stylistic non-issue: a named convenience predicate is conventional and aids readability, not redundant mechanism.
  - AMENDMENT (normative): In R3a, capture the ordered key-var list ONLY at the parse layer (ParsedRelation::DeclaredRegionKey), reconcile the SET only, and defer BOTH the contract-layer order recording AND the `order=` -contract-out token to DIFF-R5, where the store actually re-keys and consumes the order (this resolves the tension with oracle-4: pin declared_key= now, add order= at DIFF-R5). Drop the accessor-collapse suggestion — keep HasDeclaredRegionKey() as a readable convenience predicate.

---

## DIFF-R4 — Lazy vs force-complete request edges (the negation/aggregate barrier) (formalized 2026-08-03)

Formalizes the seed's DIFF-R4 (`region-model-pseudocode-seed.md` Part 2) against
Part R (`regional-arch-pseudocode.md`, session 3, fleet-verified). Goal: type the
DIFF-R1 request edge's laziness by the monotonicity of what CONSUMES its answer,
so the negation/aggregate demand-body reject becomes a `FORCE_COMPLETE` edge kind
where it can be soundly ordered — and stays a reject where it cannot. Anchors
re-verified against the `keyed-instances` tip this pass.

**Two seed over-specifications shrunk up front** (complexity is guilty until
proven inherent):

1. **No new "quiescence-detection mechanism" is needed** (the seed's "a NEW
   quiescence-detection mechanism", Part-2 line 220). For an acyclic activation
   DAG the barrier is ALREADY the existing per-stratum ascending order +
   `DeriveDRStrata` monotone lift (Rel.cpp:3093) + V-READY ("no read of a
   strictly-higher stratum", R.1.3). "Quiescent for the key domain" reduces to a
   stratum-ordering constraint the linearizer already enforces. The only concrete
   codegen change is *pulling a force-complete instance's build into its own
   activation stratum*, exactly mirroring GROUP_UPDATE (which R.1.6 already
   stratifies INSIDE the per-stratum walk at band 0, unlike SUBGRAPHINSTANCE).

2. **KV is NOT in DIFF-R4's scope.** The seed (Part-2 line 213) and the owner note
   (`running-example-disassembler.md`:274) group "negation AND aggregate/KV". But
   per PF-3 (D3.5 RATIFIED; `feature-mixing-verified.md`:50-51) a KV `mutable()`
   body hits the **R-MAT** "≠1 materialization" reject at Demand.cpp:446-448, a
   materialization-COUNT check that fires *before* the demand-sink body walk ever
   runs — orthogonal to monotonicity. DIFF-R4 lifts only the **demand-SINK**
   family (NEGATE + AGG, both algebras). KV admission additionally requires
   lifting R-MAT and is deferred (STOP-R4-KV).

Also corrected: the seed's "the differential flags (`can_receive/produce_deletions`)
already carry that bit" (owner note :270) is imprecise. `CanReceiveDeletions` is
the negated/demand relation's own retractability axis (P-DEATH; it is what
`IsCutSuccessorDR` reads at Rel.cpp:1567-1573), NOT the consumer's monotonicity.
`edge_kind` keys on the CONSUMER NODE KIND — the exact, build-time-decidable form
of "non-monotone consumer."

---

### The inference rule (diff on R.1.1 Step 3 / DIFF-R1's REQUEST node)

`edge_kind` is a pure function of the node that consumes the request's answer
columns — decided at the mint site, folded into the request edge's structural
identity (the F1 lesson: no satellite annotation for a later pass to preserve):

```
edge_kind(request R feeding consumer C):
  if C.IsNegate() or C.IsAggregate():   FORCE_COMPLETE   # non-monotone: absence /
                                                          #   summary is not irrevocable
  else:                                 LAZY             # JOIN pivot / MERGE arm /
                                                          #   TUPLE / CMP / MAP / INSERT
  # KVINDEX consumer is unreachable here — R-MAT rejects it upstream (STOP-R4-KV).
```

The LAZY arm is the already-landed regime: the e5 witness
`demand_diff_neighborhood_witness` (bare `-demand`, `kInstanceDeath=0` beside
`kSubgraphInstantiate=1`) is precisely a monotone consumer reading a partial
answer sound by IRREVOCABILITY — soundness-by-irrevocability, not quiescence
(CLAUDE.md D3.a.2 e5; the CALM-monotone / free-termination case, arXiv 2502.00222).
DIFF-R4 adds only the FORCE_COMPLETE arm.

---

### What replaces the reject (PF-3 vocabulary; three sites, one lifted)

Part R's corrected account names three distinct demanded-body rejects. DIFF-R4
touches exactly one:

| Site | Anchor | Fires on | DIFF-R4 |
| --- | --- | --- | --- |
| demand-SINK | Demand.cpp:648-651 | `!`-NEGATE, `@never`, `over(){}` AGG, config `@recompute` AGG | **LIFTED** → FORCE_COMPLETE sub-request (DOWN only) |
| R-MAT (≠1 materialization) | Demand.cpp:446-448 | KV `mutable()` body | **UNTOUCHED** (STOP-R4-KV) |
| R-BODYWALK (literal) | Demand.cpp:613/645/653/670/… | `@product`, `@barrier`/`:-` chain, mutual-recursive content | **UNTOUCHED** (separate gaps) |

**Hunk R4-1 — the demand-SINK walk (R.1.1 Step 3).** Part R reads:

```diff
     # Step 3 (:607-783): per member (rule body) of p_merge.merged_views:
-    #   - walk the body tree; reject on NEGATE/AGG (demand sink, :648-651),
+    #   - walk the body tree; on NEGATE/AGG (the demand sink, Demand.cpp:648-651):
+    #       Stage 1 (now):  reject  — the conservative barrier (UNCHANGED)
+    #       Stage 2 (DIFF-R1 landed): mint REQUEST(neg_input or agg_input,
+    #         key = the gate's join key, edge_kind = FORCE_COMPLETE) and CONTINUE
+    #         the SIP walk INTO that input under the force-complete regime, IFF
+    #         the negated/summarized activation is a strictly-LOWER activation-SCC
+    #         (a DOWN edge, DIFF-R2); an ACROSS edge stays a reject (see R4-4).
     #     a second read of p (self-join, :656-660), or any un-witnessed view.
```

The reject is the conservative stand-in for a force-complete request that cannot
yet be lowered. Rationale for the sub-request: `p(X) :- q(X), !r(X)` demanded on
`X` cannot magic-restrict `!r(X)` monotonically (the classic magic-sets-through-
negation barrier); the sound rewrite is "demand `r` on the projected gate key,
drive `r`'s demanded evaluation to completion for that key, THEN read absence."
That IS a FORCE_COMPLETE request edge from the negate gate to `r`.

**Hunk R4-2 — the HP-4 recognizer-refusal belt (R.1.2, Rel.cpp:1062-1070).** The
belt aborts (fprintf+abort, survives NDEBUG) if the re-resolved nested-lowering
input is a MAP/NEGATE/AGG/KVIndex — it TRUSTS the upstream demand-sink reject. If
R4-1's reject is lifted without relaxing the belt, an admitted negate/agg input
aborts the compile. Part R reads:

```diff
     # HP-4 recognizer-refusal belt: input MUST be a plain table-bearing view
-    # (never MAP/NEGATE/AGG/KVIndex) — a belt, not the induction-owned fence.
-    assert not (ri.input_view.{IsMap,IsNegate,IsAggregate,IsKVIndex}())  # :1066-1070
+    # (never MAP/KVIndex; and never NEGATE/AGG UNLESS reached via a FORCE_COMPLETE
+    # request edge whose activation stratum was pulled in-walk, R4-3) — a belt.
+    assert not ri.input_view.IsMap() and not ri.input_view.IsKVIndex()
+    assert (not (ri.input_view.IsNegate() or ri.input_view.IsAggregate()))
+           or (ri.request_edge.kind == FORCE_COMPLETE and ri.instance_in_walk)   # :1066-1070
```

Belt stays LOUD for every shape DIFF-R4 does not admit; it relaxes exactly and
only for the force-complete-and-in-walk case.

---

### The per-key quiescence obligation, operationally (priority-scrutiny axis)

"Target region quiescent for the requested key domain" in Part R's epoch model
(R.1.6): one received batch == one epoch; the commit/Seal boundary
(`PublishDifferentialMessageVectors` → `LowerCommitSweeps`, Procedure.cpp:589) is
the SOLE publish point. The obligation is that the negate/agg gate reads an answer
that will not later grow *within this epoch* for the keys it gates on.

**Acyclic activation DAG — the barrier ALREADY exists.** Relation-level Stratify
(R.1.3, Stratify.cpp:272-297) already places a negation in a strictly-higher
stratum than its negated relation and rejects the equal-stratum case; the
ascending per-stratum loop runs the negated relation to fixpoint before the gate.
Flat `-demand` does not perturb this — the demand relation is JUST ANOTHER
RELATION, guard-JOINs fire in ORDINARY strata (R.1.5 §D). So:

- **Flat `-demand`, DOWN edge:** the negated relation sits in a lower stratum,
  quiescent-for-all-keys before the gate — the barrier is free. The ONLY change is
  in Demand.cpp: admit the SIP walk to push a force-complete restriction through
  the sink. No scheduling change, no new runtime primitive.
- **`-demand-instance`, DOWN edge:** the negated relation is a nested InstanceStore
  activation, but `LowerSubgraphInstances` publishes+Seals at the EPOCH TAIL
  (Procedure.cpp:588), AFTER all strata — an inversion: a mid-stratum gate cannot
  read a tail-sealed instance. This is the exact reason nested lowering rejects
  negate/agg today. The bounded fix is the GROUP_UPDATE precedent (R.1.6:
  GROUP_UPDATE is emitted band-0 at its own lifted stratum INSIDE the walk;
  SUBGRAPHINSTANCE is not): pull the force-complete instance's Instantiate+Seal
  into its activation stratum so a higher-stratum gate reads a sealed instance.

**Hunk R4-3 — force-complete instance stratified in-walk (R.1.6).** Part R reads:

```diff
   BuildStratumPhases(...)                                  # :1042 = Rel-IR Step 5
     # GROUP_UPDATE (agg/KV) IS emitted HERE, band 0, at the agg view's OWN lifted
     # stratum (op_band(kGroupUpdate)=0u) — the ASYMMETRY: GROUP_UPDATE is
-    # stratified INSIDE the per-stratum walk; SUBGRAPHINSTANCE is NOT.
+    # stratified INSIDE the per-stratum walk; SUBGRAPHINSTANCE is NOT — EXCEPT a
+    # FORCE_COMPLETE-consumed instance, which (DIFF-R4) is ALSO pulled in-walk at
+    # its activation stratum so a higher-stratum NEGATE/AGG gate reads it SEALED.
+    # DeriveDRStrata (Rel.cpp:3093) already lifts the gate above the instance's
+    # drain stratum; V-READY then IS the per-key quiescence check.
```

So the per-key quiescence obligation discharges to the EXISTING stratum
machinery: `stratum(gate) > drain_stratum(force-complete activation)`, checked by
V-READY. No coordination protocol, no epoch-splitting. This is the CALM boundary
(coordination-free iff monotone): the LAZY edge needs no order; the FORCE_COMPLETE
edge needs exactly one ordering edge, which the linearizer already provides for
the acyclic case.

**ACROSS-SCC coupling — the barrier CANNOT exist.** If the negated activation is
in the SAME activation-SCC as the gate (recursion through the negation — the
globally-unstratified `evm_func_parse` class lifted to keys), there is no
topological order placing "complete for K" before "read absence(K)": the coupled
monotone fixpoint grows, so "K absent" flips to "K present" — the Stefan-Brass
push+negation stale-absence drift the owner names (`running-example`:256-268).
Irrevocability fails; no Seal precedes the read. At the flat level this is ALREADY
the existing unstratified-negation reject (Stratify), so DIFF-R4 introduces NO new
flat reject here.

---

### Interaction with DIFF-R2's activation-SCC (the genuinely-hard case)

A FORCE_COMPLETE edge inside an activation SCC is **rejected**, not scheduled —
the Variant B extension (D1.5/D1.6 RATIFIED: recursive demanded content stays an
expected-diagnostic reject) lifted from the relation grain to the KEY grain.

**Hunk R4-4 — activation-SCC classification (diff on DIFF-R2's DOWN/ACROSS split).**

```diff
   # activation-graph stratification (DIFF-R2): Tarjan SCC over the request graph
+  # DIFF-R4 edge-kind gate, applied AFTER the SCC condensation:
+    for each FORCE_COMPLETE request edge R (gate -> negated/summarized activation A):
+      if A is a strictly-lower activation-SCC (DOWN):  schedule (R4-3 stratum lift)
+      else (A in the gate's own activation-SCC, ACROSS):  REJECT
+        # Variant B, key-grained. At the FLAT level this coincides with the
+        # existing unstratified-negation Stratify reject; only the "globally
+        # unstratified -> locally stratified via key peeling" lift (running-
+        # example :42-64) could ever move a case DOWN, and that is Stage-D, NOT
+        # DIFF-R4 (STOP-R4-PEEL).
+  # LAZY edges impose no activation-graph ordering (CALM-monotone).
```

Key-peeling can DISSOLVE an apparent ACROSS into a DOWN when the negation reads an
EXTERNAL key space (a lower activation-SCC per activation), the disassembler's
`function ↔ sweep ↔ !function` local-stratification lift. That lift is Stage-D
(D2.12 / V-CW coupled fixpoint territory) and explicitly out of DIFF-R4's near
scope — DIFF-R4 admits only the syntactically-DOWN force-complete edge.

---

### SOUNDNESS OBLIGATIONS

1. **Completeness-before-gate (DOWN).** For a FORCE_COMPLETE edge, the negated/
   summarized activation must be SEALED for every requested key before the gate
   reads it. Discharged by `stratum(gate) > drain_stratum(activation)` + R4-3's
   in-walk instance placement, checked by V-READY (R.1.3). Obligation on the
   lander: prove the pulled-in-walk instance's Seal precedes the gate's stratum in
   the LITERAL region tree, not merely in the validated Kahn order (R.1.6 warns
   the two diverge for SUBGRAPHINSTANCE — R4-3 must close that divergence for the
   force-complete case).
2. **No partial-answer leak (LAZY unchanged).** A LAZY edge feeding a monotone
   consumer stays sound by irrevocability; R4 must not accidentally retype a
   monotone consumer as FORCE_COMPLETE (would over-serialize but stay sound) nor a
   negate/agg as LAZY (would leak stale absence — UNSOUND). The inference is a
   total node-kind partition, so mis-typing is structurally impossible.
3. **ACROSS is rejected, never scheduled.** A force-complete edge with no
   well-founded descent order must draw a clean diagnostic, never compile. At the
   flat level this is the existing Stratify reject; the belt R4-2 keeps the nested
   path LOUD.
4. **HP-4 stays LOUD off the admitted path.** R4-2's relaxation must fire abort for
   every MAP/KVIndex input and every NEGATE/AGG input NOT reached via an in-walk
   force-complete edge.
5. **Answer identity across lowerings.** flat `-demand` == `-demand-instance` ==
   (Stage-C declared) for every newly-admitted force-complete program — the I0/
   eqgate invariant, extended to the negate/agg-in-body family.

### OWNER DEPENDENCIES

- **D1.5/D1.6 (RATIFIED, Variant B).** The ACROSS reject IS Variant B key-grained;
  DIFF-R4 consumes it, adds no new owner call.
- **D3.5 (RATIFIED, PF-3 vocabulary).** DIFF-R4 lifts exactly the demand-SINK site;
  R-MAT and R-BODYWALK stay. Depends on the ratified split.
- **The §6-vs-§11 routing rule (OPEN; D1.5/D1.6 tail).** "What a bound query's
  request edge routes to" is still undefined for Stage-C authoring. A FORCE_COMPLETE
  edge's target (full-materialization vs nested activation of the negated
  relation) is exactly that routing question. **STOP-R4-ROUTE:** DIFF-R4 cannot
  fix the force-complete edge's realization until the §6-vs-§11 rule is defined.
- **D2.6 (DEFERRED — edge row schema).** If a per-key force-complete quiescence
  ever needs an individually-retractable lease (owner-bearing `RowReq{owner,key}`),
  the FORCE_COMPLETE edge needs D2.6's owner-bearing arm; a refcount suffices under
  R4-3's stratum-ordered discharge (no concurrent unsealed leases cross a stratum).
  **STOP-R4-SCHEMA:** the force-complete edge's runtime row schema pauses on D2.6.
- **D2.12 (RATIFIED — V-CW first).** The ACROSS/coupled case (key-peeling
  dissolution) rides V-CW's coupled-fixpoint restatement; DIFF-R4 explicitly
  DEFERS it (STOP-R4-PEEL is a Stage-D handoff, not a DIFF-R4 owner call).
- **STOP-R4-KV (new, owner call needed).** Whether to lift R-MAT so a KV `mutable()`
  demanded body is admissible is a SEPARATE decision from the monotonicity barrier;
  DIFF-R4 recommends leaving it rejected and recording it as a distinct lift-
  candidate.

### TEST / WITNESS plan (golden-master terms)

- **Stage 1 (now, zero code):** the existing negative corpus PINS the conservative
  barrier. `demand_agg_body_1`, `demand_config_agg_body_1` (demand-SINK) and a
  `demand_negate_body_1` (`!` in body) stay ALL-4-MODES-DIAGNOSTIC, refereed by
  message CLASS (PF-3: the "demand sink" string), encoded in `runall.sh`.
  `demand_kv_body_1` stays diagnostic on the R-MAT string (STOP-R4-KV). This is the
  covering-array negative floor.
- **Stage 2 (with DIFF-R1 landed) — FLIPS:** `demand_negate_body_1` and
  `demand_agg_body_1` FLIP diagnostic → golden when the negated/summarized relation
  is a strictly-DOWN activation (non-recursive); each gains `.batches` +
  `.oracle.stdout` (derivation-counter oracle over an epoch where the negated
  relation GROWS — the oracle is what proves no partial answer leaked to the gate)
  and an `.eqgate` sidecar (flat == nested == golden). These join the eqgate family.
- **New negative witness `demand_negate_cyclic_1`:** recursion-through-negation at
  one key (an ACROSS force-complete edge) — stays a clean diagnostic (the
  key-grained unstratified reject), all 4 modes, encoded in `runall.sh`. Pins that
  DIFF-R4 does NOT over-admit.
- **Referees:** I0/eqgate for answer identity; `bin/Oracle` for the completeness-
  before-gate soundness; the adornment-placement covering array (owner direction
  2026-08-03, `owner-adjudication-record.md`:463-487) enumerates bracket
  placements over these bodies and requires every one to compile-answer-identical
  or draw a PF-3-classed diagnostic — the evil-monkey rule applied to the barrier.
- **Belt fault-injection (`tests/…Validators`):** feed a NEGATE/AGG nested input
  with `edge_kind != FORCE_COMPLETE` or `!instance_in_walk` and assert R4-2 aborts;
  feed a MAP/KVIndex input and assert it still aborts.


### OPEN OWNER ITEMS (DIFF-R4)

- STOP-R4-ROUTE: the §6-vs-§11 request-edge routing rule (OPEN, D1.5/D1.6 tail) must be defined before the FORCE_COMPLETE edge's target (full-materialization vs nested activation) can be fixed
- STOP-R4-SCHEMA: the force-complete edge's runtime row schema pauses on D2.6 (owner-bearing RowReq vs refcount); R4-3's stratum-ordered discharge suggests a refcount suffices but the owner must confirm no concurrent unsealed cross-stratum leases
- STOP-R4-KV: whether to lift R-MAT (Demand.cpp:446-448) so a KV mutable() demanded body is admissible is a SEPARATE owner call from the monotonicity barrier; DIFF-R4 recommends leaving it rejected and recording it as a distinct lift-candidate
- STOP-R4-PEEL: the globally-unstratified -> locally-stratified key-peeling dissolution of an ACROSS force-complete edge (disassembler function<->sweep<->!function) is Stage-D / V-CW (D2.12) territory, explicitly deferred out of DIFF-R4

### PANEL RECORD (DIFF-R4) — 17 findings, 17 survived refutation

- **DIFF-R4-correctness-lifecycle-1** [CONFIRMED; sev MAJOR] R4-3's load-bearing soundness argument — that completeness-before-gate 'discharges to the EXISTING stratum machinery' via `stratum(gate) > drain_stratum(activation)` 'checked by V-READY', with 'no coordination protocol, no epoch-splitting' — is invalid for the nested (`-demand-instance`) case, because the validated DR-op stratum of a kSubgraphInstantiate is NOT its literal codegen placement.
  - EVIDENCE: regional-arch-pseudocode.md:1491-1504 (BAND-KEY ASYMMETRY) states outright that the Kahn key/DR-op stratum is 'used ONLY for dep-edge derivation / V-READY / dump order — never read by codegen' and that for kSubgraphInstantiate 'the LITERAL codegen placement is fixed unconditionally at the tail by LowerSubgraphInstances regardless… A reader who assumes DR-op stratum == where it executes… would be misled.' Procedure.cpp:588 confirms LowerSubgraphInstances runs once inside PublishDifferentialMessageVectors AFTER BuildStratumPhases' whole ascending loop. So `stratum(gate) > drain_stratum(activation)` passing V-READY is necessary-but-not-sufficient for Seal-before-gate. The diff's own Soundness Obligation 1 concedes this exact divergence, yet the R4-3 operative hunk asserts 'V-READY then IS the per-key quiescence check' — an internal contradiction.
  - AMENDMENT (normative): Reconcile R4-3 with Soundness Obligation 1: replace 'V-READY then IS the per-key quiescence check' with the concrete codegen change — relocate EmitSubgraphInstance emission for force-complete sids OUT of the Procedure.cpp:588 tail into the per-stratum walk at the activation stratum (the literal GROUP_UPDATE band-0 precedent, not merely its validated-stratum analogue), and add an always-on validator (V-FORCE-SEAL-BEFORE-GATE) checking the LITERAL region-tree position of the instance Seal precedes the gate's stratum body. State V-READY is necessary but not sufficient here.
- **DIFF-R4-correctness-lifecycle-2** [CONFIRMED; sev MAJOR] R4-3 relocates only 'Instantiate+Seal' and is entirely silent on kInstanceDeath, which is minted for the SAME force-complete instance under `-demand-retract` and lives in the SAME unreorderable region — leaving the retraction lifecycle of a relocated instance undefined.
  - EVIDENCE: Rel.cpp:1163 mints kInstanceDeath only `if (demand_table && TableIsDifferential(demand_table))` (== -demand-retract). regional-arch-pseudocode.md:1421 labels the whole EmitSubgraphInstance body 'ONE SUBGRAPHINSTANCE region per store; band order UNREORDERABLE', with band a0 DEATH/RecycleCurrent as the FIRST band, through band-b publish, through the self-lowered Seal — one atomic region. R4-3's 'pull Instantiate+Seal into its activation stratum' therefore either splits an unreorderable region or silently drags band-a0 kInstanceDeath along with unstated ordering relative to other strata and pub's tail COMMITSWEEP (Procedure.cpp:589). The Stage-2 test plan exercises exactly this differential (-demand-retract) regime.
  - AMENDMENT (normative): State that the ENTIRE EmitSubgraphInstance region (band a0 kInstanceDeath → a1/a2/a2' → band b → Seal) relocates as an atomic unit, and specify where band-a0 retraction lands relative to (a) the consuming gate's stratum and (b) pub's epoch-tail COMMITSWEEP that drains the band-b `touched` set. Add the kInstanceDeath case to R4-2's belt relaxation and to Soundness Obligation 1.
- **DIFF-R4-correctness-lifecycle-3** [CONFIRMED; sev MAJOR] R4-2 relaxes the HP-4 recognizer-refusal belt on a SCHEDULING predicate (`edge_kind==FORCE_COMPLETE && instance_in_walk`), but the belt's actual charter is a SEMANTIC coverage obligation (V-ALPHA / α-through-functor) that the diff never discharges — admitting a NEGATE/AGG `input_view` into an InstantiateEffects rescan mold that assumes a plain-table row lifecycle.
  - EVIDENCE: The live belt comment (Rel.cpp:1063-1066) names its charter explicitly: 'the re-resolved input side must be a plain table-bearing view… this trips LOUD if a D3 body relaxation reopens the α-through-functor path before V-ALPHA covers it' — a V-ALPHA coverage guard, not a scheduling guard. R4-2 substitutes a scheduling predicate (edge_kind==FORCE_COMPLETE && instance_in_walk), guarding the wrong axis. Notably Hunk R4-1 itself targets 'REQUEST(neg_input or agg_input…)' — the PLAIN input relation — so admitting a NEGATE/AGG `input_view` into the InstantiateEffects rescan mold is unnecessary and inconsistent with R4-1's own target.
  - AMENDMENT (normative): Keep the HP-4 belt LOUD for NEGATE/AGG inputs and route DIFF-R4's admitted case so `ri.input_view` stays a plain table-bearing view — force-complete the plain neg_input/agg_input relation (as R4-1 already specifies), never letting the negate/agg view itself become the instance input. If a negate/agg input_view must ever be admitted, gate the relaxation additionally on V-ALPHA coverage (the belt's own precondition) and specify InstantiateEffects' rescan semantics over absence/summary rows.
- **DIFF-R4-correctness-lifecycle-4** [SOFTENED; sev MAJOR] R4-2 and R4-3 hard-commit to the NESTED-activation realization of the force-complete target (belt relaxation inside BuildSubgraphInstanceOps; kSubgraphInstantiate/Seal relocation), while STOP-R4-ROUTE declares that same realization (full-materialization vs nested activation) UNDEFINED pending the §6-vs-§11 rule — so two hunks build machinery the diff elsewhere blocks on.
  - EVIDENCE: R4-2 (Rel.cpp:1062-1070 belt, the nested BuildSubgraphInstanceOps path) and R4-3 (relocate kSubgraphInstantiate/Seal) do presuppose the §11 nested-activation realization, while STOP-R4-ROUTE declares that realization undefined pending §6-vs-§11. This is a real over-commitment. But the finding overshoots in calling the hunks unbuildable/premature: the diff already stages ALL of R4-2/R4-3 behind 'Stage 2 (DIFF-R1 landed)' and flags STOP-R4-ROUTE — they read as the nested-arm spec, not 'now' content. Right size is an explicit contingency label at the hunk sites, not demotion/removal.
  - AMENDMENT (normative): Annotate R4-2 and R4-3 as CONTINGENT on §6-vs-§11 resolving to nested-activation realization (STOP-R4-ROUTE), and note that if routing resolves to flat full-materialization there is no instance and both hunks are moot (the discharge becomes an ordinary lower stratum). Keep R4-1's Demand.cpp reject-lift + the edge_kind inference as the unconditional Stage-2 content.
- **DIFF-R4-termination-confluence-1** [CONFIRMED; sev BLOCKING] R4-3 discharges the per-key quiescence obligation to V-READY ("V-READY then IS the per-key quiescence check. No coordination protocol, no epoch-splitting"), but V-READY validates the Kahn dep-edge order, in which — per R.1.6's BAND-KEY ASYMMETRY note — SUBGRAPHINSTANCE's Seal is NOT where it executes: LowerSubgraphInstances emits every instance Instantiate+Seal unconditionally at the epoch TAIL (Procedure.cpp:588, after the whole ascending stratum loop). A mid-stratum FORCE_COMPLETE gate therefore reads an UNSEALED (empty frozen) instance and interprets absence(K) as 'all absent' — the exact stale-absence leak Obligation 2 forbids. The operative pseudocode asserts the discharge is complete while the diff's own Obligation 1 lists closing this divergence as an unmet lander task; the two contradict.
  - EVIDENCE: Same load-bearing fact as lifecycle-1, verified: Procedure.cpp:585-589 emits LowerSubgraphInstances at the epoch tail after the whole ascending stratum loop; regional-arch-pseudocode.md:1491-1504 documents that the Kahn/V-READY stratum for kSubgraphInstantiate 'never read by codegen' and its literal placement 'fixed unconditionally at the tail… regardless.' So `stratum(gate) > drain_stratum(activation)` passing V-READY does NOT imply the literal Seal precedes the gate — the diff's Obligation 1 concedes this while the R4-3 hunk asserts the discharge is complete ('No coordination protocol, no epoch-splitting'). As written the nested-case discharge is unsound.
  - AMENDMENT (normative): Replace 'V-READY then IS the per-key quiescence check' with a two-part requirement: (a) split LowerSubgraphInstances so a FORCE_COMPLETE-consumed instance's Instantiate+rescan+Seal emits from INSIDE the per-stratum loop at its activation stratum (GROUP_UPDATE band-0 precedent), not the Procedure.cpp:588 tail; (b) add an always-on V-FORCE-SEAL-BEFORE-GATE validator on the LITERAL region-tree position. State V-READY alone cannot be the quiescence referee for the force-complete case.
- **DIFF-R4-termination-confluence-2** [CONFIRMED; sev MAJOR] The diff gives NO finiteness argument for the mint-time cross-relation SIP-walk continuation it introduces (Hunk R4-1: 'CONTINUE the SIP walk INTO that input'). Every stated finiteness argument covers SCHEDULING (stratum lift / V-READY), never the walk that BUILDS the request graph. The lens's requested 'finite-activation argument' is therefore absent, and the R4-4 ACROSS reject cannot supply it: R4-4 runs 'AFTER the SCC condensation', i.e. after the request graph is fully built, so it cannot bound a walk that loops while building that graph around a recursion-through-negation cycle (p :- q, !r ; r :- s, !p).
  - EVIDENCE: Demand.cpp:623 declares `std::unordered_set<VIEW *> seen;` INSIDE the per-member block of the `for (VIEW *member_v : p_merge->merged_views)` loop — it terminates the walk of one body tree only and does not span relations. Today the demand-SINK reject at :648-651 stops the walk at the relation boundary. R4-1 lifts exactly that reject and 'CONTINUE the SIP walk INTO that input' (a different relation's defining body) with no visited-set spanning the cross-relation regress and no stated finiteness bound. R4-4 runs 'AFTER the SCC condensation' (post graph-build), so it cannot bound a walk that loops WHILE building the request graph around a recursion-through-negation cycle. The diff supplies finiteness arguments only for scheduling, never for the graph-building walk.
  - AMENDMENT (normative): Add an explicit finite-activation argument to R4-1: memoize REQUEST nodes by (target predicate, adornment/key) so a continuation into an already-requested predicate reuses the node and stops (bounding by the finite predicate×adornment product), and run the continuation under a request-graph-GLOBAL visited set (not the per-member :623 `seen`) so a recursion-through-negation cycle is caught at MINT time and drawn as the R4-4/Stratify reject BEFORE Tarjan runs on the finished graph.
- **DIFF-R4-termination-confluence-3** [SOFTENED; sev MAJOR] R4-3 and R4-4 use TWO different stratification notions and assume they agree on DOWN/ACROSS without proof. R4-4 classifies DOWN via activation-SCC over the REQUEST graph (a coarsening keyed on predicates), while R4-3's quiescence is discharged to DeriveDRStrata/V-READY at DR-flow TABLE grain. A request-graph 'DOWN' edge whose gate and activation land in the same DR-flow SCC gets NO ordering lift — ready_across silently returns 0 — so DeriveDRStrata performs no lift, V-READY passes vacuously, and the gate reads an unordered (stale-absence) instance while R4-4 believes it scheduled a well-founded descent.
  - EVIDENCE: The two-notion observation is factually real: R4-4 classifies DOWN on the request/activation graph while R4-3 discharges via DeriveDRStrata's table-grain same_scc (Rel.cpp:3110, keyed on relation-level Stratify SccOf) with ready_across (Rel.cpp:3172-3173) returning 0 for same-SCC reads. BUT the feared silent under-ordering cannot bite the admitted node kinds: for NEGATE/AGG consumers (the only FORCE_COMPLETE kinds), relation-level Stratify already guarantees strict-above — Stratify.cpp:272-297 rejects negate when `negate->stratum == negated_view->stratum`, and Rel.cpp:3155-3158 records the same for the aggregate. Different strata ⇒ different SCC ⇒ !same_scc at table grain ⇒ ready_across = ready_after ≠ 0. So the ready_across:3172 zero-lift path structurally cannot carry a force-complete negate/agg gate. The MAJOR severity (implying live unsoundness) overshoots.
  - AMENDMENT (normative): Downgrade to a documentation/defense-in-depth item: note that R4-4's request-graph DOWN and R4-3's table-grain ordering agree for negate/agg gates BECAUSE Stratify's strict-above (Stratify.cpp:272-297 / Rel.cpp:3155-3158) forces !same_scc, and add an always-on assertion that every FORCE_COMPLETE DOWN edge satisfies `!same_scc(gate_table, activation_table)` before R4-3 relies on ready_after — a belt, not a live-bug fix.
- **DIFF-R4-testability-oracle-1** [CONFIRMED; sev MAJOR] The Stage-1 'negative floor' referee is misdescribed: DIFF-R4 says the diagnostic cases are 'refereed by message CLASS (PF-3: the "demand sink" string), encoded in runall.sh', but runall.sh's expect_diagnostic only checks the compiler exit code, never the diagnostic text — so the referee cannot discriminate the demand-SINK reject from R-MAT or R-BODYWALK.
  - EVIDENCE: runall.sh expect_diagnostic (verified at the function): it runs the compiler, redirects stderr to dr.log, and returns pass iff `rc -ne 1` fails — `if [ $rc -ne 1 ]; then … return 1; fi; return 0`. It NEVER greps dr.log for any string. All three DIFF-R4 rejects (demand-SINK :648-651, the KV :548 trace, R-BODYWALK :653) exit 1 with distinct messages and are indistinguishable to the referee. The diff's 'refereed by message CLASS (PF-3: the demand sink string), encoded in runall.sh' describes a discriminator that does not exist (only permcheck.py exists, permutation-only).
  - AMENDMENT (normative): Extend expect_diagnostic to accept an optional expected-substring per diagnostic case (e.g. a `<name>.diag` sidecar) and `grep -qF` it against dr.log, failing on mismatch. Pin demand_agg_body_1/demand_config_agg_body_1/demand_negate_body_1 to the 'demand sink' string and demand_kv_body_1 to its actual message. Only then can Stage 1 'pin the demand-SINK barrier by class' and can Stage-2 flips be guarded against a same-exit-code reject-site regression.
- **DIFF-R4-testability-oracle-2** [CONFIRMED; sev MAJOR] demand_negate_body_1 is presented as part of the 'existing negative corpus' pinning the conservative barrier at 'Stage 1 (now, zero code)', but no such case exists — the NEGATE arm of the demand-sink (the CENTRAL DIFF-R4 target, `p(X) :- q(X), !r(X)`) is entirely UNWITNESSED today.
  - EVIDENCE: `ls tests/OptDiff/cases` shows only demand_agg_body_1, demand_config_agg_body_1, demand_kv_body_1 — no demand_negate_body_1. runall.sh's all-4-modes-diagnostic list contains `demand_agg_body_1|demand_kv_body_1|demand_config_agg_body_1` but no negate-body case. Demand.cpp:648-651 rejects NEGATE and AGG with the same string, but only the AGG arm is exercised. So Stage 1 is NOT zero-code for DIFF-R4's central NEGATE demand-sink target.
  - AMENDMENT (normative): Reclassify demand_negate_body_1 as a NEW Stage-1 case to author NOW (not 'existing'), add it to the runall.sh all-4-modes-diagnostic list, and include it in the Stage-1 acceptance criterion — pinning the negate demand-sink reject before any lift so the Stage-2 negate flip has a real before/after baseline.
- **DIFF-R4-testability-oracle-3** [CONFIRMED; sev MAJOR] The negative witness demand_negate_cyclic_1 (the ACROSS force-complete reject) is non-discriminating: a genuine recursion-through-negation program rejects at the pre-existing global Stratify pass BEFORE any DIFF-R4 activation-SCC logic (R4-4) runs, so it cannot pin 'DIFF-R4 does NOT over-admit'.
  - EVIDENCE: Unstratified (recursion-through-negation) programs are rejected by the dataflow Stratify pass in all 4 modes regardless of -demand (Stratify.cpp:272-297, 'Negated predicate is recursively derived from the negation's own result'); the diff itself concedes 'At the FLAT level this coincides with the existing unstratified-negation Stratify reject.' R4-4's DOWN/ACROSS classification is new code reached only AFTER the demand-sink is lifted, so a genuinely cyclic case is pre-empted by Stratify (or the pre-existing sink) upstream and never reaches R4-4. The only case that is ACROSS-at-key-grain yet passes global Stratify is the key-peeling shape DIFF-R4 explicitly defers (STOP-R4-PEEL) — the unconstructible-witness trap.
  - AMENDMENT (normative): Either (a) relabel demand_negate_cyclic_1 honestly as a pin of the pre-existing Stratify/demand-sink reject (drop the 'pins DIFF-R4 does not over-admit' claim), or (b) defer the ACROSS negative witness to Stage-D alongside key-peeling and add a validators-level death test that hand-builds an ACROSS FORCE_COMPLETE edge and asserts R4-4 rejects it — the only construction that reaches the new classification code without Stratify pre-empting it.
- **DIFF-R4-testability-oracle-4** [CONFIRMED; sev MAJOR] DIFF-R4 cites the 'adornment-placement covering array' as an active referee ('enumerates bracket placements over these bodies and requires every one to compile-answer-identical or draw a PF-3-classed diagnostic'), but that harness does not exist and is scoped to DIFF-R3, not DIFF-R4.
  - EVIDENCE: owner-adjudication-record.md:463-487 is an owner DIRECTION dated 2026-08-03 describing an unbuilt 'placement-enumeration harness' that merely 'slots beside the covering-array machinery' — aspirational, and explicitly scoped to feed DIFF-R3's formalization (the declared-bracket/adornment surface). A grep for a covering-array harness under tests/ finds only FINDINGS.md (a doc mention), no harness; only permcheck.py exists (permutation-only). DIFF-R4's negate/agg demand bodies have no necessary bracket-placement axis, so even the aspirational harness would not enumerate over them. The cited harness also presumes the diagnostic-class referee that oracle-1 shows does not exist.
  - AMENDMENT (normative): Remove the adornment-placement covering array from DIFF-R4's referee list, or gate the evil-monkey coverage claim behind an explicit dependency ('requires the DIFF-R3 placement-enumeration harness (unbuilt) plus a diagnostic-class referee, oracle-1'). For DIFF-R4-now, name only referees that exist and reach negate/agg demand bodies: eqgate, byte-golden, bin/Oracle, and the R4-2 validators death test.
- **DIFF-R4-testability-oracle-5** [SOFTENED; sev MAJOR] The claim that 'bin/Oracle [proves] the completeness-before-gate soundness' / 'the oracle is what proves no partial answer leaked to the gate' over-assigns the referee: the oracle is invoked WITHOUT the demand flags and compared only to its own oracle-golden, so it cannot directly witness a demand-restricted stale-absence leak.
  - EVIDENCE: runall.sh invokes the oracle as `"$ORACLE" "$DRC" "$batches"` (verified in run_oracle) — no flags_of, no .drflags — so bin/Oracle recomputes the FULL definitional relation, blind to the demand transform, and its stdout is cmp'd against goldens/<name>.oracle.stdout, not against the demand-compiled case output. So the diff's 'the oracle is what proves no partial answer leaked to the gate' overstates it as a direct per-run discriminator. BUT the oracle is not idle for soundness: it is the independent definitional recompute the case golden must match at bless time, plus the monotone/F16 projection gate — so it does anchor leak-proofing via the golden-master discipline. Severity/attribution overshoots, not the mechanism.
  - AMENDMENT (normative): Restate Obligation-1's referee as: the case BYTE-GOLDEN (all 4 modes) plus eqgate is the per-run leak-catching referee; bin/Oracle is the independent definitional recompute the golden MUST be cross-checked against AT BLESS TIME (a bless-review gate), not an automatic per-run cross-check. If a per-run automatic cross-check is wanted, add a demand-projected oracle run (oracle taught the -demand restriction) byte-compared to the demand-compiled query output.
- **DIFF-R4-necessity-1** [SOFTENED; sev MAJOR] `edge_kind` should not be a stored field "folded into the request edge's structural identity" — it is a pure total function of the consumer node kind and must be recomputed, not persisted. Persisting it is the very F1 two-authorities disease the diff invokes to justify persisting it.
  - EVIDENCE: The diff's stated intent — 'no satellite annotation for a later pass to preserve (the F1 lesson)' — IS the recompute intent the finding advocates, so on goal they agree; reading 'folded into structural identity' strictly as 'persisted second authority' is partly a strawman (it can mean recomputed-as-part-of-identity, like cmp=/functor= re-derive from eager_view per CLAUDE.md). The valid residue is factual: at the flat level there is NO reified request-edge object — regional-arch-pseudocode.md:941-943 is explicit: 'the demand relation is JUST ANOTHER RELATION; the region call is an IMPLICIT guard-JOIN; the key stays a COLUMN. No explicit call/region node.' So 'structural identity of the request edge' presupposes an object the flat lowering lacks.
  - AMENDMENT (normative): Drop 'folded into the request edge's structural identity' as the flat framing. Define edge_kind as a query-time helper `EdgeKind(consumer) := (consumer.IsNegate()||consumer.IsAggregate()) ? FORCE_COMPLETE : LAZY` recomputed at each consult site (belt R4-2, gate R4-4) from the live consumer view — no persisted field — matching the repo's cmp=/functor= re-derive precedent and the flat 'no explicit node' reality.
- **DIFF-R4-necessity-2** [CONFIRMED; sev MAJOR] The headline shrink "No new quiescence-detection mechanism is needed" over-claims: it holds for the flat/acyclic case but is FALSE for the `-demand-instance` nested case, which provably needs a NEW emission-scheduling mechanism beyond V-READY/DeriveDRStrata. R4-3's "V-READY then IS the per-key quiescence check" is insufficient there.
  - EVIDENCE: Shrink #1 ('No new quiescence-detection mechanism is needed… No scheduling change, no new runtime primitive') is true for flat/acyclic (the negated relation is an ordinary lower stratum) but false for -demand-instance: Procedure.cpp:585-589 emits LowerSubgraphInstances at the epoch tail after all strata, and regional-arch-pseudocode.md:1409/1491-1504 states 'GROUP_UPDATE stratified INSIDE the per-stratum walk; SUBGRAPHINSTANCE is NOT' and that its literal placement is 'fixed unconditionally at the tail… regardless.' Closing the tail-Seal inversion (which the diff itself calls 'the exact reason nested lowering rejects negate/agg today') requires MOVING SUBGRAPHINSTANCE emission out of the epoch tail — a real emission-order change, contradicting shrink #1.
  - AMENDMENT (normative): Split shrink #1: keep 'no new mechanism' ONLY for flat -demand / acyclic DAG. For -demand-instance state plainly that R4-3 requires a NEW emission-order mechanism (pull SUBGRAPHINSTANCE Instantiate+Seal out of the Procedure.cpp:588 epoch tail into the activation stratum) plus a literal-region-tree ordering check; V-READY alone is not the discharge. Do not present the nested barrier as 'free.'
- **DIFF-R4-necessity-3** [CONFIRMED; sev MAJOR] The PF-3 table's KV/R-MAT anchor is wrong on both site and mechanism, which mis-pins the test referee and mis-justifies STOP-R4-KV's "orthogonal to monotonicity" claim.
  - EVIDENCE: The diff pins KV to 'R-MAT (≠1 materialization) | Demand.cpp:446-448'. Demand.cpp:446-448 is `q_rel->inserts.Size() != 1u` → 'A demanded query must have exactly one materialization' — a count of the QUERY relation's own INSERTs, which a KV body does not trigger. Compiling demand_kv_body_1 actually rejects with 'The demanded query must project from a single derived relation under -demand' — Demand.cpp:548, inside Step 2's back-trace, a different site and message (the case's own .dr comment even calls this the diagnostic). And KVINDEX is not tested in the demand-sink walk (:648-651 tests AsNegate()||AsAggregate() only), so a KV body reaching the walk would fall to the :653 else, the same walk DIFF-R4 lifts. The diff's timing/orthogonality conclusion (KV rejects before the body walk; STOP-R4-KV) survives, since :548 does precede the :610 walk — only the anchor/mechanism is wrong.
  - AMENDMENT (normative): Correct the KV anchor to Demand.cpp:548 ('single derived relation'), note the KVINDEX :653 else-branch in the lifted walk, and restate STOP-R4-KV as: KV admission additionally requires (a) a KVINDEX arm in the lifted sink walk and (b) relaxing the :548 single-derived-relation trace — NOT a distinct :446 materialization-count lift. Update the Stage-1 referee to pin the :548 message class (not an 'R-MAT string') for demand_kv_body_1.
- **DIFF-R4-necessity-4** [SOFTENED; sev MAJOR] By the diff's own STOP-R4-ROUTE (§6-vs-§11 undefined) and STOP-R4-SCHEMA (D2.6 deferred), the realized-lowering hunks R4-2 (belt relaxation) and R4-3 (in-walk stratum lift) cannot be built and are premature; only the edge_kind partition and the Stage-1 zero-code pins are non-deferrable.
  - EVIDENCE: Same substance as lifecycle-4: R4-2/R4-3 are written against the §11 nested realization while STOP-R4-ROUTE declares that realization undefined — a real over-commitment worth flagging. But 'cannot be built / premature' overshoots: the diff already stages R4-2/R4-3 behind 'Stage 2 (DIFF-R1 landed)' and gates realization via STOP-R4-ROUTE, so they function as the nested-arm spec rather than 'now' deliverables. The non-deferrable core (edge_kind partition + Stage-1 zero-code pins) is indeed what should land first.
  - AMENDMENT (normative): Label R4-2 and R4-3 'provisional, contingent on §6-vs-§11 → nested-activation (STOP-R4-ROUTE)'. Land now only: (1) the edge_kind consumer-kind partition as a recompute helper (per necessity-1); (2) the Stage-1 negative pins (existing conservative reject, zero code, with the oracle-1 diagnostic-class referee and the oracle-2 negate case). Hold R4-2/R4-3 and the Stage-2 flips until routing is decided.
- **DIFF-R4-necessity-5** [SOFTENED; sev MINOR] R4-3 is over-scoped as a general SUBGRAPHINSTANCE change: it is a strict no-op for flat `-demand` and for EVERY aggregate force-complete consumer, biting only the narrow `-demand-instance` × nested-negate-DOWN case.
  - EVIDENCE: The narrowing is directionally right: the diff establishes flat -demand DOWN needs 'No scheduling change' (ordinary lower stratum), and GROUP_UPDATE (agg/KV) is already emitted band-0 in-walk at its lifted stratum (regional-arch-pseudocode.md:1408 op_band(kGroupUpdate)=0u; Rel.cpp:3155-3158), so those paths discharge for free. But 'strict no-op for EVERY aggregate force-complete consumer' is not fully verifiable — an aggregate consumer whose summarized input is itself a nested -demand-instance store is not obviously covered by the band-0 GROUP_UPDATE path — so the MAJOR-flavored certainty overshoots; the useful content is the scope narrowing.
  - AMENDMENT (normative): Scope R4-3 explicitly to -demand-instance × nested force-complete DOWN, stating flat -demand (any consumer) and agg consumers whose input is an ordinary GROUP_UPDATE view discharge with zero R4-3 change (existing strata / band-0 GROUP_UPDATE). Flag the nested-store-summarized-input agg sub-case as needing separate confirmation rather than asserting a blanket no-op.

---

## DIFF-R5 — Non-prefix ordered bracket keys; the logical/physical key split (formalized 2026-08-03)

Formalizes the seed's Part-2 DIFF-R5 sketch against **Part R** (the fleet-verified
current-architecture pseudocode). Source direction: owner addendum *"brackets bind
arbitrary (non-prefix, ordered, multi-column) keys → the logical/physical key split"*
(owner-adjudication-record.md:435-461) and its follow-on *"adornments as a FUZZING
axis + the bracket parser obligations"* (:463-488). Both are RATIFIED as DIRECTION,
not stage items.

### The thesis, one line

A bracket `rel[c₀,c₁,…](Free…)` carries **two** decoupled things the flat positional
adornment conflates:

- **LOGICAL key** = the *set* `{c₀,c₁,…}` = the region PARTITION = semantic. `rel[A,C]`
  and `rel[C,A]` are the **same region**. This is *already landed* as the Stage-A
  contract `member_key` (D3.4 candidate 3, flat-key).
- **PHYSICAL order** = the *sequence* `(c₀,c₁,…)` = the arrangement / trie key order =
  optimization. `rel[A,C]` and `rel[C,A]` are **different arrangements**; two regions
  share a trie iff their orders are **prefix-compatible**. This is a D5/planning-tier
  cost knob with **no near-term producer**.

### Complexity verdict — the seed OVER-specifies; DIFF-R5 shrinks

The seed lists "join pivots, nested keys… separate the semantic key from the
arrangement order" as if both halves need new IR. They do not. The measured state:

1. The **logical half is already built.** `RowContract.member_key` is a
   `SemanticMemberKey = std::vector<FieldId>` (Identity.h:52) built by iterating
   `VisibleCols(v)` and keeping the value-id set (`MemberKeyFromIds`,
   RowContract.cpp:125-135); the JOIN arm is literally `key = union of mapped keys`
   over an `unordered_set<unsigned>` (RowContract.cpp:200-215/246). It is **order-free
   by construction** and rendered in one canonical (visible-column) order per view. So
   `rel[A,C] == rel[C,A]` at the contract layer holds today with zero new code.
2. The **physical half has no consumer until D5.** There is no arrangement planner, no
   trie-sharing pass, no cost-ranked order choice in the tree yet (DIFF-R6/D5). Building
   a "physical order" structure now would be a satellite nobody reads.

So DIFF-R5's own near-term IR delta is TINY and almost entirely a set of *definitions +
one dedup key*, not new machinery. The non-prefix / multi-column / decoupled-from-arg-
order EXPRESSIVENESS is a DIFF-R3 *parser-surface* concern (the bracket spelling), not a
DIFF-R5 IR change. DIFF-R5 contributes: (a) name the logical key = the landed
`member_key`; (b) reserve the physical order as a *declared-inert planning annotation*
(the H-A1 `DemandSupportCount` pattern — reserve the domain, no producer); (c) the ONE
behavioural obligation — region/store equivalence must key on the canonical logical SET,
not the ordered tuple.

### Where each lives in the IR — and why physical order stays OUT of identity

| Aspect | IR home | Identity role |
| --- | --- | --- |
| Logical key (set) | `RowContractMap` `member_key` (RowContract.h:39-46), keyed by `QueryViewImpl*` (O-A2) | Region PARTITION; input to region equivalence / CSE. A **pure recomputable function** of the final graph (RowContract.h:14-18) — never a satellite. |
| Physical order (seq) | **Reserved** D5 planning annotation (no near-term field); the *incidental* current order is `bound_indices` "decl order" (Demand.cpp:483-486) → `ProgramInstanceStoreInfo::key_types` (Program.h:1972) → the emitted `Key_<id>{c0,c1,…}` struct (Database.cpp:1259-1267) | Arrangement/trie order; a COST knob. **Must not enter any structural-identity surface.** |

**Argument against storing physical order in structural identity — the role-in-Equals-
only lesson, restated.** Stage A already litigated this exact shape: `ProjectionRole`
folds into `QueryTupleImpl::Equals` ONLY, deliberately NOT `Hash` (Part R R.1.4;
Tuple.cpp:308-310; H-A2 rationale), because a discriminant that DOES belong to semantic
identity but is order-*sensitive* perturbs hash-derived tie-breaks (the demand_tc
join-emit seq/order swap) for zero benefit. Physical order is the *stronger* case: it is
not even semantic. Folding trie order into the logical key (or into CSE-color / Hash /
`member_key`) would make `rel[A,C]` and `rel[C,A]` fail to CSE — fragmenting arrangement
sharing and flipping goldens — while computing **byte-identical answers**. That is the
F1-class mistake (a satellite the optimizer must be taught to preserve) that the whole
Stage-A contract layer was built to forbid (RowContract.h:14-18). Physical order is a
thing the *planner chooses and is free to rewrite*, never a node discriminant.

### The InstanceStore key = the bracket tuple (verified against Part R)

Part R R.1.2 (corrected via D7/D8) fixes the store model: `InstanceStore<Key,RowT>`
(InstanceStore.h:62-63), `Key` a plain hash struct with `Hash()`/`==`, `FindOrAddInstance(key)`
(:109) minting a dense `iid`, `KeyAt(iid) -> const Key&` (:157). Codegen emits
`Key_<id>{c0,c1,…}` from `store.KeyTypes()` in order (Database.cpp:1259-1267). So:

- The **runtime Key struct's field ORDER is the chosen physical (arrangement) order** —
  today an accident of `bound_indices` decl order, tomorrow a D5 choice.
- The **memo PARTITION is the field SET** — which rows land in which `iid`.
- Therefore two stores with the *same field set, different field order* are **one region
  logically** (they memoize the same partition). Whether the runtime keeps *both*
  arrangements over one pub or canonicalizes to one is the D2.6 reader-handle question
  (STOP-R5-B).

### The contract-check obligation (the DIFF-R3 declared-bracket gate)

A DECLARED bracket (DIFF-R3) must be a REAL functional key: its set `K` must
functionally determine the visible row. At Stage-A precision this is decidable *upward
only* — the contract proves an over-approximation `member_key(v)` that is *a* key, and
any superset of a key is a key. So the check is exactly:

    CheckDeclaredBracketIsKey(v, K):        # K = declared bracket SET (order dropped)
      if member_key(v) ⊆ K:  accept          # K is provably a key (superset of a proven key)
      else:                  REJECT | AllFields-widen     # STOP-R5-A

If `member_key(v) == AllFields(v)` (Stage A could not prove anything smaller — Phase-1
cyclic rule, RowContract.cpp Phase 1; or the SELECT/MERGE/MAP/KVINDEX AllFields fallback
:232-244), then any bracket smaller than the full row is **unprovable-as-a-key** at Stage-A
precision. Two responses:

- **REJECT** (recommended): a clean, all-4-modes diagnostic — consistent with the
  evil-monkey never-miscompile rule. Record a **LIFT-candidate**: Stage-B `Minimize`
  (deferred candidate 3) can prove smaller keys, so a bracket rejected here may become
  admissible there.
- **AllFields-widen** (drop the hint): answer-identical (a *finer* partition is always a
  sound key; over-partitioning changes only sharing, never answers — obligation 4), but
  it silently defeats the user's declared partition/arrangement intent, which the
  answer-identity referee **cannot catch**. Because the failure is invisible to the
  eqgate, this is a genuine policy call, not a mechanical one → **STOP-R5-A**, tied to the
  declared-regions hint-vs-mandate owner-call.

The check operates on the **logical set** (`K` with order dropped); the bracket's physical
order is irrelevant to key-validity — `rel[A,C]` and `rel[C,A]` accept-or-reject identically.

### Interaction with DIFF-R3 and the D3.a.3 sharing precedent

- **DIFF-R3 (declared brackets)** owns the surface. The bracket *column list* is parsed as
  an ordered, NAMED, DUP-FREE sequence; DIFF-R5 reads the SET off it (logical key, checked)
  and the ORDER off it (physical hint, D5). The owner fuzzing addendum's parser rejects —
  `foo[_]`/`foo[_A]` (unnamed/anonymous) and `foo[A,A]` (repetition) — are exactly the
  well-formedness that keeps the split well-defined: dup-free ⟺ a valid SET; ordered ⟺ a
  valid SEQUENCE (obligation 6). These are DIFF-R3 parser obligations DIFF-R5 depends on.
- **D3.a.3 multi-adornment (disjoint stores over one pub)** is the precedent AND the near-
  term hazard. Today N adornments of one name → N disjoint stores over one pub, disambiguated
  by `forcing_index`; V-INST-SOLE is keyed `(pub_table, forcing_index)` (Rel.cpp:4970-4987;
  Part R R.1.5). That is correct precisely because today's N adornments have N *different*
  logical SETS. Once brackets decouple order from arg-order, two brackets can name the SAME
  set in different orders — and the current forcing-index-keyed dedup would wrongly mint
  **two stores for one region** (double-materialization). DIFF-R5's obligation 2 fixes this:
  the store/region equivalence key must be the **canonical logical set**, with `forcing_index`
  demoted to the pub-sharing distinguisher only across *distinct* logical keys.

### Pseudocode-diff hunks (against Part R)

**Hunk R5-1 — R.1.1, split the bound-index locate into logical set + physical order.**
Modifies Part R line 845 (Demand.cpp:483-486, "Bound param indices, decl order"):

```diff
-    bound_indices := [param.Index() for param in redecl.Parameters() if Binding()==kBound]
+    bound_indices  := [param.Index() for param in redecl.Parameters() if Binding()==kBound]
+    logical_key    := set(bound_indices)     # the region PARTITION (order-free); becomes member_key
+    physical_order := seq(bound_indices)      # DEFAULT arrangement order = param decl order.
+                                              #   A DIFF-R3 declared bracket OVERRIDES this order.
+                                              #   A PLANNING hint (D5), NEVER a node/region identity.
```

**Hunk R5-2 — R.1.5, re-key store/region equivalence on the canonical LOGICAL SET.**
Modifies Part R line 1390 (`CheckInstanceSolePub`, Rel.cpp:4977-4987):

```diff
-      inst_per_pub : map<(pub_table_ptr, forcing_index), count>   # RE-KEYED at D3.a.3
+      region_key(rs) := canonicalize(logical_key(rs))   # SET, canonical field-id order
+      # Store DEDUP keys on region_key: two forcings with EQUAL region_key share ONE
+      #   store (obligation 2 — rel[A,C]==rel[C,A]). forcing_index stays the pub-sharing
+      #   distinguisher only across DISTINCT region_keys (the D3.a.3 case unchanged).
+      inst_per_pub : map<(pub_table_ptr, region_key), count>
```

**Hunk R5-3 — R.1.2, annotate the InstanceStore Key as set-partition + chosen order.**
Comment-only on Part R line 1069 (`KeyAt(iid) -> const Key&`):

```diff
   KeyAt(iid) -> const Key&                        # :157
+  # Key's FIELD ORDER (Database.cpp:1259-1267, KeyTypes order) is the CHOSEN PHYSICAL
+  #   arrangement order; the memo PARTITION is the field SET. Equal-set/different-order
+  #   stores are ONE region logically (D5 decides keep-both-tries vs canonicalize — D2.6).
```

**Hunk R5-4 — R.1.4, KEEP physical order OUT of the contract (a negative hunk).**
Comment on Part R line 1206 (the AllFields member_key assignment):

```diff
       out[v] = { visible_fields = AllFields(v), member_key = AllFields(v) }
+  # member_key IS the logical (region-partition) key. DO NOT add a physical/arrangement
+  #   order field to RowContract — it would be a satellite the optimizer must preserve
+  #   (F1 lesson; cf. role-in-Equals-only, Tuple.cpp:308-310). Reserve arrangement order
+  #   as an INERT D5 planning annotation with no Stage-A producer (H-A1 domain pattern).
```

**Hunk R5-5 — the declared-bracket key check (new; DIFF-R3 surface, DIFF-R5 semantics).**

```diff
+  CheckDeclaredBracketIsKey(v, K):        # K = declared bracket SET; ORDER dropped for the check
+    if member_key(v) ⊆ K:  accept          # K is a superset of a proven key ⇒ K is a key
+    else:                  REJECT           # unprovable at Stage-A precision; clean diagnostic.
+                                            #   (AllFields-widen is the answer-identical alt — STOP-R5-A)
+                                            #   LIFT: Stage-B Minimize may prove smaller K.
```

### SOUNDNESS OBLIGATIONS

1. **LOGICAL-KEY-IS-A-KEY.** A declared bracket set `K` must functionally determine the
   visible row: checked as `member_key(v) ⊆ K`. On failure: reject (or AllFields-widen —
   STOP-R5-A). Never accepted silently.
2. **ORDER-INVARIANCE OF REGION IDENTITY.** `rel[A,C]` and `rel[C,A]` must resolve to the
   SAME region — one store, one partition. The store/region equivalence key is the
   canonical logical SET (Hunk R5-2); the ordered tuple would double-mint and trip
   V-INST-SOLE. This is the near-term behavioural obligation DIFF-R5 imposes on D3.a.3.
3. **PHYSICAL-ORDER-INERTNESS.** No structural-identity surface — `QueryTupleImpl::Hash`,
   `::Equals`, CSE-color, `RowContract.member_key`, the DR/region equivalence key — may
   read the physical order. A reintroduced order-in-identity fragments arrangement sharing
   and flips goldens for zero semantic gain (the role-in-Equals-only teeth, generalized).
4. **ANSWER-IDENTITY UNDER ORDER CHOICE.** Any physical-order choice, and any AllFields-
   widen of an unprovable declared key, must be answer-identical to flat `-demand`. Over-
   or under-partitioning changes sharing only, never answers — asserted by the I0/eqgate
   referee (flat == declared == nested).
5. **STORE-KEY ↔ BRACKET-TUPLE.** The runtime `Key_<id>` field ORDER is the chosen physical
   order (Database.cpp:1259-1267); the memo partition is the field SET. Two stores with an
   equal field-set must dedup to one region before mint (obligation 2 at the codegen layer).
6. **PARSER WELL-FORMEDNESS (DIFF-R3, from the fuzzing addendum).** The bracket is an
   ordered, NAMED, DUP-FREE column list: `foo[_]`/`foo[_A]` reject (unnamed), `foo[A,A]`
   rejects (repetition). Dup-free ⟺ the SET is well-defined; ordered ⟺ the physical hint is
   well-defined. DIFF-R5 relies on these to keep the split meaningful.

### OWNER DEPENDENCIES

- **Owner 2026-08-03 "brackets bind arbitrary (non-prefix, ordered, multi-column) keys →
  logical/physical split"** (record :435-461) — RATIFIED direction; the direct source.
- **Owner 2026-08-03 "adornments as a FUZZING axis + bracket parser obligations"**
  (:463-488) — RATIFIED direction; supplies obligation 6 and the generator-based test plan.
- **D3.4 candidate 3 (flat-key RowContract)** — RATIFIED; the logical key IS the landed
  `member_key` (obligation 1/2 reuse it, no new structure).
- **Stage-A implementation adjudication #1 (Hash exclusion / H-A2 refinement)**
  (:150-156) — the role-in-Equals-only precedent grounding obligation 3.
- **D3.a.3 multi-adornment (disjoint stores over one pub) + V-INST-SOLE re-key**
  (Part R R.1.5; Rel.cpp:4970-4987) — the sharing precedent and the near-term hazard
  Hunk R5-2 repairs.
- **Owner direction candidate "USER-DECLARED REGIONS" (hint-vs-mandate, OPEN)**
  (:351-383) — DIFF-R3 dependency; the reject-vs-widen policy (STOP-R5-A) hangs off it.
- **D2.6 (DEFERRED — arrangement reader-handle discipline)** (:60-66) — the "two
  arrangements over one pub vs one refcounted arrangement" realization (STOP-R5-B).
- **D5 / DIFF-R6 / CostModel (arrangement sharing)** — the physical-order half's ONLY
  consumer; until it exists, the order is a reserved inert annotation.
- **D2.12 (V-CW)** — reads the logical key set when widening; no direct DIFF-R5 change,
  noted for consistency (the widened fixpoint keys on the logical partition).

### TEST / WITNESS plan (golden-master terms)

- **Referee.** I0 / eqgate (flat == declared == nested answer-identity) is primary; the
  owner's **generator-based adornment-placement fuzzer** (covering-array machinery) is the
  exhaustiveness referee — every enumerated bracket placement/order must either compile
  answer-identically or draw a CLEAN diagnostic (never a miscompile, never an assert),
  diagnostics compared by class (PF-3 vocabulary).
- **Order-invariance witness (obligation 2, near-term).** A multi-adornment program whose
  two adornments name the SAME bracket SET in DIFFERENT order — must (a) produce byte-
  identical answers and (b) collapse to ONE store: `kSubgraphInstantiate` census == 1,
  V-INST-SOLE not tripped. This is the deliberate twin of `demand_multi_adorn_witness`
  (which mints `kSubgraphInstantiate=2` for DIFFERENT sets); the DIFF-R5 twin mints =1 for
  a reordered SAME set. `.eqgate` sidecar for flat==nested.
- **Physical-order-inertness sweep (obligation 3).** The DIFF-R5 analog of the H-A9
  role-OFF full-corpus sweep: permuting a declared bracket's order (where legal) must yield
  ZERO `.df` / `.contract` diffs and ZERO answer diffs — the order is invisible to every
  front-end dump (it only ever surfaces at the not-yet-existent D5 arrangement tier).
- **Key-validity reject witness (obligation 1).** A declared bracket provably NOT a key
  (`member_key ⊄ K`, i.e. smaller than the AllFields floor) → all-4-modes clean diagnostic.
  Directed witness in `runall.sh`'s diagnostic list, PF-3 vocabulary.
- **Parser reject witnesses (obligation 6).** `foo[_]`, `foo[_A]`, `foo[A,A]` → all-4-modes
  diagnostics (DIFF-R3 parser corpus; DIFF-R5 inherits, does not re-author).
- **Composite-key carriers (Stage-D/D5 horizon).** The non-linear-TC pivot (composite
  interior-pivot key; corpus `tc_nonlinear_diff`) and the disassembler `(FuncEA,BlockEA)`
  nested key (running-example-disassembler.md) are the eventual end-to-end carriers of
  non-prefix composite bracket keys; each with a `.batches` + `bin/Oracle` derivation-count
  referee once the DIFF-R3 declared surface lands. Near-term they only motivate the split;
  they are not yet constructible without the parser surface.


### OPEN OWNER ITEMS (DIFF-R5)

- STOP-R5-A (declared-regions owner-call, currently OPEN — record: Owner direction candidate 2026-08-03 'USER-DECLARED REGIONS', tension (2) hint-vs-mandate): when a declared bracket set is smaller than the Stage-A proven member_key (unprovable-as-key at Stage-A precision), does the checker REJECT (clean diagnostic, recommended) or ALLFIELDS-WIDEN (drop the hint, answer-identical but silently defeats the declared partition)? DIFF-R5 recommends REJECT + a Stage-B-Minimize LIFT-candidate; the hint-vs-mandate call decides.
- STOP-R5-B (D2.6, DEFERRED): when two forcings carry the SAME logical key set but DIFFERENT physical orders, are they ONE refcounted arrangement or two individually-retractable arrangements over one pub? The physical-order-sharing realization (keep both tries, or canonicalize to one) rides the D2.6 reader-handle discipline.
- Owner confirm: adopt the region-equivalence key = canonical LOGICAL SET (obligation 2) as the store/region dedup key, superseding the ordered-tuple that (pub_table, forcing_index) implies today — so rel[A,C] and rel[C,A] collapse to one store. Needs a V-INST-SOLE re-key note (Rel.cpp:4970-4987).

### PANEL RECORD (DIFF-R5) — 13 findings, 13 survived refutation

- **DIFF-R5-correctness-lifecycle-1** [CONFIRMED; sev MAJOR] Hunk R5-2 re-keys the V-INST-SOLE VALIDATOR to region_key and is presented as the fix for obligation 2 (order-invariance / double-materialization), but a validator is a check, not a collapse mechanism — the diff never gives the shared store a single BIRTH. BuildSubgraphInstanceOps still mints one full instance-op family per forcing, so two same-region forcings produce two stores, and the re-keyed belt would ABORT the diff's own order-invariance witness rather than collapse it.
  - EVIDENCE: BuildSubgraphInstanceOps iterates `for (const RecognizedSubgraph &rs : query.RecognizedSubgraphs())` (Rel.cpp:1052) and allocates a FRESH `sid = flow.instances.size()` per forcing (Rel.cpp:1078), pushing its own DRInstance (:1107) and kSubgraphInstantiate (:1109-1159); nothing dedups on the logical set. CheckInstanceSolePub only COUNTS ops and aborts on count!=1 (Rel.cpp:4977-4990) — it is a check, not a collapse. So re-keying it to region_key with two same-region mints yields count==2 and ValidatorFail fires (:4986-4988), i.e. the diff's own order-invariance witness would ABORT, not collapse. The birth of the shared store is unowned; obligation 5's 'dedup at the codegen layer' is too late (two distinct DRInstance/sid already exist). The diff's model is internally inconsistent about the collapse mechanism.
  - AMENDMENT (normative): Add a pseudocode hunk against Part R R.1.2 / Rel.cpp:1052-1078 that dedups RecognizedSubgraphs on canonical region_key BEFORE sid assignment: the first forcing of a region mints the store (one sid + one kSubgraphInstantiate); later same-region forcings resolve to that sid and wire their guard/pub reads to it, minting no new instance-op family. Re-caption Hunk R5-2 as the always-on BELT that VERIFIES the collapse, explicitly not the mechanism, and move obligation 5's 'dedup before mint' from the codegen layer to the DR-IR sid mint (Rel.cpp:1078).
- **DIFF-R5-correctness-lifecycle-2** [CONFIRMED; sev MAJOR] The shared-store model gives the region a single birth but leaves its TEARDOWN and REBUILD undefined: kInstanceDeath, kInstanceSeal, and the band-(a2/a2') rebuild drains are all minted per forcing_index against their own sid, yet obligation 2 and the order-invariance witness constrain only kSubgraphInstantiate (census==1) and V-INST-SOLE. Two forcings collapsed to one store would be double-sealed (and, under -demand-retract, double-death-drained / double-rebuilt).
  - EVIDENCE: In the same per-forcing loop, kInstanceDeath is minted with `death.instance_store_id = sid; death.forcing_index = rs.forcing_index` (Rel.cpp:1170-1171, gated on differential demand :1163) and kInstanceSeal with `seal.instance_store_id = sid; seal.forcing_index = rs.forcing_index` (Rel.cpp:1180-1181); the rebuild drains are effects of the same per-forcing kSubgraphInstantiate (InstantiateEffects, Rel.cpp:1119-1121). The witness plan constrains only 'kSubgraphInstantiate census == 1' and 'V-INST-SOLE not tripped' — never death/seal/rebuild — so a birth-only collapse leaves the shared store double-sealed and (under -demand-retract) double-death-drained against one sid, invisible to the stated referee.
  - AMENDMENT (normative): Extend obligation 2 and the order-invariance witness to require kInstanceDeath and kInstanceSeal census == 1 PER REGION (not per forcing_index) and to collapse the band-(a2/a2') rebuild drains to the shared sid; add a per-region uniqueness belt covering kInstanceDeath/kInstanceSeal (or fold both kinds into Hunk R5-2's region-keyed uniqueness). State that the mint-side collapse (finding 1) must retire the redundant forcings' death/seal/rebuild ops, not only their birth.
- **DIFF-R5-termination-confluence-1** [SOFTENED; sev MAJOR] Hunk R5-2 re-keys the V-INST-SOLE validator to (pub, region_key) but supplies no hunk that dedups the MINT, so for two same-set/different-order forcings the pseudocode produces a validator ABORT — the exact opposite of obligation 2's stated 'kSubgraphInstantiate census == 1, V-INST-SOLE not tripped.' The dedup is targeted at the wrong layer.
  - EVIDENCE: The core diagnosis is CONFIRMED and identical to finding 1: the mint loop emits one kSubgraphInstantiate per RecognizedSubgraph (Rel.cpp:1051-1078) and re-keying CheckInstanceSolePub (Rel.cpp:4977-4990) cannot merge anything — two same-region mints abort. The proposed relocation is a plausible ALTERNATIVE fix layer, but two claims overshoot: (a) the seen_variants dedup (Demand.cpp:479) keys on the exact `BindingPattern()` STRING, so it collapses only exact-duplicate redecls, not same-set/different-order brackets — 'ALREADY collapse there today' holds only for identical patterns, and using it for set-canonical dedup is itself new logic, not a no-op; (b) fully relocating the collapse to forcing creation and leaving V-INST-SOLE untouched discards the second bracket's physical/arrangement order, which the diff explicitly reserves (D2.6 keep-both-tries vs canonicalize) — so the validator-as-belt from finding 1 remains valuable.
  - AMENDMENT (normative): Fold into finding 1 as the same defect: the collapse must key on a CANONICAL LOGICAL SET upstream of V-INST-SOLE (owner picks the layer — forcing creation OR sid mint), and the seen_variants hook must be re-keyed on the canonical set, not the raw BindingPattern string. Do not drop V-INST-SOLE's re-key to a belt; keep it as the collapse verifier. Note that a forcing-creation collapse must still preserve the reserved physical-order distinction.
- **DIFF-R5-termination-confluence-2** [CONFIRMED; sev MAJOR] Obligation 2 collapses two forcings onto one store but is silent on the demand-liveness gate's input, so under the landed differential (-demand-retract) regime a merged store can suffer premature kInstanceDeath — a per-key quiescence that depends on which sibling forcing retracts first (non-confluent death).
  - EVIDENCE: Each forcing owns its own demand relation/demand_table (`ri.demand_table` resolved per forcing_index, Rel.cpp:1059; FabricateDemandMessage is per-adornment). The differential whole-instance death gate reads `demand.Present(dq)` on the store's demand table (Database.cpp:2385-2389: 'the gate skips a rebuild whose demand is committed-absent ... iid existence is NOT a liveness signal'). If obligation 2 unifies the STORE but leaves two separate demand relations, the single gate reads only one contributor — a key still demanded by sibling B is killed when A's demand retracts (over-retraction; content depends on retract interleaving). Obligation 2 is stated as a GENERAL rule against the landed D3.a.2/D3.a.3 diff-demand regime but supplies no demand-union rule.
  - AMENDMENT (normative): Add to obligation 2: when forcings collapse to one region/store, their demand relations must unify into ONE demand table (refcounted/counter union) so `demand.Present(dq)` means 'demanded by ANY sibling forcing.' Until that union exists, explicitly SCOPE the same-set merge to MONOTONE-demand stores (where the gate degenerates to irrevocability) and add a documentary fence that diff-demand × same-set-merge is unsound-until-unified.
- **DIFF-R5-termination-confluence-3** [CONFIRMED; sev MAJOR] Obligation 4's clause 'Over- or under-partitioning changes sharing only, never answers' is false for under-partitioning and directly contradicts obligation 1 / CheckDeclaredBracketIsKey; as written it licenses accepting a coarser-than-key bracket as answer-safe, which is a miscompile.
  - EVIDENCE: Obligation 1 exists precisely because a memo key must functionally determine the visible row (CheckDeclaredBracketIsKey rejects `member_key(v) ⊄ K`). Under-partitioning (K coarser than a real key) conflates distinct demanded rows into one iid — the per-key content set and per-key death gate become ill-defined, yielding wrong answers/resurrection. The diff's own AllFields bullet correctly states only the FINER direction is safe ('a finer partition is always a sound key; over-partitioning changes only sharing'). Obligation 4's 'Over- or under-partitioning changes sharing only, never answers' therefore contradicts obligation 1, and the eqgate cannot even construct an under-keyed program because obligation 1 rejects it upstream.
  - AMENDMENT (normative): Strike 'or under-' from obligation 4: only OVER-partitioning (a finer partition, e.g. AllFields-widen) is answer-neutral. State explicitly that under-partitioning (a bracket coarser than a real key) is UNSOUND and is exactly what obligation 1 rejects, so it never reaches the eqgate.
- **DIFF-R5-testability-oracle-1** [CONFIRMED; sev MAJOR] The 'order-invariance witness' (obligation 2), labeled '(obligation 2, near-term)' and 'the deliberate twin of demand_multi_adorn_witness', is not constructible near-term — it requires the DIFF-R3 bracket parser that does not exist. This is the H-A9 unconstructible-witness trap.
  - EVIDENCE: The current adornment surface is strictly positional. demand_multi_adorn_witness.dr declares `#query q(bound u64 A, free u64 B)` + `#query q(free u64 A, bound u64 B)` whose logical SETS are {A} and {B} (DIFFERENT sets — the reason it mints kSubgraphInstantiate=2, per the file's own header). There is no positional spelling for 'same SET, different ORDER': the bound set is fixed by which positions carry `bound`. Expressing rel[A,C] vs rel[C,A] requires the DIFF-R3 bracket surface, which the diff elsewhere admits is 'not yet constructible without the parser surface.' The witness is therefore mis-labeled '(obligation 2, near-term)' — the H-A9 unconstructible-witness trap.
  - AMENDMENT (normative): Re-classify the order-invariance witness as DIFF-R3-gated (Stage-D horizon), not near-term, and state that until the bracket parser lands there is NO program that exercises the two-orders-one-set path. Move it into the same 'motivates but not yet constructible' bucket as the composite-key carriers.
- **DIFF-R5-testability-oracle-2** [SOFTENED; sev MAJOR] Hunk R5-2 (re-key V-INST-SOLE from (pub_table, forcing_index) to (pub_table, region_key)) is the ONLY near-term behavioral IR change in DIFF-R5, yet no constructible near-term test discriminates it from the current code, and it is not the 'TINY delta' the complexity verdict claims.
  - EVIDENCE: CONFIRMED core: on the entire existing corpus distinct adornments carry distinct SETS (demand_multi_adorn_witness: {A} vs {B}), so keying V-INST-SOLE on region_key vs forcing_index partitions the kSubgraphInstantiate ops IDENTICALLY — the re-key is byte-identical and no golden/census/eqgate discriminates it; its only discriminating witness is the unconstructible finding-1 witness. So R5-2 is an untested behavioral change pre-DIFF-R3. OVERSHOOT: the 'new machinery / new DROp field' claim is false — the logical key already exists as `DRInstance.key_cols` (Rel.cpp:1096, = rs.key_cols) and the op carries `instance_store_id` (Rel.cpp:1117), so region_key is `canonicalize(flow.instances[op.instance_store_id].key_cols)`, a lookup+canonicalize with zero new fields.
  - AMENDMENT (normative): Keep Hunk R5-2 as a pseudocode-only reservation until DIFF-R3 lands, and note its discriminating witness co-lands with the parser. Correct the 'new machinery' framing: the logical key is already carried by DRInstance.key_cols reachable via op.instance_store_id, so the re-key is a lookup + a canonicalization step, not a new DROp field.
- **DIFF-R5-testability-oracle-3** [CONFIRMED; sev MAJOR] The physical-order-inertness sweep (obligation 3) is vacuous near-term and mis-scoped: it has nothing to permute without the DIFF-R3 bracket surface, and where physical order IS live today — the emitted Key_<id> struct field order — it lies outside the swept front-end dumps, so the sweep cannot prove the end-to-end inertness it claims.
  - EVIDENCE: Two parts both hold. (1) 'permuting a declared bracket's order (where legal)' has no legal surface pre-DIFF-R3, so 'where legal' is empty and the sweep tests nothing (same H-A9 root as findings 1/5). (2) Physical order IS live in codegen today: EmitInstanceStructs emits `Key_<id>` fields in `store.KeyTypes()` order (Database.cpp:1251-1268, `for (TypeLoc t : store.KeyTypes())`), and KeyTypes derives from key_cols/bound_indices decl order. A sweep restricted to .df/.contract dumps excludes the one surface where reordering shows, so it cannot distinguish 'order excluded from identity' from 'order not yet plumbed.'
  - AMENDMENT (normative): State the inertness sweep is DIFF-R3-gated (nothing to permute before then), and when it lands, extend the swept surface to include the generated datalog.h Key struct plus a runtime/eqgate answer-identity assertion — demonstrating that a bracket-order permutation CHANGES codegen (Key_<id> field order) while leaving answers and every structural-identity surface untouched, rather than claiming order is invisible everywhere.
- **DIFF-R5-testability-oracle-4** [CONFIRMED; sev MAJOR] The key-validity reject witness (obligation 1) under-specifies the view kind and can silently exercise the ACCEPT arm instead of the reject arm — a green test that never tests the reject.
  - EVIDENCE: CheckDeclaredBracketIsKey rejects only when `member_key(v) ⊄ K`. TransferContract computes REFINED (proper-subset-of-AllFields) member keys for TUPLE-kMember/COMPARE/NEGATE/INSERT (passthrough, RowContract.cpp:184-198), JOIN (union, :200-215), and AGGREGATE (group prefix, :217-230); member_key == AllFields only for TUPLE-kDistinct (:182-183), the SELECT/MERGE/MAP/KVINDEX/other fallback (:232-238), the empty-key fallback (:242-244), and Phase-1 recursive-SCC members. Over a refined-key view a declared K smaller than the full row can still be a SUPERSET of the small member_key and thus ACCEPT. The witness is described only as 'smaller than the AllFields floor' without pinning that the target view's member_key must be forced to AllFields — an implementer can land it over a refined-key view and get a silent accept.
  - AMENDMENT (normative): Specify the reject witness must target a view whose member_key is provably AllFields (a MERGE/MAP/KVINDEX/SELECT view or a recursive-SCC member) and declare K a proper subset of that AllFields floor. Add an assertion or a companion accept-witness over the same shape that pins WHICH arm fired, so a mis-placed witness cannot pass by accepting.
- **DIFF-R5-testability-oracle-5** [CONFIRMED; sev MINOR] The AllFields-widen arm of STOP-R5-A has no referee at all — the diff itself states the eqgate cannot catch it, then names no substitute, leaving that policy arm entirely unobservable.
  - EVIDENCE: The diff itself states AllFields-widen 'silently defeats the user's declared partition intent, which the answer-identity referee cannot catch ... invisible to the eqgate.' Widening changes the store PARTITION (which field set keys the store, which rows land in which iid) — structural, not answer-visible. The test plan lists only I0/eqgate (answer), parser rejects, and the reject witness; no structural referee pins the widen arm. If the owner resolves STOP-R5-A toward widen, that arm has zero coverage.
  - AMENDMENT (normative): Name a structural referee for the widen arm: a kSubgraphInstantiate census count and/or a .rel store-partition golden pinning which field set keyed the store, so an incorrect widen (wrong view or dropped store) is observable even though answers are order-invariant.
- **DIFF-R5-necessity-1** [CONFIRMED; sev MAJOR] The diff identifies the region-partition/logical key with `RowContract.member_key`, but the region partition is ALREADY authoritatively the store Key (built from the demanded bound set). `member_key` is a different quantity used only in the Hunk R5-5 validity check. Naming them as one 'logical key' creates a second authority for something an existing structure already is (F1/two-authorities), and mis-routes region equivalence through a quantity that is frequently `AllFields`.
  - EVIDENCE: Two distinct quantities are conflated. The region/store partition is `DRInstance.key_cols = rs.key_cols` (Rel.cpp:1096), the DEMANDED bound set tracing to Demand.cpp bound_indices, lowered to ProgramInstanceStore key_types and the runtime Key_<id> — the thing that decides which rows land in which iid. `member_key` is an independent post-Optimize side-table keyed by QueryViewImpl* (RowContract.h:38-51), computed by TransferContract from the graph (RowContract.cpp:158-248), used NOWHERE in store construction, and it routinely collapses to AllFields for SELECT/MERGE/MAP/KVINDEX (:232-238) and recursive-SCC members — the OPPOSITE of the small demanded partition. The diff's own Hunk R5-5 separates them (`member_key(v) ⊆ K`), yet Section 1, the IR-home table, and obligation 2 route region equivalence THROUGH member_key. Hunk R5-1 even defines `logical_key := set(bound_indices)` (= key_cols), contradicting Section 1's 'the logical key IS the landed member_key.' So the diff picks the wrong authority for the region partition; key_cols already IS it.
  - AMENDMENT (normative): Split the concepts: region-partition/logical key = canonical(demanded/declared bracket SET K) = canonical(store `key_cols`), the EXISTING authority — no new structure, no member_key. Restrict member_key to its actual role, the proven-key WITNESS consumed only by CheckDeclaredBracketIsKey (`member_key(v) ⊆ K`). Rewrite the IR-home table row and Section 1 accordingly, and make Hunk R5-2's `logical_key(rs)` explicitly the store's key_cols set, not member_key.
- **DIFF-R5-necessity-2** [CONFIRMED; sev MAJOR] Hunk R5-2 — re-keying the always-on V-INST-SOLE validator from `(pub_table, forcing_index)` to `(pub_table, region_key)` — is the diff's ONLY behavioural change, yet its triggering hazard (two forcings naming the SAME set in DIFFERENT order) is unconstructible until the DIFF-R3 bracket parser lands. It modifies a live correctness validator with no near-term witness and can be deferred with zero loss.
  - EVIDENCE: The diff states physical order 'has no consumer until D5' and the double-mint hazard arises only 'once brackets decouple order from arg-order.' The only near-term producer of store keys is the positional binding pattern → bound_indices decl order (Demand.cpp:483-486); no surface expresses rel[A,C] vs rel[C,A]. For every current D3.a.3 program N adornments carry N DISTINCT sets, so `(pub_table, forcing_index)` and `(pub_table, region_key)` partition the kSubgraphInstantiate ops identically — the re-key of the live fprintf+abort CheckInstanceSolePub (Rel.cpp:4977-4990) is unobservable on the whole corpus and its only discriminating witness is unconstructible pre-DIFF-R3. Landing an untested relaxation of a correctness validator ahead of the feature that exercises it is risk with no near-term benefit.
  - AMENDMENT (normative): Demote Hunk R5-2 and obligation 2 from a near-term DIFF-R5 item to a DIFF-R3 CO-REQUISITE: the V-INST-SOLE re-key (plus the mint-side collapse of finding 1 and the death/seal collapse of finding 2) lands in the same slice as the bracket parser that first produces same-set/different-order forcings, together with the order-invariance witness. Until then leave `(pub_table, forcing_index)` untouched and record the re-key as a labeled DIFF-R3 prerequisite.
- **DIFF-R5-necessity-3** [CONFIRMED; sev MINOR] Hunk R5-1 (splitting `bound_indices` into `logical_key`/`physical_order` at the demand locate site) introduces two names no other hunk consumes, and the diff's central 'zero new code' thesis overclaims — `rel[A,C] == rel[C,A]` does NOT hold today without the new `canonicalize` in Hunk R5-2.
  - EVIDENCE: Hunk R5-1's `logical_key := set(bound_indices)` and `physical_order := seq(bound_indices)` have no executable reader: member_key is computed independently by RowContract from the graph (RowContract.cpp:158-248, not from bound_indices) and the store key uses key_cols directly (DRInstance.key_cols, Rel.cpp:1096) — neither reads these names, so the hunk is a pure relabel (and, per finding 10, a third spelling of the demanded set already carried by key_cols). Separately, MemberKeyFromIds renders member_key as a vector in per-view VisibleCols order (RowContract.cpp:125-134, `for (auto col : VisibleCols(v))`), so two views with reversed visible-column order produce DIFFERENT vectors; byte-equality of rel[A,C] and rel[C,A] requires the very `canonicalize(...)` Hunk R5-2 introduces as new code, in tension with Section 1's 'holds today with zero new code.'
  - AMENDMENT (normative): Delete Hunk R5-1 or fold it into DIFF-R3 as documentation of the parser's set/order extraction (no near-term consumer). Correct Section 1: member_key is order-free only as a SET; its rendered vector is per-view-column-ordered, so the order-invariant region key requires a canonical re-sort — a small piece of new code obligation 2 carries. Drop the 'zero new code' framing.

---

## DIFF-R6 — The matrix / semiring frame (D5 storage + cost) (formalized 2026-08-03)

**Tier: PLANNING / COST only. Zero IR nodes, zero lowering edits, zero runtime edits, zero golden churn.** DIFF-R6 is a *reading* of the already-landed structures in Part R plus one structural rule handed to `docs/proposals/CostModel.md`. It is the smallest of the R-diffs by design: everything it names already exists in the tree (the InstanceStore is R.1.2; the lfp is R.1.3); R6 adds only a correspondence and a cost discriminator.

**SHRINK vs the seed.** The seed's R6 block (`region-model-pseudocode-seed.md` Part 2) reads as if it introduces a *storage direction* ("InstanceStore = row-wise sparse matrix storage"). It does not — `InstanceStore<Key,RowT>` (R.1.2, `include/drlojekyll/Runtime/InstanceStore.h`) IS already the row-wise store; R6 only *names* it as a matrix arrangement and prices it. And the seed's "over composition (pivot-generating) recursion largely does NOT [prune]" is sharpened below from "largely does not" to the exact quadratic statement. No new format, no new object.

---

### 1. The correspondence table (keyed relation = sparse boolean-semiring matrix)

The semiring here is the **boolean reachability semiring** `(𝔹, ∨, ∧)` — a STRUCTURAL model of *which key activates which*, distinct from CostModel §1.1's two *cost-currency* semirings (counting `(ℕ,+,×)` / tropical `min/max/lfp`). See SOUNDNESS OBLIGATION O-1: R6 supplies structure (the sparsity pattern), never a cardinality.

| Region-model object | Boolean-semiring / matrix object | Part-R / code anchor |
| --- | --- | --- |
| keyed relation `rel[K](V)` | sparse boolean matrix `M` over `K × V` | notation proposal (owner 2026-08-03) |
| one instance / activation at key `k` | matrix **row** `M[k] = { v : rel(k,v) }` | `DRInstance` iid, R.1.2 (Rel.cpp:1006) |
| `InstanceStore` (row-wise store) | row-wise sparse **arrangement** = **trie by `K`** | `InstanceStore.h`; R.1.2 (`FindOrAddInstance`, :109) |
| bracket key order `[Bound...]` | trie / arrangement column (prefix) order | logical/physical key split (owner 2026-08-03) → D5 |
| **linear** rule `rel(F,T):rel(F,X),edge(X,T)` | `M := M ∨ M·E` (right-multiply by adjacency `E`) | `demand_tc_witness.dr` recursion |
| **non-linear** rule `rel(F,T):rel(F,X),rel(X,T)` | `M := M ∨ M·M` (matrix **square**) | `tc_nonlinear_diff.dr` recursion |
| region call `⋁_X` over shared pivot `X` | inner semiring **sum** over the pivot index | JOIN pivot, R.1.3 (`LowerDRRounds`) |
| transitive closure (fixpoint to closure) | **Kleene star** `M⁺` (closure over `(𝔹,∨,∧)`) | lfp, R.1.3 (Stratum.cpp:1799); CostModel §3.3 `Z_S` |
| sparsity pattern of `M` | the **activation-call graph** ("row F needs row X") | recognition, R.1.2; DIFF-R2 activation-SCC |
| **cyclic** sparsity (SCC in activation graph) | **coupled row-fixpoint** (V-CW widened) | D2.12; CostModel §3.3 lfp |
| **acyclic** sparsity (DAG) | well-founded **memoized descent**, no iteration | DIFF-R2 DOWN arm |

Two identities the table must not be misread on (O-4): `M·M` is the **one step** of the relation, not the closure — `TC = M⁺` needs the lfp run to quiescence (R.1.3's `R_S` rounds), so cost reads the `R_S`-round `Z_S`, never a single product. And the memo (next section) is what keeps the star row-count polynomial.

---

### 2. The demand-effectiveness discriminator (the cost-model rule)

**Hook: CostModel.md §4 `.cost` scenario family** (owner ruling 2026-07-30, `[[cost-scenario-family]]`), selecting the §3.3 `Z_S^demand` closure symbol and the §5.4 crossover branch. The rule is a STRUCTURAL predicate on the recursive body, evaluated at cost time, not a lowering change.

> **DISCRIMINATOR.** For a demanded recursive relation `rel` under adornment `a` with demand key `K` and demanded-key count `D_a` (CostModel §1.2, `Rel.h:704`):
>
> - **tail-in-key / LINEAR** — *every* recursive body atom of `rel` threads `K` **unchanged** (the recursive atom sits at the bracket position; `demand_tc_witness`: `path(F,·)`, source-key F invariant). Then demand **PRUNES**: the activated sub-relation is exactly the answers reachable from the `D_a` seeds, `|activated| = D_a·|reach(K)|`, ONE activation per seed, no interior sub-problem generation. It selects `Z_S^demand = D_a·|reach|` and the §5.4 recursive branch — *demand pays iff `C > N + 2D + 2·R·D·r`* (§5.4, step-8 dropped).
>
> - **pivot-generating / NON-LINEAR** — some recursive body atom re-keys on an **interior pivot** (bracket-arg variable ≠ head bracket var; DIFF-R2's X≠F; the sink-then-source seam of running-example §"endpoint-key vs interior-pivot"). Then demand **ACTIVATES the whole reachable sub-relation**: the demanded-source set fills with *every* node reachable from the seeds, so **demanded-set == answer-set**; `|activated| = Θ(|reach|²)`, and pruning is confined to *unreachable* nodes only — the demanded set never shrinks below the answer set. It selects `Z_S^demand = D_a·Z_reachsub` (super-linear), the §2.6 non-linear per-round rule (`Σ_r δ_{r-1}·|accum_{r-1}|`), and — critically — the **InstanceStore memo (R.1.2 `FindOrAddInstance`, one activation per interior key) is what bounds this at QUADRATIC rather than EXPONENTIAL** (without the memo, `tc-from-X` is recomputed once per path reaching X). This is CostModel §6.2's GATE subject and §2.7's per-key `D_a` multiplicity.

The discriminator is exactly the running-example §matrix-frame's "sparsity graph is a DAG vs has SCCs" split, priced: linear ⟺ acyclic key threading ⟺ descent-pruning; non-linear ⟺ cyclic/pivot-generating ⟺ coupled-fixpoint over the quadratic sub-relation.

---

### 3. Pseudocode-diff hunks

DIFF-R6 modifies **no lowering line** of Part R. The `+/-` diffs are (a) additive cost-tier annotations on the Part-R structures that *are* the matrix frame, and (b) the substantive rule against CostModel.md. Honesty note: (a) are comment-only reads; (b) is R6's real deliverable (seed R6 "Touches: the cost model").

**H-R6.1 — annotate R.1.2 (InstanceStore = row-wise matrix arrangement).** Against the Part-R lines:

```diff
   FindOrAddInstance(key) -> iid                   # :109, mints empty current+frozen
+  # [R6 cost frame] the trie/arrangement key IS the matrix ROW INDEX; the memo
+  #   (FindInstance :103 / FindOrAddInstance :109) activates each interior key
+  #   AT MOST ONCE -> non-linear demand is O(reach^2), never exponential (§6.2 GATE).
 ...
 So the InstanceStore ALREADY IS the memo table of activations (rows = keys);
 the recognition is subgraph-shaped (SIP-reachable), not SCC-shaped.
+# [R6] rows = keys = matrix rows of M; the row-wise store = an arrangement =
+#   a trie by K. NO storage change — R6 names the landed object, does not add one.
```

**H-R6.2 — annotate R.1.3 (the lfp is the Kleene star; linear/non-linear selects the cost branch).**

```diff
     LowerDRRounds(dr_flow, stratum):                        # Stratum.cpp:1799
+      # [R6 cost frame] this lfp IS M+ (Kleene star) over the boolean semiring;
+      #   R_S rounds = the star's iteration. Linear recursion (M v M.E) costs
+      #   Z_S per round; non-linear (M v M.M) costs the §2.6 super-linear rule.
 ...
     # DIVISION OF LABOR: a recursive SCC WITH a ProgramInductionRegion is
     # ... transitive closure) gets the OVERDELETE/REDERIVE/INSERT from THIS
     # machinery (drain_stratum populated only for non-induction-owned tables).
+    # [R6] cyclic activation-sparsity => coupled row-fixpoint (V-CW/D2.12);
+    #   acyclic => descent (DIFF-R2 DOWN). The split is a RUNTIME fact (data-graph
+    #   cyclicity), so cost assumes the coupled/quadratic branch unless linearity
+    #   is STRUCTURALLY provable (O-2, conservative like §1.4).
```

**H-R6.3 — the substantive rule (CostModel.md, §5.4-adjacent).** Refine the closure symbol the demanded relation gets:

```diff
  # CostModel §5.4 recursive crossover:
- Recursive (M = C >> N, S = D.r, R rounds): demand pays iff C > N + 2D + 2.R.D.r
+ Recursive: the demanded closure symbol Z_S^demand is DISCRIMINATOR-SELECTED:
+   linear (tail-in-key)      -> Z_S^demand = D.|reach|        (real pruning; pays as above)
+   non-linear (pivot-gen)    -> Z_S^demand = D.Z_reachsub     (demanded==answer, O(reach^2);
+                                pruning limited to unreachable nodes; memo bounds it)
+ The selector is a syntactic body predicate (bracket-arg identity, DIFF-R2 /
+   the SIP From-preservation check R.1.1:869-870), read at cost time only.
```

---

### 4. What DIFF-R6 does NOT change

- **No Query.h node.** (Contrast DIFF-R1's `REQUEST` node — R6 is orthogonal and pre-supposes nothing about it.)
- **No `Demand.cpp` mint, no `Rel.cpp`/`Stratum.cpp` lowering, no `InstanceStore.h`/runtime edit.** The store is used as-is.
- **No golden churn.** All 190 stdout goldens (`runall.sh` count) × 4 modes and the 24 dump/codegen goldens stay byte-identical — R6 emits no code (same exit gate as H-A4).
- **No cost-currency change.** R6's boolean matrix feeds `bin/Cost`'s STRUCTURAL inputs (which keys activate = CostModel §3.2 `Prov` sparsity, §1.2 `D_a`); the counting/tropical currencies of §1.1 are untouched.

---

### 5. Falsifiable predictions

- **P1 (linear TC prunes).** `demand_tc_witness` (right-linear `path(F,T):path(F,M),edge_2(M,T)`, `#query reachable_from(bound From, free To)`, `.drflags = -demand`): under `-demand` the `path` relation materializes exactly `reachable-from-F`, NOT the full closure — `|materialized| = D·|reach(F)| ≪ Z_S`. **Testable now**: the `demand_tc_witness.batches` derivation counters expose the materialized `path` row count per probed seed; the oracle already referees answer-identity. Refuted if `-demand` materializes the full closure.
- **P2 (non-linear TC activates the reachable sub-relation).** A single-bound non-linear `tc(F,T):tc(F,X),tc(X,T)` demanded on `tc(bound F,·)` would fill the demanded-source set with all of `reach(F)` (`|activated| = Θ(|reach|²)`), with the InstanceStore memo holding it quadratic. **NOT testable today** — single-bound non-linear TC hits the R.1.1 sideways/left-linear reject (:869-870), and the two-bound non-linear `transitive_closure_diff.dr` hits the C14 multi-query fence. STOP on DIFF-R2 (positive witness unconstructible until the pivot-split lift). The *negative* half IS testable now: see below.
- **P3 (memo, not exponential).** Once DIFF-R2 lands, `-demand-instance` on a non-linear tc must satisfy CostModel §6.2 `GATE(recursive-demand r0)`: `D·Z_key < Z_S^d`, where the non-linear `Z_key` is the interior sub-relation — i.e. no lowering lands unless the memo's once-per-key activation is priced and observed. Refuted if per-key rescans over-materialize (the p1/HP-5 shape).

---

### SOUNDNESS OBLIGATIONS

- **O-1 (semiring layering).** R6's boolean matrix `(𝔹,∨,∧)` is DISTINCT from CostModel §1.1's cost semirings; a boolean entry means "activated," and the *count* of activated entries is the §1.1 counting quantity. `bin/Cost` must never read `M` as a cardinality. Belt: assert the matrix frame is consumed only at the L1 `Prov`/sparsity boundary (§3.2), never as a `K`-cost, at the `V-COST-XCHECK` (§3.4) seam.
- **O-2 (cyclicity is a runtime fact — over-estimate is safe).** Data-graph cyclicity (hence linear-vs-non-linear-in-effect) is not known at compile time (running-example §matrix "Compile catch"). The discriminator must assume the **coupled / quadratic** branch UNLESS linearity is *structurally* provable (every recursive atom threads the bracket key unchanged — a syntactic check). Over-costing (assume non-linear) is the sound direction, mirroring §1.4 defaults / §8.1 `Prov` under-approximation.
- **O-3 (memo ⇔ quadratic bound must agree with the cost per-key term).** The claim "memo collapses exponential to quadratic" is discharged only if CostModel §2.7's per-key `D_a` counts each interior activation ONCE (matching `FindOrAddInstance`), not once per path. Cost-per-path over-counts; a runtime that fails to memo explodes — the SAME bound, and §6.2's GATE is where they must reconcile.
- **O-4 (`M·M` ≠ closure).** Costing one matrix square for a TC under-costs; the closure is the `R_S`-round lfp (R.1.3 / §3.3). The frame must map TC to `M⁺`, not a single product.
- **O-5 (no behavior change).** R6 adds no IR/lowering; the 190 stdout + 24 dump/codegen goldens stay byte-identical. Referee: `runall.sh` `SUITE: PASS`.

---

### OWNER DEPENDENCIES

- **D5 (storage / arrangement sharing) — OPEN.** R6 supplies "row-wise store = arrangement = trie by K" as D5's planning frame; the *physical* bracket/arrangement order (logical/physical key split, owner refinement 2026-08-03: `rel[A,C] == rel[C,A]` logically, distinct arrangements physically) is D5's share-vs-fragment knob. R6 does not decide it.
- **D2.12 / V-CW — RATIFIED-with-sharpening (owner 2026-08-03, X=F/X≠F).** Cyclic-sparsity = coupled-row-fixpoint IS the V-CW coupled case; R6's discriminator is the cost read of the same pivot split. The disjoint-union-vs-coupled correctness twin (A-corr-4) is the non-linear witness's obligation.
- **D2.6 (edge row schema / reader-handle) — OPEN.** The matrix-row = reader-handle identification (`[[shared-arrangements-mapping]]`) is named; the handle/lease decision defers to D2.6.
- **DIFF-R2 (activation-SCC / pivot split) — STOP for P2.** The non-linear positive witness is unconstructible until DIFF-R2 lifts the R.1.1 sideways/left-linear fence. R6 states the prediction; DIFF-R2 makes it realizable.
- **Cost-scenario-family ruling (owner 2026-07-30) — RATIFIED.** The `.cost` family (CostModel §4) is the discriminator's hook.
- **request-edge notation `rel[Bound](Free)` (owner proposal 2026-08-03) — NOT ratified.** Used only to name the matrix row index / trie prefix; the correspondence holds without adopting the surface (cross-ref, not a blocker).

---

### TEST / WITNESS PLAN (golden-master terms)

- **P1 — EXISTING corpus, new sidecar.** `demand_tc_witness` (linear, `-demand`). Referees: (1) the landed `.batches` oracle (answer-identity, unchanged); (2) a NEW `demand_tc_witness.cost` sidecar — this IS the "recursive tc witness" CostModel §6.1 slice-1 already calls for, SPECIALIZED to pin the *linear-pruning* number: a `deep_dense_closure` scenario asserting `cost[-demand] < cost[normal]` (build) with `Z_S^demand = D·|reach|`, and a `shallow_sparse` scenario asserting demand loses (§5.4 message-rooted). Golden: `demand_tc_witness.cost.stdout` via `bin/Cost`, byte-compare, `runall.sh --bless` after review.
- **P2 negative — NEW directed diagnostic witness.** `demand_nonlinear_tc_1.dr`: `tc(F,T):tc(F,X),tc(X,T)` + `#query q(bound F, free T)`, `.drflags = -demand`. Predicts the R.1.1 sideways/left-linear clean diagnostic (:869-870) — confirming the cost model's "non-linear demand does not prune cleanly / is fenced." Encoded in `runall.sh` as a `-demand`-gated diagnostic case. This is the honest *testable-today* half of P2.
- **P3 / §6.2 GATE — pinned, not live.** When DIFF-R2 lands, the non-linear tc `.cost` under `-demand-instance` must pass `D·Z_key < Z_S^d`; R6 records the GATE as the acceptance test the memo-quadratic claim owes. No witness until then.
- **Covering-array referee (owner adornment-fuzzing direction, 2026-08-03 session 3).** R6 contributes the **linear-vs-non-linear recursion shape** as an axis to the DIFF-R3 bracket-placement generator; the oracle is I0 / eqgate answer-identity (flat == declared == nested) plus diagnostic-class compare (PF-3) — every generated placement either compiles answer-identical or draws a clean diagnostic, never a miscompile.
- **Belt.** `V-COST-XCHECK` (§3.4) unchanged; add the O-1 scope assertion (boolean matrix never read as cardinality) at the `bin/Cost` L1/L2 boundary.


### OPEN OWNER ITEMS (DIFF-R6)

- D5 (storage / arrangement sharing) — OPEN: R6 supplies the row-wise-store = arrangement = trie frame as D5's planning input but does not decide the physical bracket/arrangement order (the logical/physical key split, owner refinement 2026-08-03); D5 owns share-vs-fragment.
- D2.6 (edge row schema / reader-handle) — OPEN: the matrix-row = reader-handle identification is named but the handle/lease decision defers to D2.6.
- DIFF-R2 (activation-SCC / pivot split) — STOP: the non-linear-demand positive witness (P2, demanded-source-set == answer-sink-set, one activation per interior pivot) is UNCONSTRUCTIBLE until DIFF-R2 lifts the R.1.1 sideways/left-linear fence (:869-870); only the NEGATIVE witness (single-bound non-linear TC rejects under -demand) is testable today.
- request-edge notation rel[Bound](Free) (owner proposal 2026-08-03) — NOT ratified: R6 uses the bracket only as the matrix-row-index name; the correspondence holds without adopting the surface, so it is a cross-ref, not a blocker.
- §6.2 GATE(recursive-demand r0): the memo-collapses-exponential-to-quadratic claim becomes a live cost witness only once -demand-instance costs a non-linear tc shape; until DIFF-R2, R6 feeds the GATE, it does not discharge it.

### PANEL RECORD (DIFF-R6) — 13 findings, 12 survived refutation

- **DIFF-R6-correctness-lifecycle-1** [CONFIRMED; sev MAJOR] H-R6.2 pins the Kleene-star / R_S-round reading onto LowerDRRounds (Stratum.cpp:1799), but that function emits the DIFFERENTIAL OVERDELETE/REDERIVE/INSERT round shells, not the monotone semi-naive fixpoint that actually computes M+ for the linear-TC closure the discriminator prices. For the diff's own P1 witness (demand_tc_witness under bare -demand, monotone) LowerDRRounds emits ZERO rounds, and the closure is iterated in Stage B's induction.
  - EVIDENCE: Part-R R.1.3 (regional-arch-pseudocode.md:1145-1169) pins LowerDRRounds (Stratum.cpp:1799) to the DIFFERENTIAL '(OVERDELETE, INSERT) round-shell pair whose drain_stratum == this stratum', and drain_stratum is populated ONLY 'for every differential table NOT induction-owned (Rel.cpp:3119: TableIsDifferential && !TableIsInductionOwnedDR)' (:1129-1131). The monotone closure iteration is Stage B's GetOrInitInduction/BuildFixpointLoop (Induction.cpp:625/138), which 'iterates to a monotone fixpoint ... NEVER does OVERDELETE/REDERIVE/INSERT -- monotone-only' (:1114-1122). The DRIFT LEDGER D9 (:1667-1676) records this EXACT misattribution as a fleet correction: 'that describes the SEPARATE, earlier eager-descent induction ... the seed's 1.3 MIS-attributed to LowerDRRounds'. The P1 witness demand_tc_witness (.drflags = -demand, no @differential/negation, monotone) is induction-owned, so LowerDRRounds emits zero rounds for it; H-R6.2's unqualified 'this lfp IS M+ (Kleene star); R_S rounds = the star iteration' on the :1799 anchor re-commits D9.
  - AMENDMENT (normative): Split H-R6.2's first hunk: attach 'M+ / R_S rounds = the star iteration' to Stage B's BuildFixpointLoop/GetOrInitInduction (Induction.cpp:138/625) for the MONOTONE induction-owned closure the P1 witness exercises, and re-label the LowerDRRounds (Stratum.cpp:1799) annotation as the DIFFERENTIAL maintenance of M+ (per-epoch OVERDELETE/REDERIVE/INSERT, non-induction-owned differential tables only), consistent with R.1.3's DIVISION OF LABOR text and DRIFT-LEDGER D9. §3.3 Z_S/R_S is read off whichever loop owns the SCC (the induction-region test), not unconditionally off LowerDRRounds.
- **DIFF-R6-correctness-lifecycle-2** [CONFIRMED; sev MINOR] The anchor Demand.cpp:869-870 cited for 'the R.1.1 sideways/left-linear reject' / 'the SIP From-preservation check' is wrong; :869-870 is the demand__ name-prefix comment inside Step 5 (message fabrication), not the sideways reject.
  - EVIDENCE: grep of lib/DataFlow/Demand.cpp: the two 'Sideways (non-From-preserving) demand propagation is not yet supported' rejects are at :693 and :748 (matching Part-R R.1.1's own ':693, :748' anchors). Line 867-870 is 'The reserved name prefix (G3) ... demand__ is the lexable reserved prefix' — Step 5 message fabrication, ~120-170 lines from the reject. The wrong :869-870 anchor recurs in P2, H-R6.3's selector line, and TEST/WITNESS P2-negative.
  - AMENDMENT (normative): Replace every 'R.1.1 ... :869-870' occurrence (P2, H-R6.3, TEST/WITNESS P2-negative) with the actual sideways/From-preservation reject anchors Demand.cpp:693 and :748 (Part-R R.1.1 Step 3).
- **DIFF-R6-correctness-lifecycle-3** [SOFTENED; sev MINOR] H-R6.1 asserts the landed InstanceStore 'IS an arrangement = a trie by K' ('R6 names the landed object, does not add one'), but the landed store is hash-keyed with no key order, so no trie / sorted-prefix arrangement exists yet -- the trie is a D5 TARGET, not the landed object.
  - EVIDENCE: include/drlojekyll/Runtime/InstanceStore.h: FindInstance routes through FindInstanceWithHash(key, key.Hash()) over open-addressing slots[]/InsertSlot/Rehash; the header states 'NESTED TABLES ARE INDEX-FREE (A.3.1): membership is Table::Find / TryAdd by WHOLE row' — a hash map with NO key order, so no trie/sorted-prefix arrangement is landed. But R6 is faithful to the owner's own framing (running-example-disassembler.md:203-208 'the InstanceStore is M stored ROW-WISE = an arrangement/index keyed by F = a TRIE by F') AND R6's OWNER DEPENDENCIES explicitly defer the physical bracket/arrangement order to D5. So 'internally contradictory' overshoots — it is an imprecise conflation in H-R6.1 of the landed LOGICAL row-memo with the physical trie that D5 must still add, not a self-contradiction.
  - AMENDMENT (normative): Rephrase H-R6.1: the LANDED object is the row-wise MEMO (rows = keys), hash-indexed with NO key order; 'arrangement = trie by K (sorted prefix)' is the D5 TARGET ordering the current store lacks. Keep 'NO storage change' for the memo/matrix-row correspondence only, not for the trie/arrangement structure.
- **DIFF-R6-termination-confluence-1** [SOFTENED; sev MAJOR] The §2 discriminator and its closing biconditional ("non-linear ⟺ cyclic/pivot-generating ⟺ coupled-fixpoint"; O-2's contrapositive) equate a SYNTACTIC property (rule linearity) with a RUNTIME property (activation-graph cyclicity), asserting that every pivot-generating recursion is a coupled row-fixpoint priced at Θ(reach²). This overstates: a non-linear rule over acyclic data has NO fixpoint at all — it is terminating memoized descent.
  - EVIDENCE: running-example-disassembler.md:162-164 ('Acyclic instance graph -> X!=F always -> pure recursive DESCENT ... no same-key fixpoint; cyclic -> coupled same-key fixpoint') and :213-215 ('data-graph cyclicity is a RUNTIME fact, so emit coupled-fixpoint code (V-CW) unless acyclicity is provable; the DAG case degenerates to descent as a cost-model optimization') confirm the syntactic non-linear shape does NOT imply a coupled fixpoint; acyclic-data non-linear is terminating descent. R6's §2-closing '⟺' overstates. HOWEVER: R6's §1 table correctly splits cyclic→coupled / acyclic→descent, and O-2 already mandates the conservative over-approximation. Since over-costing (assume coupled) is the SOUND cost direction (the disassembler itself calls the DAG degeneration a cost 'optimization', not a correctness requirement), this is an exposition imprecision in one sentence, not the soundness bug 'assume a fixpoint exists ... unsound' the MAJOR framing implies.
  - AMENDMENT (normative): Reword the §2 non-linear branch and the §2-closing sentence from '⟺' to a conservative one-directional '⟹' (rule non-linearity OVER-approximates runtime activation-graph cyclicity); inline O-2's hedge into the discriminator statement (cost assumes the coupled/quadratic branch UNLESS acyclicity/linearity is structurally provable); and note the acyclic-data non-linear case degenerates to terminating descent — a cost-model REFINEMENT opportunity, not an unsoundness.
- **DIFF-R6-termination-confluence-2** [SOFTENED; sev MAJOR] The headline finiteness argument for the non-linear case — "the InstanceStore memo (one activation per interior key) is what bounds this at QUADRATIC rather than EXPONENTIAL" (§2, O-3, P3) — is incomplete as a termination/finiteness argument. The memo bounds RE-activation (≤ once per key), but the Θ(reach²) / termination conclusion additionally requires the KEY UNIVERSE to be finite, which R6 never states as a premise.
  - EVIDENCE: The memo (InstanceStore.h:109 FindOrAddInstance, dense append-only iid space) gives 'each key activated at most once' — genuinely only the re-activation half of a finiteness argument; the Θ(reach²)/termination conclusion also needs a finite interior-key universe, which R6 states unconditionally ('for a demanded recursive relation rel under adornment a') and never premises. running-example-disassembler.md:136-140 relies on the same silent assumption. BUT for the demand fragment the interior pivot is range-restricted to the base active domain (Datalog range-restriction; the constructible TC witnesses have X over nodes), and a functor-value-generating interior pivot is a broader decidability/termination concern outside R6's cost claim — so 'unbounded / non-terminating, unsound' overshoots the practical fragment. The right fix is to STATE the missing premise, not to treat every non-linear body as possibly non-terminating.
  - AMENDMENT (normative): Add to O-3 and the §2 memo-quadratic claim the explicit premise: the Θ(reach²) bound holds only when the interior pivot is range-restricted to the finite base active domain (as for any column-sourced pivot, e.g. TC's X). State that the memo supplies the 'once-per-key' half and range-restriction the 'finitely-many-keys' half; a functor-value-generating interior pivot escapes the bound and falls to the conservative branch (O-2's over-cost-is-safe direction).
- **DIFF-R6-testability-oracle-1** [CONFIRMED; sev MAJOR] P1 is tagged "Testable now" and the TEST/WITNESS PLAN specifies "Golden: demand_tc_witness.cost.stdout via bin/Cost, byte-compare, runall.sh --bless" and O-1's belt names a V-COST-XCHECK (§3.4) seam — but the entire cost referee is a named-but-nonexistent oracle.
  - EVIDENCE: bin/CMakeLists.txt declares only drlojekyll, drlojekyll-oracle, drlojekyll-refinterp, drlojekyll-refharness — NO drlojekyll-cost target. `find tests -name '*.cost'` and `*.cost.stdout` return empty; `grep -i cost tests/OptDiff/runall.sh` returns empty. CostModel.md is a 46KB PROPOSAL that lists bin/Cost as a to-build deliverable ('1. bin/Cost (target drlojekyll-cost)', :668) and defines V-COST-XCHECK (§3.4:404) only on paper. P1's golden 'demand_tc_witness.cost.stdout via bin/Cost' and O-1's 'V-COST-XCHECK (§3.4) unchanged' belt both presuppose a harness absent from the tree.
  - AMENDMENT (normative): Retag P1's cost half as 'Testable AFTER bin/Cost + the .cost referee land'; list 'build bin/Cost (drlojekyll-cost) + wire a .cost referee into runall.sh + implement the V-COST-XCHECK seam' as an explicit PREREQUISITE deliverable of R6 (or CostModel §6.1 slice-1). Mark O-1's V-COST-XCHECK reference as to-be-built, not 'unchanged'.
- **DIFF-R6-testability-oracle-2** [CONFIRMED; sev MAJOR] P1's flagship falsifiable prediction — under `-demand` the `path` relation materializes exactly `reachable-from-F` (`|materialized| = D·|reach| ≪ Z_S`), "Refuted if -demand materializes the full closure" — has NO landed observer, and the specific claim that "the `demand_tc_witness.batches` derivation counters expose the materialized `path` row count per probed seed" is false.
  - EVIDENCE: tests/OptDiff/cases/demand_tc_witness.batches header: 'The oracle evaluates the plain (undemanded) program, so its reachable_from rows are the FULL closure -- the answer-identity referee for the demand-ON driver's per-key answers.' The oracle golden (demand_tc_witness.oracle.stdout) emits reachable_from ANSWER rows, not internal path materialization counts. runall.sh:82 states 'demand must change materialization, never answers'. No landed referee (oracle / monotone projection / RefInterp) observes the demanded internal path row count, so P1's refutation condition ('-demand materializes the full closure') is unobservable today; the specific claim that '.batches derivation counters expose the materialized path row count per probed seed' is false.
  - AMENDMENT (normative): Either (a) build an internal-materialization observer (a bin/Cost structural read of the demanded relation's row count, or a RefInterp per-relation materialized-row-count instrumentation under -demand) and cite THAT as P1's referee; or (b) demote P1's discriminating half to 'prediction pending a materialization-observing oracle' and delete the false claim that .batches derivation counters already expose the per-seed materialized path count (they referee answer-identity of the undemanded program).
- **DIFF-R6-testability-oracle-3** [SOFTENED; sev MAJOR] The P2-negative witness `demand_nonlinear_tc_1.dr` (single-bound non-linear `tc(F,T):tc(F,X),tc(X,T)`, `#query q(bound F, free T)`, `-demand`) is predicted to draw "the R.1.1 sideways/left-linear clean diagnostic (:869-870)" — but empirically it trips a DIFFERENT, earlier reject, so the witness does not discriminate the claimed From-preservation mechanism.
  - EVIDENCE: I built the single-bound non-linear witness (tc(F,T):edge_2(F,T); tc(F,T):tc(F,X),tc(X,T); #query reachable_from(bound From, free To):tc(From,To)) and compiled it with build/debug/bin/drlojekyll -demand. It rejects with 'A rule body reading its own relation more than once (a self-join) is not yet supported under -demand' — the SELF-JOIN fence at Demand.cpp:657-659 (p_reads>1), NOT the :693/:748 sideways/From-preservation reject the diff predicts. So the finding's CORE claim (the witness does NOT discriminate the From-preservation mechanism; it is caught upstream) is CONFIRMED. But the finding's own amendment misidentifies the upstream reject as the generic 'Unsupported rule-body shape' body-walk (:613ff, 'same belt as demand_recursive_content_1') — empirically it is the distinct self-join reject at :657-659.
  - AMENDMENT (normative): Correct the P2-negative prediction to the actual reject: the single-bound non-linear tc trips the SELF-JOIN fence 'A rule body reading its own relation more than once (a self-join) is not yet supported under -demand' (Demand.cpp:657-659, the p_reads>1 belt) — an UPSTREAM fence, not the :693/:748 sideways/From-preservation reject. If R6 wants to witness the From-preservation fence specifically, use a body with a SINGLE recursive atom re-keyed off a non-head-bracket var (e.g. p(F,T):edge(F,X),p(X,T) demanded on bound F) that survives the self-join belt and reaches the :693/:748 position check.
- **DIFF-R6-testability-oracle-4** [CONFIRMED; sev MINOR] O-5 / "What R6 does NOT change" asserts "190 stdout goldens (runall.sh count) × 4 modes" — the referee accounting conflates the case count with the stdout-golden count.
  - EVIDENCE: In tests/OptDiff/goldens: 169 stdout goldens (excluding oracle/monotone), 63 oracle, 63 monotone, 24 *.opt.golden; tests/OptDiff/cases has 190 *.dr files. So '190' is the CASE count, not the stdout-golden count (169). The 24 dump/codegen figure is correct. The no-churn guarantee itself holds (R6 emits no code), so this is cosmetic accounting only.
  - AMENDMENT (normative): Restate O-5 / 'What R6 does NOT change' as '190 corpus cases (169 stdout + 63 oracle + 63 monotone goldens + 24 dump/codegen), all byte-identical; referee: runall.sh SUITE: PASS' rather than '190 stdout goldens'.
- **DIFF-R6-necessity-1** [SOFTENED; sev MAJOR] The §2 'demand-effectiveness DISCRIMINATOR' — a syntactic linear/non-linear body predicate that selects the demanded closure's symbolic shape — duplicates an authority CostModel already owns. §2.6 (CostModel.md:281-290) already prices linear recursion as `Σδ·(1+f) ≈ Z_S·(1+f)` and non-linear as `Σδ·|accum|` super-linear, and §3.3 (:385) explicitly states 'The per-round work uses §2.6's actual-cards rule (linear vs non-linear falls out)'. The split is therefore already a structural determination in the model. DIFF-R6 re-derives the same split as a separate 'syntactic body predicate (bracket-arg identity, DIFF-R2 / the SIP From-preservation check R.1.1:869-870)' — its own §2 admits the predicate is DIFF-R2's / R.1.1's existing classification, not a new one.
  - EVIDENCE: CostModel.md §2.6:287-290 already prices linear vs non-linear from the join's actual feeding cards ('reports whichever the join's two feeding cards produce; it does not silently linearize') and §3.3:385 defers to it ('The per-round work uses §2.6's actual-cards rule (linear vs non-linear falls out)'). So the linear/non-linear CLASSIFICATION is an existing authority, and R6's §2 admits its predicate IS DIFF-R2/R.1.1's existing check. BUT R6 applies that shared classification to a DISTINCT question §2.6 does not decide — demand EFFECTIVENESS (does demand prune? which Z_S^demand and §5.4 branch?) — and R6 already cites §2.6/§3.3 as the source. The 'two peer authorities for one fact' framing overshoots: the classifier is shared, the application is new. R6 also genuinely adds two facts (non-linear ⇒ demanded-set==answer-set; memo ⇒ quadratic).
  - AMENDMENT (normative): Do not present the discriminator as a peer classifier. State that §2.6/§3.3's actual-cards rule is the SOLE authority for linear/non-linear determination and that R6's bracket-arg-identity check is the card-free STRUCTURAL selector §2.6 already implies (both-feeding-sides-growing ⇔ non-linear); on divergence §2.6's card rule wins. Keep the discriminator only as the DEMAND-PRUNING application plus R6's two net-new facts appended to §2.6/§6.2.
- **DIFF-R6-necessity-2** [SOFTENED; sev MAJOR] DIFF-R6 coins new cost symbols `Z_S^demand` and `Z_reachsub` that duplicate symbols already in CostModel and are not registered in the §1.2 symbol basis. §6.2 (CostModel.md:704-706) already names the flat-demand closure cost `Z_S^d` ('flat demand costs Z_S^d once') and the per-key interior sub-relation `Z_key`. DIFF-R6's §2 introduces `Z_S^demand` (linear = D·|reach|, non-linear = D·Z_reachsub) for the flat-demand closure, then its own P3 turns around and uses `Z_S^d` and `Z_key` for the same quantities.
  - EVIDENCE: CostModel.md §6.2:705 already names the flat-demand closure cost Z_S^d ('flat demand costs Z_S^d once') and the per-key interior sub-relation Z_key ('costs D · Z_key ... flags it when D · Z_key > Z_S^d'). DIFF-R6 §2 introduces Z_S^demand and Z_reachsub for the same objects, then its own P3 reverts to Z_S^d and Z_key — a verifiable duplicate-symbol + mid-document spelling switch. §1.2 is the declared symbol basis. The concern is real but it is a naming-hygiene defect (clean rename), not the MAJOR modeling error the severity implies.
  - AMENDMENT (normative): Reuse Z_S^d for the flat demanded closure and Z_key for the interior sub-relation; delete Z_S^demand and Z_reachsub. Express the discriminator results as constraints on the existing symbols (linear ⇒ Z_S^d = D·|reach|; non-linear ⇒ Z_S^d = D·Z_key, Z_key = Θ(|reach|)) and register any residual (|reach|) in §1.2.
- **DIFF-R6-necessity-4** [CONFIRMED; sev MINOR] The H-R6.2 annotation asserts 'Linear recursion (M v M.E) costs Z_S per round; non-linear (M v M.M) costs the §2.6 super-linear rule.' The 'Z_S per round' figure contradicts the very §2.6 it cites: §2.6 (CostModel.md:284-285) gives linear per-round work as `δ_{r-1}·(1+f)` with `Σ_r δ = Z_S` (Z_S is the TOTAL across rounds, i.e. §3.3:384 'frontiers partition the closure'), not a per-round quantity.
  - EVIDENCE: H-R6.2 states 'Linear recursion (M∨M·E) costs Z_S per round'. CostModel.md §2.6:284-285 gives linear per-round work as 'Σ_r δ_{r-1}·(1+f) ≈ Z_S·(1+f) — once per closure row', and §3.3:384 states 'Σ_r δ_r = Z_S (frontiers partition the closure)' — Z_S is the TOTAL across all rounds, not a per-round quantity. 'Z_S per round' misstates the figure by a factor of R_S and would mislead a bin/Cost implementer wiring §2.6 at the LowerDRRounds anchor the hunk sits on.
  - AMENDMENT (normative): Change the H-R6.2 comment to: 'linear recursion (M∨M·E) costs δ_{r-1}·(1+f) per round, Σ = Z_S·(1+f) total; non-linear (M∨M·M) costs §2.6's Σ_r δ_{r-1}·|accum_{r-1}|' — quoting §2.6's exact per-round/total split rather than 'Z_S per round'.
- Refuted: DIFF-R6-necessity-3
