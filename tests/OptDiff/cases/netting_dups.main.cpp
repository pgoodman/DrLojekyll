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

  auto dump = [&db](const char *label) {
    std::vector<int32_t> vals;
    auto c = db.out_f();
    for (int32_t v = 0; c.next(v);) {
      vals.push_back(v);
    }
    std::sort(vals.begin(), vals.end());
    std::cout << label << ": out:";
    for (auto v : vals) {
      std::cout << ' ' << v;
    }
    std::cout << '\n';
  };

  auto send = [&](std::vector<int32_t> add, std::vector<int32_t> rem) {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    for (auto v : add) {
      added.Add({v});
    }
    for (auto v : rem) {
      removed.Add({v});
    }
    db.m_1(std::move(added), std::move(removed));
  };

  auto send_other = [&](std::vector<int32_t> add, std::vector<int32_t> rem) {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    for (auto v : add) {
      added.Add({v});
    }
    for (auto v : rem) {
      removed.Add({v});
    }
    db.other_1(std::move(added), std::move(removed));
  };

  // Control: a genuine add lands.
  send({1}, {});
  dump("add 1");

  // Duplicate retractions cannot outvote an add of an ABSENT row: the
  // adds/removes intersection {2} annihilates and the surplus -2 is a
  // dedup'd no-op. Arithmetic netting would have produced a removal.
  send({2}, {2, 2});
  dump("+2 -2 -2 (absent)");

  // Duplicate adds cannot outvote a retraction of an ABSENT row: {3}
  // annihilates; arithmetic netting would have produced an addition.
  send({3, 3}, {3});
  dump("+3 +3 -3 (absent)");

  // Same two shapes against a PRESENT row: presence is unchanged.
  send({1}, {1, 1});
  dump("+1 -1 -1 (present)");
  send({1, 1}, {1});
  dump("+1 +1 -1 (present)");

  // Control: a genuine retraction lands.
  send({}, {1});
  dump("remove 1");

  // The plain annihilating pair on an absent row: no phantom presence.
  send({4}, {4});
  dump("+4 -4 (absent)");

  // Control: the same row genuinely added afterwards.
  send({4}, {});
  dump("add 4");

  // The owner's framing, second clause: an annihilating m-pair has no
  // effect through the explicit channel, so the row keeps exactly the
  // presence the ALTERNATE derivation proves. other(5) holds s(5) up
  // through an annihilating m-pair...
  send_other({5}, {});
  dump("other 5");
  send({5}, {5});
  dump("+5 -5 via m (other 5 holds)");

  // ...and annihilated m-pairs leave NO explicit residue: after another
  // m-pair epoch, retracting the alternate support must take 5 down with
  // it (a wrong implementation that turned the pair into -then-+ explicit
  // ops would have left a surviving kExplicit bit here).
  send({5}, {5});
  send_other({}, {5});
  dump("+5 -5 via m, then -other 5");
  return 0;
}
