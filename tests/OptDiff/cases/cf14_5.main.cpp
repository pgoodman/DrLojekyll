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
    for (int32_t key : {1, 2, 3}) {
      {
        std::vector<int32_t> vs;
        auto c = db.getp_bf(key);
        for (int32_t b = 0; c.next(b);) {
          vs.push_back(b);
        }
        std::sort(vs.begin(), vs.end());
        std::cout << "getp(" << key << "):";
        for (auto v : vs) {
          std::cout << ' ' << v;
        }
        std::cout << '\n';
      }
      {
        std::vector<int32_t> vs;
        auto c = db.getq_bf(key);
        for (int32_t b = 0; c.next(b);) {
          vs.push_back(b);
        }
        std::sort(vs.begin(), vs.end());
        std::cout << "getq(" << key << "):";
        for (auto v : vs) {
          std::cout << ' ' << v;
        }
        std::cout << '\n';
      }
    }
    {
      std::vector<int32_t> vs;
      auto c = db.qa_f();
      for (int32_t a = 0; c.next(a);) {
        vs.push_back(a);
      }
      std::sort(vs.begin(), vs.end());
      std::cout << "qa:";
      for (auto v : vs) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
    {
      std::vector<int32_t> vs;
      auto c = db.qb_f();
      for (int32_t a = 0; c.next(a);) {
        vs.push_back(a);
      }
      std::sort(vs.begin(), vs.end());
      std::cout << "qb:";
      for (auto v : vs) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
  };

  {
    hyde::rt::Vec<in_input> v(allocator);
    v.Add({1, 10});
    v.Add({1, 11});
    v.Add({2, 20});
    db.in_2(std::move(v));
  }
  {
    hyde::rt::Vec<in2_input> v(allocator);
    v.Add({1, 2});
    v.Add({5, 3});
    v.Add({10, 20});
    v.Add({15, 4});
    db.in2_2(std::move(v));
  }
  dump();

  {
    hyde::rt::Vec<in_input> v(allocator);
    v.Add({2, 21});
    v.Add({3, 30});
    db.in_2(std::move(v));
  }
  {
    hyde::rt::Vec<in2_input> v(allocator);
    v.Add({9, 100});
    v.Add({-3, -5});
    db.in2_2(std::move(v));
  }
  dump();
  return 0;
}
