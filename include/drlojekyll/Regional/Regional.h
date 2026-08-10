// Copyright 2026, Peter Goodman. All rights reserved.

#pragma once

#include <drlojekyll/DataFlow/Query.h>
#include <drlojekyll/Parse/Parse.h>
#include <drlojekyll/Regional/RegionInstance.h>  // the regional typed-id domains
                                                 // + the P3 request/derivation
                                                 // model (stored by value below)

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

// Stage B of the Regional Dataflow proposal (RegionalDataFlowCore.artifacts/
// stage-b-diff.md + p2-typed-owner-grounding.md): the FROZEN REGIONAL PROGRAM.
//
// P2 (the typed-owner cut): `FrozenRegionalProgram` is now the TYPED SEMANTIC
// OWNER, not a render-ready-strings + Query-passthrough shell. It stores ONE
// `RegionTemplate` of TYPED records (relation schemas, ABIs, ports, permanent
// roots) computed from the FINAL Query graph (after Query::Build, before
// Program::Build). The `-region-out` / `-region-dot-out` dumps DERIVE their
// text from these typed records at dump time (lib/Regional/Format.cpp) — the
// five owned render-STRING vectors (the pre-P2 RegionalAbi/Port/Internal/
// PermanentRoot/Contract shells) are GONE. The DataFlow graph is retained,
// exposed only via `DataFlowGraph()` for `Program::Build`.
//
// At Stage B every program is ONE ProgramRoot + ONE observation-root region
// `RegionId(0)`, with ZERO child calls.
//
// Determinism (the HP-9 rule): every id here (`R0`, `P0`, `E0`) is DENSE and
// minted by deterministic declaration/range walks — never from a pointer
// order, never from a `UniqueId`. No `%table:N` appears (those are minted
// inside `Program::Build` and do not exist when this freezes).

namespace hyde {

class ErrorLog;
class OutputStream;

// The typed id domains (`RegionId`/`PortId`/`EdgeId`/`RelationId`/
// `SymbolicFieldId`) + the P3 request/derivation model live in
// `RegionInstance.h` (included above) — the lib/DataFlow/Identity.h idiom, one
// disjoint DOMAIN per id. They moved there at P3 so this header can store a
// `RegionInstanceRelations` by value.

// The Stage-B region-skeleton census: the seven counts the `-region-out`
// dump's trailing `census:` line renders, re-derivable from the Query graph's
// PUBLIC surface alone (see `DeriveRegionalCensus`).
struct RegionalCensus {
  unsigned regions{1};
  unsigned child_calls{0};
  unsigned program_roots{1};
  unsigned request_ports{0};
  unsigned input_ports{0};
  unsigned result_ports{0};
  unsigned row_contracts{0};
};

// ---- The TYPED records (P2). No pre-rendered strings; render derives from
// these at dump time.

// The logical-fact authority for one stored/interior relation (Rule R-STORE +
// the Tier-2 origin-interior lift). Built by two arms (Planning.cpp): the
// INSERT arm copies the positional member key out of the Stage-A RowContract;
// the ORIGIN arm (undemanded interiors with no RowContract) uses the all-field
// key + the OR-over-carriers support.
struct RelationSchema {
  RelationId id;              // == decl.Id()
  ParsedDeclaration decl;     // the relation's logical identity; render reads its
                              // NthParameter names (a parse handle, not a string
                              // shell). The declared-`@key` DOT badge derives from
                              // `decl.HasInstanceKey()` — no separate field.

  // The POSITIONAL semantic member-key mask, size == `decl.Arity()`: position
  // `i` is set iff the relation's i-th declared parameter names a member-key
  // value. This IS the member key (P3's `AddDerivation` projects a row through
  // this positional mask). INSERT arm: `i < rc.visible_fields.size() &&
  // rc.visible_fields[i] in rc.member_key`. ORIGIN arm: all true (AllFields).
  // The raw value-id `SemanticMemberKey` is a lib/DataFlow PRIVATE type and
  // stays a Planning.cpp-local input to this precompute — it cannot live on
  // this PUBLIC record.
  std::vector<bool> member_key_positions;

  bool support{false};        // true => "differential"; false => "monotone".

  // P5: the relation's ORDERED declared `@key` access paths (the THIRD
  // authority — logical access path). Interned from `decl.InstanceKeys()` at
  // BOTH schema arms; empty for an unkeyed relation. The `-region-out`
  // `declared-key` render + the freeze-side binding-schema DAG read this. NEVER
  // fed to the physical `AccessPlan` (the four-authority separation).
  DeclaredAccessPathSet declared_access_paths;
};

// One program-root ABI record (retypes the pre-P2 RegionalAbi string shell).
enum class AbiKind : uint8_t { kInput, kQuery, kOutput };

// How a program-root ABI routes into the region. `kNone` is the synthetic
// `output-abi <none>` line (no published messages). `kRequestPort` (P3) is a
// bound `#query` routing to a RootLease-owned request port (`route_port` is the
// request port index).
enum class RouteKind : uint8_t {
  kToPortP,
  kPermanentRoot,
  kRequestPort,
  kNone
};

struct AbiRecord {
  AbiKind kind;

  // A message for input/output ABIs, a query declaration for query ABIs, and
  // `std::monostate` for the synthetic `<none>` output ABI. `std::monostate`
  // is FIRST so the variant is default-constructible and the decl-less `<none>`
  // case is representable (parse handles have no default constructor).
  std::variant<std::monostate, ParsedMessage, ParsedDeclaration> decl;

  RouteKind route;
  unsigned route_port{0};  // Valid iff `route == kToPortP`.
};

// One region port record (retypes the pre-P2 RegionalPort string shell).
enum class PortKind : uint8_t { kRequest, kInput, kResult };

struct PortRecord {
  PortKind kind;
  unsigned port_index;
  ParsedMessage message;  // Render derives "message=<name>/<arity>" + fields.
};

// One permanent-root record (retypes the pre-P2 RegionalPermanentRoot shell).
struct PermanentRootRecord {
  ParsedDeclaration decl;  // Render derives "<name>(<param names>)".
};

// One request-port record (P3): a bound `#query` observation entry, owned by a
// RootLease. Numbered AFTER the input/result ports, so an all-free program
// (zero request ports) is byte-identical to the pre-P3 render. Render derives
// "request-port P<k> query=<name>/<arity> bound=(<bound param names>)".
struct RequestPortRecord {
  unsigned port_index;
  ParsedDeclaration query_decl;  // the bound query redeclaration.
  RootLeaseId lease;
  CallSiteId call_site;

  // P4: the physical AccessPlan selected AT FREEZE for this bound query and READ
  // back at codegen (BuildQueryEntryPointImpl) — the real compile-time data
  // dependency that makes the AccessPlan authority non-nominal (p4-grounding.md
  // §3.1/§3.2/§8-S1). kFullScanFilter withholds the index (nonrecursive bound+free);
  // kFullKeyHashLookup is the all-bound `.Find`; kRetainedIndexScan keeps the seek.
  AccessPlan plan{AccessPlan::kRetainedIndexScan};
};

// RESERVED-EMPTY typed types at P2 (structural reservation only; P6 adds their
// fields when the P3/P6 authority types they will reference exist). Populated
// by nobody at P2 — the false-start certification (freeze is a pure read, no
// recognition pass) holds by construction.
struct RuleRoutingProjection {};   // P6.2 is the sole populator.
struct RecursiveComponent {};      // P6.1 is the sole populator.

// The ONE typed owner: the Stage-B region skeleton as typed records.
struct RegionTemplate {
  RegionId id{0};

  // Empty at Stage B (P6.2 populates the inherited symbolic-field frame).
  std::vector<SymbolicFieldId> inherited_symbolic_fields;

  std::vector<AbiRecord> abis;              // input, then query, then output.
  std::vector<PortRecord> ports;            // input ports, then result ports.
  std::vector<RequestPortRecord> request_ports;  // P3: bound-query request ports.
  std::vector<PermanentRootRecord> permanent_roots;
  std::vector<RelationSchema> relation_schemas;  // R-STORE, then Tier-2 origin.

  std::vector<RuleRoutingProjection> rules;                // RESERVED EMPTY (P6.2).
  std::vector<RecursiveComponent> recursive_components;    // RESERVED EMPTY (P6.1).
};

// Derive the Stage-B census from the Query graph's PUBLIC surface only — a
// PURE function. It is the SINGLE census authority: `Build` populates its
// stored census from it, and the ControlFlow-side V-REGION-CENSUS validator
// (lib/Rel/Rel.cpp, ValidateDROps tail) recounts through it, so a
// FrozenRegionalProgram stubbed to an empty shell aborts downstream (the
// positive-presence referee).
RegionalCensus DeriveRegionalCensus(const ::hyde::Query &query);

// The frozen regional program: ONE ProgramRoot + ONE observation-root region
// `RegionId(0)`, zero child calls. Immutable after `Build`.
class FrozenRegionalProgram {
 public:
  // The H2 degenerate planner + freeze. Deterministic: every derivation is a
  // declaration/range walk, never a pointer order. Runs the always-on freeze
  // validators (V-FROZEN-NO-OPEN-PORT, V-OWNERSHIP-ACYCLIC) before returning.
  static std::optional<FrozenRegionalProgram> Build(const ::hyde::Query &query,
                                                    const ErrorLog &log);

  // The retained DataFlow graph (`Program::Build` consumes it). RENAMED from
  // `Query()` at P2 — the frozen program is no longer a Query passthrough.
  const ::hyde::Query &DataFlowGraph(void) const;

  const RegionalCensus &Census(void) const;

  // The typed region skeleton (the `-region-out` dump reads this).
  const RegionTemplate &Region(void) const;

  // The P3 request/derivation model (populated at freeze by BuildRequestPorts;
  // the derivation/routing half stays empty for real compiles — no rule sweep
  // at P3). A freeze-side peer of `RegionTemplate`.
  const RegionInstanceRelations &Instances(void) const;

  // P4: the physical AccessPlan selected at freeze for a bound `#query`
  // redeclaration (matched by decl Id + binding pattern). `std::nullopt` for an
  // all-free query (a PermanentRoot, no request port) — the caller keeps its
  // retained behavior. Read by `BuildQueryEntryPointImpl` (§8-S1).
  std::optional<AccessPlan> PlanFor(ParsedDeclaration redecl) const;

 private:
  explicit FrozenRegionalProgram(const ::hyde::Query &query_);

  ::hyde::Query dataflow_graph;
  RegionalCensus census;
  RegionTemplate region;
  RegionInstanceRelations instances;
};

// The `-region-out` G1 text dump (byte-golden-able; tests/OptDiff `region`
// irgold surface).
struct FrozenRegionalDump {
  const FrozenRegionalProgram &program;
};

OutputStream &operator<<(OutputStream &os, FrozenRegionalDump d);

// The `-region-dot-out` GraphViz DOT twin (advisory visualization, never
// byte-goldened — the house DOT-twin directive).
struct FrozenRegionalDOT {
  const FrozenRegionalProgram &program;
};

OutputStream &operator<<(OutputStream &os, FrozenRegionalDOT d);

}  // namespace hyde
