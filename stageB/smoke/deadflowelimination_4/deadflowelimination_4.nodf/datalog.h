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

using in_input = Tup_i32;

// Rows of `table_5`.
struct Row5 {
  bool c0;
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, a);
  }
  bool operator==(const Row5 &) const noexcept = default;
};

// Key of `idx_111` over `table_5`.
struct Key111 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Key111 &) const noexcept = default;
};

// Rows of `table_9`.
struct Row9 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row9 &) const noexcept = default;
};

// Rows of `out_neg_12` (out_neg/1).
struct Row12 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row12 &) const noexcept = default;
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
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Rows of `__22` (/0).
struct Row22 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row22 &) const noexcept = default;
};

// Rows of `table_25`.
struct Row25 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row25 &) const noexcept = default;
};

// Rows of `out_pos_29` (out_pos/1).
struct Row29 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row29 &) const noexcept = default;
};

// Rows of `out_chain_32` (out_chain/1).
struct Row32 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row32 &) const noexcept = default;
};

// Rows of `table_35`.
struct Row35 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row35 &) const noexcept = default;
};

// Rows of `table_38`.
struct Row38 {
  int32_t a;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row38 &) const noexcept = default;
};

// Key of `idx_73` over `table_38`.
struct Key73 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key73 &) const noexcept = default;
};

// Rows of `table_42`.
struct Row42 {
  int32_t c0;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row42 &) const noexcept = default;
};

// Key of `idx_67` over `table_42`.
struct Key67 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key67 &) const noexcept = default;
};

// Rows of `table_46`.
struct Row46 {
  int32_t a;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row46 &) const noexcept = default;
};

// Key of `idx_79` over `table_46`.
struct Key79 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key79 &) const noexcept = default;
};

// Rows of `table_50`.
struct Row50 {
  int32_t a;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, c1);
  }
  bool operator==(const Row50 &) const noexcept = default;
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
  bool in_1(::hyde::rt::Vec<Tup_i32> vec84);

  // Query `out_pos/1` (f).
  struct out_pos_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  out_pos_f_cursor out_pos_f();

  // Query `out_neg/1` (f).
  struct out_neg_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  out_neg_f_cursor out_neg_f();

  // Query `out_chain/1` (f).
  struct out_chain_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A);
  };
  out_chain_f_cursor out_chain_f();

 private:
  bool init_4();
  bool proc_54(::hyde::rt::Vec<Tup_i32> vec56);
  bool find_60(bool v61);
  bool find_88(int32_t v89);
  bool find_90(int32_t v91);
  bool find_94(int32_t v95);
  bool find_97(int32_t v98);
  bool find_105(int32_t v106);
  bool find_108(int32_t v109);
  bool find_115(bool v116, int32_t v117);
  bool find_119(bool v120, int32_t v121);
  bool find_124(int32_t v125, bool v126);
  bool flow_129(::hyde::rt::Vec<Tup_b> vec59, ::hyde::rt::Vec<Tup_b> vec63, ::hyde::rt::Vec<Tup_b> vec64);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row5> table_5;
  ::hyde::rt::Index<Key111> idx_111;
  ::hyde::rt::Table<Row9> table_9;
  ::hyde::rt::Table<Row12> out_neg_12;
  ::hyde::rt::Table<Row15> __15;
  ::hyde::rt::Table<Row18> table_18;
  ::hyde::rt::Table<Row22> __22;
  ::hyde::rt::Table<Row25> table_25;
  ::hyde::rt::Table<Row29> out_pos_29;
  ::hyde::rt::Table<Row32> out_chain_32;
  ::hyde::rt::Table<Row35> table_35;
  ::hyde::rt::Table<Row38> table_38;
  ::hyde::rt::Index<Key73> idx_73;
  ::hyde::rt::Table<Row42> table_42;
  ::hyde::rt::Index<Key67> idx_67;
  ::hyde::rt::Table<Row46> table_46;
  ::hyde::rt::Index<Key79> idx_79;
  ::hyde::rt::Table<Row50> table_50;

  uint64_t g55 = 0;
};

