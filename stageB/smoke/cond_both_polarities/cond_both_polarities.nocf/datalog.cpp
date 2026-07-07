// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_88(allocator_),
      __9(allocator_),
      table_12(allocator_),
      idx_216(allocator_),
      table_16(allocator_),
      idx_188(allocator_),
      table_20(allocator_),
      table_23(allocator_),
      table_26(allocator_),
      idx_239(allocator_),
      mixed_30(allocator_),
      table_33(allocator_),
      table_36(allocator_),
      table_39(allocator_),
      table_42(allocator_),
      table_46(allocator_),
      pos_neg_50(allocator_),
      both_pos_53(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec157(allocator);
  ::hyde::rt::Vec<Tup_i32> vec158(allocator);
  ::hyde::rt::Vec<Tup_i32> vec159(allocator);
  ::hyde::rt::Vec<Tup_i32> vec160(allocator);
  proc_56(std::move(vec157), std::move(vec157), std::move(vec158), std::move(vec158), std::move(vec159), std::move(vec160));
  return false;
}

bool Database::proc_56(::hyde::rt::Vec<Tup_i32> vec58, ::hyde::rt::Vec<Tup_i32> vec59, ::hyde::rt::Vec<Tup_i32> vec92, ::hyde::rt::Vec<Tup_i32> vec93, ::hyde::rt::Vec<Tup_i32> vec117, ::hyde::rt::Vec<Tup_i32> vec120) {
  ::hyde::rt::Vec<Tup_b> vec74(allocator);
  for (auto [v61] : vec58) {
    if (const auto ins0 = table_20.TryChangeAbsentToPresent({v61}); ins0.changed) {
      if (const auto ins1 = table_42.TryChangeAbsentOrUnknownToPresent({true, v61}); ins1.changed) {
        if (const auto ins2 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins2.changed) {
          for (uint32_t s62 = 0; s62 < table_39.NumRows(); ++s62) {
            const auto r62 = table_39.RowAt(s62);
            const auto v63 = r62.x;
            if (find_64(v63)) {
              if (table_16.TryChangePresentToUnknown({true, v63})) {
                if (table_36.TryChangePresentToUnknown({v63})) {
                  if (mixed_30.TryChangePresentToUnknown({v63})) {
                  }
                }
              }
            }
          }
          for (uint32_t s67 = 0; s67 < table_12.NumRows(); ++s67) {
            const auto r67 = table_12.RowAt(s67);
            const auto v68 = r67.c0;
            const auto v69 = r67.x;
            if (find_70(v68, v69)) {
              if (table_26.TryChangePresentToUnknown({true, v69})) {
                if (pos_neg_50.TryChangePresentToUnknown({v69})) {
                }
              }
            }
          }
          vec74.Add({true});
        }
      }
    }
  }
  for (auto [v76] : vec59) {
    if (table_20.TryChangePresentToAbsent({v76})) {
      if (table_42.TryChangePresentToUnknown({true, v76})) {
        if (__9.TryChangePresentToUnknown({true})) {
          for (uint32_t s77 = 0; s77 < table_39.NumRows(); ++s77) {
            const auto r77 = table_39.RowAt(s77);
            const auto v78 = r77.x;
            if (find_64(v78)) {
              if (!find_80(true)) {
                if (const auto ins3 = table_16.TryChangeAbsentOrUnknownToPresent({true, v78}); ins3.changed) {
                  if (ins3.added_row) {
                    idx_188.Add({v78}, ins3.id);
                  }
                  if (const auto ins4 = table_36.TryChangeAbsentOrUnknownToPresent({v78}); ins4.changed) {
                    if (const auto ins5 = mixed_30.TryChangeAbsentOrUnknownToPresent({v78}); ins5.changed) {
                    }
                  }
                }
              }
            }
          }
          for (uint32_t s83 = 0; s83 < table_12.NumRows(); ++s83) {
            const auto r83 = table_12.RowAt(s83);
            const auto v84 = r83.c0;
            const auto v85 = r83.x;
            if (find_70(v84, v85)) {
              if (!find_80(true)) {
                if (const auto ins6 = table_26.TryChangeAbsentOrUnknownToPresent({true, v85}); ins6.changed) {
                  if (ins6.added_row) {
                    idx_239.Add({v85}, ins6.id);
                  }
                  if (const auto ins7 = pos_neg_50.TryChangeAbsentOrUnknownToPresent({v85}); ins7.changed) {
                  }
                }
              }
            }
          }
          for (uint32_t s89 = idx_88.First({true}); s89 != ::hyde::rt::kNoRow; s89 = idx_88.Next(s89)) {
            const auto r89 = table_5.RowAt(s89);
            const auto v90 = r89.x;
            const auto v91 = r89.c1;
            if (true == v91) {
              if (table_12.TryChangePresentToUnknown({true, v90})) {
                if (table_26.TryChangePresentToUnknown({true, v90})) {
                  if (pos_neg_50.TryChangePresentToUnknown({v90})) {
                  }
                }
                if (table_33.TryChangePresentToUnknown({v90})) {
                  if (mixed_30.TryChangePresentToUnknown({v90})) {
                  }
                }
                if (both_pos_53.TryChangePresentToUnknown({v90})) {
                }
              }
            }
          }
        }
      }
    }
  }
  for (auto [v95] : vec92) {
    if (const auto ins8 = table_23.TryChangeAbsentToPresent({v95}); ins8.changed) {
      if (const auto ins9 = table_46.TryChangeAbsentOrUnknownToPresent({true, v95}); ins9.changed) {
        if (const auto ins10 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins10.changed) {
          for (uint32_t s96 = 0; s96 < table_39.NumRows(); ++s96) {
            const auto r96 = table_39.RowAt(s96);
            const auto v97 = r96.x;
            if (find_64(v97)) {
              if (table_16.TryChangePresentToUnknown({true, v97})) {
                if (table_36.TryChangePresentToUnknown({v97})) {
                  if (mixed_30.TryChangePresentToUnknown({v97})) {
                  }
                }
              }
            }
          }
          for (uint32_t s99 = 0; s99 < table_12.NumRows(); ++s99) {
            const auto r99 = table_12.RowAt(s99);
            const auto v100 = r99.c0;
            const auto v101 = r99.x;
            if (find_70(v100, v101)) {
              if (table_26.TryChangePresentToUnknown({true, v101})) {
                if (pos_neg_50.TryChangePresentToUnknown({v101})) {
                }
              }
            }
          }
          vec74.Add({true});
        }
      }
    }
  }
  for (auto [v104] : vec93) {
    if (table_23.TryChangePresentToAbsent({v104})) {
      if (table_46.TryChangePresentToUnknown({true, v104})) {
        if (__9.TryChangePresentToUnknown({true})) {
          for (uint32_t s105 = 0; s105 < table_39.NumRows(); ++s105) {
            const auto r105 = table_39.RowAt(s105);
            const auto v106 = r105.x;
            if (find_64(v106)) {
              if (!find_80(true)) {
                if (const auto ins11 = table_16.TryChangeAbsentOrUnknownToPresent({true, v106}); ins11.changed) {
                  if (ins11.added_row) {
                    idx_188.Add({v106}, ins11.id);
                  }
                  if (const auto ins12 = table_36.TryChangeAbsentOrUnknownToPresent({v106}); ins12.changed) {
                    if (const auto ins13 = mixed_30.TryChangeAbsentOrUnknownToPresent({v106}); ins13.changed) {
                    }
                  }
                }
              }
            }
          }
          for (uint32_t s109 = 0; s109 < table_12.NumRows(); ++s109) {
            const auto r109 = table_12.RowAt(s109);
            const auto v110 = r109.c0;
            const auto v111 = r109.x;
            if (find_70(v110, v111)) {
              if (!find_80(true)) {
                if (const auto ins14 = table_26.TryChangeAbsentOrUnknownToPresent({true, v111}); ins14.changed) {
                  if (ins14.added_row) {
                    idx_239.Add({v111}, ins14.id);
                  }
                  if (const auto ins15 = pos_neg_50.TryChangeAbsentOrUnknownToPresent({v111}); ins15.changed) {
                  }
                }
              }
            }
          }
          for (uint32_t s114 = idx_88.First({true}); s114 != ::hyde::rt::kNoRow; s114 = idx_88.Next(s114)) {
            const auto r114 = table_5.RowAt(s114);
            const auto v115 = r114.x;
            const auto v116 = r114.c1;
            if (true == v116) {
              if (table_12.TryChangePresentToUnknown({true, v115})) {
                if (table_26.TryChangePresentToUnknown({true, v115})) {
                  if (pos_neg_50.TryChangePresentToUnknown({v115})) {
                  }
                }
                if (table_33.TryChangePresentToUnknown({v115})) {
                  if (mixed_30.TryChangePresentToUnknown({v115})) {
                  }
                }
                if (both_pos_53.TryChangePresentToUnknown({v115})) {
                }
              }
            }
          }
        }
      }
    }
  }
  for (auto [v119] : vec117) {
    if (const auto ins16 = table_5.TryChangeAbsentToPresent({v119, true}); ins16.changed) {
      if (ins16.added_row) {
        idx_88.Add({true}, ins16.id);
      }
      vec74.Add({true});
    }
  }
  for (auto [v122] : vec120) {
    if (const auto ins17 = table_39.TryChangeAbsentToPresent({v122}); ins17.changed) {
      if (!find_80(true)) {
        if (const auto ins18 = table_16.TryChangeAbsentOrUnknownToPresent({true, v122}); ins18.changed) {
          if (ins18.added_row) {
            idx_188.Add({v122}, ins18.id);
          }
          if (const auto ins19 = table_36.TryChangeAbsentOrUnknownToPresent({v122}); ins19.changed) {
            if (const auto ins20 = mixed_30.TryChangeAbsentOrUnknownToPresent({v122}); ins20.changed) {
            }
          }
        }
      }
    }
  }
  vec58.Clear();
  vec59.Clear();
  vec92.Clear();
  vec93.Clear();
  vec117.Clear();
  vec120.Clear();
  flow_334(std::move(vec74));
  return false;
}

bool Database::find_64(int32_t v65) {
  switch (table_39.State({v65})) {
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

bool Database::find_70(bool v71, int32_t v72) {
  switch (table_12.State({v71, v72})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_12.TryChangeUnknownToAbsent({v71, v72})) {
        if (find_313(v71, v72)) {
          if (const auto ins21 = table_12.TryChangeAbsentToPresent({v71, v72}); ins21.changed) {
            if (ins21.added_row) {
              idx_216.Add({v72}, ins21.id);
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
  if (find_70(v71, v72)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_80(bool v81) {
  switch (__9.State({v81})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v81})) {
        if (find_260(v81)) {
          if (const auto ins22 = __9.TryChangeAbsentToPresent({v81}); ins22.changed) {
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
  if (find_80(v81)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::set_c1_1(::hyde::rt::Vec<Tup_i32> vec131, ::hyde::rt::Vec<Tup_i32> vec132) {
  ::hyde::rt::Vec<Tup_i32> vec134(allocator);
  ::hyde::rt::Vec<Tup_i32> vec135(allocator);
  ::hyde::rt::Vec<Tup_i32> vec136(allocator);
  proc_56(std::move(vec131), std::move(vec132), std::move(vec134), std::move(vec134), std::move(vec135), std::move(vec136));
  return true;
}

bool Database::set_c2_1(::hyde::rt::Vec<Tup_i32> vec138, ::hyde::rt::Vec<Tup_i32> vec139) {
  ::hyde::rt::Vec<Tup_i32> vec141(allocator);
  ::hyde::rt::Vec<Tup_i32> vec142(allocator);
  ::hyde::rt::Vec<Tup_i32> vec143(allocator);
  proc_56(std::move(vec141), std::move(vec141), std::move(vec138), std::move(vec139), std::move(vec142), std::move(vec143));
  return true;
}

bool Database::add_r_1(::hyde::rt::Vec<Tup_i32> vec145) {
  ::hyde::rt::Vec<Tup_i32> vec147(allocator);
  ::hyde::rt::Vec<Tup_i32> vec148(allocator);
  ::hyde::rt::Vec<Tup_i32> vec149(allocator);
  proc_56(std::move(vec147), std::move(vec147), std::move(vec148), std::move(vec148), std::move(vec145), std::move(vec149));
  return true;
}

bool Database::add_s_1(::hyde::rt::Vec<Tup_i32> vec151) {
  ::hyde::rt::Vec<Tup_i32> vec153(allocator);
  ::hyde::rt::Vec<Tup_i32> vec154(allocator);
  ::hyde::rt::Vec<Tup_i32> vec155(allocator);
  proc_56(std::move(vec153), std::move(vec153), std::move(vec154), std::move(vec154), std::move(vec155), std::move(vec151));
  return true;
}

bool Database::find_161(int32_t v162) {
  switch (pos_neg_50.State({v162})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (pos_neg_50.TryChangeUnknownToAbsent({v162})) {
        if (find_232(v162)) {
          if (const auto ins23 = pos_neg_50.TryChangeAbsentToPresent({v162}); ins23.changed) {
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
  if (find_161(v162)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_163(int32_t v164) {
  switch (both_pos_53.State({v164})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (both_pos_53.TryChangeUnknownToAbsent({v164})) {
        if (find_221(v164)) {
          if (const auto ins24 = both_pos_53.TryChangeAbsentToPresent({v164}); ins24.changed) {
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

bool Database::find_165(int32_t v166) {
  switch (mixed_30.State({v166})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (mixed_30.TryChangeUnknownToAbsent({v166})) {
        if (find_167(v166)) {
          if (const auto ins25 = mixed_30.TryChangeAbsentToPresent({v166}); ins25.changed) {
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
  if (find_165(v166)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_167(int32_t v168) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_171(v168)) {
    return true;
  }
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_174(v168)) {
    return true;
  }
  return false;
}

bool Database::find_171(int32_t v172) {
  switch (table_33.State({v172})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_33.TryChangeUnknownToAbsent({v172})) {
        if (find_205(v172)) {
          if (const auto ins26 = table_33.TryChangeAbsentToPresent({v172}); ins26.changed) {
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
  if (find_208(v172)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_174(int32_t v175) {
  switch (table_36.State({v175})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_36.TryChangeUnknownToAbsent({v175})) {
        if (find_177(v175)) {
          if (const auto ins27 = table_36.TryChangeAbsentToPresent({v175}); ins27.changed) {
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
  if (find_180(v175)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_177(int32_t v178) {
  if (find_185(v178)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_180(int32_t v181) {
  switch (table_36.State({v181})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_36.TryChangeUnknownToAbsent({v181})) {
        if (find_177(v181)) {
          if (const auto ins28 = table_36.TryChangeAbsentToPresent({v181}); ins28.changed) {
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
  if (find_180(v181)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_185(int32_t v186) {
  for (uint32_t s189 = idx_188.First({v186}); s189 != ::hyde::rt::kNoRow; s189 = idx_188.Next(s189)) {
    const auto r189 = table_16.RowAt(s189);
    const auto v190 = r189.c0;
    const auto v191 = r189.x;
    if (v186 == v191) {
      if (find_192(v190, v191)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_192(bool v193, int32_t v194) {
  switch (table_16.State({v193, v194})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_16.TryChangeUnknownToAbsent({v193, v194})) {
        if (find_196(v193, v194)) {
          if (const auto ins29 = table_16.TryChangeAbsentToPresent({v193, v194}); ins29.changed) {
            if (ins29.added_row) {
              idx_188.Add({v194}, ins29.id);
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
  if (find_192(v193, v194)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_196(bool v197, int32_t v198) {
  if (find_201(v198)) {
    if (find_80(v197)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_201(int32_t v202) {
  switch (table_39.State({v202})) {
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

bool Database::find_205(int32_t v206) {
  if (find_213(v206)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_208(int32_t v209) {
  switch (table_33.State({v209})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_33.TryChangeUnknownToAbsent({v209})) {
        if (find_205(v209)) {
          if (const auto ins30 = table_33.TryChangeAbsentToPresent({v209}); ins30.changed) {
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
  if (find_208(v209)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_213(int32_t v214) {
  for (uint32_t s217 = idx_216.First({v214}); s217 != ::hyde::rt::kNoRow; s217 = idx_216.Next(s217)) {
    const auto r217 = table_12.RowAt(s217);
    const auto v218 = r217.c0;
    const auto v219 = r217.x;
    if (v214 == v219) {
      if (find_70(v218, v219)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_221(int32_t v222) {
  if (find_225(v222)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_225(int32_t v226) {
  for (uint32_t s228 = idx_216.First({v226}); s228 != ::hyde::rt::kNoRow; s228 = idx_216.Next(s228)) {
    const auto r228 = table_12.RowAt(s228);
    const auto v229 = r228.c0;
    const auto v230 = r228.x;
    if (v226 == v230) {
      if (find_70(v229, v230)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_232(int32_t v233) {
  if (find_236(v233)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_236(int32_t v237) {
  for (uint32_t s240 = idx_239.First({v237}); s240 != ::hyde::rt::kNoRow; s240 = idx_239.Next(s240)) {
    const auto r240 = table_26.RowAt(s240);
    const auto v241 = r240.c0;
    const auto v242 = r240.x;
    if (v237 == v242) {
      if (find_243(v241, v242)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_243(bool v244, int32_t v245) {
  switch (table_26.State({v244, v245})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_26.TryChangeUnknownToAbsent({v244, v245})) {
        if (find_247(v244, v245)) {
          if (const auto ins31 = table_26.TryChangeAbsentToPresent({v244, v245}); ins31.changed) {
            if (ins31.added_row) {
              idx_239.Add({v245}, ins31.id);
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
  if (find_243(v244, v245)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_247(bool v248, int32_t v249) {
  if (find_252(v249)) {
    if (find_80(v248)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_252(int32_t v253) {
  for (uint32_t s256 = idx_216.First({v253}); s256 != ::hyde::rt::kNoRow; s256 = idx_216.Next(s256)) {
    const auto r256 = table_12.RowAt(s256);
    const auto v257 = r256.c0;
    const auto v258 = r256.x;
    if (v253 == v258) {
      if (find_70(v257, v258)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_260(bool v261) {
  if (find_264(v261)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_264(bool v265) {
  for (uint32_t s267 = 0; s267 < table_42.NumRows(); ++s267) {
    const auto r267 = table_42.RowAt(s267);
    const auto v268 = r267.c0;
    const auto v269 = r267.x;
    if (v268 == v265) {
      if (find_270(v268, v269)) {
        return true;
      }
    }
  }
  for (uint32_t s274 = 0; s274 < table_46.NumRows(); ++s274) {
    const auto r274 = table_46.RowAt(s274);
    const auto v275 = r274.c0;
    const auto v276 = r274.x;
    if (v275 == v265) {
      if (find_277(v275, v276)) {
        return true;
      }
    }
  }
  return false;
  return false;
}

bool Database::find_270(bool v271, int32_t v272) {
  switch (table_42.State({v271, v272})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_42.TryChangeUnknownToAbsent({v271, v272})) {
        if (find_297(v271, v272)) {
          if (const auto ins32 = table_42.TryChangeAbsentToPresent({v271, v272}); ins32.changed) {
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
  if (find_270(v271, v272)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_277(bool v278, int32_t v279) {
  switch (table_46.State({v278, v279})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_46.TryChangeUnknownToAbsent({v278, v279})) {
        if (find_281(v278, v279)) {
          if (const auto ins33 = table_46.TryChangeAbsentToPresent({v278, v279}); ins33.changed) {
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
  if (find_277(v278, v279)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_281(bool v282, int32_t v283) {
  if (find_286(v283)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_286(int32_t v287) {
  switch (table_23.State({v287})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_23.TryChangeUnknownToAbsent({v287})) {
        if (find_289(v287)) {
          if (const auto ins34 = table_23.TryChangeAbsentToPresent({v287}); ins34.changed) {
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
  if (find_292(v287)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_289(int32_t v290) {
  return false;
}

bool Database::find_292(int32_t v293) {
  switch (table_23.State({v293})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_23.TryChangeUnknownToAbsent({v293})) {
        if (find_289(v293)) {
          if (const auto ins35 = table_23.TryChangeAbsentToPresent({v293}); ins35.changed) {
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
  if (find_292(v293)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_297(bool v298, int32_t v299) {
  if (find_302(v299)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_302(int32_t v303) {
  switch (table_20.State({v303})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_20.TryChangeUnknownToAbsent({v303})) {
        if (find_305(v303)) {
          if (const auto ins36 = table_20.TryChangeAbsentToPresent({v303}); ins36.changed) {
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
  if (find_308(v303)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_305(int32_t v306) {
  return false;
}

bool Database::find_308(int32_t v309) {
  switch (table_20.State({v309})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_20.TryChangeUnknownToAbsent({v309})) {
        if (find_305(v309)) {
          if (const auto ins37 = table_20.TryChangeAbsentToPresent({v309}); ins37.changed) {
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
  if (find_308(v309)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_313(bool v314, int32_t v315) {
  if (find_318(v315, v314)) {
    if (find_322(v314)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_318(int32_t v319, bool v320) {
  switch (table_5.State({v319, v320})) {
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

bool Database::find_322(bool v323) {
  switch (__9.State({v323})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v323})) {
        if (find_325(v323)) {
          if (const auto ins38 = __9.TryChangeAbsentToPresent({v323}); ins38.changed) {
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
  if (find_328(v323)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_325(bool v326) {
  if (find_264(v326)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_328(bool v329) {
  switch (__9.State({v329})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v329})) {
        if (find_325(v329)) {
          if (const auto ins39 = __9.TryChangeAbsentToPresent({v329}); ins39.changed) {
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
  if (find_328(v329)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::flow_334(::hyde::rt::Vec<Tup_b> vec74) {
  if ((g57 += 1) == 1) {
  }
  vec74.SortAndUnique();
  for (auto [v125] : vec74) {
    if (const uint32_t j124_0 = __9.Find({v125}); j124_0 != ::hyde::rt::kNoRow) {
      const auto r124_0 = __9.RowAt(j124_0);
      const auto v127 = r124_0.c0;
      for (uint32_t j124_1 = idx_88.First({v125}); j124_1 != ::hyde::rt::kNoRow; j124_1 = idx_88.Next(j124_1)) {
        const auto r124_1 = table_5.RowAt(j124_1);
        const auto v128 = r124_1.x;
        const auto v126 = r124_1.c1;
        if (v125 == v126 && v125 == v127) {
          switch (__9.State({v126})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins40 = table_12.TryChangeAbsentOrUnknownToPresent({v126, v128}); ins40.changed) {
                if (ins40.added_row) {
                  idx_216.Add({v128}, ins40.id);
                }
                if (!find_80(true)) {
                  if (const auto ins41 = table_26.TryChangeAbsentOrUnknownToPresent({true, v128}); ins41.changed) {
                    if (ins41.added_row) {
                      idx_239.Add({v128}, ins41.id);
                    }
                    if (const auto ins42 = pos_neg_50.TryChangeAbsentOrUnknownToPresent({v128}); ins42.changed) {
                    }
                  }
                }
                if (const auto ins43 = table_33.TryChangeAbsentOrUnknownToPresent({v128}); ins43.changed) {
                  if (const auto ins44 = mixed_30.TryChangeAbsentOrUnknownToPresent({v128}); ins44.changed) {
                  }
                }
                if (const auto ins45 = both_pos_53.TryChangeAbsentOrUnknownToPresent({v128}); ins45.changed) {
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
  vec74.Clear();
  return true;
}

Database::pos_neg_f_cursor Database::pos_neg_f() {
  return {*this, 0};
}

bool Database::pos_neg_f_cursor::next(int32_t &X) {
  while (pos < db.pos_neg_50.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.pos_neg_50.RowAt(id);
    if (!db.find_161(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

Database::both_pos_f_cursor Database::both_pos_f() {
  return {*this, 0};
}

bool Database::both_pos_f_cursor::next(int32_t &X) {
  while (pos < db.both_pos_53.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.both_pos_53.RowAt(id);
    if (!db.find_163(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

Database::mixed_f_cursor Database::mixed_f() {
  return {*this, 0};
}

bool Database::mixed_f_cursor::next(int32_t &X) {
  while (pos < db.mixed_30.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.mixed_30.RowAt(id);
    if (!db.find_165(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

