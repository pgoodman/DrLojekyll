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

// Key of `idx_56` over `table_5`.
struct Key56 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key56 &) const noexcept = default;
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
  int32_t s;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(s);
  }
  bool operator==(const Row15 &) const noexcept = default;
};

// Rows of `table_18`.
struct Row18 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Key of `idx_124` over `table_18`.
struct Key124 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key124 &) const noexcept = default;
};

// Rows of `table_22`.
struct Row22 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row22 &) const noexcept = default;
};

// Key of `idx_96` over `table_22`.
struct Key96 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key96 &) const noexcept = default;
};

// Rows of `table_26`.
struct Row26 {
  bool c0;
  int32_t s;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, s);
  }
  bool operator==(const Row26 &) const noexcept = default;
};

// Rows of `gated_30` (gated/1).
struct Row30 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row30 &) const noexcept = default;
};

// Rows of `ungated_33` (ungated/1).
struct Row33 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row33 &) const noexcept = default;
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
  bool support_1(::hyde::rt::Vec<Tup_i32> vec73, ::hyde::rt::Vec<Tup_i32> vec74);

  // Message `item/1`.
  bool item_1(::hyde::rt::Vec<Tup_i32> vec78, ::hyde::rt::Vec<Tup_i32> vec79);

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
  bool proc_36(::hyde::rt::Vec<Tup_i32> vec38, ::hyde::rt::Vec<Tup_i32> vec39, ::hyde::rt::Vec<Tup_i32> vec60, ::hyde::rt::Vec<Tup_i32> vec61);
  bool find_44(int32_t v45);
  bool find_53(bool v54);
  bool find_85(int32_t v86);
  bool find_87(int32_t v88);
  bool find_89(int32_t v90);
  bool find_93(int32_t v94);
  bool find_100(bool v101, int32_t v102);
  bool find_104(bool v105, int32_t v106);
  bool find_109(int32_t v110);
  bool find_113(int32_t v114);
  bool find_117(int32_t v118);
  bool find_121(int32_t v122);
  bool find_128(bool v129, int32_t v130);
  bool find_132(bool v133, int32_t v134);
  bool find_137(int32_t v138, bool v139);
  bool find_141(bool v142);
  bool find_144(bool v145);
  bool find_147(bool v148);
  bool find_152(bool v153);
  bool find_158(bool v159, int32_t v160);
  bool find_162(bool v163, int32_t v164);
  bool find_167(int32_t v168);
  bool find_170(int32_t v171);
  bool find_173(int32_t v174);
  bool find_178(int32_t v179, bool v180);
  bool find_182(int32_t v183, bool v184);
  bool find_188(int32_t v189);
  bool find_193(bool v194);
  bool flow_200(::hyde::rt::Vec<Tup_b> vec47);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key56> idx_56;
  ::hyde::rt::Table<Row9> __9;
  ::hyde::rt::Table<Row12> table_12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Index<Key124> idx_124;
  ::hyde::rt::Table<Row22> table_22;
  ::hyde::rt::Index<Key96> idx_96;
  ::hyde::rt::Table<Row26> table_26;
  ::hyde::rt::Table<Row30> gated_30;
  ::hyde::rt::Table<Row33> ungated_33;

  uint64_t g37 = 0;
};

