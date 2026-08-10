# P7 grounding seed — physical access planning (POST-P6.2)

> A cold-start seed for the P7 cut. Written after P1–P6.2 landed; grounded in the
> REAL current-code anchors (four sonnet readers + spot verification, 2026-08-10,
> branch `keyed-instances`, tip POST-P6.2). Every file:line here was read at tip;
> clangd in this repo is noise (no include paths) — trust the files.
>
> Companion docs (WRITTEN PRE-P4, now partly stale — see §2):
> `keyed-rewrite-p7p9-diffs.md` (§1 = P7), `keyed-rewrite-p7p9-critique.md`
> (s14 panel + s15 "P8a dissolved" re-critique), `p4-grounding.md` (the substrate
> P7 extends).

---

## §0 STATUS

- **P1–P6.2 LANDED.** The P6 compile-time first cut is complete. Tip: OptDiff
  SUITE PASS 226, ctest 5/5. The M3 full-materialization backend EVALUATES and
  answers correctly; the frozen Regional layer (`lib/Regional`) is a COMPILE-TIME
  OBSERVER layered over it.
- **P7 is the RANKED-#1 next cut: PHYSICAL access planning.** Make the P4-landed
  `AccessPlan` authority (`include/drlojekyll/Regional/RegionInstance.h:242`) a
  real physical-structure domain with an actual hash-seek arm, so a narrow `@key`
  (P5 `DeclaredAccessPath`) + P6.2 routing finally lower a bound `#query` to an
  index SEEK (`Index::First/Next`) instead of the full-scan-filter cursor P4
  emits today.
- **Greenfield structural-gate posture STILL applies.** P7 is codegen-HONEST and
  ANSWER-INVARIANT vs M3: the full-materialization baseline already answers
  correctly, so every P7 gate is STRUCTURAL (a seek plan must NOT emit the
  whole-table rescan; codegen byte-stable for programs with no narrow key), never
  answer-equality. This is the P4 precedent (V-PLAN-HONEST + `-region-out plan=`).
- **P8 (cross-relation ordered TRIE) and P9 (path INFERENCE) are OUT of P7 scope.**
  The runtime is all-hash today (§1.5); no trie/range structure exists. P7 is
  index-SELECTION + honest-plan codegen READS over the existing hash `Index<Key>`
  and the existing compile-time `GetOrCreateIndex(sorted(bound))` provisioner.

Method (per prior phases): grounding loop (pseudocode → hunk-grain diffs → opus
refuter panel → predict IR desired-states) THEN owner-gated execution.

---

## §1 WHOLE-PROGRAM PSEUDOCODE (as-is, grounded)

The compile pipeline (CLAUDE.md header; `bin/drlojekyll/Main.cpp`):

```
parse
  → Query::Build(module, log, policy)          lib/DataFlow      (data-flow IR)
  → FrozenRegionalProgram::Build(query, log)    lib/Regional      (compile-time model; SELECT plan)
  → Program::Build(frozen, first_id, optimize)  lib/ControlFlow   (control-flow IR; READ plan)
  → CodeGen                                     lib/CodeGen/CPlusPlus/Database.cpp
  → runs against                                lib/Runtime + include/drlojekyll/Runtime
```

### 1a. FREEZE — SELECT the plan (`lib/Regional`)

```
// Build() inline body BuildRequestPorts, lib/Regional/Planning.cpp:687-697
for each #query redecl:
  call_site = next_call_site++
  if HasBoundParam(redecl):                                     // Planning.cpp:689
      lease      = next_lease++
      port_index = next_port++
      plan       = ComputeQueryAccessPlan(decl, redecl)         // :691  <-- SELECT
      R.request_ports.push_back(
          RequestPortRecord{port_index, redecl, lease, call_site, plan})  // :692-693
  else:                                                          // :698 all-free
      R.permanent_roots.push_back(PermanentRootRecord{...})      // NO plan

// ComputeQueryAccessPlan, Planning.cpp:248-264 — builds the requirement
ComputeQueryAccessPlan(decl, redecl):
    has_free = false; available_bindings = []
    for p in redecl.Parameters():
        if p.Binding() == kBound: available_bindings.push_back(p.Index())
        else:                     has_free = true
    req = AccessRequirement{ RelationId{decl.Id()}, has_free,
                             available_bindings, kCompleteRelation }
    return SelectAccessPlan(req)                                  // the dispatch

// SelectAccessPlan — THE STUB, RegionInstance.h:270-275
SelectAccessPlan(req):
    if !req.has_free: return kFullKeyHashLookup      // all-bound -> .Find
    return kFullScanFilter                            // any free col -> full scan+filter
    // IGNORES req.relation, req.available_bindings, req.completeness.
    // CANNOT return kRetainedIndexScan. No cost model. No seek arm.  <-- P7 TARGET
```

Stored: `RegionTemplate::request_ports[k].plan`
(`RequestPortRecord.plan`, `Regional.h:142-154`, ctor default `kRetainedIndexScan`
= sentinel). Rendered by `-region-out` as `plan=<AccessPlanText>` via
`AccessPlanText` (`Format.cpp:137-144`: `full-scan-filter` / `full-key-hash-lookup`
/ `retained-index-scan`).

### 1b. CODEGEN — READ the plan (`lib/ControlFlow/Build/Build.cpp`)

```
// BuildQueryEntryPointImpl, Build.cpp:437-467  (primary read + belt)
plan = context.frozen ? context.frozen->PlanFor(decl) : nullopt   // :444-445
withhold_index = (plan == kFullScanFilter)                         // :446  <-- single bool

scanned_index = nullopt
if !withhold_index && !col_indices.empty():                        // :449
    if idx = model->table->GetOrCreateIndex(impl, col_indices):    // :450  provision
        scanned_index = DataIndex(idx)

// V-PLAN-HONEST (one-directional, query-build-sited) :458-464
if plan == kFullScanFilter && scanned_index.has_value():
    fprintf(stderr, "V-PLAN-HONEST: ... kept a scanned index\n"); abort()

impl->queries.emplace_back(query, table, scanned_index, forcer, retract)  // :466
// second read site: BuildEmptyQueryEntryPointImpl, Build.cpp:492-502 (mirror)

// PlanFor, lib/Regional/Planning.cpp:608-620  (key = decl Id + binding-pattern string)
PlanFor(redecl):
    for rp in region.request_ports:
        if rp.query_decl.Id()==redecl.Id() && bindpat(rp.query_decl)==bindpat(redecl):
            return rp.plan
    return nullopt                                                  // all-free query
```

`scanned_index` flows into `ProgramQuery::index`. Codegen reads it in
`EmitQueryFriends` (`Database.cpp:1527-1737`):

```
via_index = spec.index && index_member.contains(spec.index->Id())  // Database.cpp:1646
  all-bound .Find existence   (1623-1640)  <- kFullKeyHashLookup / retained
  via_index cursor            (1668-1673 next / 1724-1730 factory):  <idx>.First / <idx>.Next
  full-scan-filter cursor     (1674-1698 next / 1731-1732 factory):
      while (pos < db.<member>.NumRows()) {
        id = pos++;
        if (row.<field> != <param>) continue;   // 1687-1699  bound-col re-check
      }
```

So today: `kFullScanFilter` ⇒ `scanned_index=nullopt` ⇒ `via_index=false` ⇒ the
full-scan-filter `while (pos < NumRows())` cursor. `kFullKeyHashLookup` / retained
default ⇒ index kept ⇒ `.Find` or `First/Next`. **This is the single observable
codegen effect of the whole authority.**

### 1c. The OTHER scan surface — in-procedure `EmitScan` (NOT touched by P4)

`EmitScan(ProgramTableScanRegion)` (`Database.cpp:2778-2870`, dispatch
`:1847`) RE-DERIVES the arm from index-presence × arity — there is NO stored
plan on the region node:

```
keyed_chain = index && index_member.contains(index.Id())
                     && input_vars.size() == index.KeyColumns().size()   // :2820
keyed_probe = index && !keyed_chain && input_vars.size() == fields.size()// :2823
  keyed_chain -> for (s=idx.First(row); s!=kNoRow; s=idx.Next(s))        // 2825-2830
  keyed_probe -> if (s=member.Find(row); s!=kNoRow)                      // 2831-2835
  else full scan for (s=0; s<member.NumRows(); ++s)                      // 2836-2839
                + bound-col re-check belt if !input_vars.empty()         // 2847-2862
```

**Two `ProgramTableScanRegion` mint sites, plan_kind threaded at NEITHER:**
- `BuildMaybeScanPartial` (`Build.h:395`, mint `Build.h:448`) — index CONDITIONAL:
  `if !in_col_indices.empty() { index = GetOrCreateIndex(...) }`; index=None for a
  zero-bound full scan. Callers `Stratum.cpp:1033/1215/1333`.
- `BuildNestedLoopJoin` (`Join.cpp:181`, mint `Join.cpp:254`) — index ALWAYS Some
  (`pred_table->GetOrCreateIndex(impl, pivot_cols)`, `:263-266`).

`ProgramTableScanRegionImpl` (`lib/ControlFlow/Program.h:1524-1563`) has fields
`table / out_cols / index / in_cols / in_vars / out_vars` — **`index` (a
`UseRef<TABLEINDEX>`) is the ONLY plan signal; there is NO `plan_kind` field.**
Public accessors (`Program.h:1028-1071`): `Table/Index/IndexedColumns/…`, no plan
accessor. The `AccessPlan` enum never reaches the ControlFlow node.

### 1d. Runtime surface (`include/drlojekyll/Runtime/Table.h`, `hyde::rt`)

Physical structures available TODAY (all HASH; no trie/range — that's P8):

| Plan → structure | Runtime call | Anchor |
|---|---|---|
| full-key probe (all-bound `.Find`) | `RowStore::Find(Row) -> id\|kNoRow` | Table.h:80 (whole-row by-value) |
| **keyed SEEK** (the P7 target) | `Index<Key>::First(Key)` + `Next(id)` | Table.h:804 / :821 |
| full scan + filter | `NumRows()` + `RowAt(id)` + predicate | Table.h:71 / :75 |

`Index<Key>` (Table.h:748-888) is a secondary hash index: key → head-insertion
chain threaded through a per-row `next` array (no per-key alloc). `First/Next` is
**FULL-KEY EXACT** (Table.h:791-803, verbatim): every yielded id has key columns ==
the probe key, so generated join code emits NO per-row re-check of scanned key
columns — the probe IS the equality authority. Row liveness is filtered through
the owning table's membership predicates.

**Compile-time provisioner (already exists, already order-free):**
`DataTableImpl::GetOrCreateIndex(impl, std::vector<unsigned> col_indexes)`
(`lib/ControlFlow/Data.cpp:348-381`): `SortAndUnique(col_indexes)` (`:350`) then
returns the shared `TABLEINDEX` whose `column_spec` matches (`:357-362`), else
mints one (`:364`). So `@key(A,B)` and `@key(B,A)` (any surface order) converge to
ONE `TABLEINDEX` — the `[A,B]/[B,A]` convergence P5 needs is FREE. Each
`TABLEINDEX` becomes one emitted `::hyde::rt::Index<Key>` static member on
`Database` (`Database.cpp:1493`, driven `.First/.Next` at `:1729/:2828`).

---

## §2 P4-SUBSTRATE vs s15-DESIGN RECONCILIATION (load-bearing)

The s14–s15 `keyed-rewrite-p7p9-diffs.md` §1 was written BEFORE P4 landed. P4
shipped the **skeleton** the docs designed, but as a **flat 3-arm enum with a
has_free-only selector and a query-build-sited one-directional belt** — NOT the
docs' variant-with-`PhysicalAccessStructure`, caps-driven 4-way dispatch,
`plan_kind`-on-`ProgramTableScanRegion`, and EmitScan per-kind belt. Scope P7 as
"finish what P4 stubbed," not "introduce AccessPlan."

| p7p9-diffs P7 claim | Current status | Anchor / note |
|---|---|---|
| §1.1 "AccessPlan is a NEW typed-id domain, its own authority, NOT the join `Lowering` enum" | **DONE-by-P4** | `enum class AccessPlan : uint8_t` at `RegionInstance.h:242`; render `Format.cpp:137`. Firewall from join `Lowering` holds. |
| §1.1 `AccessPlan` = a VARIANT carrying `PhysicalAccessStructure{ HashArrangement{table,key_columns,exact_full_key}, TriePrefixIndex }` | **STALE anchor / diverged** | Landed is a FLAT `uint8_t` enum. `PhysicalAccessStructure`/`HashArrangement`/`key_columns`/`exact_full_key` grep-CLEAN — entirely unbuilt. |
| §1.1 plan kinds `{kUnplanned, kFullScanFilter, kFullKeyExactProbe, kFullKeyHashLookup, #kTriePrefixWalk}` (~5) | **Diverged (3 landed)** | `kFullScanFilter=0`, `kFullKeyHashLookup=1`, `kRetainedIndexScan=2`. `kFullKeyExactProbe` FOLDED into `kFullKeyHashLookup`; doc's `kUnplanned` sentinel shipped as `kRetainedIndexScan` (a name absent from P7 docs). `kTriePrefixWalk` correctly absent (P8). |
| §1.3 `SelectAccessPlan(req, caps)` real 4-way dispatch "replaces P4 stub `return kFullScanFilter`" | **STILL-TODO; premise STALE** | The actual stub (`RegionInstance.h:270-275`) is a 2-branch `has_free`-only selector, not `return kFullScanFilter`. No `caps` param, no `table`, no `|bound|==Columns().size()` arm, no `HasOrCanMintIndexOn` strict-subset arm, no `HashArrangement`, no bound-col sort. **This is P7's core work.** |
| §1.2 `CodegenPlanCapabilities` SET + D4 invariant (SelectAccessPlan never returns an unemittable kind) | **STILL-TODO** | Grep-CLEAN. No capability object exists; `SelectAccessPlan` takes no caps. D4 is currently implicit (only 2 emittable kinds are ever returned). |
| §1.3 `AccessRequirement{bound_field_positions, body, …}` | **Diverged** | Landed `AccessRequirement{relation, has_free, available_bindings(decl-ordinal), completeness}` (`RegionInstance.h:258-263`). `available_bindings` = doc's `bound_field_positions`; NO `body` field; **`available_bindings` is COMPUTED but DISCARDED by the stub dispatch.** |
| §1.4 `LowerAccessRequirement` mints `ProgramTableScanRegion` with a NEW `scan.plan_kind` field (Program.h:1632), plan_kind ∉ Hash/Equals | **STILL-TODO; P4 took a DIFFERENT path** | No `plan_kind` field on `ProgramTableScanRegionImpl` (grep-clean). P4 did NOT relocate the bound-query read to a region scan — it reads `PlanFor(decl)` at the existing query-entry-point and WITHHOLDS the index on the `<name>_cursor` factory (`Build.cpp:444-466`). |
| §1.5 V-PLAN-HONEST belt MOVED to `EmitScan`, per-kind IMPLICATION set, `kUnplanned` skip (B-P7) | **STILL-TODO; landed belt is elsewhere & one-directional** | Landed at `Build.cpp:458-464` (query-build site), a SINGLE `kFullScanFilter ⇒ no index` check. B-P7's `kUnplanned`-skip over the two legacy mints (`Join.cpp:254`, `Build.h:448`) is currently MOOT (belt never runs over join/interior scans) — it REVIVES the instant P7 moves the belt to EmitScan. |
| §1.6 pin (1): query cursor discriminator `pos` vs `s<id>` | **Scope note** | P4 left the `<name>_cursor` factory `pos`-keyed with the index merely withheld (the s15-LOW `p7-query-path-cursor-shape-scope` precondition came true). P7 must accept pin (1) as interior-only OR relocate per §1.6.1. |
| §0 authority map "P5 seeds `DeclaredAccessPath`; P7 introduces AccessPlan" | **Past tense** | Both P5 (`DeclaredAccessPath`, `RegionInstance.h:288`; set on `RelationSchema.declared_access_paths`, `Regional.h:94`) and AccessPlan (P4) LANDED; P6.2 routing shipped. P7's remit is dispatch + emission-fidelity + physical backing. |
| §2 (P8) trie / §3 (P9) inference | **OUT of P7 scope** | Runtime is all-hash (§1.5). s15 "P8a dissolved": the intra-relation prefix seek IS P7 `kFullKeyHashLookup` over `GetOrCreateIndex(subset)` — no separate P8a. |

**Named gap (the one sentence):** `SelectAccessPlan` is a near-stub returning only
`kFullScanFilter` / `kFullKeyHashLookup` on `has_free` alone; the real hash
partial-key SEEK arm — `GetOrCreateIndex(bound-subset) → Index::First/Next` — is
neither SELECTED at freeze nor EMITTED at codegen, and `plan_kind` is threaded at
NEITHER `ProgramTableScanRegion` mint site (0/2). P7 closes exactly that gap.

---

## §3 PATH FORWARD AS DIFFS (P7 only)

Each hunk is before/after on the §1 pseudocode with a STRUCTURAL exit gate. NO
cost model; the full-scan fallback stays ALWAYS-LEGAL.

### D1 — the seek enumerator (make `kRetainedIndexScan` returnable, or add a name)

```
// RegionInstance.h:242-250  BEFORE: kRetainedIndexScan is RESERVED + sentinel
// AFTER (option A, minimal): reuse kRetainedIndexScan as the real seek arm, and
//   introduce a distinct kUnplanned sentinel so the belt has a skip value that is
//   NEVER a produced plan. (Reconciles the P4 name-overload: today one value means
//   both "keep the seek" AND "no plan selected".)
enum class AccessPlan : uint8_t {
  kUnplanned = 0,          // NEW sentinel/default — NEVER returned by dispatch; belt skips
  kFullScanFilter,         // withhold index; full scan + bound-col filter
  kFullKeyHashLookup,      // all-bound .Find
  kPartialKeyHashSeek,     // NEW (was kRetainedIndexScan): Index::First/Next over
                           //   GetOrCreateIndex(sorted(bound-subset)); strict-subset bound
};
// (option B: keep 3 values, repurpose kRetainedIndexScan AS the seek arm and let
//  the ctor default double as sentinel — but then the belt cannot distinguish
//  "no plan" from "seek". Option A is cleaner; settle in §5-Q2.)
```
Update `AccessPlanText` (`Format.cpp:137-144`) with a token for the seek kind
(e.g. `partial-key-hash-seek`) + the sentinel.
**Gate:** `-region-out` for a program with no narrow key is byte-identical (only
the seek carrier moves).

### D2 — real `SelectAccessPlan` dispatch (the core change)

```
// RegionInstance.h:270-275  BEFORE: has_free-only 2-branch stub
// AFTER:
SelectAccessPlan(req, caps):                          // caps = CodegenPlanCapabilities (D3)
    if !req.has_free:
        return kFullKeyHashLookup                      // all-bound -> .Find (unchanged)
    // bound+free: try a partial-key seek keyed on the bound subset.
    if kPartialKeyHashSeek in caps
       && !req.available_bindings.empty()
       && HasOrCanMintHashIndexOn(req.relation, req.available_bindings):
        return kPartialKeyHashSeek
    return kFullScanFilter                             // ALWAYS-LEGAL honest fallback
```
- `req.available_bindings` (computed today, discarded today) becomes load-bearing.
- The seek key = the **raw bound subset** of the query's own read relation
  (`available_bindings`), sorted. The P5 `DeclaredAccessPath` is a HINT that this
  subset is intended (see §5-Q1) — physically the index is provisioned on the
  bound cols regardless, and `GetOrCreateIndex`'s `SortAndUnique` makes it
  order-free. NO cost model: the seek is preferred whenever an index exists/can be
  minted; fallback otherwise.
- `HasOrCanMintHashIndexOn` is answerable at freeze from the relation's model
  table (always mintable via `GetOrCreateIndex` at codegen — this predicate can be
  a constant-true given a materialized table, or gated on "not recursive-owned" if
  a fence is wanted; see §5-Q3).
**Gate:** `SelectAccessPlan` NEVER returns a kind ∉ `caps` (D4). For programs whose
queries are all-bound or all-free, the returned plan set is unchanged → codegen
byte-stable.

### D3 — `CodegenPlanCapabilities` (the emit-arms-that-exist SET; D4 invariant)

```
// NEW compile-time constant (RegionInstance.h or a Regional header):
CodegenPlanCapabilities = { kFullScanFilter, kFullKeyHashLookup, kPartialKeyHashSeek }
// # kTriePrefixWalk EXCLUDED until P8.
// D4 INVARIANT: each kind maps to EXACTLY ONE emitted cursor/scan shape; the set
//   literally enumerates the arms EmitScan/EmitQueryFriends ACTUALLY emit;
//   SelectAccessPlan may NEVER return a kind outside it.
```
May stay IMPLICIT (a `static_assert`/comment) rather than a threaded object — §5-Q4.
**Gate:** compile-time; no runtime effect.

### D4 — thread `plan_kind` at EVERY `ProgramTableScanRegion` mint (B-P7)

```
// Program.h:1524 ProgramTableScanRegionImpl  ADD:
AccessPlan plan_kind{AccessPlan::kUnplanned};   // EXCLUDED from Hash/Equals/MergeEqual
                                                //   (mint_tag S5' precedent)
// Program.h:1028 public accessor:  AccessPlan PlanKind() const;

// Mint site 1 — Build.h:448 (BuildMaybeScanPartial): set plan_kind from the
//   requirement it lowers (kPartialKeyHashSeek when index minted on a bound subset,
//   else kFullScanFilter; kUnplanned only if it is a non-query interior scan P7
//   does not classify yet).
// Mint site 2 — Join.cpp:254 (BuildNestedLoopJoin pivot): plan_kind = kUnplanned
//   (a join pivot is not a #query access; the belt SKIPS it). This is the exact
//   B-P7 correction — with a kFullScanFilter default the moved belt would abort on
//   every join.
```
**Gate:** both mints set plan_kind (2/2). `plan_kind` ∉ Hash/Equals → CSE/dedup and
emitted bytes unchanged for existing programs.

### D5 — emit the seek on the query path (the honest codegen READ)

```
// Build.cpp:444-466 BuildQueryEntryPointImpl  BEFORE: withhold_index = (plan==kFullScanFilter)
// AFTER: three-way on plan
scanned_index = nullopt
switch plan:
  kFullScanFilter:     scanned_index = nullopt                      // withhold (unchanged)
  kFullKeyHashLookup:  /* all-bound .Find path — no index needed */ // (unchanged)
  kPartialKeyHashSeek:
      cols = sorted(bound-subset col indices)                       // == available_bindings
      scanned_index = DataIndex(model->table->GetOrCreateIndex(impl, cols))  // NARROW index
// EmitQueryFriends already turns a Some index into the via_index First/Next cursor
//   (Database.cpp:1668-1673 / 1724-1730) — NO new codegen; the seek reuses that arm.
```
So the P7 seek is: SELECT `kPartialKeyHashSeek` → provision `GetOrCreateIndex(bound
subset)` → `via_index=true` → existing `<idx>.First/.Next` cursor. **New codegen
surface: NONE** — P7 is select + honest-read over arms codegen already has.
**Gate (STRUCTURAL):** the seek carrier's cursor emits `<idx>.First(...)`/`.Next(id)`
and NOT the `while (pos < db.<member>.NumRows())` rescan; answer-invariant vs M3
(the index yields exactly the bound-key rows the full scan would filter to —
`Index::First/Next` is FULL-KEY EXACT, Table.h:791). Programs with no narrow key:
byte-identical.

### D6 — V-PLAN-HONEST: per-kind implication belt at the EMISSION site (B-P7 revives)

```
// Move/duplicate the belt from Build.cpp:458 to EmitScan head (Database.cpp:2818)
//   AND keep a query-path belt in EmitQueryFriends. Guard on kUnplanned skip:
if region.plan_kind != kUnplanned:                     // B-P7 skip for legacy/join mints
    assert kFullScanFilter    ==> full-scan-filter arm chosen (index withheld)
    assert kFullKeyHashLookup ==> .Find/keyed_probe arm
    assert kPartialKeyHashSeek==> keyed_chain (First/Next over a Some index)
// NOT an Index()==nullopt biconditional — a per-kind IMPLICATION set (§1.5 Option-2).
```
**Gate:** a `kPartialKeyHashSeek` region that emitted a full scan ABORTS at compile;
a `kFullScanFilter` region that kept an index ABORTS (the P4 belt, generalized). No
join/interior scan (kUnplanned) trips it.

**OUT of P7 scope:** P8 (ordered TRIE / cross-relation prefix sharing / range
scans — no runtime structure exists), P9 (path INFERENCE / `SortedPredecessors`
DataFlow-signal order). Do not add a runtime trie, an ordered index, or an
inference pass in P7.

---

## §4 IR DESIRED-STATES SKETCH (predict-then-verify)

**Carrier: `key_partial_1`** — already exists, the natural P7 seek witness:
```
#message edge_2(u64 From, u64 To).
#local path(u64 From, u64 To) @key(From).
path(F, T) : edge_2(F, T).
#query reachable_from(bound u64 From, free u64 To) : path(From, To).
```
`.irgold` steps: `region {opt,nodf,nocf,none}`. Answer is invariant across all 4
modes (full-materialization answers a complete read); `.stdout` unchanged.

**TODAY (`key_partial_1.region.opt.golden:9`):**
```
request-port  P1  query=reachable_from/2  bound=(From)  plan=full-scan-filter
```
plus `declared-key E1 rel=path path=(From)` (P5 render; `@key` is on `path`, the
query reads `reachable_from` which routes From←path.From via P6.2 `rule`).

**P7 DESIRED `-region-out`:**
```
request-port  P1  query=reachable_from/2  bound=(From)  plan=partial-key-hash-seek
```
(one token flips; the `declared-key`, `rule`, `shared-field`, census lines stay.)

**P7 DESIRED generated cursor** (`EmitQueryFriends`, `reachable_from_bf` factory):
- BEFORE: `while (pos < db.reachable_from.NumRows()) { id = pos++;
  if (row.From != From_) continue; ... }`
- AFTER: `for (uint32_t id = db.<idx_reachable_from_From>.First(Key{From_});
  id != kNoRow; id = db.<idx_reachable_from_From>.Next(id)) { ... }`
  — `via_index=true` over `GetOrCreateIndex(reachable_from_table, {From})`.

**Goldens that MOVE (the FIRST codegen move since P4):**
- `key_partial_1.region.{opt,nodf,nocf,none}.golden` — the `plan=` token.
- `key_partial_1` generated `.h`/`.cpp` (if goldened via `.irgold` h/ir steps) —
  the new `Index<>` member + `First/Next` cursor. (Check whether key_partial_1 has
  ir/h steps; today its `.irgold` is region-only, so a new codegen golden may need
  adding — decide at execution.)
- `key_partial_1.stdout` STAYS byte-identical (answer-invariant).

**Goldens that STAY byte-identical:** every non-seek program. Any query that is
all-bound (`.Find`) or all-free (no request port) is unchanged. The full corpus
minus narrow-key seek carriers is byte-stable — the P4 posture.

Predict these THEN build THEN verify the real dumps match (standing methodology).

---

## §5 OPEN QUESTIONS (grounding loop must settle)

1. **Seek key provenance — P5 `DeclaredAccessPath` vs raw bound-subset?**
   In `key_partial_1` the `@key(From)` is on `path` but the request port is for
   `reachable_from` (bound From, routed From←path.From via P6.2). Physically the
   query scans `reachable_from`, so the seek index is on `reachable_from.{From}` =
   the raw bound subset — the `@key` is a HINT that this binding is intended, not
   the literal seek key. DECIDE: does P7 (a) select on the raw bound subset
   unconditionally (simplest; `@key` merely witnesses intent), or (b) gate the seek
   on the bound subset matching a routed `DeclaredAccessPath` (narrower; ties the
   physical choice to the logical hint)? The firewall (`Regional.h:92-93`: declared
   paths "NEVER fed to the physical AccessPlan") is what P7 DELIBERATELY bridges —
   settle how much.

2. **`kRetainedIndexScan` reconciliation.** Its comment says BOTH "RESERVED (P7)
   cost-based keep-the-seek" AND "default/sentinel for a record with no plan"
   (`RegionInstance.h:247-249`). Option A (§3-D1): split into `kUnplanned`
   (sentinel) + `kPartialKeyHashSeek` (real seek). Option B: overload one value.
   The B-P7 belt NEEDS a skip value distinct from any produced plan → lean Option
   A. Confirm.

3. **Is a fence needed on the seek?** `GetOrCreateIndex` provisions over ANY
   materialized table incl. a recursive-component-owned one (P6.1
   `recursive_components`, `Regional.h:209`). A complete read of a settled
   recursive relation via a narrow index is answer-correct (the read is acyclic
   over the settled table; the recursion's own indexes are untouched — the exact
   P4 `kFullScanFilter` argument, `RegionInstance.h:266-267`). So the seek is
   likely UNFENCED. Confirm no interaction with the fixpoint indexes; if any doubt,
   `HasOrCanMintHashIndexOn` gates on `!recursive_owned` and falls back.

4. **`CodegenPlanCapabilities` — object or implicit?** Build the threaded SET (doc
   §1.2, caps param on `SelectAccessPlan`) or keep D4 as a `static_assert` +
   comment enumerating the two/three emittable arms? The stub takes no caps today;
   a threaded object is more faithful to s15 but heavier. Decide at pseudocode.

5. **Where does V-PLAN-HONEST live?** Keep the P4 query-build belt (`Build.cpp:458`,
   generalized to the 3-way) AND add the EmitScan per-kind belt (B-P7, with the
   `kUnplanned` skip), or unify at emission? The `RegionInstance.h:240` comment
   ("dual-homes a Rel plan_kind for interior/join scans") anticipates two homes.
   The moved belt is what REVIVES B-P7 over the two legacy mints — sequence D4
   (thread plan_kind) BEFORE D6 (move belt) or the corpus aborts.

6. **Does P7 need a new Rel op, or is it purely Regional-select + codegen-read?**
   Grounding says PURELY Regional-select + codegen-read: the seek reuses the
   existing `via_index` First/Next arm (`Database.cpp:1668`) and the existing
   `GetOrCreateIndex` provisioner (`Data.cpp:348`). No new Rel op, no new runtime
   structure. Confirm the interior/join scan path (`EmitScan`) does not ALSO need a
   plan-driven rewrite in P7, or scope it to the `#query` path only (interiors stay
   index-presence-derived, refereed by the kUnplanned skip). Likely scope P7 to the
   bound-`#query` path; interiors are a follow-on.

7. **Golden surface for the codegen move.** `key_partial_1.irgold` is region-only
   today. Adding the FIRST codegen golden move since P4 may need ir/h steps on the
   carrier (or a new carrier with them). Decide whether to extend `key_partial_1`'s
   `.irgold` or add a sibling.
