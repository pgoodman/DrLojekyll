// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_7(allocator_),
      idx_98(allocator_),
      __11(allocator_),
      table_14(allocator_),
      idx_308(allocator_),
      table_18(allocator_),
      idx_269(allocator_),
      table_22(allocator_),
      idx_223(allocator_),
      table_26(allocator_),
      idx_173(allocator_),
      table_30(allocator_),
      table_33(allocator_),
      table_36(allocator_),
      table_39(allocator_),
      table_42(allocator_),
      out7_46(allocator_),
      outdup_49(allocator_),
      outflag_52(allocator_),
      table_55(allocator_),
      table_59(allocator_),
      table_63(allocator_),
      table_67(allocator_),
      table_71(allocator_),
      table_75(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32_i32> vec131(allocator);
  ::hyde::rt::Vec<Tup_i32> vec132(allocator);
  proc_79(std::move(vec131), std::move(vec132));
  return false;
}

bool Database::proc_79(::hyde::rt::Vec<Tup_i32_i32> vec82, ::hyde::rt::Vec<Tup_i32> vec102) {
  ::hyde::rt::Vec<Tup_b> vec81(allocator);
  for (auto [v84, v85] : vec82) {
    if (7 == v85) {
      if (const auto ins0 = table_55.TryChangeAbsentToPresent({v84, 7}); ins0.changed) {
        if (find_86(v84, 7)) {
          if (table_14.TryChangePresentToUnknown({v84, 7})) {
            if (table_33.TryChangePresentToUnknown({v84})) {
              if (out7_46.TryChangePresentToUnknown({v84})) {
              }
            }
          }
        }
      }
    }
    if (v84 == v85) {
      if (const auto ins1 = table_63.TryChangeAbsentToPresent({v84, v84}); ins1.changed) {
        if (find_90(v84, v84)) {
          if (table_18.TryChangePresentToUnknown({v84, v84})) {
            if (table_36.TryChangePresentToUnknown({v84})) {
              if (outdup_49.TryChangePresentToUnknown({v84})) {
              }
            }
          }
        }
      }
    }
    if (1 == v84) {
      if (7 == v85) {
        if (const auto ins2 = table_71.TryChangeAbsentToPresent({1, 7}); ins2.changed) {
          if (find_94(1, 7)) {
            if (table_22.TryChangePresentToUnknown({1, 7})) {
              if (table_42.TryChangePresentToUnknown({true, 1})) {
                if (__11.TryChangePresentToUnknown({true})) {
                  for (uint32_t s99 = idx_98.First({true}); s99 != ::hyde::rt::kNoRow; s99 = idx_98.Next(s99)) {
                    const auto r99 = table_7.RowAt(s99);
                    const auto v100 = r99.c0;
                    const auto v101 = r99.c1;
                    if (true == v101) {
                      if (table_26.TryChangePresentToUnknown({true, v100})) {
                        if (table_39.TryChangePresentToUnknown({v100})) {
                          if (table_30.TryChangePresentToUnknown({v100})) {
                            if (outflag_52.TryChangePresentToUnknown({v100})) {
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
      }
    }
  }
  for (auto [v104] : vec102) {
    if (const auto ins3 = table_59.TryChangeAbsentToPresent({v104, 7}); ins3.changed) {
      if (!find_105(v104, 7)) {
        if (const auto ins4 = table_14.TryChangeAbsentOrUnknownToPresent({v104, 7}); ins4.changed) {
          if (ins4.added_row) {
            idx_308.Add({v104}, ins4.id);
          }
          if (const auto ins5 = table_33.TryChangeAbsentOrUnknownToPresent({v104}); ins5.changed) {
            if (const auto ins6 = out7_46.TryChangeAbsentOrUnknownToPresent({v104}); ins6.changed) {
            }
          }
        }
      }
    }
    if (const auto ins7 = table_67.TryChangeAbsentToPresent({v104, v104}); ins7.changed) {
      if (!find_109(v104, v104)) {
        if (const auto ins8 = table_18.TryChangeAbsentOrUnknownToPresent({v104, v104}); ins8.changed) {
          if (ins8.added_row) {
            idx_269.Add({v104}, ins8.id);
          }
          if (const auto ins9 = table_36.TryChangeAbsentOrUnknownToPresent({v104}); ins9.changed) {
            if (const auto ins10 = outdup_49.TryChangeAbsentOrUnknownToPresent({v104}); ins10.changed) {
            }
          }
        }
      }
    }
    if (1 == v104) {
      if (const auto ins11 = table_75.TryChangeAbsentToPresent({1, 7}); ins11.changed) {
        if (!find_113(1, 7)) {
          if (const auto ins12 = table_22.TryChangeAbsentOrUnknownToPresent({1, 7}); ins12.changed) {
            if (ins12.added_row) {
              idx_223.Add({1}, ins12.id);
            }
            if (const auto ins13 = table_42.TryChangeAbsentOrUnknownToPresent({true, 1}); ins13.changed) {
              if (const auto ins14 = __11.TryChangeAbsentOrUnknownToPresent({true}); ins14.changed) {
                vec81.Add({true});
              }
            }
          }
        }
      }
    }
  }
  vec82.Clear();
  vec102.Clear();
  flow_326(std::move(vec81));
  return false;
}

bool Database::find_86(int32_t v87, int32_t v88) {
  switch (table_59.State({v87, v88})) {
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

bool Database::find_90(int32_t v91, int32_t v92) {
  switch (table_67.State({v91, v92})) {
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

bool Database::find_94(int32_t v95, int32_t v96) {
  switch (table_75.State({v95, v96})) {
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

bool Database::find_105(int32_t v106, int32_t v107) {
  switch (table_55.State({v106, v107})) {
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

bool Database::find_109(int32_t v110, int32_t v111) {
  switch (table_63.State({v110, v111})) {
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

bool Database::find_113(int32_t v114, int32_t v115) {
  switch (table_71.State({v114, v115})) {
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

bool Database::pin_2(::hyde::rt::Vec<Tup_i32_i32> vec123) {
  ::hyde::rt::Vec<Tup_i32> vec125(allocator);
  proc_79(std::move(vec123), std::move(vec125));
  return true;
}

bool Database::feed_1(::hyde::rt::Vec<Tup_i32> vec127) {
  ::hyde::rt::Vec<Tup_i32_i32> vec129(allocator);
  proc_79(std::move(vec129), std::move(vec127));
  return true;
}

bool Database::find_133(int32_t v134) {
  switch (out7_46.State({v134})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (out7_46.TryChangeUnknownToAbsent({v134})) {
        if (find_287(v134)) {
          if (const auto ins15 = out7_46.TryChangeAbsentToPresent({v134}); ins15.changed) {
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

bool Database::find_135(int32_t v136) {
  switch (outdup_49.State({v136})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (outdup_49.TryChangeUnknownToAbsent({v136})) {
        if (find_248(v136)) {
          if (const auto ins16 = outdup_49.TryChangeAbsentToPresent({v136}); ins16.changed) {
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
  if (find_135(v136)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_137(int32_t v138) {
  switch (outflag_52.State({v138})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (outflag_52.TryChangeUnknownToAbsent({v138})) {
        if (find_139(v138)) {
          if (const auto ins17 = outflag_52.TryChangeAbsentToPresent({v138}); ins17.changed) {
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
  if (find_137(v138)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_139(int32_t v140) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_143(v140)) {
    return true;
  }
  return false;
}

bool Database::find_143(int32_t v144) {
  switch (table_30.State({v144})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_30.TryChangeUnknownToAbsent({v144})) {
        if (find_146(v144)) {
          if (const auto ins18 = table_30.TryChangeAbsentToPresent({v144}); ins18.changed) {
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
  if (find_241(v147)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_149(int32_t v150) {
  switch (table_30.State({v150})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_30.TryChangeUnknownToAbsent({v150})) {
        if (find_152(v150)) {
          if (const auto ins19 = table_30.TryChangeAbsentToPresent({v150}); ins19.changed) {
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

bool Database::find_152(int32_t v153) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_156(v153)) {
    return true;
  }
  return false;
}

bool Database::find_156(int32_t v157) {
  switch (table_39.State({v157})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_39.TryChangeUnknownToAbsent({v157})) {
        if (find_159(v157)) {
          if (const auto ins20 = table_39.TryChangeAbsentToPresent({v157}); ins20.changed) {
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
  if (find_162(v157)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_159(int32_t v160) {
  if (find_167(v160)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_162(int32_t v163) {
  switch (table_39.State({v163})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_39.TryChangeUnknownToAbsent({v163})) {
        if (find_159(v163)) {
          if (const auto ins21 = table_39.TryChangeAbsentToPresent({v163}); ins21.changed) {
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
  if (find_162(v163)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_167(int32_t v168) {
  if (find_170(v168)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_170(int32_t v171) {
  for (uint32_t s174 = idx_173.First({v171}); s174 != ::hyde::rt::kNoRow; s174 = idx_173.Next(s174)) {
    const auto r174 = table_26.RowAt(s174);
    const auto v175 = r174.c0;
    const auto v176 = r174.c1;
    if (v171 == v176) {
      if (find_177(v175, v176)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_177(bool v178, int32_t v179) {
  switch (table_26.State({v178, v179})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_26.TryChangeUnknownToAbsent({v178, v179})) {
        if (find_181(v178, v179)) {
          if (const auto ins22 = table_26.TryChangeAbsentToPresent({v178, v179}); ins22.changed) {
            if (ins22.added_row) {
              idx_173.Add({v179}, ins22.id);
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
  if (find_177(v178, v179)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_181(bool v182, int32_t v183) {
  if (find_186(v183, v182)) {
    if (find_190(v182)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_186(int32_t v187, bool v188) {
  switch (table_7.State({v187, v188})) {
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

bool Database::find_190(bool v191) {
  switch (__11.State({v191})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__11.TryChangeUnknownToAbsent({v191})) {
        if (find_193(v191)) {
          if (const auto ins23 = __11.TryChangeAbsentToPresent({v191}); ins23.changed) {
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
  if (find_196(v191)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_193(bool v194) {
  if (find_201(v194)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_196(bool v197) {
  switch (__11.State({v197})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__11.TryChangeUnknownToAbsent({v197})) {
        if (find_193(v197)) {
          if (const auto ins24 = __11.TryChangeAbsentToPresent({v197}); ins24.changed) {
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
  if (find_196(v197)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_201(bool v202) {
  for (uint32_t s204 = 0; s204 < table_42.NumRows(); ++s204) {
    const auto r204 = table_42.RowAt(s204);
    const auto v205 = r204.c0;
    const auto v206 = r204.c;
    if (v205 == v202) {
      if (find_207(v205, v206)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_207(bool v208, int32_t v209) {
  switch (table_42.State({v208, v209})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_42.TryChangeUnknownToAbsent({v208, v209})) {
        if (find_211(v208, v209)) {
          if (const auto ins25 = table_42.TryChangeAbsentToPresent({v208, v209}); ins25.changed) {
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
  if (find_207(v208, v209)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_211(bool v212, int32_t v213) {
  if (find_216(v212, v213)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_216(bool v217, int32_t v218) {
  if (find_220(v218)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_220(int32_t v221) {
  for (uint32_t s224 = idx_223.First({v221}); s224 != ::hyde::rt::kNoRow; s224 = idx_223.Next(s224)) {
    const auto r224 = table_22.RowAt(s224);
    const auto v225 = r224.c;
    const auto v226 = r224.c1;
    if (v221 == v225) {
      if (find_227(v225, v226)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_227(int32_t v228, int32_t v229) {
  switch (table_22.State({v228, v229})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_22.TryChangeUnknownToAbsent({v228, v229})) {
        if (find_231(v228, v229)) {
          if (const auto ins26 = table_22.TryChangeAbsentToPresent({v228, v229}); ins26.changed) {
            if (ins26.added_row) {
              idx_223.Add({v228}, ins26.id);
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
  if (find_227(v228, v229)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_231(int32_t v232, int32_t v233) {
  if (find_236(v232, v233)) {
    if (find_113(v232, v233)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_236(int32_t v237, int32_t v238) {
  switch (table_75.State({v237, v238})) {
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

bool Database::find_241(int32_t v242) {
  if (find_244(v242)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_244(int32_t v245) {
  if (find_152(v245)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_248(int32_t v249) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_252(v249)) {
    return true;
  }
  return false;
}

bool Database::find_252(int32_t v253) {
  switch (table_36.State({v253})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_36.TryChangeUnknownToAbsent({v253})) {
        if (find_255(v253)) {
          if (const auto ins27 = table_36.TryChangeAbsentToPresent({v253}); ins27.changed) {
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
  if (find_258(v253)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_255(int32_t v256) {
  if (find_263(v256)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_258(int32_t v259) {
  switch (table_36.State({v259})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_36.TryChangeUnknownToAbsent({v259})) {
        if (find_255(v259)) {
          if (const auto ins28 = table_36.TryChangeAbsentToPresent({v259}); ins28.changed) {
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
  if (find_258(v259)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_263(int32_t v264) {
  if (find_266(v264)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_266(int32_t v267) {
  for (uint32_t s270 = idx_269.First({v267}); s270 != ::hyde::rt::kNoRow; s270 = idx_269.Next(s270)) {
    const auto r270 = table_18.RowAt(s270);
    const auto v271 = r270.a;
    const auto v272 = r270.c1;
    if (v267 == v271) {
      if (find_273(v271, v272)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_273(int32_t v274, int32_t v275) {
  switch (table_18.State({v274, v275})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v274, v275})) {
        if (find_277(v274, v275)) {
          if (const auto ins29 = table_18.TryChangeAbsentToPresent({v274, v275}); ins29.changed) {
            if (ins29.added_row) {
              idx_269.Add({v274}, ins29.id);
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
  if (find_273(v274, v275)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_277(int32_t v278, int32_t v279) {
  if (find_282(v278, v279)) {
    if (find_109(v278, v279)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_282(int32_t v283, int32_t v284) {
  switch (table_67.State({v283, v284})) {
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

bool Database::find_287(int32_t v288) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_291(v288)) {
    return true;
  }
  return false;
}

bool Database::find_291(int32_t v292) {
  switch (table_33.State({v292})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_33.TryChangeUnknownToAbsent({v292})) {
        if (find_294(v292)) {
          if (const auto ins30 = table_33.TryChangeAbsentToPresent({v292}); ins30.changed) {
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
  if (find_297(v292)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_294(int32_t v295) {
  if (find_302(v295)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_297(int32_t v298) {
  switch (table_33.State({v298})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_33.TryChangeUnknownToAbsent({v298})) {
        if (find_294(v298)) {
          if (const auto ins31 = table_33.TryChangeAbsentToPresent({v298}); ins31.changed) {
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
  if (find_297(v298)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_302(int32_t v303) {
  if (find_305(v303)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_305(int32_t v306) {
  for (uint32_t s309 = idx_308.First({v306}); s309 != ::hyde::rt::kNoRow; s309 = idx_308.Next(s309)) {
    const auto r309 = table_14.RowAt(s309);
    const auto v310 = r309.a;
    const auto v311 = r309.c1;
    if (v306 == v310) {
      if (find_312(v310, v311)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_312(int32_t v313, int32_t v314) {
  switch (table_14.State({v313, v314})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_14.TryChangeUnknownToAbsent({v313, v314})) {
        if (find_316(v313, v314)) {
          if (const auto ins32 = table_14.TryChangeAbsentToPresent({v313, v314}); ins32.changed) {
            if (ins32.added_row) {
              idx_308.Add({v313}, ins32.id);
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
  if (find_312(v313, v314)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_316(int32_t v317, int32_t v318) {
  if (find_321(v317, v318)) {
    if (find_105(v317, v318)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_321(int32_t v322, int32_t v323) {
  switch (table_59.State({v322, v323})) {
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

bool Database::flow_326(::hyde::rt::Vec<Tup_b> vec81) {
  if ((g80 += 1) == 1) {
    if (const auto ins33 = table_7.TryChangeAbsentToPresent({1, true}); ins33.changed) {
      if (ins33.added_row) {
        idx_98.Add({true}, ins33.id);
      }
      vec81.Add({true});
    }
  }
  vec81.SortAndUnique();
  for (auto [v118] : vec81) {
    if (const uint32_t j117_0 = __11.Find({v118}); j117_0 != ::hyde::rt::kNoRow) {
      const auto r117_0 = __11.RowAt(j117_0);
      const auto v120 = r117_0.c0;
      for (uint32_t j117_1 = idx_98.First({v118}); j117_1 != ::hyde::rt::kNoRow; j117_1 = idx_98.Next(j117_1)) {
        const auto r117_1 = table_7.RowAt(j117_1);
        const auto v121 = r117_1.c0;
        const auto v119 = r117_1.c1;
        if (v118 == v119 && v118 == v120) {
          switch (__11.State({v119})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins34 = table_26.TryChangeAbsentOrUnknownToPresent({v119, v121}); ins34.changed) {
                if (ins34.added_row) {
                  idx_173.Add({v121}, ins34.id);
                }
                if (const auto ins35 = table_39.TryChangeAbsentOrUnknownToPresent({v121}); ins35.changed) {
                  if (const auto ins36 = table_30.TryChangeAbsentOrUnknownToPresent({v121}); ins36.changed) {
                    if (const auto ins37 = outflag_52.TryChangeAbsentOrUnknownToPresent({v121}); ins37.changed) {
                    }
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
  vec81.Clear();
  return true;
}

Database::out7_f_cursor Database::out7_f() {
  return {*this, 0};
}

bool Database::out7_f_cursor::next(int32_t &A) {
  while (pos < db.out7_46.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out7_46.RowAt(id);
    if (!db.find_133(row.a)) {
      continue;
    }
    A = row.a;
    return true;
  }
  return false;
}

Database::outdup_f_cursor Database::outdup_f() {
  return {*this, 0};
}

bool Database::outdup_f_cursor::next(int32_t &A) {
  while (pos < db.outdup_49.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.outdup_49.RowAt(id);
    if (!db.find_135(row.a)) {
      continue;
    }
    A = row.a;
    return true;
  }
  return false;
}

Database::outflag_f_cursor Database::outflag_f() {
  return {*this, 0};
}

bool Database::outflag_f_cursor::next(int32_t &A) {
  while (pos < db.outflag_52.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.outflag_52.RowAt(id);
    if (!db.find_137(row.a)) {
      continue;
    }
    A = row.a;
    return true;
  }
  return false;
}

