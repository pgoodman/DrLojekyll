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
      idx_51(allocator_),
      __15(allocator_),
      table_18(allocator_),
      table_21(allocator_),
      table_24(allocator_),
      idx_99(allocator_),
      table_28(allocator_),
      table_31(allocator_),
      user_is_logged_in_35(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec74(allocator);
  ::hyde::rt::Vec<Tup_i32> vec75(allocator);
  proc_38(std::move(vec74), std::move(vec75), std::move(vec75));
  return false;
}

bool Database::proc_38(::hyde::rt::Vec<Tup_i32> vec40, ::hyde::rt::Vec<Tup_i32> vec45, ::hyde::rt::Vec<Tup_i32> vec46) {
  ::hyde::rt::Vec<Tup_b> vec43(allocator);
  ::hyde::rt::Vec<Tup_i32> vec44(allocator);
  for (auto [v42] : vec40) {
    if (const auto ins0 = table_8.TryChangeAbsentToPresent({v42}); ins0.changed) {
      if (const auto ins1 = table_11.TryChangeAbsentToPresent({v42, true}); ins1.changed) {
        if (ins1.added_row) {
          idx_51.Add({true}, ins1.id);
        }
        vec43.Add({true});
      }
      vec44.Add({v42});
    }
  }
  for (auto [v48] : vec45) {
    if (const auto ins2 = table_18.TryChangeAbsentToPresent({v48}); ins2.changed) {
      if (const auto ins3 = table_5.TryChangeAbsentOrUnknownToPresent({v48}); ins3.changed) {
        vec44.Add({v48});
      }
    }
  }
  for (auto [v50] : vec46) {
    if (table_18.TryChangePresentToAbsent({v50})) {
      if (table_5.TryChangePresentToUnknown({v50})) {
        if (table_21.TryChangePresentToUnknown({v50})) {
          if (table_31.TryChangePresentToUnknown({true, v50})) {
            if (__15.TryChangePresentToUnknown({true})) {
              for (uint32_t s52 = idx_51.First({true}); s52 != ::hyde::rt::kNoRow; s52 = idx_51.Next(s52)) {
                const auto r52 = table_11.RowAt(s52);
                const auto v53 = r52.id;
                const auto v54 = r52.c1;
                if (true == v54) {
                  if (table_24.TryChangePresentToUnknown({true, v53})) {
                    if (table_28.TryChangePresentToUnknown({v53})) {
                      if (user_is_logged_in_35.TryChangePresentToUnknown({v53})) {
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
  }
  vec40.Clear();
  vec45.Clear();
  vec46.Clear();
  flow_191(std::move(vec43), std::move(vec44));
  return false;
}

bool Database::add_user_1(::hyde::rt::Vec<Tup_i32> vec65) {
  ::hyde::rt::Vec<Tup_i32> vec67(allocator);
  proc_38(std::move(vec65), std::move(vec67), std::move(vec67));
  return true;
}

bool Database::log_in_1(::hyde::rt::Vec<Tup_i32> vec69, ::hyde::rt::Vec<Tup_i32> vec70) {
  ::hyde::rt::Vec<Tup_i32> vec72(allocator);
  proc_38(std::move(vec72), std::move(vec69), std::move(vec70));
  return true;
}

bool Database::find_76(int32_t v77) {
  switch (user_is_logged_in_35.State({v77})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (user_is_logged_in_35.TryChangeUnknownToAbsent({v77})) {
        if (find_78(v77)) {
          if (const auto ins4 = user_is_logged_in_35.TryChangeAbsentToPresent({v77}); ins4.changed) {
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

bool Database::find_78(int32_t v79) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_82(v79)) {
    return true;
  }
  return false;
}

bool Database::find_82(int32_t v83) {
  switch (table_28.State({v83})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_28.TryChangeUnknownToAbsent({v83})) {
        if (find_85(v83)) {
          if (const auto ins5 = table_28.TryChangeAbsentToPresent({v83}); ins5.changed) {
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
  if (find_88(v83)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_85(int32_t v86) {
  if (find_93(v86)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_88(int32_t v89) {
  switch (table_28.State({v89})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_28.TryChangeUnknownToAbsent({v89})) {
        if (find_85(v89)) {
          if (const auto ins6 = table_28.TryChangeAbsentToPresent({v89}); ins6.changed) {
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
  if (find_88(v89)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_93(int32_t v94) {
  if (find_96(v94)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_96(int32_t v97) {
  for (uint32_t s100 = idx_99.First({v97}); s100 != ::hyde::rt::kNoRow; s100 = idx_99.Next(s100)) {
    const auto r100 = table_24.RowAt(s100);
    const auto v101 = r100.c0;
    const auto v102 = r100.id;
    if (v97 == v102) {
      if (find_103(v101, v102)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_103(bool v104, int32_t v105) {
  switch (table_24.State({v104, v105})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_24.TryChangeUnknownToAbsent({v104, v105})) {
        if (find_107(v104, v105)) {
          if (const auto ins7 = table_24.TryChangeAbsentToPresent({v104, v105}); ins7.changed) {
            if (ins7.added_row) {
              idx_99.Add({v105}, ins7.id);
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
  if (find_103(v104, v105)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_107(bool v108, int32_t v109) {
  if (find_112(v109, v108)) {
    if (find_116(v108)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_112(int32_t v113, bool v114) {
  switch (table_11.State({v113, v114})) {
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

bool Database::find_116(bool v117) {
  switch (__15.State({v117})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__15.TryChangeUnknownToAbsent({v117})) {
        if (find_119(v117)) {
          if (const auto ins8 = __15.TryChangeAbsentToPresent({v117}); ins8.changed) {
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
  if (find_122(v117)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_119(bool v120) {
  if (find_127(v120)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_122(bool v123) {
  switch (__15.State({v123})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__15.TryChangeUnknownToAbsent({v123})) {
        if (find_119(v123)) {
          if (const auto ins9 = __15.TryChangeAbsentToPresent({v123}); ins9.changed) {
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

bool Database::find_127(bool v128) {
  for (uint32_t s130 = 0; s130 < table_31.NumRows(); ++s130) {
    const auto r130 = table_31.RowAt(s130);
    const auto v131 = r130.c0;
    const auto v132 = r130.id;
    if (v131 == v128) {
      if (find_133(v131, v132)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_133(bool v134, int32_t v135) {
  switch (table_31.State({v134, v135})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_31.TryChangeUnknownToAbsent({v134, v135})) {
        if (find_137(v134, v135)) {
          if (const auto ins10 = table_31.TryChangeAbsentToPresent({v134, v135}); ins10.changed) {
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
  if (find_133(v134, v135)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_137(bool v138, int32_t v139) {
  if (find_142(v138, v139)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_142(bool v143, int32_t v144) {
  if (find_146(v144)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_146(int32_t v147) {
  switch (table_21.State({v147})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_21.TryChangeUnknownToAbsent({v147})) {
        if (find_149(v147)) {
          if (const auto ins11 = table_21.TryChangeAbsentToPresent({v147}); ins11.changed) {
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
  if (find_152(v147)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_149(int32_t v150) {
  if (find_157(v150)) {
    if (find_160(v150)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_152(int32_t v153) {
  switch (table_21.State({v153})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_21.TryChangeUnknownToAbsent({v153})) {
        if (find_149(v153)) {
          if (const auto ins12 = table_21.TryChangeAbsentToPresent({v153}); ins12.changed) {
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
  if (find_152(v153)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_157(int32_t v158) {
  switch (table_5.State({v158})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v158})) {
        if (find_163(v158)) {
          if (const auto ins13 = table_5.TryChangeAbsentToPresent({v158}); ins13.changed) {
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
  if (find_166(v158)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_160(int32_t v161) {
  switch (table_8.State({v161})) {
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

bool Database::find_163(int32_t v164) {
  if (find_169(v164)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_166(int32_t v167) {
  switch (table_5.State({v167})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v167})) {
        if (find_169(v167)) {
          if (const auto ins14 = table_5.TryChangeAbsentToPresent({v167}); ins14.changed) {
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
  if (find_166(v167)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_169(int32_t v170) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_173(v170)) {
    return true;
  }
  return false;
}

bool Database::find_173(int32_t v174) {
  switch (table_18.State({v174})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v174})) {
        if (find_176(v174)) {
          if (const auto ins15 = table_18.TryChangeAbsentToPresent({v174}); ins15.changed) {
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
  if (find_179(v174)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_176(int32_t v177) {
  if (find_184(v177)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_179(int32_t v180) {
  switch (table_18.State({v180})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v180})) {
        if (find_176(v180)) {
          if (const auto ins16 = table_18.TryChangeAbsentToPresent({v180}); ins16.changed) {
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
  if (find_179(v180)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_184(int32_t v185) {
  if (find_187(v185)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_187(int32_t v188) {
  return false;
}

bool Database::flow_191(::hyde::rt::Vec<Tup_b> vec43, ::hyde::rt::Vec<Tup_i32> vec44) {
  if ((g39 += 1) == 1) {
  }
  vec44.SortAndUnique();
  for (auto [v56] : vec44) {
    if (const uint32_t j55_0 = table_8.Find({v56}); j55_0 != ::hyde::rt::kNoRow) {
      const auto r55_0 = table_8.RowAt(j55_0);
      const auto v58 = r55_0.id;
      if (const uint32_t j55_1 = table_5.Find({v56}); j55_1 != ::hyde::rt::kNoRow) {
        const auto r55_1 = table_5.RowAt(j55_1);
        const auto v57 = r55_1.id;
        if (v56 == v57 && v56 == v58) {
          switch (table_5.State({v58})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins17 = table_21.TryChangeAbsentOrUnknownToPresent({v58}); ins17.changed) {
                if (const auto ins18 = table_31.TryChangeAbsentOrUnknownToPresent({true, v58}); ins18.changed) {
                  if (const auto ins19 = __15.TryChangeAbsentOrUnknownToPresent({true}); ins19.changed) {
                    vec43.Add({true});
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
  vec44.Clear();
  vec43.SortAndUnique();
  for (auto [v60] : vec43) {
    if (const uint32_t j59_0 = __15.Find({v60}); j59_0 != ::hyde::rt::kNoRow) {
      const auto r59_0 = __15.RowAt(j59_0);
      const auto v62 = r59_0.c0;
      for (uint32_t j59_1 = idx_51.First({v60}); j59_1 != ::hyde::rt::kNoRow; j59_1 = idx_51.Next(j59_1)) {
        const auto r59_1 = table_11.RowAt(j59_1);
        const auto v63 = r59_1.id;
        const auto v61 = r59_1.c1;
        if (v60 == v61 && v60 == v62) {
          switch (__15.State({v61})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins20 = table_24.TryChangeAbsentOrUnknownToPresent({v61, v63}); ins20.changed) {
                if (ins20.added_row) {
                  idx_99.Add({v63}, ins20.id);
                }
                if (const auto ins21 = table_28.TryChangeAbsentOrUnknownToPresent({v63}); ins21.changed) {
                  if (const auto ins22 = user_is_logged_in_35.TryChangeAbsentOrUnknownToPresent({v63}); ins22.changed) {
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
  vec43.Clear();
  return true;
}

bool Database::user_is_logged_in_b(int32_t UserId) {
  return find_76(UserId);
}

