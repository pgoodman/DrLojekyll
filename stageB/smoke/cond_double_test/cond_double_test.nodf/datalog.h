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

using feed_r_input = Tup_i32;
using set_c_input = Tup_i32;

// Rows of `__5` (/0).
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

// Rows of `q_double_12` (q_double/1).
struct Row12 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Rows of `p_single_15` (p_single/1).
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
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Key of `idx_44` over `table_18`.
struct Key44 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key44 &) const noexcept = default;
};

// Rows of `table_22`.
struct Row22 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row22 &) const noexcept = default;
};

// Key of `idx_38` over `table_22`.
struct Key38 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key38 &) const noexcept = default;
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

  // Message `feed_r/1`.
  bool feed_r_1(::hyde::rt::Vec<Tup_i32> vec49);

  // Message `set_c/1`.
  bool set_c_1(::hyde::rt::Vec<Tup_i32> vec53);

  // Query `q_double/1` (f).
  struct q_double_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  q_double_f_cursor q_double_f();

  // Query `p_single/1` (f).
  struct p_single_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  p_single_f_cursor p_single_f();

 private:
  bool init_4();
  bool proc_26(::hyde::rt::Vec<Tup_i32> vec28, ::hyde::rt::Vec<Tup_i32> vec33);
  bool flow_59(::hyde::rt::Vec<Tup_b> vec31, ::hyde::rt::Vec<Tup_b> vec32);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> __5;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row12> q_double_12;
  ::hyde::rt::Table<Row15> p_single_15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Index<Key44> idx_44;
  ::hyde::rt::Table<Row22> table_22;
  ::hyde::rt::Index<Key38> idx_38;

  uint64_t g27 = 0;
};

