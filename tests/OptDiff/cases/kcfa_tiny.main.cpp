#include <cstdint>
#include <iostream>

#include "datalog.h"

// A tiny two-argument lambda-calculus program:
//   prog node 20 is app(20, f=10, a0=11, a1=11)
//   lam 10 = \(x=1, y=2). <body 21>, where node 21 is app(21, 30, 31, 31)
//   lam 11 = \(x=3, y=4). <body 32>
//   var 30 refers to 1, var 31 refers to 2, var 32 refers to 3.
// prog is sent first so that the time/lam cross product observes lam rows
// arriving after time rows. Round 2 adds the inner app node 21, extending
// the reachable control-flow graph.

namespace {

const int32_t kIds[] = {10, 11, 20, 21, 30, 31, 32};

void Dump(Database &db) {
  std::cout << "reaches_cfg:";
  for (int32_t id : kIds) {
    for (int32_t t0 : kIds) {
      for (int32_t t1 : kIds) {
        if (db.reaches_cfg_bbb(id, t0, t1)) {
          std::cout << " (" << id << ',' << t0 << ',' << t1 << ')';
        }
      }
    }
  }
  std::cout << '\n';
  std::cout << "reaches_clo:";
  for (int32_t id : kIds) {
    for (int32_t t0 : kIds) {
      for (int32_t t1 : kIds) {
        if (db.reaches_clo_bbb(id, t0, t1)) {
          std::cout << " (" << id << ',' << t0 << ',' << t1 << ')';
        }
      }
    }
  }
  std::cout << '\n';
}

}  // namespace

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator, log, functors);

  // Round 1.
  {
    hyde::rt::Vec<add_prog_input> progs(allocator);
    progs.Add({20});
    db.add_prog_1(std::move(progs));
  }
  {
    hyde::rt::Vec<add_lam_input> lams(allocator);
    lams.Add({10, 1, 2, 21});
    lams.Add({11, 3, 4, 32});
    db.add_lam_4(std::move(lams));
  }
  {
    hyde::rt::Vec<add_var_input> vars(allocator);
    vars.Add({30, 1});
    vars.Add({31, 2});
    vars.Add({32, 3});
    db.add_var_2(std::move(vars));
  }
  {
    hyde::rt::Vec<add_app_input> apps(allocator);
    apps.Add({20, 10, 11, 11});
    db.add_app_4(std::move(apps));
  }
  Dump(db);

  // Round 2: add the inner application node, extending reachability.
  {
    hyde::rt::Vec<add_app_input> apps(allocator);
    apps.Add({21, 30, 31, 31});
    db.add_app_4(std::move(apps));
  }
  Dump(db);
  return 0;
}
