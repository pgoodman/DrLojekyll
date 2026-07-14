// Auto-generated file; do not edit.

#pragma once

#include <drlojekyll/Runtime/Allocator.h>
#include <drlojekyll/Runtime/Hash.h>
#include <drlojekyll/Runtime/Table.h>
#include <drlojekyll/Runtime/Vec.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

struct Tup_b {
  bool c0;
  auto operator<=>(const Tup_b &) const noexcept = default;
};

struct Tup_b_b {
  bool c0;
  bool c1;
  auto operator<=>(const Tup_b_b &) const noexcept = default;
};

struct Tup_u32 {
  uint32_t c0;
  auto operator<=>(const Tup_u32 &) const noexcept = default;
};

using enable_feature_input = Tup_u32;

// Rows of `table_8`.
struct Row8 {
  bool x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row8 &) const noexcept = default;
};

// Rows of `table_11`.
struct Row11 {
  bool x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row11 &) const noexcept = default;
};

// Rows of `table_14`.
struct Row14 {
  bool c0;
  bool x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row14 &) const noexcept = default;
};

// Rows of `table_18`.
struct Row18 {
  bool c0;
  bool x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row18 &) const noexcept = default;
};

// Rows of `table_22`.
struct Row22 {
  bool fooenabled;
  bool barenabled;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(fooenabled, barenabled);
  }
  bool operator==(const Row22 &) const noexcept = default;
};

// Rows of `table_26`.
struct Row26 {
  bool x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row26 &) const noexcept = default;
};

// Rows of `table_29`.
struct Row29 {
  bool x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row29 &) const noexcept = default;
};

// Rows of `table_32`.
struct Row32 {
  bool x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row32 &) const noexcept = default;
};

// Rows of `table_35`.
struct Row35 {
  bool x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x);
  }
  bool operator==(const Row35 &) const noexcept = default;
};

// Rows of `__38` (/0).
struct Row38 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row38 &) const noexcept = default;
};

// Rows of `table_41`.
struct Row41 {
  bool c0;
  uint32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row41 &) const noexcept = default;
};

// Rows of `__45` (/0).
struct Row45 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Row45 &) const noexcept = default;
};

// Rows of `table_48`.
struct Row48 {
  bool c0;
  uint32_t c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, c1);
  }
  bool operator==(const Row48 &) const noexcept = default;
};

// Rows of `table_52`.
struct Row52 {
  bool x;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(x, c1);
  }
  bool operator==(const Row52 &) const noexcept = default;
};

// Key of `idx_77` over `table_52`.
struct Key77 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key77 &) const noexcept = default;
};

// Rows of `table_56`.
struct Row56 {
  bool c0;
  bool x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row56 &) const noexcept = default;
};

// Key of `idx_88` over `table_56`.
struct Key88 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Key88 &) const noexcept = default;
};

// Rows of `table_60`.
struct Row60 {
  bool c0;
  bool x;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, x);
  }
  bool operator==(const Row60 &) const noexcept = default;
};

// Key of `idx_131` over `table_60`.
struct Key131 {
  bool c0;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0);
  }
  bool operator==(const Key131 &) const noexcept = default;
};

// User-provided functors. Define the declared member functions in
// your own translation unit; the generated code calls them.
struct DatabaseFunctors {
};

// Receives published messages. The default methods do nothing. Entry
// points deduce the log's static type, so any type providing the same
// member signatures observes the published delta stream (a
// @differential message publishes one call per net presence change at
// each batch's commit sweep) with no virtual dispatch.
struct DatabaseLog {
  void enabled_features_2(bool FooEnabled, bool BarEnabled, bool added) {}
};

// Internal flow procedures. Not driver-facing: call the entry points
// declared inside `Database` instead. Each signature names exactly the
// state the procedure reads and writes.
template <typename Log>
bool init_3(::hyde::rt::Allocator &allocator, Log &log,
            ::hyde::rt::DiffTable<Row8> &table_8,
            ::hyde::rt::DiffTable<Row11> &table_11,
            ::hyde::rt::DiffTable<Row14> &table_14,
            ::hyde::rt::DiffTable<Row18> &table_18,
            ::hyde::rt::DiffTable<Row22> &table_22,
            ::hyde::rt::Table<Row26> &table_26,
            ::hyde::rt::DiffTable<Row29> &table_29,
            ::hyde::rt::Table<Row32> &table_32,
            ::hyde::rt::DiffTable<Row35> &table_35,
            ::hyde::rt::Table<Row38> &__38,
            ::hyde::rt::Table<Row41> &table_41,
            ::hyde::rt::Table<Row45> &__45,
            ::hyde::rt::Table<Row48> &table_48,
            ::hyde::rt::Table<Row52> &table_52,
            ::hyde::rt::Index<Key77> &idx_77,
            ::hyde::rt::Table<Row56> &table_56,
            ::hyde::rt::Index<Key88> &idx_88,
            ::hyde::rt::Table<Row60> &table_60,
            ::hyde::rt::Index<Key131> &idx_131,
            uint64_t &g65);
template <typename Log>
bool proc_64(::hyde::rt::Allocator &allocator, Log &log,
             ::hyde::rt::DiffTable<Row8> &table_8,
             ::hyde::rt::DiffTable<Row11> &table_11,
             ::hyde::rt::DiffTable<Row14> &table_14,
             ::hyde::rt::DiffTable<Row18> &table_18,
             ::hyde::rt::DiffTable<Row22> &table_22,
             ::hyde::rt::Table<Row26> &table_26,
             ::hyde::rt::DiffTable<Row29> &table_29,
             ::hyde::rt::Table<Row32> &table_32,
             ::hyde::rt::DiffTable<Row35> &table_35,
             ::hyde::rt::Table<Row38> &__38,
             ::hyde::rt::Table<Row41> &table_41,
             ::hyde::rt::Table<Row45> &__45,
             ::hyde::rt::Table<Row48> &table_48,
             ::hyde::rt::Table<Row52> &table_52,
             ::hyde::rt::Index<Key77> &idx_77,
             ::hyde::rt::Table<Row56> &table_56,
             ::hyde::rt::Index<Key88> &idx_88,
             ::hyde::rt::Table<Row60> &table_60,
             ::hyde::rt::Index<Key131> &idx_131,
             uint64_t &g65,
             ::hyde::rt::Vec<Tup_u32> vec70);
template <typename Log>
bool flow_251(::hyde::rt::Allocator &allocator, Log &log,
              ::hyde::rt::DiffTable<Row8> &table_8,
              ::hyde::rt::DiffTable<Row11> &table_11,
              ::hyde::rt::DiffTable<Row14> &table_14,
              ::hyde::rt::DiffTable<Row18> &table_18,
              ::hyde::rt::DiffTable<Row22> &table_22,
              ::hyde::rt::Table<Row26> &table_26,
              ::hyde::rt::DiffTable<Row29> &table_29,
              ::hyde::rt::Table<Row32> &table_32,
              ::hyde::rt::DiffTable<Row35> &table_35,
              ::hyde::rt::Table<Row38> &__38,
              ::hyde::rt::Table<Row45> &__45,
              ::hyde::rt::Table<Row52> &table_52,
              ::hyde::rt::Index<Key77> &idx_77,
              ::hyde::rt::Table<Row56> &table_56,
              ::hyde::rt::Index<Key88> &idx_88,
              ::hyde::rt::Table<Row60> &table_60,
              ::hyde::rt::Index<Key131> &idx_131,
              uint64_t &g65,
              ::hyde::rt::Vec<Tup_b> vec66,
              ::hyde::rt::Vec<Tup_b> vec67,
              ::hyde::rt::Vec<Tup_b> vec73,
              ::hyde::rt::Vec<Tup_b> vec74);

// The sealed database state: tables, indices, and epoch counters.
// Construction allocates empty tables and cannot fail; epoch 0 (the
// empty-program fixpoint) runs when the driver calls `init(db, log,
// functors)`, and every message entry point asserts it has run. All
// driver-facing functions are hidden friends: reach them by unqualified
// call with the database as an argument.
struct Database {
 public:
  explicit Database(::hyde::rt::Allocator allocator_)
      : allocator(allocator_),
        table_8(allocator_),
        table_11(allocator_),
        table_14(allocator_),
        table_18(allocator_),
        table_22(allocator_),
        table_26(allocator_),
        table_29(allocator_),
        table_32(allocator_),
        table_35(allocator_),
        __38(allocator_),
        table_41(allocator_),
        __45(allocator_),
        table_48(allocator_),
        table_52(allocator_),
        idx_77(allocator_),
        table_56(allocator_),
        idx_88(allocator_),
        table_60(allocator_),
        idx_131(allocator_) {}

  // Epoch 0: derives the empty-EDB least model and publishes its
  // t=0 deltas to `log`. Call exactly once, before any message.
  template <typename Log, typename Functors>
  friend auto init(Database &db, Log &log, Functors &) {
    assert(!db.initialized_);
    db.initialized_ = true;
    return init_3(db.allocator, log, db.table_8, db.table_11,
                  db.table_14, db.table_18, db.table_22, db.table_26,
                  db.table_29, db.table_32, db.table_35, db.__38,
                  db.table_41, db.__45, db.table_48, db.table_52,
                  db.idx_77, db.table_56, db.idx_88, db.table_60,
                  db.idx_131, db.g65);
  }

  // Message `enable_feature/1`.
  template <typename Log, typename Functors>
  friend auto enable_feature_1(Database &db, Log &log, Functors &,
                               ::hyde::rt::Vec<Tup_u32> vec247) {
    assert(db.initialized_);
    proc_64(db.allocator, log, db.table_8, db.table_11, db.table_14,
            db.table_18, db.table_22, db.table_26, db.table_29,
            db.table_32, db.table_35, db.__38, db.table_41, db.__45,
            db.table_48, db.table_52, db.idx_77, db.table_56, db.idx_88,
            db.table_60, db.idx_131, db.g65, std::move(vec247));
    return true;
  }

 private:
  ::hyde::rt::Allocator allocator;

  ::hyde::rt::DiffTable<Row8> table_8;
  ::hyde::rt::DiffTable<Row11> table_11;
  ::hyde::rt::DiffTable<Row14> table_14;
  ::hyde::rt::DiffTable<Row18> table_18;
  ::hyde::rt::DiffTable<Row22> table_22;
  ::hyde::rt::Table<Row26> table_26;
  ::hyde::rt::DiffTable<Row29> table_29;
  ::hyde::rt::Table<Row32> table_32;
  ::hyde::rt::DiffTable<Row35> table_35;
  ::hyde::rt::Table<Row38> __38;
  ::hyde::rt::Table<Row41> table_41;
  ::hyde::rt::Table<Row45> __45;
  ::hyde::rt::Table<Row48> table_48;
  ::hyde::rt::Table<Row52> table_52;
  ::hyde::rt::Index<Key77> idx_77;
  ::hyde::rt::Table<Row56> table_56;
  ::hyde::rt::Index<Key88> idx_88;
  ::hyde::rt::Table<Row60> table_60;
  ::hyde::rt::Index<Key131> idx_131;

  uint64_t g65 = 0;
  bool initialized_ = false;
};

template <typename Log>
bool init_3(::hyde::rt::Allocator &allocator, Log &log,
            ::hyde::rt::DiffTable<Row8> &table_8,
            ::hyde::rt::DiffTable<Row11> &table_11,
            ::hyde::rt::DiffTable<Row14> &table_14,
            ::hyde::rt::DiffTable<Row18> &table_18,
            ::hyde::rt::DiffTable<Row22> &table_22,
            ::hyde::rt::Table<Row26> &table_26,
            ::hyde::rt::DiffTable<Row29> &table_29,
            ::hyde::rt::Table<Row32> &table_32,
            ::hyde::rt::DiffTable<Row35> &table_35,
            ::hyde::rt::Table<Row38> &__38,
            ::hyde::rt::Table<Row41> &table_41,
            ::hyde::rt::Table<Row45> &__45,
            ::hyde::rt::Table<Row48> &table_48,
            ::hyde::rt::Table<Row52> &table_52,
            ::hyde::rt::Index<Key77> &idx_77,
            ::hyde::rt::Table<Row56> &table_56,
            ::hyde::rt::Index<Key88> &idx_88,
            ::hyde::rt::Table<Row60> &table_60,
            ::hyde::rt::Index<Key131> &idx_131,
            uint64_t &g65) {
  ::hyde::rt::Vec<Tup_u32> vec250(allocator);
  proc_64(allocator, log, table_8, table_11, table_14, table_18,
          table_22, table_26, table_29, table_32, table_35, __38,
          table_41, __45, table_48, table_52, idx_77, table_56, idx_88,
          table_60, idx_131, g65, std::move(vec250));
  return false;
}

template <typename Log>
bool proc_64(::hyde::rt::Allocator &allocator, Log &log,
             ::hyde::rt::DiffTable<Row8> &table_8,
             ::hyde::rt::DiffTable<Row11> &table_11,
             ::hyde::rt::DiffTable<Row14> &table_14,
             ::hyde::rt::DiffTable<Row18> &table_18,
             ::hyde::rt::DiffTable<Row22> &table_22,
             ::hyde::rt::Table<Row26> &table_26,
             ::hyde::rt::DiffTable<Row29> &table_29,
             ::hyde::rt::Table<Row32> &table_32,
             ::hyde::rt::DiffTable<Row35> &table_35,
             ::hyde::rt::Table<Row38> &__38,
             ::hyde::rt::Table<Row41> &table_41,
             ::hyde::rt::Table<Row45> &__45,
             ::hyde::rt::Table<Row48> &table_48,
             ::hyde::rt::Table<Row52> &table_52,
             ::hyde::rt::Index<Key77> &idx_77,
             ::hyde::rt::Table<Row56> &table_56,
             ::hyde::rt::Index<Key88> &idx_88,
             ::hyde::rt::Table<Row60> &table_60,
             ::hyde::rt::Index<Key131> &idx_131,
             uint64_t &g65,
             ::hyde::rt::Vec<Tup_u32> vec70) {
  ::hyde::rt::Vec<Tup_b> vec66(allocator);
  ::hyde::rt::Vec<Tup_b> vec67(allocator);
  ::hyde::rt::Vec<Tup_b> vec73(allocator);
  ::hyde::rt::Vec<Tup_b> vec74(allocator);
  for (auto [v72] : vec70) {
    if (1 == v72) {
      if (const auto ins0 = table_41.TryAdd({true, 1}); ins0.added) {
        if (const auto ins1 = __38.TryAdd({true}); ins1.added) {
          vec73.Add({true});
          vec66.Add({true});
        }
      }
    }
    if (2 == v72) {
      if (const auto ins2 = table_48.TryAdd({true, 2}); ins2.added) {
        if (const auto ins3 = __45.TryAdd({true}); ins3.added) {
          vec74.Add({true});
          vec67.Add({true});
        }
      }
    }
  }
  vec70.Clear();
  flow_251(allocator, log, table_8, table_11, table_14, table_18,
           table_22, table_26, table_29, table_32, table_35, __38, __45,
           table_52, idx_77, table_56, idx_88, table_60, idx_131, g65,
           std::move(vec66), std::move(vec67), std::move(vec73),
           std::move(vec74));
  return false;
}

template <typename Log>
bool flow_251(::hyde::rt::Allocator &allocator, Log &log,
              ::hyde::rt::DiffTable<Row8> &table_8,
              ::hyde::rt::DiffTable<Row11> &table_11,
              ::hyde::rt::DiffTable<Row14> &table_14,
              ::hyde::rt::DiffTable<Row18> &table_18,
              ::hyde::rt::DiffTable<Row22> &table_22,
              ::hyde::rt::Table<Row26> &table_26,
              ::hyde::rt::DiffTable<Row29> &table_29,
              ::hyde::rt::Table<Row32> &table_32,
              ::hyde::rt::DiffTable<Row35> &table_35,
              ::hyde::rt::Table<Row38> &__38,
              ::hyde::rt::Table<Row45> &__45,
              ::hyde::rt::Table<Row52> &table_52,
              ::hyde::rt::Index<Key77> &idx_77,
              ::hyde::rt::Table<Row56> &table_56,
              ::hyde::rt::Index<Key88> &idx_88,
              ::hyde::rt::Table<Row60> &table_60,
              ::hyde::rt::Index<Key131> &idx_131,
              uint64_t &g65,
              ::hyde::rt::Vec<Tup_b> vec66,
              ::hyde::rt::Vec<Tup_b> vec67,
              ::hyde::rt::Vec<Tup_b> vec73,
              ::hyde::rt::Vec<Tup_b> vec74) {
  ::hyde::rt::Vec<Tup_b_b> vec68(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec69(allocator);
  ::hyde::rt::Vec<Tup_b> vec80(allocator);
  ::hyde::rt::Vec<Tup_b> vec85(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec92(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec93(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec97(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec101(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec105(allocator);
  ::hyde::rt::Vec<Tup_b> vec112(allocator);
  ::hyde::rt::Vec<Tup_b> vec116(allocator);
  ::hyde::rt::Vec<Tup_b> vec117(allocator);
  ::hyde::rt::Vec<Tup_b> vec120(allocator);
  ::hyde::rt::Vec<Tup_b> vec123(allocator);
  ::hyde::rt::Vec<Tup_b> vec126(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec135(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec136(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec140(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec144(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec148(allocator);
  ::hyde::rt::Vec<Tup_b> vec155(allocator);
  ::hyde::rt::Vec<Tup_b> vec159(allocator);
  ::hyde::rt::Vec<Tup_b> vec160(allocator);
  ::hyde::rt::Vec<Tup_b> vec163(allocator);
  ::hyde::rt::Vec<Tup_b> vec166(allocator);
  ::hyde::rt::Vec<Tup_b> vec169(allocator);
  ::hyde::rt::Vec<Tup_b> vec174(allocator);
  ::hyde::rt::Vec<Tup_b> vec177(allocator);
  ::hyde::rt::Vec<Tup_b> vec180(allocator);
  ::hyde::rt::Vec<Tup_b> vec183(allocator);
  ::hyde::rt::Vec<Tup_b> vec186(allocator);
  ::hyde::rt::Vec<Tup_b> vec189(allocator);
  ::hyde::rt::Vec<Tup_b> vec194(allocator);
  ::hyde::rt::Vec<Tup_b> vec197(allocator);
  ::hyde::rt::Vec<Tup_b> vec200(allocator);
  ::hyde::rt::Vec<Tup_b> vec203(allocator);
  ::hyde::rt::Vec<Tup_b> vec206(allocator);
  ::hyde::rt::Vec<Tup_b> vec209(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec216(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec221(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec230(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec234(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec238(allocator);
  ::hyde::rt::Vec<Tup_b_b> vec242(allocator);
  if ((g65 += 1) == 1) {
    if (const auto ins4 = table_52.TryAdd({true, true}); ins4.added) {
      idx_77.Add({true}, ins4.id);
      vec66.Add({true});
      vec67.Add({true});
    }
    if (const auto ins5 = table_56.TryAdd({true, false}); ins5.added) {
      idx_88.Add({true}, ins5.id);
      {
        const uint32_t m6 = __38.Find({true});
        if (!(m6 != ::hyde::rt::kNoRow && __38.InI(m6))) {
          {
            const auto d7 = table_14.AddDerivation({true, false}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d7.crossed) {
              vec68.Add({true, false});
            }
          }
        }
      }
    }
    if (const auto ins8 = table_60.TryAdd({true, false}); ins8.added) {
      idx_131.Add({true}, ins8.id);
      {
        const uint32_t m9 = __45.Find({true});
        if (!(m9 != ::hyde::rt::kNoRow && __45.InI(m9))) {
          {
            const auto d10 = table_18.AddDerivation({true, false}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d10.crossed) {
              vec69.Add({true, false});
            }
          }
        }
      }
    }
  }
  vec67.SortAndUnique();
  for (auto [v76] : vec67) {
    for (uint32_t j75_0 = idx_77.First({v76}); j75_0 != ::hyde::rt::kNoRow; j75_0 = idx_77.Next(j75_0)) {
      const auto r75_0 = table_52.RowAt(j75_0);
      const auto v79 = r75_0.x;
      const auto v78 = r75_0.c1;
      if (v78 == v76) {
        if (__45.Find({v78}) != ::hyde::rt::kNoRow) {
          if (const auto ins12 = table_32.TryAdd({v79}); ins12.added) {
            vec80.Add({v79});
          }
        }
      }
    }
  }
  vec67.Clear();
  vec66.SortAndUnique();
  for (auto [v82] : vec66) {
    for (uint32_t j81_0 = idx_77.First({v82}); j81_0 != ::hyde::rt::kNoRow; j81_0 = idx_77.Next(j81_0)) {
      const auto r81_0 = table_52.RowAt(j81_0);
      const auto v84 = r81_0.x;
      const auto v83 = r81_0.c1;
      if (v83 == v82) {
        if (__38.Find({v83}) != ::hyde::rt::kNoRow) {
          if (const auto ins14 = table_26.TryAdd({v84}); ins14.added) {
            vec85.Add({v84});
          }
        }
      }
    }
  }
  vec66.Clear();
  vec73.SortAndUnique();
  for (auto [v87] : vec73) {
    for (uint32_t s89 = idx_88.First({v87}); s89 != ::hyde::rt::kNoRow; s89 = idx_88.Next(s89)) {
      const auto r89 = table_56.RowAt(s89);
      const auto v90 = r89.c0;
      const auto v91 = r89.x;
      if (v90 == v87) {
        if (table_56.Find({v90, v91}) != ::hyde::rt::kNoRow) {
          {
            const auto d16 = table_14.SubDerivation({v87, v91}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d16.crossed) {
              vec92.Add({v87, v91});
            }
          }
        }
      }
    }
  }
  vec92.SortAndUnique();
  for (auto [v95, v96] : vec92) {
    {
      const auto id17 = table_14.Find({v95, v96});
      if (id17 != ::hyde::rt::kNoRow && table_14.TryClaimDel(id17)) {
        vec93.Add({v95, v96});
      }
    }
  }
  vec68.SortAndUnique();
  for (auto [v99, v100] : vec68) {
    {
      const auto id18 = table_14.Find({v99, v100});
      if (id18 != ::hyde::rt::kNoRow && table_14.TryClaimAdd(id18)) {
        vec97.Add({v99, v100});
      }
    }
  }
  for (auto [v103, v104] : vec93) {
    {
      const uint32_t m19 = table_14.Find({v103, v104});
      if (m19 != ::hyde::rt::kNoRow && table_14.NetDeleted(m19)) {
        vec101.Add({v103, v104});
      }
    }
  }
  for (auto [v107, v108] : vec97) {
    {
      const uint32_t m20 = table_14.Find({v107, v108});
      if (m20 != ::hyde::rt::kNoRow && table_14.NetAdded(m20)) {
        vec105.Add({v107, v108});
      }
    }
  }
  vec101.SortAndUnique();
  for (auto [v110, v111] : vec101) {
    {
      const auto d21 = table_29.SubDerivation({v111}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d21.crossed) {
        vec112.Add({v111});
      }
    }
  }
  vec105.SortAndUnique();
  for (auto [v114, v115] : vec105) {
    {
      const auto d22 = table_29.AddDerivation({v115}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d22.crossed) {
        vec116.Add({v115});
      }
    }
  }
  vec112.SortAndUnique();
  for (auto [v119] : vec112) {
    {
      const auto id23 = table_29.Find({v119});
      if (id23 != ::hyde::rt::kNoRow && table_29.TryClaimDel(id23)) {
        vec117.Add({v119});
      }
    }
  }
  vec116.SortAndUnique();
  for (auto [v122] : vec116) {
    {
      const auto id24 = table_29.Find({v122});
      if (id24 != ::hyde::rt::kNoRow && table_29.TryClaimAdd(id24)) {
        vec120.Add({v122});
      }
    }
  }
  for (auto [v125] : vec117) {
    {
      const uint32_t m25 = table_29.Find({v125});
      if (m25 != ::hyde::rt::kNoRow && table_29.NetDeleted(m25)) {
        vec123.Add({v125});
      }
    }
  }
  for (auto [v128] : vec120) {
    {
      const uint32_t m26 = table_29.Find({v128});
      if (m26 != ::hyde::rt::kNoRow && table_29.NetAdded(m26)) {
        vec126.Add({v128});
      }
    }
  }
  vec74.SortAndUnique();
  for (auto [v130] : vec74) {
    for (uint32_t s132 = idx_131.First({v130}); s132 != ::hyde::rt::kNoRow; s132 = idx_131.Next(s132)) {
      const auto r132 = table_60.RowAt(s132);
      const auto v133 = r132.c0;
      const auto v134 = r132.x;
      if (v133 == v130) {
        if (table_60.Find({v133, v134}) != ::hyde::rt::kNoRow) {
          {
            const auto d28 = table_18.SubDerivation({v130, v134}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d28.crossed) {
              vec135.Add({v130, v134});
            }
          }
        }
      }
    }
  }
  vec135.SortAndUnique();
  for (auto [v138, v139] : vec135) {
    {
      const auto id29 = table_18.Find({v138, v139});
      if (id29 != ::hyde::rt::kNoRow && table_18.TryClaimDel(id29)) {
        vec136.Add({v138, v139});
      }
    }
  }
  vec69.SortAndUnique();
  for (auto [v142, v143] : vec69) {
    {
      const auto id30 = table_18.Find({v142, v143});
      if (id30 != ::hyde::rt::kNoRow && table_18.TryClaimAdd(id30)) {
        vec140.Add({v142, v143});
      }
    }
  }
  for (auto [v146, v147] : vec136) {
    {
      const uint32_t m31 = table_18.Find({v146, v147});
      if (m31 != ::hyde::rt::kNoRow && table_18.NetDeleted(m31)) {
        vec144.Add({v146, v147});
      }
    }
  }
  for (auto [v150, v151] : vec140) {
    {
      const uint32_t m32 = table_18.Find({v150, v151});
      if (m32 != ::hyde::rt::kNoRow && table_18.NetAdded(m32)) {
        vec148.Add({v150, v151});
      }
    }
  }
  vec144.SortAndUnique();
  for (auto [v153, v154] : vec144) {
    {
      const auto d33 = table_35.SubDerivation({v154}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d33.crossed) {
        vec155.Add({v154});
      }
    }
  }
  vec148.SortAndUnique();
  for (auto [v157, v158] : vec148) {
    {
      const auto d34 = table_35.AddDerivation({v158}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d34.crossed) {
        vec159.Add({v158});
      }
    }
  }
  vec155.SortAndUnique();
  for (auto [v162] : vec155) {
    {
      const auto id35 = table_35.Find({v162});
      if (id35 != ::hyde::rt::kNoRow && table_35.TryClaimDel(id35)) {
        vec160.Add({v162});
      }
    }
  }
  vec159.SortAndUnique();
  for (auto [v165] : vec159) {
    {
      const auto id36 = table_35.Find({v165});
      if (id36 != ::hyde::rt::kNoRow && table_35.TryClaimAdd(id36)) {
        vec163.Add({v165});
      }
    }
  }
  for (auto [v168] : vec160) {
    {
      const uint32_t m37 = table_35.Find({v168});
      if (m37 != ::hyde::rt::kNoRow && table_35.NetDeleted(m37)) {
        vec166.Add({v168});
      }
    }
  }
  for (auto [v171] : vec163) {
    {
      const uint32_t m38 = table_35.Find({v171});
      if (m38 != ::hyde::rt::kNoRow && table_35.NetAdded(m38)) {
        vec169.Add({v171});
      }
    }
  }
  vec85.SortAndUnique();
  for (auto [v173] : vec85) {
    {
      const auto d39 = table_8.AddDerivation({v173}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d39.crossed) {
        vec174.Add({v173});
      }
    }
  }
  vec123.SortAndUnique();
  for (auto [v176] : vec123) {
    {
      const auto d40 = table_8.SubDerivation({v176}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d40.crossed) {
        vec177.Add({v176});
      }
    }
  }
  vec126.SortAndUnique();
  for (auto [v179] : vec126) {
    {
      const auto d41 = table_8.AddDerivation({v179}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d41.crossed) {
        vec174.Add({v179});
      }
    }
  }
  vec177.SortAndUnique();
  for (auto [v182] : vec177) {
    {
      const auto id42 = table_8.Find({v182});
      if (id42 != ::hyde::rt::kNoRow && table_8.TryClaimDel(id42)) {
        vec180.Add({v182});
      }
    }
  }
  vec174.SortAndUnique();
  for (auto [v185] : vec174) {
    {
      const auto id43 = table_8.Find({v185});
      if (id43 != ::hyde::rt::kNoRow && table_8.TryClaimAdd(id43)) {
        vec183.Add({v185});
      }
    }
  }
  for (auto [v188] : vec180) {
    {
      const uint32_t m44 = table_8.Find({v188});
      if (m44 != ::hyde::rt::kNoRow && table_8.NetDeleted(m44)) {
        vec186.Add({v188});
      }
    }
  }
  for (auto [v191] : vec183) {
    {
      const uint32_t m45 = table_8.Find({v191});
      if (m45 != ::hyde::rt::kNoRow && table_8.NetAdded(m45)) {
        vec189.Add({v191});
      }
    }
  }
  vec80.SortAndUnique();
  for (auto [v193] : vec80) {
    {
      const auto d46 = table_11.AddDerivation({v193}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d46.crossed) {
        vec194.Add({v193});
      }
    }
  }
  vec166.SortAndUnique();
  for (auto [v196] : vec166) {
    {
      const auto d47 = table_11.SubDerivation({v196}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d47.crossed) {
        vec197.Add({v196});
      }
    }
  }
  vec169.SortAndUnique();
  for (auto [v199] : vec169) {
    {
      const auto d48 = table_11.AddDerivation({v199}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d48.crossed) {
        vec194.Add({v199});
      }
    }
  }
  vec197.SortAndUnique();
  for (auto [v202] : vec197) {
    {
      const auto id49 = table_11.Find({v202});
      if (id49 != ::hyde::rt::kNoRow && table_11.TryClaimDel(id49)) {
        vec200.Add({v202});
      }
    }
  }
  vec194.SortAndUnique();
  for (auto [v205] : vec194) {
    {
      const auto id50 = table_11.Find({v205});
      if (id50 != ::hyde::rt::kNoRow && table_11.TryClaimAdd(id50)) {
        vec203.Add({v205});
      }
    }
  }
  for (auto [v208] : vec200) {
    {
      const uint32_t m51 = table_11.Find({v208});
      if (m51 != ::hyde::rt::kNoRow && table_11.NetDeleted(m51)) {
        vec206.Add({v208});
      }
    }
  }
  for (auto [v211] : vec203) {
    {
      const uint32_t m52 = table_11.Find({v211});
      if (m52 != ::hyde::rt::kNoRow && table_11.NetAdded(m52)) {
        vec209.Add({v211});
      }
    }
  }
  vec189.SortAndUnique();
  for (auto [v213] : vec189) {
    for (uint32_t s214 = 0; s214 < table_11.NumRows(); ++s214) {
      const auto r214 = table_11.RowAt(s214);
      const auto v215 = r214.x;
      {
        const uint32_t m53 = table_11.Find({v215});
        if (m53 != ::hyde::rt::kNoRow && table_11.InI(m53)) {
          {
            const auto d54 = table_22.AddDerivation({v213, v215}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d54.crossed) {
              vec216.Add({v213, v215});
            }
          }
        }
      }
    }
  }
  vec186.SortAndUnique();
  for (auto [v218] : vec186) {
    for (uint32_t s219 = 0; s219 < table_11.NumRows(); ++s219) {
      const auto r219 = table_11.RowAt(s219);
      const auto v220 = r219.x;
      {
        const uint32_t m55 = table_11.Find({v220});
        if (m55 != ::hyde::rt::kNoRow && table_11.InI(m55)) {
          {
            const auto d56 = table_22.SubDerivation({v218, v220}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d56.crossed) {
              vec221.Add({v218, v220});
            }
          }
        }
      }
    }
  }
  vec209.SortAndUnique();
  for (auto [v223] : vec209) {
    for (uint32_t s224 = 0; s224 < table_8.NumRows(); ++s224) {
      const auto r224 = table_8.RowAt(s224);
      const auto v225 = r224.x;
      {
        const uint32_t m57 = table_8.Find({v225});
        if (m57 != ::hyde::rt::kNoRow && table_8.InNew(m57)) {
          {
            const auto d58 = table_22.AddDerivation({v225, v223}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d58.crossed) {
              vec216.Add({v225, v223});
            }
          }
        }
      }
    }
  }
  vec206.SortAndUnique();
  for (auto [v227] : vec206) {
    for (uint32_t s228 = 0; s228 < table_8.NumRows(); ++s228) {
      const auto r228 = table_8.RowAt(s228);
      const auto v229 = r228.x;
      {
        const uint32_t m59 = table_8.Find({v229});
        if (m59 != ::hyde::rt::kNoRow && table_8.InNew(m59)) {
          {
            const auto d60 = table_22.SubDerivation({v229, v227}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d60.crossed) {
              vec221.Add({v229, v227});
            }
          }
        }
      }
    }
  }
  vec221.SortAndUnique();
  for (auto [v232, v233] : vec221) {
    {
      const auto id61 = table_22.Find({v232, v233});
      if (id61 != ::hyde::rt::kNoRow && table_22.TryClaimDel(id61)) {
        vec230.Add({v232, v233});
      }
    }
  }
  vec216.SortAndUnique();
  for (auto [v236, v237] : vec216) {
    {
      const auto id62 = table_22.Find({v236, v237});
      if (id62 != ::hyde::rt::kNoRow && table_22.TryClaimAdd(id62)) {
        vec234.Add({v236, v237});
      }
    }
  }
  for (auto [v240, v241] : vec230) {
    {
      const uint32_t m63 = table_22.Find({v240, v241});
      if (m63 != ::hyde::rt::kNoRow && table_22.NetDeleted(m63)) {
        vec238.Add({v240, v241});
      }
    }
  }
  for (auto [v244, v245] : vec234) {
    {
      const uint32_t m64 = table_22.Find({v244, v245});
      if (m64 != ::hyde::rt::kNoRow && table_22.NetAdded(m64)) {
        vec242.Add({v244, v245});
      }
    }
  }
  table_8.Commit([](const Row8 &, bool) {});
#ifndef NDEBUG
  table_8.DebugValidateCounts();
#endif
  table_11.Commit([](const Row11 &, bool) {});
#ifndef NDEBUG
  table_11.DebugValidateCounts();
#endif
  table_14.Commit([](const Row14 &, bool) {});
#ifndef NDEBUG
  table_14.DebugValidateCounts();
#endif
  table_18.Commit([](const Row18 &, bool) {});
#ifndef NDEBUG
  table_18.DebugValidateCounts();
#endif
  table_22.Commit([&](const Row22 &row, bool added) {
    log.enabled_features_2(row.fooenabled, row.barenabled, added);
  });
#ifndef NDEBUG
  table_22.DebugValidateCounts();
#endif
  table_26.Seal();
  table_29.Commit([](const Row29 &, bool) {});
#ifndef NDEBUG
  table_29.DebugValidateCounts();
#endif
  table_32.Seal();
  table_35.Commit([](const Row35 &, bool) {});
#ifndef NDEBUG
  table_35.DebugValidateCounts();
#endif
  __38.Seal();
  __45.Seal();
  return true;
}
