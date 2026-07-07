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

// Key of `idx_112` over `table_5`.
struct Key112 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key112 &) const noexcept = default;
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
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Key of `idx_87` over `table_12`.
struct Key87 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key87 &) const noexcept = default;
};

// Rows of `__16` (/0).
struct Row16 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row16 &) const noexcept = default;
};

// Rows of `table_19`.
struct Row19 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row19 &) const noexcept = default;
};

// Key of `idx_220` over `table_19`.
struct Key220 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key220 &) const noexcept = default;
};

// Rows of `table_23`.
struct Row23 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row23 &) const noexcept = default;
};

// Key of `idx_192` over `table_23`.
struct Key192 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key192 &) const noexcept = default;
};

// Rows of `table_27`.
struct Row27 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row27 &) const noexcept = default;
};

// Rows of `table_30`.
struct Row30 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row30 &) const noexcept = default;
};

// Rows of `table_33`.
struct Row33 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row33 &) const noexcept = default;
};

// Key of `idx_310` over `table_33`.
struct Key310 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key310 &) const noexcept = default;
};

// Rows of `table_37`.
struct Row37 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row37 &) const noexcept = default;
};

// Key of `idx_232` over `table_37`.
struct Key232 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key232 &) const noexcept = default;
};

// Rows of `mixed_41` (mixed/1).
struct Row41 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row41 &) const noexcept = default;
};

// Rows of `table_44`.
struct Row44 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row44 &) const noexcept = default;
};

// Rows of `table_47`.
struct Row47 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row47 &) const noexcept = default;
};

// Rows of `table_50`.
struct Row50 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row50 &) const noexcept = default;
};

// Rows of `table_53`.
struct Row53 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row53 &) const noexcept = default;
};

// Rows of `table_57`.
struct Row57 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row57 &) const noexcept = default;
};

// Rows of `pos_neg_61` (pos_neg/1).
struct Row61 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row61 &) const noexcept = default;
};

// Rows of `both_pos_64` (both_pos/1).
struct Row64 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row64 &) const noexcept = default;
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
  bool set_c1_1(::hyde::rt::Vec<Tup_i32> vec135, ::hyde::rt::Vec<Tup_i32> vec136);

  // Message `set_c2/1`.
  bool set_c2_1(::hyde::rt::Vec<Tup_i32> vec142, ::hyde::rt::Vec<Tup_i32> vec143);

  // Message `add_r/1`.
  bool add_r_1(::hyde::rt::Vec<Tup_i32> vec149);

  // Message `add_s/1`.
  bool add_s_1(::hyde::rt::Vec<Tup_i32> vec155);

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
  bool proc_67(::hyde::rt::Vec<Tup_i32> vec69, ::hyde::rt::Vec<Tup_i32> vec70, ::hyde::rt::Vec<Tup_i32> vec91, ::hyde::rt::Vec<Tup_i32> vec92, ::hyde::rt::Vec<Tup_i32> vec116, ::hyde::rt::Vec<Tup_i32> vec119);
  bool find_75(int32_t v76);
  bool find_84(bool v85);
  bool find_98(bool v99, int32_t v100);
  bool find_109(bool v110);
  bool find_165(int32_t v166);
  bool find_167(int32_t v168);
  bool find_169(int32_t v170);
  bool find_171(int32_t v172);
  bool find_175(int32_t v176);
  bool find_178(int32_t v179);
  bool find_181(int32_t v182);
  bool find_184(int32_t v185);
  bool find_189(int32_t v190);
  bool find_196(bool v197, int32_t v198);
  bool find_200(bool v201, int32_t v202);
  bool find_205(int32_t v206);
  bool find_209(int32_t v210);
  bool find_212(int32_t v213);
  bool find_217(int32_t v218);
  bool find_225(int32_t v226);
  bool find_229(int32_t v230);
  bool find_236(bool v237, int32_t v238);
  bool find_240(bool v241, int32_t v242);
  bool find_245(int32_t v246, bool v247);
  bool find_249(bool v250);
  bool find_252(bool v253);
  bool find_255(bool v256);
  bool find_260(bool v261);
  bool find_266(bool v267, int32_t v268);
  bool find_270(bool v271, int32_t v272);
  bool find_275(int32_t v276);
  bool find_278(int32_t v279);
  bool find_281(int32_t v282);
  bool find_286(int32_t v287, bool v288);
  bool find_290(int32_t v291, bool v292);
  bool find_296(int32_t v297);
  bool find_303(int32_t v304);
  bool find_307(int32_t v308);
  bool find_314(bool v315, int32_t v316);
  bool find_318(bool v319, int32_t v320);
  bool find_323(int32_t v324);
  bool find_331(bool v332);
  bool find_336(bool v337, int32_t v338);
  bool find_341(int32_t v342, bool v343);
  bool find_345(bool v346);
  bool find_348(bool v349);
  bool find_351(bool v352);
  bool find_356(bool v357);
  bool find_362(bool v363, int32_t v364);
  bool find_366(bool v367, int32_t v368);
  bool find_371(int32_t v372);
  bool find_374(int32_t v375);
  bool find_377(int32_t v378);
  bool find_382(bool v383);
  bool flow_387(::hyde::rt::Vec<Tup_b> vec78, ::hyde::rt::Vec<Tup_b> vec102);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key112> idx_112;
  ::hyde::rt::Table<Row9> __9;
  ::hyde::rt::Table<Row12> table_12;
  ::hyde::rt::Index<Key87> idx_87;
  ::hyde::rt::Table<Row16> __16;
  ::hyde::rt::Table<Row19> table_19;
  ::hyde::rt::Index<Key220> idx_220;
  ::hyde::rt::Table<Row23> table_23;
  ::hyde::rt::Index<Key192> idx_192;
  ::hyde::rt::Table<Row27> table_27;
  ::hyde::rt::Table<Row30> table_30;
  ::hyde::rt::Table<Row33> table_33;
  ::hyde::rt::Index<Key310> idx_310;
  ::hyde::rt::Table<Row37> table_37;
  ::hyde::rt::Index<Key232> idx_232;
  ::hyde::rt::Table<Row41> mixed_41;
  ::hyde::rt::Table<Row44> table_44;
  ::hyde::rt::Table<Row47> table_47;
  ::hyde::rt::Table<Row50> table_50;
  ::hyde::rt::Table<Row53> table_53;
  ::hyde::rt::Table<Row57> table_57;
  ::hyde::rt::Table<Row61> pos_neg_61;
  ::hyde::rt::Table<Row64> both_pos_64;

  uint64_t g68 = 0;
};

