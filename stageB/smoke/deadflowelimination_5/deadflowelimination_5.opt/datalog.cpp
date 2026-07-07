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
      out_13(allocator_),
      table_16(allocator_),
      idx_47(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec60(allocator);
  ::hyde::rt::Vec<Tup_i32> vec61(allocator);
  proc_20(std::move(vec60), std::move(vec61));
  return false;
}

bool Database::proc_20(::hyde::rt::Vec<Tup_i32> vec29, ::hyde::rt::Vec<Tup_i32> vec33) {
  ::hyde::rt::Vec<Tup_i32> vec23(allocator);
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
  for (auto [v35] : vec33) {
    if (const auto ins2 = out_13.TryChangeAbsentToPresent({v35}); ins2.changed) {
      const uint64_t v36 = 0;
      (void) v36;
      vec23.Add({v35});
    }
  }
  vec29.Clear();
  vec33.Clear();
  flow_62(std::move(vec23), std::move(vec26));
  return false;
}

bool Database::cond_src_1(::hyde::rt::Vec<Tup_i32> vec52) {
  ::hyde::rt::Vec<Tup_i32> vec54(allocator);
  proc_20(std::move(vec52), std::move(vec54));
  return true;
}

bool Database::in_1(::hyde::rt::Vec<Tup_i32> vec56) {
  ::hyde::rt::Vec<Tup_i32> vec58(allocator);
  proc_20(std::move(vec58), std::move(vec56));
  return true;
}

bool Database::flow_62(::hyde::rt::Vec<Tup_i32> vec23, ::hyde::rt::Vec<Tup_b> vec26) {
  ::hyde::rt::Vec<Tup_i32> vec24(allocator);
  ::hyde::rt::Vec<Tup_i32> vec25(allocator);
  ::hyde::rt::Vec<Tup_b> vec27(allocator);
  if ((g21 += 1) == 1) {
    if (const auto ins3 = out_13.TryChangeAbsentToPresent({1}); ins3.changed) {
      const uint64_t v28 = 0;
      (void) v28;
      vec23.Add({1});
    }
  }
  // set 0 depth 1
  for (bool reenter22 = true; reenter22; ) {
    for (bool changed22 = true; changed22; changed22 = !(true && vec23.Empty() && vec26.Empty())) {
      vec24.Clear();
      vec23.SortAndUnique();
      vec23.Swap(vec24);
      for (auto [v38] : vec24) {
        vec25.Add({v38});
        if (const auto ins4 = table_16.TryChangeAbsentToPresent({v38, true}); ins4.changed) {
          if (ins4.added_row) {
            idx_47.Add({true}, ins4.id);
          }
          const uint64_t v43 = 0;
          (void) v43;
          vec26.Add({true});
        }
      }
      vec27.Clear();
      vec26.SortAndUnique();
      vec26.Swap(vec27);
      for (auto [v46] : vec27) {
        if (const uint32_t j45_0 = __6.Find({v46}); j45_0 != ::hyde::rt::kNoRow) {
          const auto r45_0 = __6.RowAt(j45_0);
          const auto v49 = r45_0.c0;
          for (uint32_t j45_1 = idx_47.First({v46}); j45_1 != ::hyde::rt::kNoRow; j45_1 = idx_47.Next(j45_1)) {
            const auto r45_1 = table_16.RowAt(j45_1);
            const auto v50 = r45_1.a;
            const auto v48 = r45_1.c1;
            if (v46 == v48 && v46 == v49) {
              if (const auto ins5 = out_13.TryChangeAbsentToPresent({v50}); ins5.changed) {
                const uint64_t v44 = 0;
                (void) v44;
                vec23.Add({v50});
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
    vec26.Clear();
    vec27.Clear();
    vec25.Clear();
    reenter22 = !(true && vec23.Empty() && vec26.Empty());
  }
  return true;
}

Database::out_f_cursor Database::out_f() {
  return {*this, 0};
}

bool Database::out_f_cursor::next(int32_t &A) {
  while (pos < db.out_13.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out_13.RowAt(id);
    if (db.out_13.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    A = row.a;
    return true;
  }
  return false;
}

