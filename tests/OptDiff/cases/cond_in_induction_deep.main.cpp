#include <cstdint>
#include <iostream>

#include "datalog.h"

// Condition tested in a deeper induction than the outer cycle that sets it.
// Three arrival orders of the gate relative to the inner induction's data:
//   A) inner-edge data arrives while the gate is closed (derivations stall);
//   B) the gate-opening edge1 tuple arrives in the same batch as fresh
//      outer-cycle data (the flip must unleash all stalled inner work);
//   C) inner-edge data arrives after the gate is already open.

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  // out is a bound query; probe fixed id ranges.
  auto dump = [&db]() {
    std::cout << "out:";
    for (int32_t a = 10; a <= 35; ++a) {
      if (db.out_b(a)) {
        std::cout << ' ' << a;
      }
    }
    for (int32_t a = 100; a <= 105; ++a) {
      if (db.out_b(a)) {
        std::cout << ' ' << a;
      }
    }
    std::cout << '\n';
  };

  dump();  // Nothing yet.

  // Phase A: marker first; the gate stays closed because 103 is not yet
  // reachable in the outer cycle.
  {
    hyde::rt::Vec<marker_input> v(allocator);
    v.Add({103});
    db.marker_1(std::move(v));
  }
  dump();
  {
    hyde::rt::Vec<base_input> v(allocator);
    v.Add({10});
    v.Add({101});
    db.base_1(std::move(v));
  }
  dump();  // reach2 copies reach1: 10 101
  {
    hyde::rt::Vec<edge1_input> v(allocator);
    v.Add({10, 11});
    v.Add({101, 102});
    db.edge1_2(std::move(v));
  }
  dump();  // outer cycle grows: 10 11 101 102
  {
    hyde::rt::Vec<edge2_input> v(allocator);
    v.Add({11, 12});
    v.Add({12, 13});
    v.Add({20, 21});
    v.Add({21, 22});
    v.Add({22, 23});
    v.Add({23, 21});  // inner cycle
    v.Add({14, 15});
    db.edge2_2(std::move(v));
  }
  dump();  // gate closed: no inner derivations appear

  // Phase B: fresh outer seed, then a single edge1 batch that both opens the
  // gate (102 -> 103 reaches the marker) and grows the outer cycle (11 -> 14).
  {
    hyde::rt::Vec<base_input> v(allocator);
    v.Add({20});
    db.base_1(std::move(v));
  }
  dump();  // 20 appears via the copy rule; 21..23 still stalled
  {
    hyde::rt::Vec<edge1_input> v(allocator);
    v.Add({102, 103});
    v.Add({11, 14});
    db.edge1_2(std::move(v));
  }
  dump();  // gate opens: stalled inner work unleashes (12 13 15 21 22 23)

  // Phase C: data arriving after the gate is open flows straight through.
  {
    hyde::rt::Vec<base_input> v(allocator);
    v.Add({30});
    db.base_1(std::move(v));
  }
  dump();
  {
    hyde::rt::Vec<edge2_input> v(allocator);
    v.Add({30, 31});
    v.Add({31, 32});
    db.edge2_2(std::move(v));
  }
  dump();  // 31 32 derived immediately under the open gate
  return 0;
}
