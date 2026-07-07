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
      q_13(allocator_),
      table_16(allocator_),
      idx_34(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec54(allocator);
  ::hyde::rt::Vec<Tup_i32> vec55(allocator);
  ::hyde::rt::Vec<Tup_i32> vec56(allocator);
  proc_20(std::move(vec54), std::move(vec55), std::move(vec56));
  return false;
}

bool Database::proc_20(::hyde::rt::Vec<Tup_i32> vec22, ::hyde::rt::Vec<Tup_i32> vec26, ::hyde::rt::Vec<Tup_i32> vec29) {
  ::hyde::rt::Vec<Tup_b> vec25(allocator);
  for (auto [v24] : vec22) {
    if (1 == v24) {
      if (const auto ins0 = table_9.TryChangeAbsentToPresent({true, 1}); ins0.changed) {
        if (const auto ins1 = __6.TryChangeAbsentToPresent({true}); ins1.changed) {
          vec25.Add({true});
        }
      }
    }
  }
  for (auto [v28] : vec26) {
    if (const auto ins2 = table_16.TryChangeAbsentToPresent({v28, true}); ins2.changed) {
      if (ins2.added_row) {
        idx_34.Add({true}, ins2.id);
      }
      vec25.Add({true});
    }
  }
  for (auto [v31] : vec29) {
    if (const auto ins3 = q_13.TryChangeAbsentToPresent({v31}); ins3.changed) {
    }
  }
  vec22.Clear();
  vec26.Clear();
  vec29.Clear();
  flow_57(std::move(vec25));
  return false;
}

bool Database::me_1(::hyde::rt::Vec<Tup_i32> vec39) {
  ::hyde::rt::Vec<Tup_i32> vec41(allocator);
  ::hyde::rt::Vec<Tup_i32> vec42(allocator);
  proc_20(std::move(vec39), std::move(vec41), std::move(vec42));
  return true;
}

bool Database::ma_1(::hyde::rt::Vec<Tup_i32> vec44) {
  ::hyde::rt::Vec<Tup_i32> vec46(allocator);
  ::hyde::rt::Vec<Tup_i32> vec47(allocator);
  proc_20(std::move(vec46), std::move(vec44), std::move(vec47));
  return true;
}

bool Database::mb_1(::hyde::rt::Vec<Tup_i32> vec49) {
  ::hyde::rt::Vec<Tup_i32> vec51(allocator);
  ::hyde::rt::Vec<Tup_i32> vec52(allocator);
  proc_20(std::move(vec51), std::move(vec52), std::move(vec49));
  return true;
}

bool Database::flow_57(::hyde::rt::Vec<Tup_b> vec25) {
  if ((g21 += 1) == 1) {
  }
  vec25.SortAndUnique();
  for (auto [v33] : vec25) {
    if (const uint32_t j32_0 = __6.Find({v33}); j32_0 != ::hyde::rt::kNoRow) {
      const auto r32_0 = __6.RowAt(j32_0);
      const auto v36 = r32_0.c0;
      for (uint32_t j32_1 = idx_34.First({v33}); j32_1 != ::hyde::rt::kNoRow; j32_1 = idx_34.Next(j32_1)) {
        const auto r32_1 = table_16.RowAt(j32_1);
        const auto v37 = r32_1.x;
        const auto v35 = r32_1.c1;
        if (v33 == v35 && v33 == v36) {
          if (const auto ins4 = q_13.TryChangeAbsentToPresent({v37}); ins4.changed) {
          }
        }
      }
    }
  }
  vec25.Clear();
  return true;
}

Database::q_f_cursor Database::q_f() {
  return {*this, 0};
}

bool Database::q_f_cursor::next(int32_t &X) {
  while (pos < db.q_13.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.q_13.RowAt(id);
    if (db.q_13.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

