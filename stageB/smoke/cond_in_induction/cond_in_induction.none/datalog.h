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

using m1_input = Tup_i32;
using step1_input = Tup_i32_i32;
using seed1_input = Tup_i32;
using m2_input = Tup_i32;
using step2_input = Tup_i32_i32;
using seed2_input = Tup_i32;
using m3_input = Tup_i32;
using step3_input = Tup_i32_i32;
using seed3_input = Tup_i32;

// Rows of `c1_5` (c1/0).
struct Row5 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row5 &) const noexcept = default;
};

// Rows of `table_8`.
struct Row8 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row8 &) const noexcept = default;
};

// Rows of `c2_12` (c2/0).
struct Row12 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Rows of `table_15`.
struct Row15 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row15 &) const noexcept = default;
};

// Rows of `c3_19` (c3/0).
struct Row19 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row19 &) const noexcept = default;
};

// Rows of `table_22`.
struct Row22 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row22 &) const noexcept = default;
};

// Rows of `t1_out_26` (t1_out/1).
struct Row26 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row26 &) const noexcept = default;
};

// Rows of `t2_out_29` (t2_out/1).
struct Row29 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row29 &) const noexcept = default;
};

// Rows of `t3_out_32` (t3_out/1).
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
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row38 &) const noexcept = default;
};

// Rows of `table_41`.
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
  int32_t y;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, y);
  }
  bool operator==(const Row44 &) const noexcept = default;
};

// Key of `idx_163` over `table_44`.
struct Key163 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key163 &) const noexcept = default;
};

// Rows of `table_48`.
struct Row48 {
  int32_t y;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(y, c1);
  }
  bool operator==(const Row48 &) const noexcept = default;
};

// Key of `idx_181` over `table_48`.
struct Key181 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key181 &) const noexcept = default;
};

// Rows of `table_52`.
struct Row52 {
  int32_t x;
  int32_t y;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, y);
  }
  bool operator==(const Row52 &) const noexcept = default;
};

// Key of `idx_169` over `table_52`.
struct Key169 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key169 &) const noexcept = default;
};

// Rows of `table_56`.
struct Row56 {
  int32_t y;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(y, c1);
  }
  bool operator==(const Row56 &) const noexcept = default;
};

// Key of `idx_187` over `table_56`.
struct Key187 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key187 &) const noexcept = default;
};

// Rows of `table_60`.
struct Row60 {
  int32_t x;
  int32_t y;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, y);
  }
  bool operator==(const Row60 &) const noexcept = default;
};

// Key of `idx_157` over `table_60`.
struct Key157 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key157 &) const noexcept = default;
};

// Rows of `table_64`.
struct Row64 {
  int32_t y;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(y, c1);
  }
  bool operator==(const Row64 &) const noexcept = default;
};

// Key of `idx_175` over `table_64`.
struct Key175 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key175 &) const noexcept = default;
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

  // Message `m1/1`.
  bool m1_1(::hyde::rt::Vec<Tup_i32> vec192);

  // Message `step1/2`.
  bool step1_2(::hyde::rt::Vec<Tup_i32_i32> vec203);

  // Message `seed1/1`.
  bool seed1_1(::hyde::rt::Vec<Tup_i32> vec214);

  // Message `m2/1`.
  bool m2_1(::hyde::rt::Vec<Tup_i32> vec225);

  // Message `step2/2`.
  bool step2_2(::hyde::rt::Vec<Tup_i32_i32> vec236);

  // Message `seed2/1`.
  bool seed2_1(::hyde::rt::Vec<Tup_i32> vec247);

  // Message `m3/1`.
  bool m3_1(::hyde::rt::Vec<Tup_i32> vec258);

  // Message `step3/2`.
  bool step3_2(::hyde::rt::Vec<Tup_i32_i32> vec269);

  // Message `seed3/1`.
  bool seed3_1(::hyde::rt::Vec<Tup_i32> vec280);

  // Query `t1_out/1` (f).
  struct t1_out_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  t1_out_f_cursor t1_out_f();

  // Query `t2_out/1` (f).
  struct t2_out_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  t2_out_f_cursor t2_out_f();

  // Query `t3_out/1` (f).
  struct t3_out_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  t3_out_f_cursor t3_out_f();

 private:
  bool init_4();
  bool proc_68(::hyde::rt::Vec<Tup_i32> vec70, ::hyde::rt::Vec<Tup_i32_i32> vec81, ::hyde::rt::Vec<Tup_i32> vec86, ::hyde::rt::Vec<Tup_i32> vec90, ::hyde::rt::Vec<Tup_i32_i32> vec100, ::hyde::rt::Vec<Tup_i32> vec105, ::hyde::rt::Vec<Tup_i32> vec109, ::hyde::rt::Vec<Tup_i32_i32> vec119, ::hyde::rt::Vec<Tup_i32> vec124);
  bool flow_300(::hyde::rt::Vec<Tup_i32> vec74, ::hyde::rt::Vec<Tup_b> vec76, ::hyde::rt::Vec<Tup_i32> vec78, ::hyde::rt::Vec<Tup_i32> vec93, ::hyde::rt::Vec<Tup_i32> vec95, ::hyde::rt::Vec<Tup_b> vec97, ::hyde::rt::Vec<Tup_i32> vec112, ::hyde::rt::Vec<Tup_i32> vec114, ::hyde::rt::Vec<Tup_b> vec116);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> c1_5;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row12> c2_12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Table<Row19> c3_19;
  ::hyde::rt::Table<Row22> table_22;
  ::hyde::rt::Table<Row26> t1_out_26;
  ::hyde::rt::Table<Row29> t2_out_29;
  ::hyde::rt::Table<Row32> t3_out_32;
  ::hyde::rt::Table<Row35> table_35;
  ::hyde::rt::Table<Row38> table_38;
  ::hyde::rt::Table<Row41> table_41;
  ::hyde::rt::Table<Row44> table_44;
  ::hyde::rt::Index<Key163> idx_163;
  ::hyde::rt::Table<Row48> table_48;
  ::hyde::rt::Index<Key181> idx_181;
  ::hyde::rt::Table<Row52> table_52;
  ::hyde::rt::Index<Key169> idx_169;
  ::hyde::rt::Table<Row56> table_56;
  ::hyde::rt::Index<Key187> idx_187;
  ::hyde::rt::Table<Row60> table_60;
  ::hyde::rt::Index<Key157> idx_157;
  ::hyde::rt::Table<Row64> table_64;
  ::hyde::rt::Index<Key175> idx_175;

  uint64_t g69 = 0;
};

