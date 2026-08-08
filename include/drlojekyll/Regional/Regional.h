// Copyright 2026, Peter Goodman. All rights reserved.

#pragma once

#include <drlojekyll/DataFlow/Query.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Stage B of the Regional Dataflow proposal (RegionalDataFlowCore.artifacts/
// stage-b-diff.md): the FROZEN REGIONAL PROGRAM — the H2 degenerate planner's
// output, an immutable skeleton computed from the FINAL Query graph (after
// Query::Build, before Program::Build) that `Program::Build` now consumes.
//
// At Stage B every program is ONE ProgramRoot + ONE observation-root region
// `RegionId(0)`, with ZERO child calls. Logical origins are the identity map
// at Stage B — no storage exists for them yet; this comment records the
// reservation (Stage C mints real origin sets).
//
// Determinism (the HP-9 rule): every id here (`R0`, `P0`, `E0`) is DENSE and
// minted by deterministic declaration/range walks — never from a pointer
// order, never from a `UniqueId`. No `%table:N` appears (those are minted
// inside `Program::Build` and do not exist when this freezes).

namespace hyde {

class ErrorLog;
class OutputStream;

// ---- Typed id domains (the lib/DataFlow/Identity.h idiom): each type
// reserves a DOMAIN with intra-domain comparison only — no cross-domain
// operator and no implicit conversion, so a port index can never silently
// stand in for a region or edge id. At Stage B only `RegionId(0)` exists.

struct RegionId {
  uint32_t v;

  constexpr bool operator==(const RegionId &) const noexcept = default;
  constexpr auto operator<=>(const RegionId &) const noexcept = default;
};

struct PortId {
  uint32_t v;

  constexpr bool operator==(const PortId &) const noexcept = default;
  constexpr auto operator<=>(const PortId &) const noexcept = default;
};

struct EdgeId {
  uint32_t v;

  constexpr bool operator==(const EdgeId &) const noexcept = default;
  constexpr auto operator<=>(const EdgeId &) const noexcept = default;
};

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

// ---- Render-ready row structs. All text fields are computed ONCE at Build
// (the freeze) and never re-derived at dump time.

// One program-root ABI line.
struct RegionalAbi {
  enum Kind { kInput, kQuery, kOutput } kind;
  std::string decl_text;
  std::string route_text;
};

// One region port line.
struct RegionalPort {
  enum Kind { kRequest, kInput, kResult } kind;
  unsigned port_index;

  // e.g. "query=reachable_from" or "message=edge_2/2" (+ an optional
  // "  adorn=bf" appended when the query name carries >= 2 adornments).
  std::string head_text;

  // e.g. "(From, To)".
  std::string fields_text;
};

// One region-internal line, e.g. "reachable_from_bf/1(c3:u64)" — an interior
// relation with no direct message binding.
struct RegionalInternal {
  std::string text;
};

// One permanent-root line, e.g. "q(B)".
struct RegionalPermanentRoot {
  std::string text;
};

// One row-contract line (Rule R-STORE, Stage-B narrowed form:
// insert-materialized relations only).
struct RegionalContract {
  unsigned edge_index;
  std::string rel_name;
  std::string member_key_text;  // "(From, To)"
  std::string support_text;     // "monotone" | "differential"

  // K6-7a: does the relation carry an `@key` instance-key pragma? Rendered as
  // a " declared-key" badge in the -region-dot-out DOT twin ONLY (never in the
  // -region-out TEXT emitter — the 16 `.region.<mode>` goldens stay
  // byte-identical). Default false.
  bool declared_key = false;
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

  const ::hyde::Query &Query(void) const;
  const RegionalCensus &Census(void) const;

  // Const-ref row accessors (the `-region-out` dump reads these).
  const std::vector<RegionalAbi> &Abis(void) const;
  const std::vector<RegionalPort> &Ports(void) const;
  const std::vector<RegionalInternal> &Internals(void) const;
  const std::vector<RegionalPermanentRoot> &PermanentRoots(void) const;
  const std::vector<RegionalContract> &Contracts(void) const;

 private:
  explicit FrozenRegionalProgram(const ::hyde::Query &query_);

  ::hyde::Query query;
  RegionalCensus census;
  std::vector<RegionalAbi> abis;
  std::vector<RegionalPort> ports;
  std::vector<RegionalInternal> internals;
  std::vector<RegionalPermanentRoot> permanent_roots;
  std::vector<RegionalContract> contracts;
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
