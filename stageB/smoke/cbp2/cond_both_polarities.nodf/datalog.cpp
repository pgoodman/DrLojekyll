// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_132(allocator_),
      __9(allocator_),
      table_12(allocator_),
      idx_136(allocator_),
      table_16(allocator_),
      idx_166(allocator_),
      __20(allocator_),
      table_23(allocator_),
      idx_140(allocator_),
      table_27(allocator_),
      table_30(allocator_),
      table_33(allocator_),
      table_36(allocator_),
      table_39(allocator_),
      idx_579(allocator_),
      table_43(allocator_),
      table_46(allocator_),
      idx_542(allocator_),
      table_50(allocator_),
      idx_492(allocator_),
      table_54(allocator_),
      table_57(allocator_),
      idx_390(allocator_),
      table_61(allocator_),
      idx_291(allocator_),
      table_65(allocator_),
      idx_259(allocator_),
      table_69(allocator_),
      table_72(allocator_),
      table_75(allocator_),
      table_78(allocator_),
      table_81(allocator_),
      table_85(allocator_),
      pos_neg_89(allocator_),
      both_pos_92(allocator_),
      mixed_95(allocator_),
      table_98(allocator_),
      idx_148(allocator_),
      table_102(allocator_),
      idx_112(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec225(allocator);
  ::hyde::rt::Vec<Tup_i32> vec226(allocator);
  ::hyde::rt::Vec<Tup_i32> vec227(allocator);
  ::hyde::rt::Vec<Tup_i32> vec228(allocator);
  proc_106(std::move(vec225), std::move(vec225), std::move(vec226), std::move(vec226), std::move(vec227), std::move(vec228));
  return false;
}

bool Database::proc_106(::hyde::rt::Vec<Tup_i32> vec108, ::hyde::rt::Vec<Tup_i32> vec109, ::hyde::rt::Vec<Tup_i32> vec144, ::hyde::rt::Vec<Tup_i32> vec145, ::hyde::rt::Vec<Tup_i32> vec170, ::hyde::rt::Vec<Tup_i32> vec173) {
  ::hyde::rt::Vec<Tup_b> vec120(allocator);
  ::hyde::rt::Vec<Tup_b> vec121(allocator);
  ::hyde::rt::Vec<Tup_b> vec122(allocator);
  ::hyde::rt::Vec<Tup_b> vec156(allocator);
  for (auto [v111] : vec108) {
    if (const auto ins0 = table_27.TryChangeAbsentToPresent({v111}); ins0.changed) {
      if (const auto ins1 = table_33.TryChangeAbsentOrUnknownToPresent({v111}); ins1.changed) {
        if (const auto ins2 = table_81.TryChangeAbsentOrUnknownToPresent({true, v111}); ins2.changed) {
          if (const auto ins3 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins3.changed) {
            for (uint32_t s113 = idx_112.First({true}); s113 != ::hyde::rt::kNoRow; s113 = idx_112.Next(s113)) {
              const auto r113 = table_102.RowAt(s113);
              const auto v114 = r113.x;
              const auto v115 = r113.c1;
              if (true == v115) {
                if (find_116(v114, v115)) {
                  if (table_65.TryChangePresentToUnknown({v115, v114})) {
                    if (table_78.TryChangePresentToUnknown({v114})) {
                      mixed_95.TryChangePresentToUnknown({v114});
                    }
                  }
                }
              }
            }
            vec120.Add({true});
            vec121.Add({true});
            vec122.Add({true});
          }
        }
      }
    }
  }
  for (auto [v124] : vec109) {
    if (table_27.TryChangePresentToAbsent({v124})) {
      if (table_33.TryChangePresentToUnknown({v124})) {
        if (table_81.TryChangePresentToUnknown({true, v124})) {
          if (__9.TryChangePresentToUnknown({true})) {
            for (uint32_t s125 = idx_112.First({true}); s125 != ::hyde::rt::kNoRow; s125 = idx_112.Next(s125)) {
              const auto r125 = table_102.RowAt(s125);
              const auto v126 = r125.x;
              const auto v127 = r125.c1;
              if (true == v127) {
                if (find_116(v126, v127)) {
                  if (!find_129(v127)) {
                    if (const auto ins4 = table_65.TryChangeAbsentOrUnknownToPresent({v127, v126}); ins4.changed) {
                      if (ins4.added_row) {
                        idx_259.Add({v126}, ins4.id);
                      }
                      if (const auto ins5 = table_78.TryChangeAbsentOrUnknownToPresent({v126}); ins5.changed) {
                        mixed_95.TryChangeAbsentOrUnknownToPresent({v126});
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s133 = idx_132.First({true}); s133 != ::hyde::rt::kNoRow; s133 = idx_132.Next(s133)) {
              const auto r133 = table_5.RowAt(s133);
              const auto v134 = r133.x;
              const auto v135 = r133.c1;
              if (true == v135) {
                if (table_39.TryChangePresentToUnknown({true, v134})) {
                  if (table_43.TryChangePresentToUnknown({v134})) {
                    if (table_98.TryChangePresentToUnknown({v134, true})) {
                      if (table_46.TryChangePresentToUnknown({true, v134})) {
                        if (table_69.TryChangePresentToUnknown({v134})) {
                          pos_neg_89.TryChangePresentToUnknown({v134});
                        }
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s137 = idx_136.First({true}); s137 != ::hyde::rt::kNoRow; s137 = idx_136.Next(s137)) {
              const auto r137 = table_12.RowAt(s137);
              const auto v138 = r137.x;
              const auto v139 = r137.c1;
              if (true == v139) {
                if (table_50.TryChangePresentToUnknown({true, v138})) {
                  if (table_54.TryChangePresentToUnknown({v138})) {
                    if (table_16.TryChangePresentToUnknown({v138, true})) {
                      if (table_57.TryChangePresentToUnknown({true, v138})) {
                        if (table_72.TryChangePresentToUnknown({v138})) {
                          both_pos_92.TryChangePresentToUnknown({v138});
                        }
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s141 = idx_140.First({true}); s141 != ::hyde::rt::kNoRow; s141 = idx_140.Next(s141)) {
              const auto r141 = table_23.RowAt(s141);
              const auto v142 = r141.x;
              const auto v143 = r141.c1;
              if (true == v143) {
                if (table_61.TryChangePresentToUnknown({true, v142})) {
                  if (table_75.TryChangePresentToUnknown({v142})) {
                    mixed_95.TryChangePresentToUnknown({v142});
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  for (auto [v147] : vec144) {
    if (const auto ins6 = table_30.TryChangeAbsentToPresent({v147}); ins6.changed) {
      if (const auto ins7 = table_36.TryChangeAbsentOrUnknownToPresent({v147}); ins7.changed) {
        if (const auto ins8 = table_85.TryChangeAbsentOrUnknownToPresent({true, v147}); ins8.changed) {
          if (const auto ins9 = __20.TryChangeAbsentOrUnknownToPresent({true}); ins9.changed) {
            for (uint32_t s149 = idx_148.First({true}); s149 != ::hyde::rt::kNoRow; s149 = idx_148.Next(s149)) {
              const auto r149 = table_98.RowAt(s149);
              const auto v150 = r149.x;
              const auto v151 = r149.c1;
              if (true == v151) {
                if (find_152(v150, v151)) {
                  if (table_46.TryChangePresentToUnknown({v151, v150})) {
                    if (table_69.TryChangePresentToUnknown({v150})) {
                      pos_neg_89.TryChangePresentToUnknown({v150});
                    }
                  }
                }
              }
            }
            vec156.Add({true});
          }
        }
      }
    }
  }
  for (auto [v158] : vec145) {
    if (table_30.TryChangePresentToAbsent({v158})) {
      if (table_36.TryChangePresentToUnknown({v158})) {
        if (table_85.TryChangePresentToUnknown({true, v158})) {
          if (__20.TryChangePresentToUnknown({true})) {
            for (uint32_t s159 = idx_148.First({true}); s159 != ::hyde::rt::kNoRow; s159 = idx_148.Next(s159)) {
              const auto r159 = table_98.RowAt(s159);
              const auto v160 = r159.x;
              const auto v161 = r159.c1;
              if (true == v161) {
                if (find_152(v160, v161)) {
                  if (!find_163(v161)) {
                    if (const auto ins10 = table_46.TryChangeAbsentOrUnknownToPresent({v161, v160}); ins10.changed) {
                      if (ins10.added_row) {
                        idx_542.Add({v160}, ins10.id);
                      }
                      if (const auto ins11 = table_69.TryChangeAbsentOrUnknownToPresent({v160}); ins11.changed) {
                        pos_neg_89.TryChangeAbsentOrUnknownToPresent({v160});
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s167 = idx_166.First({true}); s167 != ::hyde::rt::kNoRow; s167 = idx_166.Next(s167)) {
              const auto r167 = table_16.RowAt(s167);
              const auto v168 = r167.x;
              const auto v169 = r167.c1;
              if (true == v169) {
                if (table_57.TryChangePresentToUnknown({true, v168})) {
                  if (table_72.TryChangePresentToUnknown({v168})) {
                    both_pos_92.TryChangePresentToUnknown({v168});
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  for (auto [v172] : vec170) {
    if (const auto ins12 = table_5.TryChangeAbsentToPresent({v172, true}); ins12.changed) {
      if (ins12.added_row) {
        idx_132.Add({true}, ins12.id);
      }
      vec120.Add({true});
    }
    if (const auto ins13 = table_12.TryChangeAbsentToPresent({v172, true}); ins13.changed) {
      if (ins13.added_row) {
        idx_136.Add({true}, ins13.id);
      }
      vec121.Add({true});
    }
    if (const auto ins14 = table_23.TryChangeAbsentToPresent({v172, true}); ins14.changed) {
      if (ins14.added_row) {
        idx_140.Add({true}, ins14.id);
      }
      vec122.Add({true});
    }
  }
  for (auto [v175] : vec173) {
    if (const auto ins15 = table_102.TryChangeAbsentToPresent({v175, true}); ins15.changed) {
      if (ins15.added_row) {
        idx_112.Add({true}, ins15.id);
      }
      if (!find_129(true)) {
        if (const auto ins16 = table_65.TryChangeAbsentOrUnknownToPresent({true, v175}); ins16.changed) {
          if (ins16.added_row) {
            idx_259.Add({v175}, ins16.id);
          }
          if (const auto ins17 = table_78.TryChangeAbsentOrUnknownToPresent({v175}); ins17.changed) {
            mixed_95.TryChangeAbsentOrUnknownToPresent({v175});
          }
        }
      }
    }
  }
  vec108.Clear();
  vec109.Clear();
  vec144.Clear();
  vec145.Clear();
  vec170.Clear();
  vec173.Clear();
  flow_620(std::move(vec120), std::move(vec121), std::move(vec122), std::move(vec156));
  return false;
}

bool Database::find_116(int32_t v117, bool v118) {
  switch (table_102.State({v117, v118})) {
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

bool Database::find_129(bool v130) {
  switch (__9.State({v130})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v130})) {
        if (find_311(v130)) {
          if (const auto ins18 = __9.TryChangeAbsentToPresent({v130}); ins18.changed) {
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
  if (find_129(v130)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_152(int32_t v153, bool v154) {
  switch (table_98.State({v153, v154})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_98.TryChangeUnknownToAbsent({v153, v154})) {
        if (find_560(v153, v154)) {
          if (const auto ins19 = table_98.TryChangeAbsentToPresent({v153, v154}); ins19.changed) {
            if (ins19.added_row) {
              idx_148.Add({v154}, ins19.id);
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
  if (find_152(v153, v154)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_163(bool v164) {
  switch (__20.State({v164})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__20.TryChangeUnknownToAbsent({v164})) {
        if (find_410(v164)) {
          if (const auto ins20 = __20.TryChangeAbsentToPresent({v164}); ins20.changed) {
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

bool Database::set_c1_1(::hyde::rt::Vec<Tup_i32> vec199, ::hyde::rt::Vec<Tup_i32> vec200) {
  ::hyde::rt::Vec<Tup_i32> vec202(allocator);
  ::hyde::rt::Vec<Tup_i32> vec203(allocator);
  ::hyde::rt::Vec<Tup_i32> vec204(allocator);
  proc_106(std::move(vec199), std::move(vec200), std::move(vec202), std::move(vec202), std::move(vec203), std::move(vec204));
  return true;
}

bool Database::set_c2_1(::hyde::rt::Vec<Tup_i32> vec206, ::hyde::rt::Vec<Tup_i32> vec207) {
  ::hyde::rt::Vec<Tup_i32> vec209(allocator);
  ::hyde::rt::Vec<Tup_i32> vec210(allocator);
  ::hyde::rt::Vec<Tup_i32> vec211(allocator);
  proc_106(std::move(vec209), std::move(vec209), std::move(vec206), std::move(vec207), std::move(vec210), std::move(vec211));
  return true;
}

bool Database::add_r_1(::hyde::rt::Vec<Tup_i32> vec213) {
  ::hyde::rt::Vec<Tup_i32> vec215(allocator);
  ::hyde::rt::Vec<Tup_i32> vec216(allocator);
  ::hyde::rt::Vec<Tup_i32> vec217(allocator);
  proc_106(std::move(vec215), std::move(vec215), std::move(vec216), std::move(vec216), std::move(vec213), std::move(vec217));
  return true;
}

bool Database::add_s_1(::hyde::rt::Vec<Tup_i32> vec219) {
  ::hyde::rt::Vec<Tup_i32> vec221(allocator);
  ::hyde::rt::Vec<Tup_i32> vec222(allocator);
  ::hyde::rt::Vec<Tup_i32> vec223(allocator);
  proc_106(std::move(vec221), std::move(vec221), std::move(vec222), std::move(vec222), std::move(vec223), std::move(vec219));
  return true;
}

bool Database::find_229(int32_t v230) {
  switch (pos_neg_89.State({v230})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (pos_neg_89.TryChangeUnknownToAbsent({v230})) {
        if (find_521(v230)) {
          if (const auto ins21 = pos_neg_89.TryChangeAbsentToPresent({v230}); ins21.changed) {
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

bool Database::find_231(int32_t v232) {
  switch (both_pos_92.State({v232})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (both_pos_92.TryChangeUnknownToAbsent({v232})) {
        if (find_369(v232)) {
          if (const auto ins22 = both_pos_92.TryChangeAbsentToPresent({v232}); ins22.changed) {
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
  if (find_231(v232)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_233(int32_t v234) {
  switch (mixed_95.State({v234})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (mixed_95.TryChangeUnknownToAbsent({v234})) {
        if (find_235(v234)) {
          if (const auto ins23 = mixed_95.TryChangeAbsentToPresent({v234}); ins23.changed) {
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
  if (find_233(v234)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_235(int32_t v236) {
  if (find_239(v236)) {
    return true;
  }
  if (find_242(v236)) {
    return true;
  }
  return false;
}

bool Database::find_239(int32_t v240) {
  switch (table_75.State({v240})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_75.TryChangeUnknownToAbsent({v240})) {
        if (find_277(v240)) {
          if (const auto ins24 = table_75.TryChangeAbsentToPresent({v240}); ins24.changed) {
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
  if (find_239(v240)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_242(int32_t v243) {
  switch (table_78.State({v243})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_78.TryChangeUnknownToAbsent({v243})) {
        if (find_245(v243)) {
          if (const auto ins25 = table_78.TryChangeAbsentToPresent({v243}); ins25.changed) {
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
  if (find_242(v243)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_245(int32_t v246) {
  if (find_253(v246)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_253(int32_t v254) {
  if (find_256(v254)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_256(int32_t v257) {
  for (uint32_t s260 = idx_259.First({v257}); s260 != ::hyde::rt::kNoRow; s260 = idx_259.Next(s260)) {
    const auto r260 = table_65.RowAt(s260);
    const auto v261 = r260.c0;
    const auto v262 = r260.x;
    if (v262 == v257) {
      if (find_263(v261, v262)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_263(bool v264, int32_t v265) {
  switch (table_65.State({v264, v265})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_65.TryChangeUnknownToAbsent({v264, v265})) {
        if (find_267(v264, v265)) {
          if (const auto ins26 = table_65.TryChangeAbsentToPresent({v264, v265}); ins26.changed) {
            if (ins26.added_row) {
              idx_259.Add({v265}, ins26.id);
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
  if (find_263(v264, v265)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_267(bool v268, int32_t v269) {
  if (find_116(v269, v268)) {
    if (find_129(v268)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_277(int32_t v278) {
  if (find_285(v278)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_285(int32_t v286) {
  if (find_288(v286)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_288(int32_t v289) {
  for (uint32_t s292 = idx_291.First({v289}); s292 != ::hyde::rt::kNoRow; s292 = idx_291.Next(s292)) {
    const auto r292 = table_61.RowAt(s292);
    const auto v293 = r292.c0;
    const auto v294 = r292.x;
    if (v294 == v289) {
      if (find_295(v293, v294)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_295(bool v296, int32_t v297) {
  switch (table_61.State({v296, v297})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_61.TryChangeUnknownToAbsent({v296, v297})) {
        if (find_299(v296, v297)) {
          if (const auto ins27 = table_61.TryChangeAbsentToPresent({v296, v297}); ins27.changed) {
            if (ins27.added_row) {
              idx_291.Add({v297}, ins27.id);
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
  if (find_295(v296, v297)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_299(bool v300, int32_t v301) {
  if (find_304(v301, v300)) {
    if (find_129(v300)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_304(int32_t v305, bool v306) {
  switch (table_23.State({v305, v306})) {
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

bool Database::find_311(bool v312) {
  if (find_319(v312)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_319(bool v320) {
  for (uint32_t s322 = 0; s322 < table_81.NumRows(); ++s322) {
    const auto r322 = table_81.RowAt(s322);
    const auto v323 = r322.c0;
    const auto v324 = r322.c1;
    if (v323 == v320) {
      if (find_325(v323, v324)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_325(bool v326, int32_t v327) {
  switch (table_81.State({v326, v327})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_81.TryChangeUnknownToAbsent({v326, v327})) {
        if (find_329(v326, v327)) {
          if (const auto ins28 = table_81.TryChangeAbsentToPresent({v326, v327}); ins28.changed) {
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
  if (find_325(v326, v327)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_329(bool v330, int32_t v331) {
  if (find_334(v330, v331)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_334(bool v335, int32_t v336) {
  if (find_338(v336)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_338(int32_t v339) {
  switch (table_33.State({v339})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_33.TryChangeUnknownToAbsent({v339})) {
        if (find_341(v339)) {
          if (const auto ins29 = table_33.TryChangeAbsentToPresent({v339}); ins29.changed) {
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
  if (find_344(v339)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_341(int32_t v342) {
  if (find_347(v342)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_344(int32_t v345) {
  switch (table_33.State({v345})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_33.TryChangeUnknownToAbsent({v345})) {
        if (find_347(v345)) {
          if (const auto ins30 = table_33.TryChangeAbsentToPresent({v345}); ins30.changed) {
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
  if (find_344(v345)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_347(int32_t v348) {
  if (find_351(v348)) {
    return true;
  }
  return false;
}

bool Database::find_351(int32_t v352) {
  switch (table_27.State({v352})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_27.TryChangeUnknownToAbsent({v352})) {
        if (find_354(v352)) {
          if (const auto ins31 = table_27.TryChangeAbsentToPresent({v352}); ins31.changed) {
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
  if (find_351(v352)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_354(int32_t v355) {
  if (find_362(v355)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_362(int32_t v363) {
  if (find_365(v363)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_365(int32_t v366) {
  return false;
}

bool Database::find_369(int32_t v370) {
  if (find_373(v370)) {
    return true;
  }
  return false;
}

bool Database::find_373(int32_t v374) {
  switch (table_72.State({v374})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_72.TryChangeUnknownToAbsent({v374})) {
        if (find_376(v374)) {
          if (const auto ins32 = table_72.TryChangeAbsentToPresent({v374}); ins32.changed) {
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
  if (find_373(v374)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_376(int32_t v377) {
  if (find_384(v377)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_384(int32_t v385) {
  if (find_387(v385)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_387(int32_t v388) {
  for (uint32_t s391 = idx_390.First({v388}); s391 != ::hyde::rt::kNoRow; s391 = idx_390.Next(s391)) {
    const auto r391 = table_57.RowAt(s391);
    const auto v392 = r391.c0;
    const auto v393 = r391.x;
    if (v393 == v388) {
      if (find_394(v392, v393)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_394(bool v395, int32_t v396) {
  switch (table_57.State({v395, v396})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_57.TryChangeUnknownToAbsent({v395, v396})) {
        if (find_398(v395, v396)) {
          if (const auto ins33 = table_57.TryChangeAbsentToPresent({v395, v396}); ins33.changed) {
            if (ins33.added_row) {
              idx_390.Add({v396}, ins33.id);
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
  if (find_394(v395, v396)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_398(bool v399, int32_t v400) {
  if (find_403(v400, v399)) {
    if (find_163(v399)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_403(int32_t v404, bool v405) {
  switch (table_16.State({v404, v405})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_16.TryChangeUnknownToAbsent({v404, v405})) {
        if (find_468(v404, v405)) {
          if (const auto ins34 = table_16.TryChangeAbsentToPresent({v404, v405}); ins34.changed) {
            if (ins34.added_row) {
              idx_166.Add({v405}, ins34.id);
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
  if (find_403(v404, v405)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_410(bool v411) {
  if (find_418(v411)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_418(bool v419) {
  for (uint32_t s421 = 0; s421 < table_85.NumRows(); ++s421) {
    const auto r421 = table_85.RowAt(s421);
    const auto v422 = r421.c0;
    const auto v423 = r421.c1;
    if (v422 == v419) {
      if (find_424(v422, v423)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_424(bool v425, int32_t v426) {
  switch (table_85.State({v425, v426})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_85.TryChangeUnknownToAbsent({v425, v426})) {
        if (find_428(v425, v426)) {
          if (const auto ins35 = table_85.TryChangeAbsentToPresent({v425, v426}); ins35.changed) {
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
  if (find_424(v425, v426)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_428(bool v429, int32_t v430) {
  if (find_433(v429, v430)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_433(bool v434, int32_t v435) {
  if (find_437(v435)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_437(int32_t v438) {
  switch (table_36.State({v438})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_36.TryChangeUnknownToAbsent({v438})) {
        if (find_440(v438)) {
          if (const auto ins36 = table_36.TryChangeAbsentToPresent({v438}); ins36.changed) {
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
  if (find_443(v438)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_440(int32_t v441) {
  if (find_446(v441)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_443(int32_t v444) {
  switch (table_36.State({v444})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_36.TryChangeUnknownToAbsent({v444})) {
        if (find_446(v444)) {
          if (const auto ins37 = table_36.TryChangeAbsentToPresent({v444}); ins37.changed) {
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
  if (find_443(v444)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_446(int32_t v447) {
  if (find_450(v447)) {
    return true;
  }
  return false;
}

bool Database::find_450(int32_t v451) {
  switch (table_30.State({v451})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_30.TryChangeUnknownToAbsent({v451})) {
        if (find_354(v451)) {
          if (const auto ins38 = table_30.TryChangeAbsentToPresent({v451}); ins38.changed) {
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
  if (find_450(v451)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_468(int32_t v469, bool v470) {
  if (find_478(v469)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_478(int32_t v479) {
  switch (table_54.State({v479})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_54.TryChangeUnknownToAbsent({v479})) {
        if (find_481(v479)) {
          if (const auto ins39 = table_54.TryChangeAbsentToPresent({v479}); ins39.changed) {
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
  if (find_478(v479)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_481(int32_t v482) {
  if (find_489(v482)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_489(int32_t v490) {
  for (uint32_t s493 = idx_492.First({v490}); s493 != ::hyde::rt::kNoRow; s493 = idx_492.Next(s493)) {
    const auto r493 = table_50.RowAt(s493);
    const auto v494 = r493.c0;
    const auto v495 = r493.x;
    if (v495 == v490) {
      if (find_496(v494, v495)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_496(bool v497, int32_t v498) {
  switch (table_50.State({v497, v498})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_50.TryChangeUnknownToAbsent({v497, v498})) {
        if (find_500(v497, v498)) {
          if (const auto ins40 = table_50.TryChangeAbsentToPresent({v497, v498}); ins40.changed) {
            if (ins40.added_row) {
              idx_492.Add({v498}, ins40.id);
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
  if (find_496(v497, v498)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_500(bool v501, int32_t v502) {
  if (find_505(v502, v501)) {
    if (find_129(v501)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_505(int32_t v506, bool v507) {
  switch (table_12.State({v506, v507})) {
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

bool Database::find_521(int32_t v522) {
  if (find_525(v522)) {
    return true;
  }
  return false;
}

bool Database::find_525(int32_t v526) {
  switch (table_69.State({v526})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_69.TryChangeUnknownToAbsent({v526})) {
        if (find_528(v526)) {
          if (const auto ins41 = table_69.TryChangeAbsentToPresent({v526}); ins41.changed) {
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
  if (find_525(v526)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_528(int32_t v529) {
  if (find_536(v529)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_536(int32_t v537) {
  if (find_539(v537)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_539(int32_t v540) {
  for (uint32_t s543 = idx_542.First({v540}); s543 != ::hyde::rt::kNoRow; s543 = idx_542.Next(s543)) {
    const auto r543 = table_46.RowAt(s543);
    const auto v544 = r543.c0;
    const auto v545 = r543.x;
    if (v545 == v540) {
      if (find_546(v544, v545)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_546(bool v547, int32_t v548) {
  switch (table_46.State({v547, v548})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_46.TryChangeUnknownToAbsent({v547, v548})) {
        if (find_550(v547, v548)) {
          if (const auto ins42 = table_46.TryChangeAbsentToPresent({v547, v548}); ins42.changed) {
            if (ins42.added_row) {
              idx_542.Add({v548}, ins42.id);
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
  if (find_546(v547, v548)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_550(bool v551, int32_t v552) {
  if (find_152(v552, v551)) {
    if (find_163(v551)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_560(int32_t v561, bool v562) {
  if (find_565(v561)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_565(int32_t v566) {
  switch (table_43.State({v566})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_43.TryChangeUnknownToAbsent({v566})) {
        if (find_568(v566)) {
          if (const auto ins43 = table_43.TryChangeAbsentToPresent({v566}); ins43.changed) {
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
  if (find_565(v566)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_568(int32_t v569) {
  if (find_576(v569)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_576(int32_t v577) {
  for (uint32_t s580 = idx_579.First({v577}); s580 != ::hyde::rt::kNoRow; s580 = idx_579.Next(s580)) {
    const auto r580 = table_39.RowAt(s580);
    const auto v581 = r580.c0;
    const auto v582 = r580.x;
    if (v582 == v577) {
      if (find_583(v581, v582)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_583(bool v584, int32_t v585) {
  switch (table_39.State({v584, v585})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_39.TryChangeUnknownToAbsent({v584, v585})) {
        if (find_587(v584, v585)) {
          if (const auto ins44 = table_39.TryChangeAbsentToPresent({v584, v585}); ins44.changed) {
            if (ins44.added_row) {
              idx_579.Add({v585}, ins44.id);
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
  if (find_583(v584, v585)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_587(bool v588, int32_t v589) {
  if (find_592(v589, v588)) {
    if (find_129(v588)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_592(int32_t v593, bool v594) {
  switch (table_5.State({v593, v594})) {
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

bool Database::flow_620(::hyde::rt::Vec<Tup_b> vec120, ::hyde::rt::Vec<Tup_b> vec121, ::hyde::rt::Vec<Tup_b> vec122, ::hyde::rt::Vec<Tup_b> vec156) {
  g107 += 1;
  vec122.SortAndUnique();
  for (auto [v178] : vec122) {
    if (const uint32_t j177_0 = __9.Find({v178}); j177_0 != ::hyde::rt::kNoRow) {
      const auto r177_0 = __9.RowAt(j177_0);
      const auto v180 = r177_0.c0;
      for (uint32_t j177_1 = idx_140.First({v178}); j177_1 != ::hyde::rt::kNoRow; j177_1 = idx_140.Next(j177_1)) {
        const auto r177_1 = table_23.RowAt(j177_1);
        const auto v181 = r177_1.x;
        const auto v179 = r177_1.c1;
        if (v178 == v179 && v178 == v180) {
          switch (__9.State({v179})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins45 = table_61.TryChangeAbsentOrUnknownToPresent({v179, v181}); ins45.changed) {
                if (ins45.added_row) {
                  idx_291.Add({v181}, ins45.id);
                }
                if (const auto ins46 = table_75.TryChangeAbsentOrUnknownToPresent({v181}); ins46.changed) {
                  mixed_95.TryChangeAbsentOrUnknownToPresent({v181});
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
  vec122.Clear();
  vec121.SortAndUnique();
  for (auto [v183] : vec121) {
    if (const uint32_t j182_0 = __9.Find({v183}); j182_0 != ::hyde::rt::kNoRow) {
      const auto r182_0 = __9.RowAt(j182_0);
      const auto v185 = r182_0.c0;
      for (uint32_t j182_1 = idx_136.First({v183}); j182_1 != ::hyde::rt::kNoRow; j182_1 = idx_136.Next(j182_1)) {
        const auto r182_1 = table_12.RowAt(j182_1);
        const auto v186 = r182_1.x;
        const auto v184 = r182_1.c1;
        if (v183 == v184 && v183 == v185) {
          switch (__9.State({v184})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins47 = table_50.TryChangeAbsentOrUnknownToPresent({v184, v186}); ins47.changed) {
                if (ins47.added_row) {
                  idx_492.Add({v186}, ins47.id);
                }
                if (const auto ins48 = table_54.TryChangeAbsentOrUnknownToPresent({v186}); ins48.changed) {
                  if (const auto ins49 = table_16.TryChangeAbsentOrUnknownToPresent({v186, true}); ins49.changed) {
                    if (ins49.added_row) {
                      idx_166.Add({true}, ins49.id);
                    }
                    vec156.Add({true});
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
  vec121.Clear();
  vec120.SortAndUnique();
  for (auto [v188] : vec120) {
    if (const uint32_t j187_0 = __9.Find({v188}); j187_0 != ::hyde::rt::kNoRow) {
      const auto r187_0 = __9.RowAt(j187_0);
      const auto v190 = r187_0.c0;
      for (uint32_t j187_1 = idx_132.First({v188}); j187_1 != ::hyde::rt::kNoRow; j187_1 = idx_132.Next(j187_1)) {
        const auto r187_1 = table_5.RowAt(j187_1);
        const auto v191 = r187_1.x;
        const auto v189 = r187_1.c1;
        if (v188 == v189 && v188 == v190) {
          switch (__9.State({v189})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins50 = table_39.TryChangeAbsentOrUnknownToPresent({v189, v191}); ins50.changed) {
                if (ins50.added_row) {
                  idx_579.Add({v191}, ins50.id);
                }
                if (const auto ins51 = table_43.TryChangeAbsentOrUnknownToPresent({v191}); ins51.changed) {
                  if (const auto ins52 = table_98.TryChangeAbsentOrUnknownToPresent({v191, true}); ins52.changed) {
                    if (ins52.added_row) {
                      idx_148.Add({true}, ins52.id);
                    }
                    if (!find_163(true)) {
                      if (const auto ins53 = table_46.TryChangeAbsentOrUnknownToPresent({true, v191}); ins53.changed) {
                        if (ins53.added_row) {
                          idx_542.Add({v191}, ins53.id);
                        }
                        if (const auto ins54 = table_69.TryChangeAbsentOrUnknownToPresent({v191}); ins54.changed) {
                          pos_neg_89.TryChangeAbsentOrUnknownToPresent({v191});
                        }
                      }
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
  vec120.Clear();
  vec156.SortAndUnique();
  for (auto [v194] : vec156) {
    if (const uint32_t j193_0 = __20.Find({v194}); j193_0 != ::hyde::rt::kNoRow) {
      const auto r193_0 = __20.RowAt(j193_0);
      const auto v196 = r193_0.c0;
      for (uint32_t j193_1 = idx_166.First({v194}); j193_1 != ::hyde::rt::kNoRow; j193_1 = idx_166.Next(j193_1)) {
        const auto r193_1 = table_16.RowAt(j193_1);
        const auto v197 = r193_1.x;
        const auto v195 = r193_1.c1;
        if (v194 == v195 && v194 == v196) {
          switch (table_16.State({v197, v195})) {
            case ::hyde::rt::TupleState::kPresent: {
              switch (__20.State({v195})) {
                case ::hyde::rt::TupleState::kPresent: {
                  if (const auto ins55 = table_57.TryChangeAbsentOrUnknownToPresent({v195, v197}); ins55.changed) {
                    if (ins55.added_row) {
                      idx_390.Add({v197}, ins55.id);
                    }
                    if (const auto ins56 = table_72.TryChangeAbsentOrUnknownToPresent({v197}); ins56.changed) {
                      both_pos_92.TryChangeAbsentOrUnknownToPresent({v197});
                    }
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
  vec156.Clear();
  return true;
}

Database::pos_neg_f_cursor Database::pos_neg_f() {
  return {*this, 0};
}

bool Database::pos_neg_f_cursor::next(int32_t &X) {
  while (pos < db.pos_neg_89.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.pos_neg_89.RowAt(id);
    if (!db.find_229(row.x)) {
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
  while (pos < db.both_pos_92.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.both_pos_92.RowAt(id);
    if (!db.find_231(row.x)) {
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
  while (pos < db.mixed_95.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.mixed_95.RowAt(id);
    if (!db.find_233(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

