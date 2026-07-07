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
  Database db(allocator, log, functors);

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
    dump2("j", db.q_j_ff());
    dump2("k", db.q_k_ff());
    dump2("xp", db.q_xp_ff());
  };

  dump();
  {
    hyde::rt::Vec<a2_input> rows(allocator);
    rows.Add({1, 100});
    db.a2_2(std::move(rows));
  }
  {
    hyde::rt::Vec<b2_input> rows(allocator);
    rows.Add({2, 100});
    rows.Add({3, 200});
    db.b2_2(std::move(rows));
  }
  {
    hyde::rt::Vec<c2_input> rows(allocator);
    rows.Add({100, 7});
    rows.Add({200, 8});
    db.c2_2(std::move(rows));
  }
  {
    hyde::rt::Vec<e1_input> rows(allocator);
    rows.Add({1});
    db.e1_1(std::move(rows));
  }
  {
    hyde::rt::Vec<f1_input> rows(allocator);
    rows.Add({10});
    db.f1_1(std::move(rows));
  }
  {
    hyde::rt::Vec<g1_input> rows(allocator);
    rows.Add({2});
    db.g1_1(std::move(rows));
  }
  {
    hyde::rt::Vec<h1_input> rows(allocator);
    rows.Add({20});
    db.h1_1(std::move(rows));
  }
  dump();

  // Round 2: extend both join branches and both cross-products.
  {
    hyde::rt::Vec<c2_input> rows(allocator);
    rows.Add({300, 9});
    db.c2_2(std::move(rows));
  }
  {
    hyde::rt::Vec<a2_input> rows(allocator);
    rows.Add({4, 300});
    db.a2_2(std::move(rows));
  }
  {
    hyde::rt::Vec<e1_input> rows(allocator);
    rows.Add({5});
    db.e1_1(std::move(rows));
  }
  {
    hyde::rt::Vec<h1_input> rows(allocator);
    rows.Add({30});
    db.h1_1(std::move(rows));
  }
  dump();
  return 0;
}
