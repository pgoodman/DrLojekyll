// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_112(allocator_),
      __9(allocator_),
      table_12(allocator_),
      idx_87(allocator_),
      __16(allocator_),
      table_19(allocator_),
      idx_220(allocator_),
      table_23(allocator_),
      idx_192(allocator_),
      table_27(allocator_),
      table_30(allocator_),
      table_33(allocator_),
      idx_310(allocator_),
      table_37(allocator_),
      idx_232(allocator_),
      mixed_41(allocator_),
      table_44(allocator_),
      table_47(allocator_),
      table_50(allocator_),
      table_53(allocator_),
      table_57(allocator_),
      pos_neg_61(allocator_),
      both_pos_64(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec161(allocator);
  ::hyde::rt::Vec<Tup_i32> vec162(allocator);
  ::hyde::rt::Vec<Tup_i32> vec163(allocator);
  ::hyde::rt::Vec<Tup_i32> vec164(allocator);
  proc_67(std::move(vec161), std::move(vec161), std::move(vec162), std::move(vec162), std::move(vec163), std::move(vec164));
  return false;
}

bool Database::proc_67(::hyde::rt::Vec<Tup_i32> vec69, ::hyde::rt::Vec<Tup_i32> vec70, ::hyde::rt::Vec<Tup_i32> vec91, ::hyde::rt::Vec<Tup_i32> vec92, ::hyde::rt::Vec<Tup_i32> vec116, ::hyde::rt::Vec<Tup_i32> vec119) {
  ::hyde::rt::Vec<Tup_b> vec78(allocator);
  ::hyde::rt::Vec<Tup_b> vec102(allocator);
  for (auto [v72] : vec69) {
    if (const auto ins0 = table_27.TryChangeAbsentToPresent({v72}); ins0.changed) {
      if (const auto ins1 = table_53.TryChangeAbsentOrUnknownToPresent({true, v72}); ins1.changed) {
        if (const auto ins2 = __16.TryChangeAbsentOrUnknownToPresent({true}); ins2.changed) {
          for (uint32_t s73 = 0; s73 < table_50.NumRows(); ++s73) {
            const auto r73 = table_50.RowAt(s73);
            const auto v74 = r73.x;
            if (find_75(v74)) {
              if (table_23.TryChangePresentToUnknown({true, v74})) {
                if (table_47.TryChangePresentToUnknown({v74})) {
                  mixed_41.TryChangePresentToUnknown({v74});
                }
              }
            }
          }
          vec78.Add({true});
        }
      }
    }
  }
  for (auto [v80] : vec70) {
    if (table_27.TryChangePresentToAbsent({v80})) {
      if (table_53.TryChangePresentToUnknown({true, v80})) {
        if (__16.TryChangePresentToUnknown({true})) {
          for (uint32_t s81 = 0; s81 < table_50.NumRows(); ++s81) {
            const auto r81 = table_50.RowAt(s81);
            const auto v82 = r81.x;
            if (find_75(v82)) {
              if (!find_84(true)) {
                if (const auto ins3 = table_23.TryChangeAbsentOrUnknownToPresent({true, v82}); ins3.changed) {
                  if (ins3.added_row) {
                    idx_192.Add({v82}, ins3.id);
                  }
                  if (const auto ins4 = table_47.TryChangeAbsentOrUnknownToPresent({v82}); ins4.changed) {
                    mixed_41.TryChangeAbsentOrUnknownToPresent({v82});
                  }
                }
              }
            }
          }
          for (uint32_t s88 = idx_87.First({true}); s88 != ::hyde::rt::kNoRow; s88 = idx_87.Next(s88)) {
            const auto r88 = table_12.RowAt(s88);
            const auto v89 = r88.x;
            const auto v90 = r88.c1;
            if (true == v90) {
              if (table_19.TryChangePresentToUnknown({true, v89})) {
                if (table_33.TryChangePresentToUnknown({true, v89})) {
                  pos_neg_61.TryChangePresentToUnknown({v89});
                }
                if (table_5.TryChangePresentToUnknown({v89, true})) {
                  if (table_37.TryChangePresentToUnknown({true, v89})) {
                    both_pos_64.TryChangePresentToUnknown({v89});
                  }
                }
                if (table_44.TryChangePresentToUnknown({v89})) {
                  mixed_41.TryChangePresentToUnknown({v89});
                }
              }
            }
          }
        }
      }
    }
  }
  for (auto [v94] : vec91) {
    if (const auto ins5 = table_30.TryChangeAbsentToPresent({v94}); ins5.changed) {
      if (const auto ins6 = table_57.TryChangeAbsentOrUnknownToPresent({true, v94}); ins6.changed) {
        if (const auto ins7 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins7.changed) {
          for (uint32_t s95 = 0; s95 < table_19.NumRows(); ++s95) {
            const auto r95 = table_19.RowAt(s95);
            const auto v96 = r95.c0;
            const auto v97 = r95.x;
            if (find_98(v96, v97)) {
              if (table_33.TryChangePresentToUnknown({true, v97})) {
                pos_neg_61.TryChangePresentToUnknown({v97});
              }
            }
          }
          vec102.Add({true});
        }
      }
    }
  }
  for (auto [v104] : vec92) {
    if (table_30.TryChangePresentToAbsent({v104})) {
      if (table_57.TryChangePresentToUnknown({true, v104})) {
        if (__9.TryChangePresentToUnknown({true})) {
          for (uint32_t s105 = 0; s105 < table_19.NumRows(); ++s105) {
            const auto r105 = table_19.RowAt(s105);
            const auto v106 = r105.c0;
            const auto v107 = r105.x;
            if (find_98(v106, v107)) {
              if (!find_109(true)) {
                if (const auto ins8 = table_33.TryChangeAbsentOrUnknownToPresent({true, v107}); ins8.changed) {
                  if (ins8.added_row) {
                    idx_310.Add({v107}, ins8.id);
                  }
                  pos_neg_61.TryChangeAbsentOrUnknownToPresent({v107});
                }
              }
            }
          }
          for (uint32_t s113 = idx_112.First({true}); s113 != ::hyde::rt::kNoRow; s113 = idx_112.Next(s113)) {
            const auto r113 = table_5.RowAt(s113);
            const auto v114 = r113.x;
            const auto v115 = r113.c1;
            if (true == v115) {
              if (table_37.TryChangePresentToUnknown({true, v114})) {
                both_pos_64.TryChangePresentToUnknown({v114});
              }
            }
          }
        }
      }
    }
  }
  for (auto [v118] : vec116) {
    if (const auto ins9 = table_12.TryChangeAbsentToPresent({v118, true}); ins9.changed) {
      if (ins9.added_row) {
        idx_87.Add({true}, ins9.id);
      }
      vec78.Add({true});
    }
  }
  for (auto [v121] : vec119) {
    if (const auto ins10 = table_50.TryChangeAbsentToPresent({v121}); ins10.changed) {
      if (!find_84(true)) {
        if (const auto ins11 = table_23.TryChangeAbsentOrUnknownToPresent({true, v121}); ins11.changed) {
          if (ins11.added_row) {
            idx_192.Add({v121}, ins11.id);
          }
          if (const auto ins12 = table_47.TryChangeAbsentOrUnknownToPresent({v121}); ins12.changed) {
            mixed_41.TryChangeAbsentOrUnknownToPresent({v121});
          }
        }
      }
    }
  }
  vec69.Clear();
  vec70.Clear();
  vec91.Clear();
  vec92.Clear();
  vec116.Clear();
  vec119.Clear();
  flow_387(std::move(vec78), std::move(vec102));
  return false;
}

bool Database::find_75(int32_t v76) {
  switch (table_50.State({v76})) {
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

bool Database::find_84(bool v85) {
  switch (__16.State({v85})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__16.TryChangeUnknownToAbsent({v85})) {
        if (find_348(v85)) {
          if (const auto ins13 = __16.TryChangeAbsentToPresent({v85}); ins13.changed) {
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
  if (find_84(v85)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_98(bool v99, int32_t v100) {
  switch (table_19.State({v99, v100})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_19.TryChangeUnknownToAbsent({v99, v100})) {
        if (find_336(v99, v100)) {
          if (const auto ins14 = table_19.TryChangeAbsentToPresent({v99, v100}); ins14.changed) {
            if (ins14.added_row) {
              idx_220.Add({v100}, ins14.id);
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
  if (find_98(v99, v100)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_109(bool v110) {
  switch (__9.State({v110})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v110})) {
        if (find_252(v110)) {
          if (const auto ins15 = __9.TryChangeAbsentToPresent({v110}); ins15.changed) {
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
  if (find_109(v110)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::set_c1_1(::hyde::rt::Vec<Tup_i32> vec135, ::hyde::rt::Vec<Tup_i32> vec136) {
  ::hyde::rt::Vec<Tup_i32> vec138(allocator);
  ::hyde::rt::Vec<Tup_i32> vec139(allocator);
  ::hyde::rt::Vec<Tup_i32> vec140(allocator);
  proc_67(std::move(vec135), std::move(vec136), std::move(vec138), std::move(vec138), std::move(vec139), std::move(vec140));
  return true;
}

bool Database::set_c2_1(::hyde::rt::Vec<Tup_i32> vec142, ::hyde::rt::Vec<Tup_i32> vec143) {
  ::hyde::rt::Vec<Tup_i32> vec145(allocator);
  ::hyde::rt::Vec<Tup_i32> vec146(allocator);
  ::hyde::rt::Vec<Tup_i32> vec147(allocator);
  proc_67(std::move(vec145), std::move(vec145), std::move(vec142), std::move(vec143), std::move(vec146), std::move(vec147));
  return true;
}

bool Database::add_r_1(::hyde::rt::Vec<Tup_i32> vec149) {
  ::hyde::rt::Vec<Tup_i32> vec151(allocator);
  ::hyde::rt::Vec<Tup_i32> vec152(allocator);
  ::hyde::rt::Vec<Tup_i32> vec153(allocator);
  proc_67(std::move(vec151), std::move(vec151), std::move(vec152), std::move(vec152), std::move(vec149), std::move(vec153));
  return true;
}

bool Database::add_s_1(::hyde::rt::Vec<Tup_i32> vec155) {
  ::hyde::rt::Vec<Tup_i32> vec157(allocator);
  ::hyde::rt::Vec<Tup_i32> vec158(allocator);
  ::hyde::rt::Vec<Tup_i32> vec159(allocator);
  proc_67(std::move(vec157), std::move(vec157), std::move(vec158), std::move(vec158), std::move(vec159), std::move(vec155));
  return true;
}

bool Database::find_165(int32_t v166) {
  switch (pos_neg_61.State({v166})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (pos_neg_61.TryChangeUnknownToAbsent({v166})) {
        if (find_303(v166)) {
          if (const auto ins16 = pos_neg_61.TryChangeAbsentToPresent({v166}); ins16.changed) {
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
  switch (both_pos_64.State({v168})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (both_pos_64.TryChangeUnknownToAbsent({v168})) {
        if (find_225(v168)) {
          if (const auto ins17 = both_pos_64.TryChangeAbsentToPresent({v168}); ins17.changed) {
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
  if (find_167(v168)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_169(int32_t v170) {
  switch (mixed_41.State({v170})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (mixed_41.TryChangeUnknownToAbsent({v170})) {
        if (find_171(v170)) {
          if (const auto ins18 = mixed_41.TryChangeAbsentToPresent({v170}); ins18.changed) {
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
  if (find_169(v170)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_171(int32_t v172) {
  if (find_175(v172)) {
    return true;
  }
  if (find_178(v172)) {
    return true;
  }
  return false;
}

bool Database::find_175(int32_t v176) {
  switch (table_44.State({v176})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_44.TryChangeUnknownToAbsent({v176})) {
        if (find_209(v176)) {
          if (const auto ins19 = table_44.TryChangeAbsentToPresent({v176}); ins19.changed) {
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
  if (find_175(v176)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_178(int32_t v179) {
  switch (table_47.State({v179})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_47.TryChangeUnknownToAbsent({v179})) {
        if (find_181(v179)) {
          if (const auto ins20 = table_47.TryChangeAbsentToPresent({v179}); ins20.changed) {
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
  if (find_178(v179)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_181(int32_t v182) {
  if (find_189(v182)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_189(int32_t v190) {
  for (uint32_t s193 = idx_192.First({v190}); s193 != ::hyde::rt::kNoRow; s193 = idx_192.Next(s193)) {
    const auto r193 = table_23.RowAt(s193);
    const auto v194 = r193.c0;
    const auto v195 = r193.x;
    if (v195 == v190) {
      if (find_196(v194, v195)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_196(bool v197, int32_t v198) {
  switch (table_23.State({v197, v198})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_23.TryChangeUnknownToAbsent({v197, v198})) {
        if (find_200(v197, v198)) {
          if (const auto ins21 = table_23.TryChangeAbsentToPresent({v197, v198}); ins21.changed) {
            if (ins21.added_row) {
              idx_192.Add({v198}, ins21.id);
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
  if (find_196(v197, v198)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_200(bool v201, int32_t v202) {
  if (find_75(v202)) {
    if (find_84(v201)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_209(int32_t v210) {
  if (find_217(v210)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_217(int32_t v218) {
  for (uint32_t s221 = idx_220.First({v218}); s221 != ::hyde::rt::kNoRow; s221 = idx_220.Next(s221)) {
    const auto r221 = table_19.RowAt(s221);
    const auto v222 = r221.c0;
    const auto v223 = r221.x;
    if (v223 == v218) {
      if (find_98(v222, v223)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_225(int32_t v226) {
  if (find_229(v226)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_229(int32_t v230) {
  for (uint32_t s233 = idx_232.First({v230}); s233 != ::hyde::rt::kNoRow; s233 = idx_232.Next(s233)) {
    const auto r233 = table_37.RowAt(s233);
    const auto v234 = r233.c0;
    const auto v235 = r233.x;
    if (v235 == v230) {
      if (find_236(v234, v235)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_236(bool v237, int32_t v238) {
  switch (table_37.State({v237, v238})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_37.TryChangeUnknownToAbsent({v237, v238})) {
        if (find_240(v237, v238)) {
          if (const auto ins22 = table_37.TryChangeAbsentToPresent({v237, v238}); ins22.changed) {
            if (ins22.added_row) {
              idx_232.Add({v238}, ins22.id);
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
  if (find_236(v237, v238)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_240(bool v241, int32_t v242) {
  if (find_245(v242, v241)) {
    if (find_109(v241)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_245(int32_t v246, bool v247) {
  switch (table_5.State({v246, v247})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v246, v247})) {
        if (find_286(v246, v247)) {
          if (const auto ins23 = table_5.TryChangeAbsentToPresent({v246, v247}); ins23.changed) {
            if (ins23.added_row) {
              idx_112.Add({v247}, ins23.id);
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
  if (find_245(v246, v247)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_252(bool v253) {
  if (find_260(v253)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_260(bool v261) {
  for (uint32_t s263 = 0; s263 < table_57.NumRows(); ++s263) {
    const auto r263 = table_57.RowAt(s263);
    const auto v264 = r263.c0;
    const auto v265 = r263.x;
    if (v264 == v261) {
      if (find_266(v264, v265)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_266(bool v267, int32_t v268) {
  switch (table_57.State({v267, v268})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_57.TryChangeUnknownToAbsent({v267, v268})) {
        if (find_270(v267, v268)) {
          if (const auto ins24 = table_57.TryChangeAbsentToPresent({v267, v268}); ins24.changed) {
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
  if (find_266(v267, v268)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_270(bool v271, int32_t v272) {
  if (find_275(v272)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_275(int32_t v276) {
  switch (table_30.State({v276})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_30.TryChangeUnknownToAbsent({v276})) {
        if (find_278(v276)) {
          if (const auto ins25 = table_30.TryChangeAbsentToPresent({v276}); ins25.changed) {
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
  if (find_275(v276)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_278(int32_t v279) {
  return false;
}

bool Database::find_286(int32_t v287, bool v288) {
  if (find_217(v287)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_303(int32_t v304) {
  if (find_307(v304)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_307(int32_t v308) {
  for (uint32_t s311 = idx_310.First({v308}); s311 != ::hyde::rt::kNoRow; s311 = idx_310.Next(s311)) {
    const auto r311 = table_33.RowAt(s311);
    const auto v312 = r311.c0;
    const auto v313 = r311.x;
    if (v313 == v308) {
      if (find_314(v312, v313)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_314(bool v315, int32_t v316) {
  switch (table_33.State({v315, v316})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_33.TryChangeUnknownToAbsent({v315, v316})) {
        if (find_318(v315, v316)) {
          if (const auto ins26 = table_33.TryChangeAbsentToPresent({v315, v316}); ins26.changed) {
            if (ins26.added_row) {
              idx_310.Add({v316}, ins26.id);
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
  if (find_314(v315, v316)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_318(bool v319, int32_t v320) {
  if (find_217(v320)) {
    if (find_109(v319)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_336(bool v337, int32_t v338) {
  if (find_341(v338, v337)) {
    if (find_84(v337)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_341(int32_t v342, bool v343) {
  switch (table_12.State({v342, v343})) {
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

bool Database::find_348(bool v349) {
  if (find_356(v349)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_356(bool v357) {
  for (uint32_t s359 = 0; s359 < table_53.NumRows(); ++s359) {
    const auto r359 = table_53.RowAt(s359);
    const auto v360 = r359.c0;
    const auto v361 = r359.x;
    if (v360 == v357) {
      if (find_362(v360, v361)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_362(bool v363, int32_t v364) {
  switch (table_53.State({v363, v364})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_53.TryChangeUnknownToAbsent({v363, v364})) {
        if (find_366(v363, v364)) {
          if (const auto ins27 = table_53.TryChangeAbsentToPresent({v363, v364}); ins27.changed) {
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
  if (find_362(v363, v364)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_366(bool v367, int32_t v368) {
  if (find_371(v368)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_371(int32_t v372) {
  switch (table_27.State({v372})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_27.TryChangeUnknownToAbsent({v372})) {
        if (find_278(v372)) {
          if (const auto ins28 = table_27.TryChangeAbsentToPresent({v372}); ins28.changed) {
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
  if (find_371(v372)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::flow_387(::hyde::rt::Vec<Tup_b> vec78, ::hyde::rt::Vec<Tup_b> vec102) {
  g68 += 1;
  vec78.SortAndUnique();
  for (auto [v124] : vec78) {
    if (const uint32_t j123_0 = __16.Find({v124}); j123_0 != ::hyde::rt::kNoRow) {
      const auto r123_0 = __16.RowAt(j123_0);
      const auto v126 = r123_0.c0;
      for (uint32_t j123_1 = idx_87.First({v124}); j123_1 != ::hyde::rt::kNoRow; j123_1 = idx_87.Next(j123_1)) {
        const auto r123_1 = table_12.RowAt(j123_1);
        const auto v127 = r123_1.x;
        const auto v125 = r123_1.c1;
        if (v124 == v125 && v124 == v126) {
          switch (__16.State({v125})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins29 = table_19.TryChangeAbsentOrUnknownToPresent({v125, v127}); ins29.changed) {
                if (ins29.added_row) {
                  idx_220.Add({v127}, ins29.id);
                }
                if (!find_109(true)) {
                  if (const auto ins30 = table_33.TryChangeAbsentOrUnknownToPresent({true, v127}); ins30.changed) {
                    if (ins30.added_row) {
                      idx_310.Add({v127}, ins30.id);
                    }
                    pos_neg_61.TryChangeAbsentOrUnknownToPresent({v127});
                  }
                }
                if (const auto ins31 = table_5.TryChangeAbsentOrUnknownToPresent({v127, true}); ins31.changed) {
                  if (ins31.added_row) {
                    idx_112.Add({true}, ins31.id);
                  }
                  vec102.Add({true});
                }
                if (const auto ins32 = table_44.TryChangeAbsentOrUnknownToPresent({v127}); ins32.changed) {
                  mixed_41.TryChangeAbsentOrUnknownToPresent({v127});
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
  vec78.Clear();
  vec102.SortAndUnique();
  for (auto [v130] : vec102) {
    if (const uint32_t j129_0 = __9.Find({v130}); j129_0 != ::hyde::rt::kNoRow) {
      const auto r129_0 = __9.RowAt(j129_0);
      const auto v132 = r129_0.c0;
      for (uint32_t j129_1 = idx_112.First({v130}); j129_1 != ::hyde::rt::kNoRow; j129_1 = idx_112.Next(j129_1)) {
        const auto r129_1 = table_5.RowAt(j129_1);
        const auto v133 = r129_1.x;
        const auto v131 = r129_1.c1;
        if (v130 == v131 && v130 == v132) {
          switch (table_5.State({v133, v131})) {
            case ::hyde::rt::TupleState::kPresent: {
              switch (__9.State({v131})) {
                case ::hyde::rt::TupleState::kPresent: {
                  if (const auto ins33 = table_37.TryChangeAbsentOrUnknownToPresent({v131, v133}); ins33.changed) {
                    if (ins33.added_row) {
                      idx_232.Add({v133}, ins33.id);
                    }
                    both_pos_64.TryChangeAbsentOrUnknownToPresent({v133});
                  }
                  break;
                }
                default: break;
              }
              break;
            }
            default: break;
          }
        }
      }
    }
  }
  vec102.Clear();
  return true;
}

Database::pos_neg_f_cursor Database::pos_neg_f() {
  return {*this, 0};
}

bool Database::pos_neg_f_cursor::next(int32_t &X) {
  while (pos < db.pos_neg_61.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.pos_neg_61.RowAt(id);
    if (!db.find_165(row.x)) {
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
  while (pos < db.both_pos_64.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.both_pos_64.RowAt(id);
    if (!db.find_167(row.x)) {
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
  while (pos < db.mixed_41.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.mixed_41.RowAt(id);
    if (!db.find_169(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

