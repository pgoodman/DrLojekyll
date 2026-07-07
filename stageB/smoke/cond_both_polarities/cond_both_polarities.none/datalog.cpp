// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_133(allocator_),
      __9(allocator_),
      table_12(allocator_),
      idx_137(allocator_),
      table_16(allocator_),
      idx_141(allocator_),
      table_20(allocator_),
      table_23(allocator_),
      table_26(allocator_),
      table_29(allocator_),
      table_32(allocator_),
      idx_527(allocator_),
      table_36(allocator_),
      table_39(allocator_),
      idx_490(allocator_),
      table_43(allocator_),
      idx_440(allocator_),
      table_47(allocator_),
      idx_294(allocator_),
      table_51(allocator_),
      idx_262(allocator_),
      table_55(allocator_),
      table_58(allocator_),
      table_61(allocator_),
      table_64(allocator_),
      table_67(allocator_),
      table_71(allocator_),
      pos_neg_75(allocator_),
      both_pos_78(allocator_),
      mixed_81(allocator_),
      table_84(allocator_),
      idx_98(allocator_),
      table_88(allocator_),
      idx_106(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec228(allocator);
  ::hyde::rt::Vec<Tup_i32> vec229(allocator);
  ::hyde::rt::Vec<Tup_i32> vec230(allocator);
  ::hyde::rt::Vec<Tup_i32> vec231(allocator);
  proc_92(std::move(vec228), std::move(vec228), std::move(vec229), std::move(vec229), std::move(vec230), std::move(vec231));
  return false;
}

bool Database::proc_92(::hyde::rt::Vec<Tup_i32> vec94, ::hyde::rt::Vec<Tup_i32> vec95, ::hyde::rt::Vec<Tup_i32> vec145, ::hyde::rt::Vec<Tup_i32> vec146, ::hyde::rt::Vec<Tup_i32> vec178, ::hyde::rt::Vec<Tup_i32> vec181) {
  ::hyde::rt::Vec<Tup_b> vec114(allocator);
  ::hyde::rt::Vec<Tup_b> vec115(allocator);
  ::hyde::rt::Vec<Tup_b> vec116(allocator);
  for (auto [v97] : vec94) {
    if (const auto ins0 = table_20.TryChangeAbsentToPresent({v97}); ins0.changed) {
      if (const auto ins1 = table_26.TryChangeAbsentOrUnknownToPresent({v97}); ins1.changed) {
        if (const auto ins2 = table_67.TryChangeAbsentOrUnknownToPresent({true, v97}); ins2.changed) {
          if (const auto ins3 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins3.changed) {
            for (uint32_t s99 = idx_98.First({true}); s99 != ::hyde::rt::kNoRow; s99 = idx_98.Next(s99)) {
              const auto r99 = table_84.RowAt(s99);
              const auto v100 = r99.x;
              const auto v101 = r99.c1;
              if (true == v101) {
                if (find_102(v100, v101)) {
                  if (table_39.TryChangePresentToUnknown({v101, v100})) {
                    if (table_55.TryChangePresentToUnknown({v100})) {
                      if (pos_neg_75.TryChangePresentToUnknown({v100})) {
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s107 = idx_106.First({true}); s107 != ::hyde::rt::kNoRow; s107 = idx_106.Next(s107)) {
              const auto r107 = table_88.RowAt(s107);
              const auto v108 = r107.x;
              const auto v109 = r107.c1;
              if (true == v109) {
                if (find_110(v108, v109)) {
                  if (table_51.TryChangePresentToUnknown({v109, v108})) {
                    if (table_64.TryChangePresentToUnknown({v108})) {
                      if (mixed_81.TryChangePresentToUnknown({v108})) {
                      }
                    }
                  }
                }
              }
            }
            vec114.Add({true});
            vec115.Add({true});
            vec116.Add({true});
          }
        }
      }
    }
  }
  for (auto [v118] : vec95) {
    if (table_20.TryChangePresentToAbsent({v118})) {
      if (table_26.TryChangePresentToUnknown({v118})) {
        if (table_67.TryChangePresentToUnknown({true, v118})) {
          if (__9.TryChangePresentToUnknown({true})) {
            for (uint32_t s119 = idx_98.First({true}); s119 != ::hyde::rt::kNoRow; s119 = idx_98.Next(s119)) {
              const auto r119 = table_84.RowAt(s119);
              const auto v120 = r119.x;
              const auto v121 = r119.c1;
              if (true == v121) {
                if (find_102(v120, v121)) {
                  if (!find_123(v121)) {
                    if (const auto ins4 = table_39.TryChangeAbsentOrUnknownToPresent({v121, v120}); ins4.changed) {
                      if (ins4.added_row) {
                        idx_490.Add({v120}, ins4.id);
                      }
                      if (const auto ins5 = table_55.TryChangeAbsentOrUnknownToPresent({v120}); ins5.changed) {
                        if (const auto ins6 = pos_neg_75.TryChangeAbsentOrUnknownToPresent({v120}); ins6.changed) {
                        }
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s126 = idx_106.First({true}); s126 != ::hyde::rt::kNoRow; s126 = idx_106.Next(s126)) {
              const auto r126 = table_88.RowAt(s126);
              const auto v127 = r126.x;
              const auto v128 = r126.c1;
              if (true == v128) {
                if (find_110(v127, v128)) {
                  if (!find_130(v128)) {
                    if (const auto ins7 = table_51.TryChangeAbsentOrUnknownToPresent({v128, v127}); ins7.changed) {
                      if (ins7.added_row) {
                        idx_262.Add({v127}, ins7.id);
                      }
                      if (const auto ins8 = table_64.TryChangeAbsentOrUnknownToPresent({v127}); ins8.changed) {
                        if (const auto ins9 = mixed_81.TryChangeAbsentOrUnknownToPresent({v127}); ins9.changed) {
                        }
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s134 = idx_133.First({true}); s134 != ::hyde::rt::kNoRow; s134 = idx_133.Next(s134)) {
              const auto r134 = table_5.RowAt(s134);
              const auto v135 = r134.x;
              const auto v136 = r134.c1;
              if (true == v136) {
                if (table_32.TryChangePresentToUnknown({true, v135})) {
                  if (table_36.TryChangePresentToUnknown({v135})) {
                    if (table_84.TryChangePresentToUnknown({v135, true})) {
                      if (table_39.TryChangePresentToUnknown({true, v135})) {
                        if (table_55.TryChangePresentToUnknown({v135})) {
                          if (pos_neg_75.TryChangePresentToUnknown({v135})) {
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s138 = idx_137.First({true}); s138 != ::hyde::rt::kNoRow; s138 = idx_137.Next(s138)) {
              const auto r138 = table_12.RowAt(s138);
              const auto v139 = r138.x;
              const auto v140 = r138.c1;
              if (true == v140) {
                if (table_43.TryChangePresentToUnknown({true, v139})) {
                  if (table_58.TryChangePresentToUnknown({v139})) {
                    if (both_pos_78.TryChangePresentToUnknown({v139})) {
                    }
                  }
                }
              }
            }
            for (uint32_t s142 = idx_141.First({true}); s142 != ::hyde::rt::kNoRow; s142 = idx_141.Next(s142)) {
              const auto r142 = table_16.RowAt(s142);
              const auto v143 = r142.x;
              const auto v144 = r142.c1;
              if (true == v144) {
                if (table_47.TryChangePresentToUnknown({true, v143})) {
                  if (table_61.TryChangePresentToUnknown({v143})) {
                    if (mixed_81.TryChangePresentToUnknown({v143})) {
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
  for (auto [v148] : vec145) {
    if (const auto ins10 = table_23.TryChangeAbsentToPresent({v148}); ins10.changed) {
      if (const auto ins11 = table_29.TryChangeAbsentOrUnknownToPresent({v148}); ins11.changed) {
        if (const auto ins12 = table_71.TryChangeAbsentOrUnknownToPresent({true, v148}); ins12.changed) {
          if (const auto ins13 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins13.changed) {
            for (uint32_t s149 = idx_98.First({true}); s149 != ::hyde::rt::kNoRow; s149 = idx_98.Next(s149)) {
              const auto r149 = table_84.RowAt(s149);
              const auto v150 = r149.x;
              const auto v151 = r149.c1;
              if (true == v151) {
                if (find_102(v150, v151)) {
                  if (table_39.TryChangePresentToUnknown({v151, v150})) {
                    if (table_55.TryChangePresentToUnknown({v150})) {
                      if (pos_neg_75.TryChangePresentToUnknown({v150})) {
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s153 = idx_106.First({true}); s153 != ::hyde::rt::kNoRow; s153 = idx_106.Next(s153)) {
              const auto r153 = table_88.RowAt(s153);
              const auto v154 = r153.x;
              const auto v155 = r153.c1;
              if (true == v155) {
                if (find_110(v154, v155)) {
                  if (table_51.TryChangePresentToUnknown({v155, v154})) {
                    if (table_64.TryChangePresentToUnknown({v154})) {
                      if (mixed_81.TryChangePresentToUnknown({v154})) {
                      }
                    }
                  }
                }
              }
            }
            vec114.Add({true});
            vec115.Add({true});
            vec116.Add({true});
          }
        }
      }
    }
  }
  for (auto [v158] : vec146) {
    if (table_23.TryChangePresentToAbsent({v158})) {
      if (table_29.TryChangePresentToUnknown({v158})) {
        if (table_71.TryChangePresentToUnknown({true, v158})) {
          if (__9.TryChangePresentToUnknown({true})) {
            for (uint32_t s159 = idx_98.First({true}); s159 != ::hyde::rt::kNoRow; s159 = idx_98.Next(s159)) {
              const auto r159 = table_84.RowAt(s159);
              const auto v160 = r159.x;
              const auto v161 = r159.c1;
              if (true == v161) {
                if (find_102(v160, v161)) {
                  if (!find_123(v161)) {
                    if (const auto ins14 = table_39.TryChangeAbsentOrUnknownToPresent({v161, v160}); ins14.changed) {
                      if (ins14.added_row) {
                        idx_490.Add({v160}, ins14.id);
                      }
                      if (const auto ins15 = table_55.TryChangeAbsentOrUnknownToPresent({v160}); ins15.changed) {
                        if (const auto ins16 = pos_neg_75.TryChangeAbsentOrUnknownToPresent({v160}); ins16.changed) {
                        }
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s164 = idx_106.First({true}); s164 != ::hyde::rt::kNoRow; s164 = idx_106.Next(s164)) {
              const auto r164 = table_88.RowAt(s164);
              const auto v165 = r164.x;
              const auto v166 = r164.c1;
              if (true == v166) {
                if (find_110(v165, v166)) {
                  if (!find_130(v166)) {
                    if (const auto ins17 = table_51.TryChangeAbsentOrUnknownToPresent({v166, v165}); ins17.changed) {
                      if (ins17.added_row) {
                        idx_262.Add({v165}, ins17.id);
                      }
                      if (const auto ins18 = table_64.TryChangeAbsentOrUnknownToPresent({v165}); ins18.changed) {
                        if (const auto ins19 = mixed_81.TryChangeAbsentOrUnknownToPresent({v165}); ins19.changed) {
                        }
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s169 = idx_133.First({true}); s169 != ::hyde::rt::kNoRow; s169 = idx_133.Next(s169)) {
              const auto r169 = table_5.RowAt(s169);
              const auto v170 = r169.x;
              const auto v171 = r169.c1;
              if (true == v171) {
                if (table_32.TryChangePresentToUnknown({true, v170})) {
                  if (table_36.TryChangePresentToUnknown({v170})) {
                    if (table_84.TryChangePresentToUnknown({v170, true})) {
                      if (table_39.TryChangePresentToUnknown({true, v170})) {
                        if (table_55.TryChangePresentToUnknown({v170})) {
                          if (pos_neg_75.TryChangePresentToUnknown({v170})) {
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            for (uint32_t s172 = idx_137.First({true}); s172 != ::hyde::rt::kNoRow; s172 = idx_137.Next(s172)) {
              const auto r172 = table_12.RowAt(s172);
              const auto v173 = r172.x;
              const auto v174 = r172.c1;
              if (true == v174) {
                if (table_43.TryChangePresentToUnknown({true, v173})) {
                  if (table_58.TryChangePresentToUnknown({v173})) {
                    if (both_pos_78.TryChangePresentToUnknown({v173})) {
                    }
                  }
                }
              }
            }
            for (uint32_t s175 = idx_141.First({true}); s175 != ::hyde::rt::kNoRow; s175 = idx_141.Next(s175)) {
              const auto r175 = table_16.RowAt(s175);
              const auto v176 = r175.x;
              const auto v177 = r175.c1;
              if (true == v177) {
                if (table_47.TryChangePresentToUnknown({true, v176})) {
                  if (table_61.TryChangePresentToUnknown({v176})) {
                    if (mixed_81.TryChangePresentToUnknown({v176})) {
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
  for (auto [v180] : vec178) {
    if (const auto ins20 = table_5.TryChangeAbsentToPresent({v180, true}); ins20.changed) {
      if (ins20.added_row) {
        idx_133.Add({true}, ins20.id);
      }
      vec114.Add({true});
    }
    if (const auto ins21 = table_12.TryChangeAbsentToPresent({v180, true}); ins21.changed) {
      if (ins21.added_row) {
        idx_137.Add({true}, ins21.id);
      }
      vec115.Add({true});
    }
    if (const auto ins22 = table_16.TryChangeAbsentToPresent({v180, true}); ins22.changed) {
      if (ins22.added_row) {
        idx_141.Add({true}, ins22.id);
      }
      vec116.Add({true});
    }
  }
  for (auto [v183] : vec181) {
    if (const auto ins23 = table_88.TryChangeAbsentToPresent({v183, true}); ins23.changed) {
      if (ins23.added_row) {
        idx_106.Add({true}, ins23.id);
      }
      if (!find_130(true)) {
        if (const auto ins24 = table_51.TryChangeAbsentOrUnknownToPresent({true, v183}); ins24.changed) {
          if (ins24.added_row) {
            idx_262.Add({v183}, ins24.id);
          }
          if (const auto ins25 = table_64.TryChangeAbsentOrUnknownToPresent({v183}); ins25.changed) {
            if (const auto ins26 = mixed_81.TryChangeAbsentOrUnknownToPresent({v183}); ins26.changed) {
            }
          }
        }
      }
    }
  }
  vec94.Clear();
  vec95.Clear();
  vec145.Clear();
  vec146.Clear();
  vec178.Clear();
  vec181.Clear();
  flow_568(std::move(vec114), std::move(vec115), std::move(vec116));
  return false;
}

bool Database::find_102(int32_t v103, bool v104) {
  switch (table_84.State({v103, v104})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_84.TryChangeUnknownToAbsent({v103, v104})) {
        if (find_508(v103, v104)) {
          if (const auto ins27 = table_84.TryChangeAbsentToPresent({v103, v104}); ins27.changed) {
            if (ins27.added_row) {
              idx_98.Add({v104}, ins27.id);
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
  if (find_102(v103, v104)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_110(int32_t v111, bool v112) {
  switch (table_88.State({v111, v112})) {
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

bool Database::find_123(bool v124) {
  switch (__9.State({v124})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v124})) {
        if (find_561(v124)) {
          if (const auto ins28 = __9.TryChangeAbsentToPresent({v124}); ins28.changed) {
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

bool Database::find_130(bool v131) {
  switch (__9.State({v131})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v131})) {
        if (find_556(v131)) {
          if (const auto ins29 = __9.TryChangeAbsentToPresent({v131}); ins29.changed) {
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
  if (find_130(v131)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::set_c1_1(::hyde::rt::Vec<Tup_i32> vec202, ::hyde::rt::Vec<Tup_i32> vec203) {
  ::hyde::rt::Vec<Tup_i32> vec205(allocator);
  ::hyde::rt::Vec<Tup_i32> vec206(allocator);
  ::hyde::rt::Vec<Tup_i32> vec207(allocator);
  proc_92(std::move(vec202), std::move(vec203), std::move(vec205), std::move(vec205), std::move(vec206), std::move(vec207));
  return true;
}

bool Database::set_c2_1(::hyde::rt::Vec<Tup_i32> vec209, ::hyde::rt::Vec<Tup_i32> vec210) {
  ::hyde::rt::Vec<Tup_i32> vec212(allocator);
  ::hyde::rt::Vec<Tup_i32> vec213(allocator);
  ::hyde::rt::Vec<Tup_i32> vec214(allocator);
  proc_92(std::move(vec212), std::move(vec212), std::move(vec209), std::move(vec210), std::move(vec213), std::move(vec214));
  return true;
}

bool Database::add_r_1(::hyde::rt::Vec<Tup_i32> vec216) {
  ::hyde::rt::Vec<Tup_i32> vec218(allocator);
  ::hyde::rt::Vec<Tup_i32> vec219(allocator);
  ::hyde::rt::Vec<Tup_i32> vec220(allocator);
  proc_92(std::move(vec218), std::move(vec218), std::move(vec219), std::move(vec219), std::move(vec216), std::move(vec220));
  return true;
}

bool Database::add_s_1(::hyde::rt::Vec<Tup_i32> vec222) {
  ::hyde::rt::Vec<Tup_i32> vec224(allocator);
  ::hyde::rt::Vec<Tup_i32> vec225(allocator);
  ::hyde::rt::Vec<Tup_i32> vec226(allocator);
  proc_92(std::move(vec224), std::move(vec224), std::move(vec225), std::move(vec225), std::move(vec226), std::move(vec222));
  return true;
}

bool Database::find_232(int32_t v233) {
  switch (pos_neg_75.State({v233})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (pos_neg_75.TryChangeUnknownToAbsent({v233})) {
        if (find_469(v233)) {
          if (const auto ins30 = pos_neg_75.TryChangeAbsentToPresent({v233}); ins30.changed) {
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
  if (find_232(v233)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_234(int32_t v235) {
  switch (both_pos_78.State({v235})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (both_pos_78.TryChangeUnknownToAbsent({v235})) {
        if (find_419(v235)) {
          if (const auto ins31 = both_pos_78.TryChangeAbsentToPresent({v235}); ins31.changed) {
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
  if (find_234(v235)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_236(int32_t v237) {
  switch (mixed_81.State({v237})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (mixed_81.TryChangeUnknownToAbsent({v237})) {
        if (find_238(v237)) {
          if (const auto ins32 = mixed_81.TryChangeAbsentToPresent({v237}); ins32.changed) {
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
  if (find_236(v237)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_238(int32_t v239) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_242(v239)) {
    return true;
  }
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_245(v239)) {
    return true;
  }
  return false;
}

bool Database::find_242(int32_t v243) {
  switch (table_61.State({v243})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_61.TryChangeUnknownToAbsent({v243})) {
        if (find_280(v243)) {
          if (const auto ins33 = table_61.TryChangeAbsentToPresent({v243}); ins33.changed) {
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
  if (find_283(v243)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_245(int32_t v246) {
  switch (table_64.State({v246})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_64.TryChangeUnknownToAbsent({v246})) {
        if (find_248(v246)) {
          if (const auto ins34 = table_64.TryChangeAbsentToPresent({v246}); ins34.changed) {
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
  if (find_251(v246)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_248(int32_t v249) {
  if (find_256(v249)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_251(int32_t v252) {
  switch (table_64.State({v252})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_64.TryChangeUnknownToAbsent({v252})) {
        if (find_248(v252)) {
          if (const auto ins35 = table_64.TryChangeAbsentToPresent({v252}); ins35.changed) {
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
  if (find_251(v252)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_256(int32_t v257) {
  if (find_259(v257)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_259(int32_t v260) {
  for (uint32_t s263 = idx_262.First({v260}); s263 != ::hyde::rt::kNoRow; s263 = idx_262.Next(s263)) {
    const auto r263 = table_51.RowAt(s263);
    const auto v264 = r263.c0;
    const auto v265 = r263.x;
    if (v260 == v265) {
      if (find_266(v264, v265)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_266(bool v267, int32_t v268) {
  switch (table_51.State({v267, v268})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_51.TryChangeUnknownToAbsent({v267, v268})) {
        if (find_270(v267, v268)) {
          if (const auto ins36 = table_51.TryChangeAbsentToPresent({v267, v268}); ins36.changed) {
            if (ins36.added_row) {
              idx_262.Add({v268}, ins36.id);
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
  if (find_266(v267, v268)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_270(bool v271, int32_t v272) {
  if (find_275(v272, v271)) {
    if (find_130(v271)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_275(int32_t v276, bool v277) {
  switch (table_88.State({v276, v277})) {
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

bool Database::find_280(int32_t v281) {
  if (find_288(v281)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_283(int32_t v284) {
  switch (table_61.State({v284})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_61.TryChangeUnknownToAbsent({v284})) {
        if (find_280(v284)) {
          if (const auto ins37 = table_61.TryChangeAbsentToPresent({v284}); ins37.changed) {
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
  if (find_283(v284)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_288(int32_t v289) {
  if (find_291(v289)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_291(int32_t v292) {
  for (uint32_t s295 = idx_294.First({v292}); s295 != ::hyde::rt::kNoRow; s295 = idx_294.Next(s295)) {
    const auto r295 = table_47.RowAt(s295);
    const auto v296 = r295.c0;
    const auto v297 = r295.x;
    if (v292 == v297) {
      if (find_298(v296, v297)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_298(bool v299, int32_t v300) {
  switch (table_47.State({v299, v300})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_47.TryChangeUnknownToAbsent({v299, v300})) {
        if (find_302(v299, v300)) {
          if (const auto ins38 = table_47.TryChangeAbsentToPresent({v299, v300}); ins38.changed) {
            if (ins38.added_row) {
              idx_294.Add({v300}, ins38.id);
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
  if (find_298(v299, v300)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_302(bool v303, int32_t v304) {
  if (find_307(v304, v303)) {
    if (find_311(v303)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_307(int32_t v308, bool v309) {
  switch (table_16.State({v308, v309})) {
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
  switch (__9.State({v312})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v312})) {
        if (find_314(v312)) {
          if (const auto ins39 = __9.TryChangeAbsentToPresent({v312}); ins39.changed) {
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
  if (find_317(v312)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_314(bool v315) {
  if (find_322(v315)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_317(bool v318) {
  switch (__9.State({v318})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v318})) {
        if (find_314(v318)) {
          if (const auto ins40 = __9.TryChangeAbsentToPresent({v318}); ins40.changed) {
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
  if (find_317(v318)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_322(bool v323) {
  for (uint32_t s325 = 0; s325 < table_67.NumRows(); ++s325) {
    const auto r325 = table_67.RowAt(s325);
    const auto v326 = r325.c0;
    const auto v327 = r325.c1;
    if (v326 == v323) {
      if (find_328(v326, v327)) {
        return true;
      }
    }
  }
  for (uint32_t s332 = 0; s332 < table_71.NumRows(); ++s332) {
    const auto r332 = table_71.RowAt(s332);
    const auto v333 = r332.c0;
    const auto v334 = r332.c1;
    if (v333 == v323) {
      if (find_335(v333, v334)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_328(bool v329, int32_t v330) {
  switch (table_67.State({v329, v330})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_67.TryChangeUnknownToAbsent({v329, v330})) {
        if (find_379(v329, v330)) {
          if (const auto ins41 = table_67.TryChangeAbsentToPresent({v329, v330}); ins41.changed) {
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
  if (find_328(v329, v330)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_335(bool v336, int32_t v337) {
  switch (table_71.State({v336, v337})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_71.TryChangeUnknownToAbsent({v336, v337})) {
        if (find_339(v336, v337)) {
          if (const auto ins42 = table_71.TryChangeAbsentToPresent({v336, v337}); ins42.changed) {
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
  if (find_335(v336, v337)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_339(bool v340, int32_t v341) {
  if (find_344(v340, v341)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_344(bool v345, int32_t v346) {
  if (find_348(v346)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_348(int32_t v349) {
  switch (table_29.State({v349})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_29.TryChangeUnknownToAbsent({v349})) {
        if (find_351(v349)) {
          if (const auto ins43 = table_29.TryChangeAbsentToPresent({v349}); ins43.changed) {
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
  if (find_354(v349)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_351(int32_t v352) {
  if (find_357(v352)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_354(int32_t v355) {
  switch (table_29.State({v355})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_29.TryChangeUnknownToAbsent({v355})) {
        if (find_357(v355)) {
          if (const auto ins44 = table_29.TryChangeAbsentToPresent({v355}); ins44.changed) {
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
  if (find_354(v355)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_357(int32_t v358) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_361(v358)) {
    return true;
  }
  return false;
}

bool Database::find_361(int32_t v362) {
  switch (table_23.State({v362})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_23.TryChangeUnknownToAbsent({v362})) {
        if (find_364(v362)) {
          if (const auto ins45 = table_23.TryChangeAbsentToPresent({v362}); ins45.changed) {
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
  if (find_367(v362)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_364(int32_t v365) {
  if (find_372(v365)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_367(int32_t v368) {
  switch (table_23.State({v368})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_23.TryChangeUnknownToAbsent({v368})) {
        if (find_364(v368)) {
          if (const auto ins46 = table_23.TryChangeAbsentToPresent({v368}); ins46.changed) {
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
  if (find_367(v368)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_372(int32_t v373) {
  if (find_375(v373)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_375(int32_t v376) {
  return false;
}

bool Database::find_379(bool v380, int32_t v381) {
  if (find_384(v380, v381)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_384(bool v385, int32_t v386) {
  if (find_388(v386)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_388(int32_t v389) {
  switch (table_26.State({v389})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_26.TryChangeUnknownToAbsent({v389})) {
        if (find_391(v389)) {
          if (const auto ins47 = table_26.TryChangeAbsentToPresent({v389}); ins47.changed) {
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
  if (find_394(v389)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_391(int32_t v392) {
  if (find_397(v392)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_394(int32_t v395) {
  switch (table_26.State({v395})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_26.TryChangeUnknownToAbsent({v395})) {
        if (find_397(v395)) {
          if (const auto ins48 = table_26.TryChangeAbsentToPresent({v395}); ins48.changed) {
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
  if (find_394(v395)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_397(int32_t v398) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_401(v398)) {
    return true;
  }
  return false;
}

bool Database::find_401(int32_t v402) {
  switch (table_20.State({v402})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_20.TryChangeUnknownToAbsent({v402})) {
        if (find_404(v402)) {
          if (const auto ins49 = table_20.TryChangeAbsentToPresent({v402}); ins49.changed) {
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
  if (find_407(v402)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_404(int32_t v405) {
  if (find_412(v405)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_407(int32_t v408) {
  switch (table_20.State({v408})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_20.TryChangeUnknownToAbsent({v408})) {
        if (find_404(v408)) {
          if (const auto ins50 = table_20.TryChangeAbsentToPresent({v408}); ins50.changed) {
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
  if (find_407(v408)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_412(int32_t v413) {
  if (find_415(v413)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_415(int32_t v416) {
  return false;
}

bool Database::find_419(int32_t v420) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_423(v420)) {
    return true;
  }
  return false;
}

bool Database::find_423(int32_t v424) {
  switch (table_58.State({v424})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_58.TryChangeUnknownToAbsent({v424})) {
        if (find_426(v424)) {
          if (const auto ins51 = table_58.TryChangeAbsentToPresent({v424}); ins51.changed) {
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
  if (find_429(v424)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_426(int32_t v427) {
  if (find_434(v427)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_429(int32_t v430) {
  switch (table_58.State({v430})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_58.TryChangeUnknownToAbsent({v430})) {
        if (find_426(v430)) {
          if (const auto ins52 = table_58.TryChangeAbsentToPresent({v430}); ins52.changed) {
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
  if (find_429(v430)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_434(int32_t v435) {
  if (find_437(v435)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_437(int32_t v438) {
  for (uint32_t s441 = idx_440.First({v438}); s441 != ::hyde::rt::kNoRow; s441 = idx_440.Next(s441)) {
    const auto r441 = table_43.RowAt(s441);
    const auto v442 = r441.c0;
    const auto v443 = r441.x;
    if (v438 == v443) {
      if (find_444(v442, v443)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_444(bool v445, int32_t v446) {
  switch (table_43.State({v445, v446})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_43.TryChangeUnknownToAbsent({v445, v446})) {
        if (find_448(v445, v446)) {
          if (const auto ins53 = table_43.TryChangeAbsentToPresent({v445, v446}); ins53.changed) {
            if (ins53.added_row) {
              idx_440.Add({v446}, ins53.id);
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
  if (find_444(v445, v446)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_448(bool v449, int32_t v450) {
  if (find_453(v450, v449)) {
    if (find_457(v449)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_453(int32_t v454, bool v455) {
  switch (table_12.State({v454, v455})) {
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

bool Database::find_457(bool v458) {
  switch (__9.State({v458})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v458})) {
        if (find_460(v458)) {
          if (const auto ins54 = __9.TryChangeAbsentToPresent({v458}); ins54.changed) {
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
  if (find_463(v458)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_460(bool v461) {
  if (find_322(v461)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_463(bool v464) {
  switch (__9.State({v464})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v464})) {
        if (find_460(v464)) {
          if (const auto ins55 = __9.TryChangeAbsentToPresent({v464}); ins55.changed) {
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
  if (find_463(v464)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_469(int32_t v470) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_473(v470)) {
    return true;
  }
  return false;
}

bool Database::find_473(int32_t v474) {
  switch (table_55.State({v474})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_55.TryChangeUnknownToAbsent({v474})) {
        if (find_476(v474)) {
          if (const auto ins56 = table_55.TryChangeAbsentToPresent({v474}); ins56.changed) {
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
  if (find_479(v474)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_476(int32_t v477) {
  if (find_484(v477)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_479(int32_t v480) {
  switch (table_55.State({v480})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_55.TryChangeUnknownToAbsent({v480})) {
        if (find_476(v480)) {
          if (const auto ins57 = table_55.TryChangeAbsentToPresent({v480}); ins57.changed) {
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
  if (find_479(v480)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_484(int32_t v485) {
  if (find_487(v485)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_487(int32_t v488) {
  for (uint32_t s491 = idx_490.First({v488}); s491 != ::hyde::rt::kNoRow; s491 = idx_490.Next(s491)) {
    const auto r491 = table_39.RowAt(s491);
    const auto v492 = r491.c0;
    const auto v493 = r491.x;
    if (v488 == v493) {
      if (find_494(v492, v493)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_494(bool v495, int32_t v496) {
  switch (table_39.State({v495, v496})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_39.TryChangeUnknownToAbsent({v495, v496})) {
        if (find_498(v495, v496)) {
          if (const auto ins58 = table_39.TryChangeAbsentToPresent({v495, v496}); ins58.changed) {
            if (ins58.added_row) {
              idx_490.Add({v496}, ins58.id);
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
  if (find_494(v495, v496)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_498(bool v499, int32_t v500) {
  if (find_503(v500, v499)) {
    if (find_123(v499)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_503(int32_t v504, bool v505) {
  switch (table_84.State({v504, v505})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_84.TryChangeUnknownToAbsent({v504, v505})) {
        if (find_508(v504, v505)) {
          if (const auto ins59 = table_84.TryChangeAbsentToPresent({v504, v505}); ins59.changed) {
            if (ins59.added_row) {
              idx_98.Add({v505}, ins59.id);
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
  if (find_102(v504, v505)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_508(int32_t v509, bool v510) {
  if (find_513(v509)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_513(int32_t v514) {
  switch (table_36.State({v514})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_36.TryChangeUnknownToAbsent({v514})) {
        if (find_516(v514)) {
          if (const auto ins60 = table_36.TryChangeAbsentToPresent({v514}); ins60.changed) {
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
  if (find_519(v514)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_516(int32_t v517) {
  if (find_524(v517)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_519(int32_t v520) {
  switch (table_36.State({v520})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_36.TryChangeUnknownToAbsent({v520})) {
        if (find_516(v520)) {
          if (const auto ins61 = table_36.TryChangeAbsentToPresent({v520}); ins61.changed) {
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
  if (find_519(v520)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_524(int32_t v525) {
  for (uint32_t s528 = idx_527.First({v525}); s528 != ::hyde::rt::kNoRow; s528 = idx_527.Next(s528)) {
    const auto r528 = table_32.RowAt(s528);
    const auto v529 = r528.c0;
    const auto v530 = r528.x;
    if (v525 == v530) {
      if (find_531(v529, v530)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_531(bool v532, int32_t v533) {
  switch (table_32.State({v532, v533})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_32.TryChangeUnknownToAbsent({v532, v533})) {
        if (find_535(v532, v533)) {
          if (const auto ins62 = table_32.TryChangeAbsentToPresent({v532, v533}); ins62.changed) {
            if (ins62.added_row) {
              idx_527.Add({v533}, ins62.id);
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
  if (find_531(v532, v533)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_535(bool v536, int32_t v537) {
  if (find_540(v537, v536)) {
    if (find_544(v536)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_540(int32_t v541, bool v542) {
  switch (table_5.State({v541, v542})) {
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

bool Database::find_544(bool v545) {
  switch (__9.State({v545})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v545})) {
        if (find_547(v545)) {
          if (const auto ins63 = __9.TryChangeAbsentToPresent({v545}); ins63.changed) {
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
  if (find_550(v545)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_547(bool v548) {
  if (find_322(v548)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_550(bool v551) {
  switch (__9.State({v551})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v551})) {
        if (find_547(v551)) {
          if (const auto ins64 = __9.TryChangeAbsentToPresent({v551}); ins64.changed) {
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
  if (find_550(v551)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_556(bool v557) {
  if (find_322(v557)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_561(bool v562) {
  if (find_322(v562)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::flow_568(::hyde::rt::Vec<Tup_b> vec114, ::hyde::rt::Vec<Tup_b> vec115, ::hyde::rt::Vec<Tup_b> vec116) {
  if ((g93 += 1) == 1) {
  }
  vec116.SortAndUnique();
  for (auto [v186] : vec116) {
    if (const uint32_t j185_0 = __9.Find({v186}); j185_0 != ::hyde::rt::kNoRow) {
      const auto r185_0 = __9.RowAt(j185_0);
      const auto v188 = r185_0.c0;
      for (uint32_t j185_1 = idx_141.First({v186}); j185_1 != ::hyde::rt::kNoRow; j185_1 = idx_141.Next(j185_1)) {
        const auto r185_1 = table_16.RowAt(j185_1);
        const auto v189 = r185_1.x;
        const auto v187 = r185_1.c1;
        if (v186 == v187 && v186 == v188) {
          switch (__9.State({v187})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins65 = table_47.TryChangeAbsentOrUnknownToPresent({v187, v189}); ins65.changed) {
                if (ins65.added_row) {
                  idx_294.Add({v189}, ins65.id);
                }
                if (const auto ins66 = table_61.TryChangeAbsentOrUnknownToPresent({v189}); ins66.changed) {
                  if (const auto ins67 = mixed_81.TryChangeAbsentOrUnknownToPresent({v189}); ins67.changed) {
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
  vec116.Clear();
  vec115.SortAndUnique();
  for (auto [v191] : vec115) {
    if (const uint32_t j190_0 = __9.Find({v191}); j190_0 != ::hyde::rt::kNoRow) {
      const auto r190_0 = __9.RowAt(j190_0);
      const auto v193 = r190_0.c0;
      for (uint32_t j190_1 = idx_137.First({v191}); j190_1 != ::hyde::rt::kNoRow; j190_1 = idx_137.Next(j190_1)) {
        const auto r190_1 = table_12.RowAt(j190_1);
        const auto v194 = r190_1.x;
        const auto v192 = r190_1.c1;
        if (v191 == v192 && v191 == v193) {
          switch (__9.State({v192})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins68 = table_43.TryChangeAbsentOrUnknownToPresent({v192, v194}); ins68.changed) {
                if (ins68.added_row) {
                  idx_440.Add({v194}, ins68.id);
                }
                if (const auto ins69 = table_58.TryChangeAbsentOrUnknownToPresent({v194}); ins69.changed) {
                  if (const auto ins70 = both_pos_78.TryChangeAbsentOrUnknownToPresent({v194}); ins70.changed) {
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
  vec115.Clear();
  vec114.SortAndUnique();
  for (auto [v196] : vec114) {
    if (const uint32_t j195_0 = __9.Find({v196}); j195_0 != ::hyde::rt::kNoRow) {
      const auto r195_0 = __9.RowAt(j195_0);
      const auto v198 = r195_0.c0;
      for (uint32_t j195_1 = idx_133.First({v196}); j195_1 != ::hyde::rt::kNoRow; j195_1 = idx_133.Next(j195_1)) {
        const auto r195_1 = table_5.RowAt(j195_1);
        const auto v199 = r195_1.x;
        const auto v197 = r195_1.c1;
        if (v196 == v197 && v196 == v198) {
          switch (__9.State({v197})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins71 = table_32.TryChangeAbsentOrUnknownToPresent({v197, v199}); ins71.changed) {
                if (ins71.added_row) {
                  idx_527.Add({v199}, ins71.id);
                }
                if (const auto ins72 = table_36.TryChangeAbsentOrUnknownToPresent({v199}); ins72.changed) {
                  if (const auto ins73 = table_84.TryChangeAbsentOrUnknownToPresent({v199, true}); ins73.changed) {
                    if (ins73.added_row) {
                      idx_98.Add({true}, ins73.id);
                    }
                    if (!find_123(true)) {
                      if (const auto ins74 = table_39.TryChangeAbsentOrUnknownToPresent({true, v199}); ins74.changed) {
                        if (ins74.added_row) {
                          idx_490.Add({v199}, ins74.id);
                        }
                        if (const auto ins75 = table_55.TryChangeAbsentOrUnknownToPresent({v199}); ins75.changed) {
                          if (const auto ins76 = pos_neg_75.TryChangeAbsentOrUnknownToPresent({v199}); ins76.changed) {
                          }
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
  vec114.Clear();
  return true;
}

Database::pos_neg_f_cursor Database::pos_neg_f() {
  return {*this, 0};
}

bool Database::pos_neg_f_cursor::next(int32_t &X) {
  while (pos < db.pos_neg_75.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.pos_neg_75.RowAt(id);
    if (!db.find_232(row.x)) {
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
  while (pos < db.both_pos_78.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.both_pos_78.RowAt(id);
    if (!db.find_234(row.x)) {
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
  while (pos < db.mixed_81.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.mixed_81.RowAt(id);
    if (!db.find_236(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

