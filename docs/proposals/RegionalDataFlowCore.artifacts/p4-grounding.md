# P4 grounding — honest complete-path specialization (`AccessPlan` / `FullScanFilter`)

Session 19 grounding output. Branch `keyed-instances`, code byte-current at `63573a67` (tip
`e9420395` is the session-19 docs handoff). This is the code-verified P4 execution-readiness record:
the layer-site inventory, the settled **(a)-reuse-vs-(b)-fresh-scan** question, the typed additions,
the point where the P3 model first DRIVES evaluation, the D1 terminal-`BindingStateId` resolution,
the reframed **discriminating** B3 gate, and the IR desired-states. It is the sibling of
`p3-grounding.md`; where it disagrees with `reconstruction-diffs.md` §3-P4/§5.5 (which predate P1/P2/P3)
this doc wins — the drift table §5 records every correction.

Method: orchestrator-driven inventory (every anchor grep-verified against real post-P3 code at tip),
then an adversarial refuter panel (§8), each finding verified against the codebase before ranking.

---

## §1. THE HEADLINE — what P4 actually is, and the two things it must decide

**P4 is the first phase where the compile-time Regional model DRIVES a codegen change** (REVISED
§8-S3: it drives a codegen CHANGE via a compile-time authority READ — it does NOT run a runtime
evaluation; the `derivations`/`routed_results` half of the model stays ctest-only-synthetic at P4,
P3-style). P3 built the request/derivation model (`RegionInstanceRelations`) but left it a pure
compile-time observer, and a bound `#query` evaluates entirely via the retained full-materialization
backend (`BuildQueryEntryPointImpl` → `impl->queries` → `EmitQueryFriends`). P4 makes the model:

1. compute an **`AccessRequirement`** per bound-query request edge (relation + `has_free` +
   available bindings + completeness) and select an **`AccessPlan`** (`kFullScanFilter` for bound+free /
   `kFullKeyHashLookup` for all-bound; `kRetainedIndexScan` RESERVED P7 — REVISED §8-S2/S4) AT FREEZE;
2. **STORE the selected `AccessPlan` on the frozen `RequestPortRecord`** (REVISED §8-S1 — the real
   compile-time data dependency that makes the authority non-nominal); and
3. **READ + CONSUME the stored `AccessPlan` in codegen** so a bound+free query's physical read becomes
   an honest scan-and-filter (`kFullScanFilter` → withhold the index) instead of the retained index
   seek, guarded by the always-on V-PLAN-HONEST belt.

The codegen-DRIVE claim is scoped to the **bound+free slice** (`force`/`average_weight`
are the codegen-discriminating carriers; `booleans`, all-bound, is the model-half carrier — its
`.Find` stays byte-identical and only its `-region-out` gains a `plan=` token; REVISED §8-S12).
Downstream (P5 partial binding, P6 recursion + the real evaluation half, P7 hash/trie planning) makes
steps 1-3 richer. P4 keeps `AccessPlan` a DISTINCT fourth authority (physical structure), never
conflated with the logical fact id, the residual binding state, or the logical access path.

### §1.1 The two design questions P4 must settle (both settled below, §3)

- **Q1 — where does `AccessPlan` live, and is there a `.rel` op?** VERIFIED: the Rel `.rel` IR does
  NOT model query reads at all (a `#query` never appears in any `.rel` dump; Rel models the DR flow —
  strata / folds / eager markers). So the `reconstruction-diffs` §3-P4(c) "mint `kAccessKeyedRelation`
  in Rel" would bolt a brand-new modeling surface onto a layer that doesn't cover query reads.
  **RESOLUTION (§3.1):** `AccessPlan`/`AccessRequirement` are Regional-model types (`RegionInstance.h`),
  rendered in `-region-out` as a plan annotation on the request port, and consumed by ControlFlow's
  `BuildQueryEntryPointImpl`. **NO Rel op at P4.** The `.rel` `kAccessKeyedRelation` surface is
  deferred to P7/P8 (or never — query reads may stay a pure ControlFlow concern). This keeps the
  `booleans.rel.opt.golden` byte-stable unless P4 chooses to render an annotation there.

- **Q2 — reuse the existing seam (a) or mint a fresh `ProgramTableScanRegion` (b)?** This is the
  seed §3-P4 open question. It hinges on a codegen finding that INVALIDATES the diff's premise (§2).
  **RESOLUTION (§3.2):** neither as originally framed — **(a′) the AccessPlan drives
  `BuildQueryEntryPointImpl` to withhold the index (`scanned_index = nullopt`)**, reusing
  `EmitQueryFriends`'s EXISTING full-scan-filter cursor arm. This is the "deliberately simple backend"
  Phase 4 asks for, and it is fully discriminating once the B3 gate is re-anchored off `s<id>`/`pos`
  (§4). Path (b) — a fresh scan region — is heavier (a query is a PULL cursor, a scan region is a PUSH
  body; see §3.3) and is deferred.

---

## §2. The codegen finding that reframes P4 (the diff's premise is false for the carriers)

`reconstruction-diffs.md` §5.5 asserts "the baseline cursor already emits a NumRows scan + key
filter", so its four probes are non-discriminating. **VERIFIED FALSE for every bound-query carrier.**
The retained backend emits, per carrier shape:

| Carrier | shape | current baseline emission (VERIFIED in generated `datalog.h`) |
|---|---|---|
| `booleans` `user_is_logged_in(bound i32)` | all-bound (bool) | `return db.user_is_logged_in_11.Find({UserId}) != kNoRow;` — a **hash probe** (`.Find`), NOT a scan. The existence arm IGNORES `spec.index` entirely. |
| `force` `get_next_id(bound i64, free u32)` `@first` | bound+free (cursor) | `struct get_next_id_bf_cursor { … pos = db.idx_24.Next(id); … }; return {db, Time, db.idx_24.First({Time})};` — an **index seek** (`idx.First/Next`), cursor member `pos`. |
| `average_weight` `average_incoming_weight(bound i32, free i32)` | bound+free (cursor) | `struct …_bf_cursor { … pos = db.idx_248.Next(id); … }; return {db, X, db.idx_248.First({X})};` — an **index seek**, member `pos`. |

So the baseline is NOT a full scan — it is `.Find` (all-bound) or `idx.First/Next` (bound+free), both
of which are physically STRONGER than `FullScanFilter`. **P4's FullScanFilter is therefore a genuine,
physically-slower, structurally-distinct emission** — which is exactly what makes a real P4
discriminable from a stub. P7 restores the seek (`AccessPlan::kFullKeyHashLookup`/trie).

**The FullScanFilter mold already exists in TWO places** (P4 needs no new emission code):
- `EmitQueryFriends` (Database.cpp:1527), the query-cursor path — when `spec.index` is absent, the
  bound+free cursor emits `while (pos < db.<member>.NumRows()) { id = pos++; … if (row.<field> !=
  <param>) { continue; } … }` — an honest FullScanFilter in the **`pos`** idiom. (Verified in the
  `via_index=false` arm, Database.cpp:1651-1690.)
- `EmitScan` (Database.cpp:2778), the region path — when `Index()` is `nullopt` and `InputVariables()`
  is non-empty, it emits `for (uint32_t s<id> = 0; s<id> < <member>.NumRows(); ++s<id>) { const auto
  r<id> = …RowAt(s<id>); if (r<id>.<field[indexed_cols[k]]> == <input>) { … } }` — the FullScanFilter
  in the **`s<id>`** idiom, asserting `IndexedColumns().size() == InputVariables().size()`
  (Database.cpp:2848).

Both are already tested, live code. P4 chooses which to activate (§3.2).

---

## §3. Resolutions

### §3.1 `AccessPlan` lives in the Regional model; no Rel op at P4 (Q1)

**REVISED §8-S4/S5.** Add to `include/drlojekyll/Regional/RegionInstance.h` (the physical-structure
authority, a peer of the request/derivation model):

```cpp
// The PHYSICAL-STRUCTURE authority (distinct from logical fact id / binding state / access path).
// Three arms are LIVE at P4 (each names an emission codegen actually produces); P7 adds the trie arms
// and DUAL-HOMES a Rel plan_kind for interior/join scans (this doc supersedes p7p9-diffs §7-pin4 for
// the QUERY-read arm — §8-S5).
enum class AccessPlan : uint8_t {
  kFullScanFilter = 0,      // P4: withhold the index; full scan + bound-col filter (any bound+free
                            //     query — an acyclic scan of the fully-materialized relation, §8-S2)
  kFullKeyHashLookup = 1,   // P4: the all-bound `.Find` (an honest full-key hash probe; unchanged emit)
  kRetainedIndexScan = 2,   // RESERVED P7 (cost-based seek retention); default/sentinel
  // kExistingTriePrefix, kEnumeratePrefix, kBuildLazyOrdering,  // RESERVED P7/P8
};

enum class AccessCompleteness : uint8_t { kCompleteRelation, kActiveSubset /*RESERVED P5*/ };

// What a request edge needs to read. `available_bindings` is decl-ordinal (§8-S11: P5 upgrades to
// typed BoundFieldValue + required_fields). completeness is always kCompleteRelation at P4.
struct AccessRequirement {
  RelationId relation;
  bool has_free{false};              // arity classifier (all-bound vs bound+free) — §8-S4
  std::vector<uint32_t> available_bindings;   // decl-ordinal bound-column indices
  AccessCompleteness completeness{AccessCompleteness::kCompleteRelation};
};

// Branching selector (§8-S2/S4): bound+free → full scan (answer-correct over recursive relations too —
// the read is an acyclic scan of the settled table); all-bound → `.Find`. kRetainedIndexScan is a P7
// cost-based arm (no recursion gate at P4 — §8-S2).
inline AccessPlan SelectAccessPlan(const AccessRequirement &req) {
  if (!req.has_free) return AccessPlan::kFullKeyHashLookup;  // all-bound → .Find (unchanged)
  return AccessPlan::kFullScanFilter;                        // bound+free → scan
}
```

The selected `AccessPlan` is COMPUTED AT FREEZE (`BuildRequestPorts`) and STORED on `RequestPortRecord`
(`AccessPlan plan` — §8-S1). `relation_is_recursive` = a freeze-side replica of `ViewSelfReachable`
(Build.cpp:227 — reachability, NOT the induction flag) over the query relation's INSERT view.
`-region-out` renders it on the request-port body line, e.g.
`request-port P2 query=user_is_logged_in/1 bound=(UserId) plan=full-key-hash-lookup`; a `plan=` badge is
added to the advisory `-region-dot-out` twin (§8-S12). NO `.rel` op; NO codegen dispatch arm (the scan
mold already exists, §2).

### §3.2 (a′): the plan drives `BuildQueryEntryPointImpl` to withhold the index (Q2)

**REVISED §8-S1/S2/S7.** The plan is SELECTED+STORED at freeze and READ at codegen (the model
genuinely drives — not re-derived inline):

```
# --- FREEZE (Planning.cpp BuildRequestPorts, per bound-query redecl) ---
    req  = AccessRequirement{ RelationId{decl.Id()},
                              has_free = any(!p.bound for p in redecl.params),
                              available_bindings = bound decl-ordinals }   # no recursion gate — §8-S2
    plan = SelectAccessPlan(req)                  # §3.1 branching selector
    R.request_ports.push(RequestPortRecord{ …, plan })   # STORE on the frozen model (§8-S1)

# --- CODEGEN (BuildQueryEntryPointImpl, Build.cpp:413; identical gate at BuildEmptyQueryEntryPointImpl
#     :455-477 per §8-S7) ---
    …col_indices = bound decl-ordinals (unchanged)…
    plan = ctx.frozen->PlanFor(redecl)           # READ the stored plan (matched by redecl; §8-S1)
    scanned_index = nullopt
    if plan != kFullScanFilter and not col_indices.empty():
        scanned_index = GetOrCreateIndex(col_indices)   # kFullKeyHashLookup / kRetainedIndexScan
    assert(plan != kFullScanFilter or !scanned_index.has_value())   # V-PLAN-HONEST belt (§8-S1)
    impl->queries.emplace_back(query, table, scanned_index, forcer, retract)
```

With `scanned_index == nullopt`, `EmitQueryFriends`'s `via_index=false` arm emits the honest
FullScanFilter cursor (bound+free) — **no new emission code, no new region kind, no Database.cpp
dispatch arm.** The plan is CONSUMED from the model (a stub that ignores it and keeps
`GetOrCreateIndex` trips V-PLAN-HONEST / reverts the header — the NEGATIVE WITNESS, §4/§8-S1). `ctx`
gains `const FrozenRegionalProgram *frozen` (set by `Program::Build` beside `frozen_census`); `PlanFor`
matches the `redecl` against `frozen.Region().request_ports`.

**All-bound existence queries (`booleans`).** The `!has_free` arm emits `.Find` regardless of
`spec.index` (Database.cpp:1625-1640) — dropping the index changes NOTHING there. `.Find` is a hash
probe on the relation's primary dedup table = an `AccessPlan::kFullKeyHashLookup` the M3 backend
already emits, which Phase 4 explicitly permits ("Use `FullKeyHashLookup` only when code generation
emits one"). **RESOLUTION:** the all-bound slice is classified `kFullKeyHashLookup` (a plan whose
codegen pre-exists) and its emission is UNCHANGED at P4; the P4 codegen change lands on the
**bound+free** complete-path slice. (`booleans` therefore keeps its `.Find` and its `.rel`/`.h`/`.stdout`
byte-stable; it remains the MODEL carrier — its `-region-out` gains only `plan=full-key-hash-lookup`.)
This keeps `AccessPlan` honest — every arm names an emission codegen actually produces — and confines
the golden-moving codegen change to `force`/`average_weight`.

### §3.3 Why NOT (b) a fresh `ProgramTableScanRegion` at P4

A `ProgramTableScanRegion` (Program.h:1028; minted like Join.cpp:254 via
`operation_regions.CreateDerived<TABLESCAN>` + `table`/`index`/`in_cols`/`in_vars`/`out_vars`) is a
PUSH region: its body runs once per matching row, inside a procedure. A `#query` is a PULL cursor
(`cursor.next(out...)` drains lazily) synthesized from the `impl->queries` record — it is NOT a
procedure and has no scan region. Routing a query through a scan region requires either (b1) a new
query procedure that scans into a result vector the cursor drains (an ABI shape change + a new
procedure) or (b2) teaching `EmitQueryFriends` to consult the plan — which is just (a′) with a region
object in the way. Both are heavier than (a′) and neither is needed for FullScanFilter. **(b) is
deferred**; the `ProgramTableScanRegion`-reuse mechanism is CERTIFIED sound (§2, EmitScan's index-less
arm) and is the natural home for the P6 RECURSIVE regional reads, not the P4 acyclic query read.

---

## §4. The B3 discriminating exit gate — reframed (STRUCTURAL, never answer-equality)

The seed/diff B3 gate keys on region-cursor `s<id>` vs query-cursor `pos`. **That discriminator is
wrong for (a′)** — (a′) stays in the `pos` cursor idiom (it reuses `EmitQueryFriends`). The real,
verified discriminator is *the shape of the read*, tied to the threaded bound value:

- **(B3-1) positive structural, bound+free carrier (`force`):** the generated `get_next_id_bf_cursor`
  must contain `while (pos < db.get_next_id_<n>.NumRows())` AND `if (row.<field> != Time) { continue; }`
  and must NOT contain `idx_<n>.First(` / `idx_<n>.Next(` for that query. Baseline has the opposite
  (an `idx.First/Next` seek, no `NumRows`, no field re-check). A stub (ignore the plan, keep
  `GetOrCreateIndex`) emits the seek → FAILS this probe. *This is the load-bearing discriminator.*
- **(B3-2) model / `-region-out`:** the bound query's request port renders `plan=full-scan-filter`
  (bound+free) / `plan=full-key-hash-lookup` (all-bound); census unchanged from P3. A stub that never
  computes an `AccessRequirement` renders no `plan=` token.
- **(B3-3) RoutedResult in the model (not codegen):** the `RegionInstance` ctest gains a case that
  drives the P4 evaluation extension (§6) with SYNTHETIC rows over a non-empty `BindingStateId` and
  asserts (i) `AddDerivation` interns facts through the member-key mask, (ii) `RouteResults` populates
  `routed_results` for the requester, (iii) a SECOND request edge to the same `dest` adds a
  `RoutedResultId` with ZERO new `FactDerivation` (owner ⟂ support — a blind full read cannot fake
  this). This moves the caller-qualified check off a `datalog.h` grep (§5.5's correction).
- **(B3-4) D1 filter-constant identity:** the emitted filter constant equals the THREADED bound
  parameter — in (a′) the `if (row.<field> != Time)` names the query's own bound parameter `Time`, so
  the terminal `BindingStateId`'s value provenance is observable at the codegen boundary. (At P4 the
  value is the runtime parameter, not a compile-time constant; the identity checked is
  parameter-name↔filter-operand, not a literal.)
- **(B3-5) answer non-regression (necessary, NOT sufficient):** `force.stdout` / `average_weight.stdout`
  / `booleans.stdout` byte-identical (answers unchanged — FullScanFilter is a correct realization).
  Answer-equality alone is a LOST CHECK; it rides ALONGSIDE B3-1..4.

---

## §5. D1 — the terminal `BindingStateId` (non-empty `vals`) without dragging P5 in

P4 needs a `BindingStateId` whose `vals` are the query's bound values, so `AddDerivation`'s
`src_state`/`contributing_states` and `RouteResults` operate on a real (non-empty) state. P3 interned
only `EmptyBindingState`. D1's minimal, P5-free realization:

- **Schema id:** intern ONE non-empty `BindingStateSchemaId` per distinct bound-column SET of a
  bound `#query` (the complete-path set = all bound cols). This is a SCHEMA id (field-set identity),
  value-free — NOT the P5 partial-binding DAG (no `BindingEdge`, no prefix chain, no order-significant
  navigation). P4 mints only the terminal (complete) schema; P5 adds the intermediate prefixes + edges.
- **Value tuple:** at COMPILE time the values are unknown (they are runtime query parameters). So P4's
  `BindingStateId.vals` in the codegen path is SYMBOLIC — the state is identified by its schema; the
  concrete `RegionalCellValue`s only exist when a synthetic row drives the model in the ctest (B3-3).
  For the compile-time model + `-region-out`, the terminal state is `{ri, schema=<complete set>, vals={}}`
  at the schema level, and the codegen filter binds the runtime parameter (B3-4). This keeps D1 a
  SCHEMA-forward pull, exactly what §193's note ("a SCHEMA id, not value-bearing") intends.
- **Guard against P5 creep:** P4 asserts every minted non-empty schema is the COMPLETE bound set
  (`available_bindings == all bound cols`); a partial set is a P5 reject at P4 (not silently
  materialized). This is the `completeness == kCompleteRelation` invariant.

Latent-at-P3 fields (`SortedBoundFieldValues` per-field association, §8-F7) stay as documented; P4
does not sort-collapse because P4 has at most one schema per query (no `A=1,B=2` vs `A=2,B=1` ambiguity
until P5's multi-field partial states).

---

## §6. RESERVED P6 evaluation semantics — exercised at P4 ONLY via the ctest (REVISED §8-S3)

**This section is the RESERVED P6 evaluation semantics; at P4 it is NOT a compile-time sweep.** At P4
`RegionTemplate.rules` stays empty, `AddDerivation`/`RouteResults` have NO real-compile callers (the
terminal `vals` are runtime query params, empty at compile), and `EvaluateEpoch` is a doc symbol, not
code. The M3 backend computes the actual published answer; P4 changes only the query READ shape
(§3.2). The pseudocode below is the P6 target — at P4 it is realized ONLY as the `RegionInstance`
ctest driver (§4 B3-3), which runs the `FullScanFilter` iterator over SYNTHETIC rows so
`AddDerivation`/`RouteResults` are exercised on a non-empty `BindingStateId`:

```
EvaluateEpoch (P4 acyclic complete-path slice):
    live = RootedReachability(ri)                     # {EmptyBindingState} ∪ live request dests
    for e in request_edges where RootAlive(e.owner):
        req  = AccessRequirement{ e.requested_relation, bound_cols_of(e), kCompleteRelation }
        plan = SelectAccessPlan(req)                  # kFullScanFilter
        st   = terminal BindingStateId of e           # D1: complete-set schema (§5)
        for (row, sign) in FullScanFilter(model_table_of(req.relation), req.available_bindings, vals):
            AddDerivation(st, req.relation, member_key_positions_of(req.relation), row, sign)  # H3
        RouteResults(st)                              # now NON-EMPTY for a bound query
    RetractRoutedResults(all_states \ live)
```

`FullScanFilter(table, bound_cols, vals)` is the abstract model iterator (the ctest supplies synthetic
rows; the compiled program realizes it via §3.2 codegen). This is the FIRST program-driven invocation
of `AddDerivation`/`RouteResults` — the P3 dormancy is lifted. The RECURSIVE arm (rule DAG, activation
edges, joint fixpoint) stays RESERVED for P6 (the false-start "don't extend only InstanceStore" and
"don't reference-count cyclic activation" are P6 concerns, not touched here).

---

## §7. Layer-site inventory (the P4 analog of the P1/P2/P3 symbol grep)

Every current site P4 reads, drives, or leaves untouched. All anchors VERIFIED this session against
the code at `63573a67` (tip `e9420395` is the docs handoff); the seed §0 drift is folded.

| # | Site | file:line | symbol | P4 role |
|---|------|-----------|--------|---------|
| 1 | Query-entry seam | Build.cpp:413 / :439-443 | `BuildQueryEntryPointImpl` / `scanned_index = GetOrCreateIndex(col_indices)` | **DRIVE** — consult AccessPlan; withhold the index for kFullScanFilter (§3.2) |
| 1 | Bound-col walk | Build.cpp:422-426 | `param.Binding()==kBound` → `col_indices.push(param.Index())` | **read** (AccessRequirement source) |
| 1 | Empty-query arm | Build.cpp:455/:481 | `BuildEmptyQueryEntryPointImpl` | **mirror** the same plan branch (an empty-table query is trivially FullScanFilter over 0 rows) |
| 2 | Query cursor emit | Database.cpp:1527 | `EmitQueryFriends` — `via_index` branch (:1651) + `.Find` existence arm (:1625) | **reused unchanged** (the `via_index=false` arm IS the FullScanFilter, §2) |
| 3 | Region scan emit | Database.cpp:2778 | `EmitScan` (index-less arm :2843-2860) | **NOT used at P4** (deferred to (b)/P6; certified sound) |
| 3 | Scan handle / mint | Program.h:1028 / Operation.cpp:1202 / Join.cpp:254 | `ProgramTableScanRegion` / `AsTableScan` / `CreateDerived<TABLESCAN>` | **NOT minted at P4** |
| 4 | Model — request half | Planning.cpp:343-386 | `BuildRequestPorts` | **extend** — attach `AccessRequirement`/`AccessPlan` per request port |
| 4 | Model — eval half | RegionInstance.h:337/:374/:359 | `AddDerivation` / `RouteResults` / `RootedReachability` | **invoke** (§6) — the ctest drives them (B3-3) |
| 4 | Model — types | RegionInstance.h (net-new) | `AccessPlan` / `AccessRequirement` / `SelectAccessPlan` | **add** (§3.1) |
| 4 | BindingStateId | RegionInstance.h:182/:192 | `BindingStateId` / `EmptyBindingState` | **extend** — intern one complete-set schema (§5, D1) |
| 5 | Render | Format.cpp (Regional) | request-port body line | **extend** — `plan=…` token (§3.1) |
| 5 | Census | Planning.cpp:225/:228 | `DeriveRegionalCensus` | **unchanged** (P4 adds no port; plan is an attribute) |
| 6 | Gate | tests/RegionInstance/ | the ctest | **extend** — the B3-3 RoutedResult-over-non-empty-state case |
| — | Rel IR | lib/Rel/* | (query reads unmodeled) | **untouched** (Q1 — no `.rel` op at P4) |

---

## §8. Critique survivors — adversarial refuter panel (session 19, folded)

Six opus refuters (q1-rel-op / q2-drop-index / allbound-find / d1-drives-eval / b3-gate /
completeness) + a synthesizer attacked this doc against real POST-P3 code + the retained invariants +
the false-starts checklist; every finding verified against the tree. **Verdict: the DIRECTION is
sound and all MECHANICAL claims certify, but the doc-as-first-written must NOT be implemented
literally — three BLOCKING defects + a HIGH contradiction require the revised design below.** The
revision is folded into §1/§3/§6/§9 (marked "REVISED §8-Sn"); §1.1-Q1, §2, §3.3 HELD unchanged.

**S1 · BLOCKING · the AccessPlan authority was NOMINAL — B3 gated emission SHAPE, not model
CONSUMPTION.** As first written, `BuildQueryEntryPointImpl` would RE-DERIVE the drop-index decision
from `col_indices`; `frozen.Instances()`/the plan is never threaded to codegen, so a plan-blind stub
passes every probe (the recurring "regional layer is nominal" critique). **FIX (adopted):** SELECT the
plan at freeze and STORE it on `RequestPortRecord` (`AccessPlan plan`); thread `frozen` into `Context`
(beside `frozen_census`, Build.cpp:1324); `BuildQueryEntryPointImpl` READS the stored plan (matched by
`redecl`) and withholds the index iff `plan == kFullScanFilter`. Add an always-on **V-PLAN-HONEST**
belt (`plan == kFullScanFilter ⟹ !scanned_index.has_value()`, fprintf+abort) + a one-time NEGATIVE
WITNESS at execution (perturb the stored plan → the emitted `force` header reverts to `idx.First` →
proves codegen follows the model, then revert). This is the real compile-time data dependency that
makes "the model drives" non-hollow (also discharges S3). **VERIFIED at execution:** flipping
`SelectAccessPlan`'s bound+free arm to `kRetainedIndexScan` and rebuilding flipped `force`'s
`-region-out` to `plan=retained-index-scan` AND its generated header back to the `idx_24.Next(id)`
seek (index member/maintenance restored); reverting restored `full-scan-filter` + the scan cursor.

**S2 · BLOCKING (premise CORRECTED at execution — a recursion gate is UNNECESSARY, not adopted).**
The panel feared unconditional `kFullScanFilter` "drops the seek for ~30 recursive bound queries
(`fibonacci`, TC, `disassemble`)." **Execution disproved the premise, three ways:** (1) **every corpus
bound-query relation is a NON-RECURSIVE PROJECTION** — `fib(N,Res) : fib_impl(N,Res)`,
`reachable_from(F,T) : tc(F,T)` (verified in the `.dr` + `.df`); the recursion lives in the interior
`#local` (`fib_impl`/`tc`), which has NO `#query`. The query relation itself is never self-reachable,
so it is legitimately non-recursive. (2) Even for a SYNTHETIC directly-recursive query
(`#query tc(bound,free)` with `tc:tc,tc`), `full-scan-filter` is **answer-correct** — the query READ
is an acyclic scan of the FULLY-MATERIALIZED table (the induction fixpoint completes during message
handling, before the query runs), and the recursion's OWN indexes are **untouched** (verified: the
`idx_28.First`/`idx_29.First` join seeks remain in the generated flow; only the query's convenience
cursor changes). (3) Phase 4 explicitly says "the initial physical realization **may be**
FullScanFilter" — a scan of a settled relation is sanctioned. A freeze-side `ViewSelfReachable` over
the INSERT view also proved to UNDER-report (the INSERT is a sink with empty `Successors()`; the cycle
is on the materializing MERGE). **RESOLUTION:** DROP the recursion gate — `SelectAccessPlan` returns
`kFullScanFilter` for every bound+free query and `kFullKeyHashLookup` for every all-bound query.
`kRetainedIndexScan` stays RESERVED (P7 cost-based seek retention / the default sentinel). Charter-pure
recursion-aware planning is a P7 cost decision, not a P4 correctness gate.

**S3 · BLOCKING · "the model DRIVES evaluation / dormancy lifted / non-empty derivations half" was
hollow.** `AddDerivation`/`RouteResults` have ZERO real-compile callers (terminal `vals` are empty at
compile time — values are runtime query params), and `EvaluateEpoch` names nothing in the tree.
**FIX (adopted):** retract to the defensible claim — **P4 drives a codegen CHANGE** (a compile-time
authority READ, S1). The `derivations`/`routed_results` half stays ctest-only-synthetic (P3-style) at
P4; §6 is retitled "RESERVED P6 evaluation semantics, exercised at P4 ONLY via the ctest." `rules`
stays empty; the row loop is the ctest driver, not a compile-time sweep.

**S4 · HIGH · enum contradiction blocked §9.** §3.1 commented out `kFullKeyHashLookup` yet §9 requires
`booleans` rendered under it, and `AccessRequirement` carried no arity to classify all-bound vs
bound+free. **FIX (adopted):** `kFullKeyHashLookup` is a LIVE P4 enumerator (`.Find` IS an honest
full-key hash probe on the canonical dedup table — no `@key`/`bound` conflation); add `has_free` +
`relation_is_recursive` to `AccessRequirement`; branch `SelectAccessPlan` (all-bound →
`kFullKeyHashLookup`, nonrecursive bound+free → `kFullScanFilter`, recursive bound+free →
`kRetainedIndexScan`).

**S5 · HIGH · AccessPlan's home conflicts with the un-superseded P7 authority.** `p7p9-diffs`
§7-pin(4)/§1.1 say "AccessPlan appears ONLY in `.rel`/codegen." **FIX (adopted):** this doc SUPERSEDES
that pin for the QUERY-read arm — AccessPlan is DUAL-HOMED post-P7 (Regional model + `-region-out` for
query reads; a Rel `plan_kind` on interior/join scans at P7). Record it, do not claim "Rel op … or
never."

**S6 · HIGH · B3-3 gated nothing P4-specific** (its sub-assertions were verbatim P3 gates). **FIX
(adopted):** the ctest's P4 case must invoke `SelectAccessPlan` on an `AccessRequirement` built from a
request edge, run the `FullScanFilter` iterator over synthetic rows, THEN assert `routed_results` — so
deleting the §6 wiring breaks it; and assert two DISTINCT non-empty schemas route DISJOINTLY (the
non-empty state is load-bearing; empty-state cannot produce this).

**S7 · MED · `BuildEmptyQueryEntryPointImpl` (Build.cpp:455-477) also calls `GetOrCreateIndex`** and
would keep an `idx.First` seek under a full-scan-filter label. **FIX (adopted):** apply the identical
`plan == kFullScanFilter` index-withhold gate there.

**S8 · MED · §9 predict-then-verify was non-uniform.** `force`'s `idx_24` is query-ONLY → withholding
it ALSO elides the index member + its `proc_8` `.Add` maintenance (a header ripple); a join-SHARED
index survives (cursor-only). **FIX (adopted):** §9 predicts per-carrier (see revised §9). Note: under
S2 the recursive/shared-index carriers keep their seek entirely, so the "shared index survives" case
mostly doesn't arise at P4.

**S9 · MED · B3-4 was a tautology** (the filter operand is unconditionally `param_names[i]`;
`EmitQueryFriends` never references `BindingStateId`). **FIX (adopted):** drop the codegen-boundary
framing of B3-4; assert D1 provenance in the MODEL instead — the terminal `BindingStateSchemaId`
equals the complete bound-col set of the request edge (`completeness == kCompleteRelation`), in the
ctest.

**S10 · MED · the FullScanFilter partial-bound field re-check is currently DEAD code** (no corpus query
is both partial-bound and `via_index=false`; only the all-free scan skeleton is exercised today).
**FIX (adopted):** B3-1 is a NEW test of previously-dead code — MUST compile+run `force`/`average_weight`
at execution (not trust). This is why §2's "already tested" is softened to "the scan skeleton is
exercised; the partial-bound re-check first fires at P4."

**S11 · MED · `AccessRequirement` used raw decl-ordinal ints.** **FIX (partial):** keep decl-ordinals
at P4 (they mirror the existing `col_indices` idiom the codegen already consumes); documented that P5
upgrades `available_bindings` to typed `BoundFieldValue` and retains `required_fields`.

**S12 · LOW · scope the "drives codegen" claim + DOT twin.** **FIX (adopted):** §1 scopes the
codegen-DRIVE claim to the bound+free slice (`force`/`average_weight` are the codegen-discriminating
carriers; `booleans` is the model-half carrier). Add a `plan=` badge to `-region-dot-out` (advisory,
never goldened) mirroring the `declared-key` badge.

### Certifications (attacked, HELD)

- **Q1/§3.1** — the Rel `.rel` IR does not model query reads; P4 adds no Rel op; every `.rel` golden
  byte-stable by construction.
- **§3.2 via_index=false cursor** — CORRECT for DIFFERENTIAL tables (the unconditional `Present(id)`
  gate is in both arms, Database.cpp:1680-1686); a single drain cannot race compaction.
- **Q2 index-withholding** — SOUND: index maintenance is index-list-driven, so a non-created index
  emits zero `.Add`/compaction; the query is the sole reader; no `force`/`average_weight` golden pins
  generated text.
- **Q3 cursor start** — `pos=0` with `id=pos++` over `[0,NumRows())` correctly replaces `idx.First`.
- **all-columns index pruning** — `GetOrCreateIndex` over ALL columns yields an index not enrolled in
  `index_member`, so `booleans` `.h`/`.rel`/`.stdout` are byte-identical whether or not it is created
  (the enum question is label-only, never a miscompile).
- **B3-1 shape discriminator** — a NumRows scan + field re-check + no `idx.First/Next` distinguishes a
  real emission from a no-op stub (necessary, not sufficient for CONSUMPTION — see S1).
- **`.Find` honesty** — a genuine full-key hash probe on the canonical dedup table; labeling all-bound
  `kFullKeyHashLookup` is honest, no `@key`/`bound` conflation.
- **golden confinement** — all-free `#query` decls stay PermanentRoots (empty `col_indices`, no plan,
  no index drop); every structural region/rel/ir/h golden outside `booleans.region.*` is
  byte-identical.
- **`force` carrier** — valid bound+free carrier; `force.stdout` answer-invariant under FullScanFilter
  (`@first` has no row-limiting codegen; ≤1 row/Time key; driver sorts). Latent: `@first` row-limiting
  stays unenforced for any future >1-row/key carrier.
- **fourth-authority separation** — AccessPlan is a distinct authority (not aliased to a fact id /
  `BindingStateId` / logical path); the M3 backend remains the sole fact owner; the RequestEdge forest
  stays acyclic — MADE REAL (not nominal) by S1's freeze-store+codegen-read.

---

## §9. IR desired-states (predict-then-verify; STRUCTURAL pins only)

**`-region-out` (the primary P4 dump change).** Baseline (P3, `booleans`, opt):
```
  request-port  P2  query=user_is_logged_in/1  bound=(UserId)
census: … request-ports=1 …
```
POST-P4 desired (structural pin — exact token an implementer choice, pin what `Format.cpp` emits):
```
  request-port  P2  query=user_is_logged_in/1  bound=(UserId)  plan=full-key-hash-lookup   # all-bound
```
For a bound+free carrier (`force`, `average_weight`): `plan=full-scan-filter`. Census unchanged.

**Generated `datalog.h` (the codegen change — bound+free carriers only).** Baseline `force`:
```cpp
struct get_next_id_bf_cursor { … uint32_t pos; bool next(uint32_t &NextId) {
  while (pos != kNoRow) { const uint32_t id = pos; pos = db.idx_24.Next(id); … } … } };
… return {db, Time, db.idx_24.First({Time})};
```
POST-P4 desired (the `via_index=false` FullScanFilter arm):
```cpp
struct get_next_id_bf_cursor { … uint32_t pos; bool next(uint32_t &NextId) {
  while (pos < db.get_next_id_<n>.NumRows()) { const uint32_t id = pos++;
    const auto row = db.get_next_id_<n>.RowAt(id);
    if (row.<Time-field> != Time) { continue; } … } … } };
… return {db, Time, 0u};   // pos starts at 0, not idx.First
```
`booleans` (all-bound) generated header BYTE-IDENTICAL (`.Find`, §3.2). `force.stdout` /
`average_weight.stdout` / `booleans.stdout` BYTE-IDENTICAL (answers unchanged).

**Per-carrier header prediction (REVISED §8-S8 — verify each at execution):**
- `force` (`idx_24` is query-ONLY): withholding it elides the `idx_24` MEMBER, the `Key24` struct, its
  `proc_8` `.Add` maintenance, AND `idx_24` from every proc signature — a header RIPPLE beyond the
  cursor. Answer-invariant; `force` has only `.stdout` (no header golden) → NO golden moves.
- `average_weight` (`idx_248` query-ONLY, over a DiffTable): same elision + the `via_index=false`
  differential cursor (`Present(id)` gate). `.stdout`/oracle/monotone/behavioral answer-invariant → NO
  golden moves.
- queries over recursively-derived relations (`fibonacci` `fib`, TC `reachable_from`, `disassemble`):
  the QUERY RELATION is a non-recursive projection (`fib(N,Res):fib_impl(N,Res)`), so it is
  `full-scan-filter` like any bound+free query — answer-correct, the recursion's interior `#local`
  (`fib_impl`/`tc`) and its own indexes are untouched (§8-S2). No `kRetainedIndexScan` case arises in
  the corpus.

**`.rel` (Q1):** BYTE-IDENTICAL for every carrier (Rel doesn't model query reads; no P4 op).
`booleans.rel.opt.golden` does NOT move.

**Golden actions at P4 (VERIFIED at execution):**
- The ONLY suite failures were `booleans.region.{opt,nodf,nocf,none}` (IRGOLD-DIVERGE) — the single
  `plan=full-key-hash-lookup` token added to the request-port line. Re-blessed (the sanctioned
  `runall.sh --bless` path; `booleans.stdout` + `booleans.rel.opt` skipped byte-identical).
- `force` / `average_weight` have only answer goldens (`.stdout` + oracle/monotone/behavioral) — all
  BYTE-IDENTICAL (answers invariant; the index elision + full-scan cursor are not golden-pinned).
- ALL other corpus goldens byte-identical (all-free queries stay PermanentRoots; the query-free corpus
  is untouched). `.rel` byte-identical everywhere (Q1).
- The `RegionInstance` ctest gained the P4 gates (GateA selector / GateB FullScanFilter-drives-routing /
  GateC distinct-states-route-disjointly) — a code gate, not a golden; ctest 5/5.

**Verification (DONE):** compiled `booleans`/`force`/`average_weight`/`fibonacci` + a synthetic
directly-recursive `tc` query with `-region-out` + `-cpp-out`; confirmed the header FullScanFilter
shape (bound+free: `while (pos < NumRows())` + `if (row.<f> != <param>)`, `pos=0`, index elided) + the
`plan=` render + all-bound `.Find` byte-identical + `.rel` byte-identical; ran the NEGATIVE WITNESS
(§8-S1 — plan flip → header reverts to `idx.First`); `RegionInstance` ctest 5/5; OptDiff `SUITE: PASS`
after the region re-bless.
