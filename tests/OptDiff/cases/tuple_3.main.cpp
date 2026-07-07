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
    std::vector<int32_t> vals;
    auto c = db.proof_f();
    for (int32_t v = 0; c.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << "proof:";
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  dump();  // condition not yet set: expect empty

  {
    hyde::rt::Vec<something_input> rows(allocator);
    rows.Add({42});
    db.something_1(std::move(rows));
  }
  dump();  // expect: proof: 1

  {
    hyde::rt::Vec<something_input> rows(allocator);
    rows.Add({43});
    db.something_1(std::move(rows));
  }
  dump();  // still just: proof: 1
  return 0;
}
