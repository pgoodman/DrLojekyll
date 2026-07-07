// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      __7(allocator_),
      table_10(allocator_),
      __14(allocator_),
      table_17(allocator_),
      table_21(allocator_),
      proof_25(allocator_),
      q_shared_28(allocator_),
      q_fact_31(allocator_),
      table_35(allocator_),
      idx_54(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec79(allocator);
  ::hyde::rt::Vec<Tup_i32> vec80(allocator);
  ::hyde::rt::Vec<Tup_i32> vec81(allocator);
  proc_39(std::move(vec79), std::move(vec80), std::move(vec81));
  return false;
}

bool Database::proc_39(::hyde::rt::Vec<Tup_i32> vec43, ::hyde::rt::Vec<Tup_i32> vec46, ::hyde::rt::Vec<Tup_i32> vec49) {
  ::hyde::rt::Vec<Tup_b> vec41(allocator);
  ::hyde::rt::Vec<Tup_b> vec42(allocator);
  for (auto [v45] : vec43) {
    if (const auto ins0 = table_10.TryChangeAbsentToPresent({true, v45}); ins0.changed) {
      if (const auto ins1 = __7.TryChangeAbsentToPresent({true}); ins1.changed) {
        vec41.Add({true});
      }
    }
  }
  for (auto [v48] : vec46) {
    if (const auto ins2 = table_17.TryChangeAbsentToPresent({true, v48}); ins2.changed) {
      if (const auto ins3 = __14.TryChangeAbsentToPresent({true}); ins3.changed) {
        vec42.Add({true});
      }
    }
  }
  for (auto [v51] : vec49) {
    if (const auto ins4 = table_21.TryChangeAbsentToPresent({true, v51}); ins4.changed) {
      if (const auto ins5 = __14.TryChangeAbsentToPresent({true}); ins5.changed) {
        vec42.Add({true});
      }
    }
  }
  vec43.Clear();
  vec46.Clear();
  vec49.Clear();
  flow_82(std::move(vec41), std::move(vec42));
  return false;
}

bool Database::something_1(::hyde::rt::Vec<Tup_i32> vec64) {
  ::hyde::rt::Vec<Tup_i32> vec66(allocator);
  ::hyde::rt::Vec<Tup_i32> vec67(allocator);
  proc_39(std::move(vec64), std::move(vec66), std::move(vec67));
  return true;
}

bool Database::a_1(::hyde::rt::Vec<Tup_i32> vec69) {
  ::hyde::rt::Vec<Tup_i32> vec71(allocator);
  ::hyde::rt::Vec<Tup_i32> vec72(allocator);
  proc_39(std::move(vec71), std::move(vec69), std::move(vec72));
  return true;
}

bool Database::b_1(::hyde::rt::Vec<Tup_i32> vec74) {
  ::hyde::rt::Vec<Tup_i32> vec76(allocator);
  ::hyde::rt::Vec<Tup_i32> vec77(allocator);
  proc_39(std::move(vec76), std::move(vec77), std::move(vec74));
  return true;
}

bool Database::flow_82(::hyde::rt::Vec<Tup_b> vec41, ::hyde::rt::Vec<Tup_b> vec42) {
  if ((g40 += 1) == 1) {
    if (const auto ins6 = table_35.TryChangeAbsentToPresent({1, true}); ins6.changed) {
      if (ins6.added_row) {
        idx_54.Add({true}, ins6.id);
      }
      vec41.Add({true});
      vec42.Add({true});
    }
    if (const auto ins7 = q_fact_31.TryChangeAbsentToPresent({1, 2}); ins7.changed) {
    }
  }
  vec42.SortAndUnique();
  for (auto [v53] : vec42) {
    if (const uint32_t j52_0 = __14.Find({v53}); j52_0 != ::hyde::rt::kNoRow) {
      const auto r52_0 = __14.RowAt(j52_0);
      const auto v56 = r52_0.c0;
      for (uint32_t j52_1 = idx_54.First({v53}); j52_1 != ::hyde::rt::kNoRow; j52_1 = idx_54.Next(j52_1)) {
        const auto r52_1 = table_35.RowAt(j52_1);
        const auto v57 = r52_1.c0;
        const auto v55 = r52_1.c1;
        if (v53 == v55 && v53 == v56) {
          if (const auto ins8 = q_shared_28.TryChangeAbsentToPresent({v57}); ins8.changed) {
          }
        }
      }
    }
  }
  vec42.Clear();
  vec41.SortAndUnique();
  for (auto [v59] : vec41) {
    if (const uint32_t j58_0 = __7.Find({v59}); j58_0 != ::hyde::rt::kNoRow) {
      const auto r58_0 = __7.RowAt(j58_0);
      const auto v61 = r58_0.c0;
      for (uint32_t j58_1 = idx_54.First({v59}); j58_1 != ::hyde::rt::kNoRow; j58_1 = idx_54.Next(j58_1)) {
        const auto r58_1 = table_35.RowAt(j58_1);
        const auto v62 = r58_1.c0;
        const auto v60 = r58_1.c1;
        if (v59 == v60 && v59 == v61) {
          if (const auto ins9 = proof_25.TryChangeAbsentToPresent({v62}); ins9.changed) {
          }
        }
      }
    }
  }
  vec41.Clear();
  return true;
}

Database::proof_f_cursor Database::proof_f() {
  return {*this, 0};
}

bool Database::proof_f_cursor::next(int32_t &X) {
  while (pos < db.proof_25.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.proof_25.RowAt(id);
    if (db.proof_25.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.c0;
    return true;
  }
  return false;
}

Database::q_shared_f_cursor Database::q_shared_f() {
  return {*this, 0};
}

bool Database::q_shared_f_cursor::next(int32_t &X) {
  while (pos < db.q_shared_28.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.q_shared_28.RowAt(id);
    if (db.q_shared_28.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.c0;
    return true;
  }
  return false;
}

Database::q_fact_ff_cursor Database::q_fact_ff() {
  return {*this, 0};
}

bool Database::q_fact_ff_cursor::next(int32_t &A, int32_t &B) {
  while (pos < db.q_fact_31.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.q_fact_31.RowAt(id);
    if (db.q_fact_31.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    A = row.c0;
    B = row.c1;
    return true;
  }
  return false;
}

