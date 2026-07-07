// Auto-generated file; do not edit.

#pragma once

#include <drlojekyll/Runtime/Allocator.h>
#include <drlojekyll/Runtime/Hash.h>
#include <drlojekyll/Runtime/Table.h>
#include <drlojekyll/Runtime/Vec.h>

#include <cstdint>
#include <optional>
#include <tuple>
#include <vector>

struct Tup_b {
  bool c0;
  auto operator<=>(const Tup_b &) const noexcept = default;
};

struct Tup_i32 {
  int32_t c0;
  auto operator<=>(const Tup_i32 &) const noexcept = default;
};

using add_r_input = Tup_i32;
using add_s_input = Tup_i32;

// Rows of `table_5`.
struct Row5 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row5 &) const noexcept = default;
};

// Key of `idx_88` over `table_5`.
struct Key88 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key88 &) const noexcept = default;
};

// Rows of `__9` (/0).
struct Row9 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row9 &) const noexcept = default;
};

// Rows of `table_12`.
struct Row12 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Key of `idx_216` over `table_12`.
struct Key216 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key216 &) const noexcept = default;
};

// Rows of `table_16`.
struct Row16 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row16 &) const noexcept = default;
};

// Key of `idx_188` over `table_16`.
struct Key188 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key188 &) const noexcept = default;
};

// Rows of `table_20`.
struct Row20 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row20 &) const noexcept = default;
};

// Rows of `table_23`.
struct Row23 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row23 &) const noexcept = default;
};

// Rows of `table_26`.
struct Row26 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row26 &) const noexcept = default;
};

// Key of `idx_239` over `table_26`.
struct Key239 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key239 &) const noexcept = default;
};

// Rows of `mixed_30` (mixed/1).
struct Row30 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row30 &) const noexcept = default;
};

// Rows of `table_33`.
struct Row33 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row33 &) const noexcept = default;
};

// Rows of `table_36`.
struct Row36 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row36 &) const noexcept = default;
};

// Rows of `table_39`.
struct Row39 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row39 &) const noexcept = default;
};

// Rows of `table_42`.
struct Row42 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row42 &) const noexcept = default;
};

// Rows of `table_46`.
struct Row46 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row46 &) const noexcept = default;
};

// Rows of `pos_neg_50` (pos_neg/1).
struct Row50 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row50 &) const noexcept = default;
};

// Rows of `both_pos_53` (both_pos/1).
struct Row53 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row53 &) const noexcept = default;
};

// User-provided functors. Define the declared member functions in
// your own translation unit; the generated code calls them.
struct DatabaseFunctors {
};

// Receives published messages. The default methods do nothing.
struct DatabaseLog {
};

class Database {
 public:
  explicit Database(::hyde::rt::Allocator allocator_, DatabaseLog &log_, DatabaseFunctors &functors_);

  // Message `set_c1/1`.
  bool set_c1_1(::hyde::rt::Vec<Tup_i32> vec131, ::hyde::rt::Vec<Tup_i32> vec132);

  // Message `set_c2/1`.
  bool set_c2_1(::hyde::rt::Vec<Tup_i32> vec138, ::hyde::rt::Vec<Tup_i32> vec139);

  // Message `add_r/1`.
  bool add_r_1(::hyde::rt::Vec<Tup_i32> vec145);

  // Message `add_s/1`.
  bool add_s_1(::hyde::rt::Vec<Tup_i32> vec151);

  // Query `pos_neg/1` (f).
  struct pos_neg_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  pos_neg_f_cursor pos_neg_f();

  // Query `both_pos/1` (f).
  struct both_pos_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  both_pos_f_cursor both_pos_f();

  // Query `mixed/1` (f).
  struct mixed_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  mixed_f_cursor mixed_f();

 private:
  bool init_4();
  bool proc_56(::hyde::rt::Vec<Tup_i32> vec58, ::hyde::rt::Vec<Tup_i32> vec59, ::hyde::rt::Vec<Tup_i32> vec92, ::hyde::rt::Vec<Tup_i32> vec93, ::hyde::rt::Vec<Tup_i32> vec117, ::hyde::rt::Vec<Tup_i32> vec120);
  bool find_64(int32_t v65);
  bool find_70(bool v71, int32_t v72);
  bool find_80(bool v81);
  bool find_161(int32_t v162);
  bool find_163(int32_t v164);
  bool find_165(int32_t v166);
  bool find_167(int32_t v168);
  bool find_171(int32_t v172);
  bool find_174(int32_t v175);
  bool find_177(int32_t v178);
  bool find_180(int32_t v181);
  bool find_185(int32_t v186);
  bool find_192(bool v193, int32_t v194);
  bool find_196(bool v197, int32_t v198);
  bool find_201(int32_t v202);
  bool find_205(int32_t v206);
  bool find_208(int32_t v209);
  bool find_213(int32_t v214);
  bool find_221(int32_t v222);
  bool find_225(int32_t v226);
  bool find_232(int32_t v233);
  bool find_236(int32_t v237);
  bool find_243(bool v244, int32_t v245);
  bool find_247(bool v248, int32_t v249);
  bool find_252(int32_t v253);
  bool find_260(bool v261);
  bool find_264(bool v265);
  bool find_270(bool v271, int32_t v272);
  bool find_277(bool v278, int32_t v279);
  bool find_281(bool v282, int32_t v283);
  bool find_286(int32_t v287);
  bool find_289(int32_t v290);
  bool find_292(int32_t v293);
  bool find_297(bool v298, int32_t v299);
  bool find_302(int32_t v303);
  bool find_305(int32_t v306);
  bool find_308(int32_t v309);
  bool find_313(bool v314, int32_t v315);
  bool find_318(int32_t v319, bool v320);
  bool find_322(bool v323);
  bool find_325(bool v326);
  bool find_328(bool v329);
  bool flow_334(::hyde::rt::Vec<Tup_b> vec74);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key88> idx_88;
  ::hyde::rt::Table<Row9> __9;
  ::hyde::rt::Table<Row12> table_12;
  ::hyde::rt::Index<Key216> idx_216;
  ::hyde::rt::Table<Row16> table_16;
  ::hyde::rt::Index<Key188> idx_188;
  ::hyde::rt::Table<Row20> table_20;
  ::hyde::rt::Table<Row23> table_23;
  ::hyde::rt::Table<Row26> table_26;
  ::hyde::rt::Index<Key239> idx_239;
  ::hyde::rt::Table<Row30> mixed_30;
  ::hyde::rt::Table<Row33> table_33;
  ::hyde::rt::Table<Row36> table_36;
  ::hyde::rt::Table<Row39> table_39;
  ::hyde::rt::Table<Row42> table_42;
  ::hyde::rt::Table<Row46> table_46;
  ::hyde::rt::Table<Row50> pos_neg_50;
  ::hyde::rt::Table<Row53> both_pos_53;

  uint64_t g57 = 0;
};

