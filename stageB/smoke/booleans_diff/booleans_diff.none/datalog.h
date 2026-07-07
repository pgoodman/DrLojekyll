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

// Key of `idx_51` over `table_11`.
struct Key51 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key51 &) const noexcept = default;
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
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row21 &) const noexcept = default;
};

// Rows of `table_24`.
struct Row24 {
  bool c0;
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, id);
  }
  bool operator==(const Row24 &) const noexcept = default;
};

// Key of `idx_99` over `table_24`.
struct Key99 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Key99 &) const noexcept = default;
};

// Rows of `table_28`.
struct Row28 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row28 &) const noexcept = default;
};

// Rows of `table_31`.
struct Row31 {
  bool c0;
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, id);
  }
  bool operator==(const Row31 &) const noexcept = default;
};

// Rows of `user_is_logged_in_35` (user_is_logged_in/1).
struct Row35 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row35 &) const noexcept = default;
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
  bool add_user_1(::hyde::rt::Vec<Tup_i32> vec65);

  // Message `log_in/1`.
  bool log_in_1(::hyde::rt::Vec<Tup_i32> vec69, ::hyde::rt::Vec<Tup_i32> vec70);

  // Query `user_is_logged_in/1` (b).
  bool user_is_logged_in_b(int32_t UserId);

 private:
  bool init_4();
  bool proc_38(::hyde::rt::Vec<Tup_i32> vec40, ::hyde::rt::Vec<Tup_i32> vec45, ::hyde::rt::Vec<Tup_i32> vec46);
  bool find_76(int32_t v77);
  bool find_78(int32_t v79);
  bool find_82(int32_t v83);
  bool find_85(int32_t v86);
  bool find_88(int32_t v89);
  bool find_93(int32_t v94);
  bool find_96(int32_t v97);
  bool find_103(bool v104, int32_t v105);
  bool find_107(bool v108, int32_t v109);
  bool find_112(int32_t v113, bool v114);
  bool find_116(bool v117);
  bool find_119(bool v120);
  bool find_122(bool v123);
  bool find_127(bool v128);
  bool find_133(bool v134, int32_t v135);
  bool find_137(bool v138, int32_t v139);
  bool find_142(bool v143, int32_t v144);
  bool find_146(int32_t v147);
  bool find_149(int32_t v150);
  bool find_152(int32_t v153);
  bool find_157(int32_t v158);
  bool find_160(int32_t v161);
  bool find_163(int32_t v164);
  bool find_166(int32_t v167);
  bool find_169(int32_t v170);
  bool find_173(int32_t v174);
  bool find_176(int32_t v177);
  bool find_179(int32_t v180);
  bool find_184(int32_t v185);
  bool find_187(int32_t v188);
  bool flow_191(::hyde::rt::Vec<Tup_b> vec43, ::hyde::rt::Vec<Tup_i32> vec44);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row11> table_11;
  ::hyde::rt::Index<Key51> idx_51;
  ::hyde::rt::Table<Row15> __15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Table<Row21> table_21;
  ::hyde::rt::Table<Row24> table_24;
  ::hyde::rt::Index<Key99> idx_99;
  ::hyde::rt::Table<Row28> table_28;
  ::hyde::rt::Table<Row31> table_31;
  ::hyde::rt::Table<Row35> user_is_logged_in_35;

  uint64_t g39 = 0;
};

