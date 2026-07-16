// Copyright 2026, Trail of Bits, Inc. All rights reserved.
//
// drlojekyll-oracle: the executable specification of the derivation-counter
// differential-maintenance design (docs/proposals/StackSafeNegation.md §5).
//
// The oracle parses a Datalog module, builds the UNOPTIMIZED dataflow IR
// (`Query::Build(module, log, /*optimize=*/false)` — maximal independence
// from the optimizer), and interprets it twice per received batch:
//
//  1. An INCREMENTAL path that implements the §5 algorithms literally:
//     ingest netting (§5.0), per-stratum OVERDELETE → REDERIVE → INSERT
//     with the §5.1 seed/fixpoint delta schemas and their two read-state
//     tables, the §5.4 negation crossover, BUILDFRONTIERS, and the §5.5
//     commit sweep — with split signed per-row counters (C_nr, C_r) whose
//     derivation class comes from `QueryView::DerivationClassInto`.
//  2. An independent FROM-SCRATCH path: a plain semi-naive stratified
//     set evaluation (no counters) recomputing the whole materialization
//     from the accumulated explicit facts.
//
// After EVERY batch it asserts: (a) incremental presence equals
// from-scratch presence for every view; (b) every incremental counter
// equals the from-scratch derivation-instance count for its class
// (instance counts computed by enumerating rule instances over the final
// materialization, plus one C_nr for explicit message support); (c) all
// counters are non-negative and all frontier queues are drained at commit.
// Any mismatch aborts with a dump of the offending view/row/counters.
//
// Materialization model: EVERY dataflow view gets its own row set (a
// per-view "table" of rows with counters). Data-model merging — several
// views sharing one physical TABLE — is a table-layer/codegen concern that
// the oracle deliberately ignores; counting per view is strictly finer
// grained and independent of the model assignment the compiler chooses.
//
// Rules: each non-source view is the head of one or more single "rules"
// whose body atoms are its predecessor views: TUPLE/CMP/MAP/INSERT are
// one-atom pass-throughs, a MERGE is one rule per merged arm, a
// relation-SELECT is one rule per INSERT into the relation, a JOIN is one
// rule over its joined views, and a NEGATE is one positive atom plus one
// negated atom (its `NegatedView`). RECEIVEs have no rules: their rows are
// the explicit (message) facts, counted as +1 C_nr while present. A rule's
// derivation class is kRecursive iff any body atom's view has
// `DerivationClassInto(head) == kRecursive` (for one-atom rules this is
// exactly the deriving edge's class; a rule with a same-stratum body atom
// is always recursive).
//
// Aggregates, KV indices, and the aggregate corpus's MAP functors ARE
// supported (§4 R3d, docs/proposals/DeltaRelationalIR.artifacts/
// v3-spec-statecell.md): aggregates/KV are recomputed DEFINITIONALLY (group
// the summarized input rows by (group ++ config) / key, run the by-name
// reduction per group) in BOTH the from-scratch and the incremental paths,
// so the per-view presence + counter cross-check extends to aggregate views
// for free; MAP functors are implemented BY NAME (see the by-name functor
// semantics block below — the corpus drivers MUST match those semantics).
// Still rejected up front (clean diagnostics): unimplemented MAP functors,
// multi-summary/multi-value aggregates, and float/foreign-typed columns.
// Unstratified (in-SCC) negation never reaches the oracle: the dataflow
// Stratify pass rejects it inside `Query::Build`.
//
// Same-batch explicit add+remove of one fact NETS TO ZERO at ingest
// (§5.0/§5.5) — deterministic, and a known divergence from the current
// compiler's order-dependent outcome (negation_flap flap B).
//
// ---------------------------------------------------------------------
// .batches sidecar grammar (line oriented; '#' starts a comment):
//
//   file    :=  { batch }
//   batch   :=  "batch" NL { op } "end" NL
//   op      :=  ("+" | "-") msgname value* NL
//
// `msgname` resolves against the module's received messages by (name,
// value-count); "-" ops are only legal on @differential messages. Values
// are the lexemes drivers pass: decimal or 0x-hex integers (negative only
// for signed columns; each value must fit the declared column width), and
// "true"/"false"/"1"/"0" for booleans. One batch..end block is one
// received message batch = one epoch (§5.0): its ops are netted per row
// and applied as a single mixed add/remove batch, then the strata run and
// the per-batch cross-assertions fire.
//
// CLI:
//   drlojekyll-oracle <case.dr> <case.batches>
//   drlojekyll-oracle <case.dr> --stress <seed> <rounds>
//
// Output: a summary line "ORACLE: OK (N batches, M assertions)" followed
// by every named (non-condition) relation's final rows in canonical
// sorted form, one row per line: "relname<TAB>v1 v2 ...". Stress mode
// appends a "STRESS: seed=<s> rounds=<r> digest=<hex>" line. Non-zero
// exit and an "ORACLE: FAIL ..." diagnostic on any assertion failure.
//
// No construct in this file recurses over derivation chains: every
// fixpoint is a queue-driven loop. The only recursion is the bounded
// descent over a rule's body positions (a handful of atoms).

#include <drlojekyll/DataFlow/Query.h>
#include <drlojekyll/Display/DisplayConfiguration.h>
#include <drlojekyll/Display/DisplayManager.h>
#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Parse/Parse.h>
#include <drlojekyll/Parse/Parser.h>
#include <drlojekyll/Parse/Type.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using Value = uint64_t;
using Row = std::vector<Value>;

struct RowHash {
  size_t operator()(const Row &r) const noexcept {
    uint64_t h = 0x9E3779B97F4A7C15ull;
    for (auto v : r) {
      h ^= v + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
    }
    return static_cast<size_t>(h);
  }
};

[[noreturn]] void Fail(const std::string &msg) {
  std::cerr << "ORACLE: FAIL " << msg << std::endl;
  std::exit(EXIT_FAILURE);
}

bool IsSignedKind(hyde::TypeKind k) {
  switch (k) {
    case hyde::TypeKind::kSigned8:
    case hyde::TypeKind::kSigned16:
    case hyde::TypeKind::kSigned32:
    case hyde::TypeKind::kSigned64: return true;
    default: return false;
  }
}

unsigned BitWidth(hyde::TypeKind k) {
  switch (k) {
    case hyde::TypeKind::kBoolean: return 1;
    case hyde::TypeKind::kSigned8:
    case hyde::TypeKind::kUnsigned8: return 8;
    case hyde::TypeKind::kSigned16:
    case hyde::TypeKind::kUnsigned16: return 16;
    case hyde::TypeKind::kSigned32:
    case hyde::TypeKind::kUnsigned32: return 32;
    case hyde::TypeKind::kSigned64:
    case hyde::TypeKind::kUnsigned64: return 64;
    default: return 0;
  }
}

// Parse one value lexeme against a column type; sign-extend signed values
// into the canonical 64-bit representation. Returns nothing on any lexical
// or range error.
std::optional<Value> ParseValue(hyde::TypeKind k, const std::string &tok) {
  if (k == hyde::TypeKind::kBoolean) {
    if (tok == "true" || tok == "1") {
      return Value{1};
    }
    if (tok == "false" || tok == "0") {
      return Value{0};
    }
    return std::nullopt;
  }
  const unsigned w = BitWidth(k);
  if (!w) {
    return std::nullopt;
  }
  errno = 0;
  char *end = nullptr;
  if (IsSignedKind(k)) {
    const long long v = std::strtoll(tok.c_str(), &end, 0);
    if (errno || !end || *end) {
      return std::nullopt;
    }
    const long long lo = w == 64 ? INT64_MIN : -(1ll << (w - 1));
    const long long hi = w == 64 ? INT64_MAX : (1ll << (w - 1)) - 1;
    if (v < lo || v > hi) {
      return std::nullopt;
    }
    return static_cast<Value>(static_cast<int64_t>(v));
  } else {
    if (!tok.empty() && tok[0] == '-') {
      return std::nullopt;
    }
    const unsigned long long v = std::strtoull(tok.c_str(), &end, 0);
    if (errno || !end || *end) {
      return std::nullopt;
    }
    if (w < 64 && v > ((1ull << w) - 1ull)) {
      return std::nullopt;
    }
    return static_cast<Value>(v);
  }
}

std::string PrintValue(hyde::TypeKind k, Value v) {
  if (k == hyde::TypeKind::kBoolean) {
    return v ? "true" : "false";
  }
  if (IsSignedKind(k)) {
    return std::to_string(static_cast<int64_t>(v));
  }
  return std::to_string(v);
}

// Typed row comparison for canonical output ordering.
bool TypedRowLess(const std::vector<hyde::TypeKind> &ts, const Row &a,
                  const Row &b) {
  for (size_t i = 0, n = std::min(a.size(), b.size()); i < n; ++i) {
    const bool sgn = i < ts.size() && IsSignedKind(ts[i]);
    if (sgn) {
      const auto x = static_cast<int64_t>(a[i]), y = static_cast<int64_t>(b[i]);
      if (x != y) {
        return x < y;
      }
    } else if (a[i] != b[i]) {
      return a[i] < b[i];
    }
  }
  return a.size() < b.size();
}

// ---------------------------------------------------------------------
// Incremental per-view state: an append-only row log plus per-row split
// signed counters and batch-transient flags (§3.1).

struct RowState {
  int64_t c_nr{0};
  int64_t c_r{0};
  bool in_i{false};      // kInI: present at batch start (frozen per batch)
  bool del{false};       // kDel: claimed by the overdeletion set D
  bool add{false};       // kAdd: claimed by the addition set A
  bool del_now{false};   // kDelNow: in the CURRENT delete frontier round
  bool add_now{false};   // kAddNow: in the CURRENT insert frontier round
  bool is_explicit{false};  // kExplicit: +1 C_nr of message support
  bool touched{false};   // dedup bit for the per-batch `touched` list
};

// Named membership predicates (§3.1). These are the ONLY forms in which
// the delta-schema joins read incremental state.
enum class ReadK : uint8_t {
  kInI,
  kInNew,
  kSurvivesSoFar,
  kAliveAtClaim,
  kInNewWithFrontier,
  kInNewSansFrontier,
};

bool Pass(const RowState &s, ReadK k) {
  switch (k) {
    case ReadK::kInI: return s.in_i;
    case ReadK::kInNew: return (s.in_i && !s.del) || s.add;
    case ReadK::kSurvivesSoFar: return s.in_i && !s.del;
    case ReadK::kAliveAtClaim: return s.in_i && (!s.del || s.del_now);
    case ReadK::kInNewWithFrontier: return (s.in_i && !s.del) || s.add;
    case ReadK::kInNewSansFrontier:
      return (s.in_i && !s.del) || (s.add && !s.add_now);
  }
  return false;
}

// Append-only per-key index over a row log: rows are added once (mirroring
// the runtime's append-only Index), liveness is filtered at read time by
// the membership predicates.
struct KeyIndex {
  std::unordered_map<Row, std::vector<uint32_t>, RowHash> map;
  size_t consumed{0};
};

struct ViewModel {
  hyde::QueryView view;
  std::string name;
  unsigned ord{0};
  unsigned stratum{0};
  unsigned arity{0};
  bool is_receive{false};
  std::vector<hyde::TypeKind> types;

  // Append-only log + interning.
  std::vector<Row> rows;
  std::unordered_map<Row, uint32_t, RowHash> ids;
  std::vector<RowState> st;

  // Batch-transient bookkeeping.
  std::vector<uint32_t> touched;
  std::vector<uint32_t> pend_del, pend_add;      // delQ / addQ entries
  std::vector<uint32_t> claimed_del, claimed_add;  // D_s / A_s
  std::vector<uint32_t> out_del, out_add;          // D\A / A\D frontiers

  std::map<std::vector<unsigned>, KeyIndex> indexes;

  explicit ViewModel(hyde::QueryView v) : view(v) {}

  uint32_t Intern(const Row &r) {
    auto it = ids.find(r);
    if (it != ids.end()) {
      return it->second;
    }
    const auto id = static_cast<uint32_t>(rows.size());
    rows.push_back(r);
    st.emplace_back();
    ids.emplace(r, id);
    return id;
  }

  KeyIndex &GetIndex(const std::vector<unsigned> &cols) {
    auto &ix = indexes[cols];
    for (; ix.consumed < rows.size(); ++ix.consumed) {
      Row key;
      key.reserve(cols.size());
      for (auto c : cols) {
        key.push_back(rows[ix.consumed][c]);
      }
      ix.map[key].push_back(static_cast<uint32_t>(ix.consumed));
    }
    return ix;
  }
};

// From-scratch per-view row set: presence only, no counters.
struct ScratchView {
  std::vector<Row> rows;
  std::unordered_map<Row, uint32_t, RowHash> ids;
  std::map<std::vector<unsigned>, KeyIndex> indexes;

  bool Add(const Row &r) {
    if (ids.count(r)) {
      return false;
    }
    ids.emplace(r, static_cast<uint32_t>(rows.size()));
    rows.push_back(r);
    return true;
  }

  KeyIndex &GetIndex(const std::vector<unsigned> &cols) {
    auto &ix = indexes[cols];
    for (; ix.consumed < rows.size(); ++ix.consumed) {
      Row key;
      key.reserve(cols.size());
      for (auto c : cols) {
        key.push_back(rows[ix.consumed][c]);
      }
      ix.map[key].push_back(static_cast<uint32_t>(ix.consumed));
    }
    return ix;
  }
};

// ---------------------------------------------------------------------
// BY-NAME FUNCTOR SEMANTICS  (§4 coordination contract; critique HIGH #5).
//
// The oracle implements the aggregate/merge/MAP functors BY NAME as pure
// functions. These semantics are NORMATIVE: the corpus drivers (which
// supply these functors as free functions per §C-5 of the R3 spec) MUST
// match them EXACTLY, or the blessed goldens diverge from real runs.
//
//   MAP functors (@range(.); one output row per input iff the functor
//   "succeeds"):
//     div_i32(LHS, RHS) -> LHS / RHS   (signed integer division, truncating
//                          toward zero — C++ `/` on int32_t). GUARD: RHS == 0
//                          produces NO output row (the @range functor yields
//                          zero tuples), so a group with Count == 0 simply
//                          does not appear in the query output.
//     add_i32(LHS, RHS) -> LHS + RHS   (signed 32-bit addition; wraps in the
//                          two's-complement int32 domain, matching a C++
//                          int32_t add — no overflow diagnostic).
//
//   AGGREGATE reductions (over a group's members):
//     sum_i32   -> the running sum of the aggregated i32 column over the
//                  group's members (@invertible: fold(+)=+v, fold(-)=-v).
//     count_i32 -> the count of members in the group (@invertible).
//
//   KV-INDEX merge (the degenerate aggregate; group = key, value = the
//   surviving merged value):
//     new_weight_i32 (@recompute, un-annotated invertibility) -> the merge
//                  is LAST-WRITER: a KV index holds exactly ONE value per
//                  key, so the surviving value is that of the last-surviving
//                  member. In the definitional from-scratch reduction there
//                  is at most one live (From, To) member per key here (the
//                  edge_weight key is the full (From,To,Weight) input row's
//                  key projection), so "last wins" is well-defined: the
//                  merged value is the value of the group's sole member; if
//                  a key ever had multiple distinct values, the last one
//                  interned (deterministically, input-row order) wins.
//
// div-by-zero rationale: div_i32 is @range(.); a range functor emitting zero
// rows on a bad input is the modeled "produce no tuple" behavior. This
// matches average_incoming_weight over an empty group (no incoming edges =>
// Count 0 => no Avg row) — a clean, driver-matchable contract.
// ---------------------------------------------------------------------

enum class MapFn : uint8_t { kNone, kDivI32, kAddI32 };

// Apply a MAP functor by name to its bound integer args. Returns nothing when
// the @range functor "fails" (div by zero) — no output tuple is produced.
std::optional<Value> ApplyMapFn(MapFn fn, const std::vector<Value> &args) {
  switch (fn) {
    case MapFn::kNone: return std::nullopt;
    case MapFn::kDivI32: {
      if (args.size() != 2) {
        return std::nullopt;
      }
      const auto rhs = static_cast<int32_t>(static_cast<int64_t>(args[1]));
      if (rhs == 0) {
        return std::nullopt;  // @range: div-by-zero => no output row.
      }
      const auto lhs = static_cast<int32_t>(static_cast<int64_t>(args[0]));
      const int32_t res = lhs / rhs;
      return static_cast<Value>(static_cast<int64_t>(res));
    }
    case MapFn::kAddI32: {
      if (args.size() != 2) {
        return std::nullopt;
      }
      const auto lhs = static_cast<int32_t>(static_cast<int64_t>(args[0]));
      const auto rhs = static_cast<int32_t>(static_cast<int64_t>(args[1]));
      const auto res = static_cast<int32_t>(
          static_cast<uint32_t>(lhs) + static_cast<uint32_t>(rhs));
      return static_cast<Value>(static_cast<int64_t>(res));
    }
  }
  return std::nullopt;
}

// Resolve a MAP functor by (name, arity) to the by-name implementation.
MapFn ResolveMapFn(const std::string &name, unsigned num_bound) {
  if (name == "div_i32" && num_bound == 2) {
    return MapFn::kDivI32;
  }
  if (name == "add_i32" && num_bound == 2) {
    return MapFn::kAddI32;
  }
  return MapFn::kNone;
}

// ---------------------------------------------------------------------
// Rules.

struct Slot {
  bool is_const{false};
  bool is_map_result{false};  // value comes from the rule's MAP functor result
  unsigned atom{0};
  unsigned col{0};
  Value cval{0};
};

struct BodyAtom {
  ViewModel *vm{nullptr};
  bool negated{false};
  unsigned stratum{0};
};

struct Rule {
  ViewModel *head{nullptr};
  std::vector<BodyAtom> atoms;      // syntactic order
  std::vector<unsigned> pos2atom;   // schema position -> atom index (§5.1:
                                    // lower strata first, then same-stratum,
                                    // syntactic order within each group)
  std::vector<unsigned> lower_pos, same_pos;  // schema positions

  // Join constraints: equality classes over (atom, col) slots + constants.
  unsigned num_classes{0};
  std::vector<std::optional<Value>> class_const;
  // classes_of[atom][col] -> class ids that column participates in.
  std::vector<std::vector<std::vector<unsigned>>> classes_of;

  std::vector<Slot> head_tmpl;

  struct Filter {
    hyde::ComparisonOperator op;
    Slot lhs, rhs;
    bool sgn{false};
  };
  std::vector<Filter> filters;

  // For NEGATE rules: the negated atom index and the key slots (one per
  // negated-view column, referencing the positive atom or constants).
  int neg_atom{-1};
  std::vector<Slot> neg_key;

  // For MAP rules: the named functor and the slots feeding its bound args
  // (referencing the single body atom or constants). A `nullopt` result (e.g.
  // div-by-zero) suppresses the instance — @range functors emit zero rows.
  MapFn map_fn{MapFn::kNone};
  std::vector<Slot> map_args;

  hyde::DerivClass klass{hyde::DerivClass::kNonRecursive};
};

bool EvalFilter(const Rule::Filter &f, Value l, Value r) {
  switch (f.op) {
    case hyde::ComparisonOperator::kEqual: return l == r;
    case hyde::ComparisonOperator::kNotEqual: return l != r;
    case hyde::ComparisonOperator::kLessThan:
      return f.sgn ? (static_cast<int64_t>(l) < static_cast<int64_t>(r))
                   : (l < r);
    case hyde::ComparisonOperator::kGreaterThan:
      return f.sgn ? (static_cast<int64_t>(l) > static_cast<int64_t>(r))
                   : (l > r);
  }
  return false;
}

// ---------------------------------------------------------------------
// Row sources for the shared rule enumerator.

// Incremental source: candidate rows filtered by the per-position named
// membership predicate.
struct IncSrc {
  const std::vector<ReadK> *reads{nullptr};

  template <typename CB>
  void ForEach(const BodyAtom &a, unsigned pos,
               const std::vector<unsigned> &key_cols, const Row &key_vals,
               CB &&cb) const {
    const ReadK k = (*reads)[pos];
    if (key_cols.empty()) {
      for (uint32_t id = 0, n = static_cast<uint32_t>(a.vm->rows.size());
           id < n; ++id) {
        if (Pass(a.vm->st[id], k)) {
          cb(a.vm->rows[id]);
        }
      }
    } else {
      auto &ix = a.vm->GetIndex(key_cols);
      auto it = ix.map.find(key_vals);
      if (it == ix.map.end()) {
        return;
      }
      for (auto id : it->second) {
        if (Pass(a.vm->st[id], k)) {
          cb(a.vm->rows[id]);
        }
      }
    }
  }

  bool Contains(const BodyAtom &a, unsigned pos, const Row &row) const {
    auto it = a.vm->ids.find(row);
    return it != a.vm->ids.end() && Pass(a.vm->st[it->second], (*reads)[pos]);
  }
};

// From-scratch source: plain set membership; the read-state tables do not
// apply (there is exactly one, final, state).
struct ScratchSrc {
  std::vector<ScratchView> *views{nullptr};

  template <typename CB>
  void ForEach(const BodyAtom &a, unsigned /*pos*/,
               const std::vector<unsigned> &key_cols, const Row &key_vals,
               CB &&cb) const {
    auto &sv = (*views)[a.vm->ord];
    if (key_cols.empty()) {
      // Index by size first: rows may be appended to OTHER views by the
      // sink; this view is never a head of the rule being enumerated.
      for (size_t i = 0; i < sv.rows.size(); ++i) {
        cb(sv.rows[i]);
      }
    } else {
      auto &ix = sv.GetIndex(key_cols);
      auto it = ix.map.find(key_vals);
      if (it == ix.map.end()) {
        return;
      }
      for (auto id : it->second) {
        cb(sv.rows[id]);
      }
    }
  }

  bool Contains(const BodyAtom &a, unsigned /*pos*/, const Row &row) const {
    return (*views)[a.vm->ord].ids.count(row) != 0;
  }
};

// ---------------------------------------------------------------------
// The oracle.

class Oracle {
 public:
  hyde::DisplayManager display_manager;
  hyde::ErrorLog error_log{display_manager};
  std::optional<hyde::Query> query;

  std::vector<std::unique_ptr<ViewModel>> models;
  std::unordered_map<uint64_t, ViewModel *> by_view;  // QueryView::UniqueId
  unsigned num_strata{0};
  std::vector<std::vector<ViewModel *>> views_by_stratum;

  std::vector<std::unique_ptr<Rule>> rules;
  std::vector<std::vector<Rule *>> rules_by_stratum;

  // Aggregate / KV-index views: recomputed DEFINITIONALLY (§4) from their
  // single summarized input view's rows, grouped by (group ++ config) (for a
  // KV index: keyed by the key columns), with the by-name reduction run over
  // each group's members. The same recompute drives BOTH the from-scratch and
  // the incremental paths, so the per-view presence cross-check extends to
  // aggregate views for free (spec §4). A `Slot` here references a column
  // POSITION in the input view, or a constant.
  enum class AggKind : uint8_t { kSum, kCount, kMerge };
  struct AggInfo {
    ViewModel *head{nullptr};   // the aggregate/KV output view
    ViewModel *input{nullptr};  // the single summarized input view
    AggKind kind{AggKind::kSum};
    // Input-column slots for the grouping key: group ++ config (agg) or the
    // key columns (KV). These form the group key AND the leading output cols.
    std::vector<Slot> key_slots;
    // The aggregated / value column slot (the reduced column). For count this
    // is still read (to iterate members) but its value is unused.
    Slot val_slot;
    unsigned arity{0};  // output arity = key_slots.size() + 1 (one summary)
  };
  std::vector<AggInfo> aggs;
  // Per stratum: the aggregate views whose output lands in that stratum.
  std::vector<std::vector<unsigned>> aggs_by_stratum;  // indices into `aggs`
  // For the scratch fixpoint: (rule, schema position) pairs where a view
  // occupies a same-stratum positive position.
  std::unordered_map<ViewModel *, std::vector<std::pair<Rule *, unsigned>>>
      same_pos_users;

  struct MsgInfo {
    std::string name;
    unsigned arity{0};
    bool differential{false};
    std::vector<hyde::TypeKind> types;
    std::vector<ViewModel *> receives;
  };
  std::vector<MsgInfo> msgs;
  std::map<std::pair<std::string, unsigned>, unsigned> msg_by_name_arity;

  // From-scratch materialization of the current batch.
  std::vector<ScratchView> scratch;
  // Expected (C_nr, C_r) per view row, from enumerating rule instances of
  // the from-scratch materialization.
  std::vector<std::unordered_map<Row, std::pair<int64_t, int64_t>, RowHash>>
      expected;

  size_t batches_run{0};
  size_t checks{0};
  bool init_fired{false};

  // ------------------------------------------------------------------
  // Setup.

  ViewModel *VM(hyde::QueryView v) {
    auto it = by_view.find(v.UniqueId());
    if (it == by_view.end()) {
      Fail("internal: view " + std::string(v.KindName()) +
           " referenced by a rule is not in the interpreted universe");
    }
    return it->second;
  }

  static unsigned StratumOf(hyde::QueryView v) {
    auto s = v.Stratum();
    if (!s) {
      Fail("internal: view " + std::string(v.KindName()) +
           " has no stratum id");
    }
    return *s;
  }

  Value ConstValue(hyde::QueryColumn c) {
    auto qc = hyde::QueryConstant::From(c);
    if (qc.IsTag()) {
      return hyde::QueryTag::From(qc).Value();
    }
    auto lit = qc.Literal();
    if (!lit) {
      // The literal-less constant is the compiler-synthesized boolean
      // `true` token stored by unit (condition) relations.
      return Value{1};
    }
    if (lit->IsEnumerator() || lit->IsString()) {
      Fail("unsupported constant literal kind (enumerator/string)");
    }
    auto sp = lit->Spelling(hyde::Language::kCxx);
    if (!sp) {
      Fail("constant literal has no spelling");
    }
    auto k = c.Type().UnderlyingKind();
    auto v = ParseValue(k, std::string(*sp));
    if (!v) {
      Fail("cannot parse constant literal '" + std::string(*sp) + "'");
    }
    return *v;
  }

  void CheckColumnTypes(ViewModel *vm) {
    for (auto k : vm->types) {
      switch (k) {
        case hyde::TypeKind::kBoolean:
        case hyde::TypeKind::kSigned8:
        case hyde::TypeKind::kSigned16:
        case hyde::TypeKind::kSigned32:
        case hyde::TypeKind::kSigned64:
        case hyde::TypeKind::kUnsigned8:
        case hyde::TypeKind::kUnsigned16:
        case hyde::TypeKind::kUnsigned32:
        case hyde::TypeKind::kUnsigned64: break;
        default:
          Fail("unsupported column type in view " + vm->name +
               " (only bool and fixed-width integers are interpreted)");
      }
    }
  }

  int Load(const char *dr_path) {
    hyde::Parser parser(display_manager, error_log);
    hyde::DisplayConfiguration config = {dr_path, 2, true};
    auto module_opt = parser.ParsePath(dr_path, config);
    if (!module_opt) {
      error_log.Render(std::cerr);
      return EXIT_FAILURE;
    }
    // Unoptimized dataflow: the oracle interprets the graph the aggressive
    // optimization pass never touched.
    query = hyde::Query::Build(*module_opt, error_log, /*optimize=*/false);
    if (!query) {
      error_log.Render(std::cerr);
      return EXIT_FAILURE;
    }

    // Aggregates, KV-indices, and the aggregate corpus's MAP functors are
    // now supported (§4 R3d): aggregates/KV are recomputed definitionally
    // (BuildAggregates + ReduceAggregate) in BOTH the from-scratch and the
    // incremental paths; MAP functors are implemented by name (ApplyMapFn).
    // Unimplemented MAP functors are still rejected loudly at rule build.

    BuildUniverse();
    BuildMessages();
    BuildRules();
    BuildAggregates();
    return EXIT_SUCCESS;
  }

  void BuildUniverse(void) {
    num_strata = query->NumStrata();
    views_by_stratum.assign(num_strata, {});

    auto add_view = [&](hyde::QueryView v) {
      if (v.IsSelect()) {
        auto sel = hyde::QuerySelect::From(v);
        if (sel.IsStream() && sel.Stream().IsConstant()) {
          return;  // Constant columns are read at use sites, not as atoms.
        }
      }
      auto vm = std::make_unique<ViewModel>(v);
      vm->ord = static_cast<unsigned>(models.size());
      vm->stratum = StratumOf(v);
      vm->name = std::string(v.KindName()) + "#" +
                 std::to_string(v.UniqueId()) + "@s" +
                 std::to_string(vm->stratum);
      if (v.IsInsert()) {
        auto ins = hyde::QueryInsert::From(v);
        vm->arity = ins.NumInputColumns();
        for (auto c : ins.InputColumns()) {
          vm->types.push_back(c.Type().UnderlyingKind());
        }
      } else {
        for (auto c : v.Columns()) {
          vm->types.push_back(c.Type().UnderlyingKind());
          ++vm->arity;
        }
      }
      CheckColumnTypes(vm.get());
      by_view.emplace(v.UniqueId(), vm.get());
      views_by_stratum[vm->stratum].push_back(vm.get());
      models.push_back(std::move(vm));
    };

    query->ForEachView(add_view);
  }

  void BuildMessages(void) {
    for (auto io : query->IOs()) {
      const auto &decl = io.Declaration();
      if (!decl.IsMessage()) {
        continue;
      }
      std::vector<ViewModel *> recvs;
      for (auto rv : io.Receives()) {
        auto *vm = VM(rv);
        vm->is_receive = true;
        recvs.push_back(vm);
      }
      if (recvs.empty()) {
        continue;  // Transmit-only message: not an ingest surface.
      }
      MsgInfo mi;
      mi.name = std::string(decl.NameAsString());
      mi.arity = decl.Arity();
      mi.differential = hyde::ParsedMessage::From(decl).IsDifferential();
      mi.types = recvs[0]->types;
      mi.receives = std::move(recvs);
      const auto idx = static_cast<unsigned>(msgs.size());
      if (!msg_by_name_arity.emplace(std::make_pair(mi.name, mi.arity), idx)
               .second) {
        Fail("duplicate received message " + mi.name + "/" +
             std::to_string(mi.arity));
      }
      msgs.push_back(std::move(mi));
    }
  }

  // ------------------------------------------------------------------
  // Rule construction.

  // Resolve an input column to a slot given the rule's atom views.
  Slot ResolveSlot(hyde::QueryColumn c,
                   const std::vector<hyde::QueryView> &atom_views) {
    Slot s;
    if (c.IsConstant()) {
      s.is_const = true;
      s.cval = ConstValue(c);
      return s;
    }
    const auto owner = hyde::QueryView::Containing(c);
    for (unsigned i = 0; i < atom_views.size(); ++i) {
      if (atom_views[i] == owner) {
        s.atom = i;
        s.col = *c.Index();
        return s;
      }
    }
    Fail("internal: input column's defining view is not a body atom");
  }

  // Gather the single positive predecessor view of a set of input columns
  // (constants excluded); fail loud on anything else.
  std::optional<hyde::QueryView> SingleAtomOf(
      const std::vector<hyde::QueryColumn> &cols, const char *what) {
    std::optional<hyde::QueryView> found;
    for (auto c : cols) {
      if (c.IsConstant()) {
        continue;
      }
      auto v = hyde::QueryView::Containing(c);
      if (found && !(*found == v)) {
        Fail(std::string("internal: ") + what +
             " reads columns of two distinct views");
      }
      found = v;
    }
    return found;
  }

  void FinishRule(std::unique_ptr<Rule> rule) {
    Rule &r = *rule;
    const unsigned n = static_cast<unsigned>(r.atoms.size());
    const unsigned hs = r.head->stratum;

    // Ensure class metadata vectors are sized.
    if (r.classes_of.empty()) {
      r.classes_of.assign(n, {});
    }
    for (unsigned i = 0; i < n; ++i) {
      r.classes_of[i].resize(r.atoms[i].vm->arity);
    }

    // Schema position order (§5.1): lower-strata atoms first, in stratum
    // order then syntactic order; then same-stratum atoms in syntactic
    // order.
    r.pos2atom.resize(n);
    for (unsigned i = 0; i < n; ++i) {
      r.pos2atom[i] = i;
    }
    std::stable_sort(r.pos2atom.begin(), r.pos2atom.end(),
                     [&](unsigned a, unsigned b) {
                       const bool sa = r.atoms[a].stratum == hs;
                       const bool sb = r.atoms[b].stratum == hs;
                       if (sa != sb) {
                         return !sa;  // lower before same-stratum
                       }
                       if (!sa && r.atoms[a].stratum != r.atoms[b].stratum) {
                         return r.atoms[a].stratum < r.atoms[b].stratum;
                       }
                       return a < b;  // syntactic order
                     });
    for (unsigned p = 0; p < n; ++p) {
      const auto &a = r.atoms[r.pos2atom[p]];
      if (a.stratum > hs) {
        Fail("internal: body atom in a higher stratum than its head");
      }
      if (a.stratum == hs) {
        if (a.negated) {
          Fail("internal: same-stratum negated atom survived the in-SCC "
               "negation gate");
        }
        r.same_pos.push_back(p);
      } else {
        r.lower_pos.push_back(p);
      }
      if (a.vm == r.head) {
        Fail("internal: a view is a body atom of its own rule");
      }
    }

    // Derivation class: recursive iff any body atom derives over an edge
    // classified recursive into the head (DerivationClassInto).
    r.klass = hyde::DerivClass::kNonRecursive;
    for (const auto &a : r.atoms) {
      if (a.vm->view.DerivationClassInto(r.head->view) ==
          hyde::DerivClass::kRecursive) {
        r.klass = hyde::DerivClass::kRecursive;
      }
    }
    if (!r.same_pos.empty() && r.klass != hyde::DerivClass::kRecursive) {
      Fail("internal: rule with a same-stratum atom classified nonrecursive");
    }

    rules_by_stratum[hs].push_back(&r);
    for (auto p : r.same_pos) {
      same_pos_users[r.atoms[r.pos2atom[p]].vm].push_back({&r, p});
    }
    rules.push_back(std::move(rule));
  }

  std::unique_ptr<Rule> NewRule(ViewModel *head) {
    auto r = std::make_unique<Rule>();
    r->head = head;
    return r;
  }

  void AddAtom(Rule &r, hyde::QueryView v, bool negated) {
    BodyAtom a;
    a.vm = VM(v);
    a.negated = negated;
    a.stratum = a.vm->stratum;
    r.atoms.push_back(a);
  }

  void BuildRules(void) {
    rules_by_stratum.assign(num_strata, {});

    // TUPLE: one pass-through rule (an all-constant tuple is an init rule
    // with no atoms).
    for (auto t : query->Tuples()) {
      const auto head = hyde::QueryView::From(t);
      std::vector<hyde::QueryColumn> ins;
      for (auto c : t.InputColumns()) {
        ins.push_back(c);
      }
      auto r = NewRule(VM(head));
      std::vector<hyde::QueryView> atom_views;
      if (auto p = SingleAtomOf(ins, "TUPLE")) {
        atom_views.push_back(*p);
        AddAtom(*r, *p, false);
      }
      for (auto c : ins) {
        r->head_tmpl.push_back(ResolveSlot(c, atom_views));
      }
      FinishRule(std::move(r));
    }

    // CMP: one filtered pass-through rule.
    for (auto cmp : query->Compares()) {
      const auto head = hyde::QueryView::From(cmp);
      std::vector<hyde::QueryColumn> ins{cmp.InputLHS(), cmp.InputRHS()};
      for (auto c : cmp.InputCopiedColumns()) {
        ins.push_back(c);
      }
      auto r = NewRule(VM(head));
      std::vector<hyde::QueryView> atom_views;
      if (auto p = SingleAtomOf(ins, "COMPARE")) {
        atom_views.push_back(*p);
        AddAtom(*r, *p, false);
      }
      const Slot ls = ResolveSlot(cmp.InputLHS(), atom_views);
      const Slot rs = ResolveSlot(cmp.InputRHS(), atom_views);
      Rule::Filter f;
      f.op = cmp.Operator();
      f.lhs = ls;
      f.rhs = rs;
      f.sgn = IsSignedKind(cmp.InputLHS().Type().UnderlyingKind());
      r->filters.push_back(f);
      r->head_tmpl.push_back(ls);
      if (f.op != hyde::ComparisonOperator::kEqual) {
        r->head_tmpl.push_back(rs);
      }
      for (auto c : cmp.InputCopiedColumns()) {
        r->head_tmpl.push_back(ResolveSlot(c, atom_views));
      }
      FinishRule(std::move(r));
    }

    // MAP: one rule over the single predecessor view. The functor's `bound`
    // params feed its args (from `InputColumns()`); its `free` params are the
    // computed result columns; remaining columns are copied pass-throughs.
    // Output column layout (Query.cpp QueryMap::ForEachUse): the functor
    // params in declaration order (bound => fed by an input column; free =>
    // the map result), then the attached/copied columns.
    for (auto map : query->Maps()) {
      const auto head = hyde::QueryView::From(map);
      std::vector<hyde::QueryColumn> all_ins;
      for (auto c : map.InputColumns()) {
        all_ins.push_back(c);
      }
      for (auto c : map.InputCopiedColumns()) {
        all_ins.push_back(c);
      }
      auto r = NewRule(VM(head));
      std::vector<hyde::QueryView> atom_views;
      if (auto p = SingleAtomOf(all_ins, "MAP")) {
        atom_views.push_back(*p);
        AddAtom(*r, *p, false);
      }

      // The bound args feeding the functor, in `InputColumns()` order.
      const unsigned num_bound = map.NumInputColumns();
      for (auto c : map.InputColumns()) {
        r->map_args.push_back(ResolveSlot(c, atom_views));
      }
      r->map_fn =
          ResolveMapFn(std::string(map.Functor().NameAsString()), num_bound);
      if (r->map_fn == MapFn::kNone) {
        Fail("MAP functor '" + std::string(map.Functor().NameAsString()) +
             "/" + std::to_string(num_bound) +
             "' is not implemented by the oracle (implement it by name in "
             "ApplyMapFn/ResolveMapFn first)");
      }

      // Map each output column to its source. `ForEachUse` covers the bound
      // (kFunctorInput) and copied (kCopied) output columns; the remaining
      // output columns are the functor's free results.
      const unsigned arity = head.Columns().size();
      std::vector<std::optional<Slot>> out_slot(arity);
      std::unordered_map<uint64_t, unsigned> out_idx;
      {
        unsigned i = 0;
        for (auto col : head.Columns()) {
          out_idx.emplace(col.UniqueId(), i++);
        }
      }
      map.ForEachUse([&](hyde::QueryColumn in, hyde::InputColumnRole,
                         std::optional<hyde::QueryColumn> out) {
        if (!out) {
          return;
        }
        const unsigned o = out_idx.at(out->UniqueId());
        out_slot[o] = ResolveSlot(in, atom_views);
      });
      for (unsigned o = 0; o < arity; ++o) {
        if (out_slot[o]) {
          r->head_tmpl.push_back(*out_slot[o]);
        } else {
          Slot s;  // A free result column filled by the MAP functor.
          s.is_map_result = true;
          r->head_tmpl.push_back(s);
        }
      }
      FinishRule(std::move(r));
    }

    // MERGE: one rule per merged arm; columns correspond positionally
    // (an INSERT arm's row is its stored input row, so this also holds
    // for INSERT arms).
    for (auto m : query->Merges()) {
      const auto head = hyde::QueryView::From(m);
      for (auto arm : m.MergedViews()) {
        auto r = NewRule(VM(head));
        AddAtom(*r, arm, false);
        for (unsigned i = 0; i < m.Arity(); ++i) {
          Slot s;
          s.atom = 0;
          s.col = i;
          r->head_tmpl.push_back(s);
        }
        FinishRule(std::move(r));
      }
    }

    // JOIN: one rule; equality classes per output column via ForEachUse.
    for (auto j : query->Joins()) {
      const auto head = hyde::QueryView::From(j);
      auto r = NewRule(VM(head));
      std::vector<hyde::QueryView> joined;
      for (auto v : j.JoinedViews()) {
        for (const auto &prev : joined) {
          if (prev == v) {
            Fail("internal: JOIN joins the same view twice (self-join "
                 "without per-use TUPLEs); the oracle cannot attribute "
                 "input columns to atom instances");
          }
        }
        joined.push_back(v);
        AddAtom(*r, v, false);
      }
      const unsigned n_out = [&] {
        unsigned c = 0;
        for (auto col : j.Columns()) {
          (void) col;
          ++c;
        }
        return c;
      }();
      r->num_classes = n_out;
      r->class_const.assign(n_out, std::nullopt);
      r->classes_of.assign(r->atoms.size(), {});
      for (unsigned i = 0; i < r->atoms.size(); ++i) {
        r->classes_of[i].resize(r->atoms[i].vm->arity);
      }
      std::vector<std::optional<Slot>> rep(n_out);
      // Map each output column to its index.
      std::unordered_map<uint64_t, unsigned> out_idx;
      {
        unsigned i = 0;
        for (auto col : j.Columns()) {
          out_idx.emplace(col.UniqueId(), i++);
        }
      }
      j.ForEachUse([&](hyde::QueryColumn in, hyde::InputColumnRole,
                       std::optional<hyde::QueryColumn> out) {
        if (!out) {
          return;
        }
        const unsigned o = out_idx.at(out->UniqueId());
        if (in.IsConstant()) {
          const Value v = ConstValue(in);
          if (r->class_const[o] && *r->class_const[o] != v) {
            Fail("internal: JOIN output constrained to two constants");
          }
          r->class_const[o] = v;
          return;
        }
        const Slot s = ResolveSlot(in, joined);
        r->classes_of[s.atom][s.col].push_back(o);
        if (!rep[o]) {
          rep[o] = s;
        }
      });
      for (unsigned o = 0; o < n_out; ++o) {
        if (rep[o]) {
          r->head_tmpl.push_back(*rep[o]);
        } else if (r->class_const[o]) {
          Slot s;
          s.is_const = true;
          s.cval = *r->class_const[o];
          r->head_tmpl.push_back(s);
        } else {
          Fail("internal: JOIN output column with no input");
        }
      }
      FinishRule(std::move(r));
    }

    // NEGATE: positive predecessor + negated atom; output columns are the
    // negation-key inputs followed by the copied columns; the negated
    // view's column i is keyed by input column i.
    for (auto neg : query->Negations()) {
      const auto head = hyde::QueryView::From(neg);
      std::vector<hyde::QueryColumn> ins;
      for (auto c : neg.InputColumns()) {
        ins.push_back(c);
      }
      for (auto c : neg.InputCopiedColumns()) {
        ins.push_back(c);
      }
      auto r = NewRule(VM(head));
      std::vector<hyde::QueryView> atom_views;
      if (auto p = SingleAtomOf(ins, "NEGATE")) {
        atom_views.push_back(*p);
        AddAtom(*r, *p, false);
      }
      r->neg_atom = static_cast<int>(r->atoms.size());
      AddAtom(*r, neg.NegatedView(), true);
      for (auto c : neg.InputColumns()) {
        r->neg_key.push_back(ResolveSlot(c, atom_views));
      }
      for (auto c : ins) {
        r->head_tmpl.push_back(ResolveSlot(c, atom_views));
      }
      FinishRule(std::move(r));
    }

    // INSERT: stores its input columns; attached columns are read-only
    // witness edges (each witness row is one derivation of the stored row).
    for (auto ins : query->Inserts()) {
      const auto head = hyde::QueryView::From(ins);
      std::vector<hyde::QueryColumn> cols;
      for (auto c : ins.InputColumns()) {
        cols.push_back(c);
      }
      std::vector<hyde::QueryColumn> all = cols;
      for (auto c : ins.AttachedColumns()) {
        all.push_back(c);
      }
      auto r = NewRule(VM(head));
      std::vector<hyde::QueryView> atom_views;
      if (auto p = SingleAtomOf(all, "INSERT")) {
        atom_views.push_back(*p);
        AddAtom(*r, *p, false);
      }
      for (auto c : cols) {
        r->head_tmpl.push_back(ResolveSlot(c, atom_views));
      }
      FinishRule(std::move(r));
    }

    // Relation SELECT (PUSH): one rule per INSERT into the relation,
    // forwarding the stored row positionally. (Stream SELECTs — receives —
    // have no rules: their rows are the explicit facts.)
    for (auto sel : query->Selects()) {
      if (!sel.IsRelation()) {
        continue;
      }
      const auto head = hyde::QueryView::From(sel);
      auto *head_vm = VM(head);
      for (auto insv : sel.Relation().Inserts()) {
        auto r = NewRule(head_vm);
        AddAtom(*r, insv, false);
        for (unsigned i = 0; i < head_vm->arity; ++i) {
          Slot s;
          s.atom = 0;
          s.col = i;
          r->head_tmpl.push_back(s);
        }
        FinishRule(std::move(r));
      }
    }
  }

  // Resolve an aggregate/KV input column to a Slot referencing a column
  // POSITION in `input` (or a constant). All non-constant inputs to one
  // aggregate come from its single summarized input view.
  Slot ResolveInputSlot(hyde::QueryColumn c, ViewModel *input) {
    Slot s;
    if (c.IsConstant()) {
      s.is_const = true;
      s.cval = ConstValue(c);
      return s;
    }
    const auto owner = hyde::QueryView::Containing(c);
    if (VM(owner) != input) {
      Fail("internal: aggregate input column's view is not the summarized "
           "input view");
    }
    s.atom = 0;
    s.col = *c.Index();
    return s;
  }

  // ------------------------------------------------------------------
  // Aggregate / KV-index setup (§4). Each becomes an AggInfo recomputed
  // definitionally from its single summarized input view.

  void BuildAggregates(void) {
    aggs_by_stratum.assign(num_strata, {});

    auto single_input = [&](const std::vector<hyde::QueryColumn> &cols,
                            const char *what) -> ViewModel * {
      auto v = SingleAtomOf(cols, what);
      if (!v) {
        Fail(std::string("internal: ") + what +
             " has no non-constant input view");
      }
      return VM(*v);
    };

    for (auto agg : query->Aggregates()) {
      const auto head = hyde::QueryView::From(agg);
      AggInfo ai;
      ai.head = VM(head);

      // Gather every input column to find the single summarized input view.
      std::vector<hyde::QueryColumn> all_ins;
      for (auto c : agg.InputGroupColumns()) {
        all_ins.push_back(c);
      }
      for (auto c : agg.InputConfigurationColumns()) {
        all_ins.push_back(c);
      }
      for (auto c : agg.InputAggregatedColumns()) {
        all_ins.push_back(c);
      }
      ai.input = single_input(all_ins, "AGGREGATE");

      // Output/key layout: group ++ config, then one summary column.
      for (auto c : agg.InputGroupColumns()) {
        ai.key_slots.push_back(ResolveInputSlot(c, ai.input));
      }
      for (auto c : agg.InputConfigurationColumns()) {
        ai.key_slots.push_back(ResolveInputSlot(c, ai.input));
      }
      if (agg.NumAggregateColumns() != 1 || agg.NumSummaryColumns() != 1) {
        Fail("oracle supports single-column sum_i32/count_i32 aggregates "
             "only (functor '" +
             std::string(agg.Functor().NameAsString()) + "')");
      }
      ai.val_slot = ResolveInputSlot(agg.NthInputAggregateColumn(0), ai.input);

      const std::string fname(agg.Functor().NameAsString());
      if (fname == "sum_i32") {
        ai.kind = AggKind::kSum;
      } else if (fname == "count_i32") {
        ai.kind = AggKind::kCount;
      } else {
        Fail("oracle implements only sum_i32/count_i32 aggregate functors "
             "by name (got '" +
             fname + "')");
      }
      ai.arity = static_cast<unsigned>(ai.key_slots.size()) + 1u;
      if (ai.arity != ai.head->arity) {
        Fail("internal: aggregate output arity mismatch for " + ai.head->name);
      }
      const auto idx = static_cast<unsigned>(aggs.size());
      aggs.push_back(std::move(ai));
      aggs_by_stratum[aggs.back().head->stratum].push_back(idx);
    }

    for (auto kv : query->KVIndices()) {
      const auto head = hyde::QueryView::From(kv);
      AggInfo ai;
      ai.head = VM(head);

      std::vector<hyde::QueryColumn> all_ins;
      for (auto c : kv.InputKeyColumns()) {
        all_ins.push_back(c);
      }
      for (auto c : kv.InputValueColumns()) {
        all_ins.push_back(c);
      }
      ai.input = single_input(all_ins, "KVINDEX");

      // Key ++ value layout. The KV index holds ONE value per key (last-writer
      // merge, §4 semantics); here it degenerates to the sole live member.
      for (auto c : kv.InputKeyColumns()) {
        ai.key_slots.push_back(ResolveInputSlot(c, ai.input));
      }
      if (kv.NumValueColumns() != 1) {
        Fail("oracle supports single-value KV indices only (view " +
             ai.head->name + ")");
      }
      ai.val_slot = ResolveInputSlot(kv.NthInputValueColumn(0), ai.input);
      ai.kind = AggKind::kMerge;
      ai.arity = static_cast<unsigned>(ai.key_slots.size()) + 1u;
      if (ai.arity != ai.head->arity) {
        Fail("internal: KV output arity mismatch for " + ai.head->name);
      }
      const auto idx = static_cast<unsigned>(aggs.size());
      aggs.push_back(std::move(ai));
      aggs_by_stratum[aggs.back().head->stratum].push_back(idx);
    }
  }

  // Definitional group reduction: over `member_rows` (the current rows of the
  // aggregate's input view), group by the key slots and run the by-name
  // reduction per group, emitting (key ++ summary) into `out`.
  template <typename EmitFn>
  void ReduceAggregate(const AggInfo &ai, const std::vector<Row> &member_rows,
                       EmitFn &&out) {
    auto slot_val = [](const Slot &s, const Row &row) -> Value {
      return s.is_const ? s.cval : row[s.col];
    };
    // group key -> accumulator (sum/count/last-value).
    std::unordered_map<Row, int64_t, RowHash> acc;
    std::vector<Row> key_order;
    for (const auto &row : member_rows) {
      Row key;
      key.reserve(ai.key_slots.size());
      for (const auto &s : ai.key_slots) {
        key.push_back(slot_val(s, row));
      }
      const auto v = static_cast<int64_t>(slot_val(ai.val_slot, row));
      auto it = acc.find(key);
      if (it == acc.end()) {
        int64_t init = 0;
        switch (ai.kind) {
          case AggKind::kSum: init = v; break;
          case AggKind::kCount: init = 1; break;
          case AggKind::kMerge: init = v; break;  // last-writer; see below
        }
        acc.emplace(key, init);
        key_order.push_back(key);
      } else {
        switch (ai.kind) {
          case AggKind::kSum: it->second += v; break;
          case AggKind::kCount: it->second += 1; break;
          case AggKind::kMerge: it->second = v; break;  // last member wins
        }
      }
    }
    for (const auto &key : key_order) {
      Row head = key;
      head.push_back(static_cast<Value>(acc[key]));
      out(head);
    }
  }

  // ------------------------------------------------------------------
  // Shared rule-instance enumerator. `delta_pos < 0` enumerates every
  // instance; otherwise the delta position is bound to `delta_row` (for a
  // negated delta position, the §5.4 crossover: the delta row constrains
  // the negation-key slots and the position's absence check is skipped).
  //
  // The recursion below descends over a rule's body positions — bounded by
  // the rule's atom count, never by data or derivation depth.

  template <typename Src, typename SinkFn>
  void EnumerateRule(const Rule &r, int delta_pos, const Row &delta_row,
                     const Src &src, SinkFn &&sink) {
    const unsigned n = static_cast<unsigned>(r.atoms.size());
    std::vector<const Row *> bound(n, nullptr);
    std::vector<std::optional<Value>> cls(r.class_const);
    std::vector<std::vector<std::optional<Value>>> forced(n);
    for (unsigned i = 0; i < n; ++i) {
      forced[i].assign(r.atoms[i].vm->arity, std::nullopt);
    }

    // Bind one positive atom's candidate row against the constraints;
    // records newly-assigned class ids in `undo` for backtracking.
    auto bind_row = [&](unsigned ai, const Row &row,
                        std::vector<unsigned> &undo) -> bool {
      const auto &per_col = r.classes_of[ai];
      for (unsigned c = 0; c < row.size(); ++c) {
        if (forced[ai][c] && *forced[ai][c] != row[c]) {
          return false;
        }
        if (c < per_col.size()) {
          for (auto cid : per_col[c]) {
            if (cls[cid]) {
              if (*cls[cid] != row[c]) {
                return false;
              }
            } else {
              cls[cid] = row[c];
              undo.push_back(cid);
            }
          }
        }
      }
      return true;
    };

    Row delta_copy;
    if (delta_pos >= 0) {
      const unsigned da = r.pos2atom[static_cast<unsigned>(delta_pos)];
      if (r.atoms[da].negated) {
        // Crossover: the delta row of the negated view fixes the key slots.
        for (unsigned i = 0; i < r.neg_key.size(); ++i) {
          const Slot &s = r.neg_key[i];
          const Value v = delta_row[i];
          if (s.is_const) {
            if (s.cval != v) {
              return;
            }
          } else {
            if (forced[s.atom][s.col] && *forced[s.atom][s.col] != v) {
              return;
            }
            forced[s.atom][s.col] = v;
          }
        }
      } else {
        std::vector<unsigned> undo;
        if (!bind_row(da, delta_row, undo)) {
          return;
        }
        delta_copy = delta_row;
        bound[da] = &delta_copy;
      }
    }

    Value map_result{0};  // the MAP functor's free result, set in `finish`.
    auto value_of = [&](const Slot &s) -> Value {
      if (s.is_const) {
        return s.cval;
      }
      if (s.is_map_result) {
        return map_result;
      }
      return (*bound[s.atom])[s.col];
    };

    auto finish = [&](void) {
      for (const auto &f : r.filters) {
        if (!EvalFilter(f, value_of(f.lhs), value_of(f.rhs))) {
          return;
        }
      }
      // MAP functor: apply the named functor to its bound args. A `nullopt`
      // result (@range failure, e.g. div-by-zero) emits zero output rows.
      if (r.map_fn != MapFn::kNone) {
        std::vector<Value> args;
        args.reserve(r.map_args.size());
        for (const auto &s : r.map_args) {
          args.push_back(value_of(s));
        }
        auto res = ApplyMapFn(r.map_fn, args);
        if (!res) {
          return;
        }
        map_result = *res;
      }
      for (unsigned p = 0; p < n; ++p) {
        const unsigned ai = r.pos2atom[p];
        if (!r.atoms[ai].negated || static_cast<int>(p) == delta_pos) {
          continue;
        }
        Row key;
        key.reserve(r.neg_key.size());
        for (const auto &s : r.neg_key) {
          key.push_back(value_of(s));
        }
        if (src.Contains(r.atoms[ai], p, key)) {
          return;  // Key present in the negated view: instance blocked.
        }
      }
      Row head;
      head.reserve(r.head_tmpl.size());
      for (const auto &s : r.head_tmpl) {
        head.push_back(value_of(s));
      }
      sink(head);
    };

    std::function<void(unsigned)> descend = [&](unsigned p) {
      if (p == n) {
        finish();
        return;
      }
      const unsigned ai = r.pos2atom[p];
      if (static_cast<int>(p) == delta_pos || r.atoms[ai].negated) {
        descend(p + 1);
        return;
      }
      // Determine key columns whose value is already pinned (forced or via
      // a bound equality class).
      std::vector<unsigned> key_cols;
      Row key_vals;
      for (unsigned c = 0; c < r.atoms[ai].vm->arity; ++c) {
        std::optional<Value> want = forced[ai][c];
        if (!want && c < r.classes_of[ai].size()) {
          for (auto cid : r.classes_of[ai][c]) {
            if (cls[cid]) {
              want = *cls[cid];
              break;
            }
          }
        }
        if (want) {
          key_cols.push_back(c);
          key_vals.push_back(*want);
        }
      }
      src.ForEach(r.atoms[ai], p, key_cols, key_vals, [&](const Row &row) {
        std::vector<unsigned> undo;
        if (bind_row(ai, row, undo)) {
          bound[ai] = &row;
          descend(p + 1);
          bound[ai] = nullptr;
        }
        for (auto cid : undo) {
          cls[cid] = std::nullopt;
        }
      });
    };

    descend(0);
  }

  // ------------------------------------------------------------------
  // Incremental counter folds (§3.1). Crossing predicates read only the
  // fold's own before/after snapshot plus the batch-frozen kInI bit.

  void Touch(ViewModel *vm, uint32_t id) {
    auto &s = vm->st[id];
    if (!s.touched) {
      s.touched = true;
      vm->touched.push_back(id);
    }
  }

  void CheckMagnitude(int64_t c) {
    if (c > INT32_MAX || c < INT32_MIN) {
      Fail("counter magnitude overflow (|C| > 2^31-1)");
    }
  }

  void AddDerivation(ViewModel *vm, const Row &row, hyde::DerivClass k) {
    const auto id = vm->Intern(row);
    auto &s = vm->st[id];
    Touch(vm, id);
    const int64_t before = s.c_nr + s.c_r;
    (k == hyde::DerivClass::kRecursive ? s.c_r : s.c_nr) += 1;
    CheckMagnitude(s.c_nr);
    CheckMagnitude(s.c_r);
    const int64_t after = s.c_nr + s.c_r;
    if (before <= 0 && after > 0) {
      vm->pend_add.push_back(id);
    }
  }

  void SubDerivation(ViewModel *vm, const Row &row, hyde::DerivClass k) {
    const auto id = vm->Intern(row);
    auto &s = vm->st[id];
    Touch(vm, id);
    (k == hyde::DerivClass::kRecursive ? s.c_r : s.c_nr) -= 1;
    CheckMagnitude(s.c_nr);
    CheckMagnitude(s.c_r);
    if (s.in_i && s.c_nr <= 0) {
      vm->pend_del.push_back(id);  // Duplicates absorbed by TryClaimDel.
    }
  }

  void AddExplicit(ViewModel *vm, const Row &row) {
    const auto id = vm->Intern(row);
    auto &s = vm->st[id];
    if (s.is_explicit) {
      return;
    }
    Touch(vm, id);
    s.is_explicit = true;
    const int64_t before = s.c_nr + s.c_r;
    s.c_nr += 1;
    if (before <= 0 && (s.c_nr + s.c_r) > 0) {
      vm->pend_add.push_back(id);
    }
  }

  void SubExplicit(ViewModel *vm, const Row &row) {
    const auto id = vm->Intern(row);
    auto &s = vm->st[id];
    if (!s.is_explicit) {
      return;
    }
    Touch(vm, id);
    s.is_explicit = false;
    s.c_nr -= 1;
    if (s.in_i && s.c_nr <= 0) {
      vm->pend_del.push_back(id);
    }
  }

  bool TryClaimDel(ViewModel *vm, uint32_t id) {
    auto &s = vm->st[id];
    if (s.del) {
      return false;
    }
    if (s.c_nr > 0) {
      Fail("TryClaimDel on a row with C_nr > 0 (view " + vm->name + ")");
    }
    s.del = true;
    s.del_now = true;
    Touch(vm, id);
    vm->claimed_del.push_back(id);
    return true;
  }

  bool TryClaimAdd(ViewModel *vm, uint32_t id) {
    auto &s = vm->st[id];
    if (s.add) {
      return false;
    }
    s.add = true;
    s.add_now = true;
    Touch(vm, id);
    vm->claimed_add.push_back(id);
    return true;
  }

  // ------------------------------------------------------------------
  // Read-state tables (§5.1).

  // Seed schema, delta at lower position `p`: lower j<p read InNew, lower
  // j>p read InI, same-stratum (always j>p) read InI. Non-delta reads do
  // not depend on the sign.
  std::vector<ReadK> SeedReads(const Rule &r, unsigned p) {
    // Every position before a lower delta position `p` is itself lower
    // (same-stratum positions sort after all lower ones), so the table
    // collapses to: q < p reads InNew, q > p (lower or same-stratum)
    // reads InI.
    const unsigned n = static_cast<unsigned>(r.atoms.size());
    std::vector<ReadK> reads(n, ReadK::kInI);
    for (unsigned q = 0; q < p; ++q) {
      reads[q] = ReadK::kInNew;
    }
    return reads;
  }

  // Fixpoint schema, delta at same-stratum position `p`: lower j<p read
  // InNew; same-stratum j<p read SurvivesSoFar (OVERDELETE) /
  // InNewWithFrontier (INSERT); same-stratum j>p read AliveAtClaim /
  // InNewSansFrontier.
  std::vector<ReadK> FixReads(const Rule &r, unsigned p, bool deleting) {
    const unsigned n = static_cast<unsigned>(r.atoms.size());
    std::vector<ReadK> reads(n, ReadK::kInNew);
    for (unsigned q = 0; q < n; ++q) {
      const bool same = r.atoms[r.pos2atom[q]].stratum == r.head->stratum;
      if (!same) {
        reads[q] = ReadK::kInNew;
      } else if (q < p) {
        reads[q] = deleting ? ReadK::kSurvivesSoFar : ReadK::kInNewWithFrontier;
      } else {
        reads[q] = deleting ? ReadK::kAliveAtClaim : ReadK::kInNewSansFrontier;
      }
    }
    return reads;
  }

  // ------------------------------------------------------------------
  // §5.2 OVERDELETE, REDERIVE; §5.3 INSERT; §5.0 BUILDFRONTIERS.

  void Overdelete(unsigned s) {
    // − seeds: delta over each lower predecessor's D\A frontier (negated
    // positions: over the negated view's A\D frontier).
    for (Rule *r : rules_by_stratum[s]) {
      for (unsigned p : r->lower_pos) {
        const auto &a = r->atoms[r->pos2atom[p]];
        const auto &frontier = a.negated ? a.vm->out_add : a.vm->out_del;
        if (frontier.empty()) {
          continue;
        }
        const auto reads = SeedReads(*r, p);
        IncSrc src{&reads};
        for (auto id : frontier) {
          const Row delta = a.vm->rows[id];
          EnumerateRule(*r, static_cast<int>(p), delta, src,
                        [&](const Row &h) {
                          SubDerivation(r->head, h, r->klass);
                        });
        }
      }
    }

    // Fixpoint over the stratum's claim rounds.
    for (;;) {
      std::vector<std::pair<ViewModel *, uint32_t>> round;
      for (auto *vm : views_by_stratum[s]) {
        for (auto id : vm->pend_del) {
          if (TryClaimDel(vm, id)) {
            round.emplace_back(vm, id);
          }
        }
        vm->pend_del.clear();
      }
      if (round.empty()) {
        break;
      }
      for (Rule *r : rules_by_stratum[s]) {
        for (unsigned p : r->same_pos) {
          const auto &a = r->atoms[r->pos2atom[p]];
          const auto reads = FixReads(*r, p, /*deleting=*/true);
          IncSrc src{&reads};
          for (const auto &[vm, id] : round) {
            if (vm != a.vm) {
              continue;
            }
            const Row delta = vm->rows[id];
            EnumerateRule(*r, static_cast<int>(p), delta, src,
                          [&](const Row &h) {
                            SubDerivation(r->head, h, r->klass);
                          });
          }
        }
      }
      for (const auto &[vm, id] : round) {  // RetireDel per drained row
        vm->st[id].del_now = false;
      }
    }
  }

  void Rederive(unsigned s) {
    // A counter read, not a search: C_r > 0 after OVERDELETE quiescence
    // witnesses a surviving recursive derivation; re-enter via INSERT.
    for (auto *vm : views_by_stratum[s]) {
      for (auto id : vm->claimed_del) {
        if (vm->st[id].c_r > 0) {
          vm->pend_add.push_back(id);
        }
      }
    }
  }

  void InsertPhase(unsigned s) {
    // + seeds: delta over each lower predecessor's A\D frontier (negated
    // positions: over the negated view's D\A frontier).
    for (Rule *r : rules_by_stratum[s]) {
      for (unsigned p : r->lower_pos) {
        const auto &a = r->atoms[r->pos2atom[p]];
        const auto &frontier = a.negated ? a.vm->out_del : a.vm->out_add;
        if (frontier.empty()) {
          continue;
        }
        const auto reads = SeedReads(*r, p);
        IncSrc src{&reads};
        for (auto id : frontier) {
          const Row delta = a.vm->rows[id];
          EnumerateRule(*r, static_cast<int>(p), delta, src,
                        [&](const Row &h) {
                          AddDerivation(r->head, h, r->klass);
                        });
        }
      }
    }

    for (;;) {
      std::vector<std::pair<ViewModel *, uint32_t>> round;
      for (auto *vm : views_by_stratum[s]) {
        for (auto id : vm->pend_add) {
          if (TryClaimAdd(vm, id)) {
            round.emplace_back(vm, id);
          }
        }
        vm->pend_add.clear();
      }
      if (round.empty()) {
        break;
      }
      for (Rule *r : rules_by_stratum[s]) {
        for (unsigned p : r->same_pos) {
          const auto &a = r->atoms[r->pos2atom[p]];
          const auto reads = FixReads(*r, p, /*deleting=*/false);
          IncSrc src{&reads};
          for (const auto &[vm, id] : round) {
            if (vm != a.vm) {
              continue;
            }
            const Row delta = vm->rows[id];
            EnumerateRule(*r, static_cast<int>(p), delta, src,
                          [&](const Row &h) {
                            AddDerivation(r->head, h, r->klass);
                          });
          }
        }
      }
      for (const auto &[vm, id] : round) {  // RetireAdd per drained row
        vm->st[id].add_now = false;
      }
    }
  }

  void BuildFrontiers(unsigned s) {
    for (auto *vm : views_by_stratum[s]) {
      for (auto id : vm->claimed_del) {
        if (!vm->st[id].add) {
          vm->out_del.push_back(id);  // D\A — net removals
        }
      }
      for (auto id : vm->claimed_add) {
        if (!vm->st[id].del) {
          vm->out_add.push_back(id);  // A\D — net additions
        }
      }
    }
  }

  // §5.5 Commit: the only externally visible barrier. Publishing is not
  // modeled; the oracle's contract is the final materialization plus the
  // per-batch assertions.
  void Commit(void) {
    for (auto &vmp : models) {
      auto *vm = vmp.get();
      if (!vm->pend_del.empty() || !vm->pend_add.empty()) {
        Fail("frontier queue not drained at commit (view " + vm->name + ")");
      }
      ++checks;
      for (auto id : vm->touched) {
        auto &s = vm->st[id];
        ++checks;
        if (s.c_nr < 0 || s.c_r < 0) {
          Fail("negative counter at commit: view " + vm->name + " row " +
               DumpRow(vm, vm->rows[id]) + " C_nr=" + std::to_string(s.c_nr) +
               " C_r=" + std::to_string(s.c_r));
        }
        s.in_i = (s.c_nr + s.c_r) > 0;
        s.del = s.add = s.del_now = s.add_now = false;
        s.touched = false;
      }
      vm->touched.clear();
      vm->claimed_del.clear();
      vm->claimed_add.clear();
      vm->out_del.clear();
      vm->out_add.clear();
    }
  }

  std::string DumpRow(const ViewModel *vm, const Row &r) {
    std::string out = "(";
    for (size_t i = 0; i < r.size(); ++i) {
      if (i) {
        out += ", ";
      }
      out += PrintValue(i < vm->types.size() ? vm->types[i]
                                             : hyde::TypeKind::kUnsigned64,
                        r[i]);
    }
    out += ")";
    return out;
  }

  // ------------------------------------------------------------------
  // Init rules: rules with no positive atoms fire once at program start
  // (all-constant tuples; negation-only bodies fire because every negated
  // view is empty at start).

  void FireInitRules(void) {
    std::vector<ReadK> no_reads;
    for (auto &rp : rules) {
      Rule &r = *rp;
      bool has_positive = false;
      for (const auto &a : r.atoms) {
        if (!a.negated) {
          has_positive = true;
        }
      }
      if (has_positive) {
        continue;
      }
      std::vector<ReadK> reads(r.atoms.size(), ReadK::kInI);
      IncSrc src{&reads};
      EnumerateRule(r, -1, {}, src, [&](const Row &h) {
        AddDerivation(r.head, h, r.klass);
      });
    }
  }

  // ------------------------------------------------------------------
  // One received message batch = one epoch (§5.0).

  struct BatchOp {
    unsigned msg{0};
    bool add{true};
    Row row;
  };

  void RunBatch(const std::vector<BatchOp> &ops) {
    ++batches_run;
    if (!init_fired) {
      init_fired = true;
      FireInitRules();
    }

    // Ingest netting (§5.0/§5.5, Open Question 3 — decided): SET semantics
    // with annihilation, mirroring the runtime's NetBatch. Each side is
    // deduplicated (assert/retract are idempotent; multiplicity within a
    // batch is meaningless) and a row appearing on BOTH sides annihilates —
    // its presence is unchanged. Deterministic; diverges from the old
    // compiler's dequeue-order-dependent outcome AND from arithmetic
    // netting ({+x, −x, −x} is a no-op here, not a removal).
    struct NetKey {
      unsigned msg;
      Row row;
      bool operator==(const NetKey &o) const {
        return msg == o.msg && row == o.row;
      }
    };
    struct NetKeyHash {
      size_t operator()(const NetKey &k) const {
        return RowHash{}(k.row) * 31 + k.msg;
      }
    };
    std::unordered_map<NetKey, unsigned, NetKeyHash> net;  // 1: add, 2: remove.
    std::vector<NetKey> order;
    for (const auto &op : ops) {
      NetKey k{op.msg, op.row};
      auto it = net.find(k);
      if (it == net.end()) {
        net.emplace(k, op.add ? 1u : 2u);
        order.push_back(k);
      } else {
        it->second |= op.add ? 1u : 2u;
      }
    }
    for (const auto &k : order) {
      const unsigned n = net[k];
      if (n == 3u) {
        continue;  // Same-batch add+remove annihilates at ingest.
      }
      for (auto *vm : msgs[k.msg].receives) {
        if (n == 1u) {
          AddExplicit(vm, k.row);
        } else {
          SubExplicit(vm, k.row);
        }
      }
    }

    // Strata in topological order; deletion and insertion interleave per
    // stratum (mandatory with negation, §5.4). A stratum with nothing
    // queued and empty incoming frontiers degenerates to the eager insert
    // path, so monotone strata run the same skeleton.
    for (unsigned s = 0; s < num_strata; ++s) {
      RecomputeAggregatesIncremental(s);
      Overdelete(s);
      Rederive(s);
      InsertPhase(s);
      BuildFrontiers(s);
    }

    Commit();
    CheckAgainstScratch();
  }

  // Incremental aggregate/KV recompute (§4, the definitional path): the same
  // group reduction as the from-scratch referee, computed each batch over the
  // input view's NET-NEW present rows (kInNew). We diff the desired agg row
  // set against the aggregate's batch-start presence (kInI) and emit ONE
  // NonRecursive +1 / -1 per crossing — exactly emit_touched's one-net-pair
  // (−old, +new), where a value change (Sum 30→40) is naturally two distinct
  // keyed rows (X,30) retracted and (X,40) asserted. Seeded BEFORE this
  // stratum's Overdelete/Insert so the ordinary claim/frontier/commit
  // machinery builds the aggregate's downstream frontiers. The aggregate is
  // always NonRecursive (its input is strictly-lower stratum, spec §5). The
  // per-view presence + counter cross-check (CheckAgainstScratch) then applies
  // to aggregate views for free, since CheckAgainstScratch mints the matching
  // expected (1 C_nr per present agg row).
  void RecomputeAggregatesIncremental(unsigned s) {
    for (unsigned ax : aggs_by_stratum[s]) {
      const AggInfo &ai = aggs[ax];

      // The input view's NET-NEW present rows (kInI && !del, or add).
      std::vector<Row> members;
      ViewModel *iv = ai.input;
      for (uint32_t id = 0; id < iv->rows.size(); ++id) {
        if (Pass(iv->st[id], ReadK::kInNew)) {
          members.push_back(iv->rows[id]);
        }
      }

      // Desired agg row set from the definitional reduction.
      std::unordered_map<Row, bool, RowHash> desired;
      std::vector<Row> desired_order;
      ReduceAggregate(ai, members, [&](const Row &h) {
        if (desired.emplace(h, true).second) {
          desired_order.push_back(h);
        }
      });

      ViewModel *hv = ai.head;

      // +1 for each desired row absent at batch start.
      for (const auto &h : desired_order) {
        auto it = hv->ids.find(h);
        const bool present = it != hv->ids.end() && hv->st[it->second].in_i;
        if (!present) {
          AddDerivation(hv, h, hyde::DerivClass::kNonRecursive);
        }
      }

      // -1 for each batch-start-present row no longer desired.
      for (uint32_t id = 0; id < hv->rows.size(); ++id) {
        if (hv->st[id].in_i && !desired.count(hv->rows[id])) {
          SubDerivation(hv, hv->rows[id], hyde::DerivClass::kNonRecursive);
        }
      }
    }
  }

  // ------------------------------------------------------------------
  // The independent from-scratch path: semi-naive stratified set
  // evaluation over the accumulated explicit facts, then rule-instance
  // counting over the final materialization.

  // Semi-naive stratified SET evaluation over the current explicit facts
  // (the `kExplicit` bits on the receive views), materializing every view's
  // present rows into `scratch`. This is the sole monotone evaluator: the
  // per-batch cross-check and the monotone projection both drive it, the
  // former over the accumulated net facts and the latter over the surviving
  // set fed as one epoch.
  void EvaluateScratch(void) {
    scratch.assign(models.size(), ScratchView());

    ScratchSrc src{&scratch};

    // Evaluate strata bottom-up.
    for (unsigned s = 0; s < num_strata; ++s) {
      std::vector<std::pair<ViewModel *, Row>> worklist;
      auto add = [&](ViewModel *vm, const Row &row) {
        if (scratch[vm->ord].Add(row)) {
          worklist.emplace_back(vm, row);
        }
      };

      // Explicit facts seed their receive views.
      for (auto *vm : views_by_stratum[s]) {
        if (!vm->is_receive) {
          continue;
        }
        for (uint32_t id = 0; id < vm->rows.size(); ++id) {
          if (vm->st[id].is_explicit) {
            add(vm, vm->rows[id]);
          }
        }
      }

      // Aggregate / KV-index views (§4): their single summarized input view is
      // in a strictly-lower stratum (aggregates are placed like negation), so
      // its scratch rows are already final. Recompute each aggregate's rows
      // definitionally — group the input rows and run the by-name reduction —
      // BEFORE any same-stratum rule that consumes the aggregate fires.
      for (unsigned ax : aggs_by_stratum[s]) {
        const AggInfo &ai = aggs[ax];
        ReduceAggregate(ai, scratch[ai.input->ord].rows,
                        [&](const Row &h) { add(ai.head, h); });
      }

      // Rules without same-stratum atoms fire once over final lower strata.
      for (Rule *r : rules_by_stratum[s]) {
        if (!r->same_pos.empty()) {
          continue;
        }
        EnumerateRule(*r, -1, {}, src,
                      [&](const Row &h) { add(r->head, h); });
      }

      // Same-stratum fixpoint: worklist-driven; non-delta positions read
      // the full current sets (complete because rows only accumulate).
      while (!worklist.empty()) {
        auto [vm, row] = std::move(worklist.back());
        worklist.pop_back();
        auto it = same_pos_users.find(vm);
        if (it == same_pos_users.end()) {
          continue;
        }
        for (auto [r, p] : it->second) {
          if (r->head->stratum != s) {
            continue;
          }
          EnumerateRule(*r, static_cast<int>(p), row, src,
                        [&](const Row &h) { add(r->head, h); });
        }
      }
    }
  }

  void CheckAgainstScratch(void) {
    EvaluateScratch();
    expected.assign(models.size(), {});

    ScratchSrc src{&scratch};

    // Count derivation instances of the final materialization.
    for (auto &rp : rules) {
      Rule &r = *rp;
      EnumerateRule(r, -1, {}, src, [&](const Row &h) {
        auto &e = expected[r.head->ord][h];
        if (r.klass == hyde::DerivClass::kRecursive) {
          e.second += 1;
        } else {
          e.first += 1;
        }
      });
    }
    for (auto &vmp : models) {
      auto *vm = vmp.get();
      if (!vm->is_receive) {
        continue;
      }
      for (uint32_t id = 0; id < vm->rows.size(); ++id) {
        if (vm->st[id].is_explicit) {
          expected[vm->ord][vm->rows[id]].first += 1;
        }
      }
    }

    // Aggregate / KV-index views have NO rules; their rows come from the
    // definitional group reduction (§4). Mint the matching expected counter:
    // exactly one NonRecursive derivation (C_nr=1) per present agg row — the
    // one-net-pair (−old,+new) leaves each present agg row with C_nr == 1.
    // This keeps the counter cross-check MEANINGFUL for agg views (presence
    // <=> C_nr==1) rather than relaxing it to presence-only. The from-scratch
    // reduction produced these same rows in `scratch`, so both paths agree.
    for (const auto &ai : aggs) {
      for (const auto &row : scratch[ai.head->ord].rows) {
        expected[ai.head->ord][row].first += 1;
      }
    }

    // Cross-assertions.
    for (auto &vmp : models) {
      auto *vm = vmp.get();
      const auto &exp = expected[vm->ord];
      const auto &sv = scratch[vm->ord];
      for (uint32_t id = 0; id < vm->rows.size(); ++id) {
        const auto &s = vm->st[id];
        const bool p_inc = s.in_i;
        const bool p_scr = sv.ids.count(vm->rows[id]) != 0;
        ++checks;
        if (p_inc != p_scr) {
          FailMismatch(vm, vm->rows[id], s, exp, "presence mismatch");
        }
        int64_t enr = 0, er = 0;
        if (auto it = exp.find(vm->rows[id]); it != exp.end()) {
          enr = it->second.first;
          er = it->second.second;
        }
        ++checks;
        if (s.c_nr != enr || s.c_r != er) {
          FailMismatch(vm, vm->rows[id], s, exp, "counter mismatch");
        }
      }
      for (const auto &row : sv.rows) {
        ++checks;
        if (!vm->ids.count(row)) {
          Fail("row derivable from scratch never materialized "
               "incrementally: view " +
               vm->name + " row " + DumpRow(vm, row));
        }
      }
    }
  }

  [[noreturn]] void FailMismatch(
      ViewModel *vm, const Row &row, const RowState &s,
      const std::unordered_map<Row, std::pair<int64_t, int64_t>, RowHash>
          &exp,
      const char *what) {
    int64_t enr = 0, er = 0;
    if (auto it = exp.find(row); it != exp.end()) {
      enr = it->second.first;
      er = it->second.second;
    }
    std::ostringstream os;
    os << what << " after batch " << batches_run << ": view " << vm->name
       << " row " << DumpRow(vm, row) << "\n  incremental: present="
       << (s.in_i ? 1 : 0) << " C_nr=" << s.c_nr << " C_r=" << s.c_r
       << "\n  from-scratch: present="
       << (scratch[vm->ord].ids.count(row) ? 1 : 0) << " C_nr=" << enr
       << " C_r=" << er;
    Fail(os.str());
  }

  // Number of named (non-condition) relations — the M in the INVARIANT line
  // and the relation count of the canonical dump.
  unsigned CountNamedRelations(void) {
    unsigned n = 0;
    for (auto rel : query->Relations()) {
      if (!rel.IsCondition()) {
        ++n;
      }
    }
    return n;
  }

  // Intern one explicit fact into every receive view of a message and mark
  // it present, WITHOUT touching any counter or frontier. Used only by the
  // monotone projection to seed the from-scratch evaluator with the
  // surviving-fact set as a single add-only epoch.
  void SeedExplicitOnly(unsigned msg, const Row &row) {
    for (auto *vm : msgs[msg].receives) {
      const auto id = vm->Intern(row);
      vm->st[id].is_explicit = true;
    }
  }

  // ------------------------------------------------------------------
  // Final output: every named (non-condition) relation's present rows in
  // canonical sorted form. `from_scratch` selects the row source: the
  // incremental materialization (per-row `kInI`) or the from-scratch
  // `scratch` set (all rows present) built by EvaluateScratch.

  std::string DumpRelations(bool from_scratch) {
    struct RelOut {
      std::string name;
      std::vector<hyde::TypeKind> types;
      std::vector<Row> rows;
    };
    std::vector<RelOut> rels;
    for (auto rel : query->Relations()) {
      if (rel.IsCondition()) {
        continue;  // Unit relations back zero-arity predicates; their token
                   // row is an implementation detail, not user output.
      }
      RelOut ro;
      ro.name = std::string(rel.Declaration().NameAsString());
      std::unordered_map<Row, bool, RowHash> seen;
      for (auto insv : rel.Inserts()) {
        auto it = by_view.find(insv.UniqueId());
        if (it == by_view.end()) {
          Fail("relation " + ro.name +
               " has an INSERT outside the interpreted universe");
        }
        auto *vm = it->second;
        if (ro.types.empty()) {
          ro.types = vm->types;
        }
        if (from_scratch) {
          for (const auto &row : scratch[vm->ord].rows) {
            if (!seen.count(row)) {
              seen.emplace(row, true);
              ro.rows.push_back(row);
            }
          }
        } else {
          for (uint32_t id = 0; id < vm->rows.size(); ++id) {
            if (vm->st[id].in_i && !seen.count(vm->rows[id])) {
              seen.emplace(vm->rows[id], true);
              ro.rows.push_back(vm->rows[id]);
            }
          }
        }
      }
      std::sort(ro.rows.begin(), ro.rows.end(),
                [&](const Row &a, const Row &b) {
                  return TypedRowLess(ro.types, a, b);
                });
      rels.push_back(std::move(ro));
    }
    std::sort(rels.begin(), rels.end(),
              [](const RelOut &a, const RelOut &b) { return a.name < b.name; });
    std::ostringstream os;
    for (const auto &ro : rels) {
      for (const auto &row : ro.rows) {
        os << ro.name << '\t';
        for (size_t i = 0; i < row.size(); ++i) {
          if (i) {
            os << ' ';
          }
          os << PrintValue(i < ro.types.size() ? ro.types[i]
                                               : hyde::TypeKind::kUnsigned64,
                           row[i]);
        }
        os << '\n';
      }
    }
    return os.str();
  }
};

// ---------------------------------------------------------------------
// .batches parsing (grammar at the top of this file).

std::vector<std::vector<Oracle::BatchOp>> ParseBatches(const char *path,
                                                       Oracle &oracle) {
  std::ifstream in(path);
  if (!in) {
    Fail(std::string("cannot open batches file '") + path + "'");
  }
  std::vector<std::vector<Oracle::BatchOp>> batches;
  std::optional<std::vector<Oracle::BatchOp>> cur;
  std::string line;
  unsigned lineno = 0;
  while (std::getline(in, line)) {
    ++lineno;
    if (auto pos = line.find('#'); pos != std::string::npos) {
      line.erase(pos);
    }
    std::istringstream ls(line);
    std::vector<std::string> toks;
    for (std::string t; ls >> t;) {
      toks.push_back(t);
    }
    if (toks.empty()) {
      continue;
    }
    auto err = [&](const std::string &m) {
      Fail(std::string(path) + ":" + std::to_string(lineno) + ": " + m);
    };
    if (toks[0] == "batch") {
      if (cur) {
        err("nested 'batch'");
      }
      cur.emplace();
    } else if (toks[0] == "end") {
      if (!cur) {
        err("'end' outside a batch");
      }
      batches.push_back(std::move(*cur));
      cur.reset();
    } else if (toks[0] == "+" || toks[0] == "-") {
      if (!cur) {
        err("op outside a batch");
      }
      if (toks.size() < 2) {
        err("op without a message name");
      }
      const auto arity = static_cast<unsigned>(toks.size() - 2);
      auto it = oracle.msg_by_name_arity.find({toks[1], arity});
      if (it == oracle.msg_by_name_arity.end()) {
        err("no received message '" + toks[1] + "' of arity " +
            std::to_string(arity));
      }
      const auto &mi = oracle.msgs[it->second];
      Oracle::BatchOp op;
      op.msg = it->second;
      op.add = toks[0] == "+";
      if (!op.add && !mi.differential) {
        err("'-' on non-@differential message '" + mi.name + "'");
      }
      for (unsigned i = 0; i < arity; ++i) {
        auto v = ParseValue(mi.types[i], toks[2 + i]);
        if (!v) {
          err("value '" + toks[2 + i] + "' does not fit column " +
              std::to_string(i) + " of message '" + mi.name + "'");
        }
        op.row.push_back(*v);
      }
      cur->push_back(std::move(op));
    } else {
      err("unrecognized directive '" + toks[0] + "'");
    }
  }
  if (cur) {
    Fail(std::string(path) + ": unterminated batch at end of file");
  }
  return batches;
}

// ---------------------------------------------------------------------
// Stress mode: seeded pseudo-random mixed add/remove batches over the
// received message space, values from a small domain. splitmix64 only —
// never time.

struct SplitMix64 {
  uint64_t s;
  uint64_t Next(void) {
    s += 0x9E3779B97F4A7C15ull;
    uint64_t z = s;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }
};

int RunStress(Oracle &oracle, uint64_t seed, uint64_t rounds) {
  if (oracle.msgs.empty()) {
    Fail("stress mode: module receives no messages");
  }
  SplitMix64 rng{seed};
  for (uint64_t r = 0; r < rounds; ++r) {
    std::vector<Oracle::BatchOp> ops;
    const uint64_t nops = 1 + (rng.Next() % 6);
    for (uint64_t i = 0; i < nops; ++i) {
      const auto &mi =
          oracle.msgs[static_cast<size_t>(rng.Next() % oracle.msgs.size())];
      Oracle::BatchOp op;
      op.msg = static_cast<unsigned>(&mi - oracle.msgs.data());
      op.add = mi.differential ? (rng.Next() % 3 != 0) : true;
      for (auto k : mi.types) {
        if (k == hyde::TypeKind::kBoolean) {
          op.row.push_back(rng.Next() % 2);
        } else if (IsSignedKind(k) && BitWidth(k) >= 8) {
          op.row.push_back(
              static_cast<Value>(static_cast<int64_t>(rng.Next() % 8) - 3));
        } else {
          op.row.push_back(rng.Next() % 8);
        }
      }
      ops.push_back(std::move(op));
    }
    oracle.RunBatch(ops);
  }

  const std::string dump = oracle.DumpRelations(false);
  SplitMix64 h{0x5EEDF00Dull};
  uint64_t digest = 0;
  for (char ch : dump) {
    h.s ^= static_cast<unsigned char>(ch);
    digest ^= h.Next();
  }
  std::cout << "ORACLE: OK (" << oracle.batches_run << " batches, "
            << oracle.checks << " assertions)\n";
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%016llx",
                static_cast<unsigned long long>(digest));
  std::cout << "STRESS: seed=" << seed << " rounds=" << rounds
            << " digest=" << buf << "\n";
  std::cout << dump;
  return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------
// Monotone projection: evaluate the program as if nothing had ever been
// removed — over exactly the explicit facts that SURVIVE the whole batch
// sequence. No counters, no removal, no differential machinery: this is
// the ground-truth final materialization the differential run must match.
//
// A fact survives iff its net add/remove state across the sequence is
// present. Netting is per batch (matching §5.0 ingest netting) over a
// set-semantics presence bit: within a batch each side is deduplicated and
// a row appearing on BOTH sides annihilates (presence unchanged); a row on
// the add side only becomes present, on the remove side only absent. So an
// add paired with a remove in one batch does NOT change the fact's state,
// regardless of duplicate-op multiplicity. This is exactly the state the
// incremental path's `kExplicit` bit reaches, which is why the projection
// equals the differential final materialization.

int RunMonotoneProjection(
    Oracle &oracle, const std::vector<std::vector<Oracle::BatchOp>> &batches) {
  struct NetKey {
    unsigned msg;
    Row row;
    bool operator==(const NetKey &o) const {
      return msg == o.msg && row == o.row;
    }
  };
  struct NetKeyHash {
    size_t operator()(const NetKey &k) const {
      return RowHash{}(k.row) * 31 + k.msg;
    }
  };

  std::unordered_map<NetKey, bool, NetKeyHash> present;
  std::vector<NetKey> present_order;  // deterministic surviving-set walk
  for (const auto &ops : batches) {
    std::unordered_map<NetKey, unsigned, NetKeyHash> net;  // 1: add, 2: remove.
    std::vector<NetKey> order;
    for (const auto &op : ops) {
      NetKey k{op.msg, op.row};
      auto it = net.find(k);
      if (it == net.end()) {
        net.emplace(k, op.add ? 1u : 2u);
        order.push_back(k);
      } else {
        it->second |= op.add ? 1u : 2u;
      }
    }
    for (const auto &k : order) {
      const unsigned n = net[k];
      if (n == 3u) {
        continue;  // Annihilated within the batch: presence unchanged.
      }
      auto [it, inserted] = present.emplace(k, n == 1u);
      if (inserted) {
        present_order.push_back(k);
      } else {
        it->second = (n == 1u);
      }
    }
  }

  size_t n_surviving = 0;
  for (const auto &k : present_order) {
    if (present[k]) {
      ++n_surviving;
      oracle.SeedExplicitOnly(k.msg, k.row);
    }
  }

  oracle.EvaluateScratch();
  std::cout << "MONOTONE-PROJECTION: " << n_surviving << " surviving facts\n";
  std::cout << oracle.DumpRelations(/*from_scratch=*/true);
  return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    std::cerr << "USAGE: " << argv[0]
              << " <case.dr> (<case.batches> | --stress <seed> <rounds>)"
              << std::endl;
    return EXIT_FAILURE;
  }

  Oracle oracle;
  if (auto rc = oracle.Load(argv[1]); rc != EXIT_SUCCESS) {
    return rc;
  }

  if (!std::strcmp(argv[2], "--stress")) {
    if (argc != 5) {
      std::cerr << "USAGE: " << argv[0] << " <case.dr> --stress <seed> <rounds>"
                << std::endl;
      return EXIT_FAILURE;
    }
    const uint64_t seed = std::strtoull(argv[3], nullptr, 0);
    const uint64_t rounds = std::strtoull(argv[4], nullptr, 0);
    return RunStress(oracle, seed, rounds);
  }

  const auto batches = ParseBatches(argv[2], oracle);

  bool monotone = false;
  if (argc >= 4) {
    if (!std::strcmp(argv[3], "--project-monotone")) {
      monotone = true;
    } else {
      std::cerr << "USAGE: " << argv[0]
                << " <case.dr> <case.batches> [--project-monotone]"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (monotone) {
    return RunMonotoneProjection(oracle, batches);
  }

  for (const auto &b : batches) {
    oracle.RunBatch(b);
  }
  std::cout << "ORACLE: OK (" << oracle.batches_run << " batches, "
            << oracle.checks << " assertions)\n";
  std::cout << oracle.DumpRelations(/*from_scratch=*/false);

  // The per-batch cross-assertions proved incremental presence equals the
  // from-scratch (monotone) materialization for every view; since the
  // from-scratch path evaluates over exactly the surviving explicit facts,
  // reaching here means the differential final state equals the monotone
  // projection. Report it on stderr as the positive counterpart to the
  // FailMismatch dump (which names the first divergent view on failure).
  std::cerr << "INVARIANT: differential-final == monotone-projection ("
            << oracle.CountNamedRelations() << " relations)\n";
  return EXIT_SUCCESS;
}
