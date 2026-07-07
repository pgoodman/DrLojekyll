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

struct Tup_i32_i32 {
  int32_t c0;
  int32_t c1;
  auto operator<=>(const Tup_i32_i32 &) const noexcept = default;
};

using base_input = Tup_i32;
using edge1_input = Tup_i32_i32;
using marker_input = Tup_i32;
using edge2_input = Tup_i32_i32;

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
  int32_t m;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, m);
  }
  bool operator==(const Row8 &) const noexcept = default;
};

// Rows of `out_12` (out/1).
struct Row12 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row12 &) const noexcept = default;
};

// Rows of `table_15`.
struct Row15 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row15 &) const noexcept = default;
};

// Rows of `table_18`.
struct Row18 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Rows of `table_21`.
struct Row21 {
  int32_t a;
  int32_t b;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, b);
  }
  bool operator==(const Row21 &) const noexcept = default;
};

// Key of `idx_96` over `table_21`.
struct Key96 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Key96 &) const noexcept = default;
};

// Rows of `table_25`.
struct Row25 {
  int32_t m;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(m);
  }
  bool operator==(const Row25 &) const noexcept = default;
};

// Rows of `table_28`.
struct Row28 {
  int32_t a;
  int32_t b;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, b);
  }
  bool operator==(const Row28 &) const noexcept = default;
};

// Key of `idx_102` over `table_28`.
struct Key102 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Key102 &) const noexcept = default;
};

// Rows of `table_32`.
struct Row32 {
  int32_t b;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(b, c1);
  }
  bool operator==(const Row32 &) const noexcept = default;
};

// Key of `idx_108` over `table_32`.
struct Key108 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key108 &) const noexcept = default;
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

  // Message `base/1`.
  bool base_1(::hyde::rt::Vec<Tup_i32> vec113);

  // Message `edge1/2`.
  bool edge1_2(::hyde::rt::Vec<Tup_i32_i32> vec119);

  // Message `marker/1`.
  bool marker_1(::hyde::rt::Vec<Tup_i32> vec125);

  // Message `edge2/2`.
  bool edge2_2(::hyde::rt::Vec<Tup_i32_i32> vec131);

  // Query `out/1` (b).
  bool out_b(int32_t A);

 private:
  bool init_4();
  bool proc_36(::hyde::rt::Vec<Tup_i32> vec38, ::hyde::rt::Vec<Tup_i32_i32> vec47, ::hyde::rt::Vec<Tup_i32> vec52, ::hyde::rt::Vec<Tup_i32_i32> vec56);
  bool flow_141(::hyde::rt::Vec<Tup_i32> vec42, ::hyde::rt::Vec<Tup_i32> vec44, ::hyde::rt::Vec<Tup_i32> vec55, ::hyde::rt::Vec<Tup_i32> vec62);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> __5;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row12> out_12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Table<Row21> table_21;
  ::hyde::rt::Index<Key96> idx_96;
  ::hyde::rt::Table<Row25> table_25;
  ::hyde::rt::Table<Row28> table_28;
  ::hyde::rt::Index<Key102> idx_102;
  ::hyde::rt::Table<Row32> table_32;
  ::hyde::rt::Index<Key108> idx_108;

  uint64_t g37 = 0;
};

