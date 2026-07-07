// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_42(allocator_),
      __9(allocator_),
      table_12(allocator_),
      table_15(allocator_),
      table_18(allocator_),
      idx_93(allocator_),
      table_22(allocator_),
      table_26(allocator_),
      gated_30(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec81(allocator);
  ::hyde::rt::Vec<Tup_i32> vec82(allocator);
  ::hyde::rt::Vec<Tup_i32> vec83(allocator);
  proc_33(std::move(vec81), std::move(vec81), std::move(vec82), std::move(vec82), std::move(vec83));
  return false;
}

bool Database::proc_33(::hyde::rt::Vec<Tup_i32> vec35, ::hyde::rt::Vec<Tup_i32> vec36, ::hyde::rt::Vec<Tup_i32> vec46, ::hyde::rt::Vec<Tup_i32> vec47, ::hyde::rt::Vec<Tup_i32> vec55) {
  ::hyde::rt::Vec<Tup_b> vec39(allocator);
  for (auto [v38] : vec35) {
    if (const auto ins0 = table_12.TryChangeAbsentToPresent({v38}); ins0.changed) {
      if (const auto ins1 = table_22.TryChangeAbsentOrUnknownToPresent({true, v38}); ins1.changed) {
        if (const auto ins2 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins2.changed) {
          vec39.Add({true});
        }
      }
    }
  }
  for (auto [v41] : vec36) {
    if (table_12.TryChangePresentToAbsent({v41})) {
      if (table_22.TryChangePresentToUnknown({true, v41})) {
        if (__9.TryChangePresentToUnknown({true})) {
          for (uint32_t s43 = idx_42.First({true}); s43 != ::hyde::rt::kNoRow; s43 = idx_42.Next(s43)) {
            const auto r43 = table_5.RowAt(s43);
            const auto v44 = r43.x;
            const auto v45 = r43.c1;
            if (true == v45) {
              if (table_18.TryChangePresentToUnknown({true, v44})) {
                if (gated_30.TryChangePresentToUnknown({v44})) {
                }
              }
            }
          }
        }
      }
    }
  }
  for (auto [v49] : vec46) {
    if (const auto ins3 = table_15.TryChangeAbsentToPresent({v49}); ins3.changed) {
      if (const auto ins4 = table_26.TryChangeAbsentOrUnknownToPresent({true, v49}); ins4.changed) {
        if (const auto ins5 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins5.changed) {
          vec39.Add({true});
        }
      }
    }
  }
  for (auto [v51] : vec47) {
    if (table_15.TryChangePresentToAbsent({v51})) {
      if (table_26.TryChangePresentToUnknown({true, v51})) {
        if (__9.TryChangePresentToUnknown({true})) {
          for (uint32_t s52 = idx_42.First({true}); s52 != ::hyde::rt::kNoRow; s52 = idx_42.Next(s52)) {
            const auto r52 = table_5.RowAt(s52);
            const auto v53 = r52.x;
            const auto v54 = r52.c1;
            if (true == v54) {
              if (table_18.TryChangePresentToUnknown({true, v53})) {
                if (gated_30.TryChangePresentToUnknown({v53})) {
                }
              }
            }
          }
        }
      }
    }
  }
  for (auto [v57] : vec55) {
    if (const auto ins6 = table_5.TryChangeAbsentToPresent({v57, true}); ins6.changed) {
      if (ins6.added_row) {
        idx_42.Add({true}, ins6.id);
      }
      vec39.Add({true});
    }
  }
  vec35.Clear();
  vec36.Clear();
  vec46.Clear();
  vec47.Clear();
  vec55.Clear();
  flow_170(std::move(vec39));
  return false;
}

bool Database::add_a_1(::hyde::rt::Vec<Tup_i32> vec64, ::hyde::rt::Vec<Tup_i32> vec65) {
  ::hyde::rt::Vec<Tup_i32> vec67(allocator);
  ::hyde::rt::Vec<Tup_i32> vec68(allocator);
  proc_33(std::move(vec64), std::move(vec65), std::move(vec67), std::move(vec67), std::move(vec68));
  return true;
}

bool Database::add_b_1(::hyde::rt::Vec<Tup_i32> vec70, ::hyde::rt::Vec<Tup_i32> vec71) {
  ::hyde::rt::Vec<Tup_i32> vec73(allocator);
  ::hyde::rt::Vec<Tup_i32> vec74(allocator);
  proc_33(std::move(vec73), std::move(vec73), std::move(vec70), std::move(vec71), std::move(vec74));
  return true;
}

bool Database::add_item_1(::hyde::rt::Vec<Tup_i32> vec76) {
  ::hyde::rt::Vec<Tup_i32> vec78(allocator);
  ::hyde::rt::Vec<Tup_i32> vec79(allocator);
  proc_33(std::move(vec78), std::move(vec78), std::move(vec79), std::move(vec79), std::move(vec76));
  return true;
}

bool Database::find_84(int32_t v85) {
  switch (gated_30.State({v85})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (gated_30.TryChangeUnknownToAbsent({v85})) {
        if (find_86(v85)) {
          if (const auto ins7 = gated_30.TryChangeAbsentToPresent({v85}); ins7.changed) {
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

bool Database::find_86(int32_t v87) {
  if (find_90(v87)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_90(int32_t v91) {
  for (uint32_t s94 = idx_93.First({v91}); s94 != ::hyde::rt::kNoRow; s94 = idx_93.Next(s94)) {
    const auto r94 = table_18.RowAt(s94);
    const auto v95 = r94.c0;
    const auto v96 = r94.x;
    if (v91 == v96) {
      if (find_97(v95, v96)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_97(bool v98, int32_t v99) {
  switch (table_18.State({v98, v99})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v98, v99})) {
        if (find_101(v98, v99)) {
          if (const auto ins8 = table_18.TryChangeAbsentToPresent({v98, v99}); ins8.changed) {
            if (ins8.added_row) {
              idx_93.Add({v99}, ins8.id);
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
  if (find_97(v98, v99)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_101(bool v102, int32_t v103) {
  if (find_106(v103, v102)) {
    if (find_110(v102)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_106(int32_t v107, bool v108) {
  switch (table_5.State({v107, v108})) {
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

bool Database::find_110(bool v111) {
  switch (__9.State({v111})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v111})) {
        if (find_113(v111)) {
          if (const auto ins9 = __9.TryChangeAbsentToPresent({v111}); ins9.changed) {
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
  if (find_116(v111)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_113(bool v114) {
  if (find_121(v114)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_116(bool v117) {
  switch (__9.State({v117})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v117})) {
        if (find_113(v117)) {
          if (const auto ins10 = __9.TryChangeAbsentToPresent({v117}); ins10.changed) {
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
  if (find_116(v117)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_121(bool v122) {
  for (uint32_t s124 = 0; s124 < table_22.NumRows(); ++s124) {
    const auto r124 = table_22.RowAt(s124);
    const auto v125 = r124.c0;
    const auto v126 = r124.x;
    if (v125 == v122) {
      if (find_127(v125, v126)) {
        return true;
      }
    }
  }
  for (uint32_t s131 = 0; s131 < table_26.NumRows(); ++s131) {
    const auto r131 = table_26.RowAt(s131);
    const auto v132 = r131.c0;
    const auto v133 = r131.x;
    if (v132 == v122) {
      if (find_134(v132, v133)) {
        return true;
      }
    }
  }
  return false;
  return false;
}

bool Database::find_127(bool v128, int32_t v129) {
  switch (table_22.State({v128, v129})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_22.TryChangeUnknownToAbsent({v128, v129})) {
        if (find_154(v128, v129)) {
          if (const auto ins11 = table_22.TryChangeAbsentToPresent({v128, v129}); ins11.changed) {
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
  if (find_127(v128, v129)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_134(bool v135, int32_t v136) {
  switch (table_26.State({v135, v136})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_26.TryChangeUnknownToAbsent({v135, v136})) {
        if (find_138(v135, v136)) {
          if (const auto ins12 = table_26.TryChangeAbsentToPresent({v135, v136}); ins12.changed) {
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
  if (find_143(v140)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_143(int32_t v144) {
  switch (table_15.State({v144})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_15.TryChangeUnknownToAbsent({v144})) {
        if (find_146(v144)) {
          if (const auto ins13 = table_15.TryChangeAbsentToPresent({v144}); ins13.changed) {
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
  return false;
}

bool Database::find_149(int32_t v150) {
  switch (table_15.State({v150})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_15.TryChangeUnknownToAbsent({v150})) {
        if (find_146(v150)) {
          if (const auto ins14 = table_15.TryChangeAbsentToPresent({v150}); ins14.changed) {
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

bool Database::find_154(bool v155, int32_t v156) {
  if (find_159(v156)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_159(int32_t v160) {
  switch (table_12.State({v160})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_12.TryChangeUnknownToAbsent({v160})) {
        if (find_162(v160)) {
          if (const auto ins15 = table_12.TryChangeAbsentToPresent({v160}); ins15.changed) {
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
  if (find_165(v160)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_162(int32_t v163) {
  return false;
}

bool Database::find_165(int32_t v166) {
  switch (table_12.State({v166})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_12.TryChangeUnknownToAbsent({v166})) {
        if (find_162(v166)) {
          if (const auto ins16 = table_12.TryChangeAbsentToPresent({v166}); ins16.changed) {
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

bool Database::flow_170(::hyde::rt::Vec<Tup_b> vec39) {
  if ((g34 += 1) == 1) {
  }
  vec39.SortAndUnique();
  for (auto [v59] : vec39) {
    if (const uint32_t j58_0 = __9.Find({v59}); j58_0 != ::hyde::rt::kNoRow) {
      const auto r58_0 = __9.RowAt(j58_0);
      const auto v61 = r58_0.c0;
      for (uint32_t j58_1 = idx_42.First({v59}); j58_1 != ::hyde::rt::kNoRow; j58_1 = idx_42.Next(j58_1)) {
        const auto r58_1 = table_5.RowAt(j58_1);
        const auto v62 = r58_1.x;
        const auto v60 = r58_1.c1;
        if (v59 == v60 && v59 == v61) {
          switch (__9.State({v60})) {
            case ::hyde::rt::TupleState::kPresent: {
              if (const auto ins17 = table_18.TryChangeAbsentOrUnknownToPresent({v60, v62}); ins17.changed) {
                if (ins17.added_row) {
                  idx_93.Add({v62}, ins17.id);
                }
                if (const auto ins18 = gated_30.TryChangeAbsentOrUnknownToPresent({v62}); ins18.changed) {
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
  vec39.Clear();
  return true;
}

bool Database::gated_b(int32_t X) {
  return find_84(X);
}

