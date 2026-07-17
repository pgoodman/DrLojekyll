// Copyright 2021, Trail of Bits. All rights reserved.
//
// THE LIVE DEMAND TRANSFORM (magic-sets / SLDMagic as a Query-graph rewrite).
//
// Design of record:
//   docs/proposals/DemandSeeds.artifacts/d1-demand-seed-mechanism.md (A0-A7)
//   docs/proposals/SubgraphsDemand.artifacts/p3-demand-argument.md   (§1-§2)
//
// ALGORITHM (single-adornment slice; multi-adornment fabrication is paper-
// only — a second distinct adornment cleanly rejects under `-demand`):
//
//   For each bound `#query p(bound.., free..)` under the `-demand` flag:
//     1. Read the query's own adornment `α` (the bound-column set). If
//        |bound(α)| == 0 (all-free), the query is DEMAND-INERT — skip it.
//     2. Left-to-right sideways-information-passing (SIP) walk over the
//        rule bodies of each predicate reachable from `p`, propagating
//        which columns are bound. Demand never crosses a NEGATE/AGG frontier
//        (the demand sink); if `p`'s bound columns are only reachable through
//        one, emit a clean diagnostic and reject.
//     3. Discovering a second distinct adornment of any predicate -> clean
//        diagnostic ("multi-adornment demand not yet supported"), never a
//        second fabricated message.
//     4. FABRICATE a real demand message `__demand_<q>_<α>` (Option D'): a
//        `ParsedMessageImpl` registered in the module (inside the mode gate),
//        arity = |bound(α)|, params reusing `p`'s real bound-column type
//        tokens, name interned as a real display buffer so codegen's
//        `SpellingRange()` naming path resolves (A7/G1).
//     5. Mint the DEMAND RELATION `d_p^α` fed by (i) the fabricated message
//        receive (the root seed) and (ii) the demanding-subgoal projections.
//     6. Mint the DEMAND-GUARDED COPY `p'^α = p ⋈ d_p^α` on the |bound(α)|
//        pivots — the N-pivot generalization of `ApplyPositiveConditionTest`
//        (a REAL column edge, never `group_ids`; E-32). Structural
//        distinctness keeps CSE from collapsing `p'` onto `p`.
//     7. Tripwire (obligation (f), always-on under the flag): every minted
//        `d_p^α` has >= 1 producing source; else fprintf+abort.
//
// BEFORE (a bound query reads the FULL relation, scan-indexed on the bound
// columns — every instance of `p` materializes):
//
//     edge --> path (full closure) --> reachable_from (scan-index on From)
//
// AFTER (demand-ON): the caller's bound tuple seeds `d_p^α` via a fabricated
// message injector; `p'^α` materializes only the demanded frontier; the full
// unguarded `p` is dead-flow-eliminated when nothing else reads it:
//
//     __demand_reachable_from_bf (msg) --> d_path[From] --.
//                                                          \
//                edge -->  path' = path-body ⋈ d_path (pivot: From)
//                                     |
//                            reachable_from (only demanded rows)
//
// The whole body runs ONLY when `demand_mode` is true. With it false the
// function returns immediately having minted nothing and mutated no module
// state — the 166-case golden net is byte-for-bit identical (the hard
// containment gate). See d1 §6.1 / A2 for the off-mode byte-identity argument.

#include "Query.h"

#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Parse/Parse.h>

namespace hyde {

bool QueryImpl::ApplyDemandTransform(const ParsedModule &module,
                                     const ErrorLog &log, bool demand_mode) {

  // MODE GATE. When the `-demand` flag is off (the default), this pass is a
  // total no-op: nothing is minted, no module state is mutated, the id-stream
  // is untouched, and the QueryImpl graph is byte-identical to today. This is
  // the hard containment gate for the existing corpus.
  if (!demand_mode) {
    return true;
  }

  // G2: `Query::Build` is at-most-once per module instance. The fabrication
  // mutates the shared module; a re-entry would fabricate stale demand decls.
  // Detect it here and hard-fail rather than silently corrupt.
  if (module.DemandMessagesFabricated()) {
    log.Append(module.SpellingRange())
        << "Internal error: the demand transform was re-entered on a module "
        << "that already carries fabricated demand messages (Query::Build is "
        << "at-most-once per module instance)";
    return false;
  }

  // The live graph rewrite (the guarded-copy splice, demand-relation source
  // minting, and demand propagation) is not yet landed. The fabrication
  // machinery (`ParsedModule::FabricateDemandMessage`, DemandSeeds Option D'
  // A1/A7) and the mode gate are in place; the QueryImpl-level transform is
  // the remaining work. Until it lands, `-demand` fails CLEANLY rather than
  // silently producing the same (undemanded) output as `-demand`-off.
  log.Append(module.SpellingRange())
      << "The live demand transform (-demand) is not yet implemented; "
      << "recompile without -demand";
  return false;
}

}  // namespace hyde
