// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_76(allocator_),
      __9(allocator_),
      table_12(allocator_),
      table_15(allocator_),
      table_18(allocator_),
      table_21(allocator_),
      table_24(allocator_),
      idx_208(allocator_),
      table_28(allocator_),
      idx_130(allocator_),
      table_32(allocator_),
      table_35(allocator_),
      table_38(allocator_),
      gated_42(allocator_),
      ungated_45(allocator_),
      table_48(allocator_),
      idx_58(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec103(allocator);
  ::hyde::rt::Vec<Tup_i32> vec104(allocator);
  proc_52(std::move(vec103), std::move(vec103), std::move(vec104), std::move(vec104));
  return false;
}

bool Database::proc_52(::hyde::rt::Vec<Tup_i32> vec54, ::hyde::rt::Vec<Tup_i32> vec55, ::hyde::rt::Vec<Tup_i32> vec80, ::hyde::rt::Vec<Tup_i32> vec81) {
  ::hyde::rt::Vec<Tup_b> vec66(allocator);
  for (auto [v57] : vec54) {
    if (const auto ins0 = table_12.TryChangeAbsentToPresent({v57}); ins0.changed) {
      if (const auto ins1 = table_18.TryChangeAbsentOrUnknownToPresent({v57}); ins1.changed) {
        if (const auto ins2 = table_38.TryChangeAbsentOrUnknownToPresent({true, v57}); ins2.changed) {
          if (const auto ins3 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins3.changed) {
            for (uint32_t s59 = idx_58.First({true}); s59 != ::hyde::rt::kNoRow; s59 = idx_58.Next(s59)) {
              const auto r59 = table_48.RowAt(s59);
              const auto v60 = r59.x;
              const auto v61 = r59.c1;
              if (true == v61) {
                if (find_62(v60, v61)) {
                  if (table_28.TryChangePresentToUnknown({v61, v60})) {
                    if (table_35.TryChangePresentToUnknown({v60})) {
                      ungated_45.TryChangePresentToUnknown({v60});
                    }
                  }
                }
              }
            }
            vec66.Add({true});
          }
        }
      }
    }
  }
  for (auto [v68] : vec55) {
    if (table_12.TryChangePresentToAbsent({v68})) {
      if (table_18.TryChangePresentToUnknown({v68})) {
        if (table_38.TryChangePresentToUnknown({true, v68})) {
          if (__9.TryChangePresentToUnknown({true})) {
            for (uint32_t s69 = idx_58.First({true}); s69 != ::hyde::rt::kNoRow; s69 = idx_58.Next(s69)) {
              const auto r69 = table_48.RowAt(s69);
              const auto v70 = r69.x;
              const auto v71 = r69.c1;
              if (true == v71) {
                if (find_62(v70, v71)) {
                  if (!find_73(v71)) {
                    if (const auto ins4 = table_28.TryChangeAbsentOrUnknownToPresent({v71, v70}); ins4.changed) {
                      if (ins4.added_row) {
                        idx_130.Add({v70}, ins4.id);
                      }
                      if (const auto ins5 = table_35.TryChangeAbsentOrUnknownToPresent({v70}); ins5.changed) {
                        ungated_45.TryChangeAbsentOrUnknownToPresent({v70});
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s77 = idx_76.First({true}); s77 != ::hyde::rt::kNoRow; s77 = idx_76.Next(s77)) {
              const auto r77 = table_5.RowAt(s77);
              const auto v78 = r77.x;
              const auto v79 = r77.c1;
              if (true == v79) {
                if (table_24.TryChangePresentToUnknown({true, v78})) {
                  if (table_32.TryChangePresentToUnknown({v78})) {
                    gated_42.TryChangePresentToUnknown({v78});
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  for (auto [v83] : vec80) {
    if (const auto ins6 = table_15.TryChangeAbsentToPresent({v83}); ins6.changed) {
      if (const auto ins7 = table_21.TryChangeAbsentOrUnknownToPresent({v83}); ins7.changed) {
        if (const auto ins8 = table_5.TryChangeAbsentOrUnknownToPresent({v83, true}); ins8.changed) {
          if (ins8.added_row) {
            idx_76.Add({true}, ins8.id);
          }
          vec66.Add({true});
        }
        if (const auto ins9 = table_48.TryChangeAbsentOrUnknownToPresent({v83, true}); ins9.changed) {
          if (ins9.added_row) {
            idx_58.Add({true}, ins9.id);
          }
          if (!find_73(true)) {
            if (const auto ins10 = table_28.TryChangeAbsentOrUnknownToPresent({true, v83}); ins10.changed) {
              if (ins10.added_row) {
                idx_130.Add({v83}, ins10.id);
              }
              if (const auto ins11 = table_35.TryChangeAbsentOrUnknownToPresent({v83}); ins11.changed) {
                ungated_45.TryChangeAbsentOrUnknownToPresent({v83});
              }
            }
          }
        }
      }
    }
  }
  for (auto [v86] : vec81) {
    if (table_15.TryChangePresentToAbsent({v86})) {
      if (table_21.TryChangePresentToUnknown({v86})) {
        if (table_5.TryChangePresentToUnknown({v86, true})) {
          if (table_24.TryChangePresentToUnknown({true, v86})) {
            if (table_32.TryChangePresentToUnknown({v86})) {
              gated_42.TryChangePresentToUnknown({v86});
            }
          }
        }
        if (table_48.TryChangePresentToUnknown({v86, true})) {
          if (table_28.TryChangePresentToUnknown({true, v86})) {
            if (table_35.TryChangePresentToUnknown({v86})) {
              ungated_45.TryChangePresentToUnknown({v86});
            }
          }
        }
      }
    }
  }
  vec54.Clear();
  vec55.Clear();
  vec80.Clear();
  vec81.Clear();
  flow_311(std::move(vec66));
  return false;
}

bool Database::find_62(int32_t v63, bool v64) {
  switch (table_48.State({v63, v64})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_48.TryChangeUnknownToAbsent({v63, v64})) {
        if (find_148(v63, v64)) {
          if (const auto ins12 = table_48.TryChangeAbsentToPresent({v63, v64}); ins12.changed) {
            if (ins12.added_row) {
              idx_58.Add({v64}, ins12.id);
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
  if (find_62(v63, v64)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_73(bool v74) {
  switch (__9.State({v74})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v74})) {
        if (find_228(v74)) {
          if (const auto ins13 = __9.TryChangeAbsentToPresent({v74}); ins13.changed) {
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

bool Database::support_1(::hyde::rt::Vec<Tup_i32> vec93, ::hyde::rt::Vec<Tup_i32> vec94) {
  ::hyde::rt::Vec<Tup_i32> vec96(allocator);
  proc_52(std::move(vec93), std::move(vec94), std::move(vec96), std::move(vec96));
  return true;
}

bool Database::item_1(::hyde::rt::Vec<Tup_i32> vec98, ::hyde::rt::Vec<Tup_i32> vec99) {
  ::hyde::rt::Vec<Tup_i32> vec101(allocator);
  proc_52(std::move(vec101), std::move(vec101), std::move(vec98), std::move(vec99));
  return true;
}

bool Database::find_105(int32_t v106) {
  switch (gated_42.State({v106})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (gated_42.TryChangeUnknownToAbsent({v106})) {
        if (find_187(v106)) {
          if (const auto ins14 = gated_42.TryChangeAbsentToPresent({v106}); ins14.changed) {
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

bool Database::find_107(int32_t v108) {
  switch (ungated_45.State({v108})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (ungated_45.TryChangeUnknownToAbsent({v108})) {
        if (find_109(v108)) {
          if (const auto ins15 = ungated_45.TryChangeAbsentToPresent({v108}); ins15.changed) {
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
  if (find_107(v108)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_109(int32_t v110) {
  if (find_113(v110)) {
    return true;
  }
  return false;
}

bool Database::find_113(int32_t v114) {
  switch (table_35.State({v114})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_35.TryChangeUnknownToAbsent({v114})) {
        if (find_116(v114)) {
          if (const auto ins16 = table_35.TryChangeAbsentToPresent({v114}); ins16.changed) {
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
  if (find_113(v114)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_116(int32_t v117) {
  if (find_124(v117)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_124(int32_t v125) {
  if (find_127(v125)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_127(int32_t v128) {
  for (uint32_t s131 = idx_130.First({v128}); s131 != ::hyde::rt::kNoRow; s131 = idx_130.Next(s131)) {
    const auto r131 = table_28.RowAt(s131);
    const auto v132 = r131.c0;
    const auto v133 = r131.x;
    if (v133 == v128) {
      if (find_134(v132, v133)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_134(bool v135, int32_t v136) {
  switch (table_28.State({v135, v136})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_28.TryChangeUnknownToAbsent({v135, v136})) {
        if (find_138(v135, v136)) {
          if (const auto ins17 = table_28.TryChangeAbsentToPresent({v135, v136}); ins17.changed) {
            if (ins17.added_row) {
              idx_130.Add({v136}, ins17.id);
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
  if (find_134(v135, v136)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_138(bool v139, int32_t v140) {
  if (find_62(v140, v139)) {
    if (find_73(v139)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_148(int32_t v149, bool v150) {
  if (find_153(v149)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_153(int32_t v154) {
  switch (table_21.State({v154})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_21.TryChangeUnknownToAbsent({v154})) {
        if (find_156(v154)) {
          if (const auto ins18 = table_21.TryChangeAbsentToPresent({v154}); ins18.changed) {
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
  if (find_159(v154)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_156(int32_t v157) {
  if (find_183(v157)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_159(int32_t v160) {
  switch (table_21.State({v160})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_21.TryChangeUnknownToAbsent({v160})) {
        if (find_162(v160)) {
          if (const auto ins19 = table_21.TryChangeAbsentToPresent({v160}); ins19.changed) {
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
  if (find_159(v160)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_162(int32_t v163) {
  if (find_166(v163)) {
    return true;
  }
  return false;
}

bool Database::find_166(int32_t v167) {
  switch (table_15.State({v167})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_15.TryChangeUnknownToAbsent({v167})) {
        if (find_169(v167)) {
          if (const auto ins20 = table_15.TryChangeAbsentToPresent({v167}); ins20.changed) {
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
  if (find_177(v170)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_177(int32_t v178) {
  if (find_180(v178)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_180(int32_t v181) {
  return false;
}

bool Database::find_183(int32_t v184) {
  if (find_162(v184)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_187(int32_t v188) {
  if (find_191(v188)) {
    return true;
  }
  return false;
}

bool Database::find_191(int32_t v192) {
  switch (table_32.State({v192})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_32.TryChangeUnknownToAbsent({v192})) {
        if (find_194(v192)) {
          if (const auto ins21 = table_32.TryChangeAbsentToPresent({v192}); ins21.changed) {
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
  if (find_191(v192)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_194(int32_t v195) {
  if (find_202(v195)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_202(int32_t v203) {
  if (find_205(v203)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_205(int32_t v206) {
  for (uint32_t s209 = idx_208.First({v206}); s209 != ::hyde::rt::kNoRow; s209 = idx_208.Next(s209)) {
    const auto r209 = table_24.RowAt(s209);
    const auto v210 = r209.c0;
    const auto v211 = r209.x;
    if (v211 == v206) {
      if (find_212(v210, v211)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_212(bool v213, int32_t v214) {
  switch (table_24.State({v213, v214})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_24.TryChangeUnknownToAbsent({v213, v214})) {
        if (find_216(v213, v214)) {
          if (const auto ins22 = table_24.TryChangeAbsentToPresent({v213, v214}); ins22.changed) {
            if (ins22.added_row) {
              idx_208.Add({v214}, ins22.id);
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
  if (find_212(v213, v214)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_216(bool v217, int32_t v218) {
  if (find_221(v218, v217)) {
    if (find_73(v217)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_221(int32_t v222, bool v223) {
  switch (table_5.State({v222, v223})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v222, v223})) {
        if (find_148(v222, v223)) {
          if (const auto ins23 = table_5.TryChangeAbsentToPresent({v222, v223}); ins23.changed) {
            if (ins23.added_row) {
              idx_76.Add({v223}, ins23.id);
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
  if (find_221(v222, v223)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_228(bool v229) {
  if (find_236(v229)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_236(bool v237) {
  for (uint32_t s239 = 0; s239 < table_38.NumRows(); ++s239) {
    const auto r239 = table_38.RowAt(s239);
    const auto v240 = r239.c0;
    const auto v241 = r239.c1;
    if (v240 == v237) {
      if (find_242(v240, v241)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_242(bool v243, int32_t v244) {
  switch (table_38.State({v243, v244})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_38.TryChangeUnknownToAbsent({v243, v244})) {
        if (find_246(v243, v244)) {
          if (const auto ins24 = table_38.TryChangeAbsentToPresent({v243, v244}); ins24.changed) {
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
  if (find_242(v243, v244)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_246(bool v247, int32_t v248) {
  if (find_251(v247, v248)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_251(bool v252, int32_t v253) {
  if (find_255(v253)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_255(int32_t v256) {
  switch (table_18.State({v256})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v256})) {
        if (find_258(v256)) {
          if (const auto ins25 = table_18.TryChangeAbsentToPresent({v256}); ins25.changed) {
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
  if (find_261(v256)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_258(int32_t v259) {
  if (find_264(v259)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_261(int32_t v262) {
  switch (table_18.State({v262})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v262})) {
        if (find_264(v262)) {
          if (const auto ins26 = table_18.TryChangeAbsentToPresent({v262}); ins26.changed) {
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
  if (find_261(v262)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_264(int32_t v265) {
  if (find_268(v265)) {
    return true;
  }
  return false;
}

bool Database::find_268(int32_t v269) {
  switch (table_12.State({v269})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_12.TryChangeUnknownToAbsent({v269})) {
        if (find_169(v269)) {
          if (const auto ins27 = table_12.TryChangeAbsentToPresent({v269}); ins27.changed) {
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
  if (find_268(v269)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::flow_311(::hyde::rt::Vec<Tup_b> vec66) {
  g53 += 1;
  vec66.SortAndUnique();
  for (auto [v88] : vec66) {
    if (const uint32_t j87_0 = __9.Find({v88}); j87_0 != ::hyde::rt::kNoRow) {
      const auto r87_0 = __9.RowAt(j87_0);
      const auto v90 = r87_0.c0;
      for (uint32_t j87_1 = idx_76.First({v88}); j87_1 != ::hyde::rt::kNoRow; j87_1 = idx_76.Next(j87_1)) {
        const auto r87_1 = table_5.RowAt(j87_1);
        const auto v91 = r87_1.x;
        const auto v89 = r87_1.c1;
        if (v88 == v89 && v88 == v90) {
          switch (table_5.State({v91, v89})) {
            case ::hyde::rt::TupleState::kPresent: {
              switch (__9.State({v89})) {
                case ::hyde::rt::TupleState::kPresent: {
                  if (const auto ins28 = table_24.TryChangeAbsentOrUnknownToPresent({v89, v91}); ins28.changed) {
                    if (ins28.added_row) {
                      idx_208.Add({v91}, ins28.id);
                    }
                    if (const auto ins29 = table_32.TryChangeAbsentOrUnknownToPresent({v91}); ins29.changed) {
                      gated_42.TryChangeAbsentOrUnknownToPresent({v91});
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
  vec66.Clear();
  return true;
}

Database::gated_f_cursor Database::gated_f() {
  return {*this, 0};
}

bool Database::gated_f_cursor::next(int32_t &X) {
  while (pos < db.gated_42.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.gated_42.RowAt(id);
    if (!db.find_105(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

Database::ungated_f_cursor Database::ungated_f() {
  return {*this, 0};
}

bool Database::ungated_f_cursor::next(int32_t &X) {
  while (pos < db.ungated_45.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.ungated_45.RowAt(id);
    if (!db.find_107(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

