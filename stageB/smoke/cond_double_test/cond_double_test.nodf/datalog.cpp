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
      idx_44(allocator_),
      table_22(allocator_),
      idx_38(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec57(allocator);
  ::hyde::rt::Vec<Tup_i32> vec58(allocator);
  proc_26(std::move(vec57), std::move(vec58));
  return false;
}

bool Database::proc_26(::hyde::rt::Vec<Tup_i32> vec28, ::hyde::rt::Vec<Tup_i32> vec33) {
  ::hyde::rt::Vec<Tup_b> vec31(allocator);
  ::hyde::rt::Vec<Tup_b> vec32(allocator);
  for (auto [v30] : vec28) {
    if (const auto ins0 = table_18.TryChangeAbsentToPresent({v30, true}); ins0.changed) {
      if (ins0.added_row) {
        idx_44.Add({true}, ins0.id);
      }
      vec31.Add({true});
    }
    if (const auto ins1 = table_22.TryChangeAbsentToPresent({v30, true}); ins1.changed) {
      if (ins1.added_row) {
        idx_38.Add({true}, ins1.id);
      }
      vec32.Add({true});
    }
  }
  for (auto [v35] : vec33) {
    if (const auto ins2 = table_8.TryChangeAbsentToPresent({true, v35}); ins2.changed) {
      if (const auto ins3 = __5.TryChangeAbsentToPresent({true}); ins3.changed) {
        vec31.Add({true});
        vec32.Add({true});
      }
    }
  }
  vec28.Clear();
  vec33.Clear();
  flow_59(std::move(vec31), std::move(vec32));
  return false;
}

bool Database::feed_r_1(::hyde::rt::Vec<Tup_i32> vec49) {
  ::hyde::rt::Vec<Tup_i32> vec51(allocator);
  proc_26(std::move(vec49), std::move(vec51));
  return true;
}

bool Database::set_c_1(::hyde::rt::Vec<Tup_i32> vec53) {
  ::hyde::rt::Vec<Tup_i32> vec55(allocator);
  proc_26(std::move(vec55), std::move(vec53));
  return true;
}

bool Database::flow_59(::hyde::rt::Vec<Tup_b> vec31, ::hyde::rt::Vec<Tup_b> vec32) {
  g27 += 1;
  vec32.SortAndUnique();
  for (auto [v37] : vec32) {
    if (const uint32_t j36_0 = __5.Find({v37}); j36_0 != ::hyde::rt::kNoRow) {
      const auto r36_0 = __5.RowAt(j36_0);
      const auto v40 = r36_0.c0;
      for (uint32_t j36_1 = idx_38.First({v37}); j36_1 != ::hyde::rt::kNoRow; j36_1 = idx_38.Next(j36_1)) {
        const auto r36_1 = table_22.RowAt(j36_1);
        const auto v41 = r36_1.x;
        const auto v39 = r36_1.c1;
        if (v37 == v39 && v37 == v40) {
          p_single_15.TryChangeAbsentToPresent({v41});
        }
      }
    }
  }
  vec32.Clear();
  vec31.SortAndUnique();
  for (auto [v43] : vec31) {
    if (const uint32_t j42_0 = __5.Find({v43}); j42_0 != ::hyde::rt::kNoRow) {
      const auto r42_0 = __5.RowAt(j42_0);
      const auto v46 = r42_0.c0;
      for (uint32_t j42_1 = idx_44.First({v43}); j42_1 != ::hyde::rt::kNoRow; j42_1 = idx_44.Next(j42_1)) {
        const auto r42_1 = table_18.RowAt(j42_1);
        const auto v47 = r42_1.x;
        const auto v45 = r42_1.c1;
        if (v43 == v45 && v43 == v46) {
          q_double_12.TryChangeAbsentToPresent({v47});
        }
      }
    }
  }
  vec31.Clear();
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

