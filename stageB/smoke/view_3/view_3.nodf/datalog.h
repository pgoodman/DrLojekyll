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
using a_input = Tup_i32;
using b_input = Tup_i32;

// Rows of `__7` (/0).
struct Row7 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row7 &) const noexcept = default;
};

// Rows of `table_10`.
struct Row10 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row10 &) const noexcept = default;
};

// Rows of `__14` (/0).
struct Row14 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row14 &) const noexcept = default;
};

// Rows of `table_17`.
struct Row17 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row17 &) const noexcept = default;
};

// Rows of `table_21`.
struct Row21 {
  bool c0;
  int32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row21 &) const noexcept = default;
};

// Rows of `proof_25` (proof/1).
struct Row25 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row25 &) const noexcept = default;
};

// Rows of `q_shared_28` (q_shared/1).
struct Row28 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row28 &) const noexcept = default;
};

// Rows of `q_fact_31` (q_fact/2).
struct Row31 {
  int32_t a;
  int32_t b;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a, b);
  }
  bool operator==(const Row31 &) const noexcept = default;
};

// Rows of `table_35`.
struct Row35 {
  int32_t c0;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row35 &) const noexcept = default;
};

// Key of `idx_64` over `table_35`.
struct Key64 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key64 &) const noexcept = default;
};

// Rows of `table_39`.
struct Row39 {
  int32_t c0;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row39 &) const noexcept = default;
};

// Key of `idx_58` over `table_39`.
struct Key58 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key58 &) const noexcept = default;
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
  bool something_1(::hyde::rt::Vec<Tup_i32> vec69);

  // Message `a/1`.
  bool a_1(::hyde::rt::Vec<Tup_i32> vec74);

  // Message `b/1`.
  bool b_1(::hyde::rt::Vec<Tup_i32> vec79);

  // Query `proof/1` (f).
  struct proof_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  proof_f_cursor proof_f();

  // Query `q_shared/1` (f).
  struct q_shared_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &X);
  };
  q_shared_f_cursor q_shared_f();

  // Query `q_fact/2` (ff).
  struct q_fact_ff_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A, int32_t &B);
  };
  q_fact_ff_cursor q_fact_ff();

 private:
  bool init_4();
  bool proc_43(::hyde::rt::Vec<Tup_i32> vec47, ::hyde::rt::Vec<Tup_i32> vec50, ::hyde::rt::Vec<Tup_i32> vec53);
  bool flow_87(::hyde::rt::Vec<Tup_b> vec45, ::hyde::rt::Vec<Tup_b> vec46);

  ::hyde::rt::Allocator allocator;
  [[maybe_unused]] DatabaseLog &log;
  [[maybe_unused]] DatabaseFunctors &functors;

  ::hyde::rt::Table<Row7> __7;
  ::hyde::rt::Table<Row10> table_10;
  ::hyde::rt::Table<Row14> __14;
  ::hyde::rt::Table<Row17> table_17;
  ::hyde::rt::Table<Row21> table_21;
  ::hyde::rt::Table<Row25> proof_25;
  ::hyde::rt::Table<Row28> q_shared_28;
  ::hyde::rt::Table<Row31> q_fact_31;
  ::hyde::rt::Table<Row35> table_35;
  ::hyde::rt::Index<Key64> idx_64;
  ::hyde::rt::Table<Row39> table_39;
  ::hyde::rt::Index<Key58> idx_58;

  uint64_t g44 = 0;
};

