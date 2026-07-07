// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_51(allocator_),
      __9(allocator_),
      table_12(allocator_),
      table_15(allocator_),
      table_18(allocator_),
      table_21(allocator_),
      table_24(allocator_),
      idx_116(allocator_),
      table_28(allocator_),
      table_31(allocator_),
      table_35(allocator_),
      gated_39(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec90(allocator);
  ::hyde::rt::Vec<Tup_i32> vec91(allocator);
  ::hyde::rt::Vec<Tup_i32> vec92(allocator);
  proc_42(std::move(vec90), std::move(vec90), std::move(vec91), std::move(vec91), std::move(vec92));
  return false;
}

bool Database::proc_42(::hyde::rt::Vec<Tup_i32> vec44, ::hyde::rt::Vec<Tup_i32> vec45, ::hyde::rt::Vec<Tup_i32> vec55, ::hyde::rt::Vec<Tup_i32> vec56, ::hyde::rt::Vec<Tup_i32> vec64) {
  ::hyde::rt::Vec<Tup_b> vec48(allocator);
  for (auto [v47] : vec44) {
    if (const auto ins0 = table_12.TryChangeAbsentToPresent({v47}); ins0.changed) {
      if (const auto ins1 = table_18.TryChangeAbsentOrUnknownToPresent({v47}); ins1.changed) {
        if (const auto ins2 = table_31.TryChangeAbsentOrUnknownToPresent({true, v47}); ins2.changed) {
          if (const auto ins3 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins3.changed) {
            vec48.Add({true});
          }
        }
      }
    }
  }
  for (auto [v50] : vec45) {
    if (table_12.TryChangePresentToAbsent({v50})) {
      if (table_18.TryChangePresentToUnknown({v50})) {
        if (table_31.TryChangePresentToUnknown({true, v50})) {
          if (__9.TryChangePresentToUnknown({true})) {
            for (uint32_t s52 = idx_51.First({true}); s52 != ::hyde::rt::kNoRow; s52 = idx_51.Next(s52)) {
              const auto r52 = table_5.RowAt(s52);
              const auto v53 = r52.x;
              const auto v54 = r52.c1;
              if (true == v54) {
                if (table_24.TryChangePresentToUnknown({true, v53})) {
                  if (table_28.TryChangePresentToUnknown({v53})) {
                    if (gated_39.TryChangePresentToUnknown({v53})) {
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
  for (auto [v58] : vec55) {
    if (const auto ins4 = table_15.TryChangeAbsentToPresent({v58}); ins4.changed) {
      if (const auto ins5 = table_21.TryChangeAbsentOrUnknownToPresent({v58}); ins5.changed) {
        if (const auto ins6 = table_35.TryChangeAbsentOrUnknownToPresent({true, v58}); ins6.changed) {
          if (const auto ins7 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins7.changed) {
            vec48.Add({true});
          }
        }
      }
    }
  }
  for (auto [v60] : vec56) {
    if (table_15.TryChangePresentToAbsent({v60})) {
      if (table_21.TryChangePresentToUnknown({v60})) {
        if (table_35.TryChangePresentToUnknown({true, v60})) {
          if (__9.TryChangePresentToUnknown({true})) {
            for (uint32_t s61 = idx_51.First({true}); s61 != ::hyde::rt::kNoRow; s61 = idx_51.Next(s61)) {
              const auto r61 = table_5.RowAt(s61);
              const auto v62 = r61.x;
              const auto v63 = r61.c1;
              if (true == v63) {
                if (table_24.TryChangePresentToUnknown({true, v62})) {
                  if (table_28.TryChangePresentToUnknown({v62})) {
                    if (gated_39.TryChangePresentToUnknown({v62})) {
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
  for (auto [v66] : vec64) {
    if (const auto ins8 = table_5.TryChangeAbsentToPresent({v66, true}); ins8.changed) {
      if (ins8.added_row) {
        idx_51.Add({true}, ins8.id);
      }
      vec48.Add({true});
    }
  }
  vec44.Clear();
  vec45.Clear();
  vec55.Clear();
  vec56.Clear();
  vec64.Clear();
  flow_241(std::move(vec48));
  return false;
}

bool Database::add_a_1(::hyde::rt::Vec<Tup_i32> vec73, ::hyde::rt::Vec<Tup_i32> vec74) {
  ::hyde::rt::Vec<Tup_i32> vec76(allocator);
  ::hyde::rt::Vec<Tup_i32> vec77(allocator);
  proc_42(std::move(vec73), std::move(vec74), std::move(vec76), std::move(vec76), std::move(vec77));
  return true;
}

bool Database::add_b_1(::hyde::rt::Vec<Tup_i32> vec79, ::hyde::rt::Vec<Tup_i32> vec80) {
  ::hyde::rt::Vec<Tup_i32> vec82(allocator);
  ::hyde::rt::Vec<Tup_i32> vec83(allocator);
  proc_42(std::move(vec82), std::move(vec82), std::move(vec79), std::move(vec80), std::move(vec83));
  return true;
}

bool Database::add_item_1(::hyde::rt::Vec<Tup_i32> vec85) {
  ::hyde::rt::Vec<Tup_i32> vec87(allocator);
  ::hyde::rt::Vec<Tup_i32> vec88(allocator);
  proc_42(std::move(vec87), std::move(vec87), std::move(vec88), std::move(vec88), std::move(vec85));
  return true;
}

bool Database::find_93(int32_t v94) {
  switch (gated_39.State({v94})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (gated_39.TryChangeUnknownToAbsent({v94})) {
        if (find_95(v94)) {
          if (const auto ins9 = gated_39.TryChangeAbsentToPresent({v94}); ins9.changed) {
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
  if (find_93(v94)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_95(int32_t v96) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_99(v96)) {
    return true;
  }
  return false;
}

bool Database::find_99(int32_t v100) {
  switch (table_28.State({v100})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_28.TryChangeUnknownToAbsent({v100})) {
        if (find_102(v100)) {
          if (const auto ins10 = table_28.TryChangeAbsentToPresent({v100}); ins10.changed) {
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
  if (find_105(v100)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_102(int32_t v103) {
  if (find_110(v103)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_105(int32_t v106) {
  switch (table_28.State({v106})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_28.TryChangeUnknownToAbsent({v106})) {
        if (find_102(v106)) {
          if (const auto ins11 = table_28.TryChangeAbsentToPresent({v106}); ins11.changed) {
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
  if (find_105(v106)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_110(int32_t v111) {
  if (find_113(v111)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_113(int32_t v114) {
  for (uint32_t s117 = idx_116.First({v114}); s117 != ::hyde::rt::kNoRow; s117 = idx_116.Next(s117)) {
    const auto r117 = table_24.RowAt(s117);
    const auto v118 = r117.c0;
    const auto v119 = r117.x;
    if (v114 == v119) {
      if (find_120(v118, v119)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_120(bool v121, int32_t v122) {
  switch (table_24.State({v121, v122})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_24.TryChangeUnknownToAbsent({v121, v122})) {
        if (find_124(v121, v122)) {
          if (const auto ins12 = table_24.TryChangeAbsentToPresent({v121, v122}); ins12.changed) {
            if (ins12.added_row) {
              idx_116.Add({v122}, ins12.id);
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
  if (find_120(v121, v122)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_124(bool v125, int32_t v126) {
  if (find_129(v126, v125)) {
    if (find_133(v125)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_129(int32_t v130, bool v131) {
  switch (table_5.State({v130, v131})) {
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

bool Database::find_133(bool v134) {
  switch (__9.State({v134})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v134})) {
        if (find_136(v134)) {
          if (const auto ins13 = __9.TryChangeAbsentToPresent({v134}); ins13.changed) {
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
  if (find_139(v134)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_136(bool v137) {
  if (find_144(v137)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_139(bool v140) {
  switch (__9.State({v140})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v140})) {
        if (find_136(v140)) {
          if (const auto ins14 = __9.TryChangeAbsentToPresent({v140}); ins14.changed) {
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
  if (find_139(v140)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_144(bool v145) {
  for (uint32_t s147 = 0; s147 < table_31.NumRows(); ++s147) {
    const auto r147 = table_31.RowAt(s147);
    const auto v148 = r147.c0;
    const auto v149 = r147.c1;
    if (v148 == v145) {
      if (find_150(v148, v149)) {
        return true;
      }
    }
  }
  for (uint32_t s154 = 0; s154 < table_35.NumRows(); ++s154) {
    const auto r154 = table_35.RowAt(s154);
    const auto v155 = r154.c0;
    const auto v156 = r154.c1;
    if (v155 == v145) {
      if (find_157(v155, v156)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_150(bool v151, int32_t v152) {
  switch (table_31.State({v151, v152})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_31.TryChangeUnknownToAbsent({v151, v152})) {
        if (find_201(v151, v152)) {
          if (const auto ins15 = table_31.TryChangeAbsentToPresent({v151, v152}); ins15.changed) {
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
  if (find_150(v151, v152)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_157(bool v158, int32_t v159) {
  switch (table_35.State({v158, v159})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_35.TryChangeUnknownToAbsent({v158, v159})) {
        if (find_161(v158, v159)) {
          if (const auto ins16 = table_35.TryChangeAbsentToPresent({v158, v159}); ins16.changed) {
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
  if (find_157(v158, v159)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_161(bool v162, int32_t v163) {
  if (find_166(v162, v163)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_166(bool v167, int32_t v168) {
  if (find_170(v168)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_170(int32_t v171) {
  switch (table_21.State({v171})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_21.TryChangeUnknownToAbsent({v171})) {
        if (find_173(v171)) {
          if (const auto ins17 = table_21.TryChangeAbsentToPresent({v171}); ins17.changed) {
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
  if (find_176(v171)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_173(int32_t v174) {
  if (find_179(v174)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_176(int32_t v177) {
  switch (table_21.State({v177})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_21.TryChangeUnknownToAbsent({v177})) {
        if (find_179(v177)) {
          if (const auto ins18 = table_21.TryChangeAbsentToPresent({v177}); ins18.changed) {
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
  if (find_176(v177)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_179(int32_t v180) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_183(v180)) {
    return true;
  }
  return false;
}

bool Database::find_183(int32_t v184) {
  switch (table_15.State({v184})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_15.TryChangeUnknownToAbsent({v184})) {
        if (find_186(v184)) {
          if (const auto ins19 = table_15.TryChangeAbsentToPresent({v184}); ins19.changed) {
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
  if (find_189(v184)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_186(int32_t v187) {
  if (find_194(v187)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_189(int32_t v190) {
  switch (table_15.State({v190})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_15.TryChangeUnknownToAbsent({v190})) {
        if (find_186(v190)) {
          if (const auto ins20 = table_15.TryChangeAbsentToPresent({v190}); ins20.changed) {
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
  if (find_189(v190)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_194(int32_t v195) {
  if (find_197(v195)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_197(int32_t v198) {
  return false;
}

bool Database::find_201(bool v202, int32_t v203) {
  if (find_206(v202, v203)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_206(bool v207, int32_t v208) {
  if (find_210(v208)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_210(int32_t v211) {
  switch (table_18.State({v211})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v211})) {
        if (find_213(v211)) {
          if (const auto ins21 = table_18.TryChangeAbsentToPresent({v211}); ins21.changed) {
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
  if (find_216(v211)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_213(int32_t v214) {
  if (find_219(v214)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_216(int32_t v217) {
  switch (table_18.State({v217})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v217})) {
        if (find_219(v217)) {
          if (const auto ins22 = table_18.TryChangeAbsentToPresent({v217}); ins22.changed) {
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
  if (find_216(v217)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_219(int32_t v220) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_223(v220)) {
    return true;
  }
  return false;
}

bool Database::find_223(int32_t v224) {
  switch (table_12.State({v224})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_12.TryChangeUnknownToAbsent({v224})) {
        if (find_226(v224)) {
          if (const auto ins23 = table_12.TryChangeAbsentToPresent({v224}); ins23.changed) {
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
  if (find_229(v224)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_226(int32_t v227) {
  if (find_234(v227)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_229(int32_t v230) {
  switch (table_12.State({v230})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_12.TryChangeUnknownToAbsent({v230})) {
        if (find_226(v230)) {
          if (const auto ins24 = table_12.TryChangeAbsentToPresent({v230}); ins24.changed) {
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
  if (find_229(v230)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_234(int32_t v235) {
  if (find_237(v235)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_237(int32_t v238) {
  return false;
}

bool Database::flow_241(::hyde::rt::Vec<Tup_b> vec48) {
  if ((g43 += 1) == 1) {
  }
  vec48.SortAndUnique();
  for (auto [v68] : vec48) {
    if (const uint32_t j67_0 = __9.Find({v68}); j67_0 != ::hyde::rt::kNoRow) {
      const auto r67_0 = __9.RowAt(j67_0);
      const auto v70 = r67_0.c0;
      for (uint32_t j67_1 = idx_51.First({v68}); j67_1 != ::hyde::rt::kNoRow; j67_1 = idx_51.Next(j67_1)) {
        const auto r67_1 = table_5.RowAt(j67_1);
        const auto v71 = r67_1.x;
        const auto v69 = r67_1.c1;
        if (v68 == v69 && v68 == v70) {
          switch (__9.State({v69})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins25 = table_24.TryChangeAbsentOrUnknownToPresent({v69, v71}); ins25.changed) {
                if (ins25.added_row) {
                  idx_116.Add({v71}, ins25.id);
                }
                if (const auto ins26 = table_28.TryChangeAbsentOrUnknownToPresent({v71}); ins26.changed) {
                  if (const auto ins27 = gated_39.TryChangeAbsentOrUnknownToPresent({v71}); ins27.changed) {
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
  vec48.Clear();
  return true;
}

bool Database::gated_b(int32_t X) {
  return find_93(X);
}

