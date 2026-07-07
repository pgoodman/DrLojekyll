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
  int32_t b;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, b);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Key of `idx_88` over `table_18`.
struct Key88 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Key88 &) const noexcept = default;
};

// Rows of `table_22`.
struct Row22 {
  int32_t m;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(m);
  }
  bool operator==(const Row22 &) const noexcept = default;
};

// Rows of `table_25`.
struct Row25 {
  int32_t a;
  int32_t b;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, b);
  }
  bool operator==(const Row25 &) const noexcept = default;
};

// Key of `idx_106` over `table_25`.
struct Key106 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Key106 &) const noexcept = default;
};

// Rows of `table_29`.
struct Row29 {
  int32_t b;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(b, c1);
  }
  bool operator==(const Row29 &) const noexcept = default;
};

// Key of `idx_112` over `table_29`.
struct Key112 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key112 &) const noexcept = default;
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
  bool base_1(::hyde::rt::Vec<Tup_i32> vec117);

  // Message `edge1/2`.
  bool edge1_2(::hyde::rt::Vec<Tup_i32_i32> vec123);

  // Message `marker/1`.
  bool marker_1(::hyde::rt::Vec<Tup_i32> vec129);

  // Message `edge2/2`.
  bool edge2_2(::hyde::rt::Vec<Tup_i32_i32> vec135);

  // Query `out/1` (b).
  bool out_b(int32_t A);

 private:
  bool init_4();
  bool proc_33(::hyde::rt::Vec<Tup_i32> vec35, ::hyde::rt::Vec<Tup_i32_i32> vec45, ::hyde::rt::Vec<Tup_i32> vec50, ::hyde::rt::Vec<Tup_i32_i32> vec54);
  bool flow_145(::hyde::rt::Vec<Tup_i32> vec39, ::hyde::rt::Vec<Tup_i32> vec42, ::hyde::rt::Vec<Tup_i32> vec53, ::hyde::rt::Vec<Tup_i32> vec62);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> __5;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row12> out_12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Index<Key88> idx_88;
  ::hyde::rt::Table<Row22> table_22;
  ::hyde::rt::Table<Row25> table_25;
  ::hyde::rt::Index<Key106> idx_106;
  ::hyde::rt::Table<Row29> table_29;
  ::hyde::rt::Index<Key112> idx_112;

  uint64_t g34 = 0;
};

