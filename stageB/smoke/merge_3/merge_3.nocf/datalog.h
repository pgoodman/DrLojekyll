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

using me_input = Tup_i32;
using ma_input = Tup_i32;
using mb_input = Tup_i32;

// Rows of `__6` (/0).
struct Row6 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row6 &) const noexcept = default;
};

// Rows of `table_9`.
struct Row9 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row9 &) const noexcept = default;
};

// Rows of `q_13` (q/1).
struct Row13 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row13 &) const noexcept = default;
};

// Rows of `table_16`.
struct Row16 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row16 &) const noexcept = default;
};

// Key of `idx_34` over `table_16`.
struct Key34 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key34 &) const noexcept = default;
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

  // Message `me/1`.
  bool me_1(::hyde::rt::Vec<Tup_i32> vec39);

  // Message `ma/1`.
  bool ma_1(::hyde::rt::Vec<Tup_i32> vec44);

  // Message `mb/1`.
  bool mb_1(::hyde::rt::Vec<Tup_i32> vec49);

  // Query `q/1` (f).
  struct q_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  q_f_cursor q_f();

 private:
  bool init_4();
  bool proc_20(::hyde::rt::Vec<Tup_i32> vec22, ::hyde::rt::Vec<Tup_i32> vec26, ::hyde::rt::Vec<Tup_i32> vec29);
  bool flow_57(::hyde::rt::Vec<Tup_b> vec25);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row6> __6;
  ::hyde::rt::Table<Row9> table_9;
  ::hyde::rt::Table<Row13> q_13;
  ::hyde::rt::Table<Row16> table_16;
  ::hyde::rt::Index<Key34> idx_34;

  uint64_t g21 = 0;
};

