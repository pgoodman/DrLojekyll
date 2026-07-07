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
      out2_16(allocator_),
      proof_20(allocator_),
      table_23(allocator_),
      table_26(allocator_),
      table_29(allocator_),
      idx_50(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_u64_u64> vec73(allocator);
  ::hyde::rt::Vec<Tup_u64> vec74(allocator);
  ::hyde::rt::Vec<Tup_i32> vec75(allocator);
  proc_33(std::move(vec73), std::move(vec74), std::move(vec75));
  return false;
}

bool Database::proc_33(::hyde::rt::Vec<Tup_u64_u64> vec36, ::hyde::rt::Vec<Tup_u64> vec41, ::hyde::rt::Vec<Tup_i32> vec45) {
  ::hyde::rt::Vec<Tup_b> vec35(allocator);
  ::hyde::rt::Vec<Tup_u64> vec40(allocator);
  ::hyde::rt::Vec<Tup_u64> vec44(allocator);
  for (auto [v38, v39] : vec36) {
    if (std::make_tuple(v38) < std::make_tuple(v39)) {
      if (const auto ins0 = out_13.TryChangeAbsentToPresent({v38}); ins0.changed) {
      }
    }
    if (std::make_tuple(v38) < std::make_tuple(v39)) {
      if (const auto ins1 = out_13.TryChangeAbsentToPresent({v38}); ins1.changed) {
      }
    }
    if (std::make_tuple(v38) < std::make_tuple(v39)) {
      if (const auto ins2 = table_23.TryChangeAbsentToPresent({v38}); ins2.changed) {
        vec40.Add({v38});
      }
    }
  }
  for (auto [v43] : vec41) {
    if (const auto ins3 = table_26.TryChangeAbsentToPresent({v43}); ins3.changed) {
      vec44.Add({v43});
    }
  }
  for (auto [v47] : vec45) {
    if (const auto ins4 = table_9.TryChangeAbsentToPresent({true, v47}); ins4.changed) {
      if (const auto ins5 = __6.TryChangeAbsentToPresent({true}); ins5.changed) {
        vec35.Add({true});
      }
    }
  }
  vec36.Clear();
  vec41.Clear();
  vec45.Clear();
  flow_76(std::move(vec35), std::move(vec40), std::move(vec44));
  return false;
}

bool Database::in_2(::hyde::rt::Vec<Tup_u64_u64> vec58) {
  ::hyde::rt::Vec<Tup_u64> vec60(allocator);
  ::hyde::rt::Vec<Tup_i32> vec61(allocator);
  proc_33(std::move(vec58), std::move(vec60), std::move(vec61));
  return true;
}

bool Database::m_1(::hyde::rt::Vec<Tup_u64> vec63) {
  ::hyde::rt::Vec<Tup_u64_u64> vec65(allocator);
  ::hyde::rt::Vec<Tup_i32> vec66(allocator);
  proc_33(std::move(vec65), std::move(vec63), std::move(vec66));
  return true;
}

bool Database::something_1(::hyde::rt::Vec<Tup_i32> vec68) {
  ::hyde::rt::Vec<Tup_u64_u64> vec70(allocator);
  ::hyde::rt::Vec<Tup_u64> vec71(allocator);
  proc_33(std::move(vec70), std::move(vec71), std::move(vec68));
  return true;
}

bool Database::flow_76(::hyde::rt::Vec<Tup_b> vec35, ::hyde::rt::Vec<Tup_u64> vec40, ::hyde::rt::Vec<Tup_u64> vec44) {
  if ((g34 += 1) == 1) {
    if (const auto ins6 = table_29.TryChangeAbsentToPresent({1, true}); ins6.changed) {
      if (ins6.added_row) {
        idx_50.Add({true}, ins6.id);
      }
      vec35.Add({true});
    }
  }
  vec40.SortAndUnique();
  vec44.SortAndUnique();
  ::hyde::rt::Vec<Tup_u64_u64> stage54(allocator);
  for (auto [v55] : vec40) {
    for (uint32_t p54_1 = 0; p54_1 < table_26.NumRows(); ++p54_1) {
      const auto r54_1 = table_26.RowAt(p54_1);
      const auto v56 = r54_1.y;
      stage54.Add({v55, v56});
    }
  }
  for (auto [v56] : vec44) {
    for (uint32_t p54_0 = 0; p54_0 < table_23.NumRows(); ++p54_0) {
      const auto r54_0 = table_23.RowAt(p54_0);
      const auto v55 = r54_0.x;
      stage54.Add({v55, v56});
    }
  }
  for (auto [v55, v56] : stage54) {
    if (const auto ins7 = out2_16.TryChangeAbsentToPresent({v55, v56}); ins7.changed) {
    }
  }
  vec40.Clear();
  vec44.Clear();
  vec35.SortAndUnique();
  for (auto [v49] : vec35) {
    if (const uint32_t j48_0 = __6.Find({v49}); j48_0 != ::hyde::rt::kNoRow) {
      const auto r48_0 = __6.RowAt(j48_0);
      const auto v52 = r48_0.c0;
      for (uint32_t j48_1 = idx_50.First({v49}); j48_1 != ::hyde::rt::kNoRow; j48_1 = idx_50.Next(j48_1)) {
        const auto r48_1 = table_29.RowAt(j48_1);
        const auto v53 = r48_1.c0;
        const auto v51 = r48_1.c1;
        if (v49 == v51 && v49 == v52) {
          if (const auto ins8 = proof_20.TryChangeAbsentToPresent({v53}); ins8.changed) {
          }
        }
      }
    }
  }
  vec35.Clear();
  return true;
}

Database::out_f_cursor Database::out_f() {
  return {*this, 0};
}

bool Database::out_f_cursor::next(uint64_t &X) {
  while (pos < db.out_13.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out_13.RowAt(id);
    if (db.out_13.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

Database::out2_ff_cursor Database::out2_ff() {
  return {*this, 0};
}

bool Database::out2_ff_cursor::next(uint64_t &X, uint64_t &Y) {
  while (pos < db.out2_16.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out2_16.RowAt(id);
    if (db.out2_16.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.x;
    Y = row.y;
    return true;
  }
  return false;
}

Database::proof_f_cursor Database::proof_f() {
  return {*this, 0};
}

bool Database::proof_f_cursor::next(int32_t &X) {
  while (pos < db.proof_20.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.proof_20.RowAt(id);
    if (db.proof_20.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.a;
    return true;
  }
  return false;
}

