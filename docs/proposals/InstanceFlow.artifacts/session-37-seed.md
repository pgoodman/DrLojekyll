<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-37 seed — InstanceFlow after the arrangement derivation: Stage C, the allocation inversion

> START-HERE for the next session. Written at the close of session 36 (branch
> `keyed-instances`). Session 36 landed the ARRANGEMENT derivation — Stages A+B
> BUNDLED as derive-early / cross-check-late — gate GREEN (OptDiff **SUITE PASS
> 227** codegen byte-identical, ctest **5/5**, belt live-verified both ways).
> Authority: `session-36-grounding.md` (recon §3, panel §4, execution §4.5,
> Stage-C residuals §5) + the CLAUDE.md InstanceFlow s36 paragraph.

## §0 Where we are — do NOT re-litigate

- **BOTH halves of the runtime-resource decision are now first-class, derived,
  cross-checked authorities** stored on `QueryImpl::materialization` at the
  `Query::Build` tail, BEFORE `Program::Build` runs:
  - RESOURCES (s34): one `StateResource` per stateful `EquivalenceSetId` class,
    cross-checked by `CrossCheckMaterialization` (post-`FillDataModel`).
  - ARRANGEMENTS (s36): one `Arrangement` (`ArrangementId` +
    `ArrangementKey{StateResourceId, vector<ColumnOrdinal>}`) per
    `(resource, column-ordinal set)` index requirement, derived by
    `DeriveArrangements` (R-FULL / R-JOIN-UNIFORM / R-NEG / R-QUERY +
    the R-INTERFACE count), cross-checked by `CrossCheckArrangements`
    (post-region-build, via `context.dr_flow->table_to_resource`).
- The falsifiable claim held corpus-wide: the emission walk's `GetOrCreateIndex`
  requests ARE a pure function of the final graph (227 cases × 4 modes,
  quiescent). The plan is exactly the requirement set
  `AllocateRuntimeResources` must consume.
- Still an OBSERVER: nothing consumes resources/arrangements for emission.
  Stage C is the inversion; Phase D (first codegen move) rides on it.

## §1 Stage C — the allocation inversion (the megaproject payoff)

Target shape (session-35-arrangement-scope.md §2-C, now unblocked):

```diff
+ AllocateRuntimeResources(materialization):   # the ONLY allocation site
+   for r in plan.resources:     mint its TABLE up front
+   for a in plan.arrangements:  mint its index up front
+   (+ interface tables from the R-INTERFACE tier — see §2.1)
- FillDataModel's TABLE::GetOrCreate mints; the 6 inline GetOrCreateIndex sites
+ region builders LOOK UP (resource, col-set) -> ArrangementId -> handle
```

## §2 The FIVE named residual lacks (grounding §5 — the scoping input; attack
in this order)

1. **Interface-table authority.** `BuildEmptyQueryEntryPoint` (Build.cpp:519)
   mints outside the plan. s36 derives + cross-checks only the COUNT. Stage C
   needs the per-table index sets in a plan tier (pure-derivable: per dead
   query decl, the full index + per-unique-bound-pattern subsets — the same
   R-QUERY logic over uncovered decls). Small, self-contained first slice.
2. **The TABLEINDEX id-assignment contract.** Ids come from the GLOBAL
   `impl->next_id` at mint time; codegen embeds them (`idx_<Id()>`,
   CPlusPlus/Database.cpp:501). Up-front minting RENUMBERS ids program-wide ⇒
   wholesale `.h`/`.ir` golden churn. CARVE-3 says index ids are
   non-contractual and the answer goldens are the net — but the churn must be
   an EXPLICIT owner decision (a permutation-style re-bless), not a side
   effect. DECIDE THIS FIRST — it determines whether Stage C can land in
   byte-preserving slices (e.g. mint in the exact id-order emission would
   have) or accepts one loud renumbering commit.
3. **Column-order/schema authority.** A table's column order comes from its
   FIRST-registered member view (Data.cpp:150-205); the resource schema from
   the canonical representative — not proven the same view. Stage C minting
   tables FROM resources must pin one authority and discharge the E2
   congruence (the INSERT↔guard-TUPLE union, DataFlow Build.cpp:2412-2428, is
   the one congruence-unchecked path; a census-time arity belt already exists
   in `DeriveArrangements`).
4. **Step 2b** (plan `support` vs `TableIsDifferential` view-set equality) —
   needed before the runtime STORE SHAPE is decided from the plan at mint
   (diverges on agg/KV outputs, measured s35). NOTE s36's derivation
   deliberately needed NO differentialness predicate — the premise bites only
   when Stage C picks store shapes.
5. **Honesty:** the inversion re-points the six sites' key COMPUTATION to
   lookups; it deletes their allocation authority, not their column logic.
   The lookup key at each site is the same `(class, ordinals)` the derivation
   replays — the site logic and the plan stay cross-checkable.

## §3 Recommended next step (owner re-ranks)

**C0 (one-session, observer-safe): the interface tier + the id-contract
decision.** (a) Lift the R-INTERFACE count to a full plan tier (per dead query
decl: table schema + its arrangement set), cross-checked per-table — closes
residual 1 and makes the plan TOTAL over every table the program mints. (b) A
grounded owner memo on residual 2 (measure the actual golden churn of id
renumbering on 2-3 witnesses in a throwaway worktree — predict, don't guess),
producing the ratified Stage-C landing strategy (byte-preserving mint-order vs
one loud renumber). THEN C1: `AllocateRuntimeResources` behind a
cross-check-only shadow (mint nothing; verify the plan's mint ORDER reproduces
the real id stream), then the real inversion slice by slice.

## §4 Anchors (re-verify at tip — the pipeline drifts)

| Fact | Source (s36 close) |
|---|---|
| Derivation | `DeriveArrangements` `lib/DataFlow/Materialization.cpp` (after `PlanResources`; called `lib/DataFlow/Build.cpp` tail) |
| Types | `ArrangementId`/`ColumnOrdinal`/`ArrangementKey`/`Arrangement` `lib/DataFlow/Materialization.h` |
| Cross-check | `CrossCheckArrangements` (`Query.h` friend; census at `lib/ControlFlow/Build/Build.cpp` tail, pre-`return Program(...)`) |
| Render | `lib/DataFlow/Format.cpp` `QueryMaterialization` (arrangements block + `interface-tables=` token) |
| Goldens | 5 `.materialization.opt.golden` (join_1 8 / transitive_closure 4 / corecursion_1 4 / negate_1 4 / tc_nonlinear_diff 6 arrangements) |
| The six sites (Stage C deletes their allocation authority) | `Data.cpp:205`, `Build.h:443` (3 Stratum callers :1033/:1215/:1333), `Join.cpp:408` (:272 dead), `Build.cpp:432/490/526` |
| s35 map (Stage C inverts it) | `DRFlowGraph::table_to_resource`; stashed `context.dr_flow` (Stratum.cpp, always before early return) |
| Panel + residuals | `session-36-grounding.md` §4/§4.5/§5 |
| Index-id embed (residual 2) | `lib/CodeGen/CPlusPlus/Database.cpp:501` `idx_<Id()>` |

## §5 Open questions carried forward
- Step 2b view-set-equality premise (residual 4; unchanged from s35).
- Discriminating kBoundQueryRead witness (s33) still not in corpus.
- The E4 empty-query probe is scratch-only — C0(a) should promote a corpus
  witness (needs a driver; the probe program is in grounding §3-E4).
- P9 firewall relaxation: `DeriveArrangements` R-QUERY MIRRORS
  `SelectAccessPlan` rather than reading it (layering); if the selector ever
  returns kFullScanFilter for a non-empty bound set (P8 trie / cost model),
  the mirror must follow — the cross-check will catch the drift loudly.
