// Copyright 2026, Peter Goodman. All rights reserved.

// Driver for barrier_neck_1: the `:-` strict-order clause separator. The
// strict clause `t`, its explicit-@barrier twin `tb`, and the mixed
// multi-body `s` must agree row-for-row on every batch (the separators
// change the join staging, never the answers).

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

  auto dump_one = [&db](const char *name, auto make_cursor) {
    std::vector<int32_t> rows;
    auto cursor = make_cursor(db);
    for (int32_t a = 0; cursor.next(a);) {
      rows.push_back(a);
    }
    std::sort(rows.begin(), rows.end());
    std::cout << name << ':';
    for (auto a : rows) {
      std::cout << ' ' << a;
    }
    std::cout << '\n';
  };

  auto dump = [&]() {
    dump_one("t", [](Database &d) { return t_all_f(d); });
    dump_one("tb", [](Database &d) { return tb_all_f(d); });
    dump_one("tp", [](Database &d) { return tp_all_f(d); });
    dump_one("s", [](Database &d) { return s_all_f(d); });
  };

  {
    hyde::rt::Vec<mp_input> vp(allocator);
    vp.Add({1, 10});
    vp.Add({2, 20});
    vp.Add({3, 30});
    mp_2(db, log, functors, std::move(vp));
    hyde::rt::Vec<mq_input> vq(allocator);
    vq.Add({10});
    vq.Add({20});
    mq_1(db, log, functors, std::move(vq));
    hyde::rt::Vec<mr_input> vr(allocator);
    vr.Add({20});
    vr.Add({30});
    mr_1(db, log, functors, std::move(vr));
  }
  std::cout << "batch1\n";
  dump();

  {
    hyde::rt::Vec<mq_input> vq(allocator);
    vq.Add({30});
    mq_1(db, log, functors, std::move(vq));
    hyde::rt::Vec<mr_input> vr(allocator);
    vr.Add({10});
    mr_1(db, log, functors, std::move(vr));
    hyde::rt::Vec<mp_input> vp(allocator);
    vp.Add({4, 10});
    mp_2(db, log, functors, std::move(vp));
  }
  std::cout << "batch2\n";
  dump();

  return 0;
}
