#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  int step = 0;
  auto dump = [&](const char *label) {
    std::cout << "-- step " << ++step << ": " << label << '\n';
    {
      std::vector<int32_t> v;
      auto c = gated_f(db);
      for (int32_t x = 0; c.next(x);) {
        v.push_back(x);
      }
      std::sort(v.begin(), v.end());
      std::cout << "gated:";
      for (auto x : v) {
        std::cout << ' ' << x;
      }
      std::cout << '\n';
    }
    {
      std::vector<int32_t> v;
      auto c = ungated_f(db);
      for (int32_t x = 0; c.next(x);) {
        v.push_back(x);
      }
      std::sort(v.begin(), v.end());
      std::cout << "ungated:";
      for (auto x : v) {
        std::cout << ' ' << x;
      }
      std::cout << '\n';
    }
  };

  auto send_support = [&](std::vector<int32_t> add, std::vector<int32_t> rem) {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    for (auto s : add) {
      added.Add({s});
    }
    for (auto s : rem) {
      removed.Add({s});
    }
    support_1(db, log, functors, std::move(added), std::move(removed));
  };

  auto send_item = [&](std::vector<int32_t> add, std::vector<int32_t> rem) {
    hyde::rt::Vec<Tup_i32> added(allocator);
    hyde::rt::Vec<Tup_i32> removed(allocator);
    for (auto x : add) {
      added.Add({x});
    }
    for (auto x : rem) {
      removed.Add({x});
    }
    item_1(db, log, functors, std::move(added), std::move(removed));
  };

  dump("initial (gate closed, no data)");

  send_item({1, 2}, {});
  dump("items 1,2 while gate closed");

  send_support({10}, {});
  dump("flip 1 open: support 10 added");

  send_item({3}, {});
  dump("item 3 while gate open");

  send_item({}, {2});
  dump("item 2 retracted while gate open (gated row must go)");

  send_support({}, {10});
  dump("flip 1 close: support 10 retracted");

  send_item({4}, {});
  dump("item 4 while gate closed");

  send_item({}, {3});
  dump("item 3 retracted while gate closed (ungated row must go)");

  send_support({20}, {});
  dump("flip 2 open: support 20 added");

  send_item({5}, {1});
  dump("same-batch item swap while gate open: add 5, retract 1");

  send_support({}, {20});
  dump("flip 2 close: support 20 retracted");

  send_support({30, 40}, {});
  dump("flip 3 open: supports 30,40 added");

  send_support({}, {30});
  dump("partial retraction: support 30 dropped, 40 remains (gate stays open)");

  send_support({50}, {40});
  dump("same-batch swap: add 50, retract 40 (gate stays open)");

  send_support({}, {50});
  dump("flip 3 close: last support 50 retracted");

  send_support({10}, {});
  dump("flip 4 open: support 10 re-added");

  return 0;
}
