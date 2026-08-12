// Auto-generated file; do not edit.

#pragma once

#include <drlojekyll/Runtime/Allocator.h>
#include <drlojekyll/Runtime/Hash.h>
#include <drlojekyll/Runtime/Table.h>
#include <drlojekyll/Runtime/InstanceStore.h>
#include <drlojekyll/Runtime/Vec.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

struct Tup_u64 {
  uint64_t c0;
  auto operator<=>(const Tup_u64 &) const noexcept = default;
};

struct Tup_u64_u64 {
  uint64_t c0;
  uint64_t c1;
  auto operator<=>(const Tup_u64_u64 &) const noexcept = default;
};

using add_edge_input = Tup_u64_u64;

// Rows of `neighborhood_4` (neighborhood/2).
struct Row4 {
  uint64_t from;
  uint64_t to;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(from, to);
  }
  bool operator==(const Row4 &) const noexcept = default;
};

// Key of `idx_41` over `neighborhood_4`.
struct Key41 {
  uint64_t from;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(from);
  }
  bool operator==(const Key41 &) const noexcept = default;
};

// Rows of `table_8`.
struct Row8 {
  uint64_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row8 &) const noexcept = default;
};

// Rows of `table_11`.
struct Row11 {
  uint64_t from;
  uint64_t to;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(from, to);
  }
  bool operator==(const Row11 &) const noexcept = default;
};

// InstanceStore #0 key (the demanded alpha).
struct Key_0 {
  uint64_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Key_0 &) const noexcept = default;
};

// InstanceStore #0 published row.
struct Row_0 {
  uint64_t c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row_0 &) const noexcept = default;
};

// User-provided functors. Define the declared member functions in
// your own translation unit; the generated code calls them.
struct DatabaseFunctors {
};

// Receives published messages. The default methods do nothing.
// Entry points deduce the log's static type, so any type
// providing the same member signatures observes the published
// delta stream (a @differential message publishes one call per
// net presence change at each batch's commit sweep) with no
// virtual dispatch.
struct DatabaseLog {
};

// Internal flow procedures. Not driver-facing. Each signature
// names exactly the state the procedure reads and writes.
inline bool init_3(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0);
inline bool proc_15(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::Vec<Tup_u64_u64> vec17, ::hyde::rt::Vec<Tup_u64> vec22);
inline bool add_edge_2_detail(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::Vec<Tup_u64_u64> vec27);
inline bool demand__neighborhood_bf_1_detail(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::Vec<Tup_u64> vec31);
inline bool inject_37(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, uint64_t v38);
inline bool flow_42(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::Vec<Tup_u64_u64> vec21, ::hyde::rt::Vec<Tup_u64> vec25);

// The sealed database state: tables, indices, and epoch counters.
// Construction allocates empty tables and cannot fail; epoch 0
// (the empty-program fixpoint) runs when the driver calls
// `init(db, log, functors)`, and every entry point asserts it has
// run. All driver-facing functions are hidden friends: reach them
// by unqualified call with the database as an argument.
struct Database {
 public:
  explicit Database(::hyde::rt::Allocator allocator_)
    : allocator(allocator_),
      neighborhood_4(allocator_),
      idx_41(allocator_),
      table_8(allocator_),
      table_11(allocator_),
      instance_0(allocator_) {}

  // Epoch 0: derives the empty-EDB least model and publishes its
  // t=0 deltas to `log`. Call exactly once, before any message.
  template <typename Log, typename Functors>
  friend auto init(Database &db, Log &, Functors &) {
    assert(!db.initialized_);
    db.initialized_ = true;
    return init_3(db.allocator, db.neighborhood_4, db.idx_41, db.table_8, db.table_11, db.g16, db.instance_0);
  }

  // Message `add_edge/2`.
  template <typename Log, typename Functors>
  friend auto add_edge_2(Database &db, Log &, Functors &, ::hyde::rt::Vec<Tup_u64_u64> vec27) {
    assert(db.initialized_);
    return add_edge_2_detail(db.allocator, db.neighborhood_4, db.idx_41, db.table_8, db.table_11, db.g16, db.instance_0, std::move(vec27));
  }

  // Query `neighborhood/2` (bf).
  struct neighborhood_bf_cursor {
    Database &db;
    uint64_t Start;
    uint32_t pos;
    bool next(uint64_t &Node) {
      while (pos != ::hyde::rt::kNoRow) {
        const uint32_t id = pos;
        pos = db.idx_41.Next(id);
        const auto row = db.neighborhood_4.RowAt(id);
        Node = row.to;
        return true;
      }
      return false;
    }
  };
  template <typename Log, typename Functors>
  friend neighborhood_bf_cursor neighborhood_bf(Database &db, Log &, Functors &, uint64_t Start) {
    assert(db.initialized_);
    inject_37(db.allocator, db.neighborhood_4, db.idx_41, db.table_8, db.table_11, db.g16, db.instance_0, Start);
    return {db, Start, db.idx_41.First({Start})};
  }

 private:
  ::hyde::rt::Allocator allocator;

  ::hyde::rt::Table<Row4> neighborhood_4;
  ::hyde::rt::Index<Key41> idx_41;
  ::hyde::rt::Table<Row8> table_8;
  ::hyde::rt::Table<Row11> table_11;

  ::hyde::rt::InstanceStore<Key_0, Row_0> instance_0;

  uint64_t g16 = 0;
  bool initialized_ = false;
};

inline bool init_3(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0) {
  ::hyde::rt::Vec<Tup_u64_u64> vec35(allocator);
  ::hyde::rt::Vec<Tup_u64> vec36(allocator);
  proc_15(allocator, neighborhood_4, idx_41, table_8, table_11, g16, instance_0, std::move(vec35), std::move(vec36));
  return false;
}

inline bool proc_15(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::Vec<Tup_u64_u64> vec17, ::hyde::rt::Vec<Tup_u64> vec22) {
  ::hyde::rt::Vec<Tup_u64_u64> vec21(allocator);
  ::hyde::rt::Vec<Tup_u64> vec25(allocator);
  for (auto [v19, v20] : vec17) {
    if (const auto ins0 = table_11.TryAdd({v19, v20}); ins0.added) {
      vec21.Add({v19, v20});
    }
  }
  for (auto [v24] : vec22) {
    if (const auto ins1 = table_8.TryAdd({v24}); ins1.added) {
      vec25.Add({v24});
    }
  }
  vec17.Clear();
  vec22.Clear();
  flow_42(allocator, neighborhood_4, idx_41, table_8, table_11, g16, instance_0, std::move(vec21), std::move(vec25));
  return false;
}

inline bool add_edge_2_detail(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::Vec<Tup_u64_u64> vec27) {
  ::hyde::rt::Vec<Tup_u64> vec29(allocator);
  proc_15(allocator, neighborhood_4, idx_41, table_8, table_11, g16, instance_0, std::move(vec27), std::move(vec29));
  return true;
}

inline bool demand__neighborhood_bf_1_detail(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::Vec<Tup_u64> vec31) {
  ::hyde::rt::Vec<Tup_u64_u64> vec33(allocator);
  proc_15(allocator, neighborhood_4, idx_41, table_8, table_11, g16, instance_0, std::move(vec33), std::move(vec31));
  return true;
}

inline bool inject_37(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, uint64_t v38) {
  ::hyde::rt::Vec<Tup_u64> vec39(allocator);
  vec39.Add({v38});
  demand__neighborhood_bf_1_detail(allocator, neighborhood_4, idx_41, table_8, table_11, g16, instance_0, std::move(vec39));
  return true;
}

inline bool flow_42(::hyde::rt::Allocator &allocator, ::hyde::rt::Table<Row4> &neighborhood_4, ::hyde::rt::Index<Key41> &idx_41, ::hyde::rt::Table<Row8> &table_8, ::hyde::rt::Table<Row11> &table_11, uint64_t &g16, ::hyde::rt::InstanceStore<Key_0, Row_0> &instance_0, ::hyde::rt::Vec<Tup_u64_u64> vec21, ::hyde::rt::Vec<Tup_u64> vec25) {
  g16 += 1;
  for (const auto &[k0] : vec25) {
    const auto iid = instance_0.FindOrAddInstance(Key_0{k0});
    if (!instance_0.TouchedFlag(iid)) {
      if (instance_0.WorkingOccupied(iid)) {
        std::fprintf(stderr, "V-INST-FRESH: instance %u current non-empty at band-(a) entry (store 0)\n", iid); std::abort();
      }
      auto &cur = instance_0.TouchCurrent(iid);
      for (uint32_t s = 0; s < table_11.NumRows(); ++s) {
        const auto ir = table_11.RowAt(s);
        if (ir.from == k0) {
          cur.TryAdd(Row_0{ir.to});
        }
      }
    }
  }
  for (const auto &[e0, e1] : vec21) {
    const auto iid = instance_0.FindInstance(Key_0{e0});
    if (iid != ::hyde::rt::kNoInstance && !instance_0.TouchedFlag(iid)) {
      if (instance_0.WorkingOccupied(iid)) {
        std::fprintf(stderr, "V-INST-FRESH: instance %u current non-empty at band-(a) entry (store 0)\n", iid); std::abort();
      }
      auto &cur = instance_0.TouchCurrent(iid);
      for (uint32_t s = 0; s < table_11.NumRows(); ++s) {
        const auto ir = table_11.RowAt(s);
        if (ir.from == e0) {
          cur.TryAdd(Row_0{ir.to});
        }
      }
    }
  }
  for (const auto iid : instance_0.Touched()) {
    auto &cur = instance_0.Current(iid);
    const auto &frz = instance_0.Frozen(iid);
    const auto &key = instance_0.KeyAt(iid);
    (void) key;
    for (uint32_t r = 0; r < cur.NumRows(); ++r) {
      const auto &row = cur.RowAt(r);
      if (frz.Find(row) == ::hyde::rt::kNoRow) {
        if (const auto ins2 = neighborhood_4.TryAdd({key.c0, row.c0}); ins2.added) {
          idx_41.Add({key.c0}, ins2.id);
        }
      }
    }
  }
  instance_0.Seal();
#ifndef NDEBUG
  instance_0.DebugValidate();
#endif
  table_8.Seal();
  table_11.Seal();
  return true;
}

