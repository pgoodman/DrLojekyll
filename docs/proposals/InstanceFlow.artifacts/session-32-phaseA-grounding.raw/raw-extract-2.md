# IF3 — Column Lineage + Equality Classes: Grounding Notes

Scope: read-only survey of `include/drlojekyll/DataFlow/Query.h` + `lib/DataFlow/Query.cpp` (public accessor impls) + `lib/DataFlow/Build.cpp` `BuildEquivalenceSets` (`:2303-2519`, file-static inside the anonymous namespace `:35-2522`, called at `:2645`, right before `Stratify`). No new code written.

## (1) Enumerating every producer→consumer OriginUse

The single, complete, dispatch-correct enumerator is the polymorphic base method already used by every existing pass:

```cpp
// Query.h:488-490 (declared on QueryView, the common base)
void QueryView::ForEachUse(std::function<void(QueryColumn /*in*/, InputColumnRole,
                                               std::optional<QueryColumn> /*out*/)> with_col) const;
```

Impl at `Query.cpp:489-525` dispatches by kind to nine per-kind `ForEachUse` overloads (Select/Join/Map/Aggregate/Merge/Compare/Insert/Tuple/KVIndex — Negate has one too, listed separately at `Query.cpp:1471`, not reached through the dispatcher's `else` chain because `QueryView::ForEachUse` currently has no `AsNegate()` arm — **confirmed gap**, see §(4)). Driving `impl->ForEachView(...)` (the whole-graph walker already used by `BuildEquivalenceSets`) × each view's `ForEachUse` gives every **consumer-side** edge: `(producer_column, role, consumer_output_column_or_nullopt)`.

To get the producer's *view* and *ordinal* (not just the column object) from `producer_column`:

```cpp
QueryView producer = QueryView::Containing(in_col);   // Query.cpp:126-128, O(1)
std::optional<unsigned> ord = in_col.Index();           // Query.h:95, Query.cpp:604
```

Per-kind shapes actually observed (load-bearing for coverage counting — an edge-count, not a view-count):

| Kind | Edges emitted | Role(s) | Notes |
|---|---|---|---|
| `SELECT` (`Query.cpp:761-777`) | one per `(insert, column)` pair, **only if the relation has INSERTs** (doc comment `Query.h:535-536` literally warns this) | `kCopied` | A SELECT's "producer" is every live INSERT on its relation — N inserts × M columns edges, not 1. This is the relation-mediated fan-in that becomes `LogicalCollectionId` membership (§3). |
| `JOIN` (`Query.cpp:780-806`) | one per `(out_col, member of out_to_in[out_col])` | `kJoinNonPivot` (set size 1) / `kJoinPivot` (set size ≥2) | See §2. |
| `MAP` (`Query.cpp:966-993`) | one per bound-functor-param input, plus one per attached/copied column | `kFunctorInput`, `kCopied` | `free`-param output slots get **no edge at all** — they're `MappedColumns()` with no producer, exactly the design's "Derive output slots only with a pure deterministic expression certificate" (§7.4 MAP rule) — the absence of an edge *is* the certificate obligation. |
| `AGGREGATE` (`Query.cpp:1138+`) | per group/config/aggregated input col | `kAggregateGroup`/`kAggregateConfig`/`kAggregatedColumn` (per role enum `Query.h:283-292`) | see §2. |
| `MERGE` (`Query.cpp:1204-1223`) | one per `(out_col, merged_view)` for **every** merged view | `kMergedColumn` | If a merged arm is itself an `INSERT`, the in_col is `insert->input_columns[i]` (skips straight past the INSERT node) — a merge-of-insert reads the insert's stored value, not a defined output column. |
| `COMPARE` (`Query.cpp:1369+`) | LHS/RHS + copied cols | `kCompareLHS`/`kCompareRHS`/`kCopied` | |
| `NEGATE` (`Query.cpp:1471+`) | copied + negated cols | `kCopied`/`kNegated` | **Not reachable via `QueryView::ForEachUse`** — call `QueryNegate::ForEachUse` directly after `impl->AsNegate()` (§4). |
| `INSERT` (`Query.cpp:1556-1590`) | stream: one per input col, `out=nullopt`; relation with no successors: one per input col, `out=nullopt`; relation with successors: one per `(user, out_col)` for each MERGE/SELECT successor | `kPublished` / `kMaterialized` | **This is where the design's root obligation (1) "each INSERT, publication, message... use" lives**: an INSERT with `out=nullopt` is a *terminal* OriginUse — the boundary itself, not a Query-internal edge. Asserts `false` if a successor is neither Merge nor Select (`:1580`) — i.e. every live INSERT successor is provably one of those two kinds today. |
| `TUPLE` (`Query.cpp:1640-1649`) | positional 1:1, `input_columns[i] → columns[i]` | `kCopied` always | See §3 — this is the bijective-forwarding candidate. |
| `KVINDEX` (`Query.cpp:1727+`) | key + value cols | `kIndexKey`/`kIndexValue` | |

**Coverage-proof shape for §8**: iterating `impl->ForEachView` × `ForEachUse` (+ the Negate patch) visits exactly the edge multiset the design calls `OriginUseId`s; `V-IF-COVERAGE` reduces to "every emitted edge from this walk is assigned to exactly one family occurrence's coverage domain" — the walk itself is already exhaustive and needs no new graph traversal, only a `QueryNegate` dispatch fix in the enumerator IF3 (or its consumer) writes.

## (2) JOIN pivot equality classes and AGGREGATE group keys as candidate seeds

**JOIN.** `impl->out_to_in` (a `std::unordered_map<COL*, UseList<COL*>>`, populated at join-build time) is *already* the equality-class map: an output pivot column's use-list is the full set of producer columns (one per joined view) that must compare equal. Public read path:

```cpp
for (QueryColumn pivot : join.PivotColumns())     // Query.h:560, first NumPivotColumns() of impl->columns
  // pivot.Index() gives n; or iterate by index directly:
for (unsigned n = 0; n < join.NumPivotColumns(); ++n) {
  QueryColumn out = join.NthOutputPivotColumn(n);           // Query.cpp:857-861
  for (QueryColumn member : join.NthInputPivotSet(n))       // Query.cpp:832-839, asserts size()>1
    ; // member = one producer column in the equality class; QueryView::Containing(member) = its view
}
```
`NumJoinedViews()`/`JoinedViews()` (`Query.cpp:820-827`) gives the join arity for cross-checking that every pivot's equality-class size is ≤ `NumJoinedViews()` (a non-pivot/`kJoinNonPivot` set always has exactly 1 member, asserted at `Query.h` comment `:565-567` "zero pivots ⇒ cross-product"). This is the direct, no-recomputation source for `CandidateSeed::JoinPivot(join_origin, equality_classes)` (§7.3) — the compiler already materializes exactly this structure, IF3 only needs to *render* it as a candidate seed, not derive it.

**AGGREGATE.** Group columns are a contiguous input-side prefix, independently addressable:

```cpp
agg.InputGroupColumns()     // Query.cpp:1042-1046, UsedNodeRange<QueryColumn>
agg.NumGroupColumns()       // Query.cpp:1066-1068
agg.NthInputGroupColumn(n)  // Query.cpp:1111-1114
```
maps straight to `CandidateSeed::AggregateGroup(aggregate_origin, group_columns)`. `KVIndex` has the parallel `InputKeyColumns()`/`KeyColumns()` (`Query.h:941-956`) for the same purpose if a KV-index ever needs a candidate seed (design §7.2 item 3 only names join pivots and aggregate groups explicitly; KV-index keys are structurally identical and worth treating the same way, though out of the ratified Phase-A/B.1-B.2 slice — flag for the design stage, not build it here).

`ConsumptionGroup(consumer_origin, group_columns)` (root obligation 2: `ONLY`/`EXISTS`/`CHOOSE_ONE`) — **not found** as a first-class Query node kind in `Query.h`'s enumerated view classes (`QuerySelect/Tuple/KVIndex/Join/Map/Aggregate/Merge/Negate/Compare/Insert` is the exhaustive `QueryView(const Query*&)` constructor list at `Query.h:329-338`). These operators are not yet separately modeled at the Query IR level in this tip; flagged in §(4).

## (3) EquivalenceSetId → LogicalCollectionId vs ForwardingEquivalence

`QueryView::EquivalenceSetId()`/`EquivalenceSetViews()` (`Query.h:365-366`, impl `Query.cpp:234-239` + nearby) read a union-find (`lib/DataFlow/EquivalenceSet.h`) built once by `BuildEquivalenceSets` (`Build.cpp:2303-2519`, called `:2645`, **before** `Stratify` and well before the row-contract/IF3 pass slot at `:2654+`). The union-find (`EquivalenceSet::Find`/`TryUnion`, `EquivalenceSet.h:17-66`) stores only a flat `parent` pointer + `views_in_set` (a `WeakUseList<VIEW>`) — **no per-merge provenance is retained**. Once two views land in the same root, you cannot ask "why" from `EquivalenceSetId`/`EquivalenceSetViews()` alone.

The merges `BuildEquivalenceSets` performs, in order, are exactly the two kinds §10 distinguishes:

1. **`LogicalCollectionId` source** — same-relation INSERT/SELECT sharing (`Build.cpp:2386-2406`: every `rel->inserts` and `rel->selects` of one `QueryRelation` union into one model), plus the INSERT↔guard-TUPLE union (`:2408-2428`, skipped when the insert has `attached_columns`) and SELECT↔predecessor-INSERT union (`:2430-2442`) — all anchored to relation identity.
2. **`ForwardingEquivalence` source** — the bijective-TUPLE check (`Build.cpp:2475-2482`): `view.IsTuple() && preds.size()==1 && all_cols_match(tuple.InputColumns(), pred.Columns())`, and its NEGATE→successor-TUPLE mirror (`:2489-2498`). `all_cols_match` (`:2319-2334`) is strict: equal length **and** `succ_col.Id() == pred_col.Id()` at every position — i.e. the TUPLE reproduces its single predecessor's entire column list, in order, with no drop/permute/duplicate. (Contrast: `QueryTuple::ForEachUse`, `Query.cpp:1640-1649`, is only positionally 1:1 within the TUPLE's own `input_columns[i] → columns[i]`; it says nothing about whether `input_columns` as a *set* equals the predecessor's full column list — `all_cols_match` is the stronger, separate check that actually certifies bijectivity.)

Both merge families get unioned into the *same* `EquivalenceSetId` via ordinary union-find, so **the mapping IF3 needs (LogicalCollectionId vs ForwardingEquivalence) cannot be read back out of `EquivalenceSetId`/`EquivalenceSetViews()` at tip** — recovering it means re-deriving the two predicates independently, exactly as §10 prescribes ("useful migration evidence, not future authority"):

- `LogicalCollectionId` ← assign one per `QueryRelation` (iterate `query->relations`; `QueryRelation::Inserts()`/`Selects()`, `Query.h:130,133`), independent of `EquivalenceSetId` entirely.
- `ForwardingEquivalence` ← a local, per-view structural predicate re-implementing `all_cols_match` (TUPLE-single-pred-exact-match, and the NEGATE→TUPLE mirror), independent of `EquivalenceSetId` entirely.
- `EquivalenceSetId` itself is then only useful as a **cross-check**: every `EquivalenceSetViews()` group should decompose into exactly one relation-anchored `LogicalCollectionId` core plus zero or more `ForwardingEquivalence` hangers-on; a group that doesn't decompose that way is new/unexplained sharing logic added since this note and needs re-grounding before IF3/IF-whatever trusts it.

## (4) What the tip cannot answer / flags for the design stage

- **`QueryView::ForEachUse` omits NEGATE.** The dispatcher at `Query.cpp:489-525` has no `impl->AsNegate()` arm (falls through to `assert(false)` at `:523`). `QueryNegate::ForEachUse` exists (`Query.cpp:1471`) but must be called directly by any generic walker that wants to enumerate every view kind uniformly — a real gap the flat-grove builder must special-case, not something already handled.
- **No first-class `ConsumptionGroup` node.** §7.2 root obligation 2 (`ONLY`/`EXISTS`/`CHOOSE_ONE`) has no corresponding `QueryView` subclass in this tip's `Query.h`. Whether these exist as parse-level sugar lowered into MAP/COMPARE/AGGREGATE shapes, or are simply unimplemented in DataFlow today, needs a targeted grep/grounding pass of its own before Phase A/B.1-B.2 can emit that candidate-seed class — out of what this note can certify from `Query.h`/`Query.cpp` alone.
- **`BuildEquivalenceSets`/`all_cols_match` are TU-local** (anonymous namespace, `Build.cpp:35-2522`), not exported via any header. A new IF3-consuming pass cannot call the existing bijectivity check — it must reimplement the identical predicate (cheap: ~10 lines) rather than reuse code, or a small header-visible extraction would need explicit sign-off (touches Build.cpp's existing anonymous-namespace boundary).
- **`QueryColumn::Index()` is `std::optional<unsigned>`, empty for constants** (`Query.h:93-95`): any lineage walker that hits a `QueryConstant`-sourced column (not merely a view-defined one — constants are a separate `Node` hierarchy, `Query.h:171-194`) needs a distinct "constant origin" case, not a `(view, ordinal)` pair. Not exercised in the snippets read here; flag for whoever writes the actual lineage-walk code.
- **`InputColumnRole` has no explicit MERGE-arm-is-an-INSERT marker**: `QueryMerge::ForEachUse` (`Query.cpp:1213-1217`) silently special-cases `view->AsInsert()` to reach into `insert->input_columns[i]` instead of `view->columns[i]`; this bypasses the normal "defined column" identity (an INSERT has no output columns at all — `QueryInsert` is absent from the `QueryColumn`-defining-view union). Any lineage function built on "get producing view of a column" must special-case this the same way MERGE's own `ForEachUse` does, or it will mis-attribute provenance for merge arms fed directly by an INSERT.