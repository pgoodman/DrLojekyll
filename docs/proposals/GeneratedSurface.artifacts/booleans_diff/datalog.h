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

struct Tup_b_i32 {
  bool c0;
  int32_t c1;
  auto operator<=>(const Tup_b_i32 &) const noexcept = default;
};

struct Tup_i32 {
  int32_t c0;
  auto operator<=>(const Tup_i32 &) const noexcept = default;
};

struct Tup_i32_b {
  int32_t c0;
  bool c1;
  auto operator<=>(const Tup_i32_b &) const noexcept = default;
};

using add_user_input = Tup_i32;

// Rows of `table_4`.
struct Row4 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row4 &) const noexcept = default;
};

// Rows of `table_7`.
struct Row7 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row7 &) const noexcept = default;
};

// Rows of `table_10`.
struct Row10 {
  int32_t id;
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id, c1);
  }
  bool operator==(const Row10 &) const noexcept = default;
};

// Key of `idx_135` over `table_10`.
struct Key135 {
  bool c1;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c1);
  }
  bool operator==(const Key135 &) const noexcept = default;
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
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row17 &) const noexcept = default;
};

// Rows of `table_20`.
struct Row20 {
  bool c0;
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(c0, id);
  }
  bool operator==(const Row20 &) const noexcept = default;
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

// Rows of `user_is_logged_in_28` (user_is_logged_in/1).
struct Row28 {
  int32_t id;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(id);
  }
  bool operator==(const Row28 &) const noexcept = default;
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
};

// Internal flow procedures. Not driver-facing: call the entry points
// declared inside `Database` instead. Each signature names exactly the
// state the procedure reads and writes.
inline bool init_3(::hyde::rt::Allocator &allocator,
                   ::hyde::rt::DiffTable<Row4> &table_4,
                   ::hyde::rt::Table<Row7> &table_7,
                   ::hyde::rt::Table<Row10> &table_10,
                   ::hyde::rt::Index<Key135> &idx_135,
                   ::hyde::rt::DiffTable<Row14> &__14,
                   ::hyde::rt::DiffTable<Row17> &table_17,
                   ::hyde::rt::DiffTable<Row20> &table_20,
                   ::hyde::rt::DiffTable<Row24> &table_24,
                   ::hyde::rt::DiffTable<Row28> &user_is_logged_in_28,
                   uint64_t &g32);
inline bool proc_31(::hyde::rt::Allocator &allocator,
                    ::hyde::rt::DiffTable<Row4> &table_4,
                    ::hyde::rt::Table<Row7> &table_7,
                    ::hyde::rt::Table<Row10> &table_10,
                    ::hyde::rt::Index<Key135> &idx_135,
                    ::hyde::rt::DiffTable<Row14> &__14,
                    ::hyde::rt::DiffTable<Row17> &table_17,
                    ::hyde::rt::DiffTable<Row20> &table_20,
                    ::hyde::rt::DiffTable<Row24> &table_24,
                    ::hyde::rt::DiffTable<Row28> &user_is_logged_in_28,
                    uint64_t &g32,
                    ::hyde::rt::Vec<Tup_i32> vec33,
                    ::hyde::rt::Vec<Tup_i32> vec38,
                    ::hyde::rt::Vec<Tup_i32> vec39);
inline bool flow_189(::hyde::rt::Allocator &allocator,
                     ::hyde::rt::DiffTable<Row4> &table_4,
                     ::hyde::rt::Table<Row7> &table_7,
                     ::hyde::rt::Table<Row10> &table_10,
                     ::hyde::rt::Index<Key135> &idx_135,
                     ::hyde::rt::DiffTable<Row14> &__14,
                     ::hyde::rt::DiffTable<Row17> &table_17,
                     ::hyde::rt::DiffTable<Row20> &table_20,
                     ::hyde::rt::DiffTable<Row24> &table_24,
                     ::hyde::rt::DiffTable<Row28> &user_is_logged_in_28,
                     uint64_t &g32,
                     ::hyde::rt::Vec<Tup_i32_b> vec36,
                     ::hyde::rt::Vec<Tup_i32> vec37,
                     ::hyde::rt::Vec<Tup_i32> vec42,
                     ::hyde::rt::Vec<Tup_i32> vec45);

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
        table_4(allocator_),
        table_7(allocator_),
        table_10(allocator_),
        idx_135(allocator_),
        __14(allocator_),
        table_17(allocator_),
        table_20(allocator_),
        table_24(allocator_),
        user_is_logged_in_28(allocator_) {}

  // Epoch 0: derives the empty-EDB least model and publishes its
  // t=0 deltas to `log`. Call exactly once, before any message.
  template <typename Log, typename Functors>
  friend auto init(Database &db, Log &, Functors &) {
    assert(!db.initialized_);
    db.initialized_ = true;
    return init_3(db.allocator, db.table_4, db.table_7, db.table_10,
                  db.idx_135, db.__14, db.table_17, db.table_20,
                  db.table_24, db.user_is_logged_in_28, db.g32);
  }

  // Message `add_user/1`.
  template <typename Log, typename Functors>
  friend auto add_user_1(Database &db, Log &, Functors &,
                         ::hyde::rt::Vec<Tup_i32> vec178) {
    assert(db.initialized_);
    ::hyde::rt::Vec<Tup_i32> vec180(db.allocator);
    proc_31(db.allocator, db.table_4, db.table_7, db.table_10,
            db.idx_135, db.__14, db.table_17, db.table_20, db.table_24,
            db.user_is_logged_in_28, db.g32, std::move(vec178),
            std::move(vec180), std::move(vec180));
    return true;
  }

  // Message `log_in/1`.
  template <typename Log, typename Functors>
  friend auto log_in_1(Database &db, Log &, Functors &,
                       ::hyde::rt::Vec<Tup_i32> vec182,
                       ::hyde::rt::Vec<Tup_i32> vec183) {
    assert(db.initialized_);
    ::hyde::rt::Vec<Tup_i32> vec185(db.allocator);
    ::hyde::rt::NetBatch(vec182, vec183);
    proc_31(db.allocator, db.table_4, db.table_7, db.table_10,
            db.idx_135, db.__14, db.table_17, db.table_20, db.table_24,
            db.user_is_logged_in_28, db.g32, std::move(vec185),
            std::move(vec182), std::move(vec183));
    return true;
  }

  // Query `user_is_logged_in/1` (b).
  friend bool user_is_logged_in_b(Database &db, int32_t UserId) {
    assert(db.initialized_);
    const uint32_t id = db.user_is_logged_in_28.Find({UserId});
    return id != ::hyde::rt::kNoRow && db.user_is_logged_in_28.Present(id);
  }

 private:
  ::hyde::rt::Allocator allocator;

  ::hyde::rt::DiffTable<Row4> table_4;
  ::hyde::rt::Table<Row7> table_7;
  ::hyde::rt::Table<Row10> table_10;
  ::hyde::rt::Index<Key135> idx_135;
  ::hyde::rt::DiffTable<Row14> __14;
  ::hyde::rt::DiffTable<Row17> table_17;
  ::hyde::rt::DiffTable<Row20> table_20;
  ::hyde::rt::DiffTable<Row24> table_24;
  ::hyde::rt::DiffTable<Row28> user_is_logged_in_28;

  uint64_t g32 = 0;
  bool initialized_ = false;
};

inline bool init_3(::hyde::rt::Allocator &allocator,
                   ::hyde::rt::DiffTable<Row4> &table_4,
                   ::hyde::rt::Table<Row7> &table_7,
                   ::hyde::rt::Table<Row10> &table_10,
                   ::hyde::rt::Index<Key135> &idx_135,
                   ::hyde::rt::DiffTable<Row14> &__14,
                   ::hyde::rt::DiffTable<Row17> &table_17,
                   ::hyde::rt::DiffTable<Row20> &table_20,
                   ::hyde::rt::DiffTable<Row24> &table_24,
                   ::hyde::rt::DiffTable<Row28> &user_is_logged_in_28,
                   uint64_t &g32) {
  ::hyde::rt::Vec<Tup_i32> vec187(allocator);
  ::hyde::rt::Vec<Tup_i32> vec188(allocator);
  proc_31(allocator, table_4, table_7, table_10, idx_135, __14, table_17,
          table_20, table_24, user_is_logged_in_28, g32,
          std::move(vec187), std::move(vec188), std::move(vec188));
  return false;
}

inline bool proc_31(::hyde::rt::Allocator &allocator,
                    ::hyde::rt::DiffTable<Row4> &table_4,
                    ::hyde::rt::Table<Row7> &table_7,
                    ::hyde::rt::Table<Row10> &table_10,
                    ::hyde::rt::Index<Key135> &idx_135,
                    ::hyde::rt::DiffTable<Row14> &__14,
                    ::hyde::rt::DiffTable<Row17> &table_17,
                    ::hyde::rt::DiffTable<Row20> &table_20,
                    ::hyde::rt::DiffTable<Row24> &table_24,
                    ::hyde::rt::DiffTable<Row28> &user_is_logged_in_28,
                    uint64_t &g32,
                    ::hyde::rt::Vec<Tup_i32> vec33,
                    ::hyde::rt::Vec<Tup_i32> vec38,
                    ::hyde::rt::Vec<Tup_i32> vec39) {
  ::hyde::rt::Vec<Tup_i32_b> vec36(allocator);
  ::hyde::rt::Vec<Tup_i32> vec37(allocator);
  ::hyde::rt::Vec<Tup_i32> vec42(allocator);
  ::hyde::rt::Vec<Tup_i32> vec45(allocator);
  for (auto [v35] : vec33) {
    if (const auto ins0 = table_7.TryAdd({v35}); ins0.added) {
      if (const auto ins1 = table_10.TryAdd({v35, true}); ins1.added) {
        idx_135.Add({true}, ins1.id);
        vec36.Add({v35, true});
      }
      vec37.Add({v35});
    }
  }
  for (auto [v41] : vec38) {
    {
      const auto d2 = table_4.AddExplicit({v41});
      if (d2.crossed) {
        vec42.Add({v41});
      }
    }
  }
  for (auto [v44] : vec39) {
    {
      const auto d3 = table_4.SubExplicit({v44});
      if (d3.crossed) {
        vec45.Add({v44});
      }
    }
  }
  vec33.Clear();
  vec38.Clear();
  vec39.Clear();
  flow_189(allocator, table_4, table_7, table_10, idx_135, __14, table_17,
           table_20, table_24, user_is_logged_in_28, g32, std::move(vec36),
           std::move(vec37), std::move(vec42), std::move(vec45));
  return false;
}

inline bool flow_189(::hyde::rt::Allocator &allocator,
                     ::hyde::rt::DiffTable<Row4> &table_4,
                     ::hyde::rt::Table<Row7> &table_7,
                     ::hyde::rt::Table<Row10> &table_10,
                     ::hyde::rt::Index<Key135> &idx_135,
                     ::hyde::rt::DiffTable<Row14> &__14,
                     ::hyde::rt::DiffTable<Row17> &table_17,
                     ::hyde::rt::DiffTable<Row20> &table_20,
                     ::hyde::rt::DiffTable<Row24> &table_24,
                     ::hyde::rt::DiffTable<Row28> &user_is_logged_in_28,
                     uint64_t &g32,
                     ::hyde::rt::Vec<Tup_i32_b> vec36,
                     ::hyde::rt::Vec<Tup_i32> vec37,
                     ::hyde::rt::Vec<Tup_i32> vec42,
                     ::hyde::rt::Vec<Tup_i32> vec45) {
  ::hyde::rt::Vec<Tup_i32> vec46(allocator);
  ::hyde::rt::Vec<Tup_b> vec47(allocator);
  ::hyde::rt::Vec<Tup_i32> vec48(allocator);
  ::hyde::rt::Vec<Tup_i32> vec51(allocator);
  ::hyde::rt::Vec<Tup_i32> vec54(allocator);
  ::hyde::rt::Vec<Tup_i32> vec57(allocator);
  ::hyde::rt::Vec<Tup_i32> vec70(allocator);
  ::hyde::rt::Vec<Tup_i32> vec71(allocator);
  ::hyde::rt::Vec<Tup_i32> vec72(allocator);
  ::hyde::rt::Vec<Tup_i32> vec75(allocator);
  ::hyde::rt::Vec<Tup_i32> vec78(allocator);
  ::hyde::rt::Vec<Tup_i32> vec81(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec86(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec89(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec90(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec94(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec98(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec102(allocator);
  ::hyde::rt::Vec<Tup_b> vec109(allocator);
  ::hyde::rt::Vec<Tup_b> vec113(allocator);
  ::hyde::rt::Vec<Tup_b> vec114(allocator);
  ::hyde::rt::Vec<Tup_b> vec117(allocator);
  ::hyde::rt::Vec<Tup_b> vec120(allocator);
  ::hyde::rt::Vec<Tup_b> vec123(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec139(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec140(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec141(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec145(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec149(allocator);
  ::hyde::rt::Vec<Tup_b_i32> vec153(allocator);
  ::hyde::rt::Vec<Tup_i32> vec160(allocator);
  ::hyde::rt::Vec<Tup_i32> vec164(allocator);
  ::hyde::rt::Vec<Tup_i32> vec165(allocator);
  ::hyde::rt::Vec<Tup_i32> vec168(allocator);
  ::hyde::rt::Vec<Tup_i32> vec171(allocator);
  ::hyde::rt::Vec<Tup_i32> vec174(allocator);
  g32 += 1;
  vec45.SortAndUnique();
  for (auto [v50] : vec45) {
    {
      const auto id4 = table_4.Find({v50});
      if (id4 != ::hyde::rt::kNoRow && table_4.TryClaimDel(id4)) {
        vec48.Add({v50});
      }
    }
  }
  vec42.SortAndUnique();
  for (auto [v53] : vec42) {
    {
      const auto id5 = table_4.Find({v53});
      if (id5 != ::hyde::rt::kNoRow && table_4.TryClaimAdd(id5)) {
        vec51.Add({v53});
      }
    }
  }
  for (auto [v56] : vec48) {
    {
      const uint32_t m6 = table_4.Find({v56});
      if (m6 != ::hyde::rt::kNoRow && table_4.NetDeleted(m6)) {
        vec54.Add({v56});
      }
    }
  }
  for (auto [v59] : vec51) {
    {
      const uint32_t m7 = table_4.Find({v59});
      if (m7 != ::hyde::rt::kNoRow && table_4.NetAdded(m7)) {
        vec57.Add({v59});
      }
    }
  }
  vec54.SortAndUnique();
  for (auto [v61] : vec54) {
    vec46.Add({v61});
  }
  vec57.SortAndUnique();
  for (auto [v63] : vec57) {
    vec46.Add({v63});
  }
  vec37.SortAndUnique();
  for (auto [v65] : vec37) {
    vec46.Add({v65});
  }
  vec46.SortAndUnique();
  for (auto [v67] : vec46) {
    if (const uint32_t j66_0 = table_7.Find({v67}); j66_0 != ::hyde::rt::kNoRow) {
      const auto r66_0 = table_7.RowAt(j66_0);
      const auto v69 = r66_0.id;
      if (const uint32_t j66_1 = table_4.Find({v67}); j66_1 != ::hyde::rt::kNoRow) {
        const auto r66_1 = table_4.RowAt(j66_1);
        const auto v68 = r66_1.id;
        if (r66_0.id == v67 && table_7.InNew(j66_0) && r66_1.id == v67 && table_4.InNew(j66_1) && (table_7.NetAdded(j66_0) || table_4.NetAdded(j66_1))) {
          {
            const auto d8 = table_17.AddDerivation({v67}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d8.crossed) {
              vec70.Add({v67});
            }
          }
        }
        if (r66_0.id == v67 && table_7.InI(j66_0) && r66_1.id == v67 && table_4.InI(j66_1) && (table_7.NetDeleted(j66_0) || table_4.NetDeleted(j66_1))) {
          {
            const auto d9 = table_17.SubDerivation({v67}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d9.crossed) {
              vec71.Add({v67});
            }
          }
        }
      }
    }
  }
  vec46.Clear();
  vec71.SortAndUnique();
  for (auto [v74] : vec71) {
    {
      const auto id10 = table_17.Find({v74});
      if (id10 != ::hyde::rt::kNoRow && table_17.TryClaimDel(id10)) {
        vec72.Add({v74});
      }
    }
  }
  vec70.SortAndUnique();
  for (auto [v77] : vec70) {
    {
      const auto id11 = table_17.Find({v77});
      if (id11 != ::hyde::rt::kNoRow && table_17.TryClaimAdd(id11)) {
        vec75.Add({v77});
      }
    }
  }
  for (auto [v80] : vec72) {
    {
      const uint32_t m12 = table_17.Find({v80});
      if (m12 != ::hyde::rt::kNoRow && table_17.NetDeleted(m12)) {
        vec78.Add({v80});
      }
    }
  }
  for (auto [v83] : vec75) {
    {
      const uint32_t m13 = table_17.Find({v83});
      if (m13 != ::hyde::rt::kNoRow && table_17.NetAdded(m13)) {
        vec81.Add({v83});
      }
    }
  }
  vec78.SortAndUnique();
  for (auto [v85] : vec78) {
    {
      const auto d14 = table_24.SubDerivation({true, v85}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d14.crossed) {
        vec86.Add({true, v85});
      }
    }
  }
  vec81.SortAndUnique();
  for (auto [v88] : vec81) {
    {
      const auto d15 = table_24.AddDerivation({true, v88}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d15.crossed) {
        vec89.Add({true, v88});
      }
    }
  }
  vec86.SortAndUnique();
  for (auto [v92, v93] : vec86) {
    {
      const auto id16 = table_24.Find({v92, v93});
      if (id16 != ::hyde::rt::kNoRow && table_24.TryClaimDel(id16)) {
        vec90.Add({v92, v93});
      }
    }
  }
  vec89.SortAndUnique();
  for (auto [v96, v97] : vec89) {
    {
      const auto id17 = table_24.Find({v96, v97});
      if (id17 != ::hyde::rt::kNoRow && table_24.TryClaimAdd(id17)) {
        vec94.Add({v96, v97});
      }
    }
  }
  for (auto [v100, v101] : vec90) {
    {
      const uint32_t m18 = table_24.Find({v100, v101});
      if (m18 != ::hyde::rt::kNoRow && table_24.NetDeleted(m18)) {
        vec98.Add({v100, v101});
      }
    }
  }
  for (auto [v104, v105] : vec94) {
    {
      const uint32_t m19 = table_24.Find({v104, v105});
      if (m19 != ::hyde::rt::kNoRow && table_24.NetAdded(m19)) {
        vec102.Add({v104, v105});
      }
    }
  }
  vec98.SortAndUnique();
  for (auto [v107, v108] : vec98) {
    {
      const auto d20 = __14.SubDerivation({v107}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d20.crossed) {
        vec109.Add({v107});
      }
    }
  }
  vec102.SortAndUnique();
  for (auto [v111, v112] : vec102) {
    {
      const auto d21 = __14.AddDerivation({v111}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d21.crossed) {
        vec113.Add({v111});
      }
    }
  }
  vec109.SortAndUnique();
  for (auto [v116] : vec109) {
    {
      const auto id22 = __14.Find({v116});
      if (id22 != ::hyde::rt::kNoRow && __14.TryClaimDel(id22)) {
        vec114.Add({v116});
      }
    }
  }
  vec113.SortAndUnique();
  for (auto [v119] : vec113) {
    {
      const auto id23 = __14.Find({v119});
      if (id23 != ::hyde::rt::kNoRow && __14.TryClaimAdd(id23)) {
        vec117.Add({v119});
      }
    }
  }
  for (auto [v122] : vec114) {
    {
      const uint32_t m24 = __14.Find({v122});
      if (m24 != ::hyde::rt::kNoRow && __14.NetDeleted(m24)) {
        vec120.Add({v122});
      }
    }
  }
  for (auto [v125] : vec117) {
    {
      const uint32_t m25 = __14.Find({v125});
      if (m25 != ::hyde::rt::kNoRow && __14.NetAdded(m25)) {
        vec123.Add({v125});
      }
    }
  }
  vec36.SortAndUnique();
  for (auto [v127, v128] : vec36) {
    vec47.Add({v128});
  }
  vec120.SortAndUnique();
  for (auto [v130] : vec120) {
    vec47.Add({v130});
  }
  vec123.SortAndUnique();
  for (auto [v132] : vec123) {
    vec47.Add({v132});
  }
  vec47.SortAndUnique();
  for (auto [v134] : vec47) {
    if (const uint32_t j133_0 = __14.Find({v134}); j133_0 != ::hyde::rt::kNoRow) {
      const auto r133_0 = __14.RowAt(j133_0);
      const auto v137 = r133_0.c0;
      for (uint32_t j133_1 = idx_135.First({v134}); j133_1 != ::hyde::rt::kNoRow; j133_1 = idx_135.Next(j133_1)) {
        const auto r133_1 = table_10.RowAt(j133_1);
        const auto v138 = r133_1.id;
        const auto v136 = r133_1.c1;
        if (r133_0.c0 == v134 && __14.InNew(j133_0) && r133_1.c1 == v134 && table_10.InNew(j133_1) && (__14.NetAdded(j133_0) || table_10.NetAdded(j133_1))) {
          {
            const auto d26 = table_20.AddDerivation({v134, v138}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d26.crossed) {
              vec139.Add({v134, v138});
            }
          }
        }
        if (r133_0.c0 == v134 && __14.InI(j133_0) && r133_1.c1 == v134 && table_10.InI(j133_1) && (__14.NetDeleted(j133_0) || table_10.NetDeleted(j133_1))) {
          {
            const auto d27 = table_20.SubDerivation({v134, v138}, ::hyde::rt::DerivClass::kNonRecursive);
            if (d27.crossed) {
              vec140.Add({v134, v138});
            }
          }
        }
      }
    }
  }
  vec47.Clear();
  vec140.SortAndUnique();
  for (auto [v143, v144] : vec140) {
    {
      const auto id28 = table_20.Find({v143, v144});
      if (id28 != ::hyde::rt::kNoRow && table_20.TryClaimDel(id28)) {
        vec141.Add({v143, v144});
      }
    }
  }
  vec139.SortAndUnique();
  for (auto [v147, v148] : vec139) {
    {
      const auto id29 = table_20.Find({v147, v148});
      if (id29 != ::hyde::rt::kNoRow && table_20.TryClaimAdd(id29)) {
        vec145.Add({v147, v148});
      }
    }
  }
  for (auto [v151, v152] : vec141) {
    {
      const uint32_t m30 = table_20.Find({v151, v152});
      if (m30 != ::hyde::rt::kNoRow && table_20.NetDeleted(m30)) {
        vec149.Add({v151, v152});
      }
    }
  }
  for (auto [v155, v156] : vec145) {
    {
      const uint32_t m31 = table_20.Find({v155, v156});
      if (m31 != ::hyde::rt::kNoRow && table_20.NetAdded(m31)) {
        vec153.Add({v155, v156});
      }
    }
  }
  vec149.SortAndUnique();
  for (auto [v158, v159] : vec149) {
    {
      const auto d32 = user_is_logged_in_28.SubDerivation({v159}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d32.crossed) {
        vec160.Add({v159});
      }
    }
  }
  vec153.SortAndUnique();
  for (auto [v162, v163] : vec153) {
    {
      const auto d33 = user_is_logged_in_28.AddDerivation({v163}, ::hyde::rt::DerivClass::kNonRecursive);
      if (d33.crossed) {
        vec164.Add({v163});
      }
    }
  }
  vec160.SortAndUnique();
  for (auto [v167] : vec160) {
    {
      const auto id34 = user_is_logged_in_28.Find({v167});
      if (id34 != ::hyde::rt::kNoRow && user_is_logged_in_28.TryClaimDel(id34)) {
        vec165.Add({v167});
      }
    }
  }
  vec164.SortAndUnique();
  for (auto [v170] : vec164) {
    {
      const auto id35 = user_is_logged_in_28.Find({v170});
      if (id35 != ::hyde::rt::kNoRow && user_is_logged_in_28.TryClaimAdd(id35)) {
        vec168.Add({v170});
      }
    }
  }
  for (auto [v173] : vec165) {
    {
      const uint32_t m36 = user_is_logged_in_28.Find({v173});
      if (m36 != ::hyde::rt::kNoRow && user_is_logged_in_28.NetDeleted(m36)) {
        vec171.Add({v173});
      }
    }
  }
  for (auto [v176] : vec168) {
    {
      const uint32_t m37 = user_is_logged_in_28.Find({v176});
      if (m37 != ::hyde::rt::kNoRow && user_is_logged_in_28.NetAdded(m37)) {
        vec174.Add({v176});
      }
    }
  }
  table_4.Commit([](const Row4 &, bool) {});
#ifndef NDEBUG
  table_4.DebugValidateCounts();
#endif
  table_7.Seal();
  table_10.Seal();
  __14.Commit([](const Row14 &, bool) {});
#ifndef NDEBUG
  __14.DebugValidateCounts();
#endif
  table_17.Commit([](const Row17 &, bool) {});
#ifndef NDEBUG
  table_17.DebugValidateCounts();
#endif
  table_20.Commit([](const Row20 &, bool) {});
#ifndef NDEBUG
  table_20.DebugValidateCounts();
#endif
  table_24.Commit([](const Row24 &, bool) {});
#ifndef NDEBUG
  table_24.DebugValidateCounts();
#endif
  user_is_logged_in_28.Commit([](const Row28 &, bool) {});
#ifndef NDEBUG
  user_is_logged_in_28.DebugValidateCounts();
#endif
  return true;
}
