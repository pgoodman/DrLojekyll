// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_72(allocator_),
      off_vals_9(allocator_),
      __12(allocator_),
      table_15(allocator_),
      on_vals_19(allocator_),
      table_22(allocator_),
      idx_48(allocator_),
      table_26(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec61(allocator);
  ::hyde::rt::Vec<Tup_i32> vec62(allocator);
  proc_29(std::move(vec61), std::move(vec62));
  return false;
}

bool Database::proc_29(::hyde::rt::Vec<Tup_i32> vec31, ::hyde::rt::Vec<Tup_i32> vec40) {
  ::hyde::rt::Vec<Tup_b> vec39(allocator);
  for (auto [v33] : vec31) {
    if (const auto ins0 = table_15.TryChangeAbsentToPresent({true, v33}); ins0.changed) {
      if (const auto ins1 = __12.TryChangeAbsentToPresent({true}); ins1.changed) {
        for (uint32_t s34 = 0; s34 < table_26.NumRows(); ++s34) {
          const auto r34 = table_26.RowAt(s34);
          const auto v35 = r34.x;
          if (find_36(v35)) {
            if (table_5.TryChangePresentToUnknown({true, v35})) {
              off_vals_9.TryChangePresentToUnknown({v35});
            }
          }
        }
        vec39.Add({true});
      }
    }
  }
  for (auto [v42] : vec40) {
    if (const auto ins2 = table_26.TryChangeAbsentToPresent({v42}); ins2.changed) {
      if (!find_43(true)) {
        if (const auto ins3 = table_5.TryChangeAbsentOrUnknownToPresent({true, v42}); ins3.changed) {
          if (ins3.added_row) {
            idx_72.Add({v42}, ins3.id);
          }
          off_vals_9.TryChangeAbsentOrUnknownToPresent({v42});
        }
      }
      if (const auto ins4 = table_22.TryChangeAbsentToPresent({v42, true}); ins4.changed) {
        if (ins4.added_row) {
          idx_48.Add({true}, ins4.id);
        }
        vec39.Add({true});
      }
    }
  }
  vec31.Clear();
  vec40.Clear();
  flow_89(std::move(vec39));
  return false;
}

bool Database::find_36(int32_t v37) {
  switch (table_26.State({v37})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      return false;
      break;
    }
    default: break;
  }
  return false;
}

bool Database::find_43(bool v44) {
  switch (__12.State({v44})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      return false;
      break;
    }
    default: break;
  }
  return false;
}

bool Database::log_in_1(::hyde::rt::Vec<Tup_i32> vec53) {
  ::hyde::rt::Vec<Tup_i32> vec55(allocator);
  proc_29(std::move(vec53), std::move(vec55));
  return true;
}

bool Database::m_1(::hyde::rt::Vec<Tup_i32> vec57) {
  ::hyde::rt::Vec<Tup_i32> vec59(allocator);
  proc_29(std::move(vec59), std::move(vec57));
  return true;
}

bool Database::find_63(int32_t v64) {
  switch (off_vals_9.State({v64})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (off_vals_9.TryChangeUnknownToAbsent({v64})) {
        if (find_65(v64)) {
          if (const auto ins5 = off_vals_9.TryChangeAbsentToPresent({v64}); ins5.changed) {
            return true;
          }
        } else {
          return false;
        }
      }
      break;
    }
    default: break;
  }
  if (find_63(v64)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_65(int32_t v66) {
  if (find_69(v66)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_69(int32_t v70) {
  for (uint32_t s73 = idx_72.First({v70}); s73 != ::hyde::rt::kNoRow; s73 = idx_72.Next(s73)) {
    const auto r73 = table_5.RowAt(s73);
    const auto v74 = r73.c0;
    const auto v75 = r73.x;
    if (v75 == v70) {
      if (find_76(v74, v75)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_76(bool v77, int32_t v78) {
  switch (table_5.State({v77, v78})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v77, v78})) {
        if (find_80(v77, v78)) {
          if (const auto ins6 = table_5.TryChangeAbsentToPresent({v77, v78}); ins6.changed) {
            if (ins6.added_row) {
              idx_72.Add({v78}, ins6.id);
            }
            return true;
          }
        } else {
          return false;
        }
      }
      break;
    }
    default: break;
  }
  if (find_76(v77, v78)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_80(bool v81, int32_t v82) {
  if (find_36(v82)) {
    if (find_43(v81)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::flow_89(::hyde::rt::Vec<Tup_b> vec39) {
  g30 += 1;
  vec39.SortAndUnique();
  for (auto [v47] : vec39) {
    if (const uint32_t j46_0 = __12.Find({v47}); j46_0 != ::hyde::rt::kNoRow) {
      const auto r46_0 = __12.RowAt(j46_0);
      const auto v50 = r46_0.c0;
      for (uint32_t j46_1 = idx_48.First({v47}); j46_1 != ::hyde::rt::kNoRow; j46_1 = idx_48.Next(j46_1)) {
        const auto r46_1 = table_22.RowAt(j46_1);
        const auto v51 = r46_1.x;
        const auto v49 = r46_1.c1;
        if (v47 == v49 && v47 == v50) {
          on_vals_19.TryChangeAbsentToPresent({v51});
        }
      }
    }
  }
  vec39.Clear();
  return true;
}

Database::on_vals_f_cursor Database::on_vals_f() {
  return {*this, 0};
}

bool Database::on_vals_f_cursor::next(int32_t &X) {
  while (pos < db.on_vals_19.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.on_vals_19.RowAt(id);
    if (db.on_vals_19.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

Database::off_vals_f_cursor Database::off_vals_f() {
  return {*this, 0};
}

bool Database::off_vals_f_cursor::next(int32_t &X) {
  while (pos < db.off_vals_9.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.off_vals_9.RowAt(id);
    if (!db.find_63(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

