# P5 grounding — the partial-binding DAG (order-free schema, order-significant edge; the declared `@key` first specializes)

Session 20 grounding output. Branch `keyed-instances`, code byte-current at `c546a6a4` (tip
`f9f89d1b` is the session-20 docs handoff). This is the code-verified P5 execution-readiness record:
the layer-site inventory (all anchors grep-verified against real POST-P4 code this session), the
settled open questions (P5 is model+render only; the render lives in `-region-out`; interning is
value-free), the typed additions, the adversarial refuter-panel survivors (§8, folded A1–A7), and the
IR desired-states (§9). It is the sibling of `p4-grounding.md`; where it disagrees with
`reconstruction-diffs.md §3-P5/§5.6` (which predate P1–P4) this doc wins — the drift table §7 records
every correction.

Method: orchestrator inventory (every anchor grep-verified), then an opus refuter panel (4 per-facet
refuters + IR-states author + synthesizer, 528k subagent tokens), each finding re-verified against the
tree — with one empirical compile that caught the uncompilable-carrier defect A1.

**GREENFIELD RULING (owner), still governing.** The compiler is not in use → delete-then-rebuild.
Every post-P1 gate is STRUCTURAL, never answer-equality (the full-materialization backend answers
correctly with `@key` inert). Memory `greenfield-rewrite-motivation`.

---

## §1. THE HEADLINE — what P5 is, and the four authorities it must keep apart

**P5 is the first phase where a DECLARED `@key` STOPS BEING INERT and shapes the compile-time model.**
It seeds the LOGICAL-ACCESS-PATH authority (`DeclaredAccessPath`, ORDERED — `[A,B] ≠ [B,A]`) and
populates the residual BINDING-STATE-SCHEMA DAG (order-free schema nodes, order-significant edges,
prefix-shared, convergent). Like P3, P5 is a COMPILE-TIME modeling addition; the physical partial-key
seek is a P7 cost decision (`GetOrCreateIndex(subset)`, p7p9-critique headline). **P5 moves NO codegen**
(`.rel`/`datalog.h`/`.stdout` byte-stable) — it adds a `RelationSchema` field, a freeze-side DAG, an
always-on completeness belt, one `-region-out` render line, a new positive carrier golden, and the
mandatory ctest DAG gates.

The FOUR authorities stay disjoint (the P5 headline is the #2↔#3 split):

| # | Authority | Type | Status |
|---|-----------|------|--------|
| 1 | logical fact id | `RegionalFactId` (RegionInstance.h:197-204) | P3 |
| 2 | **order-free** binding schema | NEW per-relation schema DAG (`RelSchemaLocalId` + `schema_table` + `binding_edges`) | **P5 populates** |
| 3 | **ORDERED** declared access path | NEW `DeclaredAccessPath{KeyPathId, RelationId, ordered_fields}` (order = identity) | **P5 adds** |
| 4 | physical access plan | `AccessPlan` (RegionInstance.h:213-246) | P4, **unchanged** |

CROSS-CUT invariant: inference (P9) picks NO physical structure, planning (P7) picks NO logical path;
answer identity holds because an unbound read still full-scans the COMPLETE relation (F20).

---

## §2. Verified current state (all grepped/compiled this session)

- **Binding-state substrate** (`include/drlojekyll/Regional/RegionInstance.h`): `BindingStateSchemaId
  {uint32_t v}` (:115) interns ONLY `{0}` (comment :113-114); `BindingStateId{ri, schema, vals}` (:182)
  with `vals` (`SortedBoundFieldValues = vector<uint64_t>`, :166) ALWAYS `{}`; `EmptyBindingState(ri)`
  (:192) = `{ri,{0},{}}`. NO `BindingEdge`, NO `DeclaredAccessPath`, NO `KeyPathId`, NO `schema_table`.
  `RegionInstanceRelations` (:319) holds request_edges / activation_edges(reserved) / derivations /
  routed_results / requested_relation_of_owner.
- **RelationSchema** (`Regional.h:69-87`): `{ RelationId id; ParsedDeclaration decl; vector<bool>
  member_key_positions; bool support; }` — aggregate-initialized **positionally** at BOTH arms
  (Planning.cpp:208 insert, :218 origin). `RequestPortRecord{...,AccessPlan plan}` (:134); `PlanFor`
  (:208).
- **Parse surface** (LANDED, order-significant P0-item-4): `ParsedDeclaration::InstanceKeys()`
  (Parse.h:457, Parse.cpp:868) returns `const vector<InstanceKeySet>&` (InstanceKeySet =
  `vector<unsigned>` decl-ordinals, ORDER kept); `HasInstanceKey()` (Parse.h:456, Parse.cpp:858);
  `InstanceKeyRanges()` (:462). Parser stores ordinals in written order
  (`key_cur_set.push_back(resolved_index)`, Parser.cpp:918-944); rejects same-decl duplicate ORDERED
  path by DIRECT ordered-vector compare (Parser.cpp:980-991); cross-redecl consistency `SameKeySetOfSets`
  is order-free ACROSS paths / order-sig WITHIN (Parser.cpp:1475-1481). So `@key(A,B)` and `@key(B,A)`
  are DISTINCT paths that BOTH pass parse. `@key` only on `#local`/`#export`.
- **Render**: the OLD DataFlow `RenderDeclaredKeyLines` (declared=/inferred=) was DELETED at P1.
  `-contract-out` (DataFlow/Format.cpp:1546) is a pure Stage-A VIEW-level dump — decl-@key-BLIND
  (empirically confirmed: no declared-key/path token). The ONLY surviving declared-key surface is the
  advisory `-region-dot-out` badge (Regional/Format.cpp:384, `HasInstanceKey() ? " declared-key"`).
  There is NO golden-pinned declared-key render. The `-region-out` TEXT dump (Regional/Format.cpp:206-333)
  renders `row-contract E<k> rel=<name> member-key=(<names>) support=<s>` per RelationSchema; kind_w is a
  per-dump max over emitted kinds (:259-273), `"row-contract"`==`"declared-key"`==12 chars.
- **Freeze seam** (Planning.cpp): `BuildRelationSchemaFromInsert` (:180) / `BuildRelationSchemaFromOrigin`
  (:216) build RelationSchema; the schema loop is :447-453 (insert arm over `CollectContractInserts`, then
  origin arm over `CollectOriginInteriorDecls`). `ComputeQueryAccessPlan` (:242) is the P4
  selector-at-freeze precedent. Census belt `check_count` at :483-488.
- **Corpus**: only 4 `key_*_1` PARSE-REJECT cases (key_anon/dup/unknown/wildcard) + the 10 `reject_key_*`
  cases. NO positive `@key` carrier — P5 MUST author one.
- **ctest** `tests/RegionInstance/RegionInstanceTest.cpp` is a PURE unit test of the model types (no .dr
  compile). P4 `GateA_SelectAccessPlan` (:257) calls `SelectAccessPlan` directly; `KeyedState` synthetic
  helper (:233). P5 gates attach identically.
- **Empirical (this session)**: the A1 headline carrier `#local rel @key(A). #query rel(bound A,free B).`
  does NOT compile (`Cannot re-declare 'rel' as a local`). The corrected shape (`#local path @key(From).`
  read through a distinct `#query reachable_from(...) : path(...).`) compiles rc=0 and renders
  `row-contract E1 rel=path member-key=(From, To) support=monotone` (@key inert) + the DOT badge
  `declared-key`. Its driver compiles+runs; answer is stable. `.rel` (16 lines) / `datalog.h` (140 lines)
  baselines captured for the no-codegen-movement diff.

---

## §3. Settled open questions (from the grounding loop)

- **Does P5 change codegen?** NO — model + render only, answer-invariant. Nothing in `Program::Build`
  or CodeGen consumes `schema_table`/`binding_edges`/`declared_access_paths`; the ONLY freeze→codegen
  dependency is the P4 `AccessPlan` (via `PlanFor`), untouched. `.rel`/`datalog.h`/`.stdout` BYTE-STABLE.
  The physical partial-key seek is P7. (Certified §8; verified: zero consumers outside RegionInstance.h.)
- **Where does the render live?** `-region-out` (text, golden-pinned) — a `declared-key` line per
  declared path on the relation-schema block, KeyPathId-sorted. `-contract-out` is decl-@key-blind
  (wrong home); keep the DOT badge (advisory).
- **Is interning value-free?** YES — P5 interns only the SCHEMA DAG; `BindingStateId.vals` stays `{}`.
  No non-empty `BindingStateId` is minted at P5 (firewall belt, A3).
- **Real-compile vs ctest?** BOTH: the DeclaredAccessPath + declared-key render is a real-compile
  golden artifact (F21); the DAG is interned at freeze (real compile) and made LOAD-BEARING by the
  `HasInstanceKey()`-tied V-PREFIX-CHAIN belt (A2); the DAG STRUCTURAL claims (F8/convergence/prefix
  share) are gated in the ctest (the `.region` render carries no numeric schema/edge id — MANDATORY, A2).

---

## §4. The design (A1–A7 folded)

### D1. Types (add to RegionInstance.h; `= default` comparators per house idiom — A7)
```
struct KeyPathId       { uint32_t v; == <=> default; };
struct RelSchemaLocalId{ uint32_t v; == <=> default; };   // A3: DISTINCT from BindingStateSchemaId
struct DeclaredAccessPath   { KeyPathId id; RelationId relation; vector<uint32_t> ordered_fields; ==<=>default };
struct DeclaredAccessPathSet{ vector<DeclaredAccessPath> paths; };           // unique by ordered_fields
struct BindingEdge     { RelSchemaLocalId parent; uint32_t added_field; RelSchemaLocalId child; ==<=>default };
```
A3 firewall: `RelSchemaLocalId` (per-relation, order-free field-set identity) is DISTINCT from
`BindingStateSchemaId` (reserved for the P6.2 region-global `SymbolicFieldSet`); no DAG id is ever
stored into a `BindingStateId` (trivially holds — the sole construction is `EmptyBindingState`+`{0}`).

`InternDeclaredPaths(relation, raw_paths=decl.InstanceKeys())`: parser already rejected same-decl dups,
so a belt-abort on a repeat; assign KeyPathId deterministically (sort by ordered_fields) — pragma
reorder ⇒ byte-identical render (F21). PURE.

### D2. The per-relation schema DAG (order-free node, order-sig edge — A3 typing)
On `RegionInstanceRelations`:
```
map<pair<RelationId, sorted vector<uint32_t>>, RelSchemaLocalId> schema_table;   // per-relation; empty set per relation
set<BindingEdge> binding_edges;                                                   // std::set dedup is load-bearing (A7)
uint32_t next_rel_schema{0};
RelSchemaLocalId InternBindingSchema(RelationId, sorted_ordinal_set);             // interns empty set too (A3(c))
void MaterializePrefixChain(DeclaredAccessPath p):                                # LAZY: declared-or-visited, NEVER power set
    running = {}; prev = InternBindingSchema(p.relation, {})                      # per-relation root (A3(b))
    for f in p.ordered_fields:
        running.insert(f); cur = InternBindingSchema(p.relation, sorted(running))
        binding_edges.insert({prev, f, cur}); prev = cur
```
A3 rationale: the P5 ids are a safe UNDER-approximation (never conflate two relations) but are
structurally un-promotable to the P6/P8 `SymbolicFieldSet` rep — the P5 `schema_table` is REBUILT at
P8, not migrated. Per-relation root makes every edge's (parent,child) unambiguous even though
`added_field` is a bare ordinal. `@key(A)` ⊂ `@key(A,B)` share the `{A}` id (intern `sorted(running)`
each step); `[A,B]`/`[B,A]` converge on ONE `{A,B}` id via TWO ordered edges.

### D3. Attach + populate (A4: BOTH arms)
- `RelationSchema` gains `DeclaredAccessPathSet declared_access_paths;` (matches target rep). Populate
  from `decl.InstanceKeys()` via `InternDeclaredPaths` in BOTH `BuildRelationSchemaFromInsert` AND
  `BuildRelationSchemaFromOrigin` (a merge-materialized `@key`'d `#local` with no INSERT is named through
  the ORIGIN arm — A4). Both arms already hold `decl`.
- At freeze, after building `relation_schemas`, call `MaterializePrefixChain` for each declared path of
  each schema → populates `instances.schema_table` + `instances.binding_edges` (REAL compile).

### D4. V-PREFIX-CHAIN — the LOAD-BEARING completeness belt (A2 rewrite)
Always-on freeze validator (fprintf+abort, survives NDEBUG — the V-PLAN-HONEST idiom), tied to the
INDEPENDENT parse authority so it is NOT self-referential: for every `RelationSchema` with
`decl.HasInstanceKey()==true` (Parse.cpp:858), assert (i) `declared_access_paths` is NON-EMPTY, (ii)
every declared path's terminal schema is present in `schema_table`, (iii) every intermediate prefix is
interned (the reachability chain), and (iv) F8 non-prefix-absence: the relation's `schema_table` members
equal EXACTLY the UNION over that relation's declared paths of all their prefixes (A5 — implemented
against the union, never per-path, so `@key(A,B) @key(B)` legally interns `{B}`). A stub that skips
`InternDeclaredPaths`/`MaterializePrefixChain` ABORTS (HasInstanceKey true but paths empty / schema
missing) instead of passing vacuously.

### D5. Render (F21) — `-region-out` text dump, Regional/Format.cpp (A6/A7)
Append a `declared-key` block AFTER the row-contract block, iterating `relation_schemas` in E-order,
emitting each schema's declared paths in KeyPathId order:
`  declared-key  E<k>  rel=<name>  path=(<ordered field names>)`
Field names render from `RelationSchema.decl.NthParameter(ordinal)` (A6 — carrier authored with a single
decl, no redecl rename, so unambiguous). Reuse the row-contract `etok_w`/`rel_w` columns so `E<k>`/`rel=`
align; `path=(…)` is the trailing unpadded token (like `support=`). Add `"declared-key"` to the `kind_w`
max (:264-273) gated on any-schema-HasInstanceKey — width unchanged (`"declared-key"`==`"row-contract"`
==12). E-K5-PAD constraint (A7): emit as its OWN width block; do NOT fold path text into the row-contract
`rel_w`/`key_w` loop. Keep the DOT badge. NO census field (declared-key is render-only; a census field
would move all 16 region goldens).

### D6. The new positive carrier (A1 — verified-compiling shape)
`key_partial_1.dr` (`@key` rides on `#local`, read through a DISTINCT bound `#query`):
```
#message edge_2(u64 From, u64 To).
#local path(u64 From, u64 To) @key(From).
path(F, T) : edge_2(F, T).
#query reachable_from(bound u64 From, free u64 To) : path(From, To).
```
+ `key_partial_1.main.cpp` (drains the `reachable_from_bf` cursor, sorted per the cursor contract) +
`key_partial_1.stdout` (answer non-regression, 4-mode agree) + `key_partial_1.irgold` (`region opt/nodf/
nocf/none` — the declared-key render pins). Verified to compile+run this session.

### D7. Exit gate (STRUCTURAL, discriminating)
(1) **F8** (ctest): `@key(A,B,C)` → declared chain `{A}⊂{A,B}⊂{A,B,C}` present AND non-prefix `{A,C},{B},
{C}` ABSENT — absence checked against the UNION of the relation's declared prefixes (A5), so
`@key(A,B) @key(B)` legally interns `{B}`. (2) `@key(A)`/`@key(A,B)` share ONE `{A}` id + one `{}--A-->
{A}` edge; `[A,B]`/`[B,A]` TWO ordered edges into ONE `{A,B}` id (ctest). (3) **F20**: an unbound read
still returns the COMPLETE relation (`AccessCompleteness::kActiveSubset` stays unused; P5 never touches
the codegen read path). (4) **F21**: reorder two `@key` pragmas → byte-identical declared-key render
(golden). (5) NEW positive carrier golden. A stub that interns nothing / sorts within a path /
materializes the power set / conflates ordered path with schema FAILS. (6) `.rel`/`datalog.h` byte-stable
(key_partial_1 dumps == pre-P5 baseline); all existing goldens byte-unmoved; OptDiff SUITE: PASS + ctest 5/5.

---

## §5. Layer-site inventory (the P5 analog of the P1–P4 symbol grep)

| # | Site | file:line | symbol | P5 role |
|---|------|-----------|--------|---------|
| 1 | Types | RegionInstance.h (net-new) | `KeyPathId`/`RelSchemaLocalId`/`DeclaredAccessPath[Set]`/`BindingEdge`/`InternDeclaredPaths`/`InternBindingSchema`/`MaterializePrefixChain` | **add** (§4 D1/D2) |
| 2 | Model container | RegionInstance.h:319 | `RegionInstanceRelations` | **extend** — `schema_table`/`binding_edges`/`next_rel_schema` |
| 3 | RelationSchema | Regional.h:69-87 | `RelationSchema` | **extend** — `declared_access_paths` field |
| 4 | Insert arm | Planning.cpp:180-210 | `BuildRelationSchemaFromInsert` | **populate** paths (A4) |
| 4 | Origin arm | Planning.cpp:216-221 | `BuildRelationSchemaFromOrigin` | **populate** paths (A4) |
| 5 | Freeze DAG | Planning.cpp:447-453 tail | schema loop | **call** `MaterializePrefixChain` per path |
| 6 | Belt | Planning.cpp:490+ | freeze validators | **add** V-PREFIX-CHAIN (A2) |
| 7 | Render | Regional/Format.cpp:206-333 | text dump row-contract block | **add** declared-key block (A6/A7) |
| 7 | DOT badge | Regional/Format.cpp:384 | `HasInstanceKey()` badge | **keep** (advisory) |
| 8 | Census | Planning.cpp:290-309/483-488 | `DeriveRegionalCensus` | **unchanged** (no census field, D5) |
| 9 | Parse surface | Parse.cpp:858/868 | `HasInstanceKey`/`InstanceKeys` | **read** (belt + populate) |
| 10 | Gate | tests/RegionInstance/ | ctest | **add** `RegionInstanceP5` DAG gates (MANDATORY, A2) |
| 11 | Carrier | tests/OptDiff/cases/ | `key_partial_1.*` | **author** (A1/D6) |
| — | Codegen / Rel | lib/ControlFlow, lib/Rel, CodeGen | (no consumer) | **untouched** (§3 — byte-stable) |

---

## §6. LANDED (session 20) + what P6 inherits

**P5 LANDED** as one coherent cut (this session, on top of `c546a6a4`). Gate GREEN: OptDiff
`SUITE: PASS (223 cases)` (222→223, the new `key_partial_1` carrier; every existing golden
byte-unmoved), ctest **5/5** (`RegionInstance` now carries the P5 GateD–GateI DAG battery), the
`key_partial_1` `.rel`/`datalog.h`/`datalog.cpp` VERIFIED byte-identical to the pre-P5 baseline (P5
moves no codegen), and F21 confirmed (pragma reorder → byte-identical `declared-key` render). The
declared `@key` now shapes the compile-time model (the LOGICAL-ACCESS-PATH authority + the binding
schema DAG) and renders a golden-pinned `-region-out` `declared-key` line — it is no longer inert.

**What P6 inherits:** the `RelSchemaLocalId` per-relation DAG is the honest P5 stand-in for the
region-global `(region, SymbolicFieldSet)` rep — P6.2 promotes SymbolicFieldIds (the sole populator);
per A3, the P5 `schema_table` is REBUILT at P8, never migrated. The DAG is compile-time structure the
AccessPlan does not yet consume; the physical partial-key seek is P7 (`GetOrCreateIndex(subset)`).
`BindingStateId.vals` stays `{}` (value-free) until P6's real evaluation half runs.

## §7. Drift table vs the pre-P1 P5 anchors (`reconstruction-diffs §3-P5`/§5.6)

| pre-P1 anchor | drift | correction (LANDED) |
|---|---|---|
| F21 render target `DataFlow/Format.cpp:1745-1798 RenderDeclaredKeyLines` | **DELETED at P1**; `-contract-out` is now decl-@key-blind | render moved to `-region-out` text dump (`Regional/Format.cpp`), the `declared-key` block (D5) |
| `BindingStateSchema(region, field_set)` interner reuses `BindingStateSchemaId` | domain overload (A3) — that id is region-global-reserved | distinct `RelSchemaLocalId`; per-relation `(RelationId, sorted set)` key |
| V-PREFIX-CHAIN "checks DAG vs recomputed paths" | self-referential/vacuous (A2) | tied to the independent `HasInstanceKey()`; exact declared-prefix-UNION equality (A5) |
| headline carrier `#local rel @key(A). #query rel(bound A,free B).` | uncompilable — `#query` can't re-declare a `#local` name (A1) | `#local path @key(From).` read through a DISTINCT `#query reachable_from(...) : path(...)` |
| paths populated only in the insert arm | a merge-materialized `@key`'d `#local` (origin arm) drops paths (A4) | populate `declared_access_paths` in BOTH freeze arms |
| F8 "non-prefix subsets absent" per-path | false-aborts on a legal sibling (`@key(A,B) @key(B)`) (A5) | absence checked against the UNION of the relation's declared prefixes |
| `SameKeySetOfSets` order-free / P0-item-4 parser flip | **already LANDED** pre-P5 (order-significant paths) | confirmed at Parser.cpp:980-991/1475-1481 — no P5 parser change |

---

<!-- §8 (critique record) and §9 (IR desired-states) follow, folded from the refuter panel. -->

# p5-grounding.md §8 — Refuter panel critique record (synthesized)

**Method.** Opus refuter panel attacked brief D1–D7; every finding below re-verified by the synthesizer against tip code (Read/Grep + one empirical compile). Result: **0 blocking, 2 high, 3 medium (after dedup), 2 low, 15 certifications.** The DAG-anchoring findings (panel `belt-deadcode` H2 + `f20-powerset` M3) collapse to one root cause; the two `schema-edge-split` MEDIUMs collapse to one schema-typing finding.

---

## (a) Dedup map

- Panel HIGH-2 (V-PREFIX-CHAIN self-referential) **absorbs** MEDIUM `f20-powerset` M3 (".region golden does not make the DAG non-dead") — same root cause, one fix. → **A2**.
- Panel MEDIUM `schema-edge-split` #1 (BindingStateSchemaId domain overload) **+** #2 (shared-root edge-label ambiguity + dead `{}->{0}` preseed) → one schema-DAG typing finding **A3** (three concrete sub-fixes).
- All other findings stand alone.

---

## (b) Surviving findings (severity-ordered) + folded fixes

### A1 — HIGH — The D6 headline carrier `.dr` does not compile (name collision)
The literal shape at brief line 111 — `#local rel(u64 A, u64 B) @key(A). … #query rel(bound A, free B).` — is uncompilable: a `#query` on a name already declared `#local` errors `Cannot re-declare 'rel' as a local` (empirically confirmed on `hdr.dr`, plus the follow-on "declared but never defined"). `@key` rides only on `#local`/`#export`, so the keyed relation and the observing query *cannot share a name*; the keyed relation must be read through a distinct query's clause body (the shape every corpus precedent uses: `reject_key_double_1.dr:12-13`, `key_dup_1.dr:6-7`).

**Fix (fold into D6).** Replace the headline carrier with the verified-compiling shape:
```
#message edge_2_in(u64 A, u64 B).
#local rel(u64 A, u64 B) @key(A).
rel(A, B) : edge_2_in(A, B).
#query q(bound u64 A, free u64 B) : rel(A, B).
```
This compiles rc=0; `-region-out` today shows `request-port P1 query=q/2 bound=(A) plan=full-scan-filter` + `row-contract E1 rel=rel member-key=(A, B)` (@key inert). Delete the literal shape from the brief; keep the D6 parenthetical as the sole guidance. Carrier needs `q_partial_1.main.cpp` + `goldens/q_partial_1.stdout` (answer non-regression via `run_vs_golden`) **plus** the `.irgold` region step (the DAG-render golden). Confirmed coherent: the corrected program flows through the standard golden-master path.

### A2 — HIGH — V-PREFIX-CHAIN as designed is self-referential; the schema DAG is dead-except-ctest
Brief D4 claims V-PREFIX-CHAIN makes the freeze-side DAG "non-dead corpus-wide." It does not. As written it re-derives the prefix set from the *same* `decl.InstanceKeys()` the producer read and checks the interned DAG against that recomputation — no independent witness, so it is vacuously true whenever nothing was interned. This is unlike the P4 precedent it invokes: V-PLAN-HONEST cross-checks the stored `AccessPlan` against an *independent downstream fact* (codegen's actual `scanned_index` presence, Build.cpp:449-464). Verified: `Instances()`, `schema_table`, `binding_edges`, `MaterializePrefixChain` have **zero consumers** outside `RegionInstance.h` (grep clean across `lib/ bin/ include/`); `Format.cpp` reads only `Region()`/`Census()`/`DataFlowGraph()`; the D5 render reads the *ordered* `declared_access_paths`, never the order-free DAG. So for real compiles the DAG is exercised by nothing but its own belt.

**Fix (rewrite D4).** Make V-PREFIX-CHAIN a **completeness** belt tied to the independent parse authority, not a self-consistency check: for every `RelationSchema` with `decl.HasInstanceKey()==true` (Parse.cpp:858), assert `declared_access_paths` is non-empty **AND** every declared path's terminal schema is present in `schema_table` **AND** every intermediate prefix is interned (the reachability chain). This ties the DAG to `HasInstanceKey()` (a fact the belt does not itself produce), so a stub that skips `InternDeclaredPaths`/`MaterializePrefixChain` aborts instead of passing vacuously. Additionally state explicitly in D7 that **the `.region` golden does NOT cover DAG structure** — the ctest gates over `InternBindingSchema`/`MaterializePrefixChain`/`InternDeclaredPaths` are the *only* structural anchor and are therefore **mandatory**, not optional. Coherent: `HasInstanceKey()` exists and is decl-scoped; the belt runs at the freeze tail beside the existing V-REGION-CENSUS recount.

### A3 — MEDIUM — Schema-DAG typing: id-domain overload, ambiguous edge labels, dead preseed
Three defects flow from keying a per-relation DAG on the region-scoped `BindingStateSchemaId` domain. (1) `BindingStateSchemaId` is documented (RegionInstance.h:113-122) as the *region-scoped, order-free bound-field SET* identity, carried inside `BindingStateId` which has **no relation dimension**; folding `RelationId` into it via `SchemaKey{RelationId, sorted ordinals}` silently entangles relation into an authority the target rep intends region-global and cross-relation-shareable (the P8 Free-Join substrate, `SymbolicFieldId` region-global at :77-82). The P5 ids are a safe *under*-approximation (never conflate two relations) but are **structurally un-promotable** to the P6/P8 `SymbolicFieldSet` rep — the P5 `schema_table` must be *rebuilt* at P8, not migrated. (2) Under a *shared* `{0}` root, `MaterializePrefixChain` mints `{0}--f-->child` with `added_field` a bare decl-ordinal; two relations both `@key(A@ordinal0)` produce `{0}--0-->…` edges distinguishable only by child — any `(parent, added_field)` index breaks. (3) The `{}->{0}` preseed is typed `SchemaKey{RelationId, …}` but written with no relation (no well-typed key), and `MaterializePrefixChain` interns `f` *before* the empty set is ever seen, so the entry is dead and inconsistent with its own map type.

**Fix (amend D1/D2).** (a) Do **not** reuse the `BindingStateSchemaId` newtype for the DAG value — introduce a distinct P5-local id (e.g. `RelSchemaLocalId`), and reserve `BindingStateSchemaId` for the eventual region-global `SymbolicFieldSet`; add a firewall belt asserting **no `schema_table`/DAG id is ever stored into a `BindingStateId`** at P5 (trivially holds — the sole `BindingStateId` construction is `EmptyBindingState` with `{0}`, verified line 192-193). (b) Make edge labels self-describing: either qualify `added_field` as `(RelationId, ordinal)`, or scope the root per-relation (`SchemaKey{rel,{}}`) and drop the "shared `{0}` root" claim. (c) Remove the dead `{}->{0}` preseed, or special-case the empty set in `InternBindingSchema` so it is actually reachable, and reconcile with the key type. All coherent with the verified types; none touches codegen.

### A4 — MEDIUM — Origin-arm population gap: a `@key`'d relation via `BuildRelationSchemaFromOrigin` silently drops `declared_access_paths`
`RelationSchema` is aggregate-initialized **positionally** with exactly 4 fields at *both* arms (verified Planning.cpp:208-209 insert, :218-220 origin). Adding a 5th `declared_access_paths` field compiles with either arm silently value-initializing it to empty. Post-P1 `@key` is inert (no demand activation), so a `@key`'d `#local` that is merge-materialized with no INSERT view is named through the **origin** arm (`CollectOriginInteriorDecls`, Planning.cpp:93-123). If P5 wires only the insert arm (the natural spot — it already reads `row_contracts`), such a relation gets empty paths, and the A2 completeness belt (once adopted) will abort on it — or, without A2, silently under-reports (no declared-key line).

**Fix (amend D3).** Populate `declared_access_paths` from `decl.InstanceKeys()` in **both** `BuildRelationSchemaFromInsert` and `BuildRelationSchemaFromOrigin`; the A2 completeness belt then converts any missed arm from a silent under-report into a loud abort. Coherent: both arms already hold `decl`.

### A5 — MEDIUM — F8 non-prefix-absence false-aborts on a legitimate sibling path
D7(1) states F8 as `@key(A,B,C)` ⇒ `{A},{A,B},{A,B,C}` present AND `{A,C},{B},{C}` **absent**. That absent-set is valid only for a single-path carrier. The parser legally admits multiple distinct ordered `@key` paths on one decl (Parser.cpp:980-991 rejects only a repeated *identical ordered* path). With `@key(A,B) @key(B)`, `MaterializePrefixChain` legitimately interns `{A},{A,B},{B}`; `{B}` is a declared sibling, not a prefix of `[A,B]`. A per-path F8 implementation false-aborts on `{B}`. D4's parenthetical "(declared or edge-visited)" scopes it correctly, but the D7(1) restatement invites the per-path trap.

**Fix (amend D7(1)).** Specify F8 as: `schema_table` members for a given relation equal the **union over that relation's declared paths** of all their prefixes; implement the absent-check against that union, never per-path. Add a ctest carrier `@key(A,B) @key(B)` pinning no-false-abort. Coherent with the parser's multi-path legality.

### A6 — LOW — Declared-path render field names read from `RelationSchema.decl`, not the keyed redecl
D5 renders `path=(field names)` via `decl.NthParameter(ordinal).NameAsString()`, but `InstanceKeys()` returns the ordinals of the *first non-empty keyed redeclaration* (Parse.cpp:868-874) while `RelationSchema.decl` is whatever the insert/origin arm stored. Redeclarations may rename parameters (`SameKeySetOfSets` compares ordinals, not names, Parser.cpp:1475-1481), so a rendered name can differ from the pragma author's spelling. Deterministic, not a soundness bug.

**Fix.** Either resolve names through the redecl `InstanceKeys()` picked (store it beside the paths), or document that path field names render from `RelationSchema.decl`'s positional parameters and author the carrier with matching parameter names across redecls so the golden is unambiguous.

### A7 — LOW — `BindingEdge`/path types need `operator<=>`; declared-key token needs the kind_w loop
D2 stores `set<BindingEdge>` (dedup is load-bearing for D7(2) prefix-sharing) but the D1 sketch declares `BindingEdge` with no comparison operator — a `std::vector` accumulation would double-insert the shared edge and fail D7(2). Every existing id in RegionInstance.h uses `= default` spaceship (e.g. :46-47, :118-121, :187-188). Separately, the render must add `"declared-key"` to the `kind_w` per-dump max loop.

**Fix.** Give `BindingEdge`, `KeyPathId`, `DeclaredAccessPath` `operator==`/`operator<=>` = default per the house idiom; keep `binding_edges` a `std::set`. In Format.cpp, add `"declared-key"` to the kind_w computation (:264-273) gated on any-schema-`HasInstanceKey`. **Constraint** (preserves the E-K5-PAD certification): emit the declared-key line as its own width block; do not fold path text into the row-contract `rel_w`/`key_w` loop. Verified harmless: `"declared-key"`==`"row-contract"`==12 chars ≤ `"permanent-root"`==14, so the column never widens and no existing region golden moves.

---

## (c) Certifications (attacked and held — no action)

- **Prefix sharing works.** `@key(A)` reuses the `{A}` id of `@key(A,B)`; interning `sorted(running)` at every step, not just the terminal, gives one shared id + one set-deduped `{}--A-->{A}` edge.
- **`[A,B]`/`[B,A]` converge on one `{A,B}` schema via two ordered edges** — clean order-free-node / order-sig-edge split; no within-path sort leak (searched; `ordered_fields` is identity).
- **`InternDeclaredPaths` KeyPathId assignment is deterministic, order-free across pragmas, order-sig within** — pragma reorder ⇒ byte-identical render (F21).
- **F20 holds.** P5 never touches the codegen read path; `SelectAccessPlan` branches on `has_free` alone; `kActiveSubset` has zero consumers; an unbound read still full-scans the complete relation.
- **Power-set avoidance holds.** `MaterializePrefixChain` interns exactly the visited prefixes; no lattice enumeration, no consumer expands it.
- **A keyed `#local` with no query stays compile+evaluate-normal** — building the DAG is compile-time metadata; the pre-cut "unseeded/undemanded @key" reject must NOT be re-added.
- **`@key` does not become another spelling of `bound`** — `DeclaredAccessPath` sourced only from `decl.InstanceKeys()`, never fed to `AccessPlan`; the four authorities stay disjoint. (Guard a future implementer from interning a schema out of `AccessRequirement.available_bindings`.)
- **`-region-out` is the correct render home; `-contract-out` is view-keyed and decl-@key-blind.**
- **E-K5-PAD holds** — a gated declared-key line moves no existing region golden (subject to A7's own-width-block constraint).
- **The new-carrier golden is discriminating** — pins `path=(A)`, distinct from `member-key=(A, B)`; a stub reprinting the member-key or emitting no line diverges.
- **ctest-vs-golden split is correct** — `-region-out` renders no numeric schema/path id or edge, so DAG structure MUST be ctest-gated; `RegionInstanceTest` already drives model fns with synthetic ids (P3/P4 precedent), the identical attach point.
- **The carrier needs a driver + `.stdout`** (no region-only suite path); the corrected `@key #local` + distinct bound `#query` compiles today (inert), confirming answer-invariance.
- **"P5 moves no codegen" holds** — all four `FrozenRegionalProgram` consumers in `Program::Build` invariant under a `RelationSchema` field + an `Instances()` DAG; `.rel`/`datalog.h`/`.stdout` byte-stable.
- **Census identity holds** — P5 adds a field to an existing `RelationSchema`, no new port/contract; V-REGION-CENSUS stays green.

---

## (d) Verdict

**EXECUTE WITH THE LISTED AMENDMENTS (A1–A7).** No blocking finding survived; the P5 architecture (four separated authorities, order-free schema / order-sig edge, lazy prefix chain, no codegen movement) is sound and all seven of its core-invariant/false-start attacks were certified held. The two HIGH items are not design flaws but **executable defects that would waste a cycle or ship dead structure**: A1 (fix the carrier `.dr` before writing any golden) and A2 (make V-PREFIX-CHAIN a completeness belt tied to `HasInstanceKey()` and declare the ctest gates mandatory — otherwise the entire schema DAG is compile-time-dead). Both A1 and A2 must land in the brief *before* execution begins; A3–A7 fold in as the implementer reaches each seam (types, both freeze arms, F8 phrasing, render). With those seven amendments the design is coherent with the verified tip code and ready to execute.

---

# §9 — Desired P5 IR output states (predict-then-verify, STRUCTURAL pins only)

All claims below are code-verified at the working tree (branch `keyed-instances`). P5 is **model + render only**: it adds the ORDERED `DeclaredAccessPath` authority (#3) and populates the order-free `BindingStateSchema` DAG (#2's `schema_table`/`binding_edges`), renders one `-region-out` `declared-key` line, and touches **no** codegen. Every gate is structural; the full-materialization backend already answers correctly with `@key` inert.

## 9.1 The four authorities stay separate (the P5 headline)

| # | Authority | Type | Status |
|---|-----------|------|--------|
| 1 | logical fact id | `RegionalFactId` (`RegionInstance.h:197-204`) | exists (P3) |
| 2 | **order-free** binding schema | `BindingStateSchemaId` (`:115-122`) + NEW `schema_table` keyed on `(RelationId, sorted ordinal set)` | **P5 populates the DAG** |
| 3 | **ORDERED** declared access path | NEW `DeclaredAccessPath{KeyPathId, RelationId, vector<uint32_t> ordered_fields}` (order = identity) | **P5 adds** |
| 4 | physical access plan | `AccessPlan` (`:213-221`), `SelectAccessPlan` (`:241-246`) | exists (P4), **unchanged** |

Verified today `RegionInstanceRelations` (`RegionInstance.h:319-434`) has **no** `schema_table`, `binding_edges`, `KeyPathId`, or `DeclaredAccessPath` — `BindingStateSchemaId` interns only `{0}` (comment `:113-114`, `EmptyBindingState` `:192-194`), and `SortedBoundFieldValues` is always `{}` at P3 (`:164-166`). #3 must NOT conflate with #2 (order-sig path vs order-free schema) nor with #4 (P4) nor #1.

The parser already gives P5 the ordered input it needs: `key_cur_set.push_back(resolved_index)` stores **decl-ordinals** in written order (`Parser.cpp:918-944`); `@key(A,B)` vs `@key(B,A)` are DISTINCT ordered paths, both legal, dup-checked by **direct ordered-vector compare** (`Parser.cpp:980-992`); cross-redecl consistency is order-free-across-paths / order-sig-within (`SameKeySetOfSets`, `Parser.cpp:1475-1481`). Surface: `InstanceKeys()` returns `vector<InstanceKeySet>` (`Parse.h:31-32, 456-457`), `HasInstanceKey()` (`:456`), `InstanceKeyRanges()` (`:462`).

## 9.2 (1) `-region-out` declared-key render — the `key_partial_1` carrier

Carrier (must be AUTHORED — no positive `@key` case exists; corpus has only the 4 parse-reject `key_*_1` cases, all with no body/query):

```
#message edge(u64 A, u64 B).
#local rel(u64 A, u64 B) @key(A).
#query rel(bound A, free B).
rel(A, B) : edge(A, B).
```

Predicted `-region-out` (all 4 modes byte-identical; the load-bearing NEW line is `declared-key`):

```
region-program
program-root {
  input-abi   edge/2(A:u64, B:u64)          -> R0 via P0
  query-abi   rel(A:bound u64, B:free u64)  -> request-port P1
  output-abi  <none>
}
region R0  owner=program-root  parents=()  children=() {
  input-port    P0  message=edge/2  fields=(A, B)
  request-port  P1  query=rel/2  bound=(A)  plan=full-scan-filter
  row-contract  E0  rel=rel  member-key=(A, B)  support=monotone
  declared-key  E0  rel=rel  path=(A)
}
census: regions=1 child-calls=0 program-roots=1 request-ports=1 input-ports=1 result-ports=0 row-contracts=1
```

Render specifics grounded in `Format.cpp`:
- **New kind token** `declared-key` (12 chars) = same width as the always-present `row-contract` (12); it does **not** widen `kind_w` (`Format.cpp:259-273` → 12+2=14). Add one `if (declared-keys present) kind_w = max(kind_w, "declared-key")` for defensiveness; width is unchanged whenever a schema exists.
- **Layout choice (pin ONE):** a `declared-key` **block appended AFTER the row-contract block** (mirrors the ports→request-ports→permanent-roots→row-contracts block style, `Format.cpp:288-333`), iterating `relation_schemas` in E-order, emitting each schema's declared paths in **KeyPathId order** (KeyPathId is assigned deterministically sorted-by-ordered_fields, so a **pragma reorder is byte-identical** — F21). Reuse the row-contract `etok_w`/`rel_w` columns so `E<k>`/`rel=` align; `path=(…)` is the trailing unpadded token (like `support=`).
- `plan=full-scan-filter` on the request port is P4-selected (`ComputeQueryAccessPlan`/`SelectAccessPlan`, `Planning.cpp:242-258`; bound+free → `kFullScanFilter`), **not** a P5 change.
- **Predict-then-verify caveat:** `rel`'s `member-key=(A, B)`/`support=monotone` come from the Stage-A row contract (`BuildRelationSchemaFromInsert`, `Planning.cpp:180-210`), NOT from `@key` — verify at build (a plain `rel(A,B):edge(A,B)` should yield AllFields member key like `join_1`'s `rel=p member-key=(A, B)`). The `declared-key` line is load-bearing regardless of that value.

**Existing region goldens do NOT move.** `booleans` and `join_1` (verified content: `booleans.region.opt.golden`, `join_1.region.opt.golden`) have no `@key` → `HasInstanceKey()` false → **zero** `declared-key` lines; a skip-when-empty block leaves kind/port/contract widths and the census untouched. Same for `merge_2`, `tc_nonlinear_diff`. **Do NOT add a census field for declared keys** — that would move all 16 region goldens; declared-key is render-only (census belt `check_count("row-contracts", …)` at `Planning.cpp:486-488` stays keyed on `relation_schemas.size()`).

Attach points for the build: add `DeclaredAccessPathSet declared_access_paths` to `RelationSchema` (`Regional.h:69-87`), populate from `decl.InstanceKeys()` in both schema arms (`Planning.cpp:180-221`, schema loop `:447-453`); after building `relation_schemas`, call the new `MaterializePrefixChain` per declared path to populate `instances.schema_table`/`binding_edges`. Keep the DOT badge (`Format.cpp:384`, `decl.HasInstanceKey() ? " declared-key"`) — advisory, never goldened, complementary to the new text line.

## 9.3 (2) DAG-structure ctest gates (NOT `.dr` goldens)

The convergence carrier `@key(A,B) @key(B,A)`, the F8 carrier `@key(A,B,C)`, and the prefix-share `@key(A) ⊂ @key(A,B)` are best as new `RegionInstanceP5` TESTs over `InternDeclaredPaths` / `InternBindingSchema` / `MaterializePrefixChain` with **synthetic** `InstanceKeySet`s (a `.dr` golden cannot see internal ids) — attaching exactly as P4 `GateA_SelectAccessPlan` calls `SelectAccessPlan` directly (`RegionInstanceTest.cpp:257-264`; `KeyedState` synthetic-state precedent `:233-236`). The gates assert:

- **F8 `@key(A,B,C)`** (ordered_fields `[0,1,2]`): `InternDeclaredPaths` → 1 path, `KeyPathId{0}`, `ordered_fields==[0,1,2]`. `MaterializePrefixChain` → `schema_table` = exactly the 4 visited prefixes `{}, {0}, {0,1}, {0,1,2}`; `binding_edges` = exactly 3 `({},0,{0}),({0},1,{0,1}),({0,1},2,{0,1,2})`. **Non-prefix subsets ABSENT:** `schema_table.count({rel,{0,2}})==0`, `{1}==0`, `{2}==0`, `{1,2}==0` (no power-set materialization).
- **Convergence `@key(A,B) @key(B,A)`**: `InternDeclaredPaths` → **2 DISTINCT** ordered paths (`[0,1]`, `[1,0]`), 2 KeyPathIds (#3 keeps order). `MaterializePrefixChain` → both terminals **converge on ONE** `{0,1}` schema id (`schema_table[{rel,{0,1}}]` single value); intermediates `{0}` and `{1}` distinct; `binding_edges.size()==4` with **TWO ordered edges into `{0,1}`**: `({0},1,{0,1})` and `({1},0,{0,1})`.
- **Prefix-share `@key(A)` ⊂ `@key(A,B)`**: the `{0}` schema id is **SHARED** (one value) and the `({},0,{0})` edge is **not duplicated** → `schema_table` = `{}, {0}, {0,1}`, `binding_edges.size()==2`.
- **Per-relation DAG (P5 honest field identity):** the SchemaKey is `(RelationId, sorted ordinal set)` (the P5 stand-in for the P6.2-populated `(region, SymbolicFieldSet)` — `SymbolicFieldId` is reserved, `RegionInstance.h:77-82`, comment `:74-76`). So `rel1 @key(A)` and `rel2 @key(A)` get DISTINCT `{A}` ids (no cross-relation conflation; Free-Join sharing is P8).

A stub that interns nothing / sorts within a path / materializes the power set / conflates ordered path with schema **FAILS** these gates. Recommend an always-on freeze validator **V-PREFIX-CHAIN** (fprintf+abort, NDEBUG-surviving, the P4 `V-PLAN-HONEST` idiom) asserting: every declared path's terminal is reachable from `{0}` via interned edges with every intermediate interned, AND `schema_table` members are **exactly** the visited prefixes (F8 non-prefix-absent) — makes the freeze-side DAG non-dead corpus-wide.

## 9.4 (3) `.rel` and generated `datalog.h`/`.cpp` are BYTE-IDENTICAL

**Prediction:** for any program, `.rel` (emitted from `lib/Rel` inside `Program::Build`) and `datalog.h`/`datalog.cpp` (CodeGen) are byte-identical with vs without P5.

**Reason (grounded):** P5 moves no codegen. The `schema_table`/`binding_edges`/`DeclaredAccessPath` live only on the freeze-side `RegionInstanceRelations`/`RelationSchema` model; **nothing in `Program::Build` or CodeGen consumes them** — the AccessPlan (#4) is the only freeze→codegen dependency and it is P4-fixed (`PlanFor`, `Regional.h:208`; read by `BuildQueryEntryPointImpl`), untouched by P5. The schema DAG is compile-time structure the AccessPlan does not yet consume (the physical partial-key seek is P7's `GetOrCreateIndex(subset)`). `@key` stays inert in `.rel`/codegen at P5. For the NEW `key_partial_1`, pin this mechanically: its `.rel`/`.df`/`.ir`/`.h` goldens should be **byte-identical to the same program with `@key(A)` stripped** — the cleanest form is a symlinked twin (the `key_tc_witness`→`demand_tc_witness` symlink precedent), or a documented strip-recompile-diff at bless-review. `.stdout` is answer-invariant (the full-mat backend scans the materialized `rel` filtered on `A`; `@key` changes nothing stored or emitted).

## 9.5 (4) Goldens that MOVE vs STAY

**MOVE (all NEW — the authored carrier + new ctest, nothing re-blessed):**
- `key_partial_1.dr` + `key_partial_1.main.cpp` (author) → `key_partial_1.stdout` (one golden, 4 modes agree) + `key_partial_1.region.{opt,nocf,nodf,none}` (pins the `declared-key` line; via the `region` `.irgold` sidecar).
- `key_partial_1.rel`/`.df`/`.ir`/`.h` — either symlinked to a `@key`-stripped twin (recommended, mechanizes 9.4's byte-identity) or the twin's own goldens.
- `tests/RegionInstance/RegionInstanceTest.cpp` — new `RegionInstanceP5` gates (9.3). **ctest, not goldens.**

**STAY byte-identical (must not move — the regression fence):**
- All existing region goldens: `booleans`, `join_1`, `merge_2`, `tc_nonlinear_diff` `.region.{opt,nocf,nodf,none}` (16 files) — no `@key` → zero declared-key lines, census unchanged.
- All 11 `.rel` goldens, all `.df`/`.ir`/`.h`/`.stdout` across the 176-case suite (no codegen movement, no census field).
- The 4 parse-reject `key_*_1` cases stay diagnostic (reject at parse, never reach freeze — P5 doesn't touch parse); the `reject_key_*` corpus (10 cases) stays.

**Exit gate (structural):** OptDiff `SUITE: PASS` + ctest 5/5, with (a) the new `key_partial_1.region.*` declared-key golden green; (b) `key_partial_1` `.rel`/`.h` byte-identical to the `@key`-stripped twin; (c) the P5 DAG ctest gates (F8 non-prefix-absent, [A,B]/[B,A] convergence into one schema via two ordered edges, `@key(A)⊂@key(A,B)` prefix share); (d) F20 preserved — an unbound read still returns the COMPLETE relation (assert the P5 model never diverts a read to an active subset; `AccessCompleteness::kActiveSubset` stays unused, `RegionInstance.h:225`); (e) all existing goldens byte-unmoved. A stub that ignores `@key` emits no `declared-key` line → (a) fails; one that materializes the power set or conflates ordered path with schema → (c) fails.
