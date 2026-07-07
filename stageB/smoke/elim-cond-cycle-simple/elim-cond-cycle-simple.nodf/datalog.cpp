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
      table_19(allocator_),
      idx_43(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec51(allocator);
  proc_23(std::move(vec51));
  return false;
}

bool Database::proc_23(::hyde::rt::Vec<Tup_i32> vec31) {
  ::hyde::rt::Vec<Tup_b> vec28(allocator);
  for (auto [v33] : vec31) {
    if (const auto ins0 = table_9.TryChangeAbsentToPresent({true, v33}); ins0.changed) {
      if (const auto ins1 = __6.TryChangeAbsentToPresent({true}); ins1.changed) {
        const uint64_t v34 = 0;
        (void) v34;
        vec28.Add({true});
      }
    }
  }
  vec31.Clear();
  flow_52(std::move(vec28));
  return false;
}

bool Database::cond_func_1(::hyde::rt::Vec<Tup_i32> vec48) {
  proc_23(std::move(vec48));
  return true;
}

bool Database::flow_52(::hyde::rt::Vec<Tup_b> vec28) {
  ::hyde::rt::Vec<Tup_i32> vec26(allocator);
  ::hyde::rt::Vec<Tup_i32> vec27(allocator);
  ::hyde::rt::Vec<Tup_b> vec29(allocator);
  if ((g24 += 1) == 1) {
    if (const auto ins2 = table_16.TryChangeAbsentToPresent({1}); ins2.changed) {
      const uint64_t v30 = 0;
      (void) v30;
      vec26.Add({1});
    }
  }
  // set 0 depth 1
  for (bool reenter25 = true; reenter25; ) {
    for (bool changed25 = true; changed25; changed25 = !(true && vec26.Empty() && vec28.Empty())) {
      vec27.Clear();
      vec26.SortAndUnique();
      vec26.Swap(vec27);
      for (auto [v36] : vec27) {
        if (const auto ins3 = table_19.TryChangeAbsentToPresent({v36, true}); ins3.changed) {
          if (ins3.added_row) {
            idx_43.Add({true}, ins3.id);
          }
          const uint64_t v39 = 0;
          (void) v39;
          vec28.Add({true});
        }
        output_13.TryChangeAbsentToPresent({v36});
      }
      vec29.Clear();
      vec28.SortAndUnique();
      vec28.Swap(vec29);
      for (auto [v42] : vec29) {
        if (const uint32_t j41_0 = __6.Find({v42}); j41_0 != ::hyde::rt::kNoRow) {
          const auto r41_0 = __6.RowAt(j41_0);
          const auto v45 = r41_0.c0;
          for (uint32_t j41_1 = idx_43.First({v42}); j41_1 != ::hyde::rt::kNoRow; j41_1 = idx_43.Next(j41_1)) {
            const auto r41_1 = table_19.RowAt(j41_1);
            const auto v46 = r41_1.a;
            const auto v44 = r41_1.c1;
            if (v42 == v44 && v42 == v45) {
              if (const auto ins4 = table_16.TryChangeAbsentToPresent({v46}); ins4.changed) {
                const uint64_t v40 = 0;
                (void) v40;
                vec26.Add({v46});
              }
            }
          }
        }
      }
      vec29.Clear();
    }
    vec26.Clear();
    vec27.Clear();
    vec28.Clear();
    vec29.Clear();
    reenter25 = !(true && vec26.Empty() && vec28.Empty());
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

