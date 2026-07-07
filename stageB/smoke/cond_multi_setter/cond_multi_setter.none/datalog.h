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

using add_item_input = Tup_i32;

// Rows of `table_5`.
struct Row5 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row5 &) const noexcept = default;
};

// Key of `idx_51` over `table_5`.
struct Key51 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key51 &) const noexcept = default;
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
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Rows of `table_15`.
struct Row15 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row15 &) const noexcept = default;
};

// Rows of `table_18`.
struct Row18 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Rows of `table_21`.
struct Row21 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row21 &) const noexcept = default;
};

// Rows of `table_24`.
struct Row24 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row24 &) const noexcept = default;
};

// Key of `idx_116` over `table_24`.
struct Key116 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key116 &) const noexcept = default;
};

// Rows of `table_28`.
struct Row28 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row28 &) const noexcept = default;
};

// Rows of `table_31`.
struct Row31 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row31 &) const noexcept = default;
};

// Rows of `table_35`.
struct Row35 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row35 &) const noexcept = default;
};

// Rows of `gated_39` (gated/1).
struct Row39 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row39 &) const noexcept = default;
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

  // Message `add_a/1`.
  bool add_a_1(::hyde::rt::Vec<Tup_i32> vec73, ::hyde::rt::Vec<Tup_i32> vec74);

  // Message `add_b/1`.
  bool add_b_1(::hyde::rt::Vec<Tup_i32> vec79, ::hyde::rt::Vec<Tup_i32> vec80);

  // Message `add_item/1`.
  bool add_item_1(::hyde::rt::Vec<Tup_i32> vec85);

  // Query `gated/1` (b).
  bool gated_b(int32_t X);

 private:
  bool init_4();
  bool proc_42(::hyde::rt::Vec<Tup_i32> vec44, ::hyde::rt::Vec<Tup_i32> vec45, ::hyde::rt::Vec<Tup_i32> vec55, ::hyde::rt::Vec<Tup_i32> vec56, ::hyde::rt::Vec<Tup_i32> vec64);
  bool find_93(int32_t v94);
  bool find_95(int32_t v96);
  bool find_99(int32_t v100);
  bool find_102(int32_t v103);
  bool find_105(int32_t v106);
  bool find_110(int32_t v111);
  bool find_113(int32_t v114);
  bool find_120(bool v121, int32_t v122);
  bool find_124(bool v125, int32_t v126);
  bool find_129(int32_t v130, bool v131);
  bool find_133(bool v134);
  bool find_136(bool v137);
  bool find_139(bool v140);
  bool find_144(bool v145);
  bool find_150(bool v151, int32_t v152);
  bool find_157(bool v158, int32_t v159);
  bool find_161(bool v162, int32_t v163);
  bool find_166(bool v167, int32_t v168);
  bool find_170(int32_t v171);
  bool find_173(int32_t v174);
  bool find_176(int32_t v177);
  bool find_179(int32_t v180);
  bool find_183(int32_t v184);
  bool find_186(int32_t v187);
  bool find_189(int32_t v190);
  bool find_194(int32_t v195);
  bool find_197(int32_t v198);
  bool find_201(bool v202, int32_t v203);
  bool find_206(bool v207, int32_t v208);
  bool find_210(int32_t v211);
  bool find_213(int32_t v214);
  bool find_216(int32_t v217);
  bool find_219(int32_t v220);
  bool find_223(int32_t v224);
  bool find_226(int32_t v227);
  bool find_229(int32_t v230);
  bool find_234(int32_t v235);
  bool find_237(int32_t v238);
  bool flow_241(::hyde::rt::Vec<Tup_b> vec48);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key51> idx_51;
  ::hyde::rt::Table<Row9> __9;
  ::hyde::rt::Table<Row12> table_12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Table<Row21> table_21;
  ::hyde::rt::Table<Row24> table_24;
  ::hyde::rt::Index<Key116> idx_116;
  ::hyde::rt::Table<Row28> table_28;
  ::hyde::rt::Table<Row31> table_31;
  ::hyde::rt::Table<Row35> table_35;
  ::hyde::rt::Table<Row39> gated_39;

  uint64_t g43 = 0;
};

