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
      table_18(allocator_),
      table_21(allocator_),
      idx_41(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec54(allocator);
  ::hyde::rt::Vec<Tup_i32> vec55(allocator);
  proc_25(std::move(vec54), std::move(vec55));
  return false;
}

bool Database::proc_25(::hyde::rt::Vec<Tup_i32> vec27, ::hyde::rt::Vec<Tup_i32> vec32) {
  ::hyde::rt::Vec<Tup_b> vec30(allocator);
  ::hyde::rt::Vec<Tup_i32> vec31(allocator);
  for (auto [v29] : vec27) {
    if (const auto ins0 = table_18.TryChangeAbsentToPresent({v29}); ins0.changed) {
      if (const auto ins1 = table_21.TryChangeAbsentToPresent({v29, true}); ins1.changed) {
        if (ins1.added_row) {
          idx_41.Add({true}, ins1.id);
        }
        vec30.Add({true});
      }
      vec31.Add({v29});
    }
  }
  for (auto [v34] : vec32) {
    if (const auto ins2 = table_15.TryChangeAbsentToPresent({v34}); ins2.changed) {
      vec31.Add({v34});
    }
  }
  vec27.Clear();
  vec32.Clear();
  flow_56(std::move(vec30), std::move(vec31));
  return false;
}

bool Database::add_user_1(::hyde::rt::Vec<Tup_i32> vec46) {
  ::hyde::rt::Vec<Tup_i32> vec48(allocator);
  proc_25(std::move(vec46), std::move(vec48));
  return true;
}

bool Database::log_in_1(::hyde::rt::Vec<Tup_i32> vec50) {
  ::hyde::rt::Vec<Tup_i32> vec52(allocator);
  proc_25(std::move(vec52), std::move(vec50));
  return true;
}

bool Database::flow_56(::hyde::rt::Vec<Tup_b> vec30, ::hyde::rt::Vec<Tup_i32> vec31) {
  if ((g26 += 1) == 1) {
  }
  vec31.SortAndUnique();
  for (auto [v36] : vec31) {
    if (const uint32_t j35_0 = table_18.Find({v36}); j35_0 != ::hyde::rt::kNoRow) {
      const auto r35_0 = table_18.RowAt(j35_0);
      const auto v38 = r35_0.id;
      if (const uint32_t j35_1 = table_15.Find({v36}); j35_1 != ::hyde::rt::kNoRow) {
        const auto r35_1 = table_15.RowAt(j35_1);
        const auto v37 = r35_1.id;
        if (v36 == v37 && v36 == v38) {
          if (const auto ins3 = table_8.TryChangeAbsentToPresent({true, v38}); ins3.changed) {
            if (const auto ins4 = __5.TryChangeAbsentToPresent({true}); ins4.changed) {
              vec30.Add({true});
            }
          }
        }
      }
    }
  }
  vec31.Clear();
  vec30.SortAndUnique();
  for (auto [v40] : vec30) {
    if (const uint32_t j39_0 = __5.Find({v40}); j39_0 != ::hyde::rt::kNoRow) {
      const auto r39_0 = __5.RowAt(j39_0);
      const auto v43 = r39_0.c0;
      for (uint32_t j39_1 = idx_41.First({v40}); j39_1 != ::hyde::rt::kNoRow; j39_1 = idx_41.Next(j39_1)) {
        const auto r39_1 = table_21.RowAt(j39_1);
        const auto v44 = r39_1.id;
        const auto v42 = r39_1.c1;
        if (v40 == v42 && v40 == v43) {
          if (const auto ins5 = out_12.TryChangeAbsentToPresent({v44}); ins5.changed) {
          }
        }
      }
    }
  }
  vec30.Clear();
  return true;
}

Database::out_f_cursor Database::out_f() {
  return {*this, 0};
}

bool Database::out_f_cursor::next(int32_t &UserId) {
  while (pos < db.out_12.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out_12.RowAt(id);
    if (db.out_12.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    UserId = row.id;
    return true;
  }
  return false;
}

