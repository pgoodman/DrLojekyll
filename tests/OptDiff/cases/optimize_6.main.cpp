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

  auto dump = [&db]() {
    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c = out_ff(db);
    for (uint64_t a = 0, b = 0; c.next(a, b);) {
      rows.emplace_back(a, b);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "out:";
    for (auto &[a, b] : rows) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';

    std::vector<int32_t> dead;
    auto dc = dead_out_f(db);
    for (int32_t x = 0; dc.next(x);) {
      dead.push_back(x);
    }
    std::sort(dead.begin(), dead.end());
    std::cout << "dead:";
    for (auto x : dead) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  };

  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({1, 2});
    add.Add({2, 3});
    add.Add({4, 5});
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
  }
  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({1});
    rows.Add({2});
    input_1(db, log, functors, std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    add.Add({7, 8});
    rem.Add({2, 3});  // delete an earlier edge
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
  }
  dump();

  {
    hyde::rt::Vec<Tup_u64_u64> add(allocator);
    hyde::rt::Vec<Tup_u64_u64> rem(allocator);
    rem.Add({4, 5});
    rem.Add({7, 8});
    add_edge_2(db, log, functors, std::move(add), std::move(rem));
  }
  dump();
  return 0;
}
