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

// Key of `idx_45` over `table_11`.
struct Key45 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key45 &) const noexcept = default;
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

// Key of `idx_79` over `table_21`.
struct Key79 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Key79 &) const noexcept = default;
};

// Rows of `table_25`.
struct Row25 {
  bool c0;
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, id);
  }
  bool operator==(const Row25 &) const noexcept = default;
};

// Rows of `user_is_logged_in_29` (user_is_logged_in/1).
struct Row29 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row29 &) const noexcept = default;
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
  bool add_user_1(::hyde::rt::Vec<Tup_i32> vec59);

  // Message `log_in/1`.
  bool log_in_1(::hyde::rt::Vec<Tup_i32> vec63, ::hyde::rt::Vec<Tup_i32> vec64);

  // Query `user_is_logged_in/1` (b).
  bool user_is_logged_in_b(int32_t UserId);

 private:
  bool init_4();
  bool proc_32(::hyde::rt::Vec<Tup_i32> vec34, ::hyde::rt::Vec<Tup_i32> vec39, ::hyde::rt::Vec<Tup_i32> vec40);
  bool find_70(int32_t v71);
  bool find_72(int32_t v73);
  bool find_76(int32_t v77);
  bool find_83(bool v84, int32_t v85);
  bool find_87(bool v88, int32_t v89);
  bool find_92(int32_t v93, bool v94);
  bool find_96(bool v97);
  bool find_99(bool v100);
  bool find_107(bool v108);
  bool find_113(bool v114, int32_t v115);
  bool find_117(bool v118, int32_t v119);
  bool find_122(int32_t v123);
  bool find_125(int32_t v126);
  bool find_133(int32_t v134);
  bool find_136(int32_t v137);
  bool find_139(int32_t v140);
  bool find_147(int32_t v148);
  bool flow_150(::hyde::rt::Vec<Tup_b> vec37, ::hyde::rt::Vec<Tup_i32> vec38);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row11> table_11;
  ::hyde::rt::Index<Key45> idx_45;
  ::hyde::rt::Table<Row15> __15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Table<Row21> table_21;
  ::hyde::rt::Index<Key79> idx_79;
  ::hyde::rt::Table<Row25> table_25;
  ::hyde::rt::Table<Row29> user_is_logged_in_29;

  uint64_t g33 = 0;
};

