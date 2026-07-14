// Self-checking differential oracle for non-linear transitive closure.
//
// Part 1 is the deterministic hazard: with edges {3->1, 1->1, 4->3}, one
// batch adds (2,3) and removes (1,1). The removal cascade transiently marks
// tc(3,1) unknown inside the induction, and the join for the new edge must
// still derive tc(2,1) after tc(3,1) is re-proven.
//
// Part 2 replays randomized add/remove batches for several seeds and
// compares reachable_ff(db) against a naive from-scratch recomputation
// after every batch. Exits nonzero on any mismatch, so every optimization
// mode is checked for correctness, not just cross-mode agreement.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <utility>
#include <vector>

#include "datalog.h"

using Edge = std::pair<uint64_t, uint64_t>;

static std::set<Edge> NaiveTC(const std::set<Edge> &edges) {
  std::set<Edge> tc(edges);
  for (bool changed = true; changed;) {
    changed = false;
    std::vector<Edge> pending;
    for (const auto &[a, b] : tc) {
      for (const auto &[c, d] : tc) {
        if (b == c && !tc.count({a, d})) {
          pending.emplace_back(a, d);
        }
      }
    }
    for (const auto &e : pending) {
      changed = true;
      tc.insert(e);
    }
  }
  return tc;
}

static std::set<Edge> Reachable(Database &db) {
  std::set<Edge> got;
  auto c = reachable_ff(db);
  for (uint64_t f = 0, t = 0; c.next(f, t);) {
    got.insert({f, t});
  }
  return got;
}

// Simple deterministic PRNG so all platforms replay identical batches.
static uint64_t rng_state;
static uint64_t NextRand(void) {
  rng_state = rng_state * 6364136223846793005ull + 1442695040888963407ull;
  return rng_state >> 33;
}

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  auto ok = true;

  // Part 1: deterministic same-batch add+remove hazard.
  {
    DatabaseFunctors functors;
    DatabaseLog log;
    Database db(allocator);
    init(db, log, functors);

    {
      hyde::rt::Vec<Tup_u64_u64> add(allocator);
      hyde::rt::Vec<Tup_u64_u64> rem(allocator);
      add.Add({3, 1});
      add.Add({1, 1});
      add.Add({4, 3});
      add_edge_2(db, log, functors, std::move(add), std::move(rem));
    }
    {
      hyde::rt::Vec<Tup_u64_u64> add(allocator);
      hyde::rt::Vec<Tup_u64_u64> rem(allocator);
      add.Add({2, 3});
      rem.Add({1, 1});
      add_edge_2(db, log, functors, std::move(add), std::move(rem));
    }

    const std::set<Edge> expect =
        NaiveTC({{3, 1}, {4, 3}, {2, 3}});
    const std::set<Edge> got = Reachable(db);
    std::cout << "deterministic:";
    for (const auto &[f, t] : got) {
      std::cout << " (" << f << ',' << t << ')';
    }
    if (got != expect) {
      std::cout << "  WRONG";
      ok = false;
    }
    std::cout << '\n';
  }

  // Part 2: randomized batches vs. from-scratch oracle.
  const uint64_t kNodes = 4;
  // 30 seeds: the Stage-3 merge criterion's randomized breadth, pinned
  // in-suite at the (e) ratification pass (was 12; driver runtime is
  // negligible next to the per-mode clang compile).
  for (uint64_t seed = 1; seed <= 30; ++seed) {
    rng_state = seed * 0x9e3779b97f4a7c15ull + 1;

    DatabaseFunctors functors;
    DatabaseLog log;
    Database db(allocator);
    init(db, log, functors);
    std::set<Edge> edges;

    for (int round = 0; round < 40; ++round) {
      hyde::rt::Vec<Tup_u64_u64> add(allocator);
      hyde::rt::Vec<Tup_u64_u64> rem(allocator);
      std::set<Edge> touched;  // No add+remove of the same tuple in a batch.
      const int ops = 1 + static_cast<int>(NextRand() % 3);
      for (int i = 0; i < ops; ++i) {
        const uint64_t a = 1 + NextRand() % kNodes;
        const uint64_t b = 1 + NextRand() % kNodes;
        const bool removing = (NextRand() % 2) && !edges.empty();
        if (removing) {
          auto it = edges.begin();
          std::advance(it, static_cast<long>(NextRand() % edges.size()));
          if (touched.count(*it)) {
            continue;
          }
          touched.insert(*it);
          rem.Add({it->first, it->second});
          edges.erase(it);
        } else if (!edges.count({a, b}) && !touched.count({a, b})) {
          touched.insert({a, b});
          add.Add({a, b});
          edges.insert({a, b});
        }
      }
      add_edge_2(db, log, functors, std::move(add), std::move(rem));

      const std::set<Edge> expect = NaiveTC(edges);
      const std::set<Edge> got = Reachable(db);
      if (got != expect) {
        std::cout << "MISMATCH seed=" << seed << " round=" << round;
        std::cout << "\nmissing:";
        for (const auto &e : expect) {
          if (!got.count(e)) {
            std::cout << " (" << e.first << ',' << e.second << ')';
          }
        }
        std::cout << "\nextra:";
        for (const auto &e : got) {
          if (!expect.count(e)) {
            std::cout << " (" << e.first << ',' << e.second << ')';
          }
        }
        std::cout << '\n';
        ok = false;
        break;
      }
    }
    if (ok) {
      std::cout << "seed " << seed << " OK\n";
    }
  }

  return ok ? 0 : 1;
}
