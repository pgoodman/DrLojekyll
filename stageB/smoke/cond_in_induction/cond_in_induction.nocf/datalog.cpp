// Auto-generated file; do not edit.

#include "datalog.h"

#include <tuple>
#include <utility>

Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_)
    : allocator(allocator_),
      log(log_),
      functors(functors_),
      c1_5(allocator_),
      table_8(allocator_),
      c2_12(allocator_),
      table_15(allocator_),
      c3_19(allocator_),
      table_22(allocator_),
      t1_out_26(allocator_),
      t2_out_29(allocator_),
      t3_out_32(allocator_),
      table_35(allocator_),
      idx_169(allocator_),
      table_39(allocator_),
      idx_187(allocator_),
      table_43(allocator_),
      idx_163(allocator_),
      table_47(allocator_),
      idx_181(allocator_),
      table_51(allocator_),
      idx_157(allocator_),
      table_55(allocator_),
      idx_175(allocator_) {
  init_4();
}

bool Database::init_4() {
  ::hyde::rt::Vec<Tup_i32> vec291(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec292(allocator);
  ::hyde::rt::Vec<Tup_i32> vec293(allocator);
  ::hyde::rt::Vec<Tup_i32> vec294(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec295(allocator);
  ::hyde::rt::Vec<Tup_i32> vec296(allocator);
  ::hyde::rt::Vec<Tup_i32> vec297(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec298(allocator);
  ::hyde::rt::Vec<Tup_i32> vec299(allocator);
  proc_59(std::move(vec291), std::move(vec292), std::move(vec293), std::move(vec294), std::move(vec295), std::move(vec296), std::move(vec297), std::move(vec298), std::move(vec299));
  return false;
}

bool Database::proc_59(::hyde::rt::Vec<Tup_i32> vec61, ::hyde::rt::Vec<Tup_i32_i32> vec73, ::hyde::rt::Vec<Tup_i32> vec78, ::hyde::rt::Vec<Tup_i32> vec82, ::hyde::rt::Vec<Tup_i32_i32> vec93, ::hyde::rt::Vec<Tup_i32> vec98, ::hyde::rt::Vec<Tup_i32> vec102, ::hyde::rt::Vec<Tup_i32_i32> vec113, ::hyde::rt::Vec<Tup_i32> vec118) {
  ::hyde::rt::Vec<Tup_i32> vec65(allocator);
  ::hyde::rt::Vec<Tup_b> vec68(allocator);
  ::hyde::rt::Vec<Tup_i32> vec70(allocator);
  ::hyde::rt::Vec<Tup_i32> vec85(allocator);
  ::hyde::rt::Vec<Tup_i32> vec88(allocator);
  ::hyde::rt::Vec<Tup_b> vec90(allocator);
  ::hyde::rt::Vec<Tup_i32> vec105(allocator);
  ::hyde::rt::Vec<Tup_i32> vec108(allocator);
  ::hyde::rt::Vec<Tup_b> vec110(allocator);
  for (auto [v63] : vec61) {
    if (const auto ins0 = t1_out_26.TryChangeAbsentToPresent({v63}); ins0.changed) {
      const uint64_t v72 = 0;
      (void) v72;
      vec65.Add({v63});
    }
  }
  for (auto [v75, v76] : vec73) {
    if (const auto ins1 = table_35.TryChangeAbsentToPresent({v75, v76}); ins1.changed) {
      if (ins1.added_row) {
        idx_169.Add({v75}, ins1.id);
      }
      const uint64_t v77 = 0;
      (void) v77;
      vec70.Add({v75});
    }
  }
  for (auto [v80] : vec78) {
    if (const auto ins2 = table_8.TryChangeAbsentToPresent({true, v80}); ins2.changed) {
      if (const auto ins3 = c1_5.TryChangeAbsentToPresent({true}); ins3.changed) {
        const uint64_t v81 = 0;
        (void) v81;
        vec68.Add({true});
      }
    }
  }
  for (auto [v84] : vec82) {
    if (const auto ins4 = t2_out_29.TryChangeAbsentToPresent({v84}); ins4.changed) {
      const uint64_t v92 = 0;
      (void) v92;
      vec85.Add({v84});
    }
  }
  for (auto [v95, v96] : vec93) {
    if (const auto ins5 = table_43.TryChangeAbsentToPresent({v95, v96}); ins5.changed) {
      if (ins5.added_row) {
        idx_163.Add({v95}, ins5.id);
      }
      const uint64_t v97 = 0;
      (void) v97;
      vec88.Add({v95});
    }
  }
  for (auto [v100] : vec98) {
    if (const auto ins6 = table_15.TryChangeAbsentToPresent({true, v100}); ins6.changed) {
      if (const auto ins7 = c2_12.TryChangeAbsentToPresent({true}); ins7.changed) {
        const uint64_t v101 = 0;
        (void) v101;
        vec90.Add({true});
      }
    }
  }
  for (auto [v104] : vec102) {
    if (const auto ins8 = t3_out_32.TryChangeAbsentToPresent({v104}); ins8.changed) {
      const uint64_t v112 = 0;
      (void) v112;
      vec105.Add({v104});
    }
  }
  for (auto [v115, v116] : vec113) {
    if (const auto ins9 = table_51.TryChangeAbsentToPresent({v115, v116}); ins9.changed) {
      if (ins9.added_row) {
        idx_157.Add({v115}, ins9.id);
      }
      const uint64_t v117 = 0;
      (void) v117;
      vec108.Add({v115});
    }
  }
  for (auto [v120] : vec118) {
    if (const auto ins10 = table_22.TryChangeAbsentToPresent({true, v120}); ins10.changed) {
      if (const auto ins11 = c3_19.TryChangeAbsentToPresent({true}); ins11.changed) {
        const uint64_t v121 = 0;
        (void) v121;
        vec110.Add({true});
      }
    }
  }
  vec61.Clear();
  vec73.Clear();
  vec78.Clear();
  vec82.Clear();
  vec93.Clear();
  vec98.Clear();
  vec102.Clear();
  vec113.Clear();
  vec118.Clear();
  flow_300(std::move(vec65), std::move(vec68), std::move(vec70), std::move(vec85), std::move(vec88), std::move(vec90), std::move(vec105), std::move(vec108), std::move(vec110));
  return false;
}

bool Database::m1_1(::hyde::rt::Vec<Tup_i32> vec192) {
  ::hyde::rt::Vec<Tup_i32_i32> vec194(allocator);
  ::hyde::rt::Vec<Tup_i32> vec195(allocator);
  ::hyde::rt::Vec<Tup_i32> vec196(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec197(allocator);
  ::hyde::rt::Vec<Tup_i32> vec198(allocator);
  ::hyde::rt::Vec<Tup_i32> vec199(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec200(allocator);
  ::hyde::rt::Vec<Tup_i32> vec201(allocator);
  proc_59(std::move(vec192), std::move(vec194), std::move(vec195), std::move(vec196), std::move(vec197), std::move(vec198), std::move(vec199), std::move(vec200), std::move(vec201));
  return true;
}

bool Database::step1_2(::hyde::rt::Vec<Tup_i32_i32> vec203) {
  ::hyde::rt::Vec<Tup_i32> vec205(allocator);
  ::hyde::rt::Vec<Tup_i32> vec206(allocator);
  ::hyde::rt::Vec<Tup_i32> vec207(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec208(allocator);
  ::hyde::rt::Vec<Tup_i32> vec209(allocator);
  ::hyde::rt::Vec<Tup_i32> vec210(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec211(allocator);
  ::hyde::rt::Vec<Tup_i32> vec212(allocator);
  proc_59(std::move(vec205), std::move(vec203), std::move(vec206), std::move(vec207), std::move(vec208), std::move(vec209), std::move(vec210), std::move(vec211), std::move(vec212));
  return true;
}

bool Database::seed1_1(::hyde::rt::Vec<Tup_i32> vec214) {
  ::hyde::rt::Vec<Tup_i32> vec216(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec217(allocator);
  ::hyde::rt::Vec<Tup_i32> vec218(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec219(allocator);
  ::hyde::rt::Vec<Tup_i32> vec220(allocator);
  ::hyde::rt::Vec<Tup_i32> vec221(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec222(allocator);
  ::hyde::rt::Vec<Tup_i32> vec223(allocator);
  proc_59(std::move(vec216), std::move(vec217), std::move(vec214), std::move(vec218), std::move(vec219), std::move(vec220), std::move(vec221), std::move(vec222), std::move(vec223));
  return true;
}

bool Database::m2_1(::hyde::rt::Vec<Tup_i32> vec225) {
  ::hyde::rt::Vec<Tup_i32> vec227(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec228(allocator);
  ::hyde::rt::Vec<Tup_i32> vec229(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec230(allocator);
  ::hyde::rt::Vec<Tup_i32> vec231(allocator);
  ::hyde::rt::Vec<Tup_i32> vec232(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec233(allocator);
  ::hyde::rt::Vec<Tup_i32> vec234(allocator);
  proc_59(std::move(vec227), std::move(vec228), std::move(vec229), std::move(vec225), std::move(vec230), std::move(vec231), std::move(vec232), std::move(vec233), std::move(vec234));
  return true;
}

bool Database::step2_2(::hyde::rt::Vec<Tup_i32_i32> vec236) {
  ::hyde::rt::Vec<Tup_i32> vec238(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec239(allocator);
  ::hyde::rt::Vec<Tup_i32> vec240(allocator);
  ::hyde::rt::Vec<Tup_i32> vec241(allocator);
  ::hyde::rt::Vec<Tup_i32> vec242(allocator);
  ::hyde::rt::Vec<Tup_i32> vec243(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec244(allocator);
  ::hyde::rt::Vec<Tup_i32> vec245(allocator);
  proc_59(std::move(vec238), std::move(vec239), std::move(vec240), std::move(vec241), std::move(vec236), std::move(vec242), std::move(vec243), std::move(vec244), std::move(vec245));
  return true;
}

bool Database::seed2_1(::hyde::rt::Vec<Tup_i32> vec247) {
  ::hyde::rt::Vec<Tup_i32> vec249(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec250(allocator);
  ::hyde::rt::Vec<Tup_i32> vec251(allocator);
  ::hyde::rt::Vec<Tup_i32> vec252(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec253(allocator);
  ::hyde::rt::Vec<Tup_i32> vec254(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec255(allocator);
  ::hyde::rt::Vec<Tup_i32> vec256(allocator);
  proc_59(std::move(vec249), std::move(vec250), std::move(vec251), std::move(vec252), std::move(vec253), std::move(vec247), std::move(vec254), std::move(vec255), std::move(vec256));
  return true;
}

bool Database::m3_1(::hyde::rt::Vec<Tup_i32> vec258) {
  ::hyde::rt::Vec<Tup_i32> vec260(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec261(allocator);
  ::hyde::rt::Vec<Tup_i32> vec262(allocator);
  ::hyde::rt::Vec<Tup_i32> vec263(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec264(allocator);
  ::hyde::rt::Vec<Tup_i32> vec265(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec266(allocator);
  ::hyde::rt::Vec<Tup_i32> vec267(allocator);
  proc_59(std::move(vec260), std::move(vec261), std::move(vec262), std::move(vec263), std::move(vec264), std::move(vec265), std::move(vec258), std::move(vec266), std::move(vec267));
  return true;
}

bool Database::step3_2(::hyde::rt::Vec<Tup_i32_i32> vec269) {
  ::hyde::rt::Vec<Tup_i32> vec271(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec272(allocator);
  ::hyde::rt::Vec<Tup_i32> vec273(allocator);
  ::hyde::rt::Vec<Tup_i32> vec274(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec275(allocator);
  ::hyde::rt::Vec<Tup_i32> vec276(allocator);
  ::hyde::rt::Vec<Tup_i32> vec277(allocator);
  ::hyde::rt::Vec<Tup_i32> vec278(allocator);
  proc_59(std::move(vec271), std::move(vec272), std::move(vec273), std::move(vec274), std::move(vec275), std::move(vec276), std::move(vec277), std::move(vec269), std::move(vec278));
  return true;
}

bool Database::seed3_1(::hyde::rt::Vec<Tup_i32> vec280) {
  ::hyde::rt::Vec<Tup_i32> vec282(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec283(allocator);
  ::hyde::rt::Vec<Tup_i32> vec284(allocator);
  ::hyde::rt::Vec<Tup_i32> vec285(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec286(allocator);
  ::hyde::rt::Vec<Tup_i32> vec287(allocator);
  ::hyde::rt::Vec<Tup_i32> vec288(allocator);
  ::hyde::rt::Vec<Tup_i32_i32> vec289(allocator);
  proc_59(std::move(vec282), std::move(vec283), std::move(vec284), std::move(vec285), std::move(vec286), std::move(vec287), std::move(vec288), std::move(vec289), std::move(vec280));
  return true;
}

bool Database::flow_300(::hyde::rt::Vec<Tup_i32> vec65, ::hyde::rt::Vec<Tup_b> vec68, ::hyde::rt::Vec<Tup_i32> vec70, ::hyde::rt::Vec<Tup_i32> vec85, ::hyde::rt::Vec<Tup_i32> vec88, ::hyde::rt::Vec<Tup_b> vec90, ::hyde::rt::Vec<Tup_i32> vec105, ::hyde::rt::Vec<Tup_i32> vec108, ::hyde::rt::Vec<Tup_b> vec110) {
  ::hyde::rt::Vec<Tup_i32> vec66(allocator);
  ::hyde::rt::Vec<Tup_i32> vec67(allocator);
  ::hyde::rt::Vec<Tup_b> vec69(allocator);
  ::hyde::rt::Vec<Tup_i32> vec71(allocator);
  ::hyde::rt::Vec<Tup_i32> vec86(allocator);
  ::hyde::rt::Vec<Tup_i32> vec87(allocator);
  ::hyde::rt::Vec<Tup_i32> vec89(allocator);
  ::hyde::rt::Vec<Tup_b> vec91(allocator);
  ::hyde::rt::Vec<Tup_i32> vec106(allocator);
  ::hyde::rt::Vec<Tup_i32> vec107(allocator);
  ::hyde::rt::Vec<Tup_i32> vec109(allocator);
  ::hyde::rt::Vec<Tup_b> vec111(allocator);
  // set 0 depth 1
  if ((g60 += 1) == 1) {
  }
  for (bool reenter64 = true; reenter64; ) {
    for (bool changed64 = true; changed64; changed64 = !(true && vec65.Empty() && vec68.Empty() && vec70.Empty() && vec85.Empty() && vec88.Empty() && vec90.Empty() && vec105.Empty() && vec108.Empty() && vec110.Empty())) {
      vec66.Clear();
      vec65.SortAndUnique();
      vec65.Swap(vec66);
      for (auto [v123] : vec66) {
        vec67.Add({v123});
        const uint64_t v146 = 0;
        (void) v146;
        vec70.Add({v123});
      }
      vec69.Clear();
      vec68.SortAndUnique();
      vec68.Swap(vec69);
      for (auto [v186] : vec69) {
        if (const uint32_t j185_0 = c1_5.Find({v186}); j185_0 != ::hyde::rt::kNoRow) {
          const auto r185_0 = c1_5.RowAt(j185_0);
          const auto v189 = r185_0.c0;
          for (uint32_t j185_1 = idx_187.First({v186}); j185_1 != ::hyde::rt::kNoRow; j185_1 = idx_187.Next(j185_1)) {
            const auto r185_1 = table_39.RowAt(j185_1);
            const auto v190 = r185_1.y;
            const auto v188 = r185_1.c1;
            if (v186 == v188 && v186 == v189) {
              const auto v126 = v188;
              const auto v127 = v190;
              if (const auto ins12 = t1_out_26.TryChangeAbsentToPresent({v127}); ins12.changed) {
                const uint64_t v147 = 0;
                (void) v147;
                vec65.Add({v127});
              }
            }
          }
        }
      }
      vec69.Clear();
      vec71.Clear();
      vec70.SortAndUnique();
      vec70.Swap(vec71);
      for (auto [v168] : vec71) {
        if (const uint32_t j167_0 = t1_out_26.Find({v168}); j167_0 != ::hyde::rt::kNoRow) {
          const auto r167_0 = t1_out_26.RowAt(j167_0);
          const auto v170 = r167_0.x;
          for (uint32_t j167_1 = idx_169.First({v168}); j167_1 != ::hyde::rt::kNoRow; j167_1 = idx_169.Next(j167_1)) {
            const auto r167_1 = table_35.RowAt(j167_1);
            const auto v171 = r167_1.x;
            const auto v172 = r167_1.y;
            if (v168 == v170 && v168 == v171) {
              const auto v128 = v171;
              const auto v129 = v172;
              if (const auto ins13 = table_39.TryChangeAbsentToPresent({v129, true}); ins13.changed) {
                if (ins13.added_row) {
                  idx_187.Add({true}, ins13.id);
                }
                const uint64_t v148 = 0;
                (void) v148;
                vec68.Add({true});
              }
            }
          }
        }
      }
      vec71.Clear();
      vec86.Clear();
      vec85.SortAndUnique();
      vec85.Swap(vec86);
      for (auto [v131] : vec86) {
        vec87.Add({v131});
        const uint64_t v149 = 0;
        (void) v149;
        vec88.Add({v131});
      }
      vec89.Clear();
      vec88.SortAndUnique();
      vec88.Swap(vec89);
      for (auto [v162] : vec89) {
        if (const uint32_t j161_0 = t2_out_29.Find({v162}); j161_0 != ::hyde::rt::kNoRow) {
          const auto r161_0 = t2_out_29.RowAt(j161_0);
          const auto v164 = r161_0.x;
          for (uint32_t j161_1 = idx_163.First({v162}); j161_1 != ::hyde::rt::kNoRow; j161_1 = idx_163.Next(j161_1)) {
            const auto r161_1 = table_43.RowAt(j161_1);
            const auto v165 = r161_1.x;
            const auto v166 = r161_1.y;
            if (v162 == v164 && v162 == v165) {
              const auto v134 = v165;
              const auto v135 = v166;
              if (const auto ins14 = table_47.TryChangeAbsentToPresent({v135, true}); ins14.changed) {
                if (ins14.added_row) {
                  idx_181.Add({true}, ins14.id);
                }
                const uint64_t v150 = 0;
                (void) v150;
                vec90.Add({true});
              }
            }
          }
        }
      }
      vec89.Clear();
      vec91.Clear();
      vec90.SortAndUnique();
      vec90.Swap(vec91);
      for (auto [v180] : vec91) {
        if (const uint32_t j179_0 = c2_12.Find({v180}); j179_0 != ::hyde::rt::kNoRow) {
          const auto r179_0 = c2_12.RowAt(j179_0);
          const auto v183 = r179_0.c0;
          for (uint32_t j179_1 = idx_181.First({v180}); j179_1 != ::hyde::rt::kNoRow; j179_1 = idx_181.Next(j179_1)) {
            const auto r179_1 = table_47.RowAt(j179_1);
            const auto v184 = r179_1.y;
            const auto v182 = r179_1.c1;
            if (v180 == v182 && v180 == v183) {
              const auto v136 = v182;
              const auto v137 = v184;
              if (const auto ins15 = t2_out_29.TryChangeAbsentToPresent({v137}); ins15.changed) {
                const uint64_t v151 = 0;
                (void) v151;
                vec85.Add({v137});
              }
            }
          }
        }
      }
      vec91.Clear();
      vec106.Clear();
      vec105.SortAndUnique();
      vec105.Swap(vec106);
      for (auto [v139] : vec106) {
        vec107.Add({v139});
        const uint64_t v152 = 0;
        (void) v152;
        vec108.Add({v139});
      }
      vec109.Clear();
      vec108.SortAndUnique();
      vec108.Swap(vec109);
      for (auto [v156] : vec109) {
        if (const uint32_t j155_0 = t3_out_32.Find({v156}); j155_0 != ::hyde::rt::kNoRow) {
          const auto r155_0 = t3_out_32.RowAt(j155_0);
          const auto v158 = r155_0.x;
          for (uint32_t j155_1 = idx_157.First({v156}); j155_1 != ::hyde::rt::kNoRow; j155_1 = idx_157.Next(j155_1)) {
            const auto r155_1 = table_51.RowAt(j155_1);
            const auto v159 = r155_1.x;
            const auto v160 = r155_1.y;
            if (v156 == v158 && v156 == v159) {
              const auto v142 = v159;
              const auto v143 = v160;
              if (const auto ins16 = table_55.TryChangeAbsentToPresent({v143, true}); ins16.changed) {
                if (ins16.added_row) {
                  idx_175.Add({true}, ins16.id);
                }
                const uint64_t v153 = 0;
                (void) v153;
                vec110.Add({true});
              }
            }
          }
        }
      }
      vec109.Clear();
      vec111.Clear();
      vec110.SortAndUnique();
      vec110.Swap(vec111);
      for (auto [v174] : vec111) {
        if (const uint32_t j173_0 = c3_19.Find({v174}); j173_0 != ::hyde::rt::kNoRow) {
          const auto r173_0 = c3_19.RowAt(j173_0);
          const auto v177 = r173_0.c0;
          for (uint32_t j173_1 = idx_175.First({v174}); j173_1 != ::hyde::rt::kNoRow; j173_1 = idx_175.Next(j173_1)) {
            const auto r173_1 = table_55.RowAt(j173_1);
            const auto v178 = r173_1.y;
            const auto v176 = r173_1.c1;
            if (v174 == v176 && v174 == v177) {
              const auto v144 = v176;
              const auto v145 = v178;
              if (const auto ins17 = t3_out_32.TryChangeAbsentToPresent({v145}); ins17.changed) {
                const uint64_t v154 = 0;
                (void) v154;
                vec105.Add({v145});
              }
            }
          }
        }
      }
      vec111.Clear();
    }
    vec65.Clear();
    vec66.Clear();
    vec67.SortAndUnique();
    for (auto [v125] : vec67) {
    }
    vec68.Clear();
    vec69.Clear();
    vec70.Clear();
    vec71.Clear();
    vec85.Clear();
    vec86.Clear();
    vec87.SortAndUnique();
    for (auto [v133] : vec87) {
    }
    vec88.Clear();
    vec89.Clear();
    vec90.Clear();
    vec91.Clear();
    vec105.Clear();
    vec106.Clear();
    vec107.SortAndUnique();
    for (auto [v141] : vec107) {
    }
    vec108.Clear();
    vec109.Clear();
    vec110.Clear();
    vec111.Clear();
    vec110.Clear();
    vec108.Clear();
    vec85.Clear();
    vec70.Clear();
    vec88.Clear();
    vec68.Clear();
    vec105.Clear();
    vec90.Clear();
    vec65.Clear();
    vec111.Clear();
    vec109.Clear();
    vec86.Clear();
    vec71.Clear();
    vec89.Clear();
    vec69.Clear();
    vec106.Clear();
    vec91.Clear();
    vec66.Clear();
    vec107.Clear();
    vec87.Clear();
    vec67.Clear();
    reenter64 = !(true && vec65.Empty() && vec68.Empty() && vec70.Empty() && vec85.Empty() && vec88.Empty() && vec90.Empty() && vec105.Empty() && vec108.Empty() && vec110.Empty());
  }
  return true;
}

Database::t1_out_f_cursor Database::t1_out_f() {
  return {*this, 0};
}

bool Database::t1_out_f_cursor::next(int32_t &X) {
  while (pos < db.t1_out_26.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.t1_out_26.RowAt(id);
    if (db.t1_out_26.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

Database::t2_out_f_cursor Database::t2_out_f() {
  return {*this, 0};
}

bool Database::t2_out_f_cursor::next(int32_t &X) {
  while (pos < db.t2_out_29.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.t2_out_29.RowAt(id);
    if (db.t2_out_29.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

Database::t3_out_f_cursor Database::t3_out_f() {
  return {*this, 0};
}

bool Database::t3_out_f_cursor::next(int32_t &X) {
  while (pos < db.t3_out_32.NumRows()) {
    const uint32_t id = pos++;
    const auto row = db.t3_out_32.RowAt(id);
    if (db.t3_out_32.StateAt(id) != ::hyde::rt::TupleState::kPresent) {
      continue;
    }
    X = row.x;
    return true;
  }
  return false;
}

