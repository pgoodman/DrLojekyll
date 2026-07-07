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

// Rows of `proof_13` (proof/1).
struct Row13 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row13 &) const noexcept = default;
};

// Rows of `table_16`.
struct Row16 {
  int32_t c0;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row16 &) const noexcept = default;
};

// Key of `idx_28` over `table_16`.
struct Key28 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key28 &) const noexcept = default;
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

  // Message `something/1`.
  bool something_1(::hyde::rt::Vec<Tup_i32> vec33);

  // Query `proof/1` (f).
  struct proof_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  proof_f_cursor proof_f();

 private:
  bool init_4();
  bool proc_20(::hyde::rt::Vec<Tup_i32> vec23);
  bool flow_37(::hyde::rt::Vec<Tup_b> vec22);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row6> __6;
  ::hyde::rt::Table<Row9> table_9;
  ::hyde::rt::Table<Row13> proof_13;
  ::hyde::rt::Table<Row16> table_16;
  ::hyde::rt::Index<Key28> idx_28;

  uint64_t g21 = 0;
};

