#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump2 = [](const char *name, auto cursor) {
    std::vector<std::pair<int32_t, int32_t>> rows;
    for (int32_t a = 0, b = 0; cursor.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << name << ':';
    for (auto [a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';
  };
  auto dump = [&]() {
    dump2("j", q_j_ff(db));
    dump2("k", q_k_ff(db));
    dump2("xp", q_xp_ff(db));
  };

  dump();
  {
    hyde::rt::Vec<a2_input> rows(allocator);
    rows.Add({1, 100});
    a2_2(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<b2_input> rows(allocator);
    rows.Add({2, 100});
    rows.Add({3, 200});
    b2_2(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<c2_input> rows(allocator);
    rows.Add({100, 7});
    rows.Add({200, 8});
    c2_2(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<e1_input> rows(allocator);
    rows.Add({1});
    e1_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<f1_input> rows(allocator);
    rows.Add({10});
    f1_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<g1_input> rows(allocator);
    rows.Add({2});
    g1_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<h1_input> rows(allocator);
    rows.Add({20});
    h1_1(db, log, functors, std::move(rows));
  }
  dump();

  // Round 2: extend both join branches and both cross-products.
  {
    hyde::rt::Vec<c2_input> rows(allocator);
    rows.Add({300, 9});
    c2_2(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<a2_input> rows(allocator);
    rows.Add({4, 300});
    a2_2(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<e1_input> rows(allocator);
    rows.Add({5});
    e1_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<h1_input> rows(allocator);
    rows.Add({30});
    h1_1(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
