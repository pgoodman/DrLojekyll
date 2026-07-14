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
    std::vector<std::pair<int32_t, int32_t>> pairs;
    auto c = out_ff(db);
    for (int32_t a = 0, b = 0; c.next(a, b);) {
      pairs.emplace_back(a, b);
    }
    std::sort(pairs.begin(), pairs.end());
    std::cout << "out:";
    for (auto [a, b] : pairs) {
      std::cout << " (" << a << ',' << b << ')';
    }
    std::cout << '\n';

    std::vector<int32_t> vals;
    auto c2 = out2_f(db);
    for (int32_t v = 0; c2.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << "out2:";
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  dump();

  {
    hyde::rt::Vec<input_input> rows(allocator);
    rows.Add({4});
    rows.Add({-1});
    input_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<input2_input> rows(allocator);
    rows.Add({1, 100});
    rows.Add({2, 200});
    input2_2(db, log, functors, std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<input2_input> rows(allocator);
    rows.Add({1, 300});  // same key, different dropped column
    rows.Add({3, 400});
    input2_2(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
