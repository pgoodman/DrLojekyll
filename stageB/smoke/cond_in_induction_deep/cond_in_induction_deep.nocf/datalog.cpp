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
      idx_88(allocator_),
      table_22(allocator_),
      table_25(allocator_),
      idx_106(allocator_),
      table_29(allocator_),
      idx_112(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec141(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec142(allocator);
  ::hyde::rt::Vec<Tup_i32> vec143(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec144(allocator);
  proc_33(std::move(vec141), std::move(vec142), std::move(vec143), std::move(vec144));
  return false;
}

bool Database::proc_33(::hyde::rt::Vec<Tup_i32> vec35, ::hyde::rt::Vec<Tup_i32_i32> vec45, ::hyde::rt::Vec<Tup_i32> vec50, ::hyde::rt::Vec<Tup_i32_i32> vec54) {
  ::hyde::rt::Vec<Tup_i32> vec39(allocator);
  ::hyde::rt::Vec<Tup_i32> vec42(allocator);
  ::hyde::rt::Vec<Tup_i32> vec53(allocator);
  ::hyde::rt::Vec<Tup_i32> vec59(allocator);
  for (auto [v37] : vec35) {
    if (const auto ins0 = table_15.TryChangeAbsentToPresent({v37}); ins0.changed) {
      const uint64_t v44 = 0;
      (void) v44;
      vec39.Add({v37});
    }
  }
  for (auto [v47, v48] : vec45) {
    if (const auto ins1 = table_18.TryChangeAbsentToPresent({v47, v48}); ins1.changed) {
      if (ins1.added_row) {
        idx_88.Add({v47}, ins1.id);
      }
      const uint64_t v49 = 0;
      (void) v49;
      vec42.Add({v47});
    }
  }
  for (auto [v52] : vec50) {
    if (const auto ins2 = table_22.TryChangeAbsentToPresent({v52}); ins2.changed) {
      vec53.Add({v52});
    }
  }
  for (auto [v56, v57] : vec54) {
    if (const auto ins3 = table_25.TryChangeAbsentToPresent({v56, v57}); ins3.changed) {
      if (ins3.added_row) {
        idx_106.Add({v56}, ins3.id);
      }
      const uint64_t v66 = 0;
      (void) v66;
      vec59.Add({v56});
    }
  }
  vec35.Clear();
  vec45.Clear();
  vec50.Clear();
  vec54.Clear();
  flow_145(std::move(vec39), std::move(vec42), std::move(vec53), std::move(vec59));
  return false;
}

bool Database::base_1(::hyde::rt::Vec<Tup_i32> vec117) {
  ::hyde::rt::Vec<Tup_i32_i32> vec119(allocator);
  ::hyde::rt::Vec<Tup_i32> vec120(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec121(allocator);
  proc_33(std::move(vec117), std::move(vec119), std::move(vec120), std::move(vec121));
  return true;
}

bool Database::edge1_2(::hyde::rt::Vec<Tup_i32_i32> vec123) {
  ::hyde::rt::Vec<Tup_i32> vec125(allocator);
  ::hyde::rt::Vec<Tup_i32> vec126(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec127(allocator);
  proc_33(std::move(vec125), std::move(vec123), std::move(vec126), std::move(vec127));
  return true;
}

bool Database::marker_1(::hyde::rt::Vec<Tup_i32> vec129) {
  ::hyde::rt::Vec<Tup_i32> vec131(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec132(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec133(allocator);
  proc_33(std::move(vec131), std::move(vec132), std::move(vec129), std::move(vec133));
  return true;
}

bool Database::edge2_2(::hyde::rt::Vec<Tup_i32_i32> vec135) {
  ::hyde::rt::Vec<Tup_i32> vec137(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec138(allocator);
  ::hyde::rt::Vec<Tup_i32> vec139(allocator);
  proc_33(std::move(vec137), std::move(vec138), std::move(vec139), std::move(vec135));
  return true;
}

bool Database::flow_145(::hyde::rt::Vec<Tup_i32> vec39, ::hyde::rt::Vec<Tup_i32> vec42, ::hyde::rt::Vec<Tup_i32> vec53, ::hyde::rt::Vec<Tup_i32> vec59) {
  ::hyde::rt::Vec<Tup_i32> vec40(allocator);
  ::hyde::rt::Vec<Tup_i32> vec41(allocator);
  ::hyde::rt::Vec<Tup_i32> vec43(allocator);
  ::hyde::rt::Vec<Tup_i32> vec60(allocator);
  ::hyde::rt::Vec<Tup_i32> vec61(allocator);
  ::hyde::rt::Vec<Tup_i32> vec62(allocator);
  ::hyde::rt::Vec<Tup_i32> vec63(allocator);
  ::hyde::rt::Vec<Tup_b> vec64(allocator);
  ::hyde::rt::Vec<Tup_b> vec65(allocator);
  ::hyde::rt::Vec<Tup_i32> vec79(allocator);
  // set 1 depth 2
  // set 0 depth 1
  if ((g34 += 1) == 1) {
  }
  vec53.SortAndUnique();
  for (auto [v68] : vec53) {
    if (const uint32_t j67_0 = table_15.Find({v68}); j67_0 != ::hyde::rt::kNoRow) {
      const auto r67_0 = table_15.RowAt(j67_0);
      const auto v69 = r67_0.a;
      if (const uint32_t j67_1 = table_22.Find({v68}); j67_1 != ::hyde::rt::kNoRow) {
        const auto r67_1 = table_22.RowAt(j67_1);
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
  vec53.Clear();
  for (bool reenter38 = true; reenter38; ) {
    for (bool changed38 = true; changed38; changed38 = !(true && vec39.Empty() && vec42.Empty())) {
      vec40.Clear();
      vec39.SortAndUnique();
      vec39.Swap(vec40);
      for (auto [v73] : vec40) {
        vec41.Add({v73});
        const uint64_t v78 = 0;
        (void) v78;
        vec42.Add({v73});
        vec79.Add({v73});
      }
      vec43.Clear();
      vec42.SortAndUnique();
      vec42.Swap(vec43);
      for (auto [v87] : vec43) {
        if (const uint32_t j86_0 = table_15.Find({v87}); j86_0 != ::hyde::rt::kNoRow) {
          const auto r86_0 = table_15.RowAt(j86_0);
          const auto v89 = r86_0.a;
          for (uint32_t j86_1 = idx_88.First({v87}); j86_1 != ::hyde::rt::kNoRow; j86_1 = idx_88.Next(j86_1)) {
            const auto r86_1 = table_18.RowAt(j86_1);
            const auto v90 = r86_1.a;
            const auto v91 = r86_1.b;
            if (v87 == v89 && v87 == v90) {
              const auto v76 = v90;
              const auto v77 = v91;
              if (const auto ins6 = table_15.TryChangeAbsentToPresent({v77}); ins6.changed) {
                const uint64_t v80 = 0;
                (void) v80;
                vec39.Add({v77});
              }
            }
          }
        }
      }
      vec43.Clear();
      vec79.SortAndUnique();
      for (auto [v82] : vec79) {
        if (const uint32_t j81_0 = table_15.Find({v82}); j81_0 != ::hyde::rt::kNoRow) {
          const auto r81_0 = table_15.RowAt(j81_0);
          const auto v83 = r81_0.a;
          if (const uint32_t j81_1 = table_22.Find({v82}); j81_1 != ::hyde::rt::kNoRow) {
            const auto r81_1 = table_22.RowAt(j81_1);
            const auto v84 = r81_1.m;
            if (v82 == v83 && v82 == v84) {
              if (const auto ins7 = table_8.TryChangeAbsentToPresent({true, v83}); ins7.changed) {
                if (const auto ins8 = __5.TryChangeAbsentToPresent({true}); ins8.changed) {
                  const uint64_t v85 = 0;
                  (void) v85;
                  vec64.Add({true});
                }
              }
            }
          }
        }
      }
      vec79.Clear();
    }
    vec39.Clear();
    vec40.Clear();
    vec41.SortAndUnique();
    for (auto [v75] : vec41) {
      if (const auto ins9 = out_12.TryChangeAbsentToPresent({v75}); ins9.changed) {
        const uint64_t v92 = 0;
        (void) v92;
        vec61.Add({v75});
      }
    }
    vec42.Clear();
    vec43.Clear();
    vec42.Clear();
    vec39.Clear();
    vec43.Clear();
    vec40.Clear();
    vec41.Clear();
    reenter38 = !(true && vec39.Empty() && vec42.Empty());
  }
  for (bool reenter58 = true; reenter58; ) {
    for (bool changed58 = true; changed58; changed58 = !(true && vec59.Empty() && vec61.Empty() && vec64.Empty())) {
      vec60.Clear();
      vec59.SortAndUnique();
      vec59.Swap(vec60);
      for (auto [v105] : vec60) {
        if (const uint32_t j104_0 = out_12.Find({v105}); j104_0 != ::hyde::rt::kNoRow) {
          const auto r104_0 = out_12.RowAt(j104_0);
          const auto v107 = r104_0.a;
          for (uint32_t j104_1 = idx_106.First({v105}); j104_1 != ::hyde::rt::kNoRow; j104_1 = idx_106.Next(j104_1)) {
            const auto r104_1 = table_25.RowAt(j104_1);
            const auto v108 = r104_1.a;
            const auto v109 = r104_1.b;
            if (v105 == v107 && v105 == v108) {
              const auto v93 = v108;
              const auto v94 = v109;
              if (const auto ins10 = table_29.TryChangeAbsentToPresent({v94, true}); ins10.changed) {
                if (ins10.added_row) {
                  idx_112.Add({true}, ins10.id);
                }
                const uint64_t v101 = 0;
                (void) v101;
                vec64.Add({true});
              }
            }
          }
        }
      }
      vec60.Clear();
      vec62.Clear();
      vec61.SortAndUnique();
      vec61.Swap(vec62);
      for (auto [v96] : vec62) {
        vec63.Add({v96});
        const uint64_t v102 = 0;
        (void) v102;
        vec59.Add({v96});
      }
      vec65.Clear();
      vec64.SortAndUnique();
      vec64.Swap(vec65);
      for (auto [v111] : vec65) {
        if (const uint32_t j110_0 = __5.Find({v111}); j110_0 != ::hyde::rt::kNoRow) {
          const auto r110_0 = __5.RowAt(j110_0);
          const auto v114 = r110_0.c0;
          for (uint32_t j110_1 = idx_112.First({v111}); j110_1 != ::hyde::rt::kNoRow; j110_1 = idx_112.Next(j110_1)) {
            const auto r110_1 = table_29.RowAt(j110_1);
            const auto v115 = r110_1.b;
            const auto v113 = r110_1.c1;
            if (v111 == v113 && v111 == v114) {
              const auto v99 = v113;
              const auto v100 = v115;
              if (const auto ins11 = out_12.TryChangeAbsentToPresent({v100}); ins11.changed) {
                const uint64_t v103 = 0;
                (void) v103;
                vec61.Add({v100});
              }
            }
          }
        }
      }
      vec65.Clear();
    }
    vec59.Clear();
    vec60.Clear();
    vec61.Clear();
    vec62.Clear();
    vec63.SortAndUnique();
    for (auto [v98] : vec63) {
    }
    vec64.Clear();
    vec65.Clear();
    vec61.Clear();
    vec64.Clear();
    vec59.Clear();
    vec62.Clear();
    vec65.Clear();
    vec60.Clear();
    vec63.Clear();
    reenter58 = !(true && vec59.Empty() && vec61.Empty() && vec64.Empty());
  }
  return true;
}

bool Database::out_b(int32_t A) {
  return out_12.State({A}) == ::hyde::rt::TupleState::kPresent;
}

