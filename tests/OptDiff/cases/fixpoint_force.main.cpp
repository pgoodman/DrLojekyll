#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "datalog.h"

// P6.2 F28 referee driver. `c`/`a`/`b`/`p` are a monotone diamond
// (p = a ∪ b, a = b = c, c = src_c). `out_p` publishes `p`, observed via the log
// hook. The answer is a complete read, invariant across all four optimization
// modes; the DISCRIMINATING F28 gate is the `.region` nodf/none golden's
// `shared-field members=(c.K, p.K, a.K, b.K)` (p.K promoted only after a 2nd
// fixpoint iteration). This driver pins answer-invariance.
struct PrintLog {
  std::vector<std::string> rows;

  void out_p_2(uint64_t K, uint64_t V, bool added) {
    rows.push_back(std::string(added ? "+(" : "-(") + std::to_string(K) + "," +
                   std::to_string(V) + ")");
  }

  void flush(const char *label) {
    std::sort(rows.begin(), rows.end());
    std::cout << label << ":";
    for (const auto &r : rows) {
      std::cout << ' ' << r;
    }
    std::cout << '\n';
    rows.clear();
  }
};

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  PrintLog log;
  Database db(allocator);
  init(db, log, functors);
  log.flush("init");

  hyde::rt::Vec<src_c_input> v(allocator);
  v.Add({1, 100});
  v.Add({2, 200});
  src_c_2(db, log, functors, std::move(v));
  log.flush("after src_c");
  return 0;
}
