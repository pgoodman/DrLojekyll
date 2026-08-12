# TASK IF1 — Identity Substrate + Pass Slot Grounding

## 1. Pass-insertion slot (confirmed at tip)

`Query::Build` tail, `lib/DataFlow/Build.cpp`:

```
2633  impl->IdentifyInductions(log);       // det_seq FINAL stamp here (Induction.cpp:142-144)
2634  if (num_errors != log.Size()) return std::nullopt;
2636  impl->FinalizeDepths();
2637  impl->FinalizeColumnIDs();
2638  impl->TrackDifferentialUpdates(log, true);
2639  if (num_errors != log.Size()) return std::nullopt;
2643  BuildEquivalenceSets(impl.get());
2644  impl->Stratify(log);
2645  if (num_errors != log.Size()) return std::nullopt;
2649  impl->row_contracts = InferConservativeRowContracts(impl.get());
2655  if (!ValidateRowContracts(impl.get(), log)) { return std::nullopt; }  // H-A7
2660  #ifndef NDEBUG
        // K5 conservation belt (origin_decls reachability, debug-only)
2679  #endif
2682  return Query(std::move(impl));
```

**Exact slot**: insert the new InstanceFlow pass call between line 2679 (`#endif` closing the K5 belt) and line 2682 (`return`) — i.e. it runs on the fully-finalized graph (row contracts materialized, K5 belt passed in debug builds), strictly last before construction of the `Query` wrapper. This matches the task's "post-row-contracts, pre-return" instruction and InstanceFlow.md §7.1 ("runs after Query structural optimization, column identity, differential analysis, and semantic SCC discovery... before storage equivalence or ControlFlow table allocation" — note `BuildEquivalenceSets` at 2643 *already ran* by our slot; §7.1's aspirational framing that `BuildEquivalenceSets` is "not part of the target Query build" is a **Phase B/later** deletion, not something this Phase-A slice touches — flag for the design stage).

New call shape (pattern-matched to the row-contracts precedent at 2649/2655):
```cpp
impl->instance_flow = BuildFlatInstanceFlow(impl.get());   // pure, PostOpt(H-A3/H-A4 idiom)
if (!ValidateInstanceFlow(impl.get(), log)) { return std::nullopt; }
```

## 2. catch(...) removal — CONFIRMED gone

Build.cpp:2585-2591 carries an explicit **removal note**, not a live catch:
```cpp
  // NOTE(cost-audit 2026-07-31): the former `try { ... } catch (...) {
  // assert(false); <cull dead views> }` wrapper around this block was
  // deleted. Under `-DNDEBUG` the `assert(false)` vanished and the handler
  // silently continued to `Stratify` over a partially-mutated graph...
  // Optimization/finalization failures now reach the owning boundary (the
  // process) loudly in every build.
```
The `impl->Optimize(log, policy)` call at 2592 is unguarded. §20 Phase-A item 3 ("Remove the broad `catch(...)`... make invariant failures reach the compilation boundary") is **already satisfied at tip** — no action needed this slice.

## 3. Typed-ID → concrete tip API table

| Proposed ID | Concrete tip source | Stability at pass slot | Verdict |
|---|---|---|---|
| **QueryOriginId** | `QueryViewImpl::det_seq` (`lib/DataFlow/Query.h:483`; public accessor `QueryView::DeterministicOrder()`, `include/.../Query.h:438`). Stamped **once**, dense `[0,N)`, at the head of `IdentifyInductions` (`lib/DataFlow/Induction.cpp:142-144`); the only other stamp site is inside `Optimize` (`lib/DataFlow/Optimize.cpp:288-290`), which runs **before** 2633. No re-stamp occurs between 2633 and the pass slot (grep-verified: `IdentifyInductions(` appears exactly once in Build.cpp, at 2633). Every live view (`ForEachView` over `joins/selects/tuples/kv_indices/maps/aggregates/merges/negations/compares/inserts`, `lib/DataFlow/Query.h:1034-1044`) gets exactly one. | STABLE, dense, total | **Has a stable tip source — reuse `det_seq` directly**, or wrap it in a typed `QueryOriginId{det_seq}` newtype (recommended per §4's "must not collapse into raw integers" rule — `det_seq` is currently `unsigned`, untyped). |
| **OriginUseId** | No existing per-edge object. Two composable primitives exist: (a) `QueryView::Predecessors()/Successors()` (`include/.../Query.h:477-478`) — view-granularity edges only, no column/role; (b) `QueryView::ForEachUse(std::function<void(QueryColumn in_col, InputColumnRole role, std::optional<QueryColumn> out_col)>)` (`include/.../Query.h:488-490`) — the column-granular, role-tagged (`InputColumnRole` enum, `include/.../Query.h:245-302`: `kCopied/kJoinPivot/kCompareLHS/kMaterialized/kPublished/...`) producer→consumer edge enumerator, defined per concrete view kind (e.g. `QuerySelect::ForEachUse`, `:537-539`, only fires when a corresponding INSERT exists). Root "boundary obligations" (§7.2 items 1/5) are NOT views at all: `QueryInsert` (`:861-896`, `Declaration()`), and bound `#query` reads are discovered by filtering `Query::Relations()`/`query->relations` for `decl.IsQuery() && decl.Arity()` with a bound parameter (the exact idiom `lib/DataFlow/Demand.cpp:466-475` already uses). | Reconstructible but **no stable ID exists yet** | **NEEDS a new interner.** `ForEachUse` order is documented "no guarantees... assume worst-case order" (`:486-487` NOTE) — an `OriginUseId` catalog must walk in `det_seq`-then-column-index order (mirroring the Format.cpp det_seq-bijection dumps) and mint one entry per `(consumer QueryOriginId, InputColumnRole-classified use)` plus one per root obligation (INSERT/publication/bound-query). |
| **LogicalCollectionId** | `QueryRelationImpl::declaration` (`lib/DataFlow/Query.h:135`, `const ParsedDeclaration`) — QueryRelation is already effectively 1:1 with a logical collection (`QueryImpl::decl_to_relation` map keyed by `ParsedDeclaration`, `lib/DataFlow/Query.h:1169`; public enumerator `Query::Relations()`, `include/.../Query.h:1114`). Stream/message-backed collections: `QueryIOImpl` via `Query::IOs()` (`:1121`), also decl-carrying. `QueryInsertImpl` (both relation- and stream-backed ctors, `lib/DataFlow/Query.h:1012-1013`) stores its own `const ParsedDeclaration declaration` (`:1025`) independent of which stream/relation it targets — the uniform accessor. `ParsedDeclaration::Id()` (`include/drlojekyll/Parse/Parse.h:429`) is confirmed **stable across every redeclaration** — it reads through the shared `context->id` (`lib/Parse/Parse.cpp:321-328`: `auto &id = context->id`), so multiple `#local`/`#export` redeclarations of the same relation collapse to one id (this is the exact mechanism `origin_decls` sorted-unique-by-`Id()` already relies on). | STABLE, but **sparse/hash-shaped**, not dense | `decl.Id()` is a sound *source*, but is a 64-bit packed/hashed value (`ParsedDeclarationImpl::Id`, `lib/Parse/Parse.cpp:321-...`, module-id-embedded for locals), not a dense `[0,N)` index — mint a fresh interner (`decl.Id() → LogicalCollectionId`, dense, insertion-order or `det_seq`-of-first-reaching-view order) exactly the P5 `schema_table`/P6.2 `symbolic_field_table` precedent (`docs/proposals/RegionalDataFlowCore...` interner idiom cited in CLAUDE.md). **Caveat**: `EquivalenceSetId()`/`EquivalenceSetViews()` (`lib/DataFlow/Query.cpp:234-248`) is the *physical storage* grouping (union-find over `equivalence_set`) and is explicitly coarser — co-recursive relations (e.g. `ping`/`pong`) share one `EquivalenceSetId` but must remain **two distinct `LogicalCollectionId`s** (§4's "must not be confused with storage resource" is empirically real at tip, not hypothetical). |
| **DerivationSiteId** | **No stable tip source at the Query-graph level.** No `QueryViewImpl` field retains a `ParsedClause` handle or any per-clause origin (`grep ParsedClause lib/DataFlow/Query.h` → zero struct-field hits; only a comment on `Color`/highlighting). CSE structurally merges views from distinct clause heads (documented first-hand precedent: P6.2's own grounding note, `lib/Regional/Planning.cpp:418-429`, "the post-Optimize graph CSE-merges co-recursive relations onto one model table... and LOSES per-relation field identity, whereas the parsed clauses retain it — the SAME lesson"). The one existing analog, `RuleId`/`RuleRoutingProjection` (`include/drlojekyll/Regional/RegionInstance.h:172-176,405`; built by `BuildRuleRoutingProjections`, `lib/Regional/Planning.cpp:430-474`), is **parse-level, not Query-level**: it iterates `head_decl.Clauses()` (`:447`) per **frozen relation** (`R.relation_schemas`, itself a Tier-1/Tier-2 row-contract subset computed later in `lib/Regional`, not available inside `Query::Build`) and mints one `RuleId` per clause via a dense per-clause ordinal (`next_rule++`, `:449`). | **NO stable source — genuine gap** | Two candidate resolutions to flag for the design stage, mutually exclusive: **(a) Query-native, coarse**: define `DerivationSiteId` per merged `QueryInsertImpl` occurrence into a `LogicalCollectionId` (available today: `query->inserts`/`Query::Inserts()`, `include/.../Query.h:1115`) — cheap, but loses the "one per rule" granularity CSE has already erased, so two distinct source clauses that CSE-merged their INSERT arms become indistinguishable. **(b) Parse-native, fine-grained**: lift the `RuleRoutingProjection`/`RuleId` pattern out of `lib/Regional` into `lib/DataFlow` (or a shared header) and re-scope it from "frozen relations only" to "every `ParsedDeclaration` with `Clauses()` reachable from the built `QueryImpl` (via `decl_to_relation`/`decl_to_input`)" — this is a parallel parse-level enumeration, **not** a QueryView traversal, and duplicates the P6.2 clause-source lesson at DataFlow scope instead of Regional scope. Phase A's exit gate ("every Query use and derivation site is stable and total across the corpus") cannot be met by (a) alone if the design intends "rule/head contribution" literally; needs an explicit owner call. |

## 4. Column-lineage / equality-class helpers (for §5.1 `ContextSignature` proofs — inventory only, not built this task)

Available primitives a future context-transfer pass would compose:
- `QueryColumn::Id()` (`include/.../Query.h:91`) — per-column identity, finalized by `impl->FinalizeColumnIDs()` (Build.cpp:2637, **runs after** our proposed slot's predecessor `IdentifyInductions` but **before** the slot itself — already final at 2679).
- `QueryColumn::Index()` (`:95`), `QueryColumn::Variable()` (`:78`, source `ParsedVariable` when traceable), `QueryColumn::Type()` (`:79`).
- `QueryColumn::ForEachUser`/`NumUses` (`:85-88`) for forward fan-out.
- `QueryView::ForEachUse` (`:488-490`) with `InputColumnRole` (`:245-302`) is the sole *typed* role classifier already in tip — `kJoinPivot` vs `kJoinNonPivot`, `kCompareLHS/RHS`, `kAggregateGroup/Config/Aggregated`, `kMaterialized`, `kPublished` map fairly directly onto §7.4's `transfer_context` match arms (JOIN pivot distribution, COMPARE preserve, AGGREGATE group-slot preserve, INSERT/message block).
- **No existing equality-class union-find over columns** (join-pivot equality classes, §5.1's `EqualityClassProof`) — `QueryJoin::NthInputPivotSet`/`PivotColumns` (`:560,580-585`) gives per-join pivot grouping but nothing global/transitive across chained joins. This is a genuine new helper for item (2) of Phase A, not yet present at tip.

## 5. Summary of gaps flagged for the design stage

1. **DerivationSiteId has no Query-graph-level source** — the P6.2 lesson (clause identity dies under CSE) applies identically here; needs an explicit (a)-vs-(b) decision (§3 above).
2. **OriginUseId and LogicalCollectionId both need new dense interners** — sources exist (`ForEachUse`/root-obligation enumeration; `decl.Id()`) but neither is currently dense/typed.
3. **QueryOriginId can reuse `det_seq` almost as-is** — cheapest win, already dense/total/stable at the slot; just needs a typed wrapper.
4. **Global column equality-class union-find is unbuilt** — required by §5.1 but absent from both DataFlow and Regional layers today.
5. Confirmed the two structural preconditions from the task brief: catch(...) removal is **already landed** (Build.cpp:2585-2591 comment), and the insertion slot is the four-line window Build.cpp:2679-2682.

Files read (no writes made): `/Users/pag/Code/DrLojekyll/lib/DataFlow/Build.cpp` (~2400-2682), `/Users/pag/Code/DrLojekyll/include/drlojekyll/DataFlow/Query.h` (full identity/enumeration surface), `/Users/pag/Code/DrLojekyll/lib/DataFlow/Query.h` (`QueryImpl`, `QueryViewImpl`, `QueryInsertImpl`, `QueryRelationImpl` internals), `/Users/pag/Code/DrLojekyll/lib/DataFlow/Query.cpp` (`DeterministicOrder`, `EquivalenceSetId`), `/Users/pag/Code/DrLojekyll/lib/DataFlow/Induction.cpp` / `Optimize.cpp` (`det_seq` stamp sites), `/Users/pag/Code/DrLojekyll/include/drlojekyll/Parse/Parse.h` + `/Users/pag/Code/DrLojekyll/lib/Parse/Parse.cpp` (`ParsedDeclaration::Id()` stability), `/Users/pag/Code/DrLojekyll/lib/DataFlow/Demand.cpp` (bound-`#query` enumeration idiom), `/Users/pag/Code/DrLojekyll/include/drlojekyll/Regional/RegionInstance.h` + `/Users/pag/Code/DrLojekyll/lib/Regional/Planning.cpp` (`RuleId`/`RuleRoutingProjection` precedent), `/Users/pag/Code/DrLojekyll/docs/proposals/InstanceFlow.md` (§4, §7, §20).