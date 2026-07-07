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

using add_item_input = Tup_i32;

// Rows of `table_5`.
struct Row5 {
  int32_t x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row5 &) const noexcept = default;
};

// Key of `idx_42` over `table_5`.
struct Key42 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key42 &) const noexcept = default;
};

// Rows of `__9` (/0).
struct Row9 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row9 &) const noexcept = default;
};

// Rows of `table_12`.
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
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row15 &) const noexcept = default;
};

// Rows of `table_18`.
struct Row18 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Key of `idx_93` over `table_18`.
struct Key93 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Key93 &) const noexcept = default;
};

// Rows of `table_22`.
struct Row22 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row22 &) const noexcept = default;
};

// Rows of `table_26`.
struct Row26 {
  bool c0;
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row26 &) const noexcept = default;
};

// Rows of `gated_30` (gated/1).
struct Row30 {
  int32_t x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row30 &) const noexcept = default;
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

  // Message `add_a/1`.
  bool add_a_1(::hyde::rt::Vec<Tup_i32> vec64, ::hyde::rt::Vec<Tup_i32> vec65);

  // Message `add_b/1`.
  bool add_b_1(::hyde::rt::Vec<Tup_i32> vec70, ::hyde::rt::Vec<Tup_i32> vec71);

  // Message `add_item/1`.
  bool add_item_1(::hyde::rt::Vec<Tup_i32> vec76);

  // Query `gated/1` (b).
  bool gated_b(int32_t X);

 private:
  bool init_4();
  bool proc_33(::hyde::rt::Vec<Tup_i32> vec35, ::hyde::rt::Vec<Tup_i32> vec36, ::hyde::rt::Vec<Tup_i32> vec46, ::hyde::rt::Vec<Tup_i32> vec47, ::hyde::rt::Vec<Tup_i32> vec55);
  bool find_84(int32_t v85);
  bool find_86(int32_t v87);
  bool find_90(int32_t v91);
  bool find_97(bool v98, int32_t v99);
  bool find_101(bool v102, int32_t v103);
  bool find_106(int32_t v107, bool v108);
  bool find_110(bool v111);
  bool find_113(bool v114);
  bool find_121(bool v122);
  bool find_127(bool v128, int32_t v129);
  bool find_134(bool v135, int32_t v136);
  bool find_138(bool v139, int32_t v140);
  bool find_143(int32_t v144);
  bool find_146(int32_t v147);
  bool find_154(bool v155, int32_t v156);
  bool find_159(int32_t v160);
  bool flow_170(::hyde::rt::Vec<Tup_b> vec39);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key42> idx_42;
  ::hyde::rt::Table<Row9> __9;
  ::hyde::rt::Table<Row12> table_12;
  ::hyde::rt::Table<Row15> table_15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Index<Key93> idx_93;
  ::hyde::rt::Table<Row22> table_22;
  ::hyde::rt::Table<Row26> table_26;
  ::hyde::rt::Table<Row30> gated_30;

  uint64_t g34 = 0;
};

