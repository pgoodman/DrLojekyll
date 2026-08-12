I have enough grounding. The two verifications are decisive: (1) `GuardAnnotation` at tip has `Role{kBody, kQueryProjection}` and `instance_key` exactly as E4 assumes (R2 discharged), and (2) `Demand.cpp` fences multi-*clause* queries upstream (`:595`) but the body walk **explicitly traverses JOINs** (`:704-716`, `pj->out_to_in`) — a multi-predicate join body is *recognized*, not rejected. These reshape the answer-equality analysis below.

---

# REFUTER #1 (answer-equality) — Findings

## Finding 1 — BLOCKING: the canonical monotone witness resolves `ri.ok=false` → **zero instances minted → empty answer**

The design says "restore `ResolveLiveRecognition` **entire** (E4 §1)." That function resolves `input_table` **only** from a `kBody`-role guard:

```cpp
if (annots[ai].role == GuardAnnotation::kBody && !ri.input_table) {
    if (TABLE *in = model_table(jl[1])) { ri.input_table = in; ... }
}
```
and `ri.ok = demand_table && input_table && pub_table && …`.

But E1 §3, dumping the **exact canonical witness** (`neighborhood(bound Start, free Node) : edge(Start, Node)`), states the surviving guard is **role `kQueryProjection`, and the body-predicate guard set (step 7) is empty** ("edge is read only by the query itself"). The one live `^join.5` carries `role = kQueryProjection`, not `kBody`. As-extracted, `ResolveLiveRecognition` therefore finds **no `kBody` guard**, leaves `input_table == nullptr`, sets `ri.ok = false`, and `BuildSubgraphInstanceOps` hits `continue;` (E4 §2c: "fully-dead forcing … ABA-safe skip"). **Zero `kSubgraphInstantiate` ops are minted; the pub relation is never materialized; the query returns empty** — maximal divergence from both goldens.

This is exactly the silent-zero failure mode the design flags in Risk R3 and E6/A7: the census recount is **also** sourced from `ResolveLiveRecognition`, so it agrees with the (wrong) zero and masks the bug. No always-on validator catches it; only the oracle/answer golden does.

Either the E4 extract elided the `kQueryProjection` arm of the input resolution, or the guard that actually survives CSE for a single-body-predicate relation carries `kBody` (contradicting E1). This must be reconciled against a live post-`Optimize` dump **before** any answer-equality claim.

- **Anchor**: E4 §1 `ResolveLiveRecognition` (`dc965d3c^:lib/Rel/Rel.cpp:936-1025`); E1 §3 (witness guard role); tip `Query.h:1010` (`Role{kBody, kQueryProjection}`).
- **Fix**: before wiring, dump the witness under a restored keyed compile and confirm which role the surviving guard carries. If `kQueryProjection`, the input resolution predicate must accept it (or accept `jl[1]` from the query-projection guard when no `kBody` guard exists). Add a **loud** `ValidatorFail` when `RecognizedSubgraphs()` is non-empty but every forcing resolves `!ok` — turn the silent `continue` into an assert-covered census cross-check that a recognized forcing produced ≥1 instance. Do not let a recognized-but-unresolved forcing be silently skipped.

## Finding 2 — BLOCKING: "answer-equal to the non-keyed full-materialization `.stdout`" is FALSE for any non-query consumer of the demanded relation

The keyed store (like flat `-demand`) materializes the pub relation **only for demanded keys**. This is answer-equal to full materialization **only when the demanded relation is observed exclusively through the bound `#query` cursor** (which is itself the demand trigger). It is *not* equal when any downstream rule/message reads the demanded relation:

```
#message add_edge(u64 From, u64 To).
#message reachable(u64 From, u64 To).
#local edge(u64 From, u64 To).
edge(F,T) : add_edge(F,T).
#query neighborhood(bound u64 Start, free u64 Node) : edge(Start, Node).
reachable(S, N) : neighborhood(S, N).      // <-- non-query consumer, publishes a message
```
- Full-materialization `.stdout`: `reachable(S,N)` published for **every** `(S,N)` in `edge`.
- Keyed (and flat) demand: `neighborhood` exists only for demanded `Start` → `reachable` published only for demanded `Start` → **set-diverges** from the full `.stdout`.
- Worse for `.monotone.stdout` (per-epoch): under full-mat, `reachable(1,2)` publishes in the **edge-arrival epoch**; under keyed demand it publishes in the (later) **demand-arrival epoch** — a per-epoch published-delta divergence even if the final set matched.

So the orchestrator's framing "validated vs two EXISTING goldens (non-keyed `.stdout` + add-only `.monotone.stdout`)" is category-confused for the general case. The **only** defensible equalities are: (a) flat-`-demand` == nested-keyed (the eqgate), and (b) equality to the non-keyed golden **restricted to a witness whose demanded relation feeds only the bound `#query`** (a `#query` publishes nothing per-epoch, so the `.monotone.stdout` comparison is vacuous-safe there).

- **Anchor**: demand is a materialization restriction by construction (`ApplyDemandTransform`, `Demand.cpp`); this is intrinsic, not a bug.
- **Fix**: pin the witness contract explicitly — the demanded relation must have **no non-query consumer**, and the driver must observe it **only** via `neighborhood_bf(db, Start)` for the same `Start`s in both compiles. State that answer-equality-to-full-materialization holds **iff demand is total over the observed keys**. Add a recognition/keyed-mint belt (or document the existing fence) that a demanded relation with a downstream published consumer is a diagnostic, not a silent semantic change. Do not compare keyed output against a full-materialization golden produced by a whole-relation observation.

## Finding 3 — MAJOR: single-table rescan + `!ri.input_table` first-wins under-materializes a join-body demanded relation (recognition allows it; HP-4 does not refuse JOIN)

The body walk **traverses joins** (`Demand.cpp:704-716`: `JOIN *pj … pj->out_to_in.find(in_col)`, the `kPushDown` site), so a multi-predicate body like `neighborhood(S,N) : edge(S,M), edge(M,N)` is **recognized**, not rejected. But the keyed emitter models the demanded body as **one input table filtered by the key** (E4 §7 `emit_instance_rescan`: `for s in input.NumRows(): if ir.<in_key>==key add Row{ir.<in_row>}`), and `ResolveLiveRecognition` picks **exactly one** input via `&& !ri.input_table` (first `kBody` guard wins, rest silently dropped). Two failure modes:
1. `ri.input_view` is a `JOIN` — HP-4 refuses only `Map/Negate/Aggregate/KVIndex` (E4 §2c), **not** `Join`/`Merge`. If the join output is not table-backed, `input_member = table_member[input.Id()]` (E4 §7) references a table that does not exist → miscompile; if it is table-backed, `in_key`/`in_row` positions (from `annots[ai].instance_key` over the *join* schema) must line up with the pub reconstruction — an untested projection surface.
2. Even for a single clause, the `!ri.input_table` first-wins means any second body-guard site (a body that legitimately reads two source tables at the pivot) is **silently discarded** → the instance rescans one table → **under-materialization**, invisibly.

The 1-hop witness never exercises this; the `product_in_scc` / two-hop shapes would.

- **Anchor**: `Demand.cpp:704-716` (join traversal); E4 §2c HP-4 belt (no JOIN refusal); E4 §1 (`&& !ri.input_table`); E4 §7 (`input_member` single-table rescan).
- **Fix**: add a keyed-mint belt refusing `ri.input_view->IsJoin() || IsMerge()` for slice 1 (extend HP-4), **or** prove-and-assert that the resolved `input_table` is the materialized full body output whose schema the `in_key`/`in_row` positions index. Turn the silent `!ri.input_table` first-wins into a `ValidatorFail` when a forcing resolves **more than one** distinct `kBody` input — a dropped second input is always under-materialization.

## Finding 4 — MAJOR: over-materialization from a wrong key-column correspondence is NOT caught in slice 1 (no HP-5 runtime abort)

Band-(b) publish reconstructs the pub row as `key.c* → key_cols positions`, `row.c* → row_cols positions` (E4 §7 `pub_exprs_for`). This is answer-correct **only if the pub key-column value is always the demand key**, and the rescan filter uses the **value-corresponding** input positions. Recognition's "non-From-preserving" reject (`Demand.cpp:754-760, 809-812`) plus the arity-coherence belt (`si->input_key_cols.size() != inst.key_cols.size()`, E4 §4) guard the common cases — but if `key_cols` (pub α positions) and `in_key` (`annots[ai].instance_key`, input pivot positions) pass the arity belt while differing in value correspondence, the instance for key K silently contains rows belonging to a different key → **over-materialization**. The design notes HP-5 (over-materialization aborts) but that abort lives in the **eqgate driver**, not the runtime; slice 1's only runtime belt is `Seal()`'s `frozen ⊆ current` (monotonicity, E4 §8 lines 188-204), which does **not** check exact-subset. So a mis-keyed instance over-publishes silently.

- **Anchor**: E4 §7 `pub_exprs_for`; E4 §4 arity belt; E4 §8 `DebugValidate` (monotonicity only, not exactness).
- **Fix**: keep the recognition From-preserving fence (necessary) and add a slice-1 runtime `DebugValidate` band-(a) belt that asserts every row placed into instance K actually has `input.<in_key> == K` (the rescan filter guarantees it *if* positions are right — the belt catches a position bug). Cross-check at mint that `pub_cols[key_cols]` types == `input_cols[in_key]` types.

## Finding 5 — MAJOR: `inst.pub_view.Columns()` at descriptor-build reads a possibly-stale live handle → wrong key/row **types** → wrong dedup

The `ProgramInstanceStore` descriptor loop derives `key_types`/`row_types` from `inst.pub_view.Columns()` (E4 §5, `Stratum.cpp`). `pub_view` is the **live** handle re-resolved during `BuildDRInventory`, but the descriptor loop runs **later** in `Program::Build`; any ControlFlow-side graph mutation between the two invalidates it (E1: these VIEW\* handles are trustworthy only at their resolution instant). Wrong types → wrong `Key_<id>`/`Row_<id>` hash structs → wrong per-instance dedup and wrong pub column widths → answer divergence (not just a crash).

- **Anchor**: E4 §5 (`inst.pub_view.Columns()`); E4 §0/E1 (VIEW\* dangling contract); E5 (`Row_<id>` = published row shape).
- **Fix**: derive `key_types`/`row_types` from the **durable** `inst.pub_table` column projection (the `TABLE*` survives, unlike the QueryView), not from a stored `pub_view` handle. This mirrors the mint's own discipline of never dereferencing stored `QueryView`s post-`Optimize`.

## Finding 6 — MINOR: per-epoch attribution boundary (documentation, not a code bug for pure-query witnesses)

For a witness observed only via the `#query` cursor, a `#query` publishes nothing per-epoch, so the add-only `.monotone.stdout` carries no rows for the demanded relation → the comparison is vacuous and safe. The keyed store publishes into the pub table at the **demand epoch**, not the derivation epoch. This is only a divergence if a per-epoch published tap sits over the demanded relation — which Finding 2 already fences.

- **Fix**: state in the witness contract that no `@differential`/message tap observes the demanded relation per-epoch in slice 1 (that is D3.a differential territory).

---

## Residual risk I could NOT refute (noted honestly)

- **Per-key isolation itself is sound** for the intended shape: the rescan filter `ir.<in_key> == K` provably prevents cross-key contamination *within* an instance **given correct `in_key` positions** (Finding 4 covers the position risk). Set semantics (`Table::TryAdd`) match monotone Datalog set semantics — no multiplicity divergence. So for the **1-hop, query-only-observed, single-input, total-demand** witness with Findings 1/5 fixed, I find **no** answer divergence, and flat==nested==full is genuinely achievable there.

---

## VERDICT

**The monotone slice does NOT survive "answer-equal to the non-keyed full-materialization `.stdout`" as framed** — it survives only as flat-`-demand` == nested-keyed (the eqgate), plus equality to a non-keyed golden **restricted** to a witness whose demanded relation is observed solely through the bound `#query` (Finding 2), and **only after** the guard-role resolution contradiction that would zero-mint the canonical witness is reconciled (Finding 1) and the stale-`pub_view` type source is fixed (Finding 5).