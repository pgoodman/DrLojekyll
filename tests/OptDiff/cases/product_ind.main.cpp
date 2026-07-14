// F12 regression: @product straddling an inductive back-edge must derive the
// same fixpoint regardless of message arrival order. Each order runs against
// a fresh database; the driver prints every order's results and exits nonzero
// if any two orders disagree, so a regression fails under diffrun even though
// all four optimization modes emit the same code.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "datalog.h"

namespace {

struct Results {
  std::vector<int32_t> t;
  std::vector<std::pair<int32_t, int32_t>> p;

  bool operator==(const Results &) const = default;
};

// order 0: m1 then m2 (separate batches)
// order 1: m2 then m1 (separate batches)
// order 2: m1 then m2 element-by-element
Results RunOrder(int order) {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto send_m1 = [&](std::initializer_list<int32_t> vals) {
    hyde::rt::Vec<m1_input> rows(allocator);
    for (auto v : vals) rows.Add({v});
    m1_1(db, log, functors, std::move(rows));
  };
  auto send_m2 = [&](std::initializer_list<int32_t> vals) {
    hyde::rt::Vec<m2_input> rows(allocator);
    for (auto v : vals) rows.Add({v});
    m2_1(db, log, functors, std::move(rows));
  };

  switch (order) {
    case 0: send_m1({1}); send_m2({10, 20}); break;
    case 1: send_m2({10, 20}); send_m1({1}); break;
    case 2: send_m1({1}); send_m2({10}); send_m2({20}); break;
  }

  Results r;
  {
    auto cur = q_t_f(db);
    for (int32_t a = 0; cur.next(a);) r.t.push_back(a);
    std::sort(r.t.begin(), r.t.end());
  }
  {
    auto cur = q_p_ff(db);
    for (int32_t a = 0, b = 0; cur.next(a, b);) r.p.emplace_back(a, b);
    std::sort(r.p.begin(), r.p.end());
  }
  return r;
}

void Print(int order, const Results &r) {
  std::cout << "order " << order << " t:";
  for (auto a : r.t) std::cout << ' ' << a;
  std::cout << "\norder " << order << " p:";
  for (auto [a, b] : r.p) std::cout << " (" << a << ',' << b << ')';
  std::cout << '\n';
}

}  // namespace

int main() {
  const Results r0 = RunOrder(0);
  const Results r1 = RunOrder(1);
  const Results r2 = RunOrder(2);
  Print(0, r0);
  Print(1, r1);
  Print(2, r2);
  if (r0 != r1 || r0 != r2) {
    std::cout << "ORDER-DEPENDENT RESULTS\n";
    return 1;
  }
  return 0;
}
