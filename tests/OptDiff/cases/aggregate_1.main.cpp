// Driver for aggregate_1: a count_i32 aggregate with a constant group-by
// column. count_i32 carries NO declared algebra, so it defaults to @recompute
// (spec v3-spec-statecell.md §C-2), whose reduction body is the C-5 driver-
// supplied free function `count_i32_reduce` (the from-scratch per-group rescan
// over the live multiset). ADL hidden-friend surface; the all-free cursor
// drain is SORTED before printing (cursor contract, CLAUDE.md).
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

#include "datalog.h"

// C-5 reduction free function for @recompute count_i32: the number of live
// members of the group (each surviving row contributes its live count).
int32_t count_i32_reduce(const int32_t * /*values*/, const int32_t *counts,
                         std::size_t n) {
  int32_t total = 0;
  for (std::size_t i = 0; i < n; ++i) {
    if (counts[i] > 0) {
      total += counts[i];
    }
  }
  return total;
}

static void Dump(Database &db) {
  std::vector<std::tuple<int32_t, int32_t, int32_t>> rows;
  auto c = get_grouped_fff(db);
  for (int32_t a, b, n; c.next(a, b, n);) {
    rows.emplace_back(a, b, n);
  }
  std::sort(rows.begin(), rows.end());
  for (const auto &[a, b, n] : rows) {
    std::cout << a << " " << b << " " << n << "\n";
  }
  std::cout << "--\n";
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  {
    hyde::rt::Vec<pair_input> vec(allocator);
    vec.Add({1, 10});
    vec.Add({1, 11});
    vec.Add({2, 20});
    pair_2(db, log, functors, std::move(vec));
  }
  Dump(db);

  {
    hyde::rt::Vec<pair_input> vec(allocator);
    vec.Add({2, 21});
    vec.Add({3, 30});
    pair_2(db, log, functors, std::move(vec));
  }
  Dump(db);
  return 0;
}
