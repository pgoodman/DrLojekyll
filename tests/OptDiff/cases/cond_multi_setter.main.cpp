#include <cstdint>
#include <iostream>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto dump = [&db](const char *label) {
    std::cout << label << " gated:";
    for (int32_t x = 1; x <= 5; ++x) {
      if (gated_b(db, x)) {
        std::cout << ' ' << x;
      }
    }
    std::cout << '\n';
  };

  dump("start");

  // Downstream data first; the condition is not yet set.
  {
    hyde::rt::Vec<add_item_input> items(allocator);
    items.Add({1});
    items.Add({2});
    items.Add({3});
    add_item_1(db, log, functors, std::move(items));
  }
  dump("items-only");

  // Support the condition via setter clause `c : a(_).`
  {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    added.Add({10});
    add_a_1(db, log, functors, std::move(added), std::move(removed));
  }
  dump("a-set");

  // Add support via the second setter clause `c : b(_).`
  {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    added.Add({20});
    add_b_1(db, log, functors, std::move(added), std::move(removed));
  }
  dump("a+b-set");

  // Retract a's support. b still supports c: gated must be unchanged.
  {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    removed.Add({10});
    add_a_1(db, log, functors, std::move(added), std::move(removed));
  }
  dump("a-retracted");

  // Retract b's support too. c flips false: gated must be empty.
  {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    removed.Add({20});
    add_b_1(db, log, functors, std::move(added), std::move(removed));
  }
  dump("b-retracted");

  // Reopen the gate through the other setter: gated must re-derive.
  {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    added.Add({30});
    add_a_1(db, log, functors, std::move(added), std::move(removed));
  }
  dump("a-reset");

  return 0;
}
