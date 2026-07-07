// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      __5(allocator_),
      table_8(allocator_),
      out_12(allocator_),
      table_15(allocator_),
      table_18(allocator_),
      table_21(allocator_),
      idx_96(allocator_),
      table_25(allocator_),
      table_28(allocator_),
      idx_102(allocator_),
      table_32(allocator_),
      idx_108(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec137(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec138(allocator);
  ::hyde::rt::Vec<Tup_i32> vec139(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec140(allocator);
  proc_36(std::move(vec137), std::move(vec138), std::move(vec139), std::move(vec140));
  return false;
}

bool Database::proc_36(::hyde::rt::Vec<Tup_i32> vec38, ::hyde::rt::Vec<Tup_i32_i32> vec47, ::hyde::rt::Vec<Tup_i32> vec52, ::hyde::rt::Vec<Tup_i32_i32> vec56) {
  ::hyde::rt::Vec<Tup_i32> vec42(allocator);
  ::hyde::rt::Vec<Tup_i32> vec44(allocator);
  ::hyde::rt::Vec<Tup_i32> vec55(allocator);
  ::hyde::rt::Vec<Tup_i32> vec62(allocator);
  for (auto [v40] : vec38) {
    if (const auto ins0 = table_15.TryChangeAbsentToPresent({v40}); ins0.changed) {
      const uint64_t v46 = 0;
      (void) v46;
      vec42.Add({v40});
    }
  }
  for (auto [v49, v50] : vec47) {
    if (const auto ins1 = table_21.TryChangeAbsentToPresent({v49, v50}); ins1.changed) {
      if (ins1.added_row) {
        idx_96.Add({v49}, ins1.id);
      }
      const uint64_t v51 = 0;
      (void) v51;
      vec44.Add({v49});
    }
  }
  for (auto [v54] : vec52) {
    if (const auto ins2 = table_25.TryChangeAbsentToPresent({v54}); ins2.changed) {
      vec55.Add({v54});
    }
  }
  for (auto [v58, v59] : vec56) {
    if (const auto ins3 = table_28.TryChangeAbsentToPresent({v58, v59}); ins3.changed) {
      if (ins3.added_row) {
        idx_102.Add({v58}, ins3.id);
      }
      const uint64_t v66 = 0;
      (void) v66;
      vec62.Add({v58});
    }
  }
  vec38.Clear();
  vec47.Clear();
  vec52.Clear();
  vec56.Clear();
  flow_141(std::move(vec42), std::move(vec44), std::move(vec55), std::move(vec62));
  return false;
}

bool Database::base_1(::hyde::rt::Vec<Tup_i32> vec113) {
  ::hyde::rt::Vec<Tup_i32_i32> vec115(allocator);
  ::hyde::rt::Vec<Tup_i32> vec116(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec117(allocator);
  proc_36(std::move(vec113), std::move(vec115), std::move(vec116), std::move(vec117));
  return true;
}

bool Database::edge1_2(::hyde::rt::Vec<Tup_i32_i32> vec119) {
  ::hyde::rt::Vec<Tup_i32> vec121(allocator);
  ::hyde::rt::Vec<Tup_i32> vec122(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec123(allocator);
  proc_36(std::move(vec121), std::move(vec119), std::move(vec122), std::move(vec123));
  return true;
}

bool Database::marker_1(::hyde::rt::Vec<Tup_i32> vec125) {
  ::hyde::rt::Vec<Tup_i32> vec127(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec128(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec129(allocator);
  proc_36(std::move(vec127), std::move(vec128), std::move(vec125), std::move(vec129));
  return true;
}

bool Database::edge2_2(::hyde::rt::Vec<Tup_i32_i32> vec131) {
  ::hyde::rt::Vec<Tup_i32> vec133(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec134(allocator);
  ::hyde::rt::Vec<Tup_i32> vec135(allocator);
  proc_36(std::move(vec133), std::move(vec134), std::move(vec135), std::move(vec131));
  return true;
}

bool Database::flow_141(::hyde::rt::Vec<Tup_i32> vec42, ::hyde::rt::Vec<Tup_i32> vec44, ::hyde::rt::Vec<Tup_i32> vec55, ::hyde::rt::Vec<Tup_i32> vec62) {
  ::hyde::rt::Vec<Tup_i32> vec43(allocator);
  ::hyde::rt::Vec<Tup_i32> vec45(allocator);
  ::hyde::rt::Vec<Tup_i32> vec60(allocator);
  ::hyde::rt::Vec<Tup_i32> vec61(allocator);
  ::hyde::rt::Vec<Tup_i32> vec63(allocator);
  ::hyde::rt::Vec<Tup_b> vec64(allocator);
  ::hyde::rt::Vec<Tup_b> vec65(allocator);
  ::hyde::rt::Vec<Tup_i32> vec84(allocator);
  // set 1 depth 1
  if ((g37 += 1) == 1) {
  }
  vec55.SortAndUnique();
  for (auto [v68] : vec55) {
    if (const uint32_t j67_0 = table_15.Find({v68}); j67_0 != ::hyde::rt::kNoRow) {
      const auto r67_0 = table_15.RowAt(j67_0);
      const auto v69 = r67_0.a;
      if (const uint32_t j67_1 = table_25.Find({v68}); j67_1 != ::hyde::rt::kNoRow) {
        const auto r67_1 = table_25.RowAt(j67_1);
        const auto v70 = r67_1.m;
        if (v68 == v69 && v68 == v70) {
          if (const auto ins4 = table_8.TryChangeAbsentToPresent({true, v69}); ins4.changed) {
            if (const auto ins5 = __5.TryChangeAbsentToPresent({true}); ins5.changed) {
              const uint64_t v71 = 0;
              (void) v71;
              vec64.Add({true});
            }
          }
        }
      }
    }
  }
  vec55.Clear();
  for (bool reenter41 = true; reenter41; ) {
    for (bool changed41 = true; changed41; changed41 = !(true && vec42.Empty() && vec44.Empty() && vec60.Empty() && vec62.Empty() && vec64.Empty())) {
      vec43.Clear();
      vec42.SortAndUnique();
      vec42.Swap(vec43);
      for (auto [v73] : vec43) {
        if (const auto ins6 = table_18.TryChangeAbsentToPresent({v73}); ins6.changed) {
          const uint64_t v82 = 0;
          (void) v82;
          vec60.Add({v73});
        }
        const uint64_t v83 = 0;
        (void) v83;
        vec44.Add({v73});
        vec84.Add({v73});
      }
      vec45.Clear();
      vec44.SortAndUnique();
      vec44.Swap(vec45);
      for (auto [v95] : vec45) {
        if (const uint32_t j94_0 = table_15.Find({v95}); j94_0 != ::hyde::rt::kNoRow) {
          const auto r94_0 = table_15.RowAt(j94_0);
          const auto v97 = r94_0.a;
          for (uint32_t j94_1 = idx_96.First({v95}); j94_1 != ::hyde::rt::kNoRow; j94_1 = idx_96.Next(j94_1)) {
            const auto r94_1 = table_21.RowAt(j94_1);
            const auto v98 = r94_1.a;
            const auto v99 = r94_1.b;
            if (v95 == v97 && v95 == v98) {
              const auto v74 = v98;
              const auto v75 = v99;
              if (const auto ins7 = table_15.TryChangeAbsentToPresent({v75}); ins7.changed) {
                const uint64_t v85 = 0;
                (void) v85;
                vec42.Add({v75});
              }
            }
          }
        }
      }
      vec45.Clear();
      vec61.Clear();
      vec60.SortAndUnique();
      vec60.Swap(vec61);
      for (auto [v77] : vec61) {
        if (const auto ins8 = out_12.TryChangeAbsentToPresent({v77}); ins8.changed) {
        }
        const uint64_t v86 = 0;
        (void) v86;
        vec62.Add({v77});
      }
      vec63.Clear();
      vec62.SortAndUnique();
      vec62.Swap(vec63);
      for (auto [v101] : vec63) {
        if (const uint32_t j100_0 = table_18.Find({v101}); j100_0 != ::hyde::rt::kNoRow) {
          const auto r100_0 = table_18.RowAt(j100_0);
          const auto v103 = r100_0.a;
          for (uint32_t j100_1 = idx_102.First({v101}); j100_1 != ::hyde::rt::kNoRow; j100_1 = idx_102.Next(j100_1)) {
            const auto r100_1 = table_28.RowAt(j100_1);
            const auto v104 = r100_1.a;
            const auto v105 = r100_1.b;
            if (v101 == v103 && v101 == v104) {
              const auto v78 = v104;
              const auto v79 = v105;
              if (const auto ins9 = table_32.TryChangeAbsentToPresent({v79, true}); ins9.changed) {
                if (ins9.added_row) {
                  idx_108.Add({true}, ins9.id);
                }
                const uint64_t v87 = 0;
                (void) v87;
                vec64.Add({true});
              }
            }
          }
        }
      }
      vec63.Clear();
      vec65.Clear();
      vec64.SortAndUnique();
      vec64.Swap(vec65);
      for (auto [v107] : vec65) {
        if (const uint32_t j106_0 = __5.Find({v107}); j106_0 != ::hyde::rt::kNoRow) {
          const auto r106_0 = __5.RowAt(j106_0);
          const auto v110 = r106_0.c0;
          for (uint32_t j106_1 = idx_108.First({v107}); j106_1 != ::hyde::rt::kNoRow; j106_1 = idx_108.Next(j106_1)) {
            const auto r106_1 = table_32.RowAt(j106_1);
            const auto v111 = r106_1.b;
            const auto v109 = r106_1.c1;
            if (v107 == v109 && v107 == v110) {
              const auto v80 = v109;
              const auto v81 = v111;
              if (const auto ins10 = table_18.TryChangeAbsentToPresent({v81}); ins10.changed) {
                const uint64_t v88 = 0;
                (void) v88;
                vec60.Add({v81});
              }
            }
          }
        }
      }
      vec65.Clear();
      vec84.SortAndUnique();
      for (auto [v90] : vec84) {
        if (const uint32_t j89_0 = table_15.Find({v90}); j89_0 != ::hyde::rt::kNoRow) {
          const auto r89_0 = table_15.RowAt(j89_0);
          const auto v91 = r89_0.a;
          if (const uint32_t j89_1 = table_25.Find({v90}); j89_1 != ::hyde::rt::kNoRow) {
            const auto r89_1 = table_25.RowAt(j89_1);
            const auto v92 = r89_1.m;
            if (v90 == v91 && v90 == v92) {
              if (const auto ins11 = table_8.TryChangeAbsentToPresent({true, v91}); ins11.changed) {
                if (const auto ins12 = __5.TryChangeAbsentToPresent({true}); ins12.changed) {
                  const uint64_t v93 = 0;
                  (void) v93;
                  vec64.Add({true});
                }
              }
            }
          }
        }
      }
      vec84.Clear();
    }
    vec42.Clear();
    vec43.Clear();
    vec44.Clear();
    vec45.Clear();
    vec60.Clear();
    vec61.Clear();
    vec62.Clear();
    vec63.Clear();
    vec64.Clear();
    vec65.Clear();
    vec60.Clear();
    vec64.Clear();
    vec62.Clear();
    vec44.Clear();
    vec42.Clear();
    vec61.Clear();
    vec65.Clear();
    vec63.Clear();
    vec45.Clear();
    vec43.Clear();
    reenter41 = !(true && vec42.Empty() && vec44.Empty() && vec60.Empty() && vec62.Empty() && vec64.Empty());
  }
  return true;
}

bool Database::out_b(int32_t A) {
  return out_12.State({A}) == ::hyde::rt::TupleState::kPresent;
}

