// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_93(allocator_),
      table_9(allocator_),
      off_vals_12(allocator_),
      __15(allocator_),
      table_18(allocator_),
      on_vals_22(allocator_),
      table_25(allocator_),
      idx_55(allocator_),
      table_29(allocator_),
      idx_38(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec68(allocator);
  ::hyde::rt::Vec<Tup_i32> vec69(allocator);
  proc_33(std::move(vec68), std::move(vec69));
  return false;
}

bool Database::proc_33(::hyde::rt::Vec<Tup_i32> vec35, ::hyde::rt::Vec<Tup_i32> vec47) {
  ::hyde::rt::Vec<Tup_b> vec46(allocator);
  for (auto [v37] : vec35) {
    if (const auto ins0 = table_18.TryChangeAbsentToPresent({true, v37}); ins0.changed) {
      if (const auto ins1 = __15.TryChangeAbsentToPresent({true}); ins1.changed) {
        for (uint32_t s39 = idx_38.First({true}); s39 != ::hyde::rt::kNoRow; s39 = idx_38.Next(s39)) {
          const auto r39 = table_29.RowAt(s39);
          const auto v40 = r39.x;
          const auto v41 = r39.c1;
          if (true == v41) {
            if (find_42(v40, v41)) {
              if (table_5.TryChangePresentToUnknown({v41, v40})) {
                if (table_9.TryChangePresentToUnknown({v40})) {
                  off_vals_12.TryChangePresentToUnknown({v40});
                }
              }
            }
          }
        }
        vec46.Add({true});
      }
    }
  }
  for (auto [v49] : vec47) {
    if (const auto ins2 = table_25.TryChangeAbsentToPresent({v49, true}); ins2.changed) {
      if (ins2.added_row) {
        idx_55.Add({true}, ins2.id);
      }
      vec46.Add({true});
    }
    if (const auto ins3 = table_29.TryChangeAbsentToPresent({v49, true}); ins3.changed) {
      if (ins3.added_row) {
        idx_38.Add({true}, ins3.id);
      }
      if (!find_50(true)) {
        if (const auto ins4 = table_5.TryChangeAbsentOrUnknownToPresent({true, v49}); ins4.changed) {
          if (ins4.added_row) {
            idx_93.Add({v49}, ins4.id);
          }
          if (const auto ins5 = table_9.TryChangeAbsentOrUnknownToPresent({v49}); ins5.changed) {
            off_vals_12.TryChangeAbsentOrUnknownToPresent({v49});
          }
        }
      }
    }
  }
  vec35.Clear();
  vec47.Clear();
  flow_111(std::move(vec46));
  return false;
}

bool Database::find_42(int32_t v43, bool v44) {
  switch (table_29.State({v43, v44})) {
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

bool Database::find_50(bool v51) {
  switch (__15.State({v51})) {
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

bool Database::log_in_1(::hyde::rt::Vec<Tup_i32> vec60) {
  ::hyde::rt::Vec<Tup_i32> vec62(allocator);
  proc_33(std::move(vec60), std::move(vec62));
  return true;
}

bool Database::m_1(::hyde::rt::Vec<Tup_i32> vec64) {
  ::hyde::rt::Vec<Tup_i32> vec66(allocator);
  proc_33(std::move(vec66), std::move(vec64));
  return true;
}

bool Database::find_70(int32_t v71) {
  switch (off_vals_12.State({v71})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (off_vals_12.TryChangeUnknownToAbsent({v71})) {
        if (find_72(v71)) {
          if (const auto ins6 = off_vals_12.TryChangeAbsentToPresent({v71}); ins6.changed) {
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
  if (find_70(v71)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_72(int32_t v73) {
  if (find_76(v73)) {
    return true;
  }
  return false;
}

bool Database::find_76(int32_t v77) {
  switch (table_9.State({v77})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_9.TryChangeUnknownToAbsent({v77})) {
        if (find_79(v77)) {
          if (const auto ins7 = table_9.TryChangeAbsentToPresent({v77}); ins7.changed) {
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
  if (find_76(v77)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_79(int32_t v80) {
  if (find_87(v80)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_87(int32_t v88) {
  if (find_90(v88)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_90(int32_t v91) {
  for (uint32_t s94 = idx_93.First({v91}); s94 != ::hyde::rt::kNoRow; s94 = idx_93.Next(s94)) {
    const auto r94 = table_5.RowAt(s94);
    const auto v95 = r94.c0;
    const auto v96 = r94.x;
    if (v96 == v91) {
      if (find_97(v95, v96)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_97(bool v98, int32_t v99) {
  switch (table_5.State({v98, v99})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v98, v99})) {
        if (find_101(v98, v99)) {
          if (const auto ins8 = table_5.TryChangeAbsentToPresent({v98, v99}); ins8.changed) {
            if (ins8.added_row) {
              idx_93.Add({v99}, ins8.id);
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
  if (find_97(v98, v99)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_101(bool v102, int32_t v103) {
  if (find_42(v103, v102)) {
    if (find_50(v102)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::flow_111(::hyde::rt::Vec<Tup_b> vec46) {
  g34 += 1;
  vec46.SortAndUnique();
  for (auto [v54] : vec46) {
    if (const uint32_t j53_0 = __15.Find({v54}); j53_0 != ::hyde::rt::kNoRow) {
      const auto r53_0 = __15.RowAt(j53_0);
      const auto v57 = r53_0.c0;
      for (uint32_t j53_1 = idx_55.First({v54}); j53_1 != ::hyde::rt::kNoRow; j53_1 = idx_55.Next(j53_1)) {
        const auto r53_1 = table_25.RowAt(j53_1);
        const auto v58 = r53_1.x;
        const auto v56 = r53_1.c1;
        if (v54 == v56 && v54 == v57) {
          on_vals_22.TryChangeAbsentToPresent({v58});
        }
      }
    }
  }
  vec46.Clear();
  return true;
}

Database::on_vals_f_cursor Database::on_vals_f() {
  return {*this, 0};
}

bool Database::on_vals_f_cursor::next(int32_t &X) {
  while (pos < db.on_vals_22.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.on_vals_22.RowAt(id);
    if (db.on_vals_22.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
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
  while (pos < db.off_vals_12.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.off_vals_12.RowAt(id);
    if (!db.find_70(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

