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

}  // namespace hyde
