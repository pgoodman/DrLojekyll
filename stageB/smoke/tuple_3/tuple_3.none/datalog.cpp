// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      __6(allocator_),
      table_9(allocator_),
      proof_13(allocator_),
      table_16(allocator_),
      idx_28(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec36(allocator);
  proc_20(std::move(vec36));
  return false;
}

bool Database::proc_20(::hyde::rt::Vec<Tup_i32> vec23) {
  ::hyde::rt::Vec<Tup_b> vec22(allocator);
  for (auto [v25] : vec23) {
    if (const auto ins0 = table_9.TryChangeAbsentToPresent({true, v25}); ins0.changed) {
      if (const auto ins1 = __6.TryChangeAbsentToPresent({true}); ins1.changed) {
        vec22.Add({true});
      }
    }
  }
  vec23.Clear();
  flow_37(std::move(vec22));
  return false;
}

bool Database::something_1(::hyde::rt::Vec<Tup_i32> vec33) {
  proc_20(std::move(vec33));
  return true;
}

bool Database::flow_37(::hyde::rt::Vec<Tup_b> vec22) {
  if ((g21 += 1) == 1) {
    if (const auto ins2 = table_16.TryChangeAbsentToPresent({1, true}); ins2.changed) {
      if (ins2.added_row) {
        idx_28.Add({true}, ins2.id);
      }
      vec22.Add({true});
    }
  }
  vec22.SortAndUnique();
  for (auto [v27] : vec22) {
    if (const uint32_t j26_0 = __6.Find({v27}); j26_0 != ::hyde::rt::kNoRow) {
      const auto r26_0 = __6.RowAt(j26_0);
      const auto v30 = r26_0.c0;
      for (uint32_t j26_1 = idx_28.First({v27}); j26_1 != ::hyde::rt::kNoRow; j26_1 = idx_28.Next(j26_1)) {
        const auto r26_1 = table_16.RowAt(j26_1);
        const auto v31 = r26_1.c0;
        const auto v29 = r26_1.c1;
        if (v27 == v29 && v27 == v30) {
          if (const auto ins3 = proof_13.TryChangeAbsentToPresent({v31}); ins3.changed) {
          }
        }
      }
    }
  }
  vec22.Clear();
  return true;
}

Database::proof_f_cursor Database::proof_f() {
  return {*this, 0};
}

bool Database::proof_f_cursor::next(int32_t &X) {
  while (pos < db.proof_13.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.proof_13.RowAt(id);
    if (db.proof_13.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.a;
    return true;
  }
  return false;
}

