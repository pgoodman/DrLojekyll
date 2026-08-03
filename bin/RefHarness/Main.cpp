// Copyright 2026, Peter Goodman. All rights reserved.

// `drlojekyll-refharness`: the I0 behavioral-harness emitter — reads a
// parsed module and emits `behavioral_main.cpp` against the STABLE public
// query/message ABI naming (OG2-tool: the referee couples to the frozen
// seam, never to the codegen under cutover). The emitted driver + the
// case's generated `datalog.cpp` + the runtime = the BEHAVIORAL BINARY,
// whose stdout is the Canonical Behavioral Format (CBF). See
// docs/proposals/RegionalDataFlowCore.artifacts/stage-i0-interpreter.md
// (H4 + the §7 dated amendments) and the CBF contract in CLAUDE.md.
//
// The emitter itself parses the module with `hyde::DisplayManager` +
// `hyde::Parser` ONLY (no `Query::Build`, no DataFlow) — it needs nothing
// beyond the declared message/query/functor surface. Everything the driver
// must know about the SPECIFIC generated header (whether a demanded query
// carries a forcing `(log, functors)` prefix, whether a received message is
// deletion-capable) is resolved either from the parse (`@differential`) or,
// where the parse cannot see it, at the driver's OWN compile time via an
// `if constexpr (requires { ... })` overload probe against the real
// `datalog.h`. So one emitted driver is correct whether the case is compiled
// plain or under `-demand`/`-demand-instance`.
//
// CLI:
//   drlojekyll-refharness <case.dr> -o <behavioral_main.cpp>
// The EMITTED driver's CLI is:
//   ./behavioral <case.batches> [<case.probes>]   ->  CBF on stdout
//
// Definitional-envelope rejects (float/foreign columns, a functor whose
// by-name semantics are not modeled) print one
// `REFHARNESS-REJECT: <reason>` line to stderr and exit 2 — mirroring the
// interpreter's envelope so the two tools reject the same programs.

#include <drlojekyll/Display/DisplayConfiguration.h>
#include <drlojekyll/Display/DisplayManager.h>
#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Parse/Parse.h>
#include <drlojekyll/Parse/Parser.h>
#include <drlojekyll/Parse/Type.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

using hyde::TypeKind;

[[noreturn]] void Reject(const std::string &why) {
  std::cerr << "REFHARNESS-REJECT: " << why << "\n";
  std::exit(2);
}

// ---------------------------------------------------------------------
// Type mapping. The generated header spells every column with a fixed-width
// C++ type and every runtime tuple as `Tup_<suffix>_<suffix>...`.

const char *CxxType(TypeKind k) {
  switch (k) {
    case TypeKind::kBoolean: return "bool";
    case TypeKind::kSigned8: return "int8_t";
    case TypeKind::kSigned16: return "int16_t";
    case TypeKind::kSigned32: return "int32_t";
    case TypeKind::kSigned64: return "int64_t";
    case TypeKind::kUnsigned8: return "uint8_t";
    case TypeKind::kUnsigned16: return "uint16_t";
    case TypeKind::kUnsigned32: return "uint32_t";
    case TypeKind::kUnsigned64: return "uint64_t";
    default: return nullptr;
  }
}

const char *TupSuffix(TypeKind k) {
  switch (k) {
    case TypeKind::kBoolean: return "b";
    case TypeKind::kSigned8: return "i8";
    case TypeKind::kSigned16: return "i16";
    case TypeKind::kSigned32: return "i32";
    case TypeKind::kSigned64: return "i64";
    case TypeKind::kUnsigned8: return "u8";
    case TypeKind::kUnsigned16: return "u16";
    case TypeKind::kUnsigned32: return "u32";
    case TypeKind::kUnsigned64: return "u64";
    default: return nullptr;
  }
}

// The `cbf::Kind` enumerator name for a column type (used by the driver's
// runtime value parser/printer).
const char *KindEnum(TypeKind k) {
  switch (k) {
    case TypeKind::kBoolean: return "cbf::K_BOOL";
    case TypeKind::kSigned8: return "cbf::K_S8";
    case TypeKind::kSigned16: return "cbf::K_S16";
    case TypeKind::kSigned32: return "cbf::K_S32";
    case TypeKind::kSigned64: return "cbf::K_S64";
    case TypeKind::kUnsigned8: return "cbf::K_U8";
    case TypeKind::kUnsigned16: return "cbf::K_U16";
    case TypeKind::kUnsigned32: return "cbf::K_U32";
    case TypeKind::kUnsigned64: return "cbf::K_U64";
    default: return nullptr;
  }
}

bool IsSigned(TypeKind k) {
  switch (k) {
    case TypeKind::kSigned8:
    case TypeKind::kSigned16:
    case TypeKind::kSigned32:
    case TypeKind::kSigned64: return true;
    default: return false;
  }
}

// Convert a typed C++ expression `var` (already of column type `k`) to the
// canonical `uint64_t` the CBF value printer consumes.
std::string CanonExpr(TypeKind k, const std::string &var) {
  if (k == TypeKind::kBoolean) {
    return "(uint64_t)(" + var + " ? 1 : 0)";
  }
  if (IsSigned(k)) {
    return "(uint64_t)(int64_t)" + var;
  }
  return "(uint64_t)" + var;
}

// Cast a canonical `uint64_t` value expression `expr` back to column type `k`
// (used when re-materializing a batch tuple field or a bound query argument).
std::string CastFromCanon(TypeKind k, const std::string &expr) {
  if (k == TypeKind::kBoolean) {
    return "(bool)(" + expr + ")";
  }
  if (IsSigned(k)) {
    return std::string("(") + CxxType(k) + ")(int64_t)(" + expr + ")";
  }
  return std::string("(") + CxxType(k) + ")(" + expr + ")";
}

void CheckColumn(TypeKind k, const std::string &where) {
  if (!CxxType(k)) {
    Reject("unsupported column type in " + where +
           " (only bool and fixed-width integers are modeled)");
  }
}

// ---------------------------------------------------------------------
// Parsed-surface summaries.

struct Col {
  TypeKind kind;
};

struct MsgDesc {
  std::string name;
  unsigned arity{0};
  bool differential{false};
  bool received{false};
  bool published{false};
  std::vector<Col> cols;
};

struct QueryDesc {
  std::string name;
  std::string bindings;        // per-column 'b'/'f'
  std::string ident;           // "<name>_<bindings>" — the ABI entry name
  bool all_free{false};
  bool all_bound{false};
  std::vector<Col> cols;       // all params, decl order
  std::vector<unsigned> bound_pos;
  std::vector<unsigned> free_pos;
};

// ---------------------------------------------------------------------
// The emitter.

class Emitter {
 public:
  hyde::DisplayManager display_manager;
  hyde::ErrorLog error_log{display_manager};

  std::string case_name;
  std::vector<MsgDesc> received_msgs;   // decl order; feedable surface
  std::vector<MsgDesc> published_msgs;  // decl order; log-hook + FINAL surface
  std::vector<QueryDesc> queries;       // unique adornments, decl order
  std::string functors;                 // emitted functor free-function defs

  std::string out;

  void P(const std::string &s) { out += s; }
  void L(const std::string &s) { out += s; out += '\n'; }

  int Run(const char *dr_path, const char *out_path) {
    // Case name = basename without ".dr".
    {
      std::string p(dr_path);
      auto slash = p.find_last_of('/');
      if (slash != std::string::npos) {
        p = p.substr(slash + 1);
      }
      if (p.size() > 3 && p.substr(p.size() - 3) == ".dr") {
        p = p.substr(0, p.size() - 3);
      }
      case_name = p;
    }

    hyde::Parser parser(display_manager, error_log);
    hyde::DisplayConfiguration config = {dr_path, 2, true};
    auto module_opt = parser.ParsePath(dr_path, config);
    if (!module_opt) {
      error_log.Render(std::cerr);
      return EXIT_FAILURE;
    }
    const hyde::ParsedModule module = *module_opt;

    Collect(module);
    EmitFunctors(module);
    Emit();

    std::ofstream os(out_path);
    if (!os) {
      std::cerr << "drlojekyll-refharness: cannot open output '" << out_path
                << "'\n";
      return EXIT_FAILURE;
    }
    os << out;
    return EXIT_SUCCESS;
  }

  // ------------------------------------------------------------------
  void Collect(const hyde::ParsedModule &module) {
    for (auto msg : module.Messages()) {
      MsgDesc m;
      m.name = std::string(msg.NameAsString());
      m.arity = msg.Arity();
      m.differential = msg.IsDifferential();
      m.received = msg.IsReceived();
      m.published = msg.IsPublished();
      for (unsigned i = 0; i < m.arity; ++i) {
        const TypeKind k = msg.NthParameter(i).Type().UnderlyingKind();
        CheckColumn(k, "message '" + m.name + "'");
        m.cols.push_back({k});
      }
      if (m.received) {
        received_msgs.push_back(m);
      }
      if (m.published) {
        published_msgs.push_back(m);
      }
    }

    std::vector<std::string> seen;
    for (auto q : module.Queries()) {
      QueryDesc d;
      d.name = std::string(q.NameAsString());
      const unsigned n = q.Arity();
      bool any_bound = false, any_free = false;
      for (unsigned i = 0; i < n; ++i) {
        auto param = q.NthParameter(i);
        const TypeKind k = param.Type().UnderlyingKind();
        CheckColumn(k, "query '" + d.name + "'");
        d.cols.push_back({k});
        const bool bound =
            param.Binding() == hyde::ParameterBinding::kBound;
        if (bound) {
          d.bindings += 'b';
          d.bound_pos.push_back(i);
          any_bound = true;
        } else {
          d.bindings += 'f';
          d.free_pos.push_back(i);
          any_free = true;
        }
      }
      d.ident = d.name + "_" + d.bindings;
      d.all_free = !any_bound;
      d.all_bound = !any_free;

      bool dup = false;
      for (const auto &s : seen) {
        if (s == d.ident) {
          dup = true;
          break;
        }
      }
      if (dup) {
        continue;
      }
      seen.push_back(d.ident);
      queries.push_back(std::move(d));
    }
  }

  // ------------------------------------------------------------------
  // Functor free-function emission (by name, mirroring the oracle's
  // registry semantics — see bin/Oracle/Main.cpp "BY-NAME FUNCTOR
  // SEMANTICS"). The corpus drivers supply exactly these bodies.

  void EmitFunctors(const hyde::ParsedModule &module) {
    std::string body;
    std::vector<std::string> emitted;  // dedup by function name
    auto once = [&](const std::string &name) {
      for (const auto &e : emitted) {
        if (e == name) {
          return false;
        }
      }
      emitted.push_back(name);
      return true;
    };

    for (auto f : module.Functors()) {
      const std::string fname(f.NameAsString());
      const unsigned n = f.Arity();

      // Column type sanity.
      for (unsigned i = 0; i < n; ++i) {
        CheckColumn(f.NthParameter(i).Type().UnderlyingKind(),
                    "functor '" + fname + "'");
      }

      const bool is_agg = f.IsAggregate();
      const bool is_merge = f.IsMerge();

      // (1) MAP / merge-map free function: `<fname>_<pattern>`. Emitted for
      //     any range functor with at least one free (result) column, and
      //     for merge functors (their `bbf` map is the proposed-value pick).
      if (!is_agg) {
        std::string pattern;
        std::vector<TypeKind> bound_types;
        std::optional<TypeKind> free_type;
        for (unsigned i = 0; i < n; ++i) {
          auto p = f.NthParameter(i);
          const TypeKind k = p.Type().UnderlyingKind();
          if (p.Binding() == hyde::ParameterBinding::kBound) {
            pattern += 'b';
            bound_types.push_back(k);
          } else if (p.Binding() == hyde::ParameterBinding::kFree) {
            pattern += 'f';
            if (!free_type) {
              free_type = k;
            }
          } else {
            pattern += '?';
          }
        }
        if (free_type && !bound_types.empty() &&
            pattern.find('?') == std::string::npos) {
          const std::string fn = fname + "_" + pattern;
          if (once(fn)) {
            body += EmitMapBody(f, fname, fn, bound_types, *free_type, is_merge);
          }
        }
      }

      // (2) Aggregate / merge reduction bodies.
      if (is_agg || is_merge) {
        // Value type (aggregate column / value column) and summary/working
        // type (summary column / merged column).
        TypeKind vtype = TypeKind::kSigned32;
        TypeKind wtype = TypeKind::kSigned32;
        bool got_v = false, got_w = false;
        for (unsigned i = 0; i < n; ++i) {
          auto p = f.NthParameter(i);
          const TypeKind k = p.Type().UnderlyingKind();
          switch (p.Binding()) {
            case hyde::ParameterBinding::kAggregate:
              vtype = k; got_v = true; break;
            case hyde::ParameterBinding::kSummary:
              wtype = k; got_w = true; break;
            default: break;
          }
        }
        if (is_merge && !got_v) {
          // A merge functor's value type is its (bound) proposed-value column
          // and its working type the merged (free) column.
          for (unsigned i = 0; i < n; ++i) {
            auto p = f.NthParameter(i);
            const TypeKind k = p.Type().UnderlyingKind();
            if (p.Binding() == hyde::ParameterBinding::kFree) {
              wtype = k; got_w = true;
            } else if (p.Binding() == hyde::ParameterBinding::kBound) {
              vtype = k; got_v = true;  // last bound wins == proposed value
            }
          }
        }
        (void) got_v;
        (void) got_w;

        // CONFIG columns: an aggregate functor's kBound params (leading, in
        // declaration order). The landed reduction-body ABI gives _combine/
        // _uncombine/_reduce a LEADING config parameter per config column;
        // _identity stays config-free (config_agg_1/2 are the corpus shape).
        std::vector<TypeKind> cfg_types;
        if (is_agg) {
          for (unsigned i = 0; i < n; ++i) {
            auto p = f.NthParameter(i);
            if (p.Binding() == hyde::ParameterBinding::kBound) {
              cfg_types.push_back(p.Type().UnderlyingKind());
            }
          }
        }

        if (f.IsInvertible()) {
          if (once(fname + "_identity")) {
            body += EmitInvertible(fname, vtype, wtype, cfg_types);
          }
        } else if (f.IsRecompute() || (is_agg && !is_merge)) {
          // An aggregate functor with NO declared algebra defaults to
          // @recompute (aggregate_1 is the corpus witness); only a KV
          // `mutable(...)` MERGE functor requires a declared algebra
          // (the V-ALGEBRA reject).
          if (once(fname + "_reduce")) {
            body += EmitRecompute(f, fname, vtype, wtype, is_merge, cfg_types);
          }
        } else {
          Reject("KV merge functor '" + fname +
                 "' declares no @invertible/@recompute algebra");
        }
      }
    }
    functors = body;
  }

  std::string EmitMapBody(const hyde::ParsedFunctor &, const std::string &base,
                          const std::string &fn,
                          const std::vector<TypeKind> &bound_types,
                          TypeKind free_type, bool is_merge) {
    std::string s;
    s += std::string(CxxType(free_type)) + " " + fn + "(";
    for (size_t i = 0; i < bound_types.size(); ++i) {
      if (i) {
        s += ", ";
      }
      s += std::string(CxxType(bound_types[i])) + " a" + std::to_string(i);
    }
    s += ") {\n";

    // Semantics by name / role.
    if (is_merge) {
      // Proposed-value pick (last-writer): return the final bound arg.
      s += "  return a" + std::to_string(bound_types.size() - 1) + ";\n";
    } else if (base.rfind("div_", 0) == 0 && bound_types.size() == 2) {
      s += "  return a1 != 0 ? (" + std::string(CxxType(free_type)) +
           ")(a0 / a1) : 0;\n";
    } else if (base.rfind("add_", 0) == 0 && bound_types.size() == 2) {
      s += "  return (" + std::string(CxxType(free_type)) + ")(a0 + a1);\n";
    } else if (base.rfind("mul_", 0) == 0 && bound_types.size() == 2) {
      s += "  return (" + std::string(CxxType(free_type)) + ")(a0 * a1);\n";
    } else {
      Reject("MAP functor '" + base +
             "' has no by-name semantics modeled by the harness");
    }
    s += "}\n";
    return s;
  }

  std::string EmitInvertible(const std::string &base, TypeKind vtype,
                             TypeKind wtype,
                             const std::vector<TypeKind> &cfg_types) {
    const std::string W = CxxType(wtype);
    const std::string V = CxxType(vtype);
    std::string cfg_params;
    for (size_t i = 0; i < cfg_types.size(); ++i) {
      cfg_params += std::string(CxxType(cfg_types[i])) + " cfg" +
                    std::to_string(i) + ", ";
    }
    std::string combine, uncombine;
    if (!cfg_types.empty()) {
      // Config-gated folds are BY NAME (the Oracle registry semantics).
      if (base.rfind("sum_above", 0) == 0 && cfg_types.size() == 1) {
        combine = "(cfg0 <= v) ? (" + W + ")(w + v) : w";
        uncombine = "(cfg0 <= v) ? (" + W + ")(w - v) : w";
      } else {
        Reject("config @invertible functor '" + base +
               "' has no by-name semantics modeled by the harness");
      }
    } else if (base.rfind("count", 0) == 0) {
      combine = "w + 1";
      uncombine = "w - 1";
    } else {
      // sum / add / any additive @invertible fold.
      combine = "w + v";
      uncombine = "w - v";
    }
    std::string s;
    s += W + " " + base + "_identity() { return 0; }\n";
    s += W + " " + base + "_combine(" + cfg_params + W + " w, " + V +
         " v) { (void) v; return " + combine + "; }\n";
    s += W + " " + base + "_uncombine(" + cfg_params + W + " w, " + V +
         " v) { (void) v; return " + uncombine + "; }\n";
    return s;
  }

  std::string EmitRecompute(const hyde::ParsedFunctor &, const std::string &base,
                            TypeKind vtype, TypeKind wtype, bool is_merge,
                            const std::vector<TypeKind> &cfg_types) {
    const std::string W = CxxType(wtype);
    const std::string V = CxxType(vtype);
    std::string cfg_params;
    for (size_t i = 0; i < cfg_types.size(); ++i) {
      cfg_params += std::string(CxxType(cfg_types[i])) + " cfg" +
                    std::to_string(i) + ", ";
    }
    std::string s;
    s += W + " " + base + "_reduce(" + cfg_params + "const " + V +
         " *values, const int32_t *counts, std::size_t n) {\n";
    if (!cfg_types.empty()) {
      // Config-gated rescans are BY NAME (the Oracle registry semantics).
      if (base.rfind("max_above", 0) == 0 && cfg_types.size() == 1) {
        // Gated max with the empty-group sentinel (config_agg_2's landed
        // shape; the corpus keeps gated groups non-empty).
        s += "  " + W + " best = 0; bool any = false;\n";
        s += "  for (std::size_t i = 0; i < n; ++i) {\n";
        s += "    if (counts[i] <= 0) continue;\n";
        s += "    if (values[i] < cfg0) continue;\n";
        s += "    if (!any || values[i] > best) { best = values[i]; any = true; }\n";
        s += "  }\n";
        s += "  return any ? best : std::numeric_limits<" + W + ">::min();\n";
        s += "}\n";
        return s;
      }
      Reject("config @recompute functor '" + base +
             "' has no by-name semantics modeled by the harness");
    }
    if (is_merge || base.rfind("new_", 0) == 0) {
      // Surviving value (a KV cell holds one live value per key).
      s += "  " + W + " res = 0;\n";
      s += "  for (std::size_t i = 0; i < n; ++i) { if (counts[i] > 0) res = values[i]; }\n";
      s += "  return res;\n";
    } else if (base.rfind("count", 0) == 0) {
      // Count of LIVE members (aggregate_1's landed count_i32_reduce shape).
      s += "  " + W + " res = 0;\n";
      s += "  for (std::size_t i = 0; i < n; ++i) { if (counts[i] > 0) ++res; }\n";
      s += "  return res;\n";
    } else if (base.rfind("max", 0) == 0) {
      s += "  " + W + " res = 0; bool any = false;\n";
      s += "  for (std::size_t i = 0; i < n; ++i) { if (counts[i] > 0) { if (!any || values[i] > res) res = values[i]; any = true; } }\n";
      s += "  return res;\n";
    } else {
      // sum / additive rescan over the live multiset.
      s += "  " + W + " res = 0;\n";
      s += "  for (std::size_t i = 0; i < n; ++i) { if (counts[i] > 0) res = (" +
           W + ")(res + values[i]); }\n";
      s += "  return res;\n";
    }
    s += "}\n";
    return s;
  }

  // ------------------------------------------------------------------
  void Emit(void) {
    EmitHeaderAndRuntime();
    L(functors);
    EmitLog();
    EmitFeeder();
    EmitQueryDrivers();
    EmitMain();
  }

  void EmitHeaderAndRuntime(void) {
    L("// Copyright 2026, Peter Goodman. All rights reserved.");
    L("// AUTO-GENERATED by drlojekyll-refharness. Do not edit.");
    L("//");
    L("// Behavioral binary for case '" + case_name + "'. Emits the Canonical");
    L("// Behavioral Format (CBF) on stdout.");
    L("//   usage: ./behavioral <case.batches> [<case.probes>]");
    L("");
    L("#include <algorithm>");
    L("#include <cstdint>");
    L("#include <cstdio>");
    L("#include <cstdlib>");
    L("#include <cstring>");
    L("#include <fstream>");
    L("#include <iostream>");
    L("#include <limits>");
    L("#include <map>");
    L("#include <optional>");
    L("#include <set>");
    L("#include <sstream>");
    L("#include <string>");
    L("#include <vector>");
    L("");
    L("#include \"datalog.h\"");
    L("");
    L(R"CBF(namespace cbf {

enum Kind { K_BOOL, K_S8, K_S16, K_S32, K_S64, K_U8, K_U16, K_U32, K_U64 };

inline bool is_signed(Kind k) { return k >= K_S8 && k <= K_S64; }
inline unsigned width(Kind k) {
  switch (k) {
    case K_BOOL: return 1;
    case K_S8: case K_U8: return 8;
    case K_S16: case K_U16: return 16;
    case K_S32: case K_U32: return 32;
    default: return 64;
  }
}

// Mirrors bin/Oracle/Main.cpp ParseValue (lexeme-for-lexeme).
inline std::optional<uint64_t> parse_val(Kind k, const std::string &tok) {
  if (k == K_BOOL) {
    if (tok == "true" || tok == "1") return (uint64_t) 1;
    if (tok == "false" || tok == "0") return (uint64_t) 0;
    return std::nullopt;
  }
  const unsigned w = width(k);
  errno = 0;
  char *end = nullptr;
  if (is_signed(k)) {
    const long long v = std::strtoll(tok.c_str(), &end, 0);
    if (errno || !end || *end) return std::nullopt;
    const long long lo = w == 64 ? INT64_MIN : -(1ll << (w - 1));
    const long long hi = w == 64 ? INT64_MAX : (1ll << (w - 1)) - 1;
    if (v < lo || v > hi) return std::nullopt;
    return (uint64_t)(int64_t) v;
  } else {
    if (!tok.empty() && tok[0] == '-') return std::nullopt;
    const unsigned long long v = std::strtoull(tok.c_str(), &end, 0);
    if (errno || !end || *end) return std::nullopt;
    if (w < 64 && v > ((1ull << w) - 1ull)) return std::nullopt;
    return (uint64_t) v;
  }
}

// Mirrors bin/Oracle/Main.cpp PrintValue (lexeme-for-lexeme).
inline std::string print_val(Kind k, uint64_t v) {
  if (k == K_BOOL) return v ? "true" : "false";
  if (is_signed(k)) return std::to_string((int64_t) v);
  return std::to_string(v);
}

struct MsgMeta {
  std::string name;
  unsigned arity;
  std::vector<Kind> kinds;
  bool differential;
};

struct Op {
  unsigned midx;      // index into the received-message table
  bool add;
  std::vector<uint64_t> row;
};

inline int find_msg(const std::vector<MsgMeta> &t, const std::string &n,
                    unsigned ar) {
  for (size_t i = 0; i < t.size(); ++i) {
    if (t[i].name == n && t[i].arity == ar) return (int) i;
  }
  return -1;
}

[[noreturn]] inline void die(const std::string &m) {
  std::cerr << "behavioral: " << m << "\n";
  std::exit(2);
}

// One block = one epoch. Grammar identical to bin/Oracle/Main.cpp.
inline std::vector<std::vector<Op>> parse_batches(
    const char *path, const std::vector<MsgMeta> &msgs) {
  std::ifstream in(path);
  if (!in) die(std::string("cannot open batches '") + path + "'");
  std::vector<std::vector<Op>> batches;
  std::optional<std::vector<Op>> cur;
  std::string line;
  unsigned lineno = 0;
  while (std::getline(in, line)) {
    ++lineno;
    if (auto pos = line.find('#'); pos != std::string::npos) line.erase(pos);
    std::istringstream ls(line);
    std::vector<std::string> toks;
    for (std::string t; ls >> t;) toks.push_back(t);
    if (toks.empty()) continue;
    auto err = [&](const std::string &m) {
      die(std::string(path) + ":" + std::to_string(lineno) + ": " + m);
    };
    if (toks[0] == "batch") {
      if (cur) err("nested 'batch'");
      cur.emplace();
    } else if (toks[0] == "end") {
      if (!cur) err("'end' outside a batch");
      batches.push_back(std::move(*cur));
      cur.reset();
    } else if (toks[0] == "+" || toks[0] == "-") {
      if (!cur) err("op outside a batch");
      if (toks.size() < 2) err("op without a message name");
      const unsigned arity = (unsigned)(toks.size() - 2);
      const int mi = find_msg(msgs, toks[1], arity);
      if (mi < 0) err("no received message '" + toks[1] + "' of arity " +
                      std::to_string(arity));
      Op op;
      op.midx = (unsigned) mi;
      op.add = toks[0] == "+";
      if (!op.add && !msgs[mi].differential)
        err("'-' on non-@differential message '" + toks[1] + "'");
      for (unsigned i = 0; i < arity; ++i) {
        auto v = parse_val(msgs[mi].kinds[i], toks[2 + i]);
        if (!v) err("value '" + toks[2 + i] + "' does not fit column " +
                    std::to_string(i) + " of '" + toks[1] + "'");
        op.row.push_back(*v);
      }
      cur->push_back(std::move(op));
    } else {
      err("unrecognized directive '" + toks[0] + "'");
    }
  }
  if (cur) die(std::string(path) + ": unterminated batch");
  return batches;
}

struct Netted {
  // per received-message index -> netted add / remove rows (first-seen order).
  std::vector<std::vector<std::vector<uint64_t>>> adds, removes;
};

// OQ3: dedup each side, adds and removes of the same row annihilate.
inline Netted oq3(const std::vector<Op> &ops, size_t num_msgs) {
  Netted n;
  n.adds.resize(num_msgs);
  n.removes.resize(num_msgs);
  struct Key {
    unsigned midx;
    std::vector<uint64_t> row;
    bool operator<(const Key &o) const {
      if (midx != o.midx) return midx < o.midx;
      return row < o.row;
    }
  };
  std::map<Key, unsigned> sign;  // 1 add, 2 remove, 3 both
  std::vector<Key> order;
  for (const auto &op : ops) {
    Key k{op.midx, op.row};
    auto it = sign.find(k);
    if (it == sign.end()) {
      sign.emplace(k, op.add ? 1u : 2u);
      order.push_back(k);
    } else {
      it->second |= op.add ? 1u : 2u;
    }
  }
  for (const auto &k : order) {
    const unsigned s = sign[k];
    if (s == 1u) n.adds[k.midx].push_back(k.row);
    else if (s == 2u) n.removes[k.midx].push_back(k.row);
  }
  return n;
}

}  // namespace cbf
)CBF");
  }

  // ------------------------------------------------------------------
  // CBF-recording log: one hook per PUBLISHED message. The generated code
  // calls `log.<name>_<arity>(cols..., bool added)` once per net presence
  // change at each commit sweep.

  void EmitLog(void) {
    // Per-epoch CBF deltas are the MEMBERSHIP SET-DIFF of published
    // relations before vs after the epoch (the H2 contract), NOT the raw
    // captured hook lines: one .batches block can dispatch several entry-
    // point calls (one per message), and transient intra-block presence
    // flips (publish then retract of the same row) must net away.
    L("struct CbfLog {");
    L("  std::map<std::string, std::set<std::string>> final_sets;");
    L("");
    for (const auto &m : published_msgs) {
      std::string sig = "  void " + m.name + "_" + std::to_string(m.arity) + "(";
      for (unsigned i = 0; i < m.arity; ++i) {
        if (i) {
          sig += ", ";
        }
        sig += std::string(CxxType(m.cols[i].kind)) + " c" + std::to_string(i);
      }
      if (m.arity) {
        sig += ", ";
      }
      sig += "bool added) {";
      L(sig);
      // Build the space-joined value string.
      L("    std::string vals;");
      for (unsigned i = 0; i < m.arity; ++i) {
        if (i) {
          L("    vals += \" \";");
        }
        L("    vals += cbf::print_val(" + std::string(KindEnum(m.cols[i].kind)) +
          ", " + CanonExpr(m.cols[i].kind, "c" + std::to_string(i)) + ");");
      }
      L("    if (added) final_sets[\"" + m.name + "\"].insert(vals);");
      L("    else final_sets[\"" + m.name + "\"].erase(vals);");
      L("  }");
    }
    L("};");
    L("");
  }

  // ------------------------------------------------------------------
  // Batch feeder: build the message table, then per received message with
  // netted ops build its Vec(s) and call the entry point.

  void EmitFeeder(void) {
    // Message-table builder.
    L("static std::vector<cbf::MsgMeta> BuildMsgTable(void) {");
    L("  std::vector<cbf::MsgMeta> t;");
    for (const auto &m : received_msgs) {
      std::string kinds = "{";
      for (unsigned i = 0; i < m.arity; ++i) {
        if (i) {
          kinds += ", ";
        }
        kinds += KindEnum(m.cols[i].kind);
      }
      kinds += "}";
      L("  t.push_back({\"" + m.name + "\", " + std::to_string(m.arity) + ", " +
        kinds + ", " + (m.differential ? "true" : "false") + "});");
    }
    L("  return t;");
    L("}");
    L("");

    // Dispatch one netted batch: call each received message's entry point.
    L("template <typename Alloc>");
    L("static void DispatchBatch(Database &db, CbfLog &log, DatabaseFunctors "
      "&fn, Alloc &alloc, cbf::Netted &net) {");
    for (size_t mi = 0; mi < received_msgs.size(); ++mi) {
      const auto &m = received_msgs[mi];
      std::string tup = "Tup";
      for (unsigned i = 0; i < m.arity; ++i) {
        tup += "_";
        tup += TupSuffix(m.cols[i].kind);
      }
      const std::string idx = std::to_string(mi);
      L("  {");
      L("    auto &A = net.adds[" + idx + "];");
      L("    auto &R = net.removes[" + idx + "];");
      if (m.differential) {
        L("    if (!A.empty() || !R.empty()) {");
      } else {
        L("    if (!A.empty()) {");
      }
      L("      ::hyde::rt::Vec<" + tup + "> av(alloc);");
      if (m.differential) {
        L("      ::hyde::rt::Vec<" + tup + "> rv(alloc);");
      }
      // Fill the add Vec.
      L("      for (auto &row : A) {");
      P("        av.Add({");
      for (unsigned i = 0; i < m.arity; ++i) {
        if (i) {
          P(", ");
        }
        P(CastFromCanon(m.cols[i].kind, "row[" + std::to_string(i) + "]"));
      }
      L("});");
      L("      }");
      if (m.differential) {
        L("      for (auto &row : R) {");
        P("        rv.Add({");
        for (unsigned i = 0; i < m.arity; ++i) {
          if (i) {
            P(", ");
          }
          P(CastFromCanon(m.cols[i].kind, "row[" + std::to_string(i) + "]"));
        }
        L("});");
        L("      }");
        L("      " + m.name + "_" + std::to_string(m.arity) +
          "(db, log, fn, std::move(av), std::move(rv));");
      } else {
        L("      " + m.name + "_" + std::to_string(m.arity) +
          "(db, log, fn, std::move(av));");
      }
      L("    }");
      L("  }");
    }
    L("}");
    L("");
  }

  // ------------------------------------------------------------------
  // Per-query drive functions + a probe dispatcher.

  void EmitQueryDrivers(void) {
    for (const auto &q : queries) {
      EmitOneQueryDriver(q);
    }
    EmitProbeDispatch();
    EmitAllFreeDriver();
  }

  void EmitOneQueryDriver(const QueryDesc &q) {
    // Signature: drive_<ident>(db, log, fn, <uint64_t bound...>) -> rows.
    std::string sig = "static std::vector<std::string> drive_" + q.ident +
                      "(Database &db, CbfLog &log, DatabaseFunctors &fn";
    for (size_t i = 0; i < q.bound_pos.size(); ++i) {
      sig += ", uint64_t b" + std::to_string(i);
    }
    sig += ") {";
    L(sig);
    L("  (void) log; (void) fn;");

    // The typed bound-argument cast list.
    std::string args;
    for (size_t i = 0; i < q.bound_pos.size(); ++i) {
      if (i) {
        args += ", ";
      }
      args += CastFromCanon(q.cols[q.bound_pos[i]].kind, "b" + std::to_string(i));
    }

    // Forcing-vs-plain overload probe, resolved at the driver's compile time.
    // The template parameter D makes the `if constexpr` branches dependent so
    // the discarded branch is not instantiated against the real header.
    L("  auto invoke = [&]<typename D>(D &dbx) {");
    if (args.empty()) {
      L("    if constexpr (requires { " + q.ident + "(dbx, log, fn); }) {");
      L("      return " + q.ident + "(dbx, log, fn);");
      L("    } else {");
      L("      return " + q.ident + "(dbx);");
      L("    }");
    } else {
      L("    if constexpr (requires { " + q.ident + "(dbx, log, fn, " + args +
        "); }) {");
      L("      return " + q.ident + "(dbx, log, fn, " + args + ");");
      L("    } else {");
      L("      return " + q.ident + "(dbx, " + args + ");");
      L("    }");
    }
    L("  };");

    L("  std::vector<std::string> rows;");
    if (q.all_bound) {
      // Boolean presence query.
      L("  bool present = invoke(db);");
      L("  if (present) rows.push_back(std::string());  // 0-arity answer row");
    } else {
      L("  auto c = invoke(db);");
      // Free-column locals in declaration order.
      for (size_t i = 0; i < q.free_pos.size(); ++i) {
        L("  " + std::string(CxxType(q.cols[q.free_pos[i]].kind)) + " f" +
          std::to_string(i) + "{};");
      }
      std::string call = "  while (c.next(";
      for (size_t i = 0; i < q.free_pos.size(); ++i) {
        if (i) {
          call += ", ";
        }
        call += "f" + std::to_string(i);
      }
      call += ")) {";
      L(call);
      L("    std::string s;");
      for (size_t i = 0; i < q.free_pos.size(); ++i) {
        if (i) {
          L("    s += \" \";");
        }
        L("    s += cbf::print_val(" +
          std::string(KindEnum(q.cols[q.free_pos[i]].kind)) + ", " +
          CanonExpr(q.cols[q.free_pos[i]].kind, "f" + std::to_string(i)) + ");");
      }
      L("    rows.push_back(s);");
      L("  }");
    }
    L("  std::sort(rows.begin(), rows.end());");
    L("  return rows;");
    L("}");
    L("");
  }

  void EmitProbeDispatch(void) {
    L("static bool RunProbe(const std::string &id, const std::vector<std::string> "
      "&toks, Database &db, CbfLog &log, DatabaseFunctors &fn, std::ostream "
      "&os) {");
    bool first = true;
    for (const auto &q : queries) {
      if (q.all_free) {
        continue;  // all-free queries are enumerated, never probed
      }
      const std::string kw = first ? "  if" : "  } else if";
      first = false;
      L(kw + " (id == \"" + q.ident + "\") {");
      L("    if (toks.size() != " + std::to_string(q.bound_pos.size()) +
        ") cbf::die(\"probe '" + q.ident + "' expects " +
        std::to_string(q.bound_pos.size()) + " bound values\");");
      // Parse each bound token.
      for (size_t i = 0; i < q.bound_pos.size(); ++i) {
        L("    auto b" + std::to_string(i) + " = cbf::parse_val(" +
          std::string(KindEnum(q.cols[q.bound_pos[i]].kind)) + ", toks[" +
          std::to_string(i) + "]);");
        L("    if (!b" + std::to_string(i) + ") cbf::die(\"bad bound value in "
          "probe '" + q.ident + "'\");");
      }
      // Emit the PROBE header with echoed bound values.
      L("    os << \"QUERY " + q.ident + " PROBE\";");
      for (size_t i = 0; i < q.bound_pos.size(); ++i) {
        L("    os << \" \" << cbf::print_val(" +
          std::string(KindEnum(q.cols[q.bound_pos[i]].kind)) + ", *b" +
          std::to_string(i) + ");");
      }
      L("    os << \"\\n\";");
      std::string call = "    auto rows = drive_" + q.ident + "(db, log, fn";
      for (size_t i = 0; i < q.bound_pos.size(); ++i) {
        call += ", *b" + std::to_string(i);
      }
      call += ");";
      L(call);
      L("    for (auto &r : rows) os << r << \"\\n\";");
      L("    return true;");
    }
    if (!first) {
      L("  }");
    }
    L("  return false;");
    L("}");
    L("");
  }

  void EmitAllFreeDriver(void) {
    L("static void DriveAllFree(Database &db, CbfLog &log, DatabaseFunctors "
      "&fn, std::ostream &os) {");
    for (const auto &q : queries) {
      if (!q.all_free) {
        continue;
      }
      L("  os << \"QUERY " + q.ident + "\\n\";");
      L("  { auto rows = drive_" + q.ident +
        "(db, log, fn); for (auto &r : rows) os << r << \"\\n\"; }");
    }
    L("}");
    L("");
  }

  // ------------------------------------------------------------------
  void EmitMain(void) {
    L("int main(int argc, char **argv) {");
    L("  if (argc < 2) {");
    L("    std::fprintf(stderr, \"usage: %s <case.batches> "
      "[<case.probes>]\\n\", argv[0]);");
    L("    return 2;");
    L("  }");
    L("  const auto allocator = hyde::rt::MallocAllocator();");
    L("  DatabaseFunctors functors;");
    L("  CbfLog log;");
    L("  Database db(allocator);");
    L("  init(db, log, functors);");
    L("");
    L("  std::cout << \"REFINTERP " + case_name + " netting=oq3\\n\";");
    L("");
    L("  auto msgs = BuildMsgTable();");
    L("  auto batches = cbf::parse_batches(argv[1], msgs);");
    L("  int epoch = 0;");
    L("  for (auto &blk : batches) {");
    L("    cbf::Netted net = cbf::oq3(blk, msgs.size());");
    L("    auto prev = log.final_sets;");
    L("    auto alloc = allocator;");
    L("    DispatchBatch(db, log, functors, alloc, net);");
    L("    std::cout << \"EPOCH \" << epoch++ << \"\\n\";");
    L("    std::vector<std::string> dl;");
    L("    for (auto &kv : log.final_sets)");
    L("      for (auto &row : kv.second)");
    L("        if (!prev[kv.first].count(row))");
    L("          dl.push_back(\"+ \" + kv.first + (row.empty() ? \"\" : \" \" + row));");
    L("    for (auto &kv : prev)");
    L("      for (auto &row : kv.second)");
    L("        if (!log.final_sets[kv.first].count(row))");
    L("          dl.push_back(\"- \" + kv.first + (row.empty() ? \"\" : \" \" + row));");
    L("    std::sort(dl.begin(), dl.end());");
    L("    for (auto &l : dl) std::cout << l << \"\\n\";");
    L("  }");
    L("");
    L("  std::cout << \"FINAL\\n\";");
    L("  {");
    L("    std::vector<std::string> mem;");
    L("    for (auto &kv : log.final_sets)");
    L("      for (auto &row : kv.second)");
    L("        mem.push_back(row.empty() ? kv.first : kv.first + \" \" + row);");
    L("    std::sort(mem.begin(), mem.end());");
    L("    for (auto &l : mem) std::cout << l << \"\\n\";");
    L("  }");
    L("");
    L("  DriveAllFree(db, log, functors, std::cout);");
    L("");
    L("  if (argc >= 3) {");
    L("    std::ifstream pin(argv[2]);");
    L("    if (!pin) cbf::die(std::string(\"cannot open probes '\") + argv[2] "
      "+ \"'\");");
    L("    std::string line;");
    L("    while (std::getline(pin, line)) {");
    L("      if (auto pos = line.find('#'); pos != std::string::npos) "
      "line.erase(pos);");
    L("      std::istringstream ls(line);");
    L("      std::vector<std::string> toks;");
    L("      for (std::string t; ls >> t;) toks.push_back(t);");
    L("      if (toks.empty()) continue;");
    L("      const std::string id = toks[0];");
    L("      std::vector<std::string> bound(toks.begin() + 1, toks.end());");
    L("      if (!RunProbe(id, bound, db, log, functors, std::cout))");
    L("        cbf::die(\"unknown probe query '\" + id + \"'\");");
    L("    }");
    L("  }");
    L("  return 0;");
    L("}");
  }
};

}  // namespace

int main(int argc, char *argv[]) {
  const char *dr_path = nullptr;
  const char *out_path = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (!std::strcmp(argv[i], "-o")) {
      if (i + 1 >= argc) {
        std::cerr << "drlojekyll-refharness: -o requires an argument\n";
        return EXIT_FAILURE;
      }
      out_path = argv[++i];
    } else if (!dr_path) {
      dr_path = argv[i];
    } else {
      std::cerr << "drlojekyll-refharness: unexpected argument '" << argv[i]
                << "'\n";
      return EXIT_FAILURE;
    }
  }
  if (!dr_path || !out_path) {
    std::cerr << "usage: drlojekyll-refharness <case.dr> -o <out.cpp>\n";
    return EXIT_FAILURE;
  }

  Emitter emitter;
  return emitter.Run(dr_path, out_path);
}
