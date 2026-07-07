// Self-checking driver: removing one union branch's support for a tuple must
// not delete it while the sibling branch still supports it. Exits nonzero on
// a wrong result set, so every optimization mode is checked for correctness,
// not just cross-mode agreement.
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

  auto ok = true;
  auto check = [&db, &ok](const char *label, std::vector<uint64_t> expect) {
    std::vector<uint64_t> xs;
    auto c = db.out_f();
    for (uint64_t x = 0; c.next(x);) {
      xs.push_back(x);
    }
    std::sort(xs.begin(), xs.end());
    std::cout << label << ':';
    for (auto x : xs) {
      std::cout << ' ' << x;
    }
    if (xs != expect) {
      std::cout << "  WRONG, expected:";
      for (auto x : expect) {
        std::cout << ' ' << x;
      }
      ok = false;
    }
    std::cout << '\n';
  };

  auto send_a = [&](std::vector<uint64_t> adds, std::vector<uint64_t> rems) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto x : adds) add.Add({x});
    for (auto x : rems) rem.Add({x});
    db.a_1(std::move(add), std::move(rem));
  };
  auto send_b = [&](std::vector<uint64_t> adds, std::vector<uint64_t> rems) {
    hyde::rt::Vec<Tup_u64> add(allocator);
    hyde::rt::Vec<Tup_u64> rem(allocator);
    for (auto x : adds) add.Add({x});
    for (auto x : rems) rem.Add({x});
    db.b_1(std::move(add), std::move(rem));
  };

  send_a({1, 2}, {});
  send_b({1, 3}, {});
  check("after a{1,2} b{1,3}", {1, 2, 3});

  send_a({}, {1});  // 1 still supported by b.
  check("after rem a(1)", {1, 2, 3});

  send_b({}, {1});  // Last support for 1 gone.
  check("after rem b(1)", {2, 3});

  send_a({1}, {});  // Re-add through a.
  check("after re-add a(1)", {1, 2, 3});

  send_a({}, {2});
  check("after rem a(2)", {1, 3});

  return ok ? 0 : 1;
}
