// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      __5(allocator_),
      table_8(allocator_),
      out_12(allocator_),
      table_15(allocator_),
      idx_27(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec35(allocator);
  proc_19(std::move(vec35));
  return false;
}

bool Database::proc_19(::hyde::rt::Vec<Tup_i32> vec21) {
  ::hyde::rt::Vec<Tup_b> vec24(allocator);
  for (auto [v23] : vec21) {
    if (const auto ins0 = table_8.TryChangeAbsentToPresent({true, v23}); ins0.changed) {
      if (const auto ins1 = __5.TryChangeAbsentToPresent({true}); ins1.changed) {
        vec24.Add({true});
      }
    }
    if (const auto ins2 = table_15.TryChangeAbsentToPresent({v23, true}); ins2.changed) {
      if (ins2.added_row) {
        idx_27.Add({true}, ins2.id);
      }
      vec24.Add({true});
    }
  }
  vec21.Clear();
  flow_36(std::move(vec24));
  return false;
}

bool Database::ping_1(::hyde::rt::Vec<Tup_i32> vec32) {
  proc_19(std::move(vec32));
  return true;
}

bool Database::flow_36(::hyde::rt::Vec<Tup_b> vec24) {
  g20 += 1;
  vec24.SortAndUnique();
  for (auto [v26] : vec24) {
    if (const uint32_t j25_0 = __5.Find({v26}); j25_0 != ::hyde::rt::kNoRow) {
      const auto r25_0 = __5.RowAt(j25_0);
      const auto v29 = r25_0.c0;
      for (uint32_t j25_1 = idx_27.First({v26}); j25_1 != ::hyde::rt::kNoRow; j25_1 = idx_27.Next(j25_1)) {
        const auto r25_1 = table_15.RowAt(j25_1);
        const auto v30 = r25_1.x;
        const auto v28 = r25_1.c1;
        if (v26 == v28 && v26 == v29) {
          out_12.TryChangeAbsentToPresent({v30});
        }
      }
    }
  }
  vec24.Clear();
  return true;
}

Database::out_f_cursor Database::out_f() {
  return {*this, 0};
}

bool Database::out_f_cursor::next(int32_t &X) {
  while (pos < db.out_12.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out_12.RowAt(id);
    if (db.out_12.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

