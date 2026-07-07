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
      idx_64(allocator_),
      table_39(allocator_),
      idx_58(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec84(allocator);
  ::hyde::rt::Vec<Tup_i32> vec85(allocator);
  ::hyde::rt::Vec<Tup_i32> vec86(allocator);
  proc_43(std::move(vec84), std::move(vec85), std::move(vec86));
  return false;
}

bool Database::proc_43(::hyde::rt::Vec<Tup_i32> vec47, ::hyde::rt::Vec<Tup_i32> vec50, ::hyde::rt::Vec<Tup_i32> vec53) {
  ::hyde::rt::Vec<Tup_b> vec45(allocator);
  ::hyde::rt::Vec<Tup_b> vec46(allocator);
  for (auto [v49] : vec47) {
    if (const auto ins0 = table_10.TryChangeAbsentToPresent({true, v49}); ins0.changed) {
      if (const auto ins1 = __7.TryChangeAbsentToPresent({true}); ins1.changed) {
        vec45.Add({true});
      }
    }
  }
  for (auto [v52] : vec50) {
    if (const auto ins2 = table_17.TryChangeAbsentToPresent({true, v52}); ins2.changed) {
      if (const auto ins3 = __14.TryChangeAbsentToPresent({true}); ins3.changed) {
        vec46.Add({true});
      }
    }
  }
  for (auto [v55] : vec53) {
    if (const auto ins4 = table_21.TryChangeAbsentToPresent({true, v55}); ins4.changed) {
      if (const auto ins5 = __14.TryChangeAbsentToPresent({true}); ins5.changed) {
        vec46.Add({true});
      }
    }
  }
  vec47.Clear();
  vec50.Clear();
  vec53.Clear();
  flow_87(std::move(vec45), std::move(vec46));
  return false;
}

bool Database::something_1(::hyde::rt::Vec<Tup_i32> vec69) {
  ::hyde::rt::Vec<Tup_i32> vec71(allocator);
  ::hyde::rt::Vec<Tup_i32> vec72(allocator);
  proc_43(std::move(vec69), std::move(vec71), std::move(vec72));
  return true;
}

bool Database::a_1(::hyde::rt::Vec<Tup_i32> vec74) {
  ::hyde::rt::Vec<Tup_i32> vec76(allocator);
  ::hyde::rt::Vec<Tup_i32> vec77(allocator);
  proc_43(std::move(vec76), std::move(vec74), std::move(vec77));
  return true;
}

bool Database::b_1(::hyde::rt::Vec<Tup_i32> vec79) {
  ::hyde::rt::Vec<Tup_i32> vec81(allocator);
  ::hyde::rt::Vec<Tup_i32> vec82(allocator);
  proc_43(std::move(vec81), std::move(vec82), std::move(vec79));
  return true;
}

bool Database::flow_87(::hyde::rt::Vec<Tup_b> vec45, ::hyde::rt::Vec<Tup_b> vec46) {
  if ((g44 += 1) == 1) {
    q_fact_31.TryChangeAbsentToPresent({1, 2});
    if (const auto ins6 = table_35.TryChangeAbsentToPresent({1, true}); ins6.changed) {
      if (ins6.added_row) {
        idx_64.Add({true}, ins6.id);
      }
      vec45.Add({true});
    }
    if (const auto ins7 = table_39.TryChangeAbsentToPresent({1, true}); ins7.changed) {
      if (ins7.added_row) {
        idx_58.Add({true}, ins7.id);
      }
      vec46.Add({true});
    }
  }
  vec46.SortAndUnique();
  for (auto [v57] : vec46) {
    if (const uint32_t j56_0 = __14.Find({v57}); j56_0 != ::hyde::rt::kNoRow) {
      const auto r56_0 = __14.RowAt(j56_0);
      const auto v60 = r56_0.c0;
      for (uint32_t j56_1 = idx_58.First({v57}); j56_1 != ::hyde::rt::kNoRow; j56_1 = idx_58.Next(j56_1)) {
        const auto r56_1 = table_39.RowAt(j56_1);
        const auto v61 = r56_1.c0;
        const auto v59 = r56_1.c1;
        if (v57 == v59 && v57 == v60) {
          q_shared_28.TryChangeAbsentToPresent({v61});
        }
      }
    }
  }
  vec46.Clear();
  vec45.SortAndUnique();
  for (auto [v63] : vec45) {
    if (const uint32_t j62_0 = __7.Find({v63}); j62_0 != ::hyde::rt::kNoRow) {
      const auto r62_0 = __7.RowAt(j62_0);
      const auto v66 = r62_0.c0;
      for (uint32_t j62_1 = idx_64.First({v63}); j62_1 != ::hyde::rt::kNoRow; j62_1 = idx_64.Next(j62_1)) {
        const auto r62_1 = table_35.RowAt(j62_1);
        const auto v67 = r62_1.c0;
        const auto v65 = r62_1.c1;
        if (v63 == v65 && v63 == v66) {
          proof_25.TryChangeAbsentToPresent({v67});
        }
      }
    }
  }
  vec45.Clear();
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
    X = row.a;
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
    X = row.a;
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
    A = row.a;
    B = row.b;
    return true;
  }
  return false;
}

