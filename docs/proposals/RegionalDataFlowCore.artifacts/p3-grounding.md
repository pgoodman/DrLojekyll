# P3 grounding — RequestEdge / RuleActivationEdge / FactDerivation / RoutedResult (acyclic slice)

> **P3 LANDED (session 18, owner green-lit).** The acyclic slice is implemented as a COMPILE-TIME
> typed model layered over the retained backend (M3 — codegen UNCHANGED). What shipped:
> - **`include/drlojekyll/Regional/RegionInstance.h`** (new, public leaf) — hosts ALL regional typed-id
>   domains (the Stage-B skeleton ids MOVED here from `Regional.h` + the P3 residual/ownership/support
>   ids) + `RegionInstanceRelations` with inline ops (`AddRequestEdge`/`RemoveRequestEdge`/
>   `AddDerivation`/`RouteResults`/`RootedReachability`). `Regional.h` includes it and stores a
>   `RegionInstanceRelations` BY VALUE on `FrozenRegionalProgram` (no rule-of-five; the moved-ids
>   layering avoids the pointer approach).
> - **`lib/Regional/Planning.cpp` `BuildRequestPorts`** — the query loop now SPLITS by binding: a bound
>   `#query` → `RootLease` request port (`RouteKind::kRequestPort`, numbered AFTER input/result so
>   all-free programs stay byte-identical); an all-free `#query` → `PermanentRoot`. BOTH mint a
>   `RequestEdge` (B2). `census.request_ports` re-derived via `CountBoundQueryRedecls` in lockstep with
>   the check_count belt. `@first` bound queries (`force.dr`) correctly become request ports (F5).
> - **`lib/Regional/Format.cpp`** — renders `-> request-port P<k>` + the `request-port P<k>
>   query=<name>/<arity> bound=(<keys>)` body line (gated on non-empty `request_ports`).
> - **`tests/RegionInstance/`** (new ctest `RegionInstance`) — the F4a DISCRIMINATING gate: a
>   compile-time id-disjointness battery + runtime gates 1-6 + L5 + acyclic-forest, driving the edge
>   ops with synthetic rows (a stub that skips derivations/routing FAILS them).
> - **`tests/OptDiff/goldens/booleans.region.{opt,nodf,nocf,none}.golden`** (new, F4b) — the committed
>   bound-query region golden pinning `request-ports=1` + the `-> request-port` split.
> - **Design decisions folded from the critique:** `RootAlive(RootLease)≡true` (F2); region rooted by
>   the ProgramRoot via `RootedReachability`'s implicit empty-state seed, independent of `#query` (F1);
>   `FactDerivation` carries `contributing_states` (a set) so routing never keys on a single `src_state`
>   (F3); support is a plain `int64_t` at P3 (P6 upgrades to the split `DerivationSupportCount`); the
>   reserved P6 owner arm is a `RegionalMember` newtype (F8).
> - **Gate GREEN:** OptDiff `SUITE: PASS`; ctest 5/5 (incl. `RegionInstance`); the 3 pre-existing
>   all-free `.region` goldens byte-identical; all `.rel`/`.h`/`.stdout` byte-identical (codegen
>   untouched). §1-§9 below are the design record the implementation follows.

Session 18 grounding output (the design record P3 was built from). Branch `keyed-instances`,
code byte-current at `ae207c36` (tip `244be564` is a docs handoff). This is the distilled,
code-verified P3 execution-readiness record: the layer-site inventory, the M3 resolution, the
typed-id + edge-op pseudocode, the drift-correction table against `reconstruction-diffs.md`
§3-P3/§5.4, and the resolved open questions. It SUPERSEDES the stale anchors in
`reconstruction-diffs.md` §3-P3 (see the drift table §5); those anchors predate the P1 cut and P2.

Method note: produced by a four-reader opus fan-out (layer-sites / typed-ids / edge-ops /
evaluate-epoch), each grounding every anchor in the real post-P2 code, then orchestrator-verified
on the two blocking claims (the vestigial injector seam and the L5 relation-id resolution).

---

## §1. THE HEADLINE — what P3 actually is (the M3 resolution)

**At P3, `EvaluateEpoch` / `RootedReachability` / `AddDerivation` are a COMPILE-TIME TYPED MODEL
+ its own consistency validator — NOT a runtime engine.** Codegen is UNCHANGED at P3; P3 moves no
golden byte and adds no runtime branch. The compiled program's behavior is delivered entirely by
the RETAINED full-materialization backend (M3):

- bound/free query cursors: `BuildQueryEntryPoint` (Build.cpp:504) → `BuildQueryEntryPointImpl`
  (Build.cpp:413), `BuildEmptyQueryEntryPoint` (Build.cpp:481);
- publish: `BuildIOProcedure` (Procedure.cpp:397) → `PublishDifferentialMessageVectors`;
- recursion: the landed OVERDELETE→REDERIVE→INSERT fixpoint, `LowerDRRounds` (Stratum.cpp:1799),
  round shells `LowerRoundBody` (Stratum.cpp:1691, `impl->induction_regions.Create`).

What P3 *adds* is a TYPED MODEL — the `RegionInstanceRelations` struct (request edges, activation
edges, per-fact derivations, routed results) — populated at freeze time beside the immutable
`RegionTemplate`, on `FrozenRegionalProgram`. `EvaluateEpoch` is the abstract semantics that
CONSTRUCTS and self-VALIDATES this model once, statically. The structural exit gates
(owner-count / routed-result-vs-derivation / member-key intern / acyclicity abort) observe the
model via `-region-out` + compile-time asserts, **never answer-equality**.

Why this collapses to a compile-time constant at P3: every region is rooted by the always-present
ProgramRoot (`RootAlive≡true`, §8-F1), and RootLease/PermanentRoot query owners are also `RootAlive`
at P3 (the M3 backend materializes unconditionally, §8-F2), so `live == {EmptyBindingState(R)}` for
every program — with or without a `#query`. There is nothing runtime-variable to emit. Runtime
emission of the tracking is a P4+ concern (when terminal `BindingStateId` values and real
`AccessPlan`s exist).

**Consequence for the diff:** at P3 `AddDerivation` is NOT invoked per runtime row. The rules
authority is empty (`RegionTemplate.rules` is RESERVED-EMPTY, P6.2 is its sole populator), so
there is NO rule DAG to sweep. `AddDerivation` / `FactDerivation` are DEFINED (the P4+ spec) and
exercised at P3 ONLY by the F29 structural member-key intern gate — a directed check that two rows
agreeing on member-key columns intern to one `RegionalFactId`. This dissolves the seed's
`topo_order(rules_of(region))` gap: there is no backing rule structure yet, and P3 does not need one.

---

## §2. Layer-site inventory (the P3 analog of the P1/P2 symbol grep)

Every current site P3 must root, layer over, read, or leave untouched. All anchors verified at
`ae207c36`.

| # | Site | file:line | symbol | P3 role |
|---|------|-----------|--------|---------|
| 1 | Query-entry driver | Build.cpp:1378-1385 | `query.Inserts()` → `BuildQueryEntryPoint` for `IsQuery` relation-inserts | **layer-over** (retained backend) |
| 1 | Query entry point | Build.cpp:504 / :413 | `BuildQueryEntryPoint` / `...Impl` | **unchanged** |
| 1 | Bound-col test | Build.cpp:422-426 | `for param : decl.Parameters(); if Binding()==kBound: col_indices.push(param.Index())` | **read** (BuildRequestPorts replicates this test) |
| 1 | Empty-query arm | Build.cpp:1391-1403 / :481 / :455 | `BuildEmptyQueryEntryPoint` / `...Impl` | **unchanged** |
| 2 | Census hardcode | Planning.cpp:228 | `census.request_ports = 0u` | **root** (P3 re-derives = #bound-query redecls) |
| 2 | Port derivation | Planning.cpp:259-285 | input/result `PortRecord` push + `check_count` recount | **root** (add request ports) |
| 2 | **Query→permanent-root loop** | Planning.cpp:301-320 | EVERY dedup'd redecl → `RouteKind::kPermanentRoot` + `permanent_roots.push` | **root** (THE drift; split by binding + B2) |
| 2 | Census authority | Planning.cpp:225 | `DeriveRegionalCensus` (single authority; must move in lockstep) | **root** |
| 3 | Publish path | Procedure.cpp:397 / :749-752 | `BuildIOProcedure` / `PublishDifferentialMessageVectors` | **unchanged** (RoutedResult tracks over it, L5/M3) |
| 4 | Induction backend | Stratum.cpp:1799 / :1691 | `LowerDRRounds` / `LowerRoundBody` | **unchanged** (recursive baseline probe pins byte-identical) |
| 5 | Frozen→Program seam | Build.cpp:1206-1209 / :1324 | `query = frozen.DataFlowGraph()` / `context.frozen_census = &frozen.Census()` | **read** (P3 extends this frozen-accessor seam; NO new codegen pass) |
| 5 | H3 member-key mask | Regional.h:120 | `RelationSchema.member_key_positions` (size==decl.Arity(), positional bool mask) | **read** (fact identity projects through it) |
| 5 | Reserved fields | Regional.h:163-164, 178-179 | `RuleRoutingProjection` / `RecursiveComponent`, `rules`/`recursive_components` | **layer-over** (stay RESERVED-EMPTY; P3 model is a peer struct) |

**Where the tracking attaches (Site 5):** `RegionInstanceRelations` is a FROZEN-side side-structure,
a peer of `RegionTemplate` stored on `FrozenRegionalProgram`, built in `Planning.cpp`'s `Build`
(the new `BuildRequestPorts`) at freeze, consumed structurally at the freeze-time validators. P3
extends the SAME frozen-accessor seam Program::Build already uses for `Census()`/`DataFlowGraph()`;
it does NOT add a new Program::Build codegen pass.

**B2 realizability confirmed (reframed by §8-F1):** the request-port/permanent-root split is ABI +
ownership metadata; codegen entry points (Site 1) are untouched → answers are byte-identical. Region
liveness is rooted by the always-present ProgramRoot, NOT by a `#query` existing (§8-F1) — a
no-`#query` publisher (`product_diff.dr`) must still be live. Query request edges (RootLease for
bound, PermanentRoot for all-free) are ADDITIONAL observation roots, never the sole liveness source;
that keeps "no query justifies a relation" intact.

---

## §3. Typed ids — `lib/Regional/RegionInstance.h` (P3-live vs reserved)

New file, sibling of `Regional.h`'s Identity.h idiom (`struct X { uintN v; ==,<=> defaulted; }`,
one disjoint DOMAIN each, `std::variant` with monostate-first where a case is decl-less). Depends
on `Regional.h` for `RegionId`/`RelationId`. Every dense id is minted by declaration/range walks
(HP-9) — never pointer order, never `UniqueId`.

```cpp
// ---- Dense id domains
struct RootLeaseId        { uint32_t v; ==/<=>; };  // P3 LIVE: one per bound #query redecl adornment
struct PermanentRootId    { uint32_t v; ==/<=>; };  // P3 LIVE: one per all-free #query redecl (B2 owns a req edge)
struct RegionInstanceId   { uint32_t v; ==/<=>; };  // P3 LIVE-SINGLETON: only RegionInstanceId(0) at Stage B
struct BindingStateSchemaId { uint32_t v; ==/<=>; };// P3 LIVE-but-EMPTY-ONLY: only the empty schema interned
struct CallSiteId         { uint32_t v; ==/<=>; };  // P3 LIVE: interns a #query redecl call site (Id + binding pattern)
struct DerivationId       { uint32_t v; ==/<=>; };  // P3 LIVE: dense per FactDerivation
struct RuleId             { uint32_t v; ==/<=>; };  // RESERVED: RegionTemplate.rules empty at P3 (P6.2 populates)

// ---- Value / member substrate (H3: PUBLIC projected values, NEVER lib/DataFlow FieldId/SemanticMemberKey)
using RegionalCellValue      = uint64_t;                       // OPEN Q §6: confirm public cell repr
using SortedBoundFieldValues = std::vector<RegionalCellValue>; // P3: ALWAYS EMPTY (the empty state)
struct SemanticMemberIdentity {                                // one entry per SET bit of member_key_positions
  std::vector<RegionalCellValue> projected_values;  ==/<=>;    // position-order; the ONE F29≡F13 projection
};

// ---- Composite (keyed) ids
struct BindingStateId  { RegionInstanceId ri; BindingStateSchemaId schema; SortedBoundFieldValues vals; ==/<=>; };
inline BindingStateId EmptyBindingState(RegionInstanceId ri) {   // the SOLE P3 constructor
  return { ri, BindingStateSchemaId{0u}, /*vals*/{} };
}
struct RegionalFactId  { RegionInstanceId ri; RelationId relation; SemanticMemberIdentity member; ==/<=>; };

// ---- Ownership edges (RequestEdge = exact ownership, ACYCLIC forest — NEVER unified with activation)
using RequestOwnerId = std::variant<RootLeaseId, PermanentRootId,
                                    RegionalFactId /* RegionalMember, RESERVED P6+ */>;
struct RequestEdgeId        { RequestOwnerId owner; CallSiteId call_site; BindingStateId dest; ==/<=>; };
struct RuleActivationEdgeId { BindingStateId src_state; RegionalFactId src_fact; RuleId rule; BindingStateId dst; ==/<=>; };
                            // RESERVED P4/P5: DeriveActivationEdge mints NONE at P3 (no intra-state self-loop; F18)

// ---- Support authority (FactDerivation keyed BY RegionalFactId — NEVER a 2nd fact owner; B4)
struct FactDerivation  { DerivationId id; BindingStateId src_state; RegionalFactId fact; DerivationSupportCount support; };
struct RoutedResultId  { RequestEdgeId request_edge; RegionalFactId fact; ==/<=>; };

// ---- The container (a freeze-side peer of RegionTemplate)
struct RegionInstanceRelations {
  /*interned set*/          request_edges;      // dedup by tuple identity
  /*RESERVED-EMPTY at P3*/  activation_edges;
  std::map<RegionalFactId, FactDerivation> derivations;   // B4: PER-FACT support, keyed on RegionalFactId
  std::set<RoutedResultId>  routed_results;
  std::map<RequestOwnerId, RelationId> requested_relation_of_owner;  // L5 filter source
  std::set<PermanentRootId> permanent_roots;              // RootAlive(pr) ≡ true, always
  // Singleton instance descriptor: RegionInstanceId(0) => { RegionId(0), inherited_bindings={} }.
};
```

P3-live: `RootLeaseId`, `PermanentRootId`, `RegionInstanceId` (singleton=0), `CallSiteId`,
`DerivationId`, `BindingStateId` (empty-only), `RegionalFactId`, `RequestEdgeId`, `FactDerivation`,
`RoutedResultId`. RESERVED: `RuleId`, `RuleActivationEdgeId`, the `RegionalFactId` arm of
`RequestOwnerId` (P6 subgraph-member ownership), non-empty `BindingStateId::vals` (P4).

**H3 discharge:** `SemanticMemberIdentity` stores `projected_values` (public cells), the positional
mask stays on `RelationSchema.member_key_positions` keyed by `RelationId` (a sibling field of
`RegionalFactId`). ONE positional projection, ZERO second value-id bridge; `FieldId`/
`SemanticMemberKey` never leak out of lib/DataFlow. `DerivationSupportCount` is reused from
`lib/DataFlow/Identity.h`, keeping "a count stands in for an owner" a TYPE error.

---

## §4. Edge + support operations (P3 acyclic slice)

```
BuildRequestPorts(query, R /*RegionTemplate&*/, RR /*RegionInstanceRelations&*/):
    # RE-PROVIDES Planning.cpp:301-320's query loop, TYPED, split by binding.
    ri = RegionInstanceId{0}
    for parsed_query in dedup-by-Id(ParsedModuleIterator sub-module walk):   # Planning.cpp:302-307
        decl = ParsedDeclaration(parsed_query)
        rel  = RelationId{ decl.Id() }                     # L5: query decl IS the relation decl (verified §6)
        for redecl in decl.UniqueRedeclarations():         # dedup by BindingPattern (Build.cpp:508-517 idiom)
            cs = CallSiteId{ next_callsite++ }
            has_bound = any(p.Binding()==kBound for p in redecl.Parameters())   # Build.cpp:422-426 idiom
            if has_bound:                                  # bound -> RootLease request-PORT
                lease = RootLeaseId{ next_lease++ }
                AddRequestEdge(RR, RootLease(lease), cs, EmptyBindingState(ri))
                RR.requested_relation_of_owner[RootLease(lease)] = rel
                R.ports.push(PortRecord{kRequest, next_port++, msg_of(redecl)})   # AbiRecord.route = kToPortP
            else:                                          # all-free -> PermanentRoot (B2: STILL a request edge)
                pr = PermanentRootId{ next_perm++ }
                RR.permanent_roots.insert(pr)              # RootAlive(pr) ≡ true
                AddRequestEdge(RR, PermanentRoot(pr), cs, EmptyBindingState(ri))
                RR.requested_relation_of_owner[PermanentRoot(pr)] = rel
                R.permanent_roots.push(PermanentRootRecord{redecl})              # AbiRecord.route = kPermanentRoot
    # census.request_ports re-derived = count of bound-query redecls (was hardcoded 0 @ Planning.cpp:228);
    # DeriveRegionalCensus + check_count('request-ports') @ :371 move in LOCKSTEP.

AddRequestEdge(RR, owner, cs, dest) -> RequestEdgeId:      # intern; a 2nd owner = DISTINCT edge, NO re-derive
    e = RequestEdgeId{owner, cs, dest}; intern(RR.request_edges, e); return e
    # touches NO derivations map — ownership ⟂ support.

RemoveRequestEdge(RR, e):                                  # caller-qualified
    RR.request_edges.erase(e); RR.routed_results.erase_if(rr -> rr.request_edge == e)
    # derivations UNTOUCHED — the fact stays present iff some other edge still routes it.

AddDerivation(RR, src_state, relation_schema, row, delta) -> RegionalFactId:   # H3 + B4; P4+ spec (§1)
    member  = ProjectRow(row, relation_schema.member_key_positions)   # positional mask, NOT raw FieldIds
    fact_id = RegionalFactId{src_state.ri, relation_schema.id, member}
    d = RR.derivations.get_or_create(fact_id, {DerivationId{next++}, src_state, fact_id, 0})
    prev = d.support; d.support += (delta==kAdd ? +1 : -1)
    if prev==0 && d.support>0: publish_born(fact_id)       # 0 ->> 0
    if prev>0  && d.support==0: publish_dies(fact_id)      # >0 -> 0
    return fact_id
    # B4: ONE support counter per RegionalFactId. NOT a C_nr/C_r mirror indexed by BindingState.

ProjectRow(row, member_key_positions):                    # size(mask)==decl.Arity(), decl-ordinal
    return [ row[i] for i in range(len(mask)) if mask[i] ]  # AllFields (Tier-2 origin) => full tuple

DeriveActivationEdge(RR, src_state, src_fact, rule, dst) -> optional<RuleActivationEdgeId>:
    if src_state == dst: return NONE                       # F18 fix (a): no intra-state self-loop
    assert !ActivationReachable(dst, src_state)            # acyclicity guard, live only post-P4/P5
    return intern(RR.activation_edges, {src_state, src_fact, rule, dst})
    # At P3 returns NONE for every call (single empty state) -> activation_edges stays EMPTY.

RouteResults(RR, st):                                     # L5: filter by the request edge's requested relation
    for e in RR.request_edges where e.dest == st:
        want = RR.requested_relation_of_owner[e.owner]
        for (fid, d) in RR.derivations where d.src_state==st && d.support>0:
            if fid.relation != want: continue             # else a #query on p receives edge/q facts
            RR.routed_results.insert(RoutedResultId{e, fid})
```

```
RootedReachability(request_edges, activation_edges):      # acyclic, ONE topo sweep, NO fixpoint
    # BLOCKING FIX (critique §8-F1): the region is rooted by the ALWAYS-PRESENT ProgramRoot,
    # NOT by a #query existing. A no-#query publisher (product_diff.dr) must still be live.
    live = { EmptyBindingState(R0) }                      # ProgramRoot seed; RootAlive(ProgramRoot)≡true
    live |= { e.dest : e in request_edges, RootAlive(e.owner) }   # queries ADD observation roots
                                                          # RootAlive(RootLease)≡true at P3 (M3 materializes
                                                          # unconditionally); RootAlive(PermanentRoot)≡true
    for src in worklist(live):                            # forward pass; activation_edges EMPTY at P3 => no-op
        for a in activation_edges.from(src):
            if a.dst not in live: live.insert(a.dst); worklist.push(a.dst)
    return live                                           # == {EmptyBindingState(R)} for every rooted program

EvaluateEpoch(input_deltas, request_deltas):             # COMPILE-TIME model construction + self-validate
    old = SnapshotCommittedOutputs()                      # model snapshot (NOT runtime state)
    ApplyInputDeltas(input_deltas)                        # symbolic/empty at P3
    for rd in request_deltas: Add/RemoveRequestEdge(rd)   # seed from BuildRequestPorts
    live = RootedReachability(request_edges, activation_edges)   # one topo sweep
    for st in {EmptyBindingState(R)} ∩ live:
        # P3: NO rule sweep (RegionTemplate.rules empty; AddDerivation is P4+ spec, §1).
        RouteResults(RR, st)
    RetractRoutedResults(all_states \ live)               # erases routed_results entries ONLY
    Publish(Difference(old, Current())); Seal()           # model bookkeeping; NO runtime publish call
```

`RetractRoutedResults` mutates only the `routed_results` set — never `request_edges` — so the
RequestEdge forest stays acyclic vacuously. At P3 `dest` is always `EmptyBindingState` (which owns
no outgoing request edge), so the forest is depth-1 by construction.

---

## §5. Drift-correction table (vs `reconstruction-diffs.md` §3-P3 / §5.4)

All four readers independently flagged these; verified against the current tip.

| Seed/diff anchor | Status | Correction |
|---|---|---|
| §3-P3:365 "RE-PROVIDES Planning.cpp:512-562's forcing loop" | **WRONG** — Planning.cpp is 406 lines | Re-anchor to the query/permanent-root loop at **Planning.cpp:301-320** |
| §3-P3:368 "Demand.cpp:459-471's [kBound] test" | **DELETED** at P1 (whole file gone) | Bound test idiom now inline at **Build.cpp:422-426** (`param.Binding()==kBound`, `param.Index()`) |
| §5.4 H3:735-736 "declared_key_positions (the F13 bridge)" | **RENAMED** | The landed field is `RelationSchema.member_key_positions` (**Regional.h:120**) |
| §3-P3:357/384 `member = SemanticMemberIdentity{member_key, ProjectRow(row, member_key)}` | **PRE-P2** | `member_key`/`SemanticMemberKey` is lib/DataFlow-PRIVATE; member is the **projected value tuple** via `member_key_positions` |
| §3-P3:373 all-free arm `permanent_roots.push(...)` with NO request edge | **OVERRIDDEN by B2** | All-free arm MUST ALSO `AddRequestEdge(PermanentRoot,…)` (§5.4-B2) |
| §3-P3:394 `topo_order(rules_of(region))` | **NO BACKING STRUCTURE** | `RegionTemplate.rules` RESERVED-EMPTY (P6.2 sole populator); P3 runs NO rule sweep — AddDerivation is a P4+ spec, exercised at P3 only by the F29 intern gate (§1) |
| §5.4 M3 "layers over the retained backend" | **CLARIFIED** | P3 tracking is a COMPILE-TIME model; codegen unchanged; gates are structural (§1) |

**~~New finding (dead injector seam)~~ — STRUCK (critique §8-F5, verified false).** An earlier draft
claimed the query-injector seam (`BuildQueryInjectorProcedure`, Build.cpp:399-410) was inert dead
scaffolding because it returns `std::nullopt` unless `query.ForcingMessage()` is set. That is WRONG:
the `@first` forcing surface is a LIVE user feature that sets it — `@first message(...)` parses into
`forcing_predicates` (Clause.cpp:652-674), surfaced by `ParsedQuery::ForcingMessage()`
(Parse.cpp:1058-1066, 1179), and `tests/OptDiff/cases/force.dr`
(`#query get_next_id(bound i64 Time, free u32 NextId) @first : @first trigger_generate_next_id(Time)`)
reaches it. Deleting the seam would break `@first` query-time message injection. **P3 note:** §4's
`BuildRequestPorts` has NO arm for an `@first`-forced bound query; at P3 the forcer stays a
codegen-only concern (M3, unchanged), but P4 (when the model drives materialization) must model the
forced-request path.

---

## §6. Resolved open questions

- **L5 requested-relation resolution (RESOLVED, sound):** a `#query` relation-INSERT's `decl` IS the
  query declaration (`ParsedQuery::From(decl)`, Build.cpp:418), and `booleans.region.opt` shows the
  query relation `user_is_logged_in` carrying BOTH a permanent-root and a row-contract under its own
  decl. So `requested_relation_of_owner[owner] = RelationId{decl.Id()}` is correct — the owner's
  query decl Id equals the materialized relation's RelationId.
- **Rule source (RESOLVED, moot):** P3 is compile-time-model-only and runs no rule sweep (§1); the
  empty `RegionTemplate.rules` is not a gap.
- **`ProjectRow` ordinal space (verified):** `member_key_positions` is size==`decl.Arity()`,
  decl-ordinal; the bound test at Build.cpp:422-426 uses the same `param.Index()` decl-ordinal, so
  the two projections share one column space.

Remaining (P4-facing, non-blocking for the P3 diff):
- `RegionalCellValue` concrete public cell repr (uint64_t placeholder) — confirm against the codegen
  row-cell representation before typing `projected_values`. Only matters when AddDerivation sees a
  concrete row (P4+).
- `CallSiteId` interning granularity: `(ParsedDeclaration.Id(), BindingPattern)` to distinguish
  adornments of one multi-adornment `#query` name — mirror the Planning.cpp:309-314 dedup.
- Intern-key ordering for `std::map`/`std::set` keys: `std::variant` + defaulted `<=>` +
  `std::vector` `<=>` gives a strict-weak order; confirm acceptable vs a hash-set with `==`.

---

## §7. P3 exit gate (structural, discriminating — folds B2/H3/M3/L5)

> **REVISED by the critique (§8-F2/F4).** The `key_*` datasets no longer reach regional lowering
> (the demand corpus was deleted at P1), and several gates below are vacuous/non-discriminating as
> first written. The DISCRIMINATING battery is §8-F4: a new Regional unit-test binary for the edge
> ops + a committed bound-query `.region` golden + a structural (never answer-equality) rewrite of
> gate 6. Read §8-F4 as the authoritative gate; the list below is the intent, not the harness.

Directed battery over repurposed `key_*` datasets, plus the B2 no-bound-query probe. Every gate is
STRUCTURAL (observed via `-region-out` + compile-time asserts); answer-equality is a LOST CHECK.

1. **owner-count:** exact `AddRequestEdge` owner count == distinct owners minted; no fact duplicated.
2. **2nd-requester:** a second requester adds `RoutedResult`s, ZERO new `FactDerivation`s.
3. **caller-qualified removal:** `RemoveRequestEdge` on one of two retracts only its routed copies;
   the fact stays present.
4. **support ⟂ ownership:** derivation support is independently observable from request ownership.
5. **F29 member-key intern:** two rows agreeing on member-key columns intern to ONE `RegionalFactId`;
   rows differing there do not. (The sole P3 exercise of `AddDerivation`/member identity.)
6. **B2 no-bound-query probe:** a program with no bound `#query` still publishes its full answer — its
   `PermanentRoot` request edge makes `live={EmptyBindingState(R)}` non-empty.
7. **M3 recursive baseline:** a recursive program compiles + emits its induction regions + publishes,
   byte-identical to tip (P3 does not touch Stratum.cpp).
8. **acyclicity abort (dormant):** a cycle-closing `DeriveActivationEdge` trips the acyclic assert —
   only reachable once cross-state edges exist (post-P4/P5); does not misfire on P3 DAG programs (F18).

---

## §8. Critique survivors (folded) — adversarial refuter panel, session 18

Four opus refuters (authorities-edges / b2-publish / m3-discriminating / cross-phase) attacked this
doc against real code + the retained invariants + the false-starts checklist. Findings below are
deduped and ranked; each was VERIFIED against the tip. The strong certifications (what HELD) are at
the end. F1/F3/F5 are corrected INLINE above; F2/F4/F6-F9 amend the forward spec.

**F1 · BLOCKING · region liveness must not depend on a `#query` existing (fixed inline in §4).**
A program with NO `#query` that still publishes a message — `product_diff.dr`, `product_mixed`,
`product_self`, `product_conds` (all verified: no `#query`, each publishes + has a committed
`.stdout`) — mints ZERO request edges under the query-only `BuildRequestPorts`, so
`RootedReachability` returns `live={}` and the model represents a publishing program as DEAD while
the M3 backend publishes the full answer. **This reframes B2:** the real invariant is *region
liveness is rooted by the always-present ProgramRoot / result-port observation, INDEPENDENT of
queries* — a `#query` request edge is an ADDITIONAL observation root, never the sole liveness
source (else "no query justifies a relation" inverts to "a relation needs a query to be live").
Fix (applied §4): `RootedReachability` seeds `live = {EmptyBindingState(R0)}` from the singleton
ProgramRoot (`program_roots=1`, `RootAlive(ProgramRoot)≡true`) before adding query request edges.

**F2 · BLOCKING · `RootAlive(RootLease)` was contradictory (fixed inline in §4).** §1 said RootLease
owners are "always RootAlive"; §4's comment said "alive while leased" — an implementer transcribes
§4 and models a bound-query-only program (`booleans.dr`, `transitive_closure_diff.dr`) as dead.
**Unanimous adjudication (3 refuters):** at P3 `RootAlive(RootLease) ≡ true` UNCONDITIONALLY (the M3
backend materializes every relation unconditionally; "leased" has no compile-time meaning —
lease-conditionality is a P4/P8 lazy-backend concern). This also settles the two-reader dispute:
the bound→**RootLease-ONLY** topology (§4) is CORRECT; do NOT dual-root a bound query with a
PermanentRoot too (that would blur the RootLease/PermanentRoot distinction and drift toward the
"@key is another spelling of bound" false start).

**F3 · HIGH · `FactDerivation.src_state` must not be the routing key (forward-spec fix).** The
`map<RegionalFactId, FactDerivation>` stores ONE `src_state` per fact and `RouteResults` filters
`where d.src_state==st`. `next-session-prompt.md:550-551` requires a fact supported by TWO binding
states; at P4/P5 a fact derived from S1 then S2 keeps `src_state=S1`, so `RouteResults` for S2's
requester routes nothing — an under-answer, and a per-BindingState routing discriminator smuggled
onto the per-fact store (a partial B4 collapse in the frozen P4+ spec). **Fix:** keep support
per-`RegionalFactId` (count-only, B4-correct) but route a fact to a request edge with `dest=st`
whenever `st` has ANY live derivation of it — carry a per-fact SET of contributing states, or route
by the state's own derivation set. `src_state` is a per-derivation attribute, NEVER the routing key.
(Latent at P3 — single empty state — but frozen into the spec now, so corrected now.)

**F4 · HIGH · the §7 gates are vacuous / non-discriminating (gate rewrite — the AUTHORITATIVE gate).**
At P3 no `FactDerivation`/`RoutedResult` is built for any real program (§1), so gates 2/3/4/5 pass
on nothing (B3-style silent pass); gate 1 is non-discriminating because a bound `#query` currently
ALSO renders `-> permanent-root`/`request-ports=0` (verified: `average_weight.dr`, `booleans.dr`,
`force.dr`), so a P3 no-op that never touches Planning.cpp passes the whole committed golden set; and
the named `key_*` substrate no longer reaches regional lowering (demand corpus deleted at P1). The
discriminating battery:
- (a) **Regional unit-test binary** — a ctest peer of `IdentityTypes` that directly drives
  `AddRequestEdge`/`RemoveRequestEdge`/`AddDerivation` with SYNTHETIC rows, giving gates 2-5 (2nd
  requester adds RoutedResults / removal retracts only routed copies / support⟂ownership / F29
  member-key intern) real subjects. Without it those gates have no P3 harness (`-region-out` cannot
  observe them and there is no Regional unit test today).
- (b) **A committed bound-query `.region` golden** (`force` or `booleans` or `average_weight`) pinning
  `request-ports>=1` AND a `query-abi … -> request-port` (RootLease) line distinct from
  `-> permanent-root`. The three committed region goldens (`join_1`/`merge_2`/`tc_nonlinear_diff`)
  are ALL all-free queries, so none pins the bound→RootLease split; a stub is invisible to them.
- (c) **Rewrite gate 6 as purely structural** over `-region-out`: `ProgramRoot ∈ rooted owners ∧
  RootedReachability yields non-empty live`, NEVER "publishes its full answer" (publishing is a
  result-port/M3 concern, not a `#query` RoutedResult — see F1). Add a no-`#query` publisher
  (`product_diff`) to the battery so the ProgramRoot rooting is exercised.
- (d) Gates 2/3 are MODELED (typed edge ops self-validate statically) but NOT runtime-demonstrated at
  P3; `next-session-prompt.md` Phase-3's runtime attach/detach properties are DELIVERED as a typed
  model here and their BEHAVIORAL verification relocates to the P4 gate. Do not present them as
  runtime-proven at P3.
- (e) `-region-out` for every bound-query program CHANGES SHAPE by design (request-ports 0→N, a
  `-> request-port` line, the AbiRecord route flip); scope §1's "P3 moves no golden byte" to
  codegen/stdout goldens only. Pin the request-port SLOT ORDER explicitly (request ports slot FIRST,
  restoring pre-P1 numbering, or LAST) before the golden is blessed.

**F5 · HIGH · the "dead injector seam" finding was FALSE (struck inline in §5).** See §5 — the
`@first` forcing surface is live; the seam is reached for `force.dr`. Corrected.

**F6 · MEDIUM · census-side pseudocode missing.** §4 gives only the Build-side `BuildRequestPorts`;
`DeriveRegionalCensus` (Planning.cpp:225-242) hardcodes `request_ports=0` with NO query walk, and the
always-on `check_count("request-ports", …)` belt (Planning.cpp:361-376, fprintf+abort, survives
NDEBUG) aborts corpus-wide if the two authorities are not updated in EXACT lockstep. **Fix:** add
explicit `DeriveRegionalCensus` pseudocode counting bound-query redecls with the IDENTICAL dedup
(`seen_queries` by Id, `seen_variants` by BindingPattern, the `has_bound` per-redecl filter — the
Planning.cpp:301-320 walk). Mitigant: divergence is a LOUD ABORT, not a silent miscompile, and the
post-P1 corpus has no multi-adornment `#query`, so the dedup subtlety is currently unexercised.

**F7 · MEDIUM · `SortedBoundFieldValues` erases the field→value association (forward-spec fix).**
`using SortedBoundFieldValues = std::vector<RegionalCellValue>` (bare values) + opaque
`BindingStateSchemaId` carries no field tags; the name invites a value-sort that collapses `A=1,B=2`
with `A=2,B=1` — the P5 "[A,B] & [B,A] converge on one {A,B} state" requires those to be DISTINCT
`BindingStateId`s within schema {A,B}. `next-session-prompt.md:481` specifies
`canonical_bindings: sorted [BoundFieldValue]` where `BoundFieldValue = {field, value}`. **Fix:** carry
`(field, value)` pairs, OR document explicitly that `vals` is indexed by the schema's canonical
(field-id-sorted) field order and rename away from "Sorted…Values" to kill the value-sort reading.
Latent at P3 (always empty) but the substrate is chosen NOW.

**F8 · LOW · reserved P6 arm should be a `RegionalMember` newtype, not a bare `RegionalFactId`.**
`RequestOwnerId = variant<RootLeaseId, PermanentRootId, RegionalFactId>` lets a logical-fact id
double directly as a request-owner id; `next-session-prompt.md:496-499` wraps it
(`RegionalMember(RegionalFactId)`). **Fix:** introduce a `RegionalMemberId`/`RegionalMember` newtype
for the reserved arm now so the ownership authority never names a bare fact id.

**F9 · MEDIUM · Phase-3 runtime properties are MODELED, not runtime-demonstrated (scope note).**
Folded into F4(d): state in §1/§7 that attach/detach/removal are typed-modeled + statically
self-validated at P3, with behavioral verification at the P4 gate.

### Certifications (attacked, HELD)

- **Two-edge separation** — `activation_edges` RESERVED-EMPTY, `DeriveActivationEdge` returns NONE for
  every P3 call (single empty state, F18 self-loop guard), `RemoveRequestEdge` never touches
  `derivations`; the RequestEdge forest is acyclic depth-1 (dest always `EmptyBindingState`).
- **H3 no second value-id bridge** — the `FieldId/SemanticMemberKey → member_key_positions`
  conversion happens ONCE at P2 (`BuildRelationSchemaFromInsert`, Planning.cpp:200-209), stays a
  lib-private input; `projected_values` (member identity, position-ordered) and `vals` (binding
  identity) both ride the public `RegionalCellValue` substrate — a legitimate authority distinction.
- **count ⟂ owner** — `support>0` decides FACT MEMBERSHIP; binding-state LIVENESS is computed
  independently by `RootedReachability`; no count is a liveness oracle. `DerivationSupportCount` reuse
  keeps "a count stands in for an owner" a type error.
- **L5 requested-relation resolution** — verified: `booleans`/`tc_nonlinear_diff`/`force` each render
  a `permanent-root` AND a `row-contract` under one decl, so
  `requested_relation_of_owner[owner] = RelationId{decl.Id()}` is sound.
- **Message publish preservation** — a bound-query-only program that publishes a `#message` still
  publishes: publishing is a RESULT PORT (Planning.cpp:283-288) emitted by the retained backend
  (`BuildIOProcedure`, unchanged), independent of the request/routed model.
- **Answer byte-identicality** — §4 is ABI/ownership metadata only; codegen entry points (Site 1) +
  the M3 backend are untouched, so stdout is byte-identical to tip for every program.
- **Golden non-regression (current set)** — `join_1`/`merge_2`/`tc_nonlinear_diff` carry all-free
  queries → zero request ports minted, the 12 `.region` goldens are byte-stable. (Scope caveat F4(e):
  true only because no bound-query `.region` golden exists yet.)
- **Census lockstep belt** — the built-record recount (Planning.cpp:353-373) counting
  `PortKind::kRequest` + `check_count` abort catches a half-done split (kRequest pushed without the
  census update) as a loud corpus-wide abort, not a silent miscompile.
- **False starts (a)-(e) all HELD** — @key is not a spelling of bound (request-edge KIND keys on query
  binding, dest always `EmptyBindingState`, @key inert); binding state is not a 2nd fact owner
  (`derivations` keyed on `RegionalFactId`); no ref-count liveness oracle; no silent flat-arch
  fallback (codegen is the sanctioned M3 baseline); no database-per-ordering (one
  `RegionInstanceId(0)`, one `RegionalFactRelation`).
- **`CallSiteId`/`RegionInstanceId` forward-compose to P6** — uint32 domains admit multiplicity;
  `RegionInstanceId(0)`-singleton + `regions=1` are documented Stage-B constants P6.1 lifts, not
  shape assumptions.

---

## §9. IR desired-states (predict-then-verify; STRUCTURAL pins only)

**At P3 the ONLY dump that changes is `-region-out`.** `.rel` (DR ops / eager markers) and the
generated C++ are BYTE-UNCHANGED — the P3 request/routed model is compile-time-only and codegen is
the retained M3 backend (§1). So the `.rel`/`.h` goldens do NOT move at P3, and the seed's ".rel
request-edge/lifecycle op family" is a P4+ surface (when the model gains runtime emission). The P3
predict-then-verify target is the `-region-out` request-port render for a bound-query carrier.

**Carrier:** `booleans.dr` (`#query user_is_logged_in(bound i32 UserId)`), or `force`/`average_weight`
(both have bound queries). CURRENT `-region-out` (verified, opt mode):

```
  query-abi   user_is_logged_in(UserId:bound i32)  -> permanent-root      # CURRENT: bound == free
  ...
  permanent-root  user_is_logged_in(UserId)
  census: ... request-ports=0 input-ports=2 result-ports=0 row-contracts=3
```

**POST-P3 desired state (structural pin):** the bound query flips from permanent-root to a
RootLease-owned request port; census `request-ports 0→1`; the region body gains a request-port line
and (optionally) a request-edge owner line. Illustrative shape (exact tokens are an implementer
choice — pin whatever `Format.cpp` emits):

```
  query-abi   user_is_logged_in(UserId:bound i32)  -> request-port Pk      # route flips kPermanentRoot -> kToPortP
  ...
  request-port    Pk  message=user_is_logged_in/1  fields=(UserId)  owner=root-lease#0  dest=empty-state
  census: ... request-ports=1 input-ports=2 result-ports=0 row-contracts=3
```

**Slot-order STOP (§8-F4e):** where the request port `Pk` numbers relative to input/result ports is
undecided. Pre-P1 numbering put request ports FIRST (Planning.cpp:272 comment); the current code
numbers input then result. Pick one and pin it in the golden BEFORE blessing. Recommendation:
request ports FIRST (restoring pre-P1 PortId ordering) so the request/observe surface reads top-down,
but this is an owner call — it re-pads/renumbers a bound-query program's port ids either way.

**Golden actions at P3 (all STRUCTURAL, not answer):**
- BLESS a NEW bound-query `.region` golden (`booleans` or `force`) pinning `request-ports>=1` + the
  `-> request-port` route + the request-port body line. Without it no committed golden sees the
  bound→RootLease split (§8-F4b) — the three existing region goldens are all all-free.
- The three existing `.region` goldens (`join_1`/`merge_2`/`tc_nonlinear_diff`, all all-free queries)
  stay BYTE-IDENTICAL — a P3 that moves them is over-reaching (regression signal).
- ALL `.rel`, `.ir`, `.h`, `.stdout` goldens stay byte-identical (M3). Any `.rel`/codegen golden move
  at P3 is a bug (codegen must not change).
- The no-`#query` publisher probe (`product_diff`) has no `.region` golden today; add one only if it
  pins the ProgramRoot rooting structurally (§8-F1) — its census stays `request-ports=0` (it has no
  query) but its ProgramRoot must render as a rooted owner.

**Verification (when P3 lands, not now — docs-only session):** compile the carrier with `-region-out`
in all 4 modes; assert the request-port shape above + census delta; assert `.rel`/`.h` byte-identical
to tip; run the Regional unit-test binary (§8-F4a) for the edge-op gates.
