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
    {
      std::vector<int32_t> vs;
      auto c = db.out1_f();
      for (int32_t a = 0; c.next(a);) {
        vs.push_back(a);
      }
      std::sort(vs.begin(), vs.end());
      std::cout << "out1:";
      for (auto v : vs) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
    {
      std::vector<int32_t> vs;
      auto c = db.out2_f();
      for (int32_t a = 0; c.next(a);) {
        vs.push_back(a);
      }
      std::sort(vs.begin(), vs.end());
      std::cout << "out2:";
      for (auto v : vs) {
        std::cout << ' ' << v;
      }
      std::cout << '\n';
    }
  };

  {
    hyde::rt::Vec<base1_input> v(allocator);
    v.Add({1});
    db.base1_1(std::move(v));
  }
  {
    hyde::rt::Vec<base2_input> v(allocator);
    v.Add({10});
    db.base2_1(std::move(v));
  }
  {
    hyde::rt::Vec<base3_input> v(allocator);
    v.Add({100});
    db.base3_1(std::move(v));
  }
  {
    hyde::rt::Vec<edge_input> v(allocator);
    v.Add({1, 2});
    v.Add({2, 3});
    db.edge_2(std::move(v));
  }
  {
    hyde::rt::Vec<edge2_input> v(allocator);
    v.Add({100, 101});
    db.edge2_2(std::move(v));
  }
  dump();

  {
    hyde::rt::Vec<edge_input> v(allocator);
    v.Add({3, 4});
    v.Add({10, 11});
    db.edge_2(std::move(v));
  }
  {
    hyde::rt::Vec<base3_input> v(allocator);
    v.Add({200});
    db.base3_1(std::move(v));
  }
  {
    hyde::rt::Vec<edge2_input> v(allocator);
    v.Add({101, 102});
    v.Add({200, 201});
    db.edge2_2(std::move(v));
  }
  dump();
  return 0;
}
