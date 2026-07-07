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

struct Tup_u32 {
  uint32_t c0;
  auto operator<=>(const Tup_u32 &) const noexcept = default;
};

using enable_input = Tup_u32;
using ping_input = Tup_i32;

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
  uint32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row8 &) const noexcept = default;
};

// Rows of `is_on_12` (is_on/1).
struct Row12 {
  int32_t y;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(y);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Rows of `table_15`.
struct Row15 {
  int32_t y;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(y, c1);
  }
  bool operator==(const Row15 &) const noexcept = default;
};

// Key of `idx_30` over `table_15`.
struct Key30 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key30 &) const noexcept = default;
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

  // Message `enable/1`.
  bool enable_1(::hyde::rt::Vec<Tup_u32> vec35);

  // Message `ping/1`.
  bool ping_1(::hyde::rt::Vec<Tup_i32> vec39);

  // Query `is_on/1` (f).
  struct is_on_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &Y);
  };
  is_on_f_cursor is_on_f();

 private:
  bool init_4();
  bool proc_19(::hyde::rt::Vec<Tup_u32> vec21, ::hyde::rt::Vec<Tup_i32> vec25);
  bool flow_45(::hyde::rt::Vec<Tup_b> vec24);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> __5;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row12> is_on_12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Index<Key30> idx_30;

  uint64_t g20 = 0;
};

