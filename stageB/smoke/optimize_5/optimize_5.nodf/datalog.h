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
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row8 &) const noexcept = default;
};

// Rows of `out_12` (out/1).
struct Row12 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Rows of `table_15`.
struct Row15 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row15 &) const noexcept = default;
};

// Key of `idx_27` over `table_15`.
struct Key27 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key27 &) const noexcept = default;
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

  // Message `ping/1`.
  bool ping_1(::hyde::rt::Vec<Tup_i32> vec32);

  // Query `out/1` (f).
  struct out_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  out_f_cursor out_f();

 private:
  bool init_4();
  bool proc_19(::hyde::rt::Vec<Tup_i32> vec21);
  bool flow_36(::hyde::rt::Vec<Tup_b> vec24);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> __5;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row12> out_12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Index<Key27> idx_27;

  uint64_t g20 = 0;
};

