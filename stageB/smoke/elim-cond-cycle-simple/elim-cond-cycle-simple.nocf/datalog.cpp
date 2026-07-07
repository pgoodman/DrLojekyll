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
      output_13(allocator_),
      table_16(allocator_),
      idx_43(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec51(allocator);
  proc_20(std::move(vec51));
  return false;
}

bool Database::proc_20(::hyde::rt::Vec<Tup_i32> vec29) {
  ::hyde::rt::Vec<Tup_b> vec26(allocator);
  for (auto [v31] : vec29) {
    if (const auto ins0 = table_9.TryChangeAbsentToPresent({true, v31}); ins0.changed) {
      if (const auto ins1 = __6.TryChangeAbsentToPresent({true}); ins1.changed) {
        const uint64_t v32 = 0;
        (void) v32;
        vec26.Add({true});
      }
    }
  }
  vec29.Clear();
  flow_52(std::move(vec26));
  return false;
}

bool Database::cond_func_1(::hyde::rt::Vec<Tup_i32> vec48) {
  proc_20(std::move(vec48));
  return true;
}

bool Database::flow_52(::hyde::rt::Vec<Tup_b> vec26) {
  ::hyde::rt::Vec<Tup_i32> vec23(allocator);
  ::hyde::rt::Vec<Tup_i32> vec24(allocator);
  ::hyde::rt::Vec<Tup_i32> vec25(allocator);
  ::hyde::rt::Vec<Tup_b> vec27(allocator);
  // set 0 depth 1
  if ((g21 += 1) == 1) {
    if (const auto ins2 = output_13.TryChangeAbsentToPresent({1}); ins2.changed) {
      const uint64_t v28 = 0;
      (void) v28;
      vec23.Add({1});
    }
  }
  for (bool reenter22 = true; reenter22; ) {
    for (bool changed22 = true; changed22; changed22 = !(true && vec23.Empty() && vec26.Empty())) {
      vec24.Clear();
      vec23.SortAndUnique();
      vec23.Swap(vec24);
      for (auto [v34] : vec24) {
        vec25.Add({v34});
        if (const auto ins3 = table_16.TryChangeAbsentToPresent({v34, true}); ins3.changed) {
          if (ins3.added_row) {
            idx_43.Add({true}, ins3.id);
          }
          const uint64_t v39 = 0;
          (void) v39;
          vec26.Add({true});
        }
      }
      vec27.Clear();
      vec26.SortAndUnique();
      vec26.Swap(vec27);
      for (auto [v42] : vec27) {
        if (const uint32_t j41_0 = __6.Find({v42}); j41_0 != ::hyde::rt::kNoRow) {
          const auto r41_0 = __6.RowAt(j41_0);
          const auto v45 = r41_0.c0;
          for (uint32_t j41_1 = idx_43.First({v42}); j41_1 != ::hyde::rt::kNoRow; j41_1 = idx_43.Next(j41_1)) {
            const auto r41_1 = table_16.RowAt(j41_1);
            const auto v46 = r41_1.a;
            const auto v44 = r41_1.c1;
            if (v42 == v44 && v42 == v45) {
              const auto v37 = v44;
              const auto v38 = v46;
              if (const auto ins4 = output_13.TryChangeAbsentToPresent({v38}); ins4.changed) {
                const uint64_t v40 = 0;
                (void) v40;
                vec23.Add({v38});
              }
            }
          }
        }
      }
      vec27.Clear();
    }
    vec23.Clear();
    vec24.Clear();
    vec25.SortAndUnique();
    for (auto [v36] : vec25) {
    }
    vec26.Clear();
    vec27.Clear();
    vec26.Clear();
    vec23.Clear();
    vec27.Clear();
    vec24.Clear();
    vec25.Clear();
    reenter22 = !(true && vec23.Empty() && vec26.Empty());
  }
  return true;
}

Database::output_f_cursor Database::output_f() {
  return {*this, 0};
}

bool Database::output_f_cursor::next(int32_t &A) {
  while (pos < db.output_13.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.output_13.RowAt(id);
    if (db.output_13.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    A = row.a;
    return true;
  }
  return false;
}

