// Copyright 2026, Peter Goodman. All rights reserved.
//
// Expected-diagnostic case: under `-demand` (the .drflags sidecar) the
// compiler rejects demand_config_agg_body_1.dr with a demand-sink / R-MAT
// diagnostic in all 4 modes (an over(){} aggregate inside a demanded body;
// see runall.sh), so this driver is never compiled or run. It exists only to
// satisfy the suite's <name>.dr + <name>.main.cpp case layout.
#include <cstdint>
#include <iostream>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  (void) db;
  return 0;
}
