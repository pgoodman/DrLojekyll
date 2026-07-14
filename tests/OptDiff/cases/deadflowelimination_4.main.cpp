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

  auto dump = [&db]() {
    auto show = [](const char *name, std::vector<int32_t> &v) {
      std::sort(v.begin(), v.end());
      std::cout << name << ':';
      for (auto x : v) {
        std::cout << ' ' << x;
      }
      std::cout << '\n';
    };
    std::vector<int32_t> pos, neg, chain;
    {
      auto c = out_pos_f(db);
      for (int32_t v = 0; c.next(v);) {
        pos.push_back(v);
      }
    }
    {
      auto c = out_neg_f(db);
      for (int32_t v = 0; c.next(v);) {
        neg.push_back(v);
      }
    }
    {
      auto c = out_chain_f(db);
      for (int32_t v = 0; c.next(v);) {
        chain.push_back(v);
      }
    }
    show("out_pos", pos);
    show("out_neg", neg);
    show("out_chain", chain);
  };

  dump();
  {
    hyde::rt::Vec<in_input> vec(allocator);
    vec.Add({1});
    vec.Add({2});
    in_1(db, log, functors, std::move(vec));
  }
  dump();
  {
    hyde::rt::Vec<in_input> vec(allocator);
    vec.Add({3});
    in_1(db, log, functors, std::move(vec));
  }
  dump();
  return 0;
}
