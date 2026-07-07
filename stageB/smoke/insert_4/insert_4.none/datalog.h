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

// Key of `idx_93` over `table_5`.
struct Key93 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key93 &) const noexcept = default;
};

// Rows of `table_9`.
struct Row9 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row9 &) const noexcept = default;
};

// Rows of `off_vals_12` (off_vals/1).
struct Row12 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Rows of `__15` (/0).
struct Row15 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row15 &) const noexcept = default;
};

// Rows of `table_18`.
struct Row18 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Rows of `on_vals_22` (on_vals/1).
struct Row22 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row22 &) const noexcept = default;
};

// Rows of `table_25`.
struct Row25 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row25 &) const noexcept = default;
};

// Key of `idx_55` over `table_25`.
struct Key55 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key55 &) const noexcept = default;
};

// Rows of `table_29`.
struct Row29 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row29 &) const noexcept = default;
};

// Key of `idx_38` over `table_29`.
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

  // Message `log_in/1`.
  bool log_in_1(::hyde::rt::Vec<Tup_i32> vec60);

  // Message `m/1`.
  bool m_1(::hyde::rt::Vec<Tup_i32> vec64);

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
  bool proc_33(::hyde::rt::Vec<Tup_i32> vec35, ::hyde::rt::Vec<Tup_i32> vec47);
  bool find_42(int32_t v43, bool v44);
  bool find_50(bool v51);
  bool find_70(int32_t v71);
  bool find_72(int32_t v73);
  bool find_76(int32_t v77);
  bool find_79(int32_t v80);
  bool find_82(int32_t v83);
  bool find_87(int32_t v88);
  bool find_90(int32_t v91);
  bool find_97(bool v98, int32_t v99);
  bool find_101(bool v102, int32_t v103);
  bool find_106(int32_t v107, bool v108);
  bool flow_111(::hyde::rt::Vec<Tup_b> vec46);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key93> idx_93;
  ::hyde::rt::Table<Row9> table_9;
  ::hyde::rt::Table<Row12> off_vals_12;
  ::hyde::rt::Table<Row15> __15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Table<Row22> on_vals_22;
  ::hyde::rt::Table<Row25> table_25;
  ::hyde::rt::Index<Key55> idx_55;
  ::hyde::rt::Table<Row29> table_29;
  ::hyde::rt::Index<Key38> idx_38;

  uint64_t g34 = 0;
};

