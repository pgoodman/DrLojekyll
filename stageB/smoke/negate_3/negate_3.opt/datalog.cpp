// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_7(allocator_),
      idx_86(allocator_),
      __11(allocator_),
      table_14(allocator_),
      idx_180(allocator_),
      table_18(allocator_),
      idx_230(allocator_),
      table_22(allocator_),
      idx_205(allocator_),
      table_26(allocator_),
      idx_134(allocator_),
      table_30(allocator_),
      out7_34(allocator_),
      outdup_37(allocator_),
      outflag_40(allocator_),
      table_43(allocator_),
      table_47(allocator_),
      table_51(allocator_),
      table_55(allocator_),
      table_59(allocator_),
      table_63(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32_i32> vec119(allocator);
  ::hyde::rt::Vec<Tup_i32> vec120(allocator);
  proc_67(std::move(vec119), std::move(vec120));
  return false;
}

bool Database::proc_67(::hyde::rt::Vec<Tup_i32_i32> vec70, ::hyde::rt::Vec<Tup_i32> vec90) {
  ::hyde::rt::Vec<Tup_b> vec69(allocator);
  for (auto [v72, v73] : vec70) {
    if (7 == v73) {
      if (const auto ins0 = table_43.TryChangeAbsentToPresent({v72, 7}); ins0.changed) {
        if (find_74(v72, 7)) {
          if (table_18.TryChangePresentToUnknown({v72, 7})) {
            out7_34.TryChangePresentToUnknown({v72});
          }
        }
      }
    }
    if (v73 == v72) {
      if (const auto ins1 = table_51.TryChangeAbsentToPresent({v72, v72}); ins1.changed) {
        if (find_78(v72, v72)) {
          if (table_22.TryChangePresentToUnknown({v72, v72})) {
            outdup_37.TryChangePresentToUnknown({v72});
          }
        }
      }
    }
    if (1 == v72) {
      if (7 == v73) {
        if (const auto ins2 = table_59.TryChangeAbsentToPresent({1, 7}); ins2.changed) {
          if (find_82(1, 7)) {
            if (table_14.TryChangePresentToUnknown({1, 7})) {
              if (table_30.TryChangePresentToUnknown({true, 1})) {
                if (__11.TryChangePresentToUnknown({true})) {
                  for (uint32_t s87 = idx_86.First({true}); s87 != ::hyde::rt::kNoRow; s87 = idx_86.Next(s87)) {
                    const auto r87 = table_7.RowAt(s87);
                    const auto v88 = r87.c0;
                    const auto v89 = r87.c1;
                    if (true == v89) {
                      if (table_26.TryChangePresentToUnknown({true, v88})) {
                        outflag_40.TryChangePresentToUnknown({v88});
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
  for (auto [v92] : vec90) {
    if (const auto ins3 = table_47.TryChangeAbsentToPresent({v92, 7}); ins3.changed) {
      if (!find_93(v92, 7)) {
        if (const auto ins4 = table_18.TryChangeAbsentOrUnknownToPresent({v92, 7}); ins4.changed) {
          if (ins4.added_row) {
            idx_230.Add({v92}, ins4.id);
          }
          out7_34.TryChangeAbsentOrUnknownToPresent({v92});
        }
      }
    }
    if (const auto ins5 = table_55.TryChangeAbsentToPresent({v92, v92}); ins5.changed) {
      if (!find_97(v92, v92)) {
        if (const auto ins6 = table_22.TryChangeAbsentOrUnknownToPresent({v92, v92}); ins6.changed) {
          if (ins6.added_row) {
            idx_205.Add({v92}, ins6.id);
          }
          outdup_37.TryChangeAbsentOrUnknownToPresent({v92});
        }
      }
    }
    if (1 == v92) {
      if (const auto ins7 = table_63.TryChangeAbsentToPresent({1, 7}); ins7.changed) {
        if (!find_101(1, 7)) {
          if (const auto ins8 = table_14.TryChangeAbsentOrUnknownToPresent({1, 7}); ins8.changed) {
            if (ins8.added_row) {
              idx_180.Add({1}, ins8.id);
            }
            if (const auto ins9 = table_30.TryChangeAbsentOrUnknownToPresent({true, 1}); ins9.changed) {
              if (const auto ins10 = __11.TryChangeAbsentOrUnknownToPresent({true}); ins10.changed) {
                vec69.Add({true});
              }
            }
          }
        }
      }
    }
  }
  vec70.Clear();
  vec90.Clear();
  flow_248(std::move(vec69));
  return false;
}

bool Database::find_74(int32_t v75, int32_t v76) {
  switch (table_47.State({v75, v76})) {
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

bool Database::find_78(int32_t v79, int32_t v80) {
  switch (table_55.State({v79, v80})) {
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

bool Database::find_82(int32_t v83, int32_t v84) {
  switch (table_63.State({v83, v84})) {
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

bool Database::find_93(int32_t v94, int32_t v95) {
  switch (table_43.State({v94, v95})) {
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

bool Database::find_97(int32_t v98, int32_t v99) {
  switch (table_51.State({v98, v99})) {
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

bool Database::find_101(int32_t v102, int32_t v103) {
  switch (table_59.State({v102, v103})) {
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

bool Database::pin_2(::hyde::rt::Vec<Tup_i32_i32> vec111) {
  ::hyde::rt::Vec<Tup_i32> vec113(allocator);
  proc_67(std::move(vec111), std::move(vec113));
  return true;
}

bool Database::feed_1(::hyde::rt::Vec<Tup_i32> vec115) {
  ::hyde::rt::Vec<Tup_i32_i32> vec117(allocator);
  proc_67(std::move(vec117), std::move(vec115));
  return true;
}

bool Database::find_121(int32_t v122) {
  switch (out7_34.State({v122})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (out7_34.TryChangeUnknownToAbsent({v122})) {
        if (find_223(v122)) {
          if (const auto ins11 = out7_34.TryChangeAbsentToPresent({v122}); ins11.changed) {
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
  if (find_121(v122)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_123(int32_t v124) {
  switch (outdup_37.State({v124})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (outdup_37.TryChangeUnknownToAbsent({v124})) {
        if (find_198(v124)) {
          if (const auto ins12 = outdup_37.TryChangeAbsentToPresent({v124}); ins12.changed) {
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
  if (find_123(v124)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_125(int32_t v126) {
  switch (outflag_40.State({v126})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (outflag_40.TryChangeUnknownToAbsent({v126})) {
        if (find_127(v126)) {
          if (const auto ins13 = outflag_40.TryChangeAbsentToPresent({v126}); ins13.changed) {
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
  if (find_125(v126)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_127(int32_t v128) {
  if (find_131(v128)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_131(int32_t v132) {
  for (uint32_t s135 = idx_134.First({v132}); s135 != ::hyde::rt::kNoRow; s135 = idx_134.Next(s135)) {
    const auto r135 = table_26.RowAt(s135);
    const auto v136 = r135.c0;
    const auto v137 = r135.c1;
    if (v137 == v132) {
      if (find_138(v136, v137)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_138(bool v139, int32_t v140) {
  switch (table_26.State({v139, v140})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_26.TryChangeUnknownToAbsent({v139, v140})) {
        if (find_142(v139, v140)) {
          if (const auto ins14 = table_26.TryChangeAbsentToPresent({v139, v140}); ins14.changed) {
            if (ins14.added_row) {
              idx_134.Add({v140}, ins14.id);
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
  if (find_138(v139, v140)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_142(bool v143, int32_t v144) {
  if (find_147(v144, v143)) {
    if (find_151(v143)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_147(int32_t v148, bool v149) {
  switch (table_7.State({v148, v149})) {
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

bool Database::find_151(bool v152) {
  switch (__11.State({v152})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__11.TryChangeUnknownToAbsent({v152})) {
        if (find_154(v152)) {
          if (const auto ins15 = __11.TryChangeAbsentToPresent({v152}); ins15.changed) {
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
  if (find_151(v152)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_154(bool v155) {
  if (find_162(v155)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_162(bool v163) {
  for (uint32_t s165 = 0; s165 < table_30.NumRows(); ++s165) {
    const auto r165 = table_30.RowAt(s165);
    const auto v166 = r165.c0;
    const auto v167 = r165.c;
    if (v166 == v163) {
      if (find_168(v166, v167)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_168(bool v169, int32_t v170) {
  switch (table_30.State({v169, v170})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_30.TryChangeUnknownToAbsent({v169, v170})) {
        if (find_172(v169, v170)) {
          if (const auto ins16 = table_30.TryChangeAbsentToPresent({v169, v170}); ins16.changed) {
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
  if (find_168(v169, v170)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_172(bool v173, int32_t v174) {
  if (find_177(v174)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_177(int32_t v178) {
  for (uint32_t s181 = idx_180.First({v178}); s181 != ::hyde::rt::kNoRow; s181 = idx_180.Next(s181)) {
    const auto r181 = table_14.RowAt(s181);
    const auto v182 = r181.c;
    const auto v183 = r181.c1;
    if (v182 == v178) {
      if (find_184(v182, v183)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_184(int32_t v185, int32_t v186) {
  switch (table_14.State({v185, v186})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_14.TryChangeUnknownToAbsent({v185, v186})) {
        if (find_188(v185, v186)) {
          if (const auto ins17 = table_14.TryChangeAbsentToPresent({v185, v186}); ins17.changed) {
            if (ins17.added_row) {
              idx_180.Add({v185}, ins17.id);
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
  if (find_184(v185, v186)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_188(int32_t v189, int32_t v190) {
  if (find_82(v189, v190)) {
    if (find_101(v189, v190)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_198(int32_t v199) {
  if (find_202(v199)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_202(int32_t v203) {
  for (uint32_t s206 = idx_205.First({v203}); s206 != ::hyde::rt::kNoRow; s206 = idx_205.Next(s206)) {
    const auto r206 = table_22.RowAt(s206);
    const auto v207 = r206.a;
    const auto v208 = r206.c1;
    if (v207 == v203) {
      if (find_209(v207, v208)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_209(int32_t v210, int32_t v211) {
  switch (table_22.State({v210, v211})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_22.TryChangeUnknownToAbsent({v210, v211})) {
        if (find_213(v210, v211)) {
          if (const auto ins18 = table_22.TryChangeAbsentToPresent({v210, v211}); ins18.changed) {
            if (ins18.added_row) {
              idx_205.Add({v210}, ins18.id);
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
  if (find_209(v210, v211)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_213(int32_t v214, int32_t v215) {
  if (find_78(v214, v215)) {
    if (find_97(v214, v215)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_223(int32_t v224) {
  if (find_227(v224)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_227(int32_t v228) {
  for (uint32_t s231 = idx_230.First({v228}); s231 != ::hyde::rt::kNoRow; s231 = idx_230.Next(s231)) {
    const auto r231 = table_18.RowAt(s231);
    const auto v232 = r231.a;
    const auto v233 = r231.c1;
    if (v232 == v228) {
      if (find_234(v232, v233)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_234(int32_t v235, int32_t v236) {
  switch (table_18.State({v235, v236})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v235, v236})) {
        if (find_238(v235, v236)) {
          if (const auto ins19 = table_18.TryChangeAbsentToPresent({v235, v236}); ins19.changed) {
            if (ins19.added_row) {
              idx_230.Add({v235}, ins19.id);
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
  if (find_234(v235, v236)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_238(int32_t v239, int32_t v240) {
  if (find_74(v239, v240)) {
    if (find_93(v239, v240)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::flow_248(::hyde::rt::Vec<Tup_b> vec69) {
  if ((g68 += 1) == 1) {
    if (const auto ins20 = table_7.TryChangeAbsentToPresent({1, true}); ins20.changed) {
      if (ins20.added_row) {
        idx_86.Add({true}, ins20.id);
      }
      vec69.Add({true});
    }
  }
  vec69.SortAndUnique();
  for (auto [v106] : vec69) {
    if (const uint32_t j105_0 = __11.Find({v106}); j105_0 != ::hyde::rt::kNoRow) {
      const auto r105_0 = __11.RowAt(j105_0);
      const auto v108 = r105_0.c0;
      for (uint32_t j105_1 = idx_86.First({v106}); j105_1 != ::hyde::rt::kNoRow; j105_1 = idx_86.Next(j105_1)) {
        const auto r105_1 = table_7.RowAt(j105_1);
        const auto v109 = r105_1.c0;
        const auto v107 = r105_1.c1;
        if (v106 == v107 && v106 == v108) {
          switch (__11.State({v107})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins21 = table_26.TryChangeAbsentOrUnknownToPresent({v107, v109}); ins21.changed) {
                if (ins21.added_row) {
                  idx_134.Add({v109}, ins21.id);
                }
                outflag_40.TryChangeAbsentOrUnknownToPresent({v109});
              }
              break;
            }
            default: break;
          }
        }
      }
    }
  }
  vec69.Clear();
  return true;
}

Database::out7_f_cursor Database::out7_f() {
  return {*this, 0};
}

bool Database::out7_f_cursor::next(int32_t &A) {
  while (pos < db.out7_34.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out7_34.RowAt(id);
    if (!db.find_121(row.a)) {
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
  while (pos < db.outdup_37.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.outdup_37.RowAt(id);
    if (!db.find_123(row.a)) {
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
  while (pos < db.outflag_40.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.outflag_40.RowAt(id);
    if (!db.find_125(row.c0)) {
      continue;
    }
    A = row.c0;
    return true;
  }
  return false;
}

