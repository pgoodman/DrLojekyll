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

struct Tup_i32 {
  int32_t c0;
  auto operator<=>(const Tup_i32 &) const noexcept = default;
};

using in_input = Tup_i32;

// Rows of `out_neg_5` (out_neg/1).
struct Row5 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row5 &) const noexcept = default;
};

// Rows of `table_18`.
struct Row18 {
  int32_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Rows of `table_21`.
struct Row21 {
  int32_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row21 &) const noexcept = default;
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

  // Message `in/1`.
  bool in_1(::hyde::rt::Vec<Tup_i32> vec14);

  // Query `out_neg/1` (f).
  struct out_neg_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  out_neg_f_cursor out_neg_f();

  // Query `out_pos/1` (f).
  struct out_pos_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  out_pos_f_cursor out_pos_f();

  // Query `out_chain/1` (f).
  struct out_chain_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  out_chain_f_cursor out_chain_f();

 private:
  bool init_4();
  bool proc_8(::hyde::rt::Vec<Tup_i32> vec10);
  bool flow_24();

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> out_neg_5;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Table<Row21> table_21;

  uint64_t g9 = 0;
};

