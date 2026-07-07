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


// Rows of `table_5`.
struct Row5 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row5 &) const noexcept = default;
};

// Key of `idx_76` over `table_5`.
struct Key76 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key76 &) const noexcept = default;
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
  int32_t s;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(s);
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
  int32_t s;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(s);
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

// Key of `idx_208` over `table_24`.
struct Key208 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key208 &) const noexcept = default;
};

// Rows of `table_28`.
struct Row28 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row28 &) const noexcept = default;
};

// Key of `idx_130` over `table_28`.
struct Key130 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key130 &) const noexcept = default;
};

// Rows of `table_32`.
struct Row32 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row32 &) const noexcept = default;
};

// Rows of `table_35`.
struct Row35 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row35 &) const noexcept = default;
};

// Rows of `table_38`.
struct Row38 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row38 &) const noexcept = default;
};

// Rows of `gated_42` (gated/1).
struct Row42 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row42 &) const noexcept = default;
};

// Rows of `ungated_45` (ungated/1).
struct Row45 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row45 &) const noexcept = default;
};

// Rows of `table_48`.
struct Row48 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row48 &) const noexcept = default;
};

// Key of `idx_58` over `table_48`.
struct Key58 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key58 &) const noexcept = default;
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

  // Message `support/1`.
  bool support_1(::hyde::rt::Vec<Tup_i32> vec93, ::hyde::rt::Vec<Tup_i32> vec94);

  // Message `item/1`.
  bool item_1(::hyde::rt::Vec<Tup_i32> vec98, ::hyde::rt::Vec<Tup_i32> vec99);

  // Query `gated/1` (f).
  struct gated_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  gated_f_cursor gated_f();

  // Query `ungated/1` (f).
  struct ungated_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  ungated_f_cursor ungated_f();

 private:
  bool init_4();
  bool proc_52(::hyde::rt::Vec<Tup_i32> vec54, ::hyde::rt::Vec<Tup_i32> vec55, ::hyde::rt::Vec<Tup_i32> vec80, ::hyde::rt::Vec<Tup_i32> vec81);
  bool find_62(int32_t v63, bool v64);
  bool find_73(bool v74);
  bool find_105(int32_t v106);
  bool find_107(int32_t v108);
  bool find_109(int32_t v110);
  bool find_113(int32_t v114);
  bool find_116(int32_t v117);
  bool find_124(int32_t v125);
  bool find_127(int32_t v128);
  bool find_134(bool v135, int32_t v136);
  bool find_138(bool v139, int32_t v140);
  bool find_148(int32_t v149, bool v150);
  bool find_153(int32_t v154);
  bool find_156(int32_t v157);
  bool find_159(int32_t v160);
  bool find_162(int32_t v163);
  bool find_166(int32_t v167);
  bool find_169(int32_t v170);
  bool find_177(int32_t v178);
  bool find_180(int32_t v181);
  bool find_183(int32_t v184);
  bool find_187(int32_t v188);
  bool find_191(int32_t v192);
  bool find_194(int32_t v195);
  bool find_202(int32_t v203);
  bool find_205(int32_t v206);
  bool find_212(bool v213, int32_t v214);
  bool find_216(bool v217, int32_t v218);
  bool find_221(int32_t v222, bool v223);
  bool find_228(bool v229);
  bool find_236(bool v237);
  bool find_242(bool v243, int32_t v244);
  bool find_246(bool v247, int32_t v248);
  bool find_251(bool v252, int32_t v253);
  bool find_255(int32_t v256);
  bool find_258(int32_t v259);
  bool find_261(int32_t v262);
  bool find_264(int32_t v265);
  bool find_268(int32_t v269);
  bool flow_311(::hyde::rt::Vec<Tup_b> vec66);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key76> idx_76;
  ::hyde::rt::Table<Row9> __9;
  ::hyde::rt::Table<Row12> table_12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Table<Row21> table_21;
  ::hyde::rt::Table<Row24> table_24;
  ::hyde::rt::Index<Key208> idx_208;
  ::hyde::rt::Table<Row28> table_28;
  ::hyde::rt::Index<Key130> idx_130;
  ::hyde::rt::Table<Row32> table_32;
  ::hyde::rt::Table<Row35> table_35;
  ::hyde::rt::Table<Row38> table_38;
  ::hyde::rt::Table<Row42> gated_42;
  ::hyde::rt::Table<Row45> ungated_45;
  ::hyde::rt::Table<Row48> table_48;
  ::hyde::rt::Index<Key58> idx_58;

  uint64_t g53 = 0;
};

