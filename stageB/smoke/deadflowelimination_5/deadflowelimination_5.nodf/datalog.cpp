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
      table_19(allocator_),
      idx_47(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec60(allocator);
  ::hyde::rt::Vec<Tup_i32> vec61(allocator);
  proc_23(std::move(vec60), std::move(vec61));
  return false;
}

bool Database::proc_23(::hyde::rt::Vec<Tup_i32> vec31, ::hyde::rt::Vec<Tup_i32> vec35) {
  ::hyde::rt::Vec<Tup_i32> vec26(allocator);
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
  for (auto [v37] : vec35) {
    if (const auto ins2 = table_16.TryChangeAbsentToPresent({v37}); ins2.changed) {
      const uint64_t v38 = 0;
      (void) v38;
      vec26.Add({v37});
    }
  }
  vec31.Clear();
  vec35.Clear();
  flow_62(std::move(vec26), std::move(vec28));
  return false;
}

bool Database::cond_src_1(::hyde::rt::Vec<Tup_i32> vec52) {
  ::hyde::rt::Vec<Tup_i32> vec54(allocator);
  proc_23(std::move(vec52), std::move(vec54));
  return true;
}

bool Database::in_1(::hyde::rt::Vec<Tup_i32> vec56) {
  ::hyde::rt::Vec<Tup_i32> vec58(allocator);
  proc_23(std::move(vec58), std::move(vec56));
  return true;
}

bool Database::flow_62(::hyde::rt::Vec<Tup_i32> vec26, ::hyde::rt::Vec<Tup_b> vec28) {
  ::hyde::rt::Vec<Tup_i32> vec27(allocator);
  ::hyde::rt::Vec<Tup_b> vec29(allocator);
  if ((g24 += 1) == 1) {
    if (const auto ins3 = table_16.TryChangeAbsentToPresent({1}); ins3.changed) {
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
      for (auto [v40] : vec27) {
        if (const auto ins4 = table_19.TryChangeAbsentToPresent({v40, true}); ins4.changed) {
          if (ins4.added_row) {
            idx_47.Add({true}, ins4.id);
          }
          const uint64_t v43 = 0;
          (void) v43;
          vec28.Add({true});
        }
        out_13.TryChangeAbsentToPresent({v40});
      }
      vec29.Clear();
      vec28.SortAndUnique();
      vec28.Swap(vec29);
      for (auto [v46] : vec29) {
        if (const uint32_t j45_0 = __6.Find({v46}); j45_0 != ::hyde::rt::kNoRow) {
          const auto r45_0 = __6.RowAt(j45_0);
          const auto v49 = r45_0.c0;
          for (uint32_t j45_1 = idx_47.First({v46}); j45_1 != ::hyde::rt::kNoRow; j45_1 = idx_47.Next(j45_1)) {
            const auto r45_1 = table_19.RowAt(j45_1);
            const auto v50 = r45_1.a;
            const auto v48 = r45_1.c1;
            if (v46 == v48 && v46 == v49) {
              if (const auto ins5 = table_16.TryChangeAbsentToPresent({v50}); ins5.changed) {
                const uint64_t v44 = 0;
                (void) v44;
                vec26.Add({v50});
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

