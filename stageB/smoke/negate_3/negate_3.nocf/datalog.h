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

struct Tup_i32_i32 {
  int32_t c0;
  int32_t c1;
  auto operator<=>(const Tup_i32_i32 &) const noexcept = default;
};

using pin_input = Tup_i32_i32;
using feed_input = Tup_i32;

// Rows of `table_7`.
struct Row7 {
  int32_t c0;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row7 &) const noexcept = default;
};

// Key of `idx_86` over `table_7`.
struct Key86 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key86 &) const noexcept = default;
};

// Rows of `__11` (/0).
struct Row11 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row11 &) const noexcept = default;
};

// Rows of `table_14`.
struct Row14 {
  int32_t c;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c, c1);
  }
  bool operator==(const Row14 &) const noexcept = default;
};

// Key of `idx_180` over `table_14`.
struct Key180 {
  int32_t c;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c);
  }
  bool operator==(const Key180 &) const noexcept = default;
};

// Rows of `table_18`.
struct Row18 {
  int32_t a;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Key of `idx_230` over `table_18`.
struct Key230 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Key230 &) const noexcept = default;
};

// Rows of `table_22`.
struct Row22 {
  int32_t a;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row22 &) const noexcept = default;
};

// Key of `idx_205` over `table_22`.
struct Key205 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Key205 &) const noexcept = default;
};

// Rows of `table_26`.
struct Row26 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row26 &) const noexcept = default;
};

// Key of `idx_134` over `table_26`.
struct Key134 {
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key134 &) const noexcept = default;
};

// Rows of `table_30`.
struct Row30 {
  bool c0;
  int32_t c;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c);
  }
  bool operator==(const Row30 &) const noexcept = default;
};

// Rows of `out7_34` (out7/1).
struct Row34 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row34 &) const noexcept = default;
};

// Rows of `outdup_37` (outdup/1).
struct Row37 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row37 &) const noexcept = default;
};

// Rows of `outflag_40` (outflag/1).
struct Row40 {
  int32_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row40 &) const noexcept = default;
};

// Rows of `table_43`.
struct Row43 {
  int32_t a;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row43 &) const noexcept = default;
};

// Rows of `table_47`.
struct Row47 {
  int32_t c;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c, c1);
  }
  bool operator==(const Row47 &) const noexcept = default;
};

// Rows of `table_51`.
struct Row51 {
  int32_t a;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row51 &) const noexcept = default;
};

// Rows of `table_55`.
struct Row55 {
  int32_t c;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c, c1);
  }
  bool operator==(const Row55 &) const noexcept = default;
};

// Rows of `table_59`.
struct Row59 {
  int32_t c;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c, c1);
  }
  bool operator==(const Row59 &) const noexcept = default;
};

// Rows of `table_63`.
struct Row63 {
  int32_t c;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c, c1);
  }
  bool operator==(const Row63 &) const noexcept = default;
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

  // Message `pin/2`.
  bool pin_2(::hyde::rt::Vec<Tup_i32_i32> vec111);

  // Message `feed/1`.
  bool feed_1(::hyde::rt::Vec<Tup_i32> vec115);

  // Query `out7/1` (f).
  struct out7_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  out7_f_cursor out7_f();

  // Query `outdup/1` (f).
  struct outdup_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  outdup_f_cursor outdup_f();

  // Query `outflag/1` (f).
  struct outflag_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  outflag_f_cursor outflag_f();

 private:
  bool init_4();
  bool proc_67(::hyde::rt::Vec<Tup_i32_i32> vec70, ::hyde::rt::Vec<Tup_i32> vec90);
  bool find_74(int32_t v75, int32_t v76);
  bool find_78(int32_t v79, int32_t v80);
  bool find_82(int32_t v83, int32_t v84);
  bool find_93(int32_t v94, int32_t v95);
  bool find_97(int32_t v98, int32_t v99);
  bool find_101(int32_t v102, int32_t v103);
  bool find_121(int32_t v122);
  bool find_123(int32_t v124);
  bool find_125(int32_t v126);
  bool find_127(int32_t v128);
  bool find_131(int32_t v132);
  bool find_138(bool v139, int32_t v140);
  bool find_142(bool v143, int32_t v144);
  bool find_147(int32_t v148, bool v149);
  bool find_151(bool v152);
  bool find_154(bool v155);
  bool find_157(bool v158);
  bool find_162(bool v163);
  bool find_168(bool v169, int32_t v170);
  bool find_172(bool v173, int32_t v174);
  bool find_177(int32_t v178);
  bool find_184(int32_t v185, int32_t v186);
  bool find_188(int32_t v189, int32_t v190);
  bool find_193(int32_t v194, int32_t v195);
  bool find_198(int32_t v199);
  bool find_202(int32_t v203);
  bool find_209(int32_t v210, int32_t v211);
  bool find_213(int32_t v214, int32_t v215);
  bool find_218(int32_t v219, int32_t v220);
  bool find_223(int32_t v224);
  bool find_227(int32_t v228);
  bool find_234(int32_t v235, int32_t v236);
  bool find_238(int32_t v239, int32_t v240);
  bool find_243(int32_t v244, int32_t v245);
  bool flow_248(::hyde::rt::Vec<Tup_b> vec69);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row7> table_7;
  ::hyde::rt::Index<Key86> idx_86;
  ::hyde::rt::Table<Row11> __11;
  ::hyde::rt::Table<Row14> table_14;
  ::hyde::rt::Index<Key180> idx_180;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Index<Key230> idx_230;
  ::hyde::rt::Table<Row22> table_22;
  ::hyde::rt::Index<Key205> idx_205;
  ::hyde::rt::Table<Row26> table_26;
  ::hyde::rt::Index<Key134> idx_134;
  ::hyde::rt::Table<Row30> table_30;
  ::hyde::rt::Table<Row34> out7_34;
  ::hyde::rt::Table<Row37> outdup_37;
  ::hyde::rt::Table<Row40> outflag_40;
  ::hyde::rt::Table<Row43> table_43;
  ::hyde::rt::Table<Row47> table_47;
  ::hyde::rt::Table<Row51> table_51;
  ::hyde::rt::Table<Row55> table_55;
  ::hyde::rt::Table<Row59> table_59;
  ::hyde::rt::Table<Row63> table_63;

  uint64_t g68 = 0;
};

