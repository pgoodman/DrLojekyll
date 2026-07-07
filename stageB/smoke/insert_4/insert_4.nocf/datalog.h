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

using log_in_input = Tup_i32;
using m_input = Tup_i32;

// Rows of `table_5`.
struct Row5 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row5 &) const noexcept = default;
};

// Key of `idx_72` over `table_5`.
struct Key72 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key72 &) const noexcept = default;
};

// Rows of `off_vals_9` (off_vals/1).
struct Row9 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row9 &) const noexcept = default;
};

// Rows of `__12` (/0).
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

// Rows of `on_vals_19` (on_vals/1).
struct Row19 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row19 &) const noexcept = default;
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

// Key of `idx_48` over `table_22`.
struct Key48 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key48 &) const noexcept = default;
};

// Rows of `table_26`.
struct Row26 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row26 &) const noexcept = default;
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

  // Message `log_in/1`.
  bool log_in_1(::hyde::rt::Vec<Tup_i32> vec53);

  // Message `m/1`.
  bool m_1(::hyde::rt::Vec<Tup_i32> vec57);

  // Query `on_vals/1` (f).
  struct on_vals_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  on_vals_f_cursor on_vals_f();

  // Query `off_vals/1` (f).
  struct off_vals_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  off_vals_f_cursor off_vals_f();

 private:
  bool init_4();
  bool proc_29(::hyde::rt::Vec<Tup_i32> vec31, ::hyde::rt::Vec<Tup_i32> vec40);
  bool find_36(int32_t v37);
  bool find_43(bool v44);
  bool find_63(int32_t v64);
  bool find_65(int32_t v66);
  bool find_69(int32_t v70);
  bool find_76(bool v77, int32_t v78);
  bool find_80(bool v81, int32_t v82);
  bool find_85(int32_t v86);
  bool flow_89(::hyde::rt::Vec<Tup_b> vec39);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key72> idx_72;
  ::hyde::rt::Table<Row9> off_vals_9;
  ::hyde::rt::Table<Row12> __12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Table<Row19> on_vals_19;
  ::hyde::rt::Table<Row22> table_22;
  ::hyde::rt::Index<Key48> idx_48;
  ::hyde::rt::Table<Row26> table_26;

  uint64_t g30 = 0;
};

