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
      q_double_12(allocator_),
      p_single_15(allocator_),
      table_18(allocator_),
      idx_33(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec46(allocator);
  ::hyde::rt::Vec<Tup_i32> vec47(allocator);
  proc_22(std::move(vec46), std::move(vec47));
  return false;
}

bool Database::proc_22(::hyde::rt::Vec<Tup_i32> vec24, ::hyde::rt::Vec<Tup_i32> vec28) {
  ::hyde::rt::Vec<Tup_b> vec27(allocator);
  for (auto [v26] : vec24) {
    if (const auto ins0 = table_18.TryChangeAbsentToPresent({v26, true}); ins0.changed) {
      if (ins0.added_row) {
        idx_33.Add({true}, ins0.id);
      }
      vec27.Add({true});
    }
  }
  for (auto [v30] : vec28) {
    if (const auto ins1 = table_8.TryChangeAbsentToPresent({true, v30}); ins1.changed) {
      if (const auto ins2 = __5.TryChangeAbsentToPresent({true}); ins2.changed) {
        vec27.Add({true});
      }
    }
  }
  vec24.Clear();
  vec28.Clear();
  flow_48(std::move(vec27));
  return false;
}

bool Database::feed_r_1(::hyde::rt::Vec<Tup_i32> vec38) {
  ::hyde::rt::Vec<Tup_i32> vec40(allocator);
  proc_22(std::move(vec38), std::move(vec40));
  return true;
}

bool Database::set_c_1(::hyde::rt::Vec<Tup_i32> vec42) {
  ::hyde::rt::Vec<Tup_i32> vec44(allocator);
  proc_22(std::move(vec44), std::move(vec42));
  return true;
}

bool Database::flow_48(::hyde::rt::Vec<Tup_b> vec27) {
  g23 += 1;
  vec27.SortAndUnique();
  for (auto [v32] : vec27) {
    if (const uint32_t j31_0 = __5.Find({v32}); j31_0 != ::hyde::rt::kNoRow) {
      const auto r31_0 = __5.RowAt(j31_0);
      const auto v35 = r31_0.c0;
      for (uint32_t j31_1 = idx_33.First({v32}); j31_1 != ::hyde::rt::kNoRow; j31_1 = idx_33.Next(j31_1)) {
        const auto r31_1 = table_18.RowAt(j31_1);
        const auto v36 = r31_1.x;
        const auto v34 = r31_1.c1;
        if (v32 == v34 && v32 == v35) {
          q_double_12.TryChangeAbsentToPresent({v36});
          p_single_15.TryChangeAbsentToPresent({v36});
        }
      }
    }
  }
  vec27.Clear();
  return true;
}

Database::q_double_f_cursor Database::q_double_f() {
  return {*this, 0};
}

bool Database::q_double_f_cursor::next(int32_t &X) {
  while (pos < db.q_double_12.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.q_double_12.RowAt(id);
    if (db.q_double_12.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

Database::p_single_f_cursor Database::p_single_f() {
  return {*this, 0};
}

bool Database::p_single_f_cursor::next(int32_t &X) {
  while (pos < db.p_single_15.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.p_single_15.RowAt(id);
    if (db.p_single_15.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

