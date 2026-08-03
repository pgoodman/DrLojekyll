// Copyright 2026, Peter Goodman. All rights reserved.

// `drlojekyll-refinterp`: the I0 reference relational interpreter — a
// definitional evaluator over RAW PARSED CLAUSES (OG1-parsed: no
// Query::Build, no Rel, no codegen), emitting the Canonical Behavioral
// Format (CBF). See docs/proposals/RegionalDataFlowCore.artifacts/
// stage-i0-interpreter.md (H2/H3 + the §7 dated amendments) and the
// semantic authority bin/Oracle/Main.cpp (functor registry, PrintValue,
// aggregate/KV definitional recompute, OQ3 NetBatch, stratified negation).
//
// This tool deliberately links Display/Lex/Parse/Util ONLY. It evaluates
// the parsed AST directly: it stratifies the predicate-dependency graph
// definitionally, recomputes the whole materialization FROM SCRATCH per
// `.batches` epoch under set semantics, and prints the CBF transcript —
// per-epoch sorted published-message deltas, final sorted membership of
// every published (transmit) message, and per-`#query` sorted answers
// (all-free enumeration by declaration order; bound queries probe-restricted
// via a `.probes` sidecar).
//
// CLI:
//   drlojekyll-refinterp <case.dr> <case.batches> [<case.probes>]
//     -> CBF on stdout, exit 0.
//   Definitional-envelope rejects (unknown functor, float/foreign columns,
//   multi-summary aggregates, algebra-less KV merge, unstratified negation)
//   print one "REFINTERP-REJECT: <reason>" line to stderr and exit 2.
//
// The demand flag surface does not exist here: `.drflags` is never read, and
// a demanded program's CBF equals the plain program's CBF by construction.

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
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

[[noreturn]] void Reject(const std::string &msg) {
  std::cerr << "REFINTERP-REJECT: " << msg << "\n";
  std::exit(2);
}

// ---------------------------------------------------------------------
// Value lexing / rendering — MUST mirror bin/Oracle/Main.cpp lexeme-for-
// lexeme so cross-ties compare byte-wise.

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

bool IsInterpretableKind(hyde::TypeKind k) {
  return k == hyde::TypeKind::kBoolean || BitWidth(k) != 0;
}

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

// ---------------------------------------------------------------------
// The compiled program.

// A relation is any declaration that either receives base facts (a received
// #message) or is derived from clauses (#local/#export/#query/published
// #message + the synthetic aggregate over-block locals).
struct Relation {
  uint64_t id{0};
  std::string name;
  unsigned arity{0};
  std::vector<hyde::TypeKind> types;

  bool is_base{false};       // received message: rows come from batches
  bool is_published{false};  // transmit message: appears in FINAL / deltas

  bool is_kv{false};
  unsigned kv_value_col{0};
  std::vector<unsigned> kv_key_cols;
  int kv_kind{0};  // 0 = additive @invertible sum, 1 = @recompute merge

  std::vector<Row> rows;  // insertion-ordered, deterministic
  std::unordered_set<Row, RowHash> set;

  // Lazy append-only join indexes keyed by a sorted column list; `first` is
  // the count of rows already folded in (rows only append between Clears).
  std::map<std::vector<unsigned>,
           std::pair<size_t, std::unordered_map<Row, std::vector<uint32_t>,
                                                RowHash>>>
      indexes;

  bool Add(const Row &r) {
    if (set.insert(r).second) {
      rows.push_back(r);
      return true;
    }
    return false;
  }
  bool Contains(const Row &r) const { return set.count(r) != 0; }
  void Clear() {
    rows.clear();
    set.clear();
    indexes.clear();
  }

  const std::unordered_map<Row, std::vector<uint32_t>, RowHash> &GetIndex(
      const std::vector<unsigned> &cols) {
    auto &e = indexes[cols];
    for (; e.first < rows.size(); ++e.first) {
      Row k;
      k.reserve(cols.size());
      for (auto c : cols) {
        k.push_back(rows[e.first][c]);
      }
      e.second[k].push_back(static_cast<uint32_t>(e.first));
    }
    return e.second;
  }
};

// A MAP/filter functor call in a clause body (e.g. div_i32, add_i32).
enum class MapFn : uint8_t { kDivI32, kAddI32 };

struct FunctorCall {
  MapFn fn;
  std::vector<uint64_t> in_vars;  // bound-param argument variables, in order
  uint64_t out_var{0};            // the single free-param result variable
};

struct Cmp {
  hyde::ComparisonOperator op;
  uint64_t lhs{0}, rhs{0};
  bool sgn{false};
};

enum class AggKind : uint8_t { kSum, kCount, kSumAbove, kMaxAbove };

struct AggGoal {
  uint64_t input_rel{0};
  std::vector<std::pair<uint64_t, unsigned>> group_binds;   // (var, input col)
  std::vector<std::pair<uint64_t, unsigned>> config_binds;  // (var, input col)
  unsigned agg_col{0};
  uint64_t summary_var{0};
  AggKind kind{AggKind::kSum};
};

struct Atom {
  uint64_t rel{0};
  std::vector<uint64_t> args;  // argument variable ids, in column order
};

struct CompiledClause {
  uint64_t head{0};
  std::vector<uint64_t> head_vars;
  std::vector<std::pair<uint64_t, Value>> assigns;
  std::vector<Atom> pos;
  std::vector<Atom> neg;
  std::vector<FunctorCall> functors;
  std::vector<Cmp> cmps;
  std::vector<AggGoal> aggs;
  std::vector<unsigned> rec_pos;  // pos[] indices whose rel is same-stratum
  bool disabled{false};
};

struct MsgInfo {
  std::string name;
  unsigned arity{0};
  bool differential{false};
  std::vector<hyde::TypeKind> types;
  uint64_t rel{0};
};

// ---------------------------------------------------------------------

class Interp {
 public:
  hyde::DisplayManager dm;
  hyde::ErrorLog err{dm};
  std::optional<hyde::ParsedModule> module;

  std::unordered_map<uint64_t, Relation> rels;
  std::vector<uint64_t> rel_order;  // deterministic declaration order
  std::unordered_map<uint64_t, std::vector<CompiledClause>> clauses;

  // Received-message ingest surface, keyed by (name, arity).
  std::vector<MsgInfo> msgs;
  std::map<std::pair<std::string, unsigned>, unsigned> msg_index;

  // Stratification: a deterministic evaluation order over SCCs.
  std::vector<std::vector<uint64_t>> strata;  // each is a set of relation ids
  std::unordered_map<uint64_t, int> rel_stratum;

  // ------------------------------------------------------------------

  static uint64_t CanonicalId(hyde::ParsedDeclaration d) {
    for (auto r : d.Redeclarations()) {
      if (r.IsFirstDeclaration()) {
        return r.Id();
      }
    }
    return d.Id();
  }

  // Zero-arity conditions (`foo : enable_feature(1).`) all parse to a SHARED
  // anonymous (empty-named) declaration, so `CanonicalId` alone conflates
  // distinct conditions (`foo` and `bar` collapse to one relation). Key such
  // relations by the condition NAME token instead. Real declaration ids sit
  // near UINT64_MAX (the `~`-flip in ParsedDeclarationImpl::Id), so tagging
  // the small identifier id with `kCondBit` cannot collide with them.
  static constexpr uint64_t kCondBit = 1ull << 62;
  static uint64_t RelId(hyde::ParsedDeclaration d, hyde::Token name) {
    if (d.Arity() == 0u && d.NameAsString().empty() && name.IsValid()) {
      return kCondBit | static_cast<uint64_t>(name.IdentifierId());
    }
    return CanonicalId(d);
  }

  Relation &EnsureRel(hyde::ParsedDeclaration d, bool base,
                      hyde::Token cond_name = hyde::Token()) {
    const uint64_t id = RelId(d, cond_name);
    auto it = rels.find(id);
    if (it != rels.end()) {
      it->second.is_base = it->second.is_base || base;
      return it->second;
    }
    Relation r;
    r.id = id;
    r.name = std::string(d.NameAsString());
    r.arity = d.Arity();
    r.is_base = base;
    for (auto p : d.Parameters()) {
      const auto k = p.Type().UnderlyingKind();
      if (!IsInterpretableKind(k)) {
        Reject("unsupported column type (float/foreign) in relation '" +
               r.name + "'");
      }
      r.types.push_back(k);
    }
    // Key/value shape for a `mutable(...)` KV index.
    if (d.HasMutableParameter()) {
      int nval = 0;
      for (auto p : d.Parameters()) {
        if (p.Binding() == hyde::ParameterBinding::kMutable) {
          r.is_kv = true;
          r.kv_value_col = p.Index();
          ++nval;
          auto merge = hyde::ParsedFunctor::MergeOperatorOf(p);
          const std::string mn(merge.NameAsString());
          if (merge.IsInvertible()) {
            if (mn == "add_u32" || mn == "add_i32" || mn == "sum_i32") {
              r.kv_kind = 0;  // additive fold == SUM of live distinct values
            } else {
              Reject("KV @invertible merge functor '" + mn +
                     "' not modeled (only additive add_u32/add_i32/sum_i32)");
            }
          } else if (merge.IsRecompute()) {
            r.kv_kind = 1;  // per-group rescan of the live multiset
          } else {
            Reject("KV merge functor '" + mn +
                   "' declares no @-algebra (@invertible/@recompute)");
          }
        } else {
          r.kv_key_cols.push_back(p.Index());
        }
      }
      if (nval != 1) {
        Reject("multi-value KV index '" + r.name + "' not supported");
      }
    }
    rels.emplace(id, std::move(r));
    rel_order.push_back(id);
    return rels.at(id);
  }

  Value LiteralValue(hyde::ParsedVariable lhs, hyde::ParsedLiteral rhs) {
    if (rhs.IsConstant() || rhs.IsEnumerator() || rhs.IsString()) {
      Reject("unsupported constant literal (foreign/enum/string)");
    }
    const auto k = lhs.Type().UnderlyingKind();
    if (!IsInterpretableKind(k)) {
      Reject("unsupported constant column type (float/foreign)");
    }
    auto sp = rhs.Spelling(hyde::Language::kCxx);
    if (!sp) {
      Reject("constant literal has no spelling");
    }
    auto v = ParseValue(k, std::string(*sp));
    if (!v) {
      Reject("cannot parse constant literal '" + std::string(*sp) + "'");
    }
    return *v;
  }

  MapFn ResolveMapFn(const std::string &name, unsigned num_bound) {
    if (name == "div_i32" && num_bound == 2) {
      return MapFn::kDivI32;
    }
    if (name == "add_i32" && num_bound == 2) {
      return MapFn::kAddI32;
    }
    Reject("unimplemented MAP functor '" + name + "' (bound arity " +
           std::to_string(num_bound) + ")");
  }

  // Apply a MAP functor; std::nullopt suppresses the instance (a @range
  // functor emitting zero tuples, e.g. div-by-zero).
  static std::optional<Value> ApplyMapFn(MapFn fn,
                                         const std::vector<Value> &a) {
    switch (fn) {
      case MapFn::kDivI32: {
        const auto rhs = static_cast<int32_t>(static_cast<int64_t>(a[1]));
        if (rhs == 0) {
          return std::nullopt;
        }
        const auto lhs = static_cast<int32_t>(static_cast<int64_t>(a[0]));
        return static_cast<Value>(static_cast<int64_t>(lhs / rhs));
      }
      case MapFn::kAddI32: {
        const auto lhs = static_cast<int32_t>(static_cast<int64_t>(a[0]));
        const auto rhs = static_cast<int32_t>(static_cast<int64_t>(a[1]));
        const auto res = static_cast<int32_t>(static_cast<uint32_t>(lhs) +
                                              static_cast<uint32_t>(rhs));
        return static_cast<Value>(static_cast<int64_t>(res));
      }
    }
    return std::nullopt;
  }

  // ------------------------------------------------------------------
  // Compile one parsed clause into a CompiledClause.

  CompiledClause Compile(hyde::ParsedClause pc) {
    CompiledClause c;
    c.head = RelId(hyde::ParsedDeclaration::Of(pc), pc.Name());
    for (auto p : pc.Parameters()) {
      c.head_vars.push_back(p.Id());
    }
    if (pc.IsDisabled()) {
      c.disabled = true;
      return c;
    }

    const unsigned ng = pc.NumGroups();
    for (unsigned g = 0; g < ng; ++g) {
      for (auto a : pc.Assignments(g)) {
        c.assigns.emplace_back(a.LHS().Id(), LiteralValue(a.LHS(), a.RHS()));
      }

      for (auto pred : pc.PositivePredicates(g)) {
        auto decl = hyde::ParsedDeclaration::Of(pred);
        if (decl.IsFunctor()) {
          auto f = hyde::ParsedFunctor::From(decl);
          FunctorCall fc;
          std::vector<uint64_t> in_vars;
          std::optional<uint64_t> out_var;
          unsigned i = 0;
          for (auto arg : pred.Arguments()) {
            const auto b = f.NthParameter(i).Binding();
            if (b == hyde::ParameterBinding::kFree) {
              if (out_var) {
                Reject("MAP functor '" + std::string(f.NameAsString()) +
                       "' with multiple free results not supported");
              }
              out_var = arg.Id();
            } else {
              in_vars.push_back(arg.Id());
            }
            ++i;
          }
          if (!out_var) {
            Reject("functor '" + std::string(f.NameAsString()) +
                   "' used with no free result is not supported");
          }
          fc.fn = ResolveMapFn(std::string(f.NameAsString()),
                               static_cast<unsigned>(in_vars.size()));
          fc.in_vars = std::move(in_vars);
          fc.out_var = *out_var;
          c.functors.push_back(std::move(fc));
        } else {
          Atom at;
          at.rel = RelId(decl, pred.Name());
          for (auto arg : pred.Arguments()) {
            at.args.push_back(arg.Id());
          }
          c.pos.push_back(std::move(at));
        }
      }

      for (auto pred : pc.NegatedPredicates(g)) {
        Atom at;
        at.rel = RelId(hyde::ParsedDeclaration::Of(pred), pred.Name());
        for (auto arg : pred.Arguments()) {
          at.args.push_back(arg.Id());
        }
        c.neg.push_back(std::move(at));
      }

      for (auto cmp : pc.Comparisons(g)) {
        Cmp k;
        k.op = cmp.Operator();
        k.lhs = cmp.LHS().Id();
        k.rhs = cmp.RHS().Id();
        k.sgn = IsSignedKind(cmp.LHS().Type().UnderlyingKind());
        c.cmps.push_back(k);
      }

      for (auto agg : pc.Aggregates(g)) {
        c.aggs.push_back(CompileAgg(agg));
      }
    }
    return c;
  }

  AggGoal CompileAgg(hyde::ParsedAggregate agg) {
    AggGoal a;
    auto pred = agg.Predicate();
    a.input_rel = CanonicalId(hyde::ParsedDeclaration::Of(pred));

    // Classify each predicate argument (== an input-relation column) as
    // group / config / aggregated by matching variable identity.
    std::unordered_set<uint64_t> group_ids, config_ids, agg_ids;
    for (auto v : agg.GroupVariablesFromPredicate()) {
      group_ids.insert(v.Id());
    }
    for (auto v : agg.ConfigurationVariablesFromPredicate()) {
      config_ids.insert(v.Id());
    }
    for (auto v : agg.AggregatedVariablesFromPredicate()) {
      agg_ids.insert(v.Id());
    }
    unsigned col = 0;
    int agg_col = -1;
    for (auto arg : pred.Arguments()) {
      const uint64_t vid = arg.Id();
      if (group_ids.count(vid)) {
        a.group_binds.emplace_back(vid, col);
      } else if (config_ids.count(vid)) {
        a.config_binds.emplace_back(vid, col);
      } else if (agg_ids.count(vid)) {
        agg_col = static_cast<int>(col);
      } else {
        // A non-classified argument (e.g. an anonymous var) groups too.
        a.group_binds.emplace_back(vid, col);
      }
      ++col;
    }
    if (agg_col < 0) {
      Reject("aggregate has no aggregated column");
    }
    a.agg_col = static_cast<unsigned>(agg_col);

    auto f = hyde::ParsedFunctor::From(hyde::ParsedDeclaration::Of(agg.Functor()));
    // Locate the single summary variable via the functor's summary param.
    unsigned nsummary = 0, naggparam = 0, i = 0;
    std::optional<uint64_t> summary;
    for (auto arg : agg.Functor().Arguments()) {
      const auto b = f.NthParameter(i).Binding();
      if (b == hyde::ParameterBinding::kSummary) {
        summary = arg.Id();
        ++nsummary;
      } else if (b == hyde::ParameterBinding::kAggregate) {
        ++naggparam;
      }
      ++i;
    }
    if (nsummary != 1 || naggparam != 1) {
      Reject("multi-summary/multi-value aggregate functor '" +
             std::string(f.NameAsString()) + "' not supported");
    }
    a.summary_var = *summary;

    const std::string fn(f.NameAsString());
    if (fn == "sum_i32") {
      a.kind = AggKind::kSum;
    } else if (fn == "count_i32") {
      a.kind = AggKind::kCount;
    } else if (fn == "sum_above") {
      a.kind = AggKind::kSumAbove;
    } else if (fn == "max_above") {
      a.kind = AggKind::kMaxAbove;
    } else {
      Reject("unimplemented aggregate functor '" + fn + "'");
    }
    if ((a.kind == AggKind::kSumAbove || a.kind == AggKind::kMaxAbove) &&
        a.config_binds.size() != 1) {
      Reject("config-dependent aggregate '" + fn +
             "' expects exactly one config (threshold) column");
    }
    return a;
  }

  // ------------------------------------------------------------------
  // Definitional group reduction for one aggregate over its input relation's
  // current rows. Returns rows shaped (group vals ++ config vals ++ summary).

  std::vector<Row> ComputeAgg(const AggGoal &a) {
    const Relation &in = rels.at(a.input_rel);
    const unsigned ng = static_cast<unsigned>(a.group_binds.size());
    const unsigned nc = static_cast<unsigned>(a.config_binds.size());

    std::map<Row, int64_t> acc;
    std::vector<Row> key_order;
    std::map<Row, bool> gate_seen;  // kMaxAbove group registry

    for (const auto &row : in.rows) {
      Row key;
      key.reserve(ng + nc);
      for (const auto &gb : a.group_binds) {
        key.push_back(row[gb.second]);
      }
      for (const auto &cb : a.config_binds) {
        key.push_back(row[cb.second]);
      }
      const auto v = static_cast<int64_t>(row[a.agg_col]);
      const bool config_kind =
          (a.kind == AggKind::kSumAbove || a.kind == AggKind::kMaxAbove);
      const bool above =
          !config_kind ||
          (v >= static_cast<int64_t>(row[a.config_binds[0].second]));

      if (a.kind == AggKind::kMaxAbove) {
        const bool seen = gate_seen.count(key) != 0;
        if (!seen) {
          key_order.push_back(key);
          gate_seen[key] = above;
          if (above) {
            acc[key] = v;
          }
        } else if (above) {
          if (gate_seen[key]) {
            acc[key] = std::max<int64_t>(acc[key], v);
          } else {
            acc[key] = v;
            gate_seen[key] = true;
          }
        }
        continue;
      }

      auto it = acc.find(key);
      if (it == acc.end()) {
        switch (a.kind) {
          case AggKind::kSum: acc.emplace(key, v); break;
          case AggKind::kCount: acc.emplace(key, 1); break;
          case AggKind::kSumAbove: acc.emplace(key, above ? v : 0); break;
          case AggKind::kMaxAbove: break;
        }
        key_order.push_back(key);
      } else {
        switch (a.kind) {
          case AggKind::kSum: it->second += v; break;
          case AggKind::kCount: it->second += 1; break;
          case AggKind::kSumAbove: if (above) { it->second += v; } break;
          case AggKind::kMaxAbove: break;
        }
      }
    }

    std::vector<Row> out;
    for (const auto &key : key_order) {
      if (a.kind == AggKind::kMaxAbove && !gate_seen[key]) {
        continue;  // no gate-passing member -> group suppressed.
      }
      Row r = key;
      r.push_back(static_cast<Value>(acc[key]));
      out.push_back(std::move(r));
    }
    return out;
  }

  // ------------------------------------------------------------------
  // Clause solver: enumerate satisfying variable bindings and emit head rows.

  using Binding = std::unordered_map<uint64_t, Value>;

  // Apply every currently-ready deferred constraint (equalities, MAP
  // functors, comparisons, negations). Returns false on contradiction.
  bool Propagate(const CompiledClause &c, Binding &b) {
    bool changed = true;
    while (changed) {
      changed = false;

      for (const auto &cmp : c.cmps) {
        if (cmp.op != hyde::ComparisonOperator::kEqual) {
          continue;
        }
        auto l = b.find(cmp.lhs), r = b.find(cmp.rhs);
        if (l != b.end() && r != b.end()) {
          if (l->second != r->second) {
            return false;
          }
        } else if (l != b.end()) {
          b[cmp.rhs] = l->second;
          changed = true;
        } else if (r != b.end()) {
          b[cmp.lhs] = r->second;
          changed = true;
        }
      }

      for (const auto &fc : c.functors) {
        bool ready = true;
        std::vector<Value> args;
        for (auto v : fc.in_vars) {
          auto it = b.find(v);
          if (it == b.end()) {
            ready = false;
            break;
          }
          args.push_back(it->second);
        }
        if (!ready) {
          continue;
        }
        auto res = ApplyMapFn(fc.fn, args);
        if (!res) {
          return false;  // @range functor produced no tuple.
        }
        auto it = b.find(fc.out_var);
        if (it == b.end()) {
          b[fc.out_var] = *res;
          changed = true;
        } else if (it->second != *res) {
          return false;
        }
      }
    }

    // Non-equality comparisons and negations are pure checks (idempotent).
    for (const auto &cmp : c.cmps) {
      if (cmp.op == hyde::ComparisonOperator::kEqual) {
        continue;
      }
      auto l = b.find(cmp.lhs), r = b.find(cmp.rhs);
      if (l == b.end() || r == b.end()) {
        continue;
      }
      const Value lv = l->second, rv = r->second;
      bool ok = true;
      switch (cmp.op) {
        case hyde::ComparisonOperator::kNotEqual: ok = lv != rv; break;
        case hyde::ComparisonOperator::kLessThan:
          ok = cmp.sgn ? (static_cast<int64_t>(lv) < static_cast<int64_t>(rv))
                       : (lv < rv);
          break;
        case hyde::ComparisonOperator::kGreaterThan:
          ok = cmp.sgn ? (static_cast<int64_t>(lv) > static_cast<int64_t>(rv))
                       : (lv > rv);
          break;
        default: break;
      }
      if (!ok) {
        return false;
      }
    }

    for (const auto &na : c.neg) {
      bool ready = true;
      Row key;
      for (auto v : na.args) {
        auto it = b.find(v);
        if (it == b.end()) {
          ready = false;
          break;
        }
        key.push_back(it->second);
      }
      if (ready && rels.at(na.rel).Contains(key)) {
        return false;  // negated key present -> instance blocked.
      }
    }
    return true;
  }

  static bool UnifyAtom(const Atom &at, const Row &row, Binding &b) {
    if (row.size() != at.args.size()) {
      return false;
    }
    for (size_t i = 0; i < at.args.size(); ++i) {
      auto it = b.find(at.args[i]);
      if (it == b.end()) {
        b[at.args[i]] = row[i];
      } else if (it->second != row[i]) {
        return false;
      }
    }
    return true;
  }

  // `delta_pos` (>= 0) restricts the pos-atom at that index to `delta_rows`
  // (semi-naive driver); other atoms read the full relation via a join index.
  void Solve(const CompiledClause &c,
             const std::vector<std::vector<Row>> &agg_results, int delta_pos,
             const std::vector<Row> *delta_rows,
             const std::function<void(const Row &)> &emit) {
    if (c.disabled) {
      return;
    }
    Binding b;
    for (const auto &a : c.assigns) {
      auto it = b.find(a.first);
      if (it != b.end() && it->second != a.second) {
        return;
      }
      b[a.first] = a.second;
    }

    // Generators: positive relation atoms, then aggregates.
    std::function<void(Binding, size_t, size_t)> rec =
        [&](Binding cur, size_t pi, size_t ai) {
          if (!Propagate(c, cur)) {
            return;
          }
          if (pi < c.pos.size()) {
            const Atom &at = c.pos[pi];
            Relation &rel = rels.at(at.rel);

            auto visit = [&](const Row &row) {
              Binding nb = cur;
              if (UnifyAtom(at, row, nb)) {
                rec(std::move(nb), pi + 1, ai);
              }
            };

            if (static_cast<int>(pi) == delta_pos && delta_rows) {
              for (const auto &row : *delta_rows) {
                visit(row);
              }
              return;
            }
            // Index on the already-bound argument columns, if any.
            std::vector<unsigned> kcols;
            Row kvals;
            for (size_t i = 0; i < at.args.size(); ++i) {
              auto it = cur.find(at.args[i]);
              if (it != cur.end()) {
                kcols.push_back(static_cast<unsigned>(i));
                kvals.push_back(it->second);
              }
            }
            if (!kcols.empty()) {
              const auto &m = rel.GetIndex(kcols);
              auto f = m.find(kvals);
              if (f == m.end()) {
                return;
              }
              for (auto ri : f->second) {
                visit(rel.rows[ri]);
              }
            } else {
              for (const auto &row : rel.rows) {
                visit(row);
              }
            }
            return;
          }
          if (ai < c.aggs.size()) {
            const AggGoal &a = c.aggs[ai];
            const unsigned ng = static_cast<unsigned>(a.group_binds.size());
            const unsigned nc = static_cast<unsigned>(a.config_binds.size());
            for (const auto &res : agg_results[ai]) {
              Binding nb = cur;
              bool ok = true;
              for (unsigned k = 0; k < ng && ok; ++k) {
                ok = BindOrCheck(nb, a.group_binds[k].first, res[k]);
              }
              for (unsigned k = 0; k < nc && ok; ++k) {
                ok = BindOrCheck(nb, a.config_binds[k].first, res[ng + k]);
              }
              if (ok) {
                ok = BindOrCheck(nb, a.summary_var, res.back());
              }
              if (ok) {
                rec(std::move(nb), pi, ai + 1);
              }
            }
            return;
          }

          // Leaf: build the head row (every head var must be bound).
          Row h;
          h.reserve(c.head_vars.size());
          for (auto v : c.head_vars) {
            auto it = cur.find(v);
            if (it == cur.end()) {
              return;
            }
            h.push_back(it->second);
          }
          emit(h);
        };

    rec(std::move(b), 0, 0);
  }

  static bool BindOrCheck(Binding &b, uint64_t var, Value v) {
    auto it = b.find(var);
    if (it == b.end()) {
      b[var] = v;
      return true;
    }
    return it->second == v;
  }

  // ------------------------------------------------------------------
  // From-scratch materialization of every derived relation.

  std::vector<std::vector<Row>> AggResultsOf(const CompiledClause &c) {
    std::vector<std::vector<Row>> ar(c.aggs.size());
    for (size_t i = 0; i < c.aggs.size(); ++i) {
      ar[i] = ComputeAgg(c.aggs[i]);
    }
    return ar;
  }

  void EvalKV(Relation &rel) {
    // Gather DISTINCT raw (key ++ value) rows, then merge per key.
    std::vector<Row> raw;
    std::unordered_set<Row, RowHash> seen;
    for (const auto &c : clauses[rel.id]) {
      auto ar = AggResultsOf(c);
      Solve(c, ar, -1, nullptr, [&](const Row &h) {
        if (seen.insert(h).second) {
          raw.push_back(h);
        }
      });
    }
    std::sort(raw.begin(), raw.end());  // deterministic merge order

    std::map<Row, int64_t> acc;
    std::vector<Row> key_order;
    for (const auto &row : raw) {
      Row key;
      for (auto col : rel.kv_key_cols) {
        key.push_back(row[col]);
      }
      const auto v = static_cast<int64_t>(row[rel.kv_value_col]);
      auto it = acc.find(key);
      if (it == acc.end()) {
        acc.emplace(key, v);
        key_order.push_back(key);
      } else if (rel.kv_kind == 0) {
        it->second += v;  // additive @invertible fold
      } else {
        it->second = v;   // @recompute rescan: last live value wins
      }
    }
    rel.Clear();
    for (const auto &key : key_order) {
      Row r(rel.arity, 0);
      for (size_t i = 0; i < rel.kv_key_cols.size(); ++i) {
        r[rel.kv_key_cols[i]] = key[i];
      }
      r[rel.kv_value_col] = static_cast<Value>(acc[key]);
      rel.Add(r);
    }
  }

  void Recompute() {
    for (auto id : rel_order) {
      Relation &r = rels.at(id);
      if (r.is_base) {
        r.indexes.clear();  // base rows changed during NetAndApply.
      } else {
        r.Clear();
      }
    }

    for (const auto &stratum : strata) {
      bool recursive = false;
      for (auto id : stratum) {
        for (const auto &c : clauses[id]) {
          if (!c.rec_pos.empty()) {
            recursive = true;
          }
        }
      }

      // Seed: KV merges + every clause with no same-stratum body atom fires
      // once over the (complete) lower strata. Track newly-added rows as the
      // initial semi-naive delta.
      std::unordered_map<uint64_t, std::vector<Row>> delta;
      for (auto id : stratum) {
        Relation &rel = rels.at(id);
        if (rel.is_base) {
          continue;
        }
        if (rel.is_kv) {
          EvalKV(rel);
          delta[id] = rel.rows;
          continue;
        }
        for (const auto &c : clauses[id]) {
          if (!c.rec_pos.empty()) {
            continue;
          }
          auto ar = AggResultsOf(c);
          Solve(c, ar, -1, nullptr, [&](const Row &h) {
            if (rel.Add(h)) {
              delta[id].push_back(h);
            }
          });
        }
      }

      if (!recursive) {
        continue;
      }

      // Semi-naive fixpoint: each recursive clause is driven by the delta of
      // each of its same-stratum body atoms; other atoms read the full set.
      while (true) {
        std::unordered_map<uint64_t, std::vector<Row>> next;
        for (auto id : stratum) {
          Relation &rel = rels.at(id);
          if (rel.is_base || rel.is_kv) {
            continue;
          }
          for (const auto &c : clauses[id]) {
            if (c.rec_pos.empty()) {
              continue;
            }
            auto ar = AggResultsOf(c);
            for (unsigned p : c.rec_pos) {
              auto dit = delta.find(c.pos[p].rel);
              if (dit == delta.end() || dit->second.empty()) {
                continue;
              }
              Solve(c, ar, static_cast<int>(p), &dit->second,
                    [&](const Row &h) {
                      if (rel.Add(h)) {
                        next[id].push_back(h);
                      }
                    });
            }
          }
        }
        bool any = false;
        for (auto &kv : next) {
          if (!kv.second.empty()) {
            any = true;
          }
        }
        if (!any) {
          break;
        }
        delta = std::move(next);
      }
    }
  }

  // ------------------------------------------------------------------
  // Stratification: SCC condensation with negation/aggregate edges required
  // to cross strictly downward.

  struct Edge {
    uint64_t dst;
    bool strict;  // negation / aggregation
  };

  void Stratify() {
    std::unordered_map<uint64_t, std::vector<Edge>> graph;
    for (auto id : rel_order) {
      graph[id];
    }
    for (auto id : rel_order) {
      for (const auto &c : clauses[id]) {
        for (const auto &a : c.pos) {
          graph[a.rel].push_back({id, false});
        }
        for (const auto &a : c.neg) {
          graph[a.rel].push_back({id, true});
        }
        for (const auto &a : c.aggs) {
          graph[a.input_rel].push_back({id, true});
        }
      }
    }

    // Tarjan SCC (iterative-friendly recursion; corpus graphs are small).
    std::unordered_map<uint64_t, int> index, low, comp;
    std::unordered_map<uint64_t, bool> on_stack;
    std::vector<uint64_t> stack;
    int next_index = 0, next_comp = 0;

    std::function<void(uint64_t)> dfs = [&](uint64_t v) {
      index[v] = low[v] = next_index++;
      stack.push_back(v);
      on_stack[v] = true;
      for (const auto &e : graph[v]) {
        if (!index.count(e.dst)) {
          dfs(e.dst);
          low[v] = std::min(low[v], low[e.dst]);
        } else if (on_stack[e.dst]) {
          low[v] = std::min(low[v], index[e.dst]);
        }
      }
      if (low[v] == index[v]) {
        while (true) {
          uint64_t w = stack.back();
          stack.pop_back();
          on_stack[w] = false;
          comp[w] = next_comp;
          if (w == v) {
            break;
          }
        }
        ++next_comp;
      }
    };
    for (auto id : rel_order) {
      if (!index.count(id)) {
        dfs(id);
      }
    }

    // Reject an intra-SCC strict edge (unstratified negation/aggregation).
    for (auto id : rel_order) {
      for (const auto &e : graph[id]) {
        if (e.strict && comp[id] == comp[e.dst]) {
          Reject("unstratified negation/aggregation");
        }
      }
    }

    // Kahn topological order of the condensation (edges src -> dst); sources
    // evaluate first. Ties broken by declaration order for determinism.
    const int nc = next_comp;
    std::vector<std::vector<uint64_t>> members(nc);
    for (auto id : rel_order) {
      members[comp[id]].push_back(id);
    }
    std::vector<std::set<int>> cedges(nc);
    std::vector<int> indeg(nc, 0);
    for (auto id : rel_order) {
      for (const auto &e : graph[id]) {
        if (comp[id] != comp[e.dst]) {
          if (cedges[comp[id]].insert(comp[e.dst]).second) {
            ++indeg[comp[e.dst]];
          }
        }
      }
    }
    // Deterministic Kahn: pick ready components in declaration order of their
    // first member.
    std::vector<int> comp_rank(nc, 0);
    {
      int rank = 0;
      std::unordered_map<int, int> first_seen;
      for (auto id : rel_order) {
        if (!first_seen.count(comp[id])) {
          first_seen[comp[id]] = rank++;
        }
      }
      for (int i = 0; i < nc; ++i) {
        comp_rank[i] = first_seen[i];
      }
    }
    std::vector<bool> done(nc, false);
    for (int placed = 0; placed < nc; ++placed) {
      int best = -1;
      for (int i = 0; i < nc; ++i) {
        if (!done[i] && indeg[i] == 0) {
          if (best < 0 || comp_rank[i] < comp_rank[best]) {
            best = i;
          }
        }
      }
      if (best < 0) {
        Reject("internal: stratification cycle in condensation");
      }
      done[best] = true;
      strata.push_back(members[best]);
      for (int d : cedges[best]) {
        --indeg[d];
      }
    }

    // Record each relation's stratum index and mark same-stratum (recursive)
    // positive body-atom positions per clause.
    for (size_t s = 0; s < strata.size(); ++s) {
      for (auto id : strata[s]) {
        rel_stratum[id] = static_cast<int>(s);
      }
    }
    for (auto &kv : clauses) {
      const int hs = rel_stratum[kv.first];
      for (auto &c : kv.second) {
        c.rec_pos.clear();
        for (unsigned p = 0; p < c.pos.size(); ++p) {
          if (rel_stratum[c.pos[p].rel] == hs) {
            c.rec_pos.push_back(p);
          }
        }
      }
    }
  }

  // ------------------------------------------------------------------
  // Build the universe from the parsed module.

  int Load(const char *dr_path) {
    hyde::Parser parser(dm, err);
    hyde::DisplayConfiguration config = {dr_path, 2, true};
    auto m = parser.ParsePath(dr_path, config);
    if (!m) {
      err.Render(std::cerr);
      return 1;
    }
    module = *m;

    for (auto msg : module->Messages()) {
      hyde::ParsedDeclaration d(msg);
      Relation &r = EnsureRel(d, msg.IsReceived());
      r.is_published = msg.IsPublished();
      if (msg.IsReceived()) {
        const std::string name(msg.NameAsString());
        const unsigned arity = msg.Arity();
        auto key = std::make_pair(name, arity);
        if (!msg_index.count(key)) {
          MsgInfo mi;
          mi.name = name;
          mi.arity = arity;
          mi.differential = msg.IsDifferential();
          mi.types = r.types;
          mi.rel = r.id;
          msg_index.emplace(key, static_cast<unsigned>(msgs.size()));
          msgs.push_back(std::move(mi));
        }
      }
    }
    for (auto l : module->Locals()) {
      EnsureRel(hyde::ParsedDeclaration(l), false);
    }
    for (auto e : module->Exports()) {
      EnsureRel(hyde::ParsedDeclaration(e), false);
    }
    for (auto q : module->Queries()) {
      EnsureRel(hyde::ParsedDeclaration(q), false);
    }
    for (auto c : module->Clauses()) {
      auto decl = hyde::ParsedDeclaration::Of(c);
      EnsureRel(decl, false, c.Name());
      clauses[RelId(decl, c.Name())].push_back(Compile(c));
    }

    Stratify();
    return 0;
  }
};

// ---------------------------------------------------------------------
// .batches parsing — mirrors bin/Oracle/Main.cpp exactly.

struct BatchOp {
  unsigned msg{0};
  bool add{true};
  Row row;
};

std::vector<std::vector<BatchOp>> ParseBatches(const char *path, Interp &it) {
  std::ifstream in(path);
  if (!in) {
    std::cerr << "REFINTERP-REJECT: cannot open batches file '" << path
              << "'\n";
    std::exit(2);
  }
  std::vector<std::vector<BatchOp>> batches;
  std::optional<std::vector<BatchOp>> cur;
  std::string line;
  unsigned lineno = 0;
  auto fail = [&](const std::string &m) {
    std::cerr << "REFINTERP-REJECT: " << path << ":" << lineno << ": " << m
              << "\n";
    std::exit(2);
  };
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
    if (toks[0] == "batch") {
      if (cur) {
        fail("nested 'batch'");
      }
      cur.emplace();
    } else if (toks[0] == "end") {
      if (!cur) {
        fail("'end' outside a batch");
      }
      batches.push_back(std::move(*cur));
      cur.reset();
    } else if (toks[0] == "+" || toks[0] == "-") {
      if (!cur) {
        fail("op outside a batch");
      }
      if (toks.size() < 2) {
        fail("op without a message name");
      }
      const auto arity = static_cast<unsigned>(toks.size() - 2);
      auto mit = it.msg_index.find({toks[1], arity});
      if (mit == it.msg_index.end()) {
        fail("no received message '" + toks[1] + "' of arity " +
             std::to_string(arity));
      }
      const auto &mi = it.msgs[mit->second];
      BatchOp op;
      op.msg = mit->second;
      op.add = toks[0] == "+";
      if (!op.add && !mi.differential) {
        fail("'-' on non-@differential message '" + mi.name + "'");
      }
      for (unsigned i = 0; i < arity; ++i) {
        auto v = ParseValue(mi.types[i], toks[2 + i]);
        if (!v) {
          fail("value '" + toks[2 + i] + "' does not fit column " +
               std::to_string(i) + " of '" + mi.name + "'");
        }
        op.row.push_back(*v);
      }
      cur->push_back(std::move(op));
    } else {
      fail("unrecognized directive '" + toks[0] + "'");
    }
  }
  if (cur) {
    fail("unterminated batch at end of file");
  }
  return batches;
}

// OQ3 netting: dedup each side, adds-intersect-removes annihilate.
void NetAndApply(Interp &it, const std::vector<BatchOp> &ops) {
  struct Key {
    unsigned msg;
    Row row;
    bool operator==(const Key &o) const {
      return msg == o.msg && row == o.row;
    }
  };
  struct KeyHash {
    size_t operator()(const Key &k) const {
      return RowHash{}(k.row) * 31 + k.msg;
    }
  };
  std::unordered_map<Key, unsigned, KeyHash> net;  // 1 add, 2 remove, 3 both
  std::vector<Key> order;
  for (const auto &op : ops) {
    Key k{op.msg, op.row};
    auto f = net.find(k);
    if (f == net.end()) {
      net.emplace(k, op.add ? 1u : 2u);
      order.push_back(k);
    } else {
      f->second |= op.add ? 1u : 2u;
    }
  }
  for (const auto &k : order) {
    const unsigned n = net[k];
    if (n == 3u) {
      continue;  // annihilated.
    }
    Relation &r = it.rels.at(it.msgs[k.msg].rel);
    if (n == 1u) {
      r.Add(k.row);
    } else {
      if (r.set.erase(k.row)) {
        r.rows.erase(std::remove(r.rows.begin(), r.rows.end(), k.row),
                     r.rows.end());
      }
    }
  }
}

// ---------------------------------------------------------------------
// .probes sidecar.

struct Probe {
  std::string token;      // "<name>_<bindings>"
  std::vector<std::string> bound;  // raw value lexemes, declaration order
};

std::vector<Probe> ParseProbes(const char *path) {
  std::vector<Probe> probes;
  std::ifstream in(path);
  if (!in) {
    return probes;
  }
  std::string line;
  while (std::getline(in, line)) {
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
    Probe p;
    p.token = toks[0];
    p.bound.assign(toks.begin() + 1, toks.end());
    probes.push_back(std::move(p));
  }
  return probes;
}

// ---------------------------------------------------------------------

std::string RenderRow(const std::vector<hyde::TypeKind> &types, const Row &row,
                      const std::vector<unsigned> &cols) {
  std::string s;
  bool first = true;
  for (auto c : cols) {
    if (!first) {
      s += ' ';
    }
    first = false;
    s += PrintValue(types[c], row[c]);
  }
  return s;
}

}  // namespace

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "USAGE: " << argv[0]
              << " <case.dr> <case.batches> [<case.probes>]\n";
    return 2;
  }

  Interp it;
  if (int rc = it.Load(argv[1]); rc != 0) {
    return rc;
  }

  const std::string case_name =
      std::filesystem::path(argv[1]).stem().string();

  const auto batches = ParseBatches(argv[2], it);
  const std::vector<Probe> probes =
      argc >= 4 ? ParseProbes(argv[3]) : std::vector<Probe>{};

  std::ostringstream out;
  out << "REFINTERP " << case_name << " netting=oq3\n";

  // Published (transmit) messages, in declaration order.
  std::vector<uint64_t> pub;
  for (auto id : it.rel_order) {
    const Relation &r = it.rels.at(id);
    if (r.is_published) {
      pub.push_back(id);
    }
  }

  // Per-epoch from-scratch recompute + published-delta emission.
  //
  // Epoch N's published delta is new-minus-old / old-minus-new of published
  // membership. Epoch 0's "old" snapshot is NOT the empty set: a negation can
  // derive rows from an empty database (e.g. `!foo` holds before any batch,
  // so `enabled_features(false, false)` already exists). Materialize the
  // zero-fact program once and use its published membership as that snapshot.
  std::unordered_map<uint64_t, std::unordered_set<Row, RowHash>> prev;
  for (auto id : pub) {
    prev[id];
  }
  it.Recompute();
  for (auto id : pub) {
    const Relation &r = it.rels.at(id);
    prev[id] = r.set;
  }
  unsigned epoch = 0;
  for (const auto &ops : batches) {
    NetAndApply(it, ops);
    it.Recompute();

    out << "EPOCH " << epoch << "\n";
    std::vector<std::string> deltas;
    for (auto id : pub) {
      const Relation &r = it.rels.at(id);
      std::vector<unsigned> all(r.arity);
      for (unsigned i = 0; i < r.arity; ++i) {
        all[i] = i;
      }
      std::unordered_set<Row, RowHash> now(r.set);
      for (const auto &row : r.rows) {
        if (!prev[id].count(row)) {
          std::string s = "+ " + r.name;
          if (r.arity) {
            s += " " + RenderRow(r.types, row, all);
          }
          deltas.push_back(std::move(s));
        }
      }
      for (const auto &row : prev[id]) {
        if (!now.count(row)) {
          std::string s = "- " + r.name;
          if (r.arity) {
            s += " " + RenderRow(r.types, row, all);
          }
          deltas.push_back(std::move(s));
        }
      }
      prev[id] = std::move(now);
    }
    std::sort(deltas.begin(), deltas.end());
    for (const auto &d : deltas) {
      out << d << "\n";
    }
    ++epoch;
  }

  // FINAL: every published relation's membership.
  out << "FINAL\n";
  {
    std::vector<std::string> members;
    for (auto id : pub) {
      const Relation &r = it.rels.at(id);
      std::vector<unsigned> all(r.arity);
      for (unsigned i = 0; i < r.arity; ++i) {
        all[i] = i;
      }
      for (const auto &row : r.rows) {
        std::string s = r.name;
        if (r.arity) {
          s += " " + RenderRow(r.types, row, all);
        }
        members.push_back(std::move(s));
      }
    }
    std::sort(members.begin(), members.end());
    for (const auto &m : members) {
      out << m << "\n";
    }
  }

  // QUERY blocks. All-free queries: full enumeration in declaration order.
  for (auto q : it.module->Queries()) {
    const std::string pattern(hyde::ParsedDeclaration(q).BindingPattern());
    if (pattern.find('b') != std::string::npos) {
      continue;  // bound queries are probe-restricted (below).
    }
    const uint64_t id = Interp::CanonicalId(hyde::ParsedDeclaration(q));
    auto rit = it.rels.find(id);
    out << "QUERY " << q.NameAsString() << "_" << pattern << "\n";
    if (rit == it.rels.end()) {
      continue;
    }
    const Relation &r = rit->second;
    std::vector<unsigned> all(r.arity);
    for (unsigned i = 0; i < r.arity; ++i) {
      all[i] = i;
    }
    std::vector<std::string> answers;
    for (const auto &row : r.rows) {
      answers.push_back(RenderRow(r.types, row, all));
    }
    std::sort(answers.begin(), answers.end());
    for (const auto &a : answers) {
      out << a << "\n";
    }
  }

  // Probe-restricted bound queries, in sidecar order.
  for (const auto &p : probes) {
    // Match the probe token against a query's "<name>_<bindings>".
    std::optional<hyde::ParsedQuery> found;
    for (auto q : it.module->Queries()) {
      const std::string tok = std::string(q.NameAsString()) + "_" +
                              std::string(hyde::ParsedDeclaration(q).BindingPattern());
      if (tok == p.token) {
        found = q;
        break;
      }
    }
    if (!found) {
      continue;  // unknown probe token: recorded, not an error.
    }
    const std::string pattern(hyde::ParsedDeclaration(*found).BindingPattern());
    std::vector<unsigned> bound_cols, free_cols;
    for (unsigned i = 0; i < pattern.size(); ++i) {
      if (pattern[i] == 'b') {
        bound_cols.push_back(i);
      } else {
        free_cols.push_back(i);
      }
    }
    const uint64_t id = Interp::CanonicalId(hyde::ParsedDeclaration(*found));
    const Relation &r = it.rels.at(id);

    // Parse the probe's bound values (declaration column order).
    std::vector<Value> bvals;
    bool ok = bound_cols.size() == p.bound.size();
    for (size_t i = 0; ok && i < bound_cols.size(); ++i) {
      auto v = ParseValue(r.types[bound_cols[i]], p.bound[i]);
      if (!v) {
        ok = false;
      } else {
        bvals.push_back(*v);
      }
    }

    std::string head = "QUERY " + std::string(found->NameAsString()) + "_" +
                       pattern + " PROBE";
    for (const auto &bv : p.bound) {
      head += " " + bv;
    }
    out << head << "\n";
    if (!ok) {
      continue;
    }
    std::vector<std::string> answers;
    for (const auto &row : r.rows) {
      bool match = true;
      for (size_t i = 0; i < bound_cols.size(); ++i) {
        if (row[bound_cols[i]] != bvals[i]) {
          match = false;
          break;
        }
      }
      if (match) {
        answers.push_back(RenderRow(r.types, row, free_cols));
      }
    }
    std::sort(answers.begin(), answers.end());
    for (const auto &a : answers) {
      out << a << "\n";
    }
  }

  std::cout << out.str();
  return 0;
}
