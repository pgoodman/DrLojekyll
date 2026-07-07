#include <algorithm>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

#include "datalog.h"

static void Dump(Database &db) {
  std::vector<std::tuple<int32_t, int32_t, int32_t>> rows;
  auto c = db.get_grouped_fff();
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
  Database db(allocator, log, functors);

  {
    hyde::rt::Vec<pair_input> vec(allocator);
    vec.Add({1, 10});
    vec.Add({1, 11});
    vec.Add({2, 20});
    db.pair_2(vec);
  }
  Dump(db);

  {
    hyde::rt::Vec<pair_input> vec(allocator);
    vec.Add({2, 21});
    vec.Add({3, 30});
    db.pair_2(vec);
  }
  Dump(db);
  return 0;
}
