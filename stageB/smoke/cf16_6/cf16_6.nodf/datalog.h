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

// Rows of `table_5`.
struct Row5 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row5 &) const noexcept = default;
};

// Rows of `table_8`.
struct Row8 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row8 &) const noexcept = default;
};

// Rows of `table_11`.
struct Row11 {
  int32_t id;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id, c1);
  }
  bool operator==(const Row11 &) const noexcept = default;
};

// Key of `idx_48` over `table_11`.
struct Key48 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key48 &) const noexcept = default;
};

// Rows of `__15` (/0).
struct Row15 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
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
  bool c0;
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, id);
  }
  bool operator==(const Row21 &) const noexcept = default;
};

// Key of `idx_96` over `table_21`.
struct Key96 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Key96 &) const noexcept = default;
};

// Rows of `table_25`.
struct Row25 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row25 &) const noexcept = default;
};

// Rows of `table_28`.
struct Row28 {
  bool c0;
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, id);
  }
  bool operator==(const Row28 &) const noexcept = default;
};

// Rows of `user_is_logged_in_32` (user_is_logged_in/1).
struct Row32 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row32 &) const noexcept = default;
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
  bool add_user_1(::hyde::rt::Vec<Tup_i32> vec62);

  // Message `log_in/1`.
  bool log_in_1(::hyde::rt::Vec<Tup_i32> vec66, ::hyde::rt::Vec<Tup_i32> vec67);

  // Query `user_is_logged_in/1` (b).
  bool user_is_logged_in_b(int32_t UserId);

 private:
  bool init_4();
  bool proc_35(::hyde::rt::Vec<Tup_i32> vec37, ::hyde::rt::Vec<Tup_i32> vec42, ::hyde::rt::Vec<Tup_i32> vec43);
  bool find_73(int32_t v74);
  bool find_75(int32_t v76);
  bool find_79(int32_t v80);
  bool find_82(int32_t v83);
  bool find_90(int32_t v91);
  bool find_93(int32_t v94);
  bool find_100(bool v101, int32_t v102);
  bool find_104(bool v105, int32_t v106);
  bool find_109(int32_t v110, bool v111);
  bool find_113(bool v114);
  bool find_116(bool v117);
  bool find_124(bool v125);
  bool find_130(bool v131, int32_t v132);
  bool find_134(bool v135, int32_t v136);
  bool find_139(bool v140, int32_t v141);
  bool find_143(int32_t v144);
  bool find_146(int32_t v147);
  bool find_154(int32_t v155);
  bool find_157(int32_t v158);
  bool find_160(int32_t v161);
  bool find_168(int32_t v169);
  bool flow_171(::hyde::rt::Vec<Tup_b> vec40, ::hyde::rt::Vec<Tup_i32> vec41);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row11> table_11;
  ::hyde::rt::Index<Key48> idx_48;
  ::hyde::rt::Table<Row15> __15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Table<Row21> table_21;
  ::hyde::rt::Index<Key96> idx_96;
  ::hyde::rt::Table<Row25> table_25;
  ::hyde::rt::Table<Row28> table_28;
  ::hyde::rt::Table<Row32> user_is_logged_in_32;

  uint64_t g36 = 0;
};

