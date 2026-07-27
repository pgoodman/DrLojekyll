// Driver for agg_distinct_1: the aggregate multiplicity-semantics witness
// (docs/Language.md "Aggregation"). Prints, after each batch, the
// @invertible and @recompute counts of the same projected shape (they must
// agree -- the algebra is a lowering selector), the count-unique result, and
// the row-counting-recipe result. ADL hidden-friend surface; every keyed
// cursor drain is SORTED before printing (cursor contract, CLAUDE.md).
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

#include "datalog.h"

// C-5 reduction free functions. cinv (@invertible) counts group members via
// O(1) combine/uncombine; crec (@recompute) rescans the live multiset --
// each surviving value contributes its live count (the shipped aggregate_1
// idiom; band (a) folds per presence transition, so counts are 0/1 and this
// equals the number of distinct live rows).
int32_t cinv_identity() {
  return 0;
}
int32_t cinv_combine(int32_t w, int32_t) {
  return w + 1;
}
int32_t cinv_uncombine(int32_t w, int32_t) {
  return w - 1;
}
int32_t crec_reduce(const int32_t *, const int32_t *counts, std::size_t n) {
  int32_t total = 0;
  for (std::size_t i = 0; i < n; ++i) {
    if (counts[i] > 0) {
      total += counts[i];
    }
  }
  return total;
}

namespace {

template <typename Cursor>
void DumpPairs(const char *tag, Cursor c) {
  std::vector<std::tuple<int32_t, int32_t>> rows;
  for (int32_t a, b; c.next(a, b);) {
    rows.emplace_back(a, b);
  }
  std::sort(rows.begin(), rows.end());
  std::cout << tag << ":";
  for (const auto &[a, b] : rows) {
    std::cout << " (" << a << "," << b << ")";
  }
  std::cout << "\n";
}

void Dump(Database &db) {
  DumpPairs("inv", get_inv_ff(db));
  DumpPairs("rec", get_rec_ff(db));
  DumpPairs("ux", get_ux_ff(db));
  DumpPairs("rows", get_rows_ff(db));
  std::cout << "--\n";
}

}  // namespace

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  // Batch 1: X=5 has TWO incoming edges with the SAME weight 10 (ids 1,2);
  // X=6 and X=7 one edge each. Distinct-(X,W) semantics: inv/rec (5,1);
  // row semantics would say (5,2) -- the recipe (rows) reports it.
  {
    hyde::rt::Vec<edge_input> vec(allocator);
    vec.Add({1, 5, 10});
    vec.Add({2, 5, 10});
    vec.Add({3, 6, 20});
    vec.Add({4, 7, 30});
    edge_3(db, log, functors, std::move(vec));
  }
  Dump(db);

  // Batch 2: a THIRD same-weight edge into 5 (still distinct-1) and a
  // second, NEW-weight edge into 6 (distinct-2). ux stays 3 (unique Xs).
  {
    hyde::rt::Vec<edge_input> vec(allocator);
    vec.Add({5, 5, 10});
    vec.Add({6, 6, 21});
    edge_3(db, log, functors, std::move(vec));
  }
  Dump(db);
  return 0;
}
