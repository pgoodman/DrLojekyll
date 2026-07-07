// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_56(allocator_),
      __9(allocator_),
      table_12(allocator_),
      table_15(allocator_),
      table_18(allocator_),
      idx_124(allocator_),
      table_22(allocator_),
      idx_96(allocator_),
      table_26(allocator_),
      gated_30(allocator_),
      ungated_33(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec83(allocator);
  ::hyde::rt::Vec<Tup_i32> vec84(allocator);
  proc_36(std::move(vec83), std::move(vec83), std::move(vec84), std::move(vec84));
  return false;
}

bool Database::proc_36(::hyde::rt::Vec<Tup_i32> vec38, ::hyde::rt::Vec<Tup_i32> vec39, ::hyde::rt::Vec<Tup_i32> vec60, ::hyde::rt::Vec<Tup_i32> vec61) {
  ::hyde::rt::Vec<Tup_b> vec47(allocator);
  for (auto [v41] : vec38) {
    if (const auto ins0 = table_15.TryChangeAbsentToPresent({v41}); ins0.changed) {
      if (const auto ins1 = table_26.TryChangeAbsentOrUnknownToPresent({true, v41}); ins1.changed) {
        if (const auto ins2 = __9.TryChangeAbsentOrUnknownToPresent({true}); ins2.changed) {
          for (uint32_t s42 = 0; s42 < table_12.NumRows(); ++s42) {
            const auto r42 = table_12.RowAt(s42);
            const auto v43 = r42.x;
            if (find_44(v43)) {
              if (table_22.TryChangePresentToUnknown({true, v43})) {
                ungated_33.TryChangePresentToUnknown({v43});
              }
            }
          }
          vec47.Add({true});
        }
      }
    }
  }
  for (auto [v49] : vec39) {
    if (table_15.TryChangePresentToAbsent({v49})) {
      if (table_26.TryChangePresentToUnknown({true, v49})) {
        if (__9.TryChangePresentToUnknown({true})) {
          for (uint32_t s50 = 0; s50 < table_12.NumRows(); ++s50) {
            const auto r50 = table_12.RowAt(s50);
            const auto v51 = r50.x;
            if (find_44(v51)) {
              if (!find_53(true)) {
                if (const auto ins3 = table_22.TryChangeAbsentOrUnknownToPresent({true, v51}); ins3.changed) {
                  if (ins3.added_row) {
                    idx_96.Add({v51}, ins3.id);
                  }
                  ungated_33.TryChangeAbsentOrUnknownToPresent({v51});
                }
              }
            }
          }
          for (uint32_t s57 = idx_56.First({true}); s57 != ::hyde::rt::kNoRow; s57 = idx_56.Next(s57)) {
            const auto r57 = table_5.RowAt(s57);
            const auto v58 = r57.x;
            const auto v59 = r57.c1;
            if (true == v59) {
              if (table_18.TryChangePresentToUnknown({true, v58})) {
                gated_30.TryChangePresentToUnknown({v58});
              }
            }
          }
        }
      }
    }
  }
  for (auto [v63] : vec60) {
    if (const auto ins4 = table_12.TryChangeAbsentToPresent({v63}); ins4.changed) {
      if (!find_53(true)) {
        if (const auto ins5 = table_22.TryChangeAbsentOrUnknownToPresent({true, v63}); ins5.changed) {
          if (ins5.added_row) {
            idx_96.Add({v63}, ins5.id);
          }
          ungated_33.TryChangeAbsentOrUnknownToPresent({v63});
        }
      }
      if (const auto ins6 = table_5.TryChangeAbsentOrUnknownToPresent({v63, true}); ins6.changed) {
        if (ins6.added_row) {
          idx_56.Add({true}, ins6.id);
        }
        vec47.Add({true});
      }
    }
  }
  for (auto [v66] : vec61) {
    if (table_12.TryChangePresentToAbsent({v66})) {
      if (table_22.TryChangePresentToUnknown({true, v66})) {
        ungated_33.TryChangePresentToUnknown({v66});
      }
      if (table_5.TryChangePresentToUnknown({v66, true})) {
        if (table_18.TryChangePresentToUnknown({true, v66})) {
          gated_30.TryChangePresentToUnknown({v66});
        }
      }
    }
  }
  vec38.Clear();
  vec39.Clear();
  vec60.Clear();
  vec61.Clear();
  flow_200(std::move(vec47));
  return false;
}

bool Database::find_44(int32_t v45) {
  switch (table_12.State({v45})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_12.TryChangeUnknownToAbsent({v45})) {
        if (find_113(v45)) {
          if (const auto ins7 = table_12.TryChangeAbsentToPresent({v45}); ins7.changed) {
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
  if (find_44(v45)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_53(bool v54) {
  switch (__9.State({v54})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (__9.TryChangeUnknownToAbsent({v54})) {
        if (find_144(v54)) {
          if (const auto ins8 = __9.TryChangeAbsentToPresent({v54}); ins8.changed) {
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
  if (find_53(v54)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::support_1(::hyde::rt::Vec<Tup_i32> vec73, ::hyde::rt::Vec<Tup_i32> vec74) {
  ::hyde::rt::Vec<Tup_i32> vec76(allocator);
  proc_36(std::move(vec73), std::move(vec74), std::move(vec76), std::move(vec76));
  return true;
}

bool Database::item_1(::hyde::rt::Vec<Tup_i32> vec78, ::hyde::rt::Vec<Tup_i32> vec79) {
  ::hyde::rt::Vec<Tup_i32> vec81(allocator);
  proc_36(std::move(vec81), std::move(vec81), std::move(vec78), std::move(vec79));
  return true;
}

bool Database::find_85(int32_t v86) {
  switch (gated_30.State({v86})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (gated_30.TryChangeUnknownToAbsent({v86})) {
        if (find_117(v86)) {
          if (const auto ins9 = gated_30.TryChangeAbsentToPresent({v86}); ins9.changed) {
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
  if (find_85(v86)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_87(int32_t v88) {
  switch (ungated_33.State({v88})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (ungated_33.TryChangeUnknownToAbsent({v88})) {
        if (find_89(v88)) {
          if (const auto ins10 = ungated_33.TryChangeAbsentToPresent({v88}); ins10.changed) {
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
  if (find_87(v88)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_89(int32_t v90) {
  if (find_93(v90)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_93(int32_t v94) {
  for (uint32_t s97 = idx_96.First({v94}); s97 != ::hyde::rt::kNoRow; s97 = idx_96.Next(s97)) {
    const auto r97 = table_22.RowAt(s97);
    const auto v98 = r97.c0;
    const auto v99 = r97.x;
    if (v99 == v94) {
      if (find_100(v98, v99)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_100(bool v101, int32_t v102) {
  switch (table_22.State({v101, v102})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_22.TryChangeUnknownToAbsent({v101, v102})) {
        if (find_104(v101, v102)) {
          if (const auto ins11 = table_22.TryChangeAbsentToPresent({v101, v102}); ins11.changed) {
            if (ins11.added_row) {
              idx_96.Add({v102}, ins11.id);
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
  if (find_100(v101, v102)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_104(bool v105, int32_t v106) {
  if (find_44(v106)) {
    if (find_53(v105)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_113(int32_t v114) {
  return false;
}

bool Database::find_117(int32_t v118) {
  if (find_121(v118)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_121(int32_t v122) {
  for (uint32_t s125 = idx_124.First({v122}); s125 != ::hyde::rt::kNoRow; s125 = idx_124.Next(s125)) {
    const auto r125 = table_18.RowAt(s125);
    const auto v126 = r125.c0;
    const auto v127 = r125.x;
    if (v127 == v122) {
      if (find_128(v126, v127)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_128(bool v129, int32_t v130) {
  switch (table_18.State({v129, v130})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_18.TryChangeUnknownToAbsent({v129, v130})) {
        if (find_132(v129, v130)) {
          if (const auto ins12 = table_18.TryChangeAbsentToPresent({v129, v130}); ins12.changed) {
            if (ins12.added_row) {
              idx_124.Add({v130}, ins12.id);
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
  if (find_128(v129, v130)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_132(bool v133, int32_t v134) {
  if (find_137(v134, v133)) {
    if (find_53(v133)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_137(int32_t v138, bool v139) {
  switch (table_5.State({v138, v139})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v138, v139})) {
        if (find_178(v138, v139)) {
          if (const auto ins13 = table_5.TryChangeAbsentToPresent({v138, v139}); ins13.changed) {
            if (ins13.added_row) {
              idx_56.Add({v139}, ins13.id);
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
  if (find_137(v138, v139)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_144(bool v145) {
  if (find_152(v145)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_152(bool v153) {
  for (uint32_t s155 = 0; s155 < table_26.NumRows(); ++s155) {
    const auto r155 = table_26.RowAt(s155);
    const auto v156 = r155.c0;
    const auto v157 = r155.s;
    if (v156 == v153) {
      if (find_158(v156, v157)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_158(bool v159, int32_t v160) {
  switch (table_26.State({v159, v160})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_26.TryChangeUnknownToAbsent({v159, v160})) {
        if (find_162(v159, v160)) {
          if (const auto ins14 = table_26.TryChangeAbsentToPresent({v159, v160}); ins14.changed) {
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
  if (find_158(v159, v160)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_162(bool v163, int32_t v164) {
  if (find_167(v164)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_167(int32_t v168) {
  switch (table_15.State({v168})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_15.TryChangeUnknownToAbsent({v168})) {
        if (find_113(v168)) {
          if (const auto ins15 = table_15.TryChangeAbsentToPresent({v168}); ins15.changed) {
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

bool Database::find_178(int32_t v179, bool v180) {
  if (find_44(v179)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::flow_200(::hyde::rt::Vec<Tup_b> vec47) {
  g37 += 1;
  vec47.SortAndUnique();
  for (auto [v68] : vec47) {
    if (const uint32_t j67_0 = __9.Find({v68}); j67_0 != ::hyde::rt::kNoRow) {
      const auto r67_0 = __9.RowAt(j67_0);
      const auto v70 = r67_0.c0;
      for (uint32_t j67_1 = idx_56.First({v68}); j67_1 != ::hyde::rt::kNoRow; j67_1 = idx_56.Next(j67_1)) {
        const auto r67_1 = table_5.RowAt(j67_1);
        const auto v71 = r67_1.x;
        const auto v69 = r67_1.c1;
        if (v68 == v69 && v68 == v70) {
          switch (table_5.State({v71, v69})) {
            case ::hyde::rt::TupleState::kPresent: {
              switch (__9.State({v69})) {
                case ::hyde::rt::TupleState::kPresent: {
                  if (const auto ins16 = table_18.TryChangeAbsentOrUnknownToPresent({v69, v71}); ins16.changed) {
                    if (ins16.added_row) {
                      idx_124.Add({v71}, ins16.id);
                    }
                    gated_30.TryChangeAbsentOrUnknownToPresent({v71});
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
  vec47.Clear();
  return true;
}

Database::gated_f_cursor Database::gated_f() {
  return {*this, 0};
}

bool Database::gated_f_cursor::next(int32_t &X) {
  while (pos < db.gated_30.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.gated_30.RowAt(id);
    if (!db.find_85(row.x)) {
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
  while (pos < db.ungated_33.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.ungated_33.RowAt(id);
    if (!db.find_87(row.x)) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

