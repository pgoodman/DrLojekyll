// Copyright 2026, Peter Goodman. All rights reserved.
//
// The cross-IR pass-harness policy (stage P1 of
// docs/proposals/KeyedInstances.artifacts/pass-harness-design.md, as pinned
// by the P1 judge round — ledger §17). One CLI-parsed object threaded through
// `Query::Build` and `Program::Build`; each registered OPTIONAL pass site
// asks `Gate(name)` once per application. REQUIRED stages (build steps,
// stratification, DeltaRel derivation/validation, codegen) never consult it.
//
// Byte-identity contract: an EMPTY policy is exactly today's default
// pipeline, and the legacy mode flags are EXACT aliases —
//   -disable-dataflow-opt   == -opt-disable=df.cse,df.canon,df.dfe,df.sink
//                              (NOT df.* — df.simplify and df.demand run
//                              OUTSIDE the optimize guard in all 4 modes)
//   -disable-controlflow-opt == -opt-disable=cf.*
// The wholesale-skip branches at the three Optimize() call sites are
// PRESERVED via AnyBodyOptionalEnabled(): entering the Optimize() bodies
// with every pass gated is NOT byte-equivalent to never entering them (the
// region-list `.Sort(depth_cmp)` calls inside the ControlFlow sweep loop are
// emission-visible).
//
// PURITY (the E-72 lesson): the allow path is side-effect-free beyond the
// diagnostic bisect index — no NDEBUG-conditional logic anywhere in here,
// no eager description formatting, nothing an ordering decision or a dump
// can read.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hyde {

enum class PassLevel { kDataFlow, kControlFlow };

class PassPolicy {
 public:
  // Empty policy == run everything == today's default pipeline.
  std::vector<std::string> disabled_globs;  // -opt-disable
  std::vector<std::string> only_globs;      // -opt-only (empty => all)

  // -opt-bisect-limit: -2 unset (no limit, no print), -1 run-all-and-print,
  // >= 0 skip applications with index > N.
  int64_t bisect_limit{-2};

  // The single monotone cross-level application index (DataFlow +
  // ControlFlow share it, ticked in program order — deterministic because
  // the compiler is single-threaded and, post-(F), iteration is id-ordered).
  // Reset per compiled module.
  mutable uint64_t bisect_counter{0};

  // Pure glob-filter predicate: -opt-only selects (empty = all), then
  // -opt-disable subtracts and WINS on conflict. No counter tick.
  bool Enabled(std::string_view name) const;

  // The wholesale-skip predicate: does this policy leave at least one of the
  // level's BODY-RESIDENT optional passes enabled? (df body = cse, canon,
  // dfe, sink; cf body = regionopt, procdedup. df.simplify / df.demand are
  // NOT body-resident.)
  bool AnyBodyOptionalEnabled(PassLevel level) const;

  // The one call per gateable application. Filtered-out passes return false
  // WITHOUT ticking the bisect counter (index space stays stable within a
  // config); enabled applications tick, then honor the bisect limit.
  bool Gate(std::string_view name, std::string_view desc = {}) const;

  // The exact legacy aliases (the P1 pinned contract §1).
  static PassPolicy DisableDataFlowOpt(void);
  static PassPolicy DisableControlFlowOpt(void);

  // A parse-time validity check for one CLI glob (prefix-star + exact only;
  // no '?', no bracket classes, no mid-string '*'). Returns false on a
  // malformed glob so the driver can emit a clean diagnostic.
  static bool IsValidGlob(std::string_view glob);

  // Parse-time reachability check: does the glob match at least one
  // REGISTERED pass name? A shape-valid but unmatchable glob (a typo, a
  // bare 'df', a trailing-dot 'df.') must be a clean diagnostic, never a
  // silent no-op — and under -opt-only a silent no-match would invert to
  // "disable everything" (the Fable review's catch).
  static bool MatchesAnyKnownPass(std::string_view glob);
};

}  // namespace hyde
