# Stage D as diffs — deepen the forest, qualify local recursion

Diff target: `regional-arch-pseudocode.md` (verified 2026-08-02 at tip
f0c913e0) as amended by the Stage A / I0 / Stage B / Stage C hunks of its
§6. Normative target: `RegionalDataFlowCore.md` §13 Stage D (sequence),
§7.1 (termination), §8/§8.1 (planning/admissibility), §9 (Rel lowering),
§11 (validators), §12 (testing), §14 (deletions), §15 (acceptance).
Standing review input: `fable-review-2026-08-01.md`.

**Precondition (what Stage D inherits from Stage C).** By Stage C:
`ApplyDemandTransform`, `GuardAnnotation`/`RecognizedSubgraph`/
`QueryDemandForcing`, `BuildSubgraphInstanceOps`, the `kSubgraphInstantiate`/
`kInstanceDeath`/`kInstanceSeal` op family, `InstanceStore`-as-semantic-model,
the a0/a1/a2/a2'/b bands, and the three demand flags are GONE. Demand is now
`RequestEdgeRelation` + `RoutedResultRelation` + `RootRequestLease`; extraction
is `ExtractPureChild` via `FirstStableAdmissibleChild`, **one level only**; the
lifecycle vocabulary `request_edge_add/remove … local_fixpoint … seal_epoch`
(proposal §9 `RelRegionLifecycle`) exists and is lowered through Rel. At Stage C
a slice whose body carries a relation SCC (induction-owned content) is
**inadmissible** and therefore stays in the observation root, fully materialized
by the ordinary global Rel strata (answer-correct; matches today's flag-off
recursion). Stage D is a diff **on the Stage-C tree**, not on the demand tree.

Because Stage D is the terminal stage, **every deletion hunk below names a
replacement that lands inside Stage D itself** (nothing may defer past the last
stage). Where that is impossible the item is escalated, not shipped.

---

## Part 0 — the load-bearing reconciliation (the Phase-1 adjudication finding)

The charge's central obligation: today keyed instances run OUTSIDE all strata
and are never inductive (pseudocode §4b Step 6 — `LowerSubgraphInstances` runs
ONCE at the epoch tail, `flow.instance_stratum[sid]` only orders the single
instance op after its input/demand drains; there is no per-instance fixpoint),
and two fences forbid induction-owned demanded content. Stage D's "induction
machinery PER INSTANCE" is the direct inverse. This part is the audit; Parts 1-6
are the hunks.

### 0.1 Fence audit — each fence, where it dies, what replaces it

Both fences live in ONE block, `Build.cpp:1428-1476` (pseudocode §1
`Program::Build` `:1467-1473`). That block reads `query.GuardAnnotations()`,
which **Stage C deletes**. So the literal block is gone before Stage D opens;
Stage D must account for the two SEMANTIC restrictions the block encoded, each of
which migrated to a different place at Stage C:

| Fence (today) | Build.cpp clause | What it forbids | Stage C fate | Stage D fate |
| --- | --- | --- | --- | --- |
| **cyclic_demand** | `:1463-1465` + reject `:1467-1469` (`ViewSelfReachable(jl[0])`; `demand_cyclic_1`) | A demand relation that is recursive — i.e. a **region-request cycle** | Block deleted with GuardAnnotations; the restriction re-expressed as proposal §2.2 "Recursive or key-changing region calls" excluded + `ValidatePlanningProgram`'s "cycle in the region ownership/call graph" validator (§11) | **STAYS EXCLUDED — permanently.** Recursive region calls are out of this architecture entirely (proposal §2.2, §6 excludes "any future recursive request architecture"). Not lifted; escalated as a permanent boundary (§Escalations E1). |
| **recursive_content** (ADJ-C2) | `:1452-1462` + reject `:1470-1473` (`kBody` guard's input is `InductionGroupId().has_value()` or `ViewSelfReachable` or has an induction-owned predecessor; `demand_recursive_content_1`) | A demanded body whose **content** (not its request edge) is induction-owned — a local relation SCC inside the demanded subgraph | Block deleted; restriction re-expressed as a Stage-C **admissibility** exclusion (§8.1 "local stratification and recursion remain valid" reads, at Stage C, as "the slice has NO local recursion"); inadmissible ⇒ full materialization in observation root | **LIFTED at Stage D** — this is Stage D item 4 ("existing relation recursion entirely inside a region"). The admissibility predicate is widened to ADMIT local-recursion slices; the replacement is the per-`(RegionId, InstanceId)` qualified fixpoint of Parts 3-4. |

There is also a THIRD, plain-`-demand` body-walk reject
(`demand_recursive_content_1`'s upstream catch, CLAUDE.md) — same semantic
class as recursive_content; same disposition (lifted at Stage D).

**Deletion-replacement discipline check.** The only Stage-D deletion of a
*restriction* is the recursive_content admissibility exclusion, and its
replacement (per-instance fixpoint lowering, Parts 3-4) lands in the same stage.
PASS. cyclic_demand is NOT deleted — it is promoted from a codegen-time reject to
a planning-time validator that stays live forever.

### 0.2 Why the exclusion was safe to defer to Stage D (not earlier)

At Stage C, the recursion-bearing query still compiles: inadmissible extraction ⇒
full materialization (Concern 2's OWNER-GATED decision, resolved in the Stage-C
hunk). So a recursive-content bound query is answer-correct at Stage C by
materializing globally. Stage D is a pure *capability add* (extract it into a
child, evaluate its SCC per instance) with an answer-equivalence obligation to
the Stage-C full-materialization behavior. That equivalence is the exit gate.

---

## Part 1 — nested deterministic child extraction (diff on §6 Stage C planner)

Stage C introduced (pseudocode §6 Stage C):

```text
+ planner: ExtractPureChild via FirstStableAdmissibleChild
  (proposal §8; ONE level in this stage)
  + ExtractionPolicy(candidate) -> bool as a NAMED SEAM
```

### Hunk 1.1 — recursion loop, not one level (anchor: proposal §8 `BuildPlanningRegionalProgram`)

```diff
 BuildPlanningRegionalProgram(logical):
     contracts = InferConservativeRowContracts(logical)
     planning  = BuildProgramRootAndObservationRoots(logical, contracts)
     for region in stable ownership order:
         OptimizeLocalGraphToFixpoint(region)
         SolveBackwardRequirements(region)
-        # STAGE C: one level. `FirstStableAdmissibleChild` returns at
-        # most one candidate; no re-descent into the extracted child.
+        # STAGE D: full descent. The worklist re-enqueues each freshly
+        # extracted child so its own admissible children extract too.
         while candidate = FirstStableAdmissibleChild(region):
             child = ExtractPureChild(region, candidate)
             OptimizeLocalGraphToFixpoint(child)
             SolveBackwardRequirements(child)
             ReplaceSliceWithOpenChildCall(region, child)
             ReoptimizeParent(region)
+            worklist.push(child)          # NEW: descend into the child
     SolvePortsInOwnershipPostorder(planning)   # Part 2
     ValidatePlanningProgram(planning)
     return FreezeAndValidate(planning)
```

Implementer note: "stable ownership order" must remain a total, deterministic
order as the forest deepens. Order children by `(parent RegionId, extraction
sequence index)` — the extraction sequence is the deterministic order
`FirstStableAdmissibleChild` visits candidates, itself keyed on a DefList VIEW
WALK (HP-9 discipline: never on pointer-derived UniqueId, never on vector append
order). This is the determinism witness obligation (Part 6, `symrec_tie_1`
analogue).

### Hunk 1.2 — admissibility widened to admit local recursion (anchor: proposal §8.1)

The recursive_content fence's Stage-D lift lives here:

```diff
 A child slice is admissible only when:
     all operators in the slice are pure
     the callee is a newly owned direct child
     all member distinctions have a realizable local or InstancePath-qualified key
     every request and result removal has an exact identity
     every crossing input has a differential maintenance strategy
     sealed declared ABIs remain in ProgramRoot and unchanged
     no region parameter escapes except through an explicit port mapping
-    local stratification and recursion remain valid
-      # STAGE C READING: "the slice contains NO relation SCC" (a recursive
-      # slice is inadmissible; falls back to full materialization).
+    local stratification remains valid
+    AND every local relation SCC in the slice is INSTANCE-CLOSED:
+      # STAGE D: a relation SCC MAY be inside the slice iff
+      #   (a) the whole SCC is inside this one slice (no back-edge crosses a
+      #       child-call boundary — that would be a region-request cycle,
+      #       the cyclic_demand exclusion of §0.1, forbidden forever); AND
+      #   (b) no InstanceKey/InstancePath column of this region appears on a
+      #       recursive back-edge of the SCC (the key is BOUND per instance,
+      #       never DERIVED by the recursion — the termination premise §4.2).
```

`InstanceClosedSCC(slice)` is a NEW planner predicate (Part 5, checked again as a
frozen validator V-SCC-INSTANCE-CLOSED). (a) is the direct inverse of the
recursive_content fence; (b) is the new obligation the fence never had to state
because keyed instances were never inductive.

---

## Part 2 — ports solved in ownership postorder, parents reoptimized (diff on §6 Stage C)

Stage C already calls `SolvePortsInOwnershipPostorder` (proposal §8) and
`ReoptimizeParent` after each single extraction. Stage D makes both operate over a
DEPTH ≥ 2 forest.

### Hunk 2.1 — postorder over the deep forest (anchor: proposal §8 `SolvePortsInOwnershipPostorder`)

```diff
 SolvePortsInOwnershipPostorder(planning):
-    # STAGE C: at most one child under each observation root; postorder
-    # is a two-node visit (child then parent).
+    # STAGE D: full postorder DFS of the ownership forest. Because calls
+    # are acyclic and every call targets a DIRECT child (proposal §3.2),
+    # postorder is well-defined and each child's ports are frozen before
+    # its parent's ports are solved. No recursive port-schema fixpoint
+    # (proposal §2.2 excludes recursive port-schema SCCs).
     for region in planning.regions.ownership_postorder():
         SolveOpenPorts(region)          # child ports already frozen below it
```

### Hunk 2.2 — parent reoptimization AFTER child freeze (anchor: proposal §8 loop body)

Stage C's `ReoptimizeParent(region)` runs after a single extraction while the
child is still OPEN. Stage D must additionally reoptimize the parent after the
child is FROZEN (the child's solved ports change what the parent can prove about
the port boundary). This is proposal §13 Stage D item 2 ("reoptimize parents
after child freeze").

```diff
     while candidate = FirstStableAdmissibleChild(region):
         child = ExtractPureChild(region, candidate)
         OptimizeLocalGraphToFixpoint(child)
         SolveBackwardRequirements(child)
         ReplaceSliceWithOpenChildCall(region, child)
         ReoptimizeParent(region)                  # pre-freeze (Stage C)
         worklist.push(child)
+    # STAGE D: after this region and all its descendants freeze in
+    # postorder (Hunk 2.1), a FINAL parent reoptimization pass runs, so a
+    # parent sees each child as an opaque frozen port-set. Monotone: a
+    # frozen child is never reopened (proposal §8 termination clause), so
+    # ReoptimizeParent strictly shrinks the parent's open slice and the
+    # outer loop still terminates.
```

Determinism obligation: `ReoptimizeParent` must be a pure function of the frozen
child port-set + parent local graph — no dependence on extraction timing. Pinned
by the multi-owner permutation witnesses (Part 6): attach order must not change
the frozen parent.

---

## Part 3 — local Rel qualified by (RegionId, InstanceId): the induction machinery

This is the heart of Stage D. Today (pseudocode §4b Step 5) `BuildStratumPhases`
runs `LowerDRRounds` with ONE induction vector set per SCC — the `view_to_swap_vec`
map in `lib/ControlFlow/Build/Induction.cpp` (one working + one swap VECTOR per
SCC-member view, semi-naive frontiers via `TryClaimDel`/`TryClaimAdd`). Keyed
instances (§4b Step 6) never touch this — they run once, unstratified, at the
epoch tail. Stage D routes a region's local relation SCC through this machinery
**qualified by the instance**.

### 3.1 The stratification story (OWNER-GATED — two realizations)

The SEMANTIC model is fixed by proposal §7.1:

```text
for each affected (RegionId, InstanceId):
    while local recursive frontier is non-empty:
        execute next Rel induction round
```

The PHYSICAL realization of "per (RegionId, InstanceId)" is deliberately left
open by proposal §9 ("per-instance tables, a shared keyed index, or
recomputation"). This is a genuine open decision. Two realizations, laid out — NOT
silently picked:

**Variant V-PI (per-instance vector sets).** Allocate a fresh `view_to_swap_vec`
set — working + swap VECTOR per SCC-member view — per live `(RegionId,
InstanceId)`. Runtime holds `InstanceId → VectorSet`, allocated at instance
birth (the descendant of today's `FindOrAddInstance`). The fixpoint of §7.1 runs
literally: one instance's rounds are isolated from another's by construction.
- + Isolation is structural; no cross-instance contamination possible.
- + Matches the existing `InstanceStore` double-buffer physical strategy (§4/§4c) — the smallest conceptual leap from Stage C's inherited runtime.
- + The termination measure (§4.2) is per-instance and unchanged.
- − Allocation churn: a working/swap vector pair per view per live instance; a program with many small instances pays N × the vector overhead.
- − The differential machinery (claim gates, counters, commit sweeps, the 29-kind census) must be re-parameterized by InstanceId at the VECTOR level — every `TableVec`, every `TryClaim*`, every commit sweep index rebuild gains an InstanceId dimension.

**Variant V-CW (instance-column-widened vectors).** Keep ONE `view_to_swap_vec`
set per SCC (as today), but prepend the region's `InstanceKey` (the frozen
`InstanceKeySchema`, proposal §5.1) as leading columns of every local Rel row,
vector, table, and index key. All live instances' frontier rows coexist in one
vector; join predicates and `TryClaimDel`/`TryClaimAdd` gates match on the
InstanceKey columns exactly as they already match on ordinary key columns. The
SCC fixpoint runs to GLOBAL quiescence across all instances at once; the
InstanceKey column keeps the instances from cross-contaminating (a back-edge fold
carries its InstanceKey unchanged — the termination premise §4.2/(b) guarantees
the key is never derived).
- + REUSES the differential machinery UNCHANGED — claim gates, counters, commit sweeps, semi-naive frontiers, the census — modulo a widened key projection. This is the minimal-new-mechanism realization (necessity-audit ethos; Concern-4's "results stored once" generalized to "one SCC, instances distinguished by a key column").
- + One vector set → better batching/cache behavior; no per-instance allocation.
- + `EvaluateEpoch` (§7) already quiesces ALL frontiers per epoch — a shared cross-instance fixpoint matches the epoch semantics directly.
- − Every local table/index/predicate is widened by the InstanceKey; the commit-sweep `CompactDead()` index rebuild (§4b Step 7) must project on the widened key.
- − Couples all instances of one region into one fixpoint loop; a per-instance termination proof must be recovered as "the widened fixpoint is the disjoint union of the per-instance fixpoints" (Part 4 termination).

**Recommendation (owner-owed).** V-CW is the recommended FIRST realization: it
reuses the landed differential machinery essentially unchanged and keeps the new
mechanism count near zero, which is the whole ethos of this replacement (the §1
audit's smaller-architecture goal, and the necessity-audit input below). V-PI
matches §7.1's literal per-instance loop and the inherited `InstanceStore`
double-buffer, and is the safer choice if the widened-key commit sweep proves
hard to validate. **The decision is OWNER-GATED** — record it as such; do not let
an implementer pick by default. Whichever wins, proposal §9's clause that runtime
strategy is not selected by this architecture means the OTHER remains a legal
future physical alternative behind the same lifecycle contract.

### 3.2 Hunk — instance-qualified rounds (anchor: pseudocode §4b Step 5 `BuildStratumPhases` / §6 Stage C `local_fixpoint`)

```diff
 Step 5 — DIFFERENTIAL MACHINERY: BuildStratumPhases.
     BuildDRInventory
     DeriveDRStrata
     ...
     per stratum (ascending): seed vectors → LowerDRFlow → LowerDRRounds
-      # STAGE C: local_fixpoint in RelRegionLifecycle exists but a region
-      # with a relation SCC is inadmissible (Part 1.2), so LowerDRRounds
-      # runs only over the global observation-root strata. No instance
-      # qualification.
+      # STAGE D: a FrozenRegionTemplate whose body carries an
+      # instance-closed SCC (Hunk 1.2) lowers that SCC's rounds through
+      # LowerDRRounds QUALIFIED by (RegionId, InstanceId):
+      #   V-CW: the SCC's tables/vecs/indices/claim-gates are widened by
+      #     the frozen InstanceKeySchema; LowerDRRounds is UNCHANGED except
+      #     it reads the widened key projection.
+      #   V-PI: LowerDRRounds is parameterized by InstanceId; the
+      #     view_to_swap_vec set is looked up per live instance.
+      # local_fixpoint (RelRegionLifecycle) is now the ACTUAL inductive
+      # region schedule, not a trivial pass-through.
```

### 3.3 Hunk — instance rounds are STRATIFIED (retire the epoch-tail asymmetry) (anchor: pseudocode §4b Step 6)

Today's ASYMMETRY (§4b Step 6): `GROUP_UPDATE` is stratified inside `LowerDRFlow`
but `SUBGRAPHINSTANCE` is NOT — it runs once at the epoch tail. Stage C already
deleted `LowerSubgraphInstances` and the SUBGRAPHINSTANCE op. Stage D must ensure
the replacement — the region's `local_fixpoint` — is placed INSIDE the stratum
schedule, not at the tail:

```diff
 Step 6 — PUBLICATION TAIL: PublishDifferentialMessageVectors.
-    if dr_flow:
-        LowerSubgraphInstances   # ONCE, OUTSIDE all strata, at epoch tail
-        LowerCommitSweeps
+    # STAGE C already removed LowerSubgraphInstances. STAGE D: a region's
+    # request_edge/local_fixpoint/child_result/routed_result ops are
+    # scheduled by DeriveDRStrata like any other Rel op — the region's SCC
+    # gets a real stratum, and the ownership-postorder of the region forest
+    # induces an ADDITIONAL ordering constraint on those strata:
+    #   child region strata precede parent region strata that consume the
+    #   child's routed_result (proposal §7 EvaluateEpoch: maintain child
+    #   before RoutedResult before parent).
+    LowerCommitSweeps
```

New stratum-ordering obligation: `DeriveDRStrata` (the monotone integer lift)
must respect BOTH the ordinary dataflow dependence AND the region ownership
postorder. Concretely: `stratum(child.local_fixpoint) < stratum(parent op that
reads routed_result of child)`. This is the instance-qualified analogue of
today's `flow.instance_stratum[sid] = 1 + max(ready_after(demand),
ready_after(input))` (§3 `:1195`) — but now it participates in the SCC machinery
instead of ordering a single tail op. Validated by V-REGION-STRATUM-ORDER
(Part 5).

---

## Part 4 — termination re-established per proposal §7.1

Two independent measures (proposal §7.1), each re-proven for the deep,
instance-qualified forest:

**Measure 1 — finite region depth.** Region requests move only region → proper
direct child in a finite acyclic ownership forest (proposal §3.2). Nested
extraction (Part 1) preserves this: `ExtractPureChild` sets the new child's owner
to the caller, and admissibility (Hunk 1.2) forbids any back-edge crossing a
child-call boundary (that is the cyclic_demand exclusion, §0.1). Depth-2 nesting
(the witness matrix) is finite; there is no regional request cycle. Validated by
the existing "cycle in the region ownership/call graph" validator (§11), promoted
from the deleted cyclic_demand codegen reject.

**Measure 2 — local relation fixpoint per (RegionId, InstanceId).** Each SCC's
rounds terminate by the EXISTING Rel measure (CLAUDE.md core invariant: every
inductive back-edge fold is an `UPDATECOUNT` dominated by its zero crossing).
Stage D adds ONE obligation to keep this measure valid under instance
qualification: **the InstanceKey/InstancePath columns never appear on a recursive
back-edge** (Hunk 1.2 clause (b); proposal §7.1 final line "No InstancePath
growth participates in recursive relation evaluation"). Given that:

- Under **V-PI**, each instance's fixpoint is literally a separate invocation of
  the unchanged machinery — terminates exactly as today.
- Under **V-CW**, the widened fixpoint is the DISJOINT UNION of the per-instance
  fixpoints: because the InstanceKey is bound (never derived) on every fold, no
  fold ever moves a row from one instance's key-partition to another's, so the
  single loop is provably the sum of independent per-instance loops, each
  terminating by the original measure. The proof obligation is exactly clause
  (b), checked by V-INSTANCEKEY-NOT-DERIVED.

**Escalation note (E2 below):** the disjoint-union argument for V-CW is a NEW
termination lemma this proposal introduces. It must be discharged with a directed
witness (nonlinear local recursion under two distinct instance keys, proving the
two fixpoints do not interleave into non-termination). It is in the witness
matrix (Part 6, `local recursion × key aliases`).

---

## Part 5 — validators added / retired (diff on proposal §11)

Stage C introduced the lifecycle census + `request_edge` balance validators.
Stage D adds the nesting/recursion validators. Named V-* per house convention;
each is always-on (fprintf+abort, survives NDEBUG) unless marked frozen-compile
(a clean diagnostic).

| Validator | Kind | Checks | Replaces / relates to |
| --- | --- | --- | --- |
| **V-SCC-INSTANCE-CLOSED** | frozen-compile (clean diagnostic) | every local relation SCC lies wholly inside one region; no back-edge crosses a child-call boundary | the descendant of cyclic_demand's within-content half; Build.cpp `:1454` `ViewSelfReachable` check, moved to the frozen planner |
| **V-INSTANCEKEY-NOT-DERIVED** | frozen-compile | no InstanceKey/InstancePath column appears on a recursive back-edge (Part 4 measure 2, clause (b)) | NEW — no analogue today (instances were never inductive) |
| **V-REGION-STRATUM-ORDER** | always-on | `stratum(child.local_fixpoint) < stratum(parent op reading child.routed_result)`; the ownership postorder is respected by `DeriveDRStrata` | generalizes `flow.instance_stratum[sid]` ordering (§3 `:1195`) into the SCC machinery |
| **V-PORT-POSTORDER-CLOSED** | frozen-compile | every child port is frozen before its parent's ports solve (Hunk 2.1); no open port reaches a parent solve | proposal §11 "open port reaching FrozenRegionalProgram", deepened for nesting |
| **V-NEST-DEPTH-FINITE** | always-on | ownership forest is acyclic and finite-depth; DFS finds no back-edge (Measure 1) | proposal §11 "cycle in the region ownership/call graph"; promoted from the deleted cyclic_demand codegen reject |
| **V-INSTANCE-KEY-ALIAS** | always-on | two lexically distinct child instances with textually equal keys at different `InstancePath` depths are kept DISTINCT (the depth-2 key-alias witness); `InstancePath` — not `InstanceKey` alone — is the identity | NEW — the nesting-depth-2 lexical-alias hazard has no analogue in the one-level Stage C |

Retired at Stage D (their subject no longer exists / is subsumed):

```diff
- (already gone at Stage C) V-INST-EFFECT / V-INST-SOLE / V-INST-PAIR /
-   CheckInstantiateEffects / CheckInstanceSolePub / CheckInstanceDeathFrontier /
-   CheckInstanceInputArm / V-ALPHA / V-INST-ORDER / CheckInstanceOrder
-   (Rel.cpp:4280-4552, 4993) — the kSubgraphInstantiate/kInstanceDeath/
-   kInstanceSeal validator family; deleted WITH those ops at Stage C.
  # Stage D introduces NO new instance-OP census because there is no instance
  # op — the region's SCC is ordinary Rel rounds qualified by a key. The
  # per-region local census is the EXISTING 29-kind Rel census (rel-arch §7),
  # now run per FrozenRegionTemplate (proposal §11 "per-region local census").
```

---

## Part 6 — the witness matrix (diff on proposal §12.3 rows)

Each charge-named witness is mapped to its §12.3 row and its referee. All are
NEW directed witnesses added at Stage D (the non-recursive demand witnesses were
rewritten against edges/leases at Stage C).

| Charge witness | §12.3 row(s) | Concrete shape | Referee |
| --- | --- | --- | --- |
| **Nesting depth 2 with lexical key aliases** | Nested regions | Parent region P calls child C(k); C's body calls grandchild G(k) reusing the SAME lexical key name `k` at a different `InstancePath` depth. Assert G@(P-inst,C-inst,k) ≠ G@(other C-inst,k). | I0 agreement (final membership) + V-INSTANCE-KEY-ALIAS / V-NEST-DEPTH-FINITE never abort |
| **Linear local recursion** | Local recursion | Transitive closure inside one extracted child instance (single recursive back-edge). | I0 agreement + same-epoch permutation invariance |
| **Nonlinear local recursion** | Local recursion | `path(a,c) :- path(a,b), path(b,c)` inside one instance (two recursive uses per rule) — stresses the V-CW disjoint-union lemma (Part 4/E2). | I0 agreement + permutation invariance |
| **Mutual local recursion** | Local recursion | `even/odd` mutual SCC inside one instance (two views in one SCC). | I0 agreement + permutation invariance |
| **Multi-owner ATTACH permutations** | Multiple owners + Late subscriber + Multiple requester members | Two requester members demand the same child key; attach in each order; assert late subscriber receives already-maintained results. | Same-epoch permutation invariance (permcheck-style: published-delta tokens order-free per epoch) + I0 |
| **Multi-owner DETACH permutations** | Detachment + Multiple owners | Two owners, retract each first; assert removing one edge retracts only its routed results, other owner unaffected; child state retired only after last edge dies. | I0 agreement (final membership + live RequestEdgeId set) + permutation invariance |
| **Request/data order flaps** | Request/data order | request-before-data, data-before-request, and same-epoch interleave, over the nested + recursive shapes above. | Same-epoch permutation invariance (§7 "Same-epoch request and input permutations must produce the same final relations and publication delta") |

Determinism witness (house precedent `symrec_tie_1`): the nested-extraction
ordering (Hunk 1.1 worklist) must be a pure graph function — a directed
determinism witness with ≥2 sibling children whose extraction order is
tie-broken deterministically; its dump is byte-stable across runs.

---

## Exit gate (golden-master terms)

**Stays byte-identical (byte-compare referee).** The non-demand corpus — every
one of the 180 goldens that carries NO bound query (~127 cases; the ~53 with
bound queries were rewritten at Stage C) — never extracts and never enters a
region SCC, so all four optimization modes stay byte-identical through Stage D.
This includes the entire aggregate/KV/negation/join corpus. Gate: full
`runall.sh` ends `SUITE: PASS`, no bless.

**Legitimately changes shape (adjudicated, not byte-compared).** The
demand-derived witnesses that GAIN nesting or local recursion. There is no
byte-golden for these — their emission shape is new at Stage D — so they are
adjudicated by SEMANTIC referees only:

1. **I0 agreement** — the reference relational interpreter (built + corpus-
   validated at Stage A/B per Concern 1) is the sole correctness oracle for each
   matrix witness: final relation membership + net published deltas + live
   RequestEdgeId set + live ChildInstanceId set + caller-qualified routed results
   (proposal §12.2). Disagreement is a finding, never tolerance-fudged.
2. **Same-epoch permutation invariance** — `permcheck.py`-style referee
   generalized to the request/data axis: permute independent request and input
   updates within one epoch; final relations + net publication delta must be
   identical (published-delta tokens compare order-free per epoch, all committed
   membership byte-identical). This is proposal §15 invariant 14.
3. **Answer-equivalence to Stage-C full materialization** — for each
   recursive-content witness, the Stage-D extracted-and-instance-evaluated result
   must equal the Stage-C observation-root full-materialization result (§0.2). I0
   carries both since both are answer-correct by construction; this is a
   cross-check that extraction did not change answers.

**Dump surface.** The regional dump (grammar decided at Stage B) gains
per-`(RegionId, InstanceId)`-qualified local-graph rendering and the nested
ownership-postorder region listing; new directed `.rel`/regional-dump pins for
the determinism witness + one nested + one recursive witness, blessed ONLY via
`runall.sh --bless` after review, keyed order-free where the pin is a
published-delta set.

---

## Mechanisms carried forward / introduced (necessity-audit input)

One line each. "CARRY" = survives from a prior layer into Stage D; "NEW" =
introduced by Stage D; "RETIRE" = deleted at/by Stage D (or already at Stage C
and confirmed absent).

- CARRY — `RequestEdgeRelation` / `ActiveInstanceRelation` / `ChildResultRelation`
  / `RoutedResultRelation` (Stage C): the demand authority; Stage D routes them
  across nesting depth ≥ 2.
- CARRY — move-only `RootRequestLease` (Stage C): one lifetime API; unchanged by
  nesting (a lease still names a ROOT edge; nested edges are RegionalMember-owned).
- CARRY — the Rel differential machinery: claim gates (`TryClaimDel`/`TryClaimAdd`),
  split signed derivation counters, commit sweeps + `CompactDead()`, ingest folds,
  eager markers, the 29-kind census, the V-* intrinsic validators — the local-graph
  lowering (rel-arch §7); Stage D runs it PER region template, and (under V-CW)
  widened by the InstanceKey.
- CARRY — semi-naive induction machinery (`view_to_swap_vec`, working+swap vectors,
  `DeriveDRStrata` monotone lift, `LowerDRRounds`): now the per-instance local
  fixpoint (Part 3).
- CARRY — `FrozenRegionalProgram` / `FrozenRegionTemplate` / port types (Stage B);
  Stage D deepens the forest through them, never reopens a frozen port.
- CARRY — I0 reference interpreter (Stage A/B): the sole correctness referee for
  the Stage-D matrix.
- CARRY — `ExtractPureChild` / `FirstStableAdmissibleChild` / `ExtractionPolicy`
  seam (Stage C): Stage D wraps them in the descent worklist (Hunk 1.1).
- NEW — deep-forest extraction worklist (Hunk 1.1): re-enqueues each extracted
  child for its own extraction.
- NEW — `InstanceClosedSCC` planner predicate + admissibility clause (Hunk 1.2):
  the recursive_content fence's replacement.
- NEW — instance-qualified local fixpoint (Part 3): the (RegionId, InstanceId)
  qualification of the SCC rounds. **Physical realization OWNER-GATED**: V-PI
  (per-instance vector sets) vs V-CW (instance-column-widened vectors).
- NEW — `InstancePath` runtime qualification for depth-2 lexical key aliases
  (the descendant of today's `ChildInstanceId`/`InstanceId`, now a PATH not a
  flat key): distinguishes textually-equal keys at different ownership depths.
- NEW — region ownership-postorder stratum-ordering constraint on `DeriveDRStrata`
  (Hunk 3.3): child local_fixpoint stratum precedes parent routed_result reads.
- NEW — parent post-freeze reoptimization pass (Hunk 2.2).
- NEW validators — V-SCC-INSTANCE-CLOSED, V-INSTANCEKEY-NOT-DERIVED,
  V-REGION-STRATUM-ORDER, V-PORT-POSTORDER-CLOSED, V-NEST-DEPTH-FINITE,
  V-INSTANCE-KEY-ALIAS (Part 5).
- NEW oracle referee — same-epoch permutation invariance over the request/data
  axis (permcheck-generalized), added to the I0 gate.
- RETIRE (at Stage C, confirmed absent at Stage D) — `kSubgraphInstantiate`/
  `kInstanceDeath`/`kInstanceSeal`; a0/a1/a2/a2'/b bands; `LowerSubgraphInstances`
  epoch-tail asymmetry; `flow.instance_stratum[sid]` single-tail-op ordering;
  the V-INST-*/CheckInstance*/V-ALPHA/CheckInstanceOrder validator family;
  `InstanceStore`-as-semantic-model (its double-buffer MAY survive as a V-PI
  physical realization, but not as the identity/liveness authority).
- RETIRE (at Stage D) — the recursive_content admissibility EXCLUSION (Hunk 1.2);
  replacement = the per-instance fixpoint, same stage.
- PERMANENT EXCLUSION (never lifted) — recursive/key-changing region calls (the
  cyclic_demand descendant): promoted from a codegen reject to V-NEST-DEPTH-FINITE
  / V-SCC-INSTANCE-CLOSED, live forever (proposal §2.2, §6).

---

## Escalations

- **E1 — OWNER-GATED: the stratification realization (Part 3.1).** V-PI
  (per-instance vector sets, matches §7.1's literal loop + the inherited
  InstanceStore) vs V-CW (instance-column-widened, reuses the differential
  machinery unchanged; recommended first realization). Do not let an implementer
  default-pick. Whichever ships, the other stays a legal physical alternative
  behind proposal §9's open-runtime clause.

- **E2 — MISSING ORACLE for recursive-content demand-instance.** Today a
  recursive-content demanded body is a REJECT (`demand_recursive_content_1`), so
  there is NO pre-cutover tagged binary that ever executed instance-evaluated
  local recursion. The tagged-binary behavioral-golden referee (Concern 1) is
  therefore UNAVAILABLE for the recursive matrix witnesses — I0 is the SOLE
  referee for them. This mirrors §7.1 open-decision 1 (the R-DIFF-pub demand-
  instance gap: no legacy binary to compare against). Owner must ratify that I0
  alone (plus the Stage-C-full-materialization answer-equivalence cross-check,
  §0.2) is a sufficient oracle for the recursion witnesses, OR author a
  hand-verified expected-output golden per recursive witness. Do NOT proceed on
  vibes: without E2 resolved there is no referee for nonlinear/mutual local
  recursion under instance keys — exactly the case the V-CW disjoint-union lemma
  (Part 4/E2) most needs pinned.

- **E3 — OWNER-DECISION carried from Stage C (Concern 2), re-confirmed load-
  bearing at Stage D.** Stage D promotes recursive-content from
  full-materialization (Stage C) to extracted per-instance evaluation. But the
  OTHER Stage-C inadmissible cases — `demand_cyclic_1` (recursive region call)
  and `demand_multi_adorn_allfree_1` — stay inadmissible → full materialization
  FOREVER (E1/cyclic_demand permanence). The owner must confirm the Stage-C
  decision (inadmissible ⇒ full materialization, never a reject) still holds at
  Stage D for the permanently-excluded classes, i.e. these become silent
  full-materialization, not compile errors. This is a behavior change from
  today's clean diagnostics and must be a stated, owner-ratified decision — not
  discovered by an implementer.

- **E4 — determinism of deep extraction order.** Hunk 1.1's worklist and Hunk
  2.2's post-freeze reoptimization must be pure graph functions (HP-9). If the
  deterministic tie-break for sibling extraction cannot be stated as a DefList
  view-walk order, that is a determinism hole — escalate rather than pin a
  run-order-dependent golden.
