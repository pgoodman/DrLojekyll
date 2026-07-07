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
      q_chain_13(allocator_),
      q_gated_16(allocator_),
      table_19(allocator_),
      idx_34(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec47(allocator);
  ::hyde::rt::Vec<Tup_i32> vec48(allocator);
  proc_23(std::move(vec47), std::move(vec48));
  return false;
}

bool Database::proc_23(::hyde::rt::Vec<Tup_i32> vec25, ::hyde::rt::Vec<Tup_i32> vec29) {
  ::hyde::rt::Vec<Tup_b> vec28(allocator);
  for (auto [v27] : vec25) {
    if (const auto ins0 = table_19.TryChangeAbsentToPresent({v27, true}); ins0.changed) {
      if (ins0.added_row) {
        idx_34.Add({true}, ins0.id);
      }
      vec28.Add({true});
    }
    if (!(0 == v27)) {
      q_chain_13.TryChangeAbsentToPresent({v27});
    }
  }
  for (auto [v31] : vec29) {
    if (const auto ins1 = table_9.TryChangeAbsentToPresent({true, v31}); ins1.changed) {
      if (const auto ins2 = __6.TryChangeAbsentToPresent({true}); ins2.changed) {
        vec28.Add({true});
      }
    }
  }
  vec25.Clear();
  vec29.Clear();
  flow_49(std::move(vec28));
  return false;
}

bool Database::in_1(::hyde::rt::Vec<Tup_i32> vec39) {
  ::hyde::rt::Vec<Tup_i32> vec41(allocator);
  proc_23(std::move(vec39), std::move(vec41));
  return true;
}

bool Database::log_in_1(::hyde::rt::Vec<Tup_i32> vec43) {
  ::hyde::rt::Vec<Tup_i32> vec45(allocator);
  proc_23(std::move(vec45), std::move(vec43));
  return true;
}

bool Database::flow_49(::hyde::rt::Vec<Tup_b> vec28) {
  g24 += 1;
  vec28.SortAndUnique();
  for (auto [v33] : vec28) {
    if (const uint32_t j32_0 = __6.Find({v33}); j32_0 != ::hyde::rt::kNoRow) {
      const auto r32_0 = __6.RowAt(j32_0);
      const auto v36 = r32_0.c0;
      for (uint32_t j32_1 = idx_34.First({v33}); j32_1 != ::hyde::rt::kNoRow; j32_1 = idx_34.Next(j32_1)) {
        const auto r32_1 = table_19.RowAt(j32_1);
        const auto v37 = r32_1.x;
        const auto v35 = r32_1.c1;
        if (v33 == v35 && v33 == v36) {
          if (!(0 == v37)) {
            q_gated_16.TryChangeAbsentToPresent({v37});
          }
        }
      }
    }
  }
  vec28.Clear();
  return true;
}

Database::q_chain_f_cursor Database::q_chain_f() {
  return {*this, 0};
}

bool Database::q_chain_f_cursor::next(int32_t &X) {
  while (pos < db.q_chain_13.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.q_chain_13.RowAt(id);
    if (db.q_chain_13.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.a;
    return true;
  }
  return false;
}

Database::q_gated_f_cursor Database::q_gated_f() {
  return {*this, 0};
}

bool Database::q_gated_f_cursor::next(int32_t &X) {
  while (pos < db.q_gated_16.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.q_gated_16.RowAt(id);
    if (db.q_gated_16.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.a;
    return true;
  }
  return false;
}

