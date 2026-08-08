# P2 — typed FrozenRegionalProgram owner: grounding + compile-clean re-point inventory (session 17)

Branch `keyed-instances`, tip `dc965d3c` (+ docs handoff `0b64aec4`). All code anchors below
were RE-VERIFIED at tip this session by direct read (not range-anchored trust). Owner directive
this session: **"1 then 2"** — run the grounding loop to execution-readiness, THEN execute P2.

This file is the P2 execution-readiness deliverable: (§1) the compile-clean consumer inventory
(the P2 analog of the P1 symbol-driven grep), (§2) the FULL typed-record design — the diff
§3-P2 fully specs only `RelationSchema`; byte-identical render also needs typed port / ABI /
permanent-root records, designed here, (§3) the byte-identical render mapping, (§4) the
re-pointed recount belt, (§5) the exit gate, (§6) the adversarial-critique survivors.

---

## §1. Compile-clean re-point inventory (the P2 symbol-driven grep — VERIFIED at tip)

The blast radius is REMARKABLY contained. Nothing in `lib/Rel` or `lib/ControlFlow` (beyond one
call site) re-parses the render-string shells or unwraps `frozen.Query()`.

### §1.1 `frozen.Query()` unwrap consumers — EXACTLY ONE
- `lib/ControlFlow/Build/Build.cpp:1209` — `const ::hyde::Query &query = frozen.Query();`
  → RE-POINT to `frozen.DataFlowGraph()` (the accessor rename). The Query passthrough STAYS
  (Program::Build still consumes the DataFlow graph); only the accessor NAME changes.

### §1.2 `frozen.Census()` consumer — EXACTLY ONE (STAYS, H1)
- `lib/ControlFlow/Build/Build.cpp:1324` — `context.frozen_census = &frozen.Census();`
  → UNCHANGED. `Context::frozen_census` stays `const RegionalCensus *` (Build.h:208 comment).
  Its reader is the V-REGION-CENSUS validator (Rel.cpp tail). H1: do NOT retype.

### §1.3 Render-string field consumers — ALL inside `lib/Regional/Format.cpp`
Every read of `decl_text / route_text / head_text / fields_text / member_key_text /
support_text / rel_name / .text` is in `lib/Regional/Format.cpp` (the `-region-out` text dump
+ the `-region-dot-out` DOT twin). NONE outside lib/Regional. The DOT twin (Format.cpp:198-204)
STRING-PARSES `route_text` (`.rfind(" P")`, `== "-> permanent-root"`) — P2 replaces that parse
with a typed `RouteKind` enum + `port_index` read.

### §1.4 The row-contract friend leak — EXACTLY ONE (STAYS, M1)
- `lib/Regional/Planning.cpp:402` — `const RowContractMap &row_contracts = query.impl->row_contracts;`
  → KEEP the friend-class access (M1: `RowContractMap`/`RowContract`/`QueryViewImpl` are PRIVATE
  lib types; a public `Query` accessor WIDENS the leak). lib/Regional already includes the private
  `lib/DataFlow/Query.h` (Planning.cpp:30 `#include "Query.h"`). The read MOVES into
  `BuildRelationSchema`, same site semantics.

### §1.5 Struct-name consumers to retype/delete
- `include/drlojekyll/Regional/Regional.h:75/82/96/101/107` — `RegionalAbi`/`RegionalPort`/
  `RegionalInternal`/`RegionalPermanentRoot`/`RegionalContract` structs + the 5 owned vectors +
  the 5 const-ref accessors + `RegionalCensus` (KEPT).
- `lib/Regional/Planning.cpp` — the whole `Build` body constructs the 5 string vectors → rebuilt
  to construct ONE typed `RegionTemplate`.
- `lib/Regional/Format.cpp` — both dumps rewritten to render from the typed records.

### §1.6 EXIT-GATE grep target (must return zero after P2)
```
grep -rn 'frozen\.Query()\|RegionalAbi\|RegionalPort\|RegionalContract\|RegionalInternal\|
          RegionalPermanentRoot\|member_key_text\|route_text\|head_text' lib/Regional lib/Rel lib/ControlFlow
```
`query.impl->row_contracts` is EXEMPT (M1 — it stays, contained to lib/Regional).

**Nothing in `bin/` reads the frozen render structs** (Main.cpp only calls
`FrozenRegionalProgram::Build` + streams `FrozenRegionalDump`/`FrozenRegionalDOT`). Verified.

---

## §2. The FULL typed-record design (diff §3-P2 specs only RelationSchema — this completes it)

The diff's `RegionTemplate` lists `relation_schemas / rules / recursive_components /
request_ports / result_ports / permanent_roots`. For BYTE-IDENTICAL render (goldens are the
anti-stub belt) the template must ALSO carry typed records for the program-root ABIs and the
R0 input/result ports — today's `RegionalAbi` and `RegionalPort`. Retype, do not pre-render.

```cpp
// ---- Field / relation identity (typed, decl-ordinal) ----
struct SymbolicFieldId { uint32_t v; <=> };     // one per decl parameter ordinal
struct RelationId      { uint64_t v; <=> };     // == decl.Id()

// ---- Logical-fact authority: the P2 HEADLINE record ----
// Built by TWO arms (mirroring Planning.cpp's two contract loops), appended R-STORE then Tier-2
// on ONE dense positional edge counter:
//   INSERT arm (BuildRelationSchemaFromInsert): rc = row_contracts.at(view.impl);
//     member_key_positions[i] = (i < rc.visible_fields.size() && rc.visible_fields[i] in rc.member_key);
//     support = view.CanReceiveDeletions().
//   ORIGIN arm (BuildRelationSchemaFromOrigin): NO RowContract (Tier-2 is insert-cleared);
//     member_key_positions[i] = TRUE for all i in [0,arity) (reproduces AllParamNames);
//     support = ResolveOriginSupport(query, decl)  (OR over origin-carrying views — B1 fix).
struct RelationSchema {
  RelationId id;                          // RelationId(decl.Id())
  ParsedDeclaration decl;                 // the logical relation identity (render reads its
                                          //   NthParameter names — a parse handle, NOT a string
                                          //   shell, NOT a RowContract re-lookup). DOT badge =
                                          //   decl.HasInstanceKey() (L2: no separate field).
  std::vector<bool> member_key_positions;     // size==arity; the POSITIONAL SEMANTIC-member-key mask
                                              //   (NOT @key — L1 rename). Render reads THIS (no bridge).
                                              //   INSERT arm: the Planning.cpp:419-436 bool; ORIGIN arm:
                                              //   all-true. This IS the P3-usable member key (B3/H3:
                                              //   AddDerivation projects the row through the POSITIONAL
                                              //   mask, not raw value-id SemanticMemberKey).
  bool support;                           // "differential"/"monotone" (per-arm source above)
  // B3 (public-header visibility): the raw `SemanticMemberKey visible_fields/member_key` the diff
  //   listed CANNOT live here — SemanticMemberKey is a lib/DataFlow/Identity.h PRIVATE type, and
  //   Regional.h is a PUBLIC header (public DataFlow/Query.h does not re-export it). They stay
  //   Planning.cpp-LOCAL inputs to the member_key_positions precompute (Planning.cpp sees the
  //   private types via the lib/DataFlow include seam). `fields: vector<SymbolicFieldId>` is
  //   OMITTED at P2 (unused by render/recount; P6.2 adds it when SymbolicFieldId is consumed).
};

// ---- Program-root ABI (retypes RegionalAbi; NO decl_text/route_text strings) ----
enum class AbiKind  { kInput, kQuery, kOutput };
enum class RouteKind{ kToPortP, kPermanentRoot, kNone };   // kNone == the `output-abi <none>` line
struct AbiRecord {
  AbiKind kind;
  std::variant<std::monostate, ParsedMessage, ParsedDeclaration> decl;  // monostate FIRST (B2 fix):
                                          //   the <none> output-abi holds monostate (Parse handles have
                                          //   no default ctor, Node.h:17); message for input/output,
                                          //   decl for query. Render branches holds_alternative<monostate>
                                          //   BEFORE touching the decl.
  RouteKind route;
  unsigned  route_port;                  // valid iff route==kToPortP
};

// ---- R0 port (retypes RegionalPort; NO head_text/fields_text strings) ----
enum class PortKind { kRequest, kInput, kResult };
struct PortRecord {
  PortKind kind;
  unsigned port_index;
  ParsedMessage message;                 // render derives "message=<name>/<arity>" + fields
};

// ---- Permanent root (retypes RegionalPermanentRoot) ----
struct PermanentRootRecord {
  ParsedDeclaration decl;                // render derives "<name>(<param names>)"
};

// ---- The ONE typed owner ----
struct RegionTemplate {
  RegionId id{0};                                  // RegionId(0) at Stage B
  std::vector<SymbolicFieldId> inherited_symbolic_fields;  // EMPTY at Stage B
  std::vector<AbiRecord>            abis;           // input, then query, then output (RegionalAbi::Kind order)
  std::vector<PortRecord>           ports;          // input ports, then result ports (push order)
  std::vector<PermanentRootRecord>  permanent_roots;
  std::vector<RelationSchema>       relation_schemas;   // R-STORE contracts THEN Tier-2 origin interiors
  std::vector<RuleRoutingProjection> rules;             // NEW field, RESERVED EMPTY at P2 (P6.2 fills)
  std::vector<RecursiveComponent>    recursive_components; // RESERVED EMPTY at P2 (P6.1 sole populator)
  // request_ports live inside `ports` (PortKind::kRequest); count==0 post-cut.
};

class FrozenRegionalProgram {
  const ::hyde::Query &DataFlowGraph(void) const;  // RENAMED from Query()
  const RegionalCensus &Census(void) const;        // KEPT
  const RegionTemplate &Region(void) const;        // NEW — the single typed accessor Format reads
 private:
  ::hyde::Query dataflow_graph;                     // RENAMED owner field
  RegionalCensus census;                            // KEPT (query-derived, F12)
  RegionTemplate region;                            // replaces the 5 string vectors
};
```

`RuleRoutingProjection` / `RecursiveComponent` — EMPTY structs `{}` at P2 (L4 fix: NO fields —
a field referencing a P3/P6 authority type would fabricate that authority now). P6 adds their
fields when the authority types exist. `internals` is DROPPED from RegionTemplate (L5:
provably always-empty post-P1). No `DeclaredAccessPathSet` / access-path interner at P2 (L2:
the field was dropped — the @key metadata reaches render via `decl.HasInstanceKey()`).

**Boundary discipline (four authorities):** P2 touches ONLY the logical-fact authority
(`member_key`, via `declared_key_positions`) + render/naming plumbing. It must NOT fabricate:
the residual authority (`BindingState` — P3), the physical authority (`AccessPlan` — P7), or a
value-bearing access-path interner (P5). `declared_access_paths` is carried INERT (the @key
metadata is parse-surface-only until P5). `rules`/`recursive_components` are structurally
reserved, never populated by a recognition pass (false-start certified: freeze is a pure
post-Optimize read).

---

## §3. Byte-identical render mapping (Format.cpp rewritten off `p.Region()`)

Every string the current Format.cpp reads is re-derived from the typed record. The goldens
(join_1, merge_2, tc_nonlinear_diff × 4 modes) are the byte belt.

| current string field | typed source | render fn |
|---|---|---|
| `abi.decl_text` (input/output) | `AbiRecord.decl` (ParsedMessage) | `MessageDeclText(module, m)` |
| `abi.decl_text` (query) | `AbiRecord.decl` (ParsedDeclaration) | `QueryDeclText(module, decl)` |
| `abi.decl_text` (`<none>`) | kOutput+kNone | literal `"<none>"` |
| `abi.route_text` | `AbiRecord.route` + `route_port` | `kToPortP`→`"-> R0 via P"+n`; `kPermanentRoot`→`"-> permanent-root"`; `kNone`→`""` |
| `port.head_text` | `PortRecord.message` | `"message="+name+"/"+arity` |
| `port.fields_text` | `PortRecord.message` | `MessageFieldNames(m)` |
| `root.text` | `PermanentRootRecord.decl` | `name + AllParamNames(decl)` |
| `contract.rel_name` | `RelationSchema.decl` | `decl.NameAsString()` |
| `contract.member_key_text` | `RelationSchema.{decl,declared_key_positions}` | `RenderMemberKeyText` below |
| `contract.support_text` | `RelationSchema.support` | `support ? "differential" : "monotone"` |
| `contract.declared_key` (DOT badge) | `RelationSchema.declared_access_paths` | `!declared_access_paths.empty()` |

```
RenderMemberKeyText(schema):
    parts = [ schema.decl.NthParameter(i).NameAsString()
              for i in [0, schema.decl.Arity())
              if schema.declared_key_positions[i] ]
    return "(" + join(parts, ", ") + ")"
```
This is EXACTLY Planning.cpp:417-437's inline loop, moved to render and sourced off the
precomputed bool vector — the `decl.Arity() <= i` unit-relation break is subsumed because
`declared_key_positions` is sized to `decl.Arity()` (the F13 unit-relation `member-key=()` case
falls out: a unit relation's arity-sized position vector is all-false → empty parens).

The DOT twin's `route_text` string-parse (Format.cpp:198-204) is replaced by a typed switch on
`AbiRecord.route` → `proot_j` (kPermanentRoot) / `port_p<route_port>` (kToPortP) / no edge (kNone).

---

## §4. The re-pointed recount belt (H2 — the real anti-stub referee)

V-REGION-CENSUS at the Rel tail is TAUTOLOGICAL (stored `DeriveRegionalCensus(query)` vs fresh
`DeriveRegionalCensus(query)`). The genuine anti-stub belt is the in-`Build` recount
(Planning.cpp:475-497), which today counts the BUILT string vectors P2 deletes. RE-POINT it at
the typed `R`:

```
// after `out.census = DeriveRegionalCensus(query);`
built_input = built_result = built_request = 0
for port in R.ports: switch(port.kind) { kInput:++built_input; kResult:++built_result; kRequest:++built_request; }
check_count("input-ports",   built_input,                     census.input_ports)
check_count("result-ports",  built_result,                    census.result_ports)
check_count("request-ports", built_request,                   census.request_ports)   // 0==0
check_count("row-contracts", R.relation_schemas.size(),       census.row_contracts)
```
Now the recount is a GENUINE cross-check: typed `R` (built by the freeze walk) vs
`DeriveRegionalCensus(query)` (an independent query re-walk). A hollow/stub `R` diverges and
aborts. Census stays a function of `query`, NEVER of `RegionTemplate` (F12).

---

## §5. Exit gate (structural, discriminating — answer-equality is a LOST CHECK post-P1)

1. The 12 surviving `.region.<mode>` goldens (join_1, merge_2, tc_nonlinear_diff × 4 modes)
   re-derive from typed `R` with NO `--bless`. (Byte-identical render is the primary belt.)
2. `grep §1.6` returns zero (no `frozen.Query()` / `Regional{Abi,Port,Contract,Internal,
   PermanentRoot}` / `*_text` string-shell outside the exempt friend leak).
3. The re-pointed in-`Build` recount (§4) reads typed `R` vs `DeriveRegionalCensus(query)`.
4. `IdentityTypes` ctest passes with `member_key` typed (SemanticMemberKey unchanged — it was
   already typed; the gate confirms no regression).
5. OptDiff `SUITE: PASS`; ctest 4/4.

**One coherent commit.** The region goldens are the anti-stub belt; no golden re-blessed
(P2 is byte-preserving by construction — it retypes the OWNER, not the RENDERED BYTES).

---

## §6. Adversarial-critique survivors (4-lens opus panel, folded)

Two BLOCKING design gaps + refinements. All verified against real code. Folded into §2–§5 above.

- **B1 (Tier-2 arm) — FOLDED.** The original §2/§3 specced only the R-STORE arm. A Tier-2
  origin-interior schema (CollectOriginInteriorDecls) has NO RowContract; its bytes come from
  `AllParamNames(decl)` (member-key = ALL params) + `ResolveOriginSupport(query, decl)` (OR over
  origin-carrying views), NOT `view.CanReceiveDeletions()`. Tier-2 rows are ~half of EVERY golden
  (join_1 E2/E3, merge_2 E2, tc_nonlinear_diff E1/E2). FIX: `BuildRelationSchema` has TWO arms
  mirroring Planning.cpp's two loops — the insert arm (rc-copy) and the origin arm (all-true
  positions + ResolveOriginSupport). See §2/§3.
- **B2 (AbiRecord `<none>`) — FOLDED.** `variant<ParsedMessage, ParsedDeclaration>` cannot hold
  the `<none>` output-abi (present in all 12 goldens; Parse handles have no default ctor,
  Node.h:17). FIX: `variant<std::monostate, ParsedMessage, ParsedDeclaration>` (monostate first);
  render branches on `holds_alternative<monostate>` before touching the decl. See §2/§3.
- **L1 (naming) — FOLDED.** `declared_key_positions` renamed `member_key_positions` — it is the
  semantic-member-key mask, unrelated to @key; the old name collided with @key metadata.
- **L2 (declared_access_paths) — FOLDED (DROPPED the field).** Redundant with `decl` (DOT badge
  == `decl.HasInstanceKey()`) and "access path" pre-commits the P7 physical authority onto inert
  P2 logical @key metadata — the four-authority-collapse hazard. Dropped from RelationSchema; DOT
  badge derives from `schema.decl.HasInstanceKey()`. (If P5 wants a typed carry, it names it
  `declared_key_sets` — LOGICAL — then, not now.)
- **L3 (module provenance) — FOLDED.** Both `operator<<` bodies obtain the module via
  `p.DataFlowGraph().ParsedModule()` and thread it to `MessageDeclText`/`QueryDeclText`.
- **L4 (reserved structs) — FOLDED.** `RuleRoutingProjection`/`RecursiveComponent` are EMPTY
  structs `{}` at P2 (no fields — fields referencing P3/P6 authority types would fabricate them).
- **L5 (internals deletion) — FOLDED.** Format.cpp:84-86/112-114/170-173 (the Internals render)
  are deleted with RegionalInternal; byte-safe because `internals` is provably never populated
  post-P1 (no writer in Planning.cpp), so the `region-internal` kind-width contribution (which
  would inject width 15) never fired.
- **L6 (§5 belt honesty) — FOLDED.** The 12 goldens are VACUOUS for: a published output-abi
  (kOutput+ParsedMessage+kToPortP), a result-port (PortKind::kResult), the declared-key DOT badge
  (DOT never goldened), and the unit-relation `member-key=()` case. Those render arms ship covered
  by logic argument + the §4 recount + IdentityTypes, NOT the byte belt. §5 scopes this honestly.
- **L7 (§4 language) — FOLDED.** The re-pointed recount is the SAME anti-stub referee as today
  (both share CollectContractInserts/CollectOriginInteriorDecls), not a strictly stronger one; a
  hollow `R` still diverges and aborts. §4 no longer oversells it.
- **CERTIFICATIONS (design points that HELD):** the R-STORE `member_key_positions` precompute is
  byte-exact (incl. unit-relation `()` and vf.size()>arity); ownership forest stays acyclic
  (V-OWNERSHIP-ACYCLIC gates on query-derived counts only); census stays a pure fn of query (F12);
  freeze introduces NO mutate-then-recognize authority (BuildRelationSchema is a pure read;
  reserved vectors have zero populators); the §1 blast-radius inventory is accurate; carrying
  `ParsedDeclaration decl` is legitimate logical identity, not a re-derivation back-door;
  `DeclaredAccessPathSet` was definable without a P5 interner (moot — field dropped).
