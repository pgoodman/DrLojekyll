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

struct Tup_i32 {
  int32_t c0;
  auto operator<=>(const Tup_i32 &) const noexcept = default;
};


// Rows of `q_4` (q/1).
struct Row4 {
  int32_t a;
  uint64_t Hash(void) const noexcept {
    return ::hyde::rt::HashRow(a);
  }
  bool operator==(const Row4 &) const noexcept = default;
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
                   ::hyde::rt::DiffTable<Row4> &q_4, uint64_t &g8);
inline bool proc_7(::hyde::rt::Allocator &allocator,
                   ::hyde::rt::DiffTable<Row4> &q_4, uint64_t &g8,
                   ::hyde::rt::Vec<Tup_i32> vec9,
                   ::hyde::rt::Vec<Tup_i32> vec10);
inline bool flow_35(::hyde::rt::Allocator &allocator,
                    ::hyde::rt::DiffTable<Row4> &q_4, uint64_t &g8,
                    ::hyde::rt::Vec<Tup_i32> vec13,
                    ::hyde::rt::Vec<Tup_i32> vec16);

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
        q_4(allocator_) {}

  // Epoch 0: derives the empty-EDB least model and publishes its
  // t=0 deltas to `log`. Call exactly once, before any message.
  template <typename Log, typename Functors>
  friend auto init(Database &db, Log &, Functors &) {
    assert(!db.initialized_);
    db.initialized_ = true;
    return init_3(db.allocator, db.q_4, db.g8);
  }

  // Message `add/1`.
  template <typename Log, typename Functors>
  friend auto add_1(Database &db, Log &, Functors &,
                    ::hyde::rt::Vec<Tup_i32> vec30,
                    ::hyde::rt::Vec<Tup_i32> vec31) {
    assert(db.initialized_);
    ::hyde::rt::NetBatch(vec30, vec31);
    proc_7(db.allocator, db.q_4, db.g8, std::move(vec30),
           std::move(vec31));
    return true;
  }

  // Query `q/1` (f).
  struct q_f_cursor {
    Database &db;
    uint32_t pos;
    bool next(int32_t &A) {
      while (pos < db.q_4.NumRows()) {
        const uint32_t id = pos++;
        const auto row = db.q_4.RowAt(id);
        if (!db.q_4.Present(id)) {
          continue;
        }
        A = row.a;
        return true;
      }
      return false;
    }
  };
  friend q_f_cursor q_f(Database &db) {
    assert(db.initialized_);
    return {db, 0};
  }

 private:
  ::hyde::rt::Allocator allocator;

  ::hyde::rt::DiffTable<Row4> q_4;

  uint64_t g8 = 0;
  bool initialized_ = false;
};

inline bool init_3(::hyde::rt::Allocator &allocator,
                   ::hyde::rt::DiffTable<Row4> &q_4, uint64_t &g8) {
  ::hyde::rt::Vec<Tup_i32> vec34(allocator);
  proc_7(allocator, q_4, g8, std::move(vec34), std::move(vec34));
  return false;
}

inline bool proc_7(::hyde::rt::Allocator &allocator,
                   ::hyde::rt::DiffTable<Row4> &q_4, uint64_t &g8,
                   ::hyde::rt::Vec<Tup_i32> vec9,
                   ::hyde::rt::Vec<Tup_i32> vec10) {
  ::hyde::rt::Vec<Tup_i32> vec13(allocator);
  ::hyde::rt::Vec<Tup_i32> vec16(allocator);
  for (auto [v12] : vec9) {
    {
      const auto d0 = q_4.AddExplicit({v12});
      if (d0.crossed) {
        vec13.Add({v12});
      }
    }
  }
  for (auto [v15] : vec10) {
    {
      const auto d1 = q_4.SubExplicit({v15});
      if (d1.crossed) {
        vec16.Add({v15});
      }
    }
  }
  vec9.Clear();
  vec10.Clear();
  flow_35(allocator, q_4, g8, std::move(vec13), std::move(vec16));
  return false;
}

inline bool flow_35(::hyde::rt::Allocator &allocator,
                    ::hyde::rt::DiffTable<Row4> &q_4, uint64_t &g8,
                    ::hyde::rt::Vec<Tup_i32> vec13,
                    ::hyde::rt::Vec<Tup_i32> vec16) {
  ::hyde::rt::Vec<Tup_i32> vec17(allocator);
  ::hyde::rt::Vec<Tup_i32> vec20(allocator);
  ::hyde::rt::Vec<Tup_i32> vec23(allocator);
  ::hyde::rt::Vec<Tup_i32> vec26(allocator);
  g8 += 1;
  vec16.SortAndUnique();
  for (auto [v19] : vec16) {
    {
      const auto id2 = q_4.Find({v19});
      if (id2 != ::hyde::rt::kNoRow && q_4.TryClaimDel(id2)) {
        vec17.Add({v19});
      }
    }
  }
  vec13.SortAndUnique();
  for (auto [v22] : vec13) {
    {
      const auto id3 = q_4.Find({v22});
      if (id3 != ::hyde::rt::kNoRow && q_4.TryClaimAdd(id3)) {
        vec20.Add({v22});
      }
    }
  }
  for (auto [v25] : vec17) {
    {
      const uint32_t m4 = q_4.Find({v25});
      if (m4 != ::hyde::rt::kNoRow && q_4.NetDeleted(m4)) {
        vec23.Add({v25});
      }
    }
  }
  for (auto [v28] : vec20) {
    {
      const uint32_t m5 = q_4.Find({v28});
      if (m5 != ::hyde::rt::kNoRow && q_4.NetAdded(m5)) {
        vec26.Add({v28});
      }
    }
  }
  q_4.Commit([](const Row4 &, bool) {});
#ifndef NDEBUG
  q_4.DebugValidateCounts();
#endif
  return true;
}
