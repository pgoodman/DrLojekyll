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

struct Tup_u64 {
  uint64_t c0;
  auto operator<=>(const Tup_u64 &) const noexcept = default;
};

struct Tup_u64_u64 {
  uint64_t c0;
  uint64_t c1;
  auto operator<=>(const Tup_u64_u64 &) const noexcept = default;
};

using in_input = Tup_u64_u64;
using m_input = Tup_u64;
using something_input = Tup_i32;

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

// Rows of `out_13` (out/1).
struct Row13 {
  uint64_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row13 &) const noexcept = default;
};

// Rows of `out2_16` (out2/2).
struct Row16 {
  uint64_t x;
  uint64_t y;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, y);
  }
  bool operator==(const Row16 &) const noexcept = default;
};

// Rows of `proof_20` (proof/1).
struct Row20 {
  int32_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row20 &) const noexcept = default;
};

// Rows of `table_23`.
struct Row23 {
  uint64_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row23 &) const noexcept = default;
};

// Rows of `table_26`.
struct Row26 {
  uint64_t y;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(y);
  }
  bool operator==(const Row26 &) const noexcept = default;
};

// Rows of `table_29`.
struct Row29 {
  int32_t c0;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row29 &) const noexcept = default;
};

// Key of `idx_53` over `table_29`.
struct Key53 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key53 &) const noexcept = default;
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

  // Message `in/2`.
  bool in_2(::hyde::rt::Vec<Tup_u64_u64> vec58);

  // Message `m/1`.
  bool m_1(::hyde::rt::Vec<Tup_u64> vec63);

  // Message `something/1`.
  bool something_1(::hyde::rt::Vec<Tup_i32> vec68);

  // Query `out/1` (f).
  struct out_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(uint64_t &X);
  };
  out_f_cursor out_f();

  // Query `out2/2` (ff).
  struct out2_ff_cursor {
    Database &db;
    uint32_t pos;
    bool next(uint64_t &X, uint64_t &Y);
  };
  out2_ff_cursor out2_ff();

  // Query `proof/1` (f).
  struct proof_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  proof_f_cursor proof_f();

 private:
  bool init_4();
  bool proc_33(::hyde::rt::Vec<Tup_u64_u64> vec36, ::hyde::rt::Vec<Tup_u64> vec41, ::hyde::rt::Vec<Tup_i32> vec45);
  bool flow_76(::hyde::rt::Vec<Tup_b> vec35, ::hyde::rt::Vec<Tup_u64> vec40, ::hyde::rt::Vec<Tup_u64> vec44);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row6> __6;
  ::hyde::rt::Table<Row9> table_9;
  ::hyde::rt::Table<Row13> out_13;
  ::hyde::rt::Table<Row16> out2_16;
  ::hyde::rt::Table<Row20> proof_20;
  ::hyde::rt::Table<Row23> table_23;
  ::hyde::rt::Table<Row26> table_26;
  ::hyde::rt::Table<Row29> table_29;
  ::hyde::rt::Index<Key53> idx_53;

  uint64_t g34 = 0;
};

