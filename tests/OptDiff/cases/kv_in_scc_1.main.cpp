// Expected-diagnostic case: the compiler rejects kv_in_scc_1.dr with an
// unstratified-aggregation error in all 4 modes (see runall.sh), so this
// driver is never compiled or run. It exists only to satisfy the suite's
// <name>.dr + <name>.main.cpp case layout.
#include <cstdint>
#include <iostream>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);
  (void) db;
  return 0;
}
