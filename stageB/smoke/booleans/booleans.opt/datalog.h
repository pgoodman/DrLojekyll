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

using add_user_input = Tup_i32;
using log_in_input = Tup_i32;

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
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, id);
  }
  bool operator==(const Row8 &) const noexcept = default;
};

// Rows of `user_is_logged_in_12` (user_is_logged_in/1).
struct Row12 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Rows of `table_15`.
struct Row15 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row15 &) const noexcept = default;
};

// Rows of `table_18`.
struct Row18 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Rows of `table_21`.
struct Row21 {
  int32_t id;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id, c1);
  }
  bool operator==(const Row21 &) const noexcept = default;
};

// Key of `idx_41` over `table_21`.
struct Key41 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key41 &) const noexcept = default;
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

  // Message `add_user/1`.
  bool add_user_1(::hyde::rt::Vec<Tup_i32> vec46);

  // Message `log_in/1`.
  bool log_in_1(::hyde::rt::Vec<Tup_i32> vec50);

  // Query `user_is_logged_in/1` (b).
  bool user_is_logged_in_b(int32_t UserId);

 private:
  bool init_4();
  bool proc_25(::hyde::rt::Vec<Tup_i32> vec27, ::hyde::rt::Vec<Tup_i32> vec32);
  bool flow_56(::hyde::rt::Vec<Tup_b> vec30, ::hyde::rt::Vec<Tup_i32> vec31);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> __5;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row12> user_is_logged_in_12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Table<Row21> table_21;
  ::hyde::rt::Index<Key41> idx_41;

  uint64_t g26 = 0;
};

