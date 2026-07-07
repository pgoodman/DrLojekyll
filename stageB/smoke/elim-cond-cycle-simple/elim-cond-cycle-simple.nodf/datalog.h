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

using cond_func_input = Tup_i32;

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

// Rows of `output_13` (output/1).
struct Row13 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row13 &) const noexcept = default;
};

// Rows of `table_16`.
struct Row16 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row16 &) const noexcept = default;
};

// Rows of `table_19`.
struct Row19 {
  int32_t a;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row19 &) const noexcept = default;
};

// Key of `idx_43` over `table_19`.
struct Key43 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key43 &) const noexcept = default;
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

  // Message `cond_func/1`.
  bool cond_func_1(::hyde::rt::Vec<Tup_i32> vec48);

  // Query `output/1` (f).
  struct output_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  output_f_cursor output_f();

 private:
  bool init_4();
  bool proc_23(::hyde::rt::Vec<Tup_i32> vec31);
  bool flow_52(::hyde::rt::Vec<Tup_b> vec28);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row6> __6;
  ::hyde::rt::Table<Row9> table_9;
  ::hyde::rt::Table<Row13> output_13;
  ::hyde::rt::Table<Row16> table_16;
  ::hyde::rt::Table<Row19> table_19;
  ::hyde::rt::Index<Key43> idx_43;

  uint64_t g24 = 0;
};

