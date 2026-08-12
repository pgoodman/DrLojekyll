<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-32 grounding — InstanceFlow Phase A + flat grove (adjudicated)

> Produced by the session-32 InstanceFlow grounding workflow (10 agents: 4 sonnet
> extractors over the identity substrate / dump+flag pattern / lineage+equality /
> the flat grove for 3 witnesses, opus design, a 3-refuter opus panel, opus
> desired-dump-states, opus adjudicator). Owner decisions this session: fork →
> InstanceFlow (over the empirically-grounded InstanceStore restoration, which is
> preserved as the proven back-end for a later phase); first slice → Phase A +
> flat grove + `-instanceflow-out` + validators (the foundational, inspectable
> observer at the Query→Rel seam; codegen byte-unchanged).
>
> Orchestrator verifications this pass (not on faith): (1) the fail-loud `catch(...)`
> removal is ALREADY landed (`Build.cpp` note; `Optimize` unguarded) — Phase A step 3
> is a no-op. (2) B3's "fold into lib/DataFlow, by-value member, direct tail call" IS
> the RowContract precedent verbatim (`row_contracts` on `QueryImpl` `Query.h:1240`,
> built+validated `Build.cpp:2654-2656`; the insertion slot is exactly there). (3)
> `QueryView::DeterministicOrder()` (`Query.h:438`, `det_seq`) backs QueryOriginId.
> (4) `ForEachUse(QueryColumn,InputColumnRole,...)`, `QueryView::Containing`, and the
> `InputColumnRole` enum all exist. (5) `lib/DataFlow/Identity.h` is the typed-ID-domain
> precedent (+ IdentityTypes ctest). Raw design/refuter/dump transcripts in the
> `.raw/` companion.
>
> KNOWN CP1 grammar deviation to reconcile: InstanceFlow.md §16 shows `authority=`
> INLINE on `covers` lines, but refuter finding B2 correctly decoupled coverage from
> emission. Reconciliation (to pin at CP1, before blessing goldens): render authority
> inline only on terminal-INSERT covers lines — matches §16's shape AND B2's soundness.

---

# InstanceFlow Phase A + Flat Grove — FINAL Grounding Doc & Execution Plan

*Adjudicated synthesis of the design + three refuter panels + the desired-dump derivation. Branch `keyed-instances`, all anchors as verified by the panel against tip. Paste-ready.*

---

## 1. VERDICT

**The flat slice SURVIVES, but only after a forced re-scope on four axes. It does NOT survive as originally framed.** Three independent refuters converged on the same structural defects, two of which fire on the design's own nominated witnesses (`transitive_closure`, `join_1`). None is a tuning issue; each is a model defect that would either (a) produce a non-deterministic golden, (b) abort a validator on a real corpus case, or (c) force a full golden rebaseline at Phase D — violating the owner's explicit "no rework at Phase B/D" gate.

The re-scope keeps the slice small and keeps the codegen-byte-identity observer property intact. It is **not** a hand-wave: each blocking finding changes the design.

### Forced changes (all adopted below)

| # | Finding (refuters) | Forced change |
|---|---|---|
| **B1** | R1-F1, R2-F3, dump-F1: the use sort key `(consumer, producer_col, role)` is **not a total order** — ties on leading-shared-column joins (`join_1`) and multi-INSERT relations (`tc`) fall back to `ForEachUse`'s explicitly-unspecified order → unstable `u#` golden. | **Total-order the use catalog** on `(consumer det_seq, producer det_seq, producer_col, role-ordinal, consumer-out-col)`. |
| **B2** | R1-F2, R2-F1, R2-F2, dump-F1: "one terminal authority per use" is **undefined** — a shared subexpression feeding ≥2 INSERTs (every CSE'd node upstream of a fan-out; the whole `tc` SCC) has 3 reachable terminals; dead-end live views (`compare.14/15` in `join_1`) reach **none**. Resolving by a forward graph walk leaks `Successors()` order AND multiply-counts coverage. | **Decouple coverage from emission.** V-IF-COVERAGE is a per-**consumer-obligation** bijection (interior + root uses, no authority). V-IF-EMISSION assigns an authority **only to terminal-INSERT root uses**, as a bijection onto sites. No use is ever resolved by a reachability walk. |
| **B3** | R1-F3, R3-F5: the separate `lib/InstanceFlow` target forces a `DataFlow→InstanceFlow` link cycle (the `unique_ptr<Incomplete>` dtor needs the complete type in `Query.cpp`); the escape hook (`gInstanceFlowTailPass`) leaves the "always-on" validators **off** in every non-CLI `Query::Build` caller (`bin/Oracle` is a correctness gate!), contradicting §17. | **Fold Phase A + flat grove into `lib/DataFlow`**, by-value member beside `row_contracts`, direct unconditional tail call. Defer the library split to Phase C when a real `Rel→InstanceFlow` reader exists. |
| **B4** | R3-F1 ∧ R3-F2: to emit the §16 grammar faithfully, each `family` block carries a **typed** `scc_ownership ∈ {Acyclic, WholeQueryScc}` (§6: "a family does **not** have nullable `scc`"). A single family holding an SCC **and** its acyclic tail (`transitive_closure`) is **untypeable**. The two are coupled: faithful grammar ⟹ SCC-split. Keeping one flat family rebaselines every `family`/`node`/`covers` line at Phase D. | **The flat grove is a *set* of empty-context families, partitioned by SCC condensation** — one `Acyclic` family for all trivial components, one `WholeQueryScc` family per non-trivial SCC. Emit the §16 grammar (`root=`, per-node `role=`, coverage nested under its family) **now**. Still empty-context (no contextual specialization) → in scope. |

### Secondary corrections (adopted, non-blocking)

- **S1 (R2-F5, dump-F2):** `LogicalCollectionId` = distinct `insert.Declaration().Id()` over **live INSERT views only**, *not* `relations ⧺ ios`. Message inputs and read-only bound-query relations are **not** collections. Use `.at()` + abort-on-miss, never `operator[]`.
- **S2 (R1-F4):** the "load-bearing NEGATE patch" is **dead** — `Query.cpp:519-520` already dispatches `AsNegate()`. Call `v.ForEachUse(...)` uniformly. Re-verify (do not assume) the `QueryMerge::ForEachUse` MERGE-arm-INSERT behavior and `all_cols_match` against tip during CP1.
- **S3 (R2-F4, dump-F5):** the materialize-then-reread edge is double-represented (INSERT-side `kMaterialized` + SELECT-side `kCopied`). **Canonical direction = the consumer/SELECT side.** The INSERT-successor arm contributes a use only as a terminal boundary obligation (`out == nullopt`). Reconcile against `insert->successors` non-empty at CP1.
- **S4 (dump-F3, R2 constant edges):** constant-sourced producer columns are **real uses** (`QueryCompare::ForEachUse` always yields the literal operand). Record `producer_col = UINT32_MAX` (`col=*`), producer = the literal SELECT view. V-IF-ORIGIN **must accept** literal-SELECT producers. Never reparse `-df` text. This flips the stale "orphan `compare.14/15`" claim — they are constant-elided, not orphans.
- **S5 (R3-F4, dump-F6):** **do not golden `seeds`.** Compute `CandidateSeed`s for validation/review but keep them out of the blessed `-instanceflow-out` (or behind a never-goldened advisory tail, `-origin-out` precedent). This avoids pinning the §7.3 seed vocabulary before §5.1's literal-operand question (`join_1`'s `A=1` pivot has no `EqualityClassProof` vocabulary) is resolved.

### The one carried-open item (owner-reconcile, non-gating)

Prior grounding IF4(c) asserted a **single** `FlatFamily` containing mixed SCC/acyclic nodes. R3-F1 refuted that against §6 verbatim. B4 sides with §6 (the type system is normative). If the owner's "§7.3 FlatFamily" intends a literal single family, §6-vs-§7.3 must be reconciled in the doc. **Recommended default: split (B4).** Rationale: it is the only reading under which the §16 `family <ownership>` token is well-typed, and it is forward-compatible. This is surfaced as **Residual Risk RR1**, to be closed by reading §6/§16 verbatim at CP1 (see §7).

---

## 2. FINAL FILE LAYOUT — fold into `lib/DataFlow` (B3)

No new library. Exactly the `RowContract` precedent (`lib/DataFlow/RowContract.h`, by-value member on `QueryImpl`, built+validated in-library):

```
include/drlojekyll/DataFlow/InstanceFlow.h   # public IDs, InstanceFlowProgram (header-only, implicit dtor),
                                             #   QueryInstanceFlow tag, Build/Validate decls
lib/DataFlow/InstanceFlow.cpp                # BuildFlatInstanceFlow + the 4 catalogs + SCC partition
lib/DataFlow/InstanceFlowValidate.cpp        # ValidateInstanceFlow (V-IF-*)   [or fold into InstanceFlow.cpp]
lib/DataFlow/Format.cpp                      # add operator<<(OutputStream&, QueryInstanceFlow) beside QueryContracts
```

- `InstanceFlowProgram` is **header-only, all `std::vector`/`std::unordered_map` members, implicit dtor** (R1-F5) — no out-of-line dtor, no link edge, no incomplete-type dance.
- Storage on `QueryImpl` beside `row_contracts` (`lib/DataFlow/Query.h:1234-1240`), **by value**:
  ```cpp
  InstanceFlowProgram instance_flow;  // built at Query::Build tail, post-Optimize; empty otherwise
  ```
- Build + validate are **called directly** at the `Query::Build` tail (`lib/DataFlow/Build.cpp`, in the window after `ValidateRowContracts` at `:2657`, before the K5 `#ifndef NDEBUG` belt at `:2659`, before `return Query(...)` at `:2682`). Unconditional in **every** `Query::Build` caller (`bin/drlojekyll`, `bin/Oracle`, `bin/RefHarness`, `bin/RefInterp`) — the §17 always-on contract is now literally true.
- Extract the TU-local `for_each_view` lambda (`Format.cpp:1553-1573`) into one DataFlow-internal free function `ForEachViewKindTagged(Query, cb)` (R1-F6, R3), shared by the builder, the dump, and `QueryContracts` — one ordering contract, three readers.
- Tag + friend: `struct QueryInstanceFlow { Query query; };` in the public header; `friend` line in `class Query` beside the `QueryContracts` friend (`include/drlojekyll/DataFlow/Query.h:1173-1176`).

**Phase-C debt (accepted, cheap):** when Rel becomes a real consumer, lift `InstanceFlow.{h,cpp}` into a `lib/InstanceFlow` node (`Rel → InstanceFlow → DataFlow`). Localized, no data-model change.

---

## 3. FINAL PHASE-A ID CATALOGS

### 3.1 Typed-ID domains (`include/drlojekyll/DataFlow/InstanceFlow.h`, mirror `lib/DataFlow/Identity.h:38-101`)

Never raw `unsigned`; defaulted intra-domain `==`/`<=>` only; no cross-domain operator.

```cpp
struct QueryOriginId       { uint32_t v; /* ==, <=> defaulted */ };  // = view det_seq; dense [0,N)
struct OriginUseId         { uint32_t v; /* ... */ };                // canonical-order dense [0,M)
struct LogicalCollectionId { uint32_t v; /* ... */ };                // interned decl.Id(); dense [0,C)
struct DerivationSiteId    { uint32_t v; /* ... */ };                // per live INSERT; dense [0,D)
struct EmissionAuthorityId { uint32_t v; /* ... */ };               // 1:1 with DerivationSiteId this slice
struct FamilyId            { uint32_t v; /* ... */ };                // SCC-condensation component
struct FamilyNodeId        { uint64_t v; /* packed (FamilyId, local) */ };
struct QuerySccId          { uint32_t v; /* ... */ };               // dense remap of InductionGroupId
```

### 3.2 `QueryOriginId` — **[verified-safe, no change]**

`= view.DeterministicOrder()` (the `det_seq` stamped once at `IdentifyInductions`, `Induction.cpp:142-144`, "run-stable; pointer values are not"), dense `[0,N)`, no re-stamp before the slot. A `det_seq` bijection onto live (`!is_dead`) views, produced by the extracted `ForEachViewKindTagged` walk (Selects→…→Inserts).

### 3.3 `LogicalCollectionId` — **[S1 correction]**

Interner over `insert.Declaration().Id()` across **live INSERT views only**, first-seen order. **Not** `relations` then `ios`. `decl.Id()` is stable across redeclarations (`Parse.cpp:321-328`). Message-input relations and read-only bound-query relations are deliberately absent (they are input/request ports, not collections). Lookups via `.at()` + abort-on-miss (S1). Condition/unit relations (`barrier_neck_1`) surface a real INSERT → they get a normal LC; add `barrier_neck_1` to the golden set to pin this path (R2-F5).

### 3.4 `DerivationSiteId` — **[candidate (a), per-live-INSERT; R2 concession]**

One per live `QueryInsert` view, stored order. `collection = by_decl_id.at(insert.Declaration().Id())`. One writer per site by construction. **Verified faithful for the flat slice** (dump-F7): `tc`'s two INSERTs → two sites into one LC, two authorities over `domain=All` on **distinct** sites → no §8.4 overlap. Parse-rule count is unrecoverable post-CSE (IF4(c): `tc` 2 rules→2 INSERTs but `is_node` 2 rules→1 INSERT), so per-live-INSERT is the only *stable, total* definition. Candidate (b) (rule-level via `RuleRoutingProjection`) deferred to Phase D → **RR2**.

### 3.5 `OriginUseId` — **[B1 + B2 + S2 + S3 + S4 corrections]**

The central catalog. One record per `ForEachUse` column-role edge, plus one per root/terminal obligation.

```cpp
enum class UseClass { kInterior, kTerminalInsert, kBoundQueryRead };
struct OriginUse {
  OriginUseId    id;
  QueryOriginId  consumer;         // the view whose ForEachUse produced this edge (occurrence)
  QueryOriginId  producer;         // QueryView::Containing(in)  (a literal SELECT for constants — S4)
  uint32_t       producer_col;     // in.Index().value_or(UINT32_MAX)   (col=* sentinel — S4)
  uint32_t       consumer_out_col; // the consumer slot ordinal (tie-break — B1)
  InputColumnRole role;
  UseClass       cls;
  // authority is NOT stored on the use (B2). Terminal-insert uses map to a site by construction.
  DerivationSiteId terminal_site;  // valid iff cls==kTerminalInsert
  LogicalCollectionId read_collection; // valid iff cls==kBoundQueryRead
};
```

**Population (canonical, deterministic):**
- Drive `ForEachViewKindTagged` in `det_seq` order; for each view call `v.ForEachUse(cb)` **uniformly** — **no NEGATE special-case** (S2: `Query.cpp:519-520` already handles it). Re-verify MERGE-arm-INSERT / `all_cols_match` at CP1 (do not carry the stale gap list).
- `producer = QueryView::Containing(in)` (`Query.cpp:126`, O(1)); constants → the literal SELECT, `producer_col = UINT32_MAX` (S4).
- **Canonical direction (S3):** the materialization edge is recorded on the **consumer/SELECT side** (`kCopied`); the INSERT-successor arm contributes a use only when `out == nullopt` (a terminal boundary). Reconcile `insert->successors` non-empty at CP1.
- **Root/terminal uses:** an INSERT boundary (`out == nullopt`) → `kTerminalInsert`, `terminal_site` = that INSERT's site. Bound-`#query` reads → enumerate as `Demand.cpp:466-475` (`decl.IsQuery() && decl.Arity()`, ≥1 bound param) → one `kBoundQueryRead` per bound read, `read_collection` = the read LC, **no authority**.
- **Total order (B1):** sort the whole catalog by `(consumer.v, producer.v, producer_col, role-ordinal, consumer_out_col)` — a proven strict total order (producer det_seq breaks the leading-shared-column and multi-INSERT ties). `OriginUseId` = index in this order. No `unordered_map` iteration is ever emitted.

### 3.6 Column-lineage + equality-class helpers (read-only, tip primitives)

- `LineageOf(QueryColumn) → {producer QueryOriginId, ord, is_constant}` via `Containing` + `Index` (constants → `is_constant`).
- JOIN pivot classes: read `join.NthInputPivotSet(n)` / `NthOutputPivotColumn(n)` (`Query.cpp:832,857`) directly — the class is already materialized; the helper renders it. No union-find this slice.
- AGG group key: `agg.InputGroupColumns()` / `NthInputGroupColumn(n)` (`Query.cpp:1042,1111`).

These seed `CandidateSeed` records for review only — **not goldened** (S5).

---

## 4. FINAL FLAT GROVE BUILDER

### 4.1 Data model (`InstanceFlowProgram`, header-only)

```cpp
struct ResidualSchema { std::vector<FieldId> fields; };  // FULL logical schema (empty-context baseline)

enum class OccurrenceRole { kRoot, kInterior };   // family position — CP1-verify against §6 (RR1)

struct FamilyNode {
  FamilyNodeId       id;             // (family, local-det_seq-index)
  QueryOriginId      origin;
  QueryViewKind      kind;
  OccurrenceRole     role;           // kRoot for the family's root node, else kInterior
  ResidualSchema     residual;       // == full logical schema; context EMPTY; transfers EMPTY
  std::optional<LogicalCollectionId> output_collection;  // INSERT nodes only
};

enum class SccOwnership { kAcyclic, kWholeQueryScc };
struct Family {                       // one per SCC-condensation component (B4)
  FamilyId       id;
  SccOwnership   ownership;
  std::optional<QuerySccId> scc;      // set iff kWholeQueryScc
  OriginUseId    root_use;            // the anchoring boundary use — CP1-verify (RR1)
  std::vector<FamilyNode> nodes;      // family-local det_seq order
  std::vector<OriginUseId> covers;    // uses whose occurrence node ∈ this family
};

struct EmissionAuthority {            // one per DerivationSite; domain=All
  EmissionAuthorityId id;
  DerivationSiteId    site;
  FamilyNodeId        writer;         // the INSERT-origin node
};

struct CandidateSeed { /* inert, NOT goldened (S5) */ };

struct InstanceFlowProgram {
  QueryOriginCatalog          origins;
  OriginUseCatalog            uses;          // total-ordered (B1)
  LogicalCollectionCatalog    collections;   // INSERT-target only (S1)
  DerivationSiteCatalog       sites;
  std::vector<Family>         families;      // SCC-partitioned (B4); empty grove ⇒ empty
  std::vector<UseCoverage>    coverage;      // bijection onto uses (B2)
  std::vector<EmissionAuthority> authorities;// bijection onto sites (B2)
  std::vector<CandidateSeed>  seeds;         // computed, un-goldened
};
```

Where `UseCoverage { OriginUseId use; FamilyNodeId occurrence; };` — **no authority field** (B2). A `kTerminalInsert` use's emission is expressed via `authorities`, keyed on its site, never on the use.

### 4.2 `BuildFlatInstanceFlow(QueryImpl*)` — ordered

1. **origins** — `ForEachViewKindTagged`, `QueryOriginId = det_seq`, record `kind`.
2. **collections (S1)** — intern `insert.Declaration().Id()` over live INSERTs, first-seen order.
3. **sites + authorities** — one per live INSERT, stored order; `collection = by_decl_id.at(...)`; `authorities[i] = {i, site_i, writer_node}` (bijection).
4. **SCC condensation (B4)** — group origins by `InductionGroupId()` (`Query.h:493`). Each distinct group id with ≥2 members (or a self-loop) → a `WholeQueryScc` family (dense `QuerySccId` remap). All remaining origins → **one** `Acyclic` family. Nodes numbered family-local in det_seq order. Populate `output_collection` on INSERT nodes.
5. **uses + coverage (B1/B2/S3/S4)** — the total-ordered `ForEachUse` walk (§3.5). Each use → exactly one `UseCoverage{use, occurrence = consumer-node (interior) / writer-node (kTerminalInsert) / tc-model-node (kBoundQueryRead)}`. Append each use's id to its occurrence family's `covers`.
6. **root_use + role (RR1, CP1-verify)** — per family, set `root_use` per the deterministic rule pinned at CP1 (recommended default: the minimal `OriginUseId` among the family's boundary/terminal uses; for a pure-interior SCC family, the minimal `OriginUseId` consuming a member from outside). Mark that node `kRoot`; others `kInterior`.
7. **seeds (S5)** — compute `JoinPivot` / `AggregateGroup` / `BoundaryBinding`; store; **do not golden**.

**§7.6 atomicity** is now discharged structurally: no family straddles an SCC boundary (B4), so V-IF-SCC's "a family never contains half of a recursive SCC" is true by construction. Orphan/dead-end origins (S4: `compare.14/15` are constant-elided, not orphans) still get a node and contribute their consumer-side uses (covered, no authority — B2). Unsat arms (`never`) get an ordinary LC.

---

## 5. FINAL `-instanceflow-out` GRAMMAR + THE 3 DUMPS

CLI: verbatim `-contract-out` clone renamed `-instanceflow-out` (global `gInstanceFlowStream` `Main.cpp:~59`, local `~330`, flag arm after `-origin-out` `~448`, help after `-contract-out` `~247`, drain after the `-origin-out` drain / before `FrozenRegionalProgram::Build` `~86`). Emitter mirrors `QueryContracts` (`Format.cpp:1546-1600`): `ForEachViewKindTagged` for node lines, per-block `ostringstream`/`OutputStream bos`/`take()` buffering, a redundant dump-time census belt in the `Format.cpp:1575-1597` fprintf+abort shape.

### 5.1 Grammar (§16-faithful — B4/R3-F2)

```
instanceflow  origins=<N> uses=<M> collections=<C> sites=<D> families=<F>

collections
  lc#<k> decl=<name>/<arity>
sites
  ds#<k> writer=q#<det_seq> -> lc#<k>

family if#<f> <acyclic|whole-query-scc scc#<s>> root=u#<k>
  node if#<f>.<local> origin=q#<det_seq> <kind> role=<root|interior> residual=(<fields>) [-> lc#<k>]
  ...
  covers u#<k> <interior col=<n|*> role=<role> src=q#<producer>> | <insert q#<w> -> lc#<k>> | <query <name> bound=(…) -> reads lc#<k>>  occ=if#<f>.<local>
  ...
authorities
  ea#<k> site=ds#<k> domain=all writer=if#<f>.<local>
```

Stable tokens: `q#`, `lc#`, `ds#`, `ea#`, `u#`, kinds, `residual`, `col=*` sentinel, `role=` (InputColumnRole on `covers`). **CP1-verify tokens (RR1):** `<ownership>`, `root=u#`, per-node `role=<root|interior>` (`OccurrenceRole` enum values), and the `covers`-nesting shape — all read verbatim from §6/§16 before CP1 goldens are blessed. **Not emitted:** `seeds` (S5).

### 5.2 Predicted dumps

> These reuse the desired-dump author's tip-verified DAG structure (origins/collections/sites) with the four re-scopes applied: total-ordered uses (B1), authority only on terminal-insert uses (B2), collections = INSERT-targets (S1), SCC-split families with nested coverage (B4). `u#` numbering and `root=`/`role=` tokens are **predicted — blessed from real output at CP2** (RR1).

**(a) `join_1`** — no SCC → one `Acyclic` family. 20 origins, 2 collections (`q/1`, `never/1`), 2 sites. Uses total-ordered by `(consumer, producer, producer_col, role, out_col)`. **Authority appears only on the two terminal-insert `covers` lines** (`insert q#18 -> lc#0` → `ea#0`; `insert q#19 -> lc#1` → `ea#1`); all interior `covers` lines carry **no** authority. Constant operands render `col=*` with `src=q#2`/`q#3` (the literal SELECTs — S4). `compare.14/15` are constant-elided consumers, not orphans (S4): their input uses are covered, no authority. `seeds` omitted (the `A=1`/`A=2` pivots are degenerate — S5).

**(b) `merge_2`** — no SCC → one `Acyclic` family. 13 origins, 2 collections (`q_outer/2`, `q_proj/1`), 2 sites. The maximal-sharing flagship: `q#1`→{covers into q#4, q#7} and `q#2`→{covers into q#3, q#8} — one shared producer node, **disjoint** consumer-keyed uses, each covered once, into disjoint authorities `ea#1`/`ea#0`. Authority only on `insert q#12 -> lc#0` (`ea#0`) and `insert q#13 -> lc#1` (`ea#1`). `seeds` empty.

**(c) `transitive_closure`** — **two families** (B4). This is the load-bearing corrected dump:

```
instanceflow  origins=16 uses=30 collections=2 sites=3 families=2

collections
  lc#0 decl=tc/2
  lc#1 decl=is_node/1
sites
  ds#0 writer=q#13 -> lc#0
  ds#1 writer=q#14 -> lc#0
  ds#2 writer=q#15 -> lc#1

family if#0 acyclic root=u#<k>            ; CP1-verify root selection
  node if#0.0  origin=q#0  select role=interior residual=(From,To)
  node if#0.1  origin=q#4  tuple  role=interior residual=(Node)
  node if#0.2  origin=q#5  tuple  role=interior residual=(Node)
  node if#0.3  origin=q#6  tuple  role=interior residual=(From,To)
  node if#0.4  origin=q#7  tuple  role=interior residual=(From,To)
  node if#0.5  origin=q#8  tuple  role=interior residual=(Node)
  node if#0.6  origin=q#9  tuple  role=interior residual=(From,To)
  node if#0.7  origin=q#12 merge  role=interior residual=(Node)
  node if#0.8  origin=q#13 insert role=root     residual=(From,To) -> lc#0
  node if#0.9  origin=q#14 insert role=interior residual=(From,To) -> lc#0
  node if#0.10 origin=q#15 insert role=interior residual=(Node)    -> lc#1
  covers u#6  interior col=0 role=copied src=q#11 occ=if#0.1
  covers u#7  interior col=1 role=copied src=q#11 occ=if#0.2
  covers u#8  interior col=0 role=copied src=q#11 occ=if#0.3
  covers u#9  interior col=1 role=copied src=q#11 occ=if#0.3
  covers u#10 interior col=0 role=copied src=q#11 occ=if#0.4
  covers u#11 interior col=1 role=copied src=q#11 occ=if#0.4
  covers u#12 interior col=0 role=copied src=q#12 occ=if#0.5
  covers u#13 interior col=0 role=copied src=q#0  occ=if#0.6
  covers u#14 interior col=1 role=copied src=q#0  occ=if#0.6
  covers u#23 interior col=0 role=merged src=q#4  occ=if#0.7
  covers u#24 interior col=0 role=merged src=q#5  occ=if#0.7
  covers u#25 insert q#13 -> lc#0 occ=if#0.8
  covers u#26 insert q#14 -> lc#0 occ=if#0.9
  covers u#27 insert q#15 -> lc#1 occ=if#0.10

family if#1 whole-query-scc scc#0 root=u#<k>    ; members {q#1,q#2,q#3,q#10,q#11}
  node if#1.0 origin=q#1  tuple role=interior residual=(From,To)
  node if#1.1 origin=q#2  tuple role=interior residual=(AutoVar_2,Node)
  node if#1.2 origin=q#3  tuple role=interior residual=(From,X)
  node if#1.3 origin=q#10 join  role=interior residual=(X,From,To)
  node if#1.4 origin=q#11 merge role=root     residual=(From,To)
  covers u#0  interior col=1 role=copied   src=q#10 occ=if#1.0
  covers u#1  interior col=2 role=copied   src=q#10 occ=if#1.0
  covers u#2  interior col=0 role=copied   src=q#11 occ=if#1.1
  covers u#3  interior col=1 role=copied   src=q#11 occ=if#1.1
  covers u#4  interior col=0 role=copied   src=q#11 occ=if#1.2
  covers u#5  interior col=1 role=copied   src=q#11 occ=if#1.2
  covers u#15 interior col=0 role=pivot    src=q#2  occ=if#1.3
  covers u#16 interior col=0 role=join-col src=q#3  occ=if#1.3
  covers u#17 interior col=1 role=pivot    src=q#3  occ=if#1.3
  covers u#18 interior col=1 role=join-col src=q#2  occ=if#1.3
  covers u#19 interior col=0 role=merged   src=q#1  occ=if#1.4
  covers u#20 interior col=0 role=merged   src=q#9  occ=if#1.4
  covers u#21 interior col=1 role=merged   src=q#1  occ=if#1.4
  covers u#22 interior col=1 role=merged   src=q#9  occ=if#1.4
  covers u#28 query reachable_from bound=(From) -> reads lc#0 occ=if#1.4   ; occ=tc model — CP1-verify
  covers u#29 query reaching_to   bound=(To)   -> reads lc#0 occ=if#1.4

authorities
  ea#0 site=ds#0 domain=all writer=if#0.8
  ea#1 site=ds#1 domain=all writer=if#0.9
  ea#2 site=ds#2 domain=all writer=if#0.10
```

Note the decoupling pays off exactly where the original design broke: the 16 SCC-interior uses and `u#13`/`u#14` (upstream of the `merge.11` 3-way fan-out) carry **no authority** — the undefined "terminal reached along the chain" is gone. Emission stays a clean `authorities ↔ sites` bijection (3↔3). The two bound-query reads are coverage obligations with no authority. `occ` of a read = the `tc` model node (`if#1.4`) — **CP1-verify**.

---

## 6. FINAL VALIDATORS — `ValidateInstanceFlow(QueryImpl*, const ErrorLog&)`

All always-on, `fprintf(stderr,…)+abort()` (survive NDEBUG). **Abort-only** — no `return std::nullopt`/green→red path (R3-F3): a fire is a compiler bug, not a user diagnostic. Iterate the deterministic catalog vectors only. Called directly at the tail (B3), so genuinely on in every `Query::Build` caller.

- **V-IF-ORIGIN** — every `FamilyNode.origin` is a live `QueryOriginId ∈ [0,N)` (view `!is_dead`); `residual.fields.size()` == the origin's logical column count; the union of all families' node sets is a **det_seq bijection** onto live views (none missing, none doubled). **Accepts literal-SELECT producers** for constant-sourced uses (S4).
- **V-IF-CONTEXT** — every node's `context` and `transfers` are empty; `residual` == full logical schema (the empty-context reduction of "residual + context reconstructs the schema"). Any non-empty context here is a builder bug (this slice never grows).
- **V-IF-COVERAGE** (B2) — **consumer-obligation bijection**: every `OriginUseId ∈ [0,M)` appears in exactly one `UseCoverage`; every `UseCoverage.use`/`.occurrence` resolves to a live use/node; each use's `occurrence` family membership matches its `covers` block. **No authority in this validator.** Reprised as the dump-time census belt.
- **V-IF-EMISSION** (B2) — **`authorities ↔ sites` bijection**; every `writer` is an INSERT-origin node; every `kTerminalInsert` use's `terminal_site` has exactly one authority; **no non-terminal use carries an authority**; no two authorities share a `(site, domain=All)`. Never resolves a use by graph reachability.
- **V-IF-SCC** (new, from B4) — every `WholeQueryScc` family's node set == exactly one `InductionGroupId`'s members (no straddle, no half-SCC); the `Acyclic` family contains no group-id-bearing view. Discharges §7.6 structurally.

---

## 7. EXECUTION PLAN

Standard repo loop: build/test **silent on success** (memory `silent-tests-save-tokens`); **predict-then-verify** every dump (memory `predict-then-verify-ir`); WIP-commit at each checkpoint with the Co-Authored-By trailer; never bless red→green.

### CP0 — lib skeleton + IDs, byte-unchanged
1. Read **`InstanceFlow.md` §6 and §16 verbatim** — pin `SccOwnership` values, `OccurrenceRole` enum, `root_use` semantics, and the exact `family`/`node`/`covers` token grammar. **This closes RR1 and is the gate on all golden shapes.** (The panel demonstrably worked from a loose/pre-tip reading — R1-F4.)
2. Add `include/drlojekyll/DataFlow/InstanceFlow.h` (typed IDs, header-only `InstanceFlowProgram`, `QueryInstanceFlow` tag, `Build`/`Validate` decls). Add the by-value `instance_flow` member to `QueryImpl` (`Query.h:1234-1240`). Add the `friend` line.
3. Extract `ForEachViewKindTagged` from the `Format.cpp` lambda; repoint `QueryContracts` at it (must stay byte-identical).
4. `lib/DataFlow/InstanceFlow.cpp` with the 4 catalogs (§3) + SCC condensation (§4.2 steps 1-4). No uses/coverage/dump yet.
- **WIP commit.** **Exit gate:** build green; OptDiff `SUITE: PASS` **byte-identical** (nothing emits/consumes the grove); ctest 5/5.

### CP1 — flat grove + `-instanceflow-out`
5. Uses + coverage (§4.2 step 5): total-ordered walk (B1), consumer-side canonical direction (S3), constant sentinels (S4), decoupled coverage (B2), **verify S2** (NEGATE already handled) and the MERGE-arm-INSERT / `insert->successors` behavior against tip.
6. `root_use` + `OccurrenceRole` per the §6-pinned rule (step 1).
7. `operator<<(OutputStream&, QueryInstanceFlow)` + CLI wiring (`-instanceflow-out`, §5.1).
8. **Predict** the three dumps (§5.2, refined by step 1), then diff against real output.
- **WIP commit.** **Exit gate:** build green; OptDiff `SUITE: PASS` byte-identical (all `.stdout`/oracle/monotone/behavioral/`.df`/`.rel`/`.ir`/codegen goldens unchanged — the observer proof); ctest 5/5; three predicted dumps match real output.

### CP2 — validators live + witness goldens
9. `ValidateInstanceFlow` (all five, §6), called at the tail; run the **full OptDiff suite with validators live** across all 4 modes — no `abort()` on any corpus case (this discharges RR3; orphans/`never`/constant edges are the shapes to watch — R3-F6). Optionally harden temporarily to `assert(!families.empty() || no-inserts)` to prove no case escapes, then relax.
10. Add witnesses to `tests/OptDiff/goldens/` via the **`.irgold` sidecar** surface — a new `instanceflow opt` step (peer of `region`/`contract`/`ir`/`h`): pin `join_1.instanceflow.opt` (single Acyclic family, constant `col=*` edges, degenerate-seed absence), `merge_2.instanceflow.opt` (maximal-sharing, disjoint authorities), `transitive_closure.instanceflow.opt` (two families, SCC split, decoupled authority, bound reads), and `barrier_neck_1.instanceflow.opt` (condition-relation LC path — S1/R2-F5). Opt-mode only (the grove is post-Optimize; other modes are answer-invariance nets, not grove pins — confirm cross-mode grove shape and pin per-mode only if it differs).
- **WIP commit.** **Exit gate:** build green; OptDiff `SUITE: PASS` (now including the 4 new `.instanceflow` goldens + validators live) byte-identical on all pre-existing goldens; ctest 5/5.

**`.irgold` mechanics:** each case's `<name>.irgold` lists `<artifact> <mode>` steps that `runall.sh` runs and byte-compares to `goldens/<name>.<artifact>.<mode>`. Add `instanceflow opt` and drop the emitted dump in as the golden after review; never bless a red case green (memory + CLAUDE.md bless rule).

---

## 8. RESIDUAL RISKS carried into execution

- **RR1 (top, gates CP1 goldens) — §6/§16 verbatim not yet read.** `SccOwnership` values, `OccurrenceRole` enum, `root_use` selection, and the `family`/`covers` grammar are predicted, not doc-verified. The panel worked from a loose reading (R1-F4 caught a dead "NEGATE patch"). **Close by reading §6/§16 at CP0 step 1 before any golden is blessed.** Also reconcile the §6-vs-§7.3/IF4(c) "single FlatFamily" contradiction — recommended resolution: split (B4), because the §16 `family <ownership>` token is only well-typed under a split.
- **RR2 — `DerivationSiteId` = per-live-INSERT (candidate a).** Sound and total for the flat slice (dump-F7), but Phase D's per-rule authority partition (§8.4) may need candidate (b) (lift `RuleRoutingProjection` to DataFlow scope). Two CSE-merged clauses cannot be re-separated post-hoc. Deferred, flagged.
- **RR3 — byte-identity is contingent on validator quiescence.** A V-IF-* fire is `abort()`, i.e. a crash, not byte-identity. Must be discharged **empirically** by CP2 step 9 (full suite, validators live, all 4 modes) before claiming observer status. Highest-risk shapes: constant-elided edges (`compare.*`), the `never` unsat arm, condition relations, bound-query reads' `occ`.
- **RR4 — the `insert->successors` / materialize-then-reread reconciliation (S3, dump-F5).** The three nominated witnesses have terminal INSERTs into distinct tables; the double-representation (INSERT-side `kMaterialized` + SELECT-side `kCopied`) is **untested** for a derived `#local`/`#export` re-read downstream. Add such a case at CP1 verification, or the canonical-direction choice ships unexercised.
- **RR5 — `occ` of a bound-query read.** Assigned to the collection's model node (`tc` merge) here; §7.2 item 5's exact reader-node semantics are unconfirmed (CP1-verify).
- **RR6 — seed vocabulary (S5, deferred not resolved).** Seeds are computed but un-goldened, so no `.md` rebaseline risk — but Phase D still owes the §5.1 literal-operand vocabulary (`join_1`'s constant pivot) before `JoinPivot` seeds can be pinned. Recorded, not blocking.