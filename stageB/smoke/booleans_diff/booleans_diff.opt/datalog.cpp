// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      table_8(allocator_),
      table_11(allocator_),
      idx_45(allocator_),
      __15(allocator_),
      table_18(allocator_),
      table_21(allocator_),
      idx_79(allocator_),
      table_25(allocator_),
      user_is_logged_in_29(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec68(allocator);
  ::hyde::rt::Vec<Tup_i32> vec69(allocator);
  proc_32(std::move(vec68), std::move(vec69), std::move(vec69));
  return false;
}

bool Database::proc_32(::hyde::rt::Vec<Tup_i32> vec34, ::hyde::rt::Vec<Tup_i32> vec39, ::hyde::rt::Vec<Tup_i32> vec40) {
  ::hyde::rt::Vec<Tup_b> vec37(allocator);
  ::hyde::rt::Vec<Tup_i32> vec38(allocator);
  for (auto [v36] : vec34) {
    if (const auto ins0 = table_8.TryChangeAbsentToPresent({v36}); ins0.changed) {
      if (const auto ins1 = table_11.TryChangeAbsentToPresent({v36, true}); ins1.changed) {
        if (ins1.added_row) {
          idx_45.Add({true}, ins1.id);
        }
        vec37.Add({true});
      }
      vec38.Add({v36});
    }
  }
  for (auto [v42] : vec39) {
    if (const auto ins2 = table_5.TryChangeAbsentToPresent({v42}); ins2.changed) {
      vec38.Add({v42});
    }
  }
  for (auto [v44] : vec40) {
    if (table_5.TryChangePresentToAbsent({v44})) {
      if (table_18.TryChangePresentToUnknown({v44})) {
        if (table_25.TryChangePresentToUnknown({true, v44})) {
          if (__15.TryChangePresentToUnknown({true})) {
            for (uint32_t s46 = idx_45.First({true}); s46 != ::hyde::rt::kNoRow; s46 = idx_45.Next(s46)) {
              const auto r46 = table_11.RowAt(s46);
              const auto v47 = r46.id;
              const auto v48 = r46.c1;
              if (true == v48) {
                if (table_21.TryChangePresentToUnknown({true, v47})) {
                  user_is_logged_in_29.TryChangePresentToUnknown({v47});
                }
              }
            }
          }
        }
      }
    }
  }
  vec34.Clear();
  vec39.Clear();
  vec40.Clear();
  flow_150(std::move(vec37), std::move(vec38));
  return false;
}

bool Database::add_user_1(::hyde::rt::Vec<Tup_i32> vec59) {
  ::hyde::rt::Vec<Tup_i32> vec61(allocator);
  proc_32(std::move(vec59), std::move(vec61), std::move(vec61));
  return true;
}

bool Database::log_in_1(::hyde::rt::Vec<Tup_i32> vec63, ::hyde::rt::Vec<Tup_i32> vec64) {
  ::hyde::rt::Vec<Tup_i32> vec66(allocator);
  proc_32(std::move(vec66), std::move(vec63), std::move(vec64));
  return true;
}

bool Database::find_70(int32_t v71) {
  switch (user_is_logged_in_29.State({v71})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (user_is_logged_in_29.TryChangeUnknownToAbsent({v71})) {
        if (find_72(v71)) {
          if (const auto ins3 = user_is_logged_in_29.TryChangeAbsentToPresent({v71}); ins3.changed) {
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
  } else {
    return false;
  }
  return false;
}

bool Database::find_76(int32_t v77) {
  for (uint32_t s80 = idx_79.First({v77}); s80 != ::hyde::rt::kNoRow; s80 = idx_79.Next(s80)) {
    const auto r80 = table_21.RowAt(s80);
    const auto v81 = r80.c0;
    const auto v82 = r80.id;
    if (v82 == v77) {
      if (find_83(v81, v82)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_83(bool v84, int32_t v85) {
  switch (table_21.State({v84, v85})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_21.TryChangeUnknownToAbsent({v84, v85})) {
        if (find_87(v84, v85)) {
          if (const auto ins4 = table_21.TryChangeAbsentToPresent({v84, v85}); ins4.changed) {
            if (ins4.added_row) {
              idx_79.Add({v85}, ins4.id);
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
  if (find_83(v84, v85)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_87(bool v88, int32_t v89) {
  if (find_92(v89, v88)) {
    if (find_96(v88)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_92(int32_t v93, bool v94) {
  switch (table_11.State({v93, v94})) {
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

bool Database::find_96(bool v97) {
  switch (__15.State({v97})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__15.TryChangeUnknownToAbsent({v97})) {
        if (find_99(v97)) {
          if (const auto ins5 = __15.TryChangeAbsentToPresent({v97}); ins5.changed) {
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
  if (find_96(v97)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_99(bool v100) {
  if (find_107(v100)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_107(bool v108) {
  for (uint32_t s110 = 0; s110 < table_25.NumRows(); ++s110) {
    const auto r110 = table_25.RowAt(s110);
    const auto v111 = r110.c0;
    const auto v112 = r110.id;
    if (v111 == v108) {
      if (find_113(v111, v112)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_113(bool v114, int32_t v115) {
  switch (table_25.State({v114, v115})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_25.TryChangeUnknownToAbsent({v114, v115})) {
        if (find_117(v114, v115)) {
          if (const auto ins6 = table_25.TryChangeAbsentToPresent({v114, v115}); ins6.changed) {
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
  if (find_113(v114, v115)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_117(bool v118, int32_t v119) {
  if (find_122(v119)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_122(int32_t v123) {
  switch (table_18.State({v123})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v123})) {
        if (find_125(v123)) {
          if (const auto ins7 = table_18.TryChangeAbsentToPresent({v123}); ins7.changed) {
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
  if (find_122(v123)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_125(int32_t v126) {
  if (find_133(v126)) {
    if (find_136(v126)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_133(int32_t v134) {
  switch (table_5.State({v134})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v134})) {
        if (find_139(v134)) {
          if (const auto ins8 = table_5.TryChangeAbsentToPresent({v134}); ins8.changed) {
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
  if (find_133(v134)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_136(int32_t v137) {
  switch (table_8.State({v137})) {
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

bool Database::find_139(int32_t v140) {
  if (find_147(v140)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_147(int32_t v148) {
  return false;
}

bool Database::flow_150(::hyde::rt::Vec<Tup_b> vec37, ::hyde::rt::Vec<Tup_i32> vec38) {
  g33 += 1;
  vec38.SortAndUnique();
  for (auto [v50] : vec38) {
    if (const uint32_t j49_0 = table_8.Find({v50}); j49_0 != ::hyde::rt::kNoRow) {
      const auto r49_0 = table_8.RowAt(j49_0);
      const auto v52 = r49_0.id;
      if (const uint32_t j49_1 = table_5.Find({v50}); j49_1 != ::hyde::rt::kNoRow) {
        const auto r49_1 = table_5.RowAt(j49_1);
        const auto v51 = r49_1.id;
        if (v50 == v51 && v50 == v52) {
          switch (table_5.State({v52})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins9 = table_18.TryChangeAbsentOrUnknownToPresent({v52}); ins9.changed) {
                if (const auto ins10 = table_25.TryChangeAbsentOrUnknownToPresent({true, v52}); ins10.changed) {
                  if (const auto ins11 = __15.TryChangeAbsentOrUnknownToPresent({true}); ins11.changed) {
                    vec37.Add({true});
                  }
                }
              }
              break;
            }
            default: break;
          }
        }
      }
    }
  }
  vec38.Clear();
  vec37.SortAndUnique();
  for (auto [v54] : vec37) {
    if (const uint32_t j53_0 = __15.Find({v54}); j53_0 != ::hyde::rt::kNoRow) {
      const auto r53_0 = __15.RowAt(j53_0);
      const auto v56 = r53_0.c0;
      for (uint32_t j53_1 = idx_45.First({v54}); j53_1 != ::hyde::rt::kNoRow; j53_1 = idx_45.Next(j53_1)) {
        const auto r53_1 = table_11.RowAt(j53_1);
        const auto v57 = r53_1.id;
        const auto v55 = r53_1.c1;
        if (v54 == v55 && v54 == v56) {
          switch (__15.State({v55})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins12 = table_21.TryChangeAbsentOrUnknownToPresent({v55, v57}); ins12.changed) {
                if (ins12.added_row) {
                  idx_79.Add({v57}, ins12.id);
                }
                user_is_logged_in_29.TryChangeAbsentOrUnknownToPresent({v57});
              }
              break;
            }
            default: break;
          }
        }
      }
    }
  }
  vec37.Clear();
  return true;
}

bool Database::user_is_logged_in_b(int32_t UserId) {
  return find_70(UserId);
}

