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
      idx_48(allocator_),
      __15(allocator_),
      table_18(allocator_),
      table_21(allocator_),
      idx_96(allocator_),
      table_25(allocator_),
      table_28(allocator_),
      user_is_logged_in_32(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec71(allocator);
  ::hyde::rt::Vec<Tup_i32> vec72(allocator);
  proc_35(std::move(vec71), std::move(vec72), std::move(vec72));
  return false;
}

bool Database::proc_35(::hyde::rt::Vec<Tup_i32> vec37, ::hyde::rt::Vec<Tup_i32> vec42, ::hyde::rt::Vec<Tup_i32> vec43) {
  ::hyde::rt::Vec<Tup_b> vec40(allocator);
  ::hyde::rt::Vec<Tup_i32> vec41(allocator);
  for (auto [v39] : vec37) {
    if (const auto ins0 = table_8.TryChangeAbsentToPresent({v39}); ins0.changed) {
      if (const auto ins1 = table_11.TryChangeAbsentToPresent({v39, true}); ins1.changed) {
        if (ins1.added_row) {
          idx_48.Add({true}, ins1.id);
        }
        vec40.Add({true});
      }
      vec41.Add({v39});
    }
  }
  for (auto [v45] : vec42) {
    if (const auto ins2 = table_5.TryChangeAbsentToPresent({v45}); ins2.changed) {
      vec41.Add({v45});
    }
  }
  for (auto [v47] : vec43) {
    if (table_5.TryChangePresentToAbsent({v47})) {
      if (table_18.TryChangePresentToUnknown({v47})) {
        if (table_28.TryChangePresentToUnknown({true, v47})) {
          if (__15.TryChangePresentToUnknown({true})) {
            for (uint32_t s49 = idx_48.First({true}); s49 != ::hyde::rt::kNoRow; s49 = idx_48.Next(s49)) {
              const auto r49 = table_11.RowAt(s49);
              const auto v50 = r49.id;
              const auto v51 = r49.c1;
              if (true == v51) {
                if (table_21.TryChangePresentToUnknown({true, v50})) {
                  if (table_25.TryChangePresentToUnknown({v50})) {
                    if (user_is_logged_in_32.TryChangePresentToUnknown({v50})) {
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  vec37.Clear();
  vec42.Clear();
  vec43.Clear();
  flow_171(std::move(vec40), std::move(vec41));
  return false;
}

bool Database::add_user_1(::hyde::rt::Vec<Tup_i32> vec62) {
  ::hyde::rt::Vec<Tup_i32> vec64(allocator);
  proc_35(std::move(vec62), std::move(vec64), std::move(vec64));
  return true;
}

bool Database::log_in_1(::hyde::rt::Vec<Tup_i32> vec66, ::hyde::rt::Vec<Tup_i32> vec67) {
  ::hyde::rt::Vec<Tup_i32> vec69(allocator);
  proc_35(std::move(vec69), std::move(vec66), std::move(vec67));
  return true;
}

bool Database::find_73(int32_t v74) {
  switch (user_is_logged_in_32.State({v74})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (user_is_logged_in_32.TryChangeUnknownToAbsent({v74})) {
        if (find_75(v74)) {
          if (const auto ins3 = user_is_logged_in_32.TryChangeAbsentToPresent({v74}); ins3.changed) {
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
  if (find_73(v74)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_75(int32_t v76) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_79(v76)) {
    return true;
  }
  return false;
}

bool Database::find_79(int32_t v80) {
  switch (table_25.State({v80})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_25.TryChangeUnknownToAbsent({v80})) {
        if (find_82(v80)) {
          if (const auto ins4 = table_25.TryChangeAbsentToPresent({v80}); ins4.changed) {
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
  if (find_85(v80)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_82(int32_t v83) {
  if (find_90(v83)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_85(int32_t v86) {
  switch (table_25.State({v86})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_25.TryChangeUnknownToAbsent({v86})) {
        if (find_82(v86)) {
          if (const auto ins5 = table_25.TryChangeAbsentToPresent({v86}); ins5.changed) {
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
  if (find_85(v86)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_90(int32_t v91) {
  if (find_93(v91)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_93(int32_t v94) {
  for (uint32_t s97 = idx_96.First({v94}); s97 != ::hyde::rt::kNoRow; s97 = idx_96.Next(s97)) {
    const auto r97 = table_21.RowAt(s97);
    const auto v98 = r97.c0;
    const auto v99 = r97.id;
    if (v94 == v99) {
      if (find_100(v98, v99)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_100(bool v101, int32_t v102) {
  switch (table_21.State({v101, v102})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_21.TryChangeUnknownToAbsent({v101, v102})) {
        if (find_104(v101, v102)) {
          if (const auto ins6 = table_21.TryChangeAbsentToPresent({v101, v102}); ins6.changed) {
            if (ins6.added_row) {
              idx_96.Add({v102}, ins6.id);
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
  if (find_100(v101, v102)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_104(bool v105, int32_t v106) {
  if (find_109(v106, v105)) {
    if (find_113(v105)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_109(int32_t v110, bool v111) {
  switch (table_11.State({v110, v111})) {
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

bool Database::find_113(bool v114) {
  switch (__15.State({v114})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__15.TryChangeUnknownToAbsent({v114})) {
        if (find_116(v114)) {
          if (const auto ins7 = __15.TryChangeAbsentToPresent({v114}); ins7.changed) {
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
  if (find_119(v114)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_116(bool v117) {
  if (find_124(v117)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_119(bool v120) {
  switch (__15.State({v120})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__15.TryChangeUnknownToAbsent({v120})) {
        if (find_116(v120)) {
          if (const auto ins8 = __15.TryChangeAbsentToPresent({v120}); ins8.changed) {
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
  if (find_119(v120)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_124(bool v125) {
  for (uint32_t s127 = 0; s127 < table_28.NumRows(); ++s127) {
    const auto r127 = table_28.RowAt(s127);
    const auto v128 = r127.c0;
    const auto v129 = r127.id;
    if (v128 == v125) {
      if (find_130(v128, v129)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_130(bool v131, int32_t v132) {
  switch (table_28.State({v131, v132})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_28.TryChangeUnknownToAbsent({v131, v132})) {
        if (find_134(v131, v132)) {
          if (const auto ins9 = table_28.TryChangeAbsentToPresent({v131, v132}); ins9.changed) {
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
  if (find_130(v131, v132)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_134(bool v135, int32_t v136) {
  if (find_139(v135, v136)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_139(bool v140, int32_t v141) {
  if (find_143(v141)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_143(int32_t v144) {
  switch (table_18.State({v144})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v144})) {
        if (find_146(v144)) {
          if (const auto ins10 = table_18.TryChangeAbsentToPresent({v144}); ins10.changed) {
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
  if (find_149(v144)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_146(int32_t v147) {
  if (find_154(v147)) {
    if (find_157(v147)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_149(int32_t v150) {
  switch (table_18.State({v150})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v150})) {
        if (find_146(v150)) {
          if (const auto ins11 = table_18.TryChangeAbsentToPresent({v150}); ins11.changed) {
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
  if (find_149(v150)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_154(int32_t v155) {
  switch (table_5.State({v155})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v155})) {
        if (find_160(v155)) {
          if (const auto ins12 = table_5.TryChangeAbsentToPresent({v155}); ins12.changed) {
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
  if (find_163(v155)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_157(int32_t v158) {
  switch (table_8.State({v158})) {
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

bool Database::find_160(int32_t v161) {
  if (find_168(v161)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_163(int32_t v164) {
  switch (table_5.State({v164})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v164})) {
        if (find_160(v164)) {
          if (const auto ins13 = table_5.TryChangeAbsentToPresent({v164}); ins13.changed) {
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
  if (find_163(v164)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_168(int32_t v169) {
  return false;
}

bool Database::flow_171(::hyde::rt::Vec<Tup_b> vec40, ::hyde::rt::Vec<Tup_i32> vec41) {
  if ((g36 += 1) == 1) {
  }
  vec41.SortAndUnique();
  for (auto [v53] : vec41) {
    if (const uint32_t j52_0 = table_8.Find({v53}); j52_0 != ::hyde::rt::kNoRow) {
      const auto r52_0 = table_8.RowAt(j52_0);
      const auto v55 = r52_0.id;
      if (const uint32_t j52_1 = table_5.Find({v53}); j52_1 != ::hyde::rt::kNoRow) {
        const auto r52_1 = table_5.RowAt(j52_1);
        const auto v54 = r52_1.id;
        if (v53 == v54 && v53 == v55) {
          switch (table_5.State({v55})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins14 = table_18.TryChangeAbsentOrUnknownToPresent({v55}); ins14.changed) {
                if (const auto ins15 = table_28.TryChangeAbsentOrUnknownToPresent({true, v55}); ins15.changed) {
                  if (const auto ins16 = __15.TryChangeAbsentOrUnknownToPresent({true}); ins16.changed) {
                    vec40.Add({true});
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
  vec41.Clear();
  vec40.SortAndUnique();
  for (auto [v57] : vec40) {
    if (const uint32_t j56_0 = __15.Find({v57}); j56_0 != ::hyde::rt::kNoRow) {
      const auto r56_0 = __15.RowAt(j56_0);
      const auto v59 = r56_0.c0;
      for (uint32_t j56_1 = idx_48.First({v57}); j56_1 != ::hyde::rt::kNoRow; j56_1 = idx_48.Next(j56_1)) {
        const auto r56_1 = table_11.RowAt(j56_1);
        const auto v60 = r56_1.id;
        const auto v58 = r56_1.c1;
        if (v57 == v58 && v57 == v59) {
          switch (__15.State({v58})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins17 = table_21.TryChangeAbsentOrUnknownToPresent({v58, v60}); ins17.changed) {
                if (ins17.added_row) {
                  idx_96.Add({v60}, ins17.id);
                }
                if (const auto ins18 = table_25.TryChangeAbsentOrUnknownToPresent({v60}); ins18.changed) {
                  if (const auto ins19 = user_is_logged_in_32.TryChangeAbsentOrUnknownToPresent({v60}); ins19.changed) {
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
  vec40.Clear();
  return true;
}

bool Database::user_is_logged_in_b(int32_t UserId) {
  return find_73(UserId);
}

