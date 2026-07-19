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

}  // namespace hyde
