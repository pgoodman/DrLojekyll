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
      table_38(allocator_),
      table_41(allocator_),
      table_44(allocator_),
      idx_163(allocator_),
      table_48(allocator_),
      idx_181(allocator_),
      table_52(allocator_),
      idx_169(allocator_),
      table_56(allocator_),
      idx_187(allocator_),
      table_60(allocator_),
      idx_157(allocator_),
      table_64(allocator_),
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
  proc_68(std::move(vec291), std::move(vec292), std::move(vec293), std::move(vec294), std::move(vec295), std::move(vec296), std::move(vec297), std::move(vec298), std::move(vec299));
  return false;
}

bool Database::proc_68(::hyde::rt::Vec<Tup_i32> vec70, ::hyde::rt::Vec<Tup_i32_i32> vec81, ::hyde::rt::Vec<Tup_i32> vec86, ::hyde::rt::Vec<Tup_i32> vec90, ::hyde::rt::Vec<Tup_i32_i32> vec100, ::hyde::rt::Vec<Tup_i32> vec105, ::hyde::rt::Vec<Tup_i32> vec109, ::hyde::rt::Vec<Tup_i32_i32> vec119, ::hyde::rt::Vec<Tup_i32> vec124) {
  ::hyde::rt::Vec<Tup_i32> vec74(allocator);
  ::hyde::rt::Vec<Tup_b> vec76(allocator);
  ::hyde::rt::Vec<Tup_i32> vec78(allocator);
  ::hyde::rt::Vec<Tup_i32> vec93(allocator);
  ::hyde::rt::Vec<Tup_i32> vec95(allocator);
  ::hyde::rt::Vec<Tup_b> vec97(allocator);
  ::hyde::rt::Vec<Tup_i32> vec112(allocator);
  ::hyde::rt::Vec<Tup_i32> vec114(allocator);
  ::hyde::rt::Vec<Tup_b> vec116(allocator);
  for (auto [v72] : vec70) {
    if (const auto ins0 = table_35.TryChangeAbsentToPresent({v72}); ins0.changed) {
      const uint64_t v80 = 0;
      (void) v80;
      vec78.Add({v72});
    }
  }
  for (auto [v83, v84] : vec81) {
    if (const auto ins1 = table_44.TryChangeAbsentToPresent({v83, v84}); ins1.changed) {
      if (ins1.added_row) {
        idx_163.Add({v83}, ins1.id);
      }
      const uint64_t v85 = 0;
      (void) v85;
      vec74.Add({v83});
    }
  }
  for (auto [v88] : vec86) {
    if (const auto ins2 = table_8.TryChangeAbsentToPresent({true, v88}); ins2.changed) {
      if (const auto ins3 = c1_5.TryChangeAbsentToPresent({true}); ins3.changed) {
        const uint64_t v89 = 0;
        (void) v89;
        vec76.Add({true});
      }
    }
  }
  for (auto [v92] : vec90) {
    if (const auto ins4 = table_38.TryChangeAbsentToPresent({v92}); ins4.changed) {
      const uint64_t v99 = 0;
      (void) v99;
      vec93.Add({v92});
    }
  }
  for (auto [v102, v103] : vec100) {
    if (const auto ins5 = table_52.TryChangeAbsentToPresent({v102, v103}); ins5.changed) {
      if (ins5.added_row) {
        idx_169.Add({v102}, ins5.id);
      }
      const uint64_t v104 = 0;
      (void) v104;
      vec95.Add({v102});
    }
  }
  for (auto [v107] : vec105) {
    if (const auto ins6 = table_15.TryChangeAbsentToPresent({true, v107}); ins6.changed) {
      if (const auto ins7 = c2_12.TryChangeAbsentToPresent({true}); ins7.changed) {
        const uint64_t v108 = 0;
        (void) v108;
        vec97.Add({true});
      }
    }
  }
  for (auto [v111] : vec109) {
    if (const auto ins8 = table_41.TryChangeAbsentToPresent({v111}); ins8.changed) {
      const uint64_t v118 = 0;
      (void) v118;
      vec112.Add({v111});
    }
  }
  for (auto [v121, v122] : vec119) {
    if (const auto ins9 = table_60.TryChangeAbsentToPresent({v121, v122}); ins9.changed) {
      if (ins9.added_row) {
        idx_157.Add({v121}, ins9.id);
      }
      const uint64_t v123 = 0;
      (void) v123;
      vec114.Add({v121});
    }
  }
  for (auto [v126] : vec124) {
    if (const auto ins10 = table_22.TryChangeAbsentToPresent({true, v126}); ins10.changed) {
      if (const auto ins11 = c3_19.TryChangeAbsentToPresent({true}); ins11.changed) {
        const uint64_t v127 = 0;
        (void) v127;
        vec116.Add({true});
      }
    }
  }
  vec70.Clear();
  vec81.Clear();
  vec86.Clear();
  vec90.Clear();
  vec100.Clear();
  vec105.Clear();
  vec109.Clear();
  vec119.Clear();
  vec124.Clear();
  flow_300(std::move(vec74), std::move(vec76), std::move(vec78), std::move(vec93), std::move(vec95), std::move(vec97), std::move(vec112), std::move(vec114), std::move(vec116));
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
  proc_68(std::move(vec192), std::move(vec194), std::move(vec195), std::move(vec196), std::move(vec197), std::move(vec198), std::move(vec199), std::move(vec200), std::move(vec201));
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
  proc_68(std::move(vec205), std::move(vec203), std::move(vec206), std::move(vec207), std::move(vec208), std::move(vec209), std::move(vec210), std::move(vec211), std::move(vec212));
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
  proc_68(std::move(vec216), std::move(vec217), std::move(vec214), std::move(vec218), std::move(vec219), std::move(vec220), std::move(vec221), std::move(vec222), std::move(vec223));
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
  proc_68(std::move(vec227), std::move(vec228), std::move(vec229), std::move(vec225), std::move(vec230), std::move(vec231), std::move(vec232), std::move(vec233), std::move(vec234));
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
  proc_68(std::move(vec238), std::move(vec239), std::move(vec240), std::move(vec241), std::move(vec236), std::move(vec242), std::move(vec243), std::move(vec244), std::move(vec245));
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
  proc_68(std::move(vec249), std::move(vec250), std::move(vec251), std::move(vec252), std::move(vec253), std::move(vec247), std::move(vec254), std::move(vec255), std::move(vec256));
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
  proc_68(std::move(vec260), std::move(vec261), std::move(vec262), std::move(vec263), std::move(vec264), std::move(vec265), std::move(vec258), std::move(vec266), std::move(vec267));
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
  proc_68(std::move(vec271), std::move(vec272), std::move(vec273), std::move(vec274), std::move(vec275), std::move(vec276), std::move(vec277), std::move(vec269), std::move(vec278));
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
  proc_68(std::move(vec282), std::move(vec283), std::move(vec284), std::move(vec285), std::move(vec286), std::move(vec287), std::move(vec288), std::move(vec289), std::move(vec280));
  return true;
}

bool Database::flow_300(::hyde::rt::Vec<Tup_i32> vec74, ::hyde::rt::Vec<Tup_b> vec76, ::hyde::rt::Vec<Tup_i32> vec78, ::hyde::rt::Vec<Tup_i32> vec93, ::hyde::rt::Vec<Tup_i32> vec95, ::hyde::rt::Vec<Tup_b> vec97, ::hyde::rt::Vec<Tup_i32> vec112, ::hyde::rt::Vec<Tup_i32> vec114, ::hyde::rt::Vec<Tup_b> vec116) {
  ::hyde::rt::Vec<Tup_i32> vec75(allocator);
  ::hyde::rt::Vec<Tup_b> vec77(allocator);
  ::hyde::rt::Vec<Tup_i32> vec79(allocator);
  ::hyde::rt::Vec<Tup_i32> vec94(allocator);
  ::hyde::rt::Vec<Tup_i32> vec96(allocator);
  ::hyde::rt::Vec<Tup_b> vec98(allocator);
  ::hyde::rt::Vec<Tup_i32> vec113(allocator);
  ::hyde::rt::Vec<Tup_i32> vec115(allocator);
  ::hyde::rt::Vec<Tup_b> vec117(allocator);
  // set 2 depth 1
  if ((g69 += 1) == 1) {
  }
  for (bool reenter73 = true; reenter73; ) {
    for (bool changed73 = true; changed73; changed73 = !(true && vec74.Empty() && vec76.Empty() && vec78.Empty() && vec93.Empty() && vec95.Empty() && vec97.Empty() && vec112.Empty() && vec114.Empty() && vec116.Empty())) {
      vec75.Clear();
      vec74.SortAndUnique();
      vec74.Swap(vec75);
      for (auto [v162] : vec75) {
        if (const uint32_t j161_0 = table_35.Find({v162}); j161_0 != ::hyde::rt::kNoRow) {
          const auto r161_0 = table_35.RowAt(j161_0);
          const auto v164 = r161_0.x;
          for (uint32_t j161_1 = idx_163.First({v162}); j161_1 != ::hyde::rt::kNoRow; j161_1 = idx_163.Next(j161_1)) {
            const auto r161_1 = table_44.RowAt(j161_1);
            const auto v165 = r161_1.x;
            const auto v166 = r161_1.y;
            if (v162 == v164 && v162 == v165) {
              const auto v128 = v165;
              const auto v129 = v166;
              if (const auto ins12 = table_48.TryChangeAbsentToPresent({v129, true}); ins12.changed) {
                if (ins12.added_row) {
                  idx_181.Add({true}, ins12.id);
                }
                const uint64_t v146 = 0;
                (void) v146;
                vec76.Add({true});
              }
            }
          }
        }
      }
      vec75.Clear();
      vec77.Clear();
      vec76.SortAndUnique();
      vec76.Swap(vec77);
      for (auto [v180] : vec77) {
        if (const uint32_t j179_0 = c1_5.Find({v180}); j179_0 != ::hyde::rt::kNoRow) {
          const auto r179_0 = c1_5.RowAt(j179_0);
          const auto v183 = r179_0.c0;
          for (uint32_t j179_1 = idx_181.First({v180}); j179_1 != ::hyde::rt::kNoRow; j179_1 = idx_181.Next(j179_1)) {
            const auto r179_1 = table_48.RowAt(j179_1);
            const auto v184 = r179_1.y;
            const auto v182 = r179_1.c1;
            if (v180 == v182 && v180 == v183) {
              const auto v130 = v182;
              const auto v131 = v184;
              if (const auto ins13 = table_35.TryChangeAbsentToPresent({v131}); ins13.changed) {
                const uint64_t v147 = 0;
                (void) v147;
                vec78.Add({v131});
              }
            }
          }
        }
      }
      vec77.Clear();
      vec79.Clear();
      vec78.SortAndUnique();
      vec78.Swap(vec79);
      for (auto [v133] : vec79) {
        if (const auto ins14 = t1_out_26.TryChangeAbsentToPresent({v133}); ins14.changed) {
        }
        const uint64_t v148 = 0;
        (void) v148;
        vec74.Add({v133});
      }
      vec94.Clear();
      vec93.SortAndUnique();
      vec93.Swap(vec94);
      for (auto [v135] : vec94) {
        if (const auto ins15 = t2_out_29.TryChangeAbsentToPresent({v135}); ins15.changed) {
        }
        const uint64_t v149 = 0;
        (void) v149;
        vec95.Add({v135});
      }
      vec96.Clear();
      vec95.SortAndUnique();
      vec95.Swap(vec96);
      for (auto [v168] : vec96) {
        if (const uint32_t j167_0 = table_38.Find({v168}); j167_0 != ::hyde::rt::kNoRow) {
          const auto r167_0 = table_38.RowAt(j167_0);
          const auto v170 = r167_0.x;
          for (uint32_t j167_1 = idx_169.First({v168}); j167_1 != ::hyde::rt::kNoRow; j167_1 = idx_169.Next(j167_1)) {
            const auto r167_1 = table_52.RowAt(j167_1);
            const auto v171 = r167_1.x;
            const auto v172 = r167_1.y;
            if (v168 == v170 && v168 == v171) {
              const auto v136 = v171;
              const auto v137 = v172;
              if (const auto ins16 = table_56.TryChangeAbsentToPresent({v137, true}); ins16.changed) {
                if (ins16.added_row) {
                  idx_187.Add({true}, ins16.id);
                }
                const uint64_t v150 = 0;
                (void) v150;
                vec97.Add({true});
              }
            }
          }
        }
      }
      vec96.Clear();
      vec98.Clear();
      vec97.SortAndUnique();
      vec97.Swap(vec98);
      for (auto [v186] : vec98) {
        if (const uint32_t j185_0 = c2_12.Find({v186}); j185_0 != ::hyde::rt::kNoRow) {
          const auto r185_0 = c2_12.RowAt(j185_0);
          const auto v189 = r185_0.c0;
          for (uint32_t j185_1 = idx_187.First({v186}); j185_1 != ::hyde::rt::kNoRow; j185_1 = idx_187.Next(j185_1)) {
            const auto r185_1 = table_56.RowAt(j185_1);
            const auto v190 = r185_1.y;
            const auto v188 = r185_1.c1;
            if (v186 == v188 && v186 == v189) {
              const auto v138 = v188;
              const auto v139 = v190;
              if (const auto ins17 = table_38.TryChangeAbsentToPresent({v139}); ins17.changed) {
                const uint64_t v151 = 0;
                (void) v151;
                vec93.Add({v139});
              }
            }
          }
        }
      }
      vec98.Clear();
      vec113.Clear();
      vec112.SortAndUnique();
      vec112.Swap(vec113);
      for (auto [v141] : vec113) {
        if (const auto ins18 = t3_out_32.TryChangeAbsentToPresent({v141}); ins18.changed) {
        }
        const uint64_t v152 = 0;
        (void) v152;
        vec114.Add({v141});
      }
      vec115.Clear();
      vec114.SortAndUnique();
      vec114.Swap(vec115);
      for (auto [v156] : vec115) {
        if (const uint32_t j155_0 = table_41.Find({v156}); j155_0 != ::hyde::rt::kNoRow) {
          const auto r155_0 = table_41.RowAt(j155_0);
          const auto v158 = r155_0.x;
          for (uint32_t j155_1 = idx_157.First({v156}); j155_1 != ::hyde::rt::kNoRow; j155_1 = idx_157.Next(j155_1)) {
            const auto r155_1 = table_60.RowAt(j155_1);
            const auto v159 = r155_1.x;
            const auto v160 = r155_1.y;
            if (v156 == v158 && v156 == v159) {
              const auto v142 = v159;
              const auto v143 = v160;
              if (const auto ins19 = table_64.TryChangeAbsentToPresent({v143, true}); ins19.changed) {
                if (ins19.added_row) {
                  idx_175.Add({true}, ins19.id);
                }
                const uint64_t v153 = 0;
                (void) v153;
                vec116.Add({true});
              }
            }
          }
        }
      }
      vec115.Clear();
      vec117.Clear();
      vec116.SortAndUnique();
      vec116.Swap(vec117);
      for (auto [v174] : vec117) {
        if (const uint32_t j173_0 = c3_19.Find({v174}); j173_0 != ::hyde::rt::kNoRow) {
          const auto r173_0 = c3_19.RowAt(j173_0);
          const auto v177 = r173_0.c0;
          for (uint32_t j173_1 = idx_175.First({v174}); j173_1 != ::hyde::rt::kNoRow; j173_1 = idx_175.Next(j173_1)) {
            const auto r173_1 = table_64.RowAt(j173_1);
            const auto v178 = r173_1.y;
            const auto v176 = r173_1.c1;
            if (v174 == v176 && v174 == v177) {
              const auto v144 = v176;
              const auto v145 = v178;
              if (const auto ins20 = table_41.TryChangeAbsentToPresent({v145}); ins20.changed) {
                const uint64_t v154 = 0;
                (void) v154;
                vec112.Add({v145});
              }
            }
          }
        }
      }
      vec117.Clear();
    }
    vec74.Clear();
    vec75.Clear();
    vec76.Clear();
    vec77.Clear();
    vec78.Clear();
    vec79.Clear();
    vec93.Clear();
    vec94.Clear();
    vec95.Clear();
    vec96.Clear();
    vec97.Clear();
    vec98.Clear();
    vec112.Clear();
    vec113.Clear();
    vec114.Clear();
    vec115.Clear();
    vec116.Clear();
    vec117.Clear();
    vec116.Clear();
    vec114.Clear();
    vec112.Clear();
    vec95.Clear();
    vec93.Clear();
    vec97.Clear();
    vec74.Clear();
    vec78.Clear();
    vec76.Clear();
    vec117.Clear();
    vec115.Clear();
    vec113.Clear();
    vec96.Clear();
    vec94.Clear();
    vec98.Clear();
    vec75.Clear();
    vec79.Clear();
    vec77.Clear();
    reenter73 = !(true && vec74.Empty() && vec76.Empty() && vec78.Empty() && vec93.Empty() && vec95.Empty() && vec97.Empty() && vec112.Empty() && vec114.Empty() && vec116.Empty());
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

