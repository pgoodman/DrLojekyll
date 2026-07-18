// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2021, Trail of Bits. All rights reserved.

#include <DrTest.h>

#include <sys/time.h>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "FactPaths.h"
#include "points_to.h"  // Auto-generated.

namespace {

class Timed {
 private:
  const char *const timer;
  int64_t start;

 public:
  Timed(const char *timer_) : timer(timer_) {
    struct timeval x2_t;
    gettimeofday(&x2_t, NULL);
    start = x2_t.tv_sec * 1000000L + x2_t.tv_usec;
  }

  ~Timed(void) {
    struct timeval x2_t;
    gettimeofday(&x2_t, NULL);
    auto now = (x2_t.tv_sec * 1000000L) + x2_t.tv_usec;
    std::cerr << timer << ": " << (now - start) << "\n";
  }
};

}  // namespace

TEST(PointsTo, RunOnFacts) {
  const auto allocator = hyde::rt::MallocAllocator();

  points_to::DatabaseFunctors functors;
  points_to::DatabaseLog log;
  points_to::Database db(allocator);
  init(db, log, functors);  // ADL: hidden friends are unqualified-only.

  hyde::rt::Vec<points_to::assign_alloc_input> assign_alloc_facts(allocator);
  {
    Timed timer("Time to load AssignAlloc.facts");
    std::ifstream fs(kAssignAllocPath);
    for (std::string line; std::getline(fs, line);) {
      uint32_t var;
      uint32_t heap;
      if (2 == sscanf(line.c_str(), "%u\t%u", &var, &heap)) {
        assign_alloc_facts.Add({var, heap});
      }
    }
  }

  hyde::rt::Vec<points_to::load_input> load_facts(allocator);
  {
    Timed timer("Time to load Load.facts");
    std::ifstream fs(kLoadPath);
    for (std::string line; std::getline(fs, line);) {
      uint32_t base;
      uint32_t dest;
      uint32_t field;
      if (3 == sscanf(line.c_str(), "%u\t%u\t%u", &base, &dest, &field)) {
        load_facts.Add({base, dest, field});
      }
    }
  }

  hyde::rt::Vec<points_to::primitive_assign_input> primitive_assign_facts(
      allocator);
  {
    Timed timer("Time to load PrimitiveAssign.facts");
    std::ifstream fs(kPrimitiveAssignPath);
    for (std::string line; std::getline(fs, line);) {
      uint32_t source;
      uint32_t dest;
      if (2 == sscanf(line.c_str(), "%u\t%u", &source, &dest)) {
        primitive_assign_facts.Add({source, dest});
      }
    }
  }

  hyde::rt::Vec<points_to::store_input> store_facts(allocator);
  {
    Timed timer("Time to load Store.facts");
    std::ifstream fs(kStorePath);
    for (std::string line; std::getline(fs, line);) {
      uint32_t source;
      uint32_t base;
      uint32_t field;
      if (3 == sscanf(line.c_str(), "%u\t%u\t%u", &source, &base, &field)) {
        store_facts.Add({source, base, field});
      }
    }
  }

  {
    Timed timer("Time to apply all inputs");
    assign_alloc_2(db, log, functors, std::move(assign_alloc_facts));
    load_3(db, log, functors, std::move(load_facts));
    primitive_assign_2(db, log, functors, std::move(primitive_assign_facts));
    store_3(db, log, functors, std::move(store_facts));
  }

  size_t num_aliases = 0;
  {
    Timed timer("Time to write Alias.tsv");
    std::ofstream fs(kAliasPath);
    auto cursor = alias_ff(db);
    for (uint32_t x, y; cursor.next(x, y);) {
      fs << x << '\t' << y << '\n';
      ++num_aliases;
    }
  }

  size_t num_assigns = 0;
  {
    Timed timer("Time to write Assign.tsv");
    std::ofstream fs(kAssignPath);
    auto cursor = assign_ff(db);
    for (uint32_t source, dest; cursor.next(source, dest);) {
      fs << source << '\t' << dest << '\n';
      ++num_assigns;
    }
  }

  size_t num_points_to = 0;
  {
    Timed timer("Time to write VarPointsTo.tsv");
    std::ofstream fs(kVarPointsToPath);
    auto cursor = var_points_to_ff(db);
    for (uint32_t var, heap; cursor.next(var, heap);) {
      fs << var << '\t' << heap << '\n';
      ++num_points_to;
    }
  }

  // The relations must be non-trivial; the analysis derived something.
  ASSERT_TRUE(num_aliases > 0u);
  ASSERT_TRUE(num_assigns > 0u);
  ASSERT_TRUE(num_points_to > 0u);
}
