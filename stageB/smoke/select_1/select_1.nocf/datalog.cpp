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
      is_on_12(allocator_),
      table_15(allocator_),
      idx_30(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_u32> vec43(allocator);
  ::hyde::rt::Vec<Tup_i32> vec44(allocator);
  proc_19(std::move(vec43), std::move(vec44));
  return false;
}

bool Database::proc_19(::hyde::rt::Vec<Tup_u32> vec21, ::hyde::rt::Vec<Tup_i32> vec25) {
  ::hyde::rt::Vec<Tup_b> vec24(allocator);
  for (auto [v23] : vec21) {
    if (const auto ins0 = table_8.TryChangeAbsentToPresent({true, v23}); ins0.changed) {
      if (const auto ins1 = __5.TryChangeAbsentToPresent({true}); ins1.changed) {
        vec24.Add({true});
      }
    }
  }
  for (auto [v27] : vec25) {
    if (const auto ins2 = table_15.TryChangeAbsentToPresent({v27, true}); ins2.changed) {
      if (ins2.added_row) {
        idx_30.Add({true}, ins2.id);
      }
      vec24.Add({true});
    }
  }
  vec21.Clear();
  vec25.Clear();
  flow_45(std::move(vec24));
  return false;
}

bool Database::enable_1(::hyde::rt::Vec<Tup_u32> vec35) {
  ::hyde::rt::Vec<Tup_i32> vec37(allocator);
  proc_19(std::move(vec35), std::move(vec37));
  return true;
}

bool Database::ping_1(::hyde::rt::Vec<Tup_i32> vec39) {
  ::hyde::rt::Vec<Tup_u32> vec41(allocator);
  proc_19(std::move(vec41), std::move(vec39));
  return true;
}

bool Database::flow_45(::hyde::rt::Vec<Tup_b> vec24) {
  if ((g20 += 1) == 1) {
  }
  vec24.SortAndUnique();
  for (auto [v29] : vec24) {
    if (const uint32_t j28_0 = __5.Find({v29}); j28_0 != ::hyde::rt::kNoRow) {
      const auto r28_0 = __5.RowAt(j28_0);
      const auto v32 = r28_0.c0;
      for (uint32_t j28_1 = idx_30.First({v29}); j28_1 != ::hyde::rt::kNoRow; j28_1 = idx_30.Next(j28_1)) {
        const auto r28_1 = table_15.RowAt(j28_1);
        const auto v33 = r28_1.y;
        const auto v31 = r28_1.c1;
        if (v29 == v31 && v29 == v32) {
          if (const auto ins3 = is_on_12.TryChangeAbsentToPresent({v33}); ins3.changed) {
          }
        }
      }
    }
  }
  vec24.Clear();
  return true;
}

Database::is_on_f_cursor Database::is_on_f() {
  return {*this, 0};
}

bool Database::is_on_f_cursor::next(int32_t &Y) {
  while (pos < db.is_on_12.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.is_on_12.RowAt(id);
    if (db.is_on_12.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    Y = row.y;
    return true;
  }
  return false;
}

