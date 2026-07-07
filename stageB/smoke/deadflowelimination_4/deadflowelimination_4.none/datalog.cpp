// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      table_5(allocator_),
      idx_111(allocator_),
      table_9(allocator_),
      out_neg_12(allocator_),
      __15(allocator_),
      table_18(allocator_),
      __22(allocator_),
      table_25(allocator_),
      out_pos_29(allocator_),
      out_chain_32(allocator_),
      table_35(allocator_),
      table_38(allocator_),
      idx_73(allocator_),
      table_42(allocator_),
      idx_67(allocator_),
      table_46(allocator_),
      idx_79(allocator_),
      table_50(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec87(allocator);
  proc_54(std::move(vec87));
  return false;
}

bool Database::proc_54(::hyde::rt::Vec<Tup_i32> vec56) {
  ::hyde::rt::Vec<Tup_b> vec59(allocator);
  ::hyde::rt::Vec<Tup_b> vec63(allocator);
  ::hyde::rt::Vec<Tup_b> vec64(allocator);
  for (auto [v58] : vec56) {
    if (const auto ins0 = table_38.TryChangeAbsentToPresent({v58, true}); ins0.changed) {
      if (ins0.added_row) {
        idx_73.Add({true}, ins0.id);
      }
      vec59.Add({true});
    }
    if (const auto ins1 = table_50.TryChangeAbsentToPresent({v58, true}); ins1.changed) {
      if (!find_60(true)) {
        if (const auto ins2 = table_5.TryChangeAbsentOrUnknownToPresent({true, v58}); ins2.changed) {
          if (ins2.added_row) {
            idx_111.Add({v58}, ins2.id);
          }
          if (const auto ins3 = table_9.TryChangeAbsentOrUnknownToPresent({v58}); ins3.changed) {
            if (const auto ins4 = out_neg_12.TryChangeAbsentOrUnknownToPresent({v58}); ins4.changed) {
            }
          }
        }
      }
    }
    if (const auto ins5 = table_42.TryChangeAbsentToPresent({v58, true}); ins5.changed) {
      if (ins5.added_row) {
        idx_67.Add({true}, ins5.id);
      }
      vec63.Add({true});
    }
    if (const auto ins6 = table_46.TryChangeAbsentToPresent({v58, true}); ins6.changed) {
      if (ins6.added_row) {
        idx_79.Add({true}, ins6.id);
      }
      vec64.Add({true});
    }
  }
  vec56.Clear();
  flow_129(std::move(vec59), std::move(vec63), std::move(vec64));
  return false;
}

bool Database::find_60(bool v61) {
  switch (__15.State({v61})) {
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

bool Database::in_1(::hyde::rt::Vec<Tup_i32> vec84) {
  proc_54(std::move(vec84));
  return true;
}

bool Database::find_88(int32_t v89) {
  switch (out_neg_12.State({v89})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (out_neg_12.TryChangeUnknownToAbsent({v89})) {
        if (find_90(v89)) {
          if (const auto ins7 = out_neg_12.TryChangeAbsentToPresent({v89}); ins7.changed) {
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
  if (find_88(v89)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_90(int32_t v91) {
  // /Users/orangesloth/Code/DrLojekyll/lib/ControlFlow/Build/Union.cpp: BuildTopDownUnionChecker call differential predecessor
  if (find_94(v91)) {
    return true;
  }
  return false;
}

bool Database::find_94(int32_t v95) {
  switch (table_9.State({v95})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_9.TryChangeUnknownToAbsent({v95})) {
        if (find_97(v95)) {
          if (const auto ins8 = table_9.TryChangeAbsentToPresent({v95}); ins8.changed) {
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
  if (find_100(v95)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_97(int32_t v98) {
  if (find_105(v98)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_100(int32_t v101) {
  switch (table_9.State({v101})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_9.TryChangeUnknownToAbsent({v101})) {
        if (find_97(v101)) {
          if (const auto ins9 = table_9.TryChangeAbsentToPresent({v101}); ins9.changed) {
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
  if (find_100(v101)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_105(int32_t v106) {
  if (find_108(v106)) {
    return true;
  } else {
    return false;
  }
  return false;
}

bool Database::find_108(int32_t v109) {
  for (uint32_t s112 = idx_111.First({v109}); s112 != ::hyde::rt::kNoRow; s112 = idx_111.Next(s112)) {
    const auto r112 = table_5.RowAt(s112);
    const auto v113 = r112.c0;
    const auto v114 = r112.a;
    if (v109 == v114) {
      if (find_115(v113, v114)) {
        return true;
      }
    }
  }
  return false;
}

bool Database::find_115(bool v116, int32_t v117) {
  switch (table_5.State({v116, v117})) {
    case ::hyde::rt::TupleState::kPresent: {
      return true;
      break;
    }
    case ::hyde::rt::TupleState::kAbsent: {
      return false;
      break;
    }
    case ::hyde::rt::TupleState::kUnknown: {
      if (table_5.TryChangeUnknownToAbsent({v116, v117})) {
        if (find_119(v116, v117)) {
          if (const auto ins10 = table_5.TryChangeAbsentToPresent({v116, v117}); ins10.changed) {
            if (ins10.added_row) {
              idx_111.Add({v117}, ins10.id);
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
  if (find_115(v116, v117)) {
    return true;
  } else {
    return true;
  }
  return false;
}

bool Database::find_119(bool v120, int32_t v121) {
  if (find_124(v121, v120)) {
    if (find_60(v120)) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  return false;
}

bool Database::find_124(int32_t v125, bool v126) {
  switch (table_50.State({v125, v126})) {
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

bool Database::flow_129(::hyde::rt::Vec<Tup_b> vec59, ::hyde::rt::Vec<Tup_b> vec63, ::hyde::rt::Vec<Tup_b> vec64) {
  if ((g55 += 1) == 1) {
  }
  vec63.SortAndUnique();
  for (auto [v66] : vec63) {
    if (const uint32_t j65_0 = __15.Find({v66}); j65_0 != ::hyde::rt::kNoRow) {
      const auto r65_0 = __15.RowAt(j65_0);
      const auto v69 = r65_0.c0;
      for (uint32_t j65_1 = idx_67.First({v66}); j65_1 != ::hyde::rt::kNoRow; j65_1 = idx_67.Next(j65_1)) {
        const auto r65_1 = table_42.RowAt(j65_1);
        const auto v70 = r65_1.c0;
        const auto v68 = r65_1.c1;
        if (v66 == v68 && v66 == v69) {
          if (const auto ins11 = table_25.TryChangeAbsentToPresent({true, v70}); ins11.changed) {
            if (const auto ins12 = __22.TryChangeAbsentToPresent({true}); ins12.changed) {
              vec64.Add({true});
            }
          }
        }
      }
    }
  }
  vec63.Clear();
  vec59.SortAndUnique();
  for (auto [v72] : vec59) {
    if (const uint32_t j71_0 = __15.Find({v72}); j71_0 != ::hyde::rt::kNoRow) {
      const auto r71_0 = __15.RowAt(j71_0);
      const auto v75 = r71_0.c0;
      for (uint32_t j71_1 = idx_73.First({v72}); j71_1 != ::hyde::rt::kNoRow; j71_1 = idx_73.Next(j71_1)) {
        const auto r71_1 = table_38.RowAt(j71_1);
        const auto v76 = r71_1.a;
        const auto v74 = r71_1.c1;
        if (v72 == v74 && v72 == v75) {
          if (const auto ins13 = out_pos_29.TryChangeAbsentToPresent({v76}); ins13.changed) {
          }
        }
      }
    }
  }
  vec59.Clear();
  vec64.SortAndUnique();
  for (auto [v78] : vec64) {
    if (const uint32_t j77_0 = __22.Find({v78}); j77_0 != ::hyde::rt::kNoRow) {
      const auto r77_0 = __22.RowAt(j77_0);
      const auto v81 = r77_0.c0;
      for (uint32_t j77_1 = idx_79.First({v78}); j77_1 != ::hyde::rt::kNoRow; j77_1 = idx_79.Next(j77_1)) {
        const auto r77_1 = table_46.RowAt(j77_1);
        const auto v82 = r77_1.a;
        const auto v80 = r77_1.c1;
        if (v78 == v80 && v78 == v81) {
          if (const auto ins14 = out_chain_32.TryChangeAbsentToPresent({v82}); ins14.changed) {
          }
        }
      }
    }
  }
  vec64.Clear();
  return true;
}

Database::out_pos_f_cursor Database::out_pos_f() {
  return {*this, 0};
}

bool Database::out_pos_f_cursor::next(int32_t &A) {
  while (pos < db.out_pos_29.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out_pos_29.RowAt(id);
    if (db.out_pos_29.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    A = row.a;
    return true;
  }
  return false;
}

Database::out_neg_f_cursor Database::out_neg_f() {
  return {*this, 0};
}

bool Database::out_neg_f_cursor::next(int32_t &A) {
  while (pos < db.out_neg_12.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out_neg_12.RowAt(id);
    if (!db.find_88(row.a)) {
      continue;
    }
    A = row.a;
    return true;
  }
  return false;
}

Database::out_chain_f_cursor Database::out_chain_f() {
  return {*this, 0};
}

bool Database::out_chain_f_cursor::next(int32_t &A) {
  while (pos < db.out_chain_32.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.out_chain_32.RowAt(id);
    if (db.out_chain_32.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    A = row.a;
    return true;
  }
  return false;
}

