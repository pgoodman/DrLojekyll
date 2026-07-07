#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  auto dump = [&db]() {
    std::vector<int32_t> qs;
    auto qc = db.q_f();
    for (int32_t b = 0; qc.next(b);) {
      qs.push_back(b);
    }
    std::sort(qs.begin(), qs.end());
    std::cout << "q:";
    for (auto b : qs) {
      std::cout << ' ' << b;
    }
    std::cout << '\n';
  };

  // Round 1: some B values, but no X == 1 yet, so q must be empty.
  {
    hyde::rt::Vec<add_b_input> vb(allocator);
    vb.Add({10});
    vb.Add({20});
    db.add_b_1(std::move(vb));
    hyde::rt::Vec<add_x_input> vx(allocator);
    vx.Add({2});
    vx.Add({3});
    db.add_x_1(std::move(vx));
  }
  dump();

  // Round 2: X == 1 arrives, enabling the condition; plus one more B.
  {
    hyde::rt::Vec<add_x_input> vx(allocator);
    vx.Add({1});
    db.add_x_1(std::move(vx));
    hyde::rt::Vec<add_b_input> vb(allocator);
    vb.Add({30});
    db.add_b_1(std::move(vb));
  }
  dump();
  return 0;
}
