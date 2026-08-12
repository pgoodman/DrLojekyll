// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2019, Trail of Bits. All rights reserved.

#pragma once

#include <drlojekyll/DataFlow/Query.h>

namespace hyde {

class OutputStream;

OutputStream &operator<<(OutputStream &os, Query query);

// The DataFlow BB-with-arguments text dump (the `-df-out` surface). A tag
// struct keeps this operator<< overload disjoint from the GraphViz DOT one
// above. Grammar and byte-contracts: t2-dump-spec.md §1 + pins (p1)-(p9);
// must be drained post-Program::Build so TableId() is populated.
struct QueryDF {
  Query query;
};

OutputStream &operator<<(OutputStream &os, QueryDF df);

// The Stage-A row-contract text dump (the `-contract-out` surface, hunk H-A8).
// A tag struct keeps this operator<< disjoint from the DOT and `.df` ones.
// One block per live view in the SAME det_seq order as the `.df` dump, so a
// reviewer reads the two side by side. Deterministic, pure byte-compare (no
// order-free field — permcheck N/A); OPT-MODE-only pinning (contracts are a
// post-Optimize graph property). The `.df` surface is byte-UNCHANGED by
// Stage A; contracts live only here.
struct QueryContracts {
  Query query;
};

OutputStream &operator<<(OutputStream &os, QueryContracts qc);

// The K5 Tier-2 origin-provenance text dump (the advisory `-origin-out`
// surface, belt (ii)). A tag struct keeps this operator<< disjoint from the
// DOT / `.df` / contract ones. One line per LIVE view carrying a nonempty
// origin decl-set; row order keys on `(min decl.Id() in the set, det_seq
// tie-break)` — a payload-covarying primary key that keeps same-provenance
// rows adjacent across compiler versions/modes, det_seq only the printed label
// + tie-break. Advisory, NEVER-GOLDENED-BY-DEFAULT (det_seq/min-Id ordering is
// per-mode divergent by design); a pure deterministic function of the frozen
// graph, null-safe when the sink is unset.
struct QueryOrigins {
  Query query;
};

OutputStream &operator<<(OutputStream &os, QueryOrigins qo);

// The InstanceFlow flat-grove text dump (the `-instanceflow-out` surface;
// docs/proposals/InstanceFlow.md §16). A tag struct keeps this operator<<
// disjoint from the DOT / `.df` / contract / origin ones. The maximally-shared
// empty-context grove of the FINAL (post-Optimize) graph — deterministic (ids
// from `det_seq` + canonical catalog order, never pointer/iteration order),
// pure byte-compare, OPT-MODE-only pinning (the grove is a post-Optimize graph
// property). Reads `impl->instance_flow`; an OBSERVER — codegen byte-unchanged.
struct QueryInstanceFlow {
  Query query;
};

OutputStream &operator<<(OutputStream &os, QueryInstanceFlow qif);

}  // namespace hyde
