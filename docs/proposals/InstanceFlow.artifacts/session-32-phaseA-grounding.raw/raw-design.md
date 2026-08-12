All anchors verified against tip. Synthesizing the design.

---

# InstanceFlow Phase A + Flat Grove — Whole-Slice Design (buildable against `keyed-instances` tip)

Scope: Phase A identity catalogs + column-lineage/equality helpers, the empty-context FlatFamily grove, `-instanceflow-out`, and the four always-on validators. **Observer at the Query→Rel seam — nothing consumes the grove; codegen stays byte-identical.** Confirmed preconditions: the `catch(...)` removal is already landed (`lib/DataFlow/Build.cpp:2585-2591` note; the `Optimize` call at `:2622` is unguarded), and the insertion slot is the window `Build.cpp:2657→2659` (right after `ValidateRowContracts`, before the `#ifndef NDEBUG` K5 belt at `:2659`, before `return Query(...)` at `:2682`).

---

## 1. FILE LAYOUT — recommendation: **new `lib/InstanceFlow/` + `include/drlojekyll/InstanceFlow/`** static-lib target

**Decision: separate library, not under `lib/DataFlow`.** Rationale, from the two governing constraints:

- **Acyclic dep rule** `bin → {ControlFlow, Rel} → Regional → DataFlow`. InstanceFlow sits *between* Query and Rel (§0, §11). It must **depend on DataFlow** (it reads the final `QueryImpl`) and will eventually be **depended on by Rel** (Phase C). Placing it in `lib/DataFlow` would force Rel→DataFlow to import InstanceFlow symbols transitively and would make the eventual "Rel consumes InstanceFlow, not `TABLE*`" cutover a same-library entanglement. A distinct node `lib/InstanceFlow` slots cleanly as `Rel → InstanceFlow → DataFlow`, mirroring the `lib/Rel` split precedent (CLAUDE.md: "its own compiler-internal static-library target").
- **The `-contract-out` precedent** *does* keep Stage-A row contracts inside `lib/DataFlow` — but row contracts are terminal Stage-A metadata with no downstream IR consumer. InstanceFlow is explicitly a **new IR** (§1) with a planned Rel reader, so the `lib/Rel` precedent (own target, `SetRelDumpStream`-style narrow public seam) is the better fit than the `-contract-out` one.

**Concession to keep this slice small:** the grove is *stored on* `QueryImpl` (like `row_contracts`) because the pass runs inside `Query::Build` and the drain happens off `Query`. To avoid a `DataFlow → InstanceFlow` back-edge, store it **by opaque forward-declared pointer**:

```cpp
// lib/DataFlow/Query.h, near row_contracts (:1234-1240)
struct InstanceFlowProgram;                         // fwd decl only
std::unique_ptr<InstanceFlowProgram> instance_flow; // owned; defined in lib/InstanceFlow
```

`lib/DataFlow` needs only the incomplete type + a `unique_ptr` (its destructor is emitted in `lib/InstanceFlow`, so `QueryImpl`'s dtor must be out-of-lined or the header must see the complete type at the TU that destroys it — simplest: give `QueryImpl` an out-of-line dtor in `Query.cpp` that `#include`s the InstanceFlow header). `BuildInstanceFlowGrove`/`ValidateInstanceFlow` are declared in the InstanceFlow public header and *called from* `Build.cpp`, which already links against neighbouring libs — the call is `lib/DataFlow → lib/InstanceFlow`, i.e. a **cycle** with the storage edge above.

**To break that cycle cleanly:** the *build + validate* call sites move **out of `Query::Build`** and into a thin shim that `Query::Build` invokes through a function pointer, OR — simpler and what I recommend — **the pass is invoked from `bin/drlojekyll` and Stage-B, not from `Build.cpp`**. But the ratified slot is "Query::Build tail." Resolve by making `lib/InstanceFlow` depend on `lib/DataFlow` **only**, and having `lib/DataFlow` expose a *weak extension hook*:

```cpp
// include/drlojekyll/DataFlow/Query.h  (DataFlow-owned, no InstanceFlow include)
namespace hyde {
// Set by lib/InstanceFlow at static-init or by an explicit wire call in main();
// nullptr in a DataFlow-only link (tests that don't pull InstanceFlow).
extern bool (*gInstanceFlowTailPass)(QueryImpl *, const ErrorLog &);
}
```

`Build.cpp` tail calls `if (gInstanceFlowTailPass && !gInstanceFlowTailPass(impl.get(), log)) return std::nullopt;`. `lib/InstanceFlow` provides the definition and a `WireInstanceFlow()` called once from `bin/drlojekyll/Main.cpp` (and from any test main that wants the grove). This keeps DataFlow→InstanceFlow at **zero link-time dependency** (function pointer, defaulted null), InstanceFlow→DataFlow as the only real edge, and preserves the exact Build.cpp slot. **Flag this hook indirection for the refuter panel** (Risk R1) — the alternative (fold everything into `lib/DataFlow`, accept the Rel-cutover cleanup as Phase-C debt) is a legitimate simpler choice the owner may prefer.

Files:
```
include/drlojekyll/InstanceFlow/InstanceFlow.h   # public IDs, InstanceFlowProgram, Build/Validate decls, QueryInstanceFlow tag
include/drlojekyll/InstanceFlow/Identity.h        # typed-ID domain structs (mirrors lib/DataFlow/Identity.h)
lib/InstanceFlow/Build.cpp                        # BuildFlatInstanceFlow + the 4 catalogs
lib/InstanceFlow/Validate.cpp                     # ValidateInstanceFlow (V-IF-*)
lib/InstanceFlow/Format.cpp                       # operator<<(OutputStream&, QueryInstanceFlow)
lib/InstanceFlow/CMakeLists.txt                   # static lib, links drlojekyll_dataflow
```

---

## 2. PHASE-A ID CATALOGS

### 2.1 Typed-ID domain structs (mirror `lib/DataFlow/Identity.h` — never raw `unsigned`)

Per §4 ("must not collapse into raw integers") and the `Identity.h` idiom (each struct owns its domain, defaulted intra-domain `==`/`<=>` only, no cross-domain operator):

```cpp
// include/drlojekyll/InstanceFlow/Identity.h
namespace hyde {

// A live post-optimization Query view occurrence. Dense [0,N). SOURCE: the
// view's det_seq (QueryView::DeterministicOrder()), which is already dense,
// total and stable at the pass slot (stamped once at IdentifyInductions head,
// no re-stamp before the slot — IF1 §3).
struct QueryOriginId { uint32_t v;
  constexpr bool operator==(const QueryOriginId&) const noexcept = default;
  constexpr auto operator<=>(const QueryOriginId&) const noexcept = default; };

// One producer→consumer column-role edge, OR one root boundary obligation
// (terminal INSERT / publication / bound-query read). Dense [0,M). No tip
// source — minted by a canonical walk (below).
struct OriginUseId { uint32_t v;  /* == , <=> defaulted */ };

// A logical set-valued collection (§8.1 "same set-valued truth"). Dense [0,C).
// SOURCE: interner over QueryRelation identity (ParsedDeclaration::Id()).
// MUST NOT be conflated with EquivalenceSetId (co-recursive ping/pong share one
// storage set but are two collections — IF1 §3 caveat).
struct LogicalCollectionId { uint32_t v; /* ... */ };

// One live INSERT view into a collection (§ IF4(c): "1 per live INSERT view
// post-optimization", NOT per parse rule — CSE folds/keeps rule provenance
// unpredictably). Dense [0,D).
struct DerivationSiteId { uint32_t v; /* ... */ };

// Flat-grove-local minted ids (dump/trace only; dense, canonical-order).
struct FamilyId          { uint32_t v; /* ... */ };  // exactly one this slice: if#0
struct FamilyNodeId      { uint64_t v; /* packed (FamilyId, local) */ };
struct EmissionAuthorityId { uint32_t v; /* ... */ };  // 1:1 with DerivationSiteId this slice
struct QuerySccId        { uint32_t v; /* ... */ };  // dense remap of InductionGroupId
}
```

`DerivationSiteId` resolution — **adopt IF1 candidate (a), the Query-native coarse one**, explicitly. Phase A's flat baseline does not need per-rule granularity (the flat grove has no contextual split, and §8.4's authority partition is stated per `(DerivationSiteId, CoverageDomain)`; one authority per live INSERT is exactly-one-writer by construction). IF4(c) proves the parse-rule count is *not* recoverable post-CSE anyway (`tc`:2 rules→2 INSERTs, `is_node`:2 rules→1 INSERT), so "one `DerivationSiteId` per live `QueryInsertImpl`" is the only *stable, total* definition — which is what the Phase-A exit gate demands. Candidate (b) (lifting `RuleRoutingProjection` to DataFlow scope) is deferred to whenever contextual specialization needs rule-level provenance; **record as Risk R2**.

### 2.2 The four catalogs (structs) and where populated

All populated inside `BuildFlatInstanceFlow(QueryImpl*)` at the Build.cpp tail slot, in this order (each later catalog may reference earlier ids):

```cpp
// include/drlojekyll/InstanceFlow/InstanceFlow.h
struct QueryOriginCatalog {          // det_seq bijection onto live views
  // origin[det_seq] -> the QueryView (by det_seq); dense, total.
  std::vector<QueryOriginId> ids;               // ids[i].v == i (identity, but typed)
  std::vector<QueryViewKind> kind;              // for the dump's role token
  // reverse lookups the builder needs:
  QueryOriginId Of(QueryView v) const { return {v.DeterministicOrder()}; }
};

struct OriginUse {
  OriginUseId       id;
  QueryOriginId     consumer;      // the view that USES (nullopt-out => root obligation)
  QueryOriginId     producer;      // QueryView::Containing(in_col) as origin
  uint32_t          producer_col;  // in_col.Index() (UINT32_MAX for constant-sourced)
  InputColumnRole   role;          // typed role
  bool              is_root;       // terminal INSERT/publication/bound-query
  DerivationSiteId  terminal_site; // valid iff is_root (the boundary's collection writer)
};
struct OriginUseCatalog { std::vector<OriginUse> uses; };

struct LogicalCollectionCatalog {
  std::vector<ParsedDeclaration> decl;   // decl[lc.v] = the collection's declaration
  std::unordered_map<uint64_t, LogicalCollectionId> by_decl_id;  // decl.Id() -> lc
};

struct DerivationSiteCatalog {
  std::vector<QueryOriginId>       insert_origin;  // the QueryInsert view
  std::vector<LogicalCollectionId> collection;     // which LC it writes
};
```

**Population — exact tip APIs, canonical order (§16.1 / HP-9: stable Query order, never pointer/iteration order):**

1. **`QueryOriginCatalog`** — walk with the **same kind-tagged `for_each_view` lambda** verbatim from `Format.cpp:1553-1573` (Selects→Tuples→…→Inserts, `is_dead`-skipped). Each live view's `QueryOriginId = {v.impl->det_seq}`. Reuse of that exact traversal guarantees the catalog is a `det_seq` bijection (the same tripwire V-CONTRACT-CENSUS already relies on). Records `kind` from the tag enum.

2. **`LogicalCollectionCatalog`** — iterate `query->relations` **in stored order** (already deterministic), then `query->ios` (message/stream-backed) — for each, `decl.Id()` → intern a fresh dense `LogicalCollectionId` in first-seen order. (P5 `schema_table`/P6.2 `symbolic_field_table` interner precedent.) `decl.Id()` is stable across redeclarations (IF1 §3, `Parse.cpp:321-328`), so this is total and stable.

3. **`DerivationSiteCatalog`** — iterate `query.Inserts()` in stored order (det_seq-monotone within kind); for each live `QueryInsert`, one `DerivationSiteId`; its `collection` = `by_decl_id[insert.Declaration().Id()]` (the uniform `QueryInsertImpl::declaration` accessor, IF1 §3). One writer per site by construction.

4. **`OriginUseCatalog`** — the central walk. Drive the `for_each_view` traversal in `det_seq` order; for each view emit uses **sorted by `(consumer det_seq, producer_col index, role)`** so ordering is canonical regardless of `ForEachUse`'s documented "no guarantees" iteration (IF1 §3 / IF3 §1). For each view `v`:
   - Call `v.ForEachUse([&](QueryColumn in, InputColumnRole role, std::optional<QueryColumn> out){...})`.
   - **NEGATE patch (IF3 §4, load-bearing):** `QueryView::ForEachUse`'s dispatcher has no `AsNegate()` arm → `assert(false)`. The builder must special-case: `if (auto n = v.AsNegate()) n.ForEachUse(cb); else v.ForEachUse(cb);`.
   - producer view/ord: `QueryView producer = QueryView::Containing(in)` (`Query.cpp:126`, O(1)); `producer_col = in.Index().value_or(UINT32_MAX)` (constant-sourced → sentinel, IF3 §4).
   - **MERGE-arm-is-INSERT special case (IF3 §4):** when the merged arm view is an INSERT (no output columns), `QueryMerge::ForEachUse` already reaches into `insert->input_columns[i]`; `Containing(in)` yields that INSERT view as producer — record it as-is (the INSERT is a valid origin).
   - **Root obligations** (§7.2 items 1/5): a use with `out == std::nullopt` on an INSERT is a *terminal* boundary use (IF3 §1) — set `is_root=true`, `terminal_site` = the DerivationSite of that INSERT. **Bound-`#query` reads**: enumerate exactly as `Demand.cpp:466-475` (filter `query->relations` for `decl.IsQuery() && decl.Arity()` with ≥1 bound param) and mint one root use per bound read against the collection's LC.

**Determinism guarantee:** every catalog's order is a pure function of `det_seq` + stored relation/insert order + the fixed kind sequence. No `unordered_map` iteration is ever emitted (maps are lookup-only; all emitted vectors are built by the ordered walks).

### 2.3 Column-lineage + equality-class helpers (Phase A item 2 — inventory + the one new helper)

Read-only helpers on the catalogs, composing tip primitives (IF3 §2):

- **Lineage step**: `LineageOf(QueryColumn c) -> {QueryOriginId producer, uint32_t ord, bool is_constant}` via `QueryView::Containing` + `QueryColumn::Index`. Constants (`QueryConstant`, empty `Index()`) return `is_constant=true` (IF3 §4).
- **JOIN pivot equality classes** (seed for `JoinPivot`): read directly off `join.NthInputPivotSet(n)` / `NthOutputPivotColumn(n)` (`Query.cpp:832,857`) — the compiler *already* materializes the class as `out_to_in`; the helper only renders it. No new union-find needed for the flat baseline (transitive cross-join closure — §5.1's global `EqualityClassProof` — is **not built this slice**; flat has empty context).
- **AGGREGATE group key** (seed for `AggregateGroup`): `agg.InputGroupColumns()`/`NthInputGroupColumn(n)` (`Query.cpp:1042,1111`).

The seeds are **recorded, not acted on** (flat grove has empty context; §7.3 says candidates are the growth frontier, and Phase-A/B.1-B.2 never grows). Recording them now makes the dump reviewable and pins the seed vocabulary for Phase D.

---

## 3. FLAT GROVE DATA MODEL (minimal §6 subset for empty-context)

```cpp
// include/drlojekyll/InstanceFlow/InstanceFlow.h
struct ResidualSchema { std::vector<FieldId> fields; };  // = full logical schema this slice

struct FamilyNode {
  FamilyNodeId       id;
  QueryOriginId      origin;
  ResidualSchema     residual;          // FULL schema (empty-context baseline, §7.3)
  LogicalCollectionId output_collection;// valid only for INSERT-origin nodes
  std::optional<QuerySccId> scc;        // set iff origin has an InductionGroupId
  // transfers: EMPTY this slice; context: EMPTY this slice.
};

struct FlatFamily {                     // exactly ONE, FamilyId{0}, context=empty
  FamilyId id;
  std::vector<FamilyNode> nodes;        // one per QueryOriginId, det_seq order
  // edges mirror OriginUseCatalog 1:1 — not stored separately; derived on dump.
};

struct UseCoverage {
  OriginUseId  use;
  // domain = All(empty context) — a single tag this slice, no payload.
  FamilyNodeId occurrence;             // the CONSUMER node (or producer node for roots)
  EmissionAuthorityId authority;       // the terminal INSERT's authority
};

struct EmissionAuthority {
  EmissionAuthorityId id;
  DerivationSiteId derivation_site;
  FamilyNodeId    writer;              // the INSERT-origin node
  // domain = All (empty context)
};

struct CandidateSeed {                  // recorded, inert this slice
  enum Kind { kBoundaryBinding, kJoinPivot, kAggregateGroup } kind;
  QueryOriginId origin;                 // join/aggregate origin, or the bound query's read
  std::vector<uint32_t> columns;        // pivot/group/bound column ordinals
};

struct SccRegion {                      // one WholeQueryScc per InductionGroupId
  QuerySccId id;
  std::vector<QueryOriginId> members;   // origins sharing the group id (det_seq order)
};

struct InstanceFlowProgram {
  QueryOriginCatalog       origins;
  OriginUseCatalog         uses;
  LogicalCollectionCatalog collections;
  DerivationSiteCatalog    sites;
  FlatFamily               family;      // the single flat family
  std::vector<UseCoverage>       coverage;
  std::vector<EmissionAuthority> authorities;
  std::vector<CandidateSeed>     seeds;
  std::vector<SccRegion>         scc_regions;
  // provenance (TraceCatalog): this slice = the origin→OriginDecls() reuse only.
};
```

### 3.1 `BuildFlatInstanceFlow(QueryImpl*)` — pseudocode-to-C++ over tip APIs

```cpp
std::unique_ptr<InstanceFlowProgram> BuildFlatInstanceFlow(QueryImpl *impl) {
  auto p = std::make_unique<InstanceFlowProgram>();
  const Query query(impl);                       // wrap for the public accessors

  // (1) origins: det_seq bijection via the Format.cpp for_each_view lambda.
  for_each_view(query, [&](QueryView v, unsigned kind, const char*) {
    p->origins.ids.push_back({v.DeterministicOrder()});
    p->origins.kind.push_back(KindOf(kind));
  });

  // (2) collections: intern decl.Id() over relations then ios, first-seen order.
  for (QueryRelation r : query.Relations()) InternLC(p->collections, r.Declaration());
  for (QueryIO io : query.IOs())            InternLC(p->collections, io.Declaration());

  // (3) derivation sites + authorities: one per live INSERT, stored order.
  for (QueryInsert ins : query.Inserts()) {
    if (ins.impl->is_dead) continue;
    DerivationSiteId ds{(uint32_t)p->sites.insert_origin.size()};
    p->sites.insert_origin.push_back(p->origins.Of(QueryView::From(ins)));
    p->sites.collection.push_back(p->collections.by_decl_id[ins.Declaration().Id()]);
    p->authorities.push_back({ {ds.v}, ds, NodeIdOf(ins) });
  }

  // (4) family nodes: mirror the DAG 1:1 (§7.3 shared structure), full residual.
  for_each_view(query, [&](QueryView v, unsigned, const char*) {
    FamilyNode n; n.id = NodeIdOf(v); n.origin = p->origins.Of(v);
    n.residual = FullSchemaOf(v);                // all logical fields, FieldId order
    if (auto ins = v.AsInsert())
      n.output_collection = p->collections.by_decl_id[ins.Declaration().Id()];
    if (auto g = v.InductionGroupId()) n.scc = QuerySccId{RemapGroup(*g)};
    p->family.nodes.push_back(std::move(n));
  });

  // (5) uses + coverage: the canonical ForEachUse walk (+ NEGATE patch),
  //     sorted (consumer det_seq, producer_col, role). Each use -> one coverage
  //     record, authority = the terminal INSERT reached along its chain.
  BuildUsesAndCoverage(query, *p);               // §2.2 item 4

  // (6) seeds (inert): JoinPivot per join, AggregateGroup per aggregate,
  //     BoundaryBinding per bound #query read.
  RecordCandidateSeeds(query, *p);

  // (7) scc regions: group origins by InductionGroupId (§7.6 atomic).
  BuildSccRegions(query, *p);
  return p;
}
```

**§7.6 SCC handling (flat baseline):** each distinct `InductionGroupId` becomes one `SccRegion` (`WholeQueryScc`), and every member node carries that `QuerySccId` in `FamilyNode.scc`. Per IF4(c) Stress #1, the flat family legitimately contains **both** SCC-interior and SCC-exterior nodes in the single `FlatFamily`; the SCC boundary is tracked **per-`FamilyNode`** (`scc` optional field), not by splitting the family. This is the sound flat reading — the `FamilyTemplate.scc_ownership` template-level field (§6) is a *contextual-specialization* concept; at empty context there is one family and the SCC atomicity obligation is discharged by V-IF-SCC checking that every member of a group id is present and un-split (no contextual family ever half-covers it, vacuously true this slice). **Flag as Risk R3.**

**Constant-sourced / orphan / unsat origins (IF4(a) stresses):** every live post-optimization view gets a `QueryOriginId` and a `FamilyNode`, dead-ends included (`compare.14`/`compare.15` orphans, the `never` unsat arm). An orphan origin simply produces zero non-root uses and trivially satisfies V-IF-COVERAGE. Constant-sourced producer columns record `producer_col = UINT32_MAX` and are *not* treated as a normal `(view, ord)` edge. Provably-empty collections (`never`) get an ordinary LC — no cost tag this slice.

---

## 4. `-instanceflow-out` DUMP FORMAT

Grammar (flat case; adapts §16). Canonical ids from stable order: `q#<det_seq>` (origin), `u#<index>` (use, in canonical sort order), `lc#<index>`, `ds#<index>`, `ea#<index>`, `if#0` (the single family), `if#0.<node_local>` (node = origin det_seq), `scc#<index>`. Deterministic, goldenable, round-trip-reviewable.

```
instanceflow  origins=<N> uses=<M> collections=<C> sites=<D> family=1

collections
  lc#0 decl=tc/2
  lc#1 decl=is_node/1
sites
  ds#0 writer=q#13 -> lc#0
  ds#1 writer=q#14 -> lc#0
  ds#2 writer=q#15 -> lc#1

family if#0 context=empty scc-regions=<S>
  node if#0.0 origin=q#0 select  residual=(From,To)
  node if#0.10 origin=q#10 join  residual=(From,To)
  node if#0.13 origin=q#13 insert residual=(From,To) -> lc#0
  ...
  scc scc#0 members=(q#11,q#2,q#3,q#10,q#1)
  ...
coverage
  u#0 use q#0->q#9 role=copied col=0 domain=all occ=if#0.9 authority=ea#0
  u#7 root insert q#13 domain=all authority=ea#0
  u#8 root query reachable_from bound=(From) domain=all authority=ea#0
  ...
authorities
  ea#0 site=ds#0 domain=all writer=if#0.13
  ea#1 site=ds#1 domain=all writer=if#0.14
  ea#2 site=ds#2 domain=all writer=if#0.15
seeds
  seed join-pivot origin=q#10 columns=(X)
  seed boundary-binding query=reachable_from columns=(From)
  seed boundary-binding query=reaching_to columns=(To)
```

Emitter mirrors `QueryContracts` (`Format.cpp:1546+`): pull `qif.query.impl->instance_flow`, reuse the exact `for_each_view` det_seq traversal for node lines, buffer per block through the `std::ostringstream`/`OutputStream bos`/`take()` idiom (`Format.cpp:1599-1600`), and gate a redundant dump-time census belt (V-IF-COVERAGE reprised) in the same fprintf+abort shape as V-CONTRACT-CENSUS (`Format.cpp:1575-1597`). Tag struct + friend grant per IF2:

```cpp
// include/drlojekyll/InstanceFlow/InstanceFlow.h
struct QueryInstanceFlow { Query query; };
OutputStream &operator<<(OutputStream &os, QueryInstanceFlow qif);
```
plus the `friend` line in `class Query`'s private section (`include/drlojekyll/DataFlow/Query.h:1173-1176`, beside the `QueryContracts` friend).

**CLI wiring** (IF2 §1, verbatim `-contract-out` clone, renamed `-instanceflow-out`): global `gInstanceFlowStream` (`Main.cpp:~59`), local `instanceflow_out` unique_ptr (`~330`), flag-parse arm appended after the `-origin-out` arm (`~448`), help line after `-contract-out` (`~247`), and the drain **after the `-origin-out` drain, before `FrozenRegionalProgram::Build`** (`Main.cpp:~86`): `if (gInstanceFlowStream) { (*gInstanceFlowStream) << hyde::QueryInstanceFlow{*query_opt}; gInstanceFlowStream->Flush(); }`.

Golden witnesses (from IF4, the three hand-derivations pin the format): `join_1` (orphan origins, constant-pivot degenerate seeds, 2 collections/2 sites), `merge_2` (shared-origin: `select.1`/`select.2` each one node with two coverage records — the §7.3 maximal-sharing flagship; `@inline` origins absent), `transitive_closure` (one `scc#0` region of 5, `tc` = 2 sites into 1 lc, three boundary-binding seeds).

---

## 5. VALIDATORS — `ValidateInstanceFlow(QueryImpl*, const ErrorLog&)`

All always-on, `fprintf(stderr,…)+abort()` (survive NDEBUG — the Rel V-* precedent), called from the tail hook right after `BuildFlatInstanceFlow`. Return `bool` for uniformity with `ValidateRowContracts`, but a violation is an internal-invariant abort, not a user diagnostic (the grove is compiler-derived; a fire is a compiler bug, §17 "transactional... no catch-and-clean").

- **V-IF-ORIGIN** — *reads* `family.nodes` × `origins`. Obligation: every `FamilyNode.origin` is a live `QueryOriginId` (det_seq in `[0,N)`, view not `is_dead`) and `node.residual.fields.size()` equals the origin view's logical column count (schema-compatible). Also asserts the node set is a **det_seq bijection** onto live views (no origin missing, none doubled) — the coverage-of-nodes half.

- **V-IF-CONTEXT** (vacuous-empty this slice) — *reads* each node's `context`+`residual`. Obligation (§17): "residual schema plus context reconstructs the logical schema." With empty context, this reduces to `residual == full logical schema` for every node. Assert `context` is empty and `transfers` is empty on every node (the empty-context baseline is total — any non-empty context here is a builder bug, since this slice never grows).

- **V-IF-COVERAGE** (the central §8 use-partition invariant) — *reads* `uses` × `coverage`. Obligation: for every `OriginUseId u`, the union of coverage domains assigned to `u` equals `u`'s complete domain, and pairwise intersections are empty. Flat reduction: **every use has exactly one coverage record, domain=All** → check is "coverage is a bijection onto `uses`" (each `OriginUseId` appears in exactly one `UseCoverage`, and every `UseCoverage.use` is a live use id). This is the belt reprised in the dump emitter too.

- **V-IF-EMISSION** (exact-one authority, §8.4) — *reads* `authorities` × `sites`. Obligation: for every `(DerivationSiteId, domain=All)` there is exactly one `EmissionAuthority` whose `writer` is the INSERT-origin node of that site; no two authorities share a `(site, domain)`; every site is covered. Flat reduction: `authorities` is a bijection onto `sites`, each writer node is an INSERT origin, and each coverage record's `authority` resolves to a live authority whose site's collection matches the use's terminal collection.

Every validator iterates the deterministic catalog vectors only (never a map's iteration order), so a fire's message is reproducible.

---

## 6. OPEN RISKS for the refuter panel

- **R1 — the DataFlow→InstanceFlow dependency hook.** The `gInstanceFlowTailPass` function-pointer indirection (or the `unique_ptr<InstanceFlowProgram>` incomplete-type storage) keeps the layering acyclic but adds a wire-once step and an out-of-line `QueryImpl` dtor. Refute: is the simpler "fold InstanceFlow into `lib/DataFlow`, pay the Rel-cutover cleanup as Phase-C debt" strictly better for a slice that has no Rel consumer yet? The whole point is future Rel→InstanceFlow, absent this slice.
- **R2 — DerivationSiteId = per-live-INSERT (candidate a).** IF4(c) proves parse-rule count is unrecoverable post-CSE, so (a) is the only stable/total choice — but §7.2's "rule/head contribution" language and any future per-rule authority partition (§8.4) may need candidate (b) (lift `RuleRoutingProjection` to DataFlow). Does adopting (a) now paint Phase D into a corner where two CSE-merged clauses can never be re-separated for a contextual authority split?
- **R3 — SCC tracked per-FamilyNode, not per-family.** §6's `FamilyTemplate.scc_ownership` is template-level (singular), implying the flat baseline should *split* at SCC boundaries into ≥2 families (IF4(c) Stress #1). This design keeps one `FlatFamily` with a per-node `scc` optional and defers the split to contextual specialization. Is that faithful to §7.6 atomicity, or does V-IF-SCC need a real boundary-split rule *now* to avoid re-baselining the whole grove at Phase D?
- **R4 — constant-sourced edges as `UINT32_MAX` sentinel.** IF4(a) Stress #1: `EqualityClassProof`/`ContextTransfer::Bind` has no literal-operand vocabulary. This slice sidesteps it (records the sentinel, builds no transfer), but the recorded `JoinPivot` seed on a constant-bound pivot (`join_1`) is degenerate/misleading in the dump. Should the seed recorder suppress provably-constant-keyed candidates, or is surfacing them (with a benefit note) the right reviewable behavior?
- **R5 — `ForEachUse` NEGATE gap + MERGE-arm-INSERT special case are re-implemented, not shared.** IF3 §4: `BuildEquivalenceSets`/`all_cols_match` are TU-local; the enumerator's NEGATE dispatch is missing. The builder special-cases both. Risk: a future edit to `QueryMerge::ForEachUse`/`QueryNegate::ForEachUse` silently desyncs the grove walk. Is a header-visible extraction of the canonical use-walk (touching Build.cpp's anon namespace) warranted now, or is the V-IF-ORIGIN bijection belt a sufficient tripwire?
- **R6 — byte-identity claim.** The grove is built + validated + optionally dumped, all off `QueryImpl` post-return; nothing in Regional/Rel/ControlFlow/codegen reads `instance_flow`. Codegen byte-identity should hold **only if** the tail hook is a pure read (no view mutation) and the `#ifndef NDEBUG` K5 belt at `Build.cpp:2659` still runs unchanged after it. Confirm the hook runs strictly between `:2657` and `:2659` and never touches `impl`'s graph. The abort-on-invariant validators mean a builder bug fails loud rather than silently perturbing anything — but a refuter should confirm no allocation/interning writes back into `QueryImpl`.

Key file:line anchors: slot `lib/DataFlow/Build.cpp:2654-2682`; catch-removal note `Build.cpp:2585-2591`; traversal/idiom to clone `lib/DataFlow/Format.cpp:1546-1600`; tag/friend `include/drlojekyll/DataFlow/Format.h:31-50` + `include/drlojekyll/DataFlow/Query.h:1173-1176`, storage beside `row_contracts` `lib/DataFlow/Query.h:1234-1240`; typed-ID idiom `lib/DataFlow/Identity.h:38-101`; use-walk `include/drlojekyll/DataFlow/Query.h:488-490` (+ NEGATE patch, `lib/DataFlow/Query.cpp:489-525,1471`); SCC source `QueryView::InductionGroupId()` `Query.h:493`; CLI clone points `bin/drlojekyll/Main.cpp:~59/~247/~330/~448/~86`.