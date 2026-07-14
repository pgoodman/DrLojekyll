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
    std::vector<uint64_t> outs;
    auto c1 = out_f(db);
    for (uint64_t x = 0; c1.next(x);) {
      outs.push_back(x);
    }
    std::sort(outs.begin(), outs.end());
    std::cout << "out:";
    for (auto x : outs) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';

    std::vector<std::pair<uint64_t, uint64_t>> rows;
    auto c2 = out2_ff(db);
    for (uint64_t x = 0, y = 0; c2.next(x, y);) {
      rows.emplace_back(x, y);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << "out2:";
    for (auto &[x, y] : rows) {
      std::cout << " (" << x << ',' << y << ')';
    }
    std::cout << '\n';

    std::vector<int32_t> proofs;
    auto pc = proof_f(db);
    for (int32_t x = 0; pc.next(x);) {
      proofs.push_back(x);
    }
    std::sort(proofs.begin(), proofs.end());
    std::cout << "proof:";
    for (auto x : proofs) {
      std::cout << ' ' << x;
    }
    std::cout << '\n';
  };

  dump();  // empty everywhere

  {
    hyde::rt::Vec<in_input> rows(allocator);
    rows.Add({3, 5});    // passes X < Y
    rows.Add({7, 2});    // filtered out
    rows.Add({12, 20});  // passes
    in_2(db, log, functors, std::move(rows));
  }
  dump();

  {
    hyde::rt::Vec<m_input> rows(allocator);
    rows.Add({100});
    rows.Add({200});
    m_1(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<in_input> rows(allocator);
    rows.Add({5, 6});
    in_2(db, log, functors, std::move(rows));
  }
  {
    hyde::rt::Vec<something_input> rows(allocator);
    rows.Add({99});
    something_1(db, log, functors, std::move(rows));
  }
  dump();
  return 0;
}
