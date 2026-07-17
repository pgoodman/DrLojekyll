// Copyright 2026, Trail of Bits. All rights reserved.
//
// The C++ backend. Emits a header-only implementation of a Datalog
// program: named row structs, enums, a sealed `Database` state struct
// whose driver surface is hidden-friend entry points (epoch 0 = an
// explicit `init(db, log, functors)`; messages and forced queries deduce
// the driver's Log/Functors types statically — no virtual dispatch), and
// namespace-scope detail functions carrying the flow-procedure bodies,
// whose parameter lists name exactly the state they read and write. The
// source file is an anchor translation unit (banner + include) so
// one-TU-per-database build systems keep working. Storage is provided by
// the small concrete runtime (`hyde::rt::Table`, `hyde::rt::Index`,
// `hyde::rt::Vec`) and all allocation goes through an explicit
// `hyde::rt::Allocator`.

#include <drlojekyll/CodeGen/CodeGen.h>
#include <drlojekyll/ControlFlow/Program.h>
#include <drlojekyll/DataFlow/Query.h>
#include <drlojekyll/Display/Format.h>
#include <drlojekyll/Lex/Format.h>
#include <drlojekyll/Parse/Format.h>
#include <drlojekyll/Parse/Parse.h>

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hyde {
namespace cxx {
namespace {

[[noreturn]] void Unsupported(const char *what) {
  std::fprintf(stderr, "error: C++ backend cannot emit %s\n", what);
  std::abort();
}

// Turns arbitrary text into a valid C++ identifier fragment.
std::string Sanitize(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for (char c : text) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_';
    out.push_back(ok ? c : '_');
  }
  if (out.empty() || (out[0] >= '0' && out[0] <= '9')) {
    out.insert(out.begin(), '_');
  }
  return out;
}

// The C++ spelling of a Datalog type.
std::string TypeName(ParsedModule module, TypeLoc kind) {
  switch (kind.UnderlyingKind()) {
    case TypeKind::kBoolean: return "bool";
    case TypeKind::kSigned8: return "int8_t";
    case TypeKind::kSigned16: return "int16_t";
    case TypeKind::kSigned32: return "int32_t";
    case TypeKind::kSigned64: return "int64_t";
    case TypeKind::kUnsigned8: return "uint8_t";
    case TypeKind::kUnsigned16: return "uint16_t";
    case TypeKind::kUnsigned32: return "uint32_t";
    case TypeKind::kUnsigned64: return "uint64_t";
    case TypeKind::kFloat: return "float";
    case TypeKind::kDouble: return "double";
    case TypeKind::kForeignType:
      if (auto type = module.ForeignType(kind); type) {
        if (type->IsEnum()) {
          return std::string(type->NameAsString());
        } else if (auto code = type->CodeToInline(Language::kCxx)) {
          return std::string(*code);
        }
      }
      Unsupported("a foreign type with no C++ representation");
    default: Unsupported("an invalid type");
  }
}

// A short type tag used to mangle tuple-struct names.
std::string TypeTag(ParsedModule module, TypeLoc kind) {
  switch (kind.UnderlyingKind()) {
    case TypeKind::kBoolean: return "b";
    case TypeKind::kSigned8: return "i8";
    case TypeKind::kSigned16: return "i16";
    case TypeKind::kSigned32: return "i32";
    case TypeKind::kSigned64: return "i64";
    case TypeKind::kUnsigned8: return "u8";
    case TypeKind::kUnsigned16: return "u16";
    case TypeKind::kUnsigned32: return "u32";
    case TypeKind::kUnsigned64: return "u64";
    case TypeKind::kFloat: return "f32";
    case TypeKind::kDouble: return "f64";
    default: return Sanitize(TypeName(module, kind));
  }
}

const char *OperatorString(ComparisonOperator op) {
  switch (op) {
    case ComparisonOperator::kEqual: return "==";
    case ComparisonOperator::kNotEqual: return "!=";
    case ComparisonOperator::kLessThan: return "<";
    case ComparisonOperator::kGreaterThan: return ">";
  }
  Unsupported("an unknown comparison operator");
}

// Recover a relation name for a table from the insert views that feed it.
std::optional<ParsedDeclaration> TableDecl(DataTable table) {
  for (QueryView view : table.Views()) {
    if (view.IsInsert()) {
      return QueryInsert::From(view).Declaration();
    }
  }
  return std::nullopt;
}

class Generator {
 public:
  Generator(const Program &program_, OutputStream &hh_, OutputStream &cc_,
            std::string_view header_name_)
      : program(program_),
        module(program_.ParsedModule()),
        hh(hh_),
        cc(cc_),
        header_name(header_name_) {}

  void Run(void);

 private:
  // ---------------------------------------------------------------------
  // Naming.

  // Stringify anything the OutputStream can print (tokens, names).
  template <typename T>
  std::string ToString(const T &val) {
    std::stringstream ss;
    do {
      OutputStream ss_os(hh.display_manager, ss);
      ss_os << val;
    } while (false);
    return ss.str();
  }

  std::string VarName(DataVariable var) {
    switch (var.DefiningRole()) {
      case VariableRole::kConstantZero: return "0";
      case VariableRole::kConstantFalse: return "false";
      case VariableRole::kConstantTrue: return "true";
      default: break;
    }
    if (var.IsConstant()) {
      return ConstantExpr(var);
    }
    if (var.IsGlobal()) {
      return "g" + std::to_string(var.Id());
    }
    return "v" + std::to_string(var.Id());
  }

  // The inline expression for a constant variable: an enumerator reference,
  // a literal spelling, or a zero default.
  std::string ConstantExpr(DataVariable var) {
    auto val = var.Value();
    if (val && val->IsTag()) {
      return std::to_string(QueryTag::From(*val).Value());
    }
    if (val) {
      if (std::optional<ParsedLiteral> lit = val->Literal()) {
        if (lit->IsEnumerator()) {
          auto type = ParsedForeignType::Of(*lit);
          auto enumerator = ParsedForeignConstant::From(*lit);
          return std::string(type->NameAsString()) + "::" +
                 std::string(enumerator.NameAsString());
        }
        if (auto spelling = lit->Spelling(Language::kCxx); spelling) {
          return std::string(*spelling);
        }
      }
    }
    if (var.Type().UnderlyingKind() == TypeKind::kBoolean) {
      return "false";
    }
    return TypeName(module, var.Type()) + "{}";
  }

  std::string ProcName(ProgramProcedure proc) {
    switch (proc.Kind()) {
      case ProcedureKind::kInitializer:
        return "init_" + std::to_string(proc.Id());
      case ProcedureKind::kPrimaryDataFlowFunc:
        return "flow_" + std::to_string(proc.Id());
      case ProcedureKind::kMessageHandler:
        return Sanitize(ToString(proc.Message()->Name())) + "_" +
               std::to_string(proc.Message()->Arity());
      case ProcedureKind::kQueryMessageInjector:
        return "inject_" + std::to_string(proc.Id());
      default: return "proc_" + std::to_string(proc.Id());
    }
  }

  // The namespace-scope detail function carrying a procedure's body. A
  // message handler's public name is claimed by its driver-facing entry
  // point, so its detail twin is suffixed; every other kind's ProcName
  // already carries the procedure id and serves as-is.
  std::string DetailName(ProgramProcedure proc) {
    if (proc.Kind() == ProcedureKind::kMessageHandler) {
      return ProcName(proc) + "_detail";
    }
    return ProcName(proc);
  }

  // Vectors are passed by value into dataflow entry points and moved down;
  // all other procedure kinds take them by reference.
  static bool TakesVectorsByValue(ProgramProcedure proc) {
    switch (proc.Kind()) {
      case ProcedureKind::kMessageHandler:
      case ProcedureKind::kEntryDataFlowFunc:
      case ProcedureKind::kPrimaryDataFlowFunc: return true;
      default: return false;
    }
  }

  std::string VecName(DataVector vec) {
    return "vec" + std::to_string(vec.Id());
  }

  // ---------------------------------------------------------------------
  // Layout pre-pass.

  void ComputeNames(void);
  void CollectVectorShapes(void);
  void RegisterShape(const std::vector<TypeLoc> &types);
  std::string ShapeName(const std::vector<TypeLoc> &types);
  std::string VecType(DataVector vec);
  void WalkRegion(ProgramRegion region);

  // ---------------------------------------------------------------------
  // Per-procedure effects: the exact state a procedure's body (and,
  // transitively, its callees') reads and writes. The detail function's
  // parameter list is rendered from this set, so the signature IS the
  // read/write set.

  struct ProcEffects {
    std::unordered_set<unsigned> tables;   // DataTable ids
    std::unordered_set<unsigned> indexes;  // DataIndex ids (member-backed)
    std::unordered_set<unsigned> globals;  // global DataVariable ids
    std::unordered_set<unsigned> statecells;  // R3: StateCell store ids
    bool uses_log{false};       // publishes (publish region / commit sweep)
    bool uses_functors{false};  // calls a non-inline functor
  };

  const ProcEffects &EffectsOf(ProgramProcedure proc);
  void CollectEffects(ProgramRegion region, ProcEffects &out);

  // `template <...>` header line for a detail function, or "".
  static std::string DetailTemplateHeader(const ProcEffects &fx) {
    if (fx.uses_log && fx.uses_functors) {
      return "template <typename Log, typename Functors>\n";
    }
    if (fx.uses_log) {
      return "template <typename Log>\n";
    }
    if (fx.uses_functors) {
      return "template <typename Functors>\n";
    }
    return "";
  }

  // The state half of a detail function's parameter list, rendered from
  // its effects: allocator, deduced log/functors, then tables with their
  // indexes and globals in member-declaration order.
  std::string DetailStateParams(ProgramProcedure proc);

  // The matching argument list at a call site. `prefix` qualifies the
  // state names ("" where they are in scope as parameters or members,
  // "db." inside a hidden friend).
  std::string DetailStateArgs(ProgramProcedure proc, const char *prefix);

  // R3: the fully-qualified `StateCellStore<Key_<id>, Algebra_<id>>` type for
  // one state cell (member declaration + detail reference-param type).
  std::string StateCellStoreType(const ProgramStateCellInfo &cell);

  // Vector/scalar parameter tail shared by declarations and definitions.
  std::string ProcValueParams(ProgramProcedure proc);

  void EmitDetailDecls(OutputStream &os);

  // ---------------------------------------------------------------------
  // Header emission.

  void EmitInlines(OutputStream &os, const char *stage);
  void EmitEnums(void);
  void EmitShapeStructs(void);
  void EmitHashStruct(const std::string &name,
                      const std::vector<std::pair<std::string, std::string>>
                          &typed_fields);
  void EmitRowStructs(void);
  void EmitStateCellStructs(void);  // R3
  void EmitFunctorsDecl(void);
  void EmitLogDecl(void);
  void EmitDatabaseDecl(void);
  void EmitQueryFriends(const ProgramQuery &spec);

  // ---------------------------------------------------------------------
  // Procedure emission (the detail definitions; header-resident).

  void EmitProcedure(ProgramProcedure proc);

  void EmitRegion(ProgramRegion region);
  void EmitOptional(std::optional<ProgramRegion> region) {
    if (region) {
      EmitRegion(*region);
    }
  }
  void EmitComment(ProgramRegion region) {
    if (!region.Comment().empty()) {
      cc << cc.Indent() << "// " << region.Comment() << "\n";
    }
  }

  void EmitCall(ProgramCallRegion region);
  void EmitUpdateCount(ProgramUpdateCountRegion region);
  void EmitCheckMember(ProgramCheckMemberRegion region);
  void EmitCommitSweep(ProgramCommitSweepRegion region);
  void EmitGroupUpdate(ProgramGroupUpdateRegion region);  // R3
  void EmitClaim(ProgramClaimRegion region);
  void EmitRetire(ProgramRetireRegion region);
  void EmitNetBatch(ProgramNetBatchRegion region);
  void EmitGenerate(ProgramGenerateRegion region);
  void EmitInduction(ProgramInductionRegion region);
  void EmitJoin(ProgramTableJoinRegion region);
  void EmitProduct(ProgramTableProductRegion region);
  void EmitScan(ProgramTableScanRegion region);
  void EmitCompare(ProgramTupleCompareRegion region);

  // Emits the per-index maintenance that follows a fresh row insertion.
  void EmitIndexAdds(DataTable table, const std::string &ins,
                     const std::vector<std::string> &tuple_exprs);

  // The braced row expression `{a, b, c}`.
  static std::string RowExpr(const std::vector<std::string> &exprs) {
    std::string out = "{";
    auto sep = "";
    for (const auto &e : exprs) {
      out += sep;
      out += e;
      sep = ", ";
    }
    out += "}";
    return out;
  }

  static std::string JoinExprs(const std::vector<std::string> &exprs,
                               const char *sep_text) {
    std::string out;
    auto sep = "";
    for (const auto &e : exprs) {
      out += sep;
      out += e;
      sep = sep_text;
    }
    return out;
  }

  template <typename Range>
  std::vector<std::string> VarExprs(Range vars) {
    std::vector<std::string> out;
    for (DataVariable var : vars) {
      out.push_back(VarName(var));
    }
    return out;
  }

  // Whether a region's control flow always ends in a `return`.
  static bool EndsWithReturn(ProgramRegion region) {
    if (region.IsReturn()) {
      return true;
    }
    if (region.IsSeries()) {
      auto regions = ProgramSeriesRegion::From(region).Regions();
      if (regions.empty()) {
        return false;
      }
      return EndsWithReturn(regions[regions.size() - 1u]);
    }
    return false;
  }

  const Program &program;
  const ParsedModule module;
  OutputStream &hh;
  OutputStream &cc;
  const std::string header_name;

  std::string ns_name;

  // Table id -> member name / row struct name / per-column field names.
  std::unordered_map<unsigned, std::string> table_member;
  std::unordered_map<unsigned, std::string> row_type;
  std::unordered_map<unsigned, std::vector<std::string>> col_field;

  // Index id -> member name / key struct name. Covering indexes (keys over
  // every column) get neither: the table's own hash lookup serves them.
  std::unordered_map<unsigned, std::string> index_member;
  std::unordered_map<unsigned, std::string> key_type;

  // Tuple shape (mangled type list) -> (struct name, column types).
  std::map<std::string, std::pair<std::string, std::vector<TypeLoc>>> shapes;

  // Procedure id -> transitive effects (memoized; the call graph is a
  // DAG — recursion lives inside INDUCTION regions, never in calls).
  std::unordered_map<unsigned, ProcEffects> proc_effects;
  std::unordered_set<unsigned> effects_in_progress;

  unsigned next_ins_id{0};

  // Row-binding scope stack. While a join-side or table-scan body is being
  // emitted, one entry records (table id, the scanned row's bound variable
  // names in physical column order, the cursor variable holding that row's
  // id). A CHECKMEMBER whose (table, tuple exprs) exactly equals a live
  // entry reads its membership predicate directly on the cursor id: the
  // value-keyed re-Find is redundant because FindOrAdd is the runtime's
  // only writer (value -> id is a function) and the tuple was bound
  // verbatim from RowAt(cursor). Preconditions this match rests on, both
  // re-verified at the epoch seed review: a CHECKMEMBER's tuple order
  // equals physical column order (same-DataModel views are column-position
  // compatible), and variable names are globally unique (two live scans of
  // one table — a self-join — cannot false-match each other's entry).
  // Entries are pushed ONLY by EmitJoin and EmitScan and only for dense
  // full-column bindings; EmitProduct must never push (its staging loop
  // rebinds the scans' variable names after the cursors are dead), and
  // vector-loop-driven reads never match (their variables are bound from a
  // Vec, not a scan). Mismatches of any kind keep the Find: conservative.
  struct RowScopeEntry {
    unsigned table_id;
    std::vector<std::string> var_names;
    std::string cursor;
  };
  std::vector<RowScopeEntry> row_scope;

  // Innermost live binding for (table, exprs), or null.
  const RowScopeEntry *FindRowScope(unsigned table_id,
                                    const std::vector<std::string> &exprs) {
    for (auto it = row_scope.rbegin(); it != row_scope.rend(); ++it) {
      if (it->table_id == table_id && it->var_names == exprs) {
        return &*it;
      }
    }
    return nullptr;
  }
};

void Generator::ComputeNames(void) {
  if (auto db_name = module.DatabaseName()) {
    ns_name = db_name->NamespaceName(Language::kCxx);
  }

  for (DataTable table : program.Tables()) {
    const auto id = table.Id();

    std::string base = "table";
    if (auto decl = TableDecl(table)) {
      base = Sanitize(ToString(decl->Name()));
    }
    table_member.emplace(id, base + "_" + std::to_string(id));
    row_type.emplace(id, "Row" + std::to_string(id));

    // Column field names: prefer a parsed name, fall back to c<position>,
    // dedupe within the table.
    std::unordered_set<std::string> used_fields;
    std::vector<std::string> fields;
    for (DataColumn col : table.Columns()) {
      std::string field;
      if (const auto &names = col.PossibleNames(); !names.empty()) {
        field = Sanitize(ToString(names[0]));
        for (auto &c : field) {
          c = static_cast<char>(std::tolower(c));
        }
      }
      if (field.empty() || field == "_" || used_fields.contains(field)) {
        field = "c" + std::to_string(col.Index());
      }
      used_fields.insert(field);
      fields.push_back(std::move(field));
    }
    col_field.emplace(id, std::move(fields));

    for (DataIndex index : table.Indices()) {
      if (index.ValueColumns().empty()) {
        continue;
      }
      index_member.emplace(index.Id(), "idx_" + std::to_string(index.Id()));
      key_type.emplace(index.Id(), "Key" + std::to_string(index.Id()));
    }
  }
}

void Generator::RegisterShape(const std::vector<TypeLoc> &types) {
  std::string mangled;
  for (TypeLoc t : types) {
    mangled += "_";
    mangled += TypeTag(module, t);
  }
  if (!shapes.contains(mangled)) {
    shapes.emplace(mangled, std::make_pair("Tup" + mangled, types));
  }
}

std::string Generator::ShapeName(const std::vector<TypeLoc> &types) {
  std::string mangled;
  for (TypeLoc t : types) {
    mangled += "_";
    mangled += TypeTag(module, t);
  }
  const auto it = shapes.find(mangled);
  assert(it != shapes.end());
  return it->second.first;
}

std::string Generator::VecType(DataVector vec) {
  return ShapeName(vec.ColumnTypes());
}

// Collect every tuple shape the program needs: procedure parameter and local
// vectors, plus product staging vectors.
void Generator::CollectVectorShapes(void) {
  for (ProgramProcedure proc : program.Procedures()) {
    for (DataVector vec : proc.VectorParameters()) {
      RegisterShape(vec.ColumnTypes());
    }
    for (DataVector vec : proc.DefinedVectors()) {
      RegisterShape(vec.ColumnTypes());
    }
    WalkRegion(proc.Body());
  }
}

void Generator::WalkRegion(ProgramRegion region) {
  if (region.IsSeries()) {
    for (auto sub : ProgramSeriesRegion::From(region).Regions()) {
      WalkRegion(sub);
    }
  } else if (region.IsParallel()) {
    for (auto sub : ProgramParallelRegion::From(region).Regions()) {
      WalkRegion(sub);
    }
  } else if (region.IsInduction()) {
    auto induction = ProgramInductionRegion::From(region);
    if (auto init = induction.Initializer()) {
      WalkRegion(*init);
    }
    WalkRegion(induction.FixpointLoop());
    if (auto out = induction.Output()) {
      WalkRegion(*out);
    }
  } else if (region.IsTableProduct()) {
    auto product = ProgramTableProductRegion::From(region);
    std::vector<TypeLoc> types;
    const auto num_tables = product.Tables().size();
    for (auto i = 0u; i < num_tables; ++i) {
      for (DataVariable var : product.OutputVariables(i)) {
        types.push_back(var.Type());
      }
    }
    RegisterShape(types);
    if (auto body = product.Body()) {
      WalkRegion(*body);
    }
  } else if (region.IsVectorLoop()) {
    if (auto body = ProgramVectorLoopRegion::From(region).Body()) {
      WalkRegion(*body);
    }
  } else if (region.IsLetBinding()) {
    if (auto body = ProgramLetBindingRegion::From(region).Body()) {
      WalkRegion(*body);
    }
  } else if (region.IsUpdateCount()) {
    auto change = ProgramUpdateCountRegion::From(region);
    if (auto body = change.Body()) {
      WalkRegion(*body);
    }
  } else if (region.IsClaim()) {
    auto claim = ProgramClaimRegion::From(region);
    if (auto body = claim.Body()) {
      WalkRegion(*body);
    }
  } else if (region.IsCheckMember()) {
    auto check = ProgramCheckMemberRegion::From(region);
    if (auto body = check.IfPresent()) {
      WalkRegion(*body);
    }
    if (auto body = check.IfAbsent()) {
      WalkRegion(*body);
    }
  } else if (region.IsTableJoin()) {
    auto join = ProgramTableJoinRegion::From(region);
    if (auto body = join.Body()) {
      WalkRegion(*body);
    }
    if (auto body = join.AddedBody()) {
      WalkRegion(*body);
    }
    if (auto body = join.RemovedBody()) {
      WalkRegion(*body);
    }
  } else if (region.IsTableScan()) {
    if (auto body = ProgramTableScanRegion::From(region).Body()) {
      WalkRegion(*body);
    }
  } else if (region.IsTupleCompare()) {
    auto cmp = ProgramTupleCompareRegion::From(region);
    if (auto body = cmp.BodyIfTrue()) {
      WalkRegion(*body);
    }
    if (auto body = cmp.BodyIfFalse()) {
      WalkRegion(*body);
    }
  } else if (region.IsGenerate()) {
    auto gen = ProgramGenerateRegion::From(region);
    if (auto body = gen.BodyIfResults()) {
      WalkRegion(*body);
    }
    if (auto body = gen.BodyIfEmpty()) {
      WalkRegion(*body);
    }
  } else if (region.IsCall()) {
    auto call = ProgramCallRegion::From(region);
    if (auto body = call.BodyIfTrue()) {
      WalkRegion(*body);
    }
    if (auto body = call.BodyIfFalse()) {
      WalkRegion(*body);
    }
  } else if (region.IsTestAndSet()) {
    if (auto body = ProgramTestAndSetRegion::From(region).Body()) {
      WalkRegion(*body);
    }
  } else if (region.IsWorkerId()) {
    if (auto body = ProgramWorkerIdRegion::From(region).Body()) {
      WalkRegion(*body);
    }
  }
  // Leaf regions (return, publish, vector ops) have no nested bodies.
}

// -----------------------------------------------------------------------
// Per-procedure effects.

const Generator::ProcEffects &Generator::EffectsOf(ProgramProcedure proc) {
  const auto id = proc.Id();
  if (auto it = proc_effects.find(id); it != proc_effects.end()) {
    return it->second;
  }

  // The procedure call graph is a DAG (recursion lives inside INDUCTION
  // regions, never in calls); a cycle would make explicit parameter sets
  // ill-founded.
  assert(!effects_in_progress.contains(id));
  effects_in_progress.insert(id);
  ProcEffects out;
  CollectEffects(proc.Body(), out);
  effects_in_progress.erase(id);
  return proc_effects.emplace(id, std::move(out)).first->second;
}

void Generator::CollectEffects(ProgramRegion region, ProcEffects &out) {
  // A global variable renders as a `g<id>` reference (VarName); constants
  // render inline and need no parameter.
  const auto add_var = [&](DataVariable var) {
    switch (var.DefiningRole()) {
      case VariableRole::kConstantZero:
      case VariableRole::kConstantFalse:
      case VariableRole::kConstantTrue: return;
      default: break;
    }
    if (!var.IsConstant() && var.IsGlobal()) {
      out.globals.insert(var.Id());
    }
  };
  const auto add_vars = [&](auto range) {
    for (DataVariable var : range) {
      add_var(var);
    }
  };

  // A written table's member-backed indexes are maintained alongside the
  // insertion (EmitIndexAdds), whether or not a region names them.
  const auto add_written_table = [&](DataTable table) {
    out.tables.insert(table.Id());
    for (DataIndex index : table.Indices()) {
      if (index_member.contains(index.Id())) {
        out.indexes.insert(index.Id());
      }
    }
  };

  const auto recurse = [&](std::optional<ProgramRegion> sub) {
    if (sub) {
      CollectEffects(*sub, out);
    }
  };

  if (region.IsSeries()) {
    for (auto sub : ProgramSeriesRegion::From(region).Regions()) {
      CollectEffects(sub, out);
    }
  } else if (region.IsParallel()) {
    for (auto sub : ProgramParallelRegion::From(region).Regions()) {
      CollectEffects(sub, out);
    }
  } else if (region.IsLetBinding()) {
    auto let = ProgramLetBindingRegion::From(region);
    add_vars(let.UsedVariables());
    recurse(let.Body());
  } else if (region.IsVectorLoop()) {
    recurse(ProgramVectorLoopRegion::From(region).Body());
  } else if (region.IsVectorAppend()) {
    add_vars(ProgramVectorAppendRegion::From(region).TupleVariables());
  } else if (region.IsUpdateCount()) {
    auto change = ProgramUpdateCountRegion::From(region);
    add_written_table(change.Table());
    add_vars(change.TupleVariables());
    recurse(change.Body());
  } else if (region.IsCheckMember()) {
    auto check = ProgramCheckMemberRegion::From(region);
    out.tables.insert(check.Table().Id());
    add_vars(check.TupleVariables());
    recurse(check.IfPresent());
    recurse(check.IfAbsent());
  } else if (region.IsCommitSweep()) {
    auto sweep = ProgramCommitSweepRegion::From(region);
    out.tables.insert(sweep.Table().Id());
    if (sweep.Message()) {
      out.uses_log = true;
    }
    if (auto id = sweep.SealStateCellId()) {
      out.statecells.insert(*id);
    }
  } else if (region.IsGroupUpdate()) {
    auto gu = ProgramGroupUpdateRegion::From(region);
    out.tables.insert(gu.AggTable().Id());
    out.statecells.insert(gu.StateCellId());
  } else if (region.IsClaim()) {
    auto claim = ProgramClaimRegion::From(region);
    out.tables.insert(claim.Table().Id());
    add_vars(claim.TupleVariables());
    recurse(claim.Body());
  } else if (region.IsRetire()) {
    auto retire = ProgramRetireRegion::From(region);
    out.tables.insert(retire.Table().Id());
    add_vars(retire.TupleVariables());
  } else if (region.IsTableJoin()) {
    auto join = ProgramTableJoinRegion::From(region);
    const auto tables = join.Tables();
    for (auto i = 0u; i < tables.size(); ++i) {
      out.tables.insert(tables[i].Id());
      if (auto index = join.Index(i);
          index && index_member.contains(index->Id())) {
        out.indexes.insert(index->Id());
      }
    }
    recurse(join.Body());
    recurse(join.AddedBody());
    recurse(join.RemovedBody());
  } else if (region.IsTableProduct()) {
    auto product = ProgramTableProductRegion::From(region);
    for (DataTable table : product.Tables()) {
      out.tables.insert(table.Id());
    }
    recurse(product.Body());
  } else if (region.IsTableScan()) {
    auto scan = ProgramTableScanRegion::From(region);
    out.tables.insert(scan.Table().Id());
    add_vars(scan.InputVariables());
    if (auto index = scan.Index();
        index && index_member.contains(index->Id()) &&
        scan.InputVariables().size() == index->KeyColumns().size()) {
      out.indexes.insert(index->Id());
    }
    recurse(scan.Body());
  } else if (region.IsTupleCompare()) {
    auto cmp = ProgramTupleCompareRegion::From(region);
    add_vars(cmp.LHS());
    add_vars(cmp.RHS());
    recurse(cmp.BodyIfTrue());
    recurse(cmp.BodyIfFalse());
  } else if (region.IsGenerate()) {
    auto gen = ProgramGenerateRegion::From(region);
    if (!gen.Functor().InlineName(Language::kCxx)) {
      out.uses_functors = true;
    }
    add_vars(gen.InputVariables());
    recurse(gen.BodyIfResults());
    recurse(gen.BodyIfEmpty());
  } else if (region.IsCall()) {
    auto call = ProgramCallRegion::From(region);
    add_vars(call.VariableArguments());
    const auto &callee = EffectsOf(call.CalledProcedure());
    out.tables.insert(callee.tables.begin(), callee.tables.end());
    out.indexes.insert(callee.indexes.begin(), callee.indexes.end());
    out.globals.insert(callee.globals.begin(), callee.globals.end());
    out.statecells.insert(callee.statecells.begin(), callee.statecells.end());
    out.uses_log |= callee.uses_log;
    out.uses_functors |= callee.uses_functors;
    recurse(call.BodyIfTrue());
    recurse(call.BodyIfFalse());
  } else if (region.IsPublish()) {
    auto publish = ProgramPublishRegion::From(region);
    out.uses_log = true;
    add_vars(publish.VariableArguments());
  } else if (region.IsTestAndSet()) {
    auto tas = ProgramTestAndSetRegion::From(region);
    add_var(tas.Accumulator());
    recurse(tas.Body());
  } else if (region.IsWorkerId()) {
    recurse(ProgramWorkerIdRegion::From(region).Body());
  } else if (region.IsInduction()) {
    auto induction = ProgramInductionRegion::From(region);
    recurse(induction.Initializer());
    CollectEffects(induction.FixpointLoop(), out);
    recurse(induction.Output());
  }
  // Remaining leaves (return, vector clear/unique/swap, net-batch) touch
  // vectors and constants only.
}

std::string Generator::DetailStateParams(ProgramProcedure proc) {
  const auto &fx = EffectsOf(proc);
  std::vector<std::string> parts;
  parts.push_back("::hyde::rt::Allocator &allocator");
  if (fx.uses_log) {
    parts.push_back("Log &log");
  }
  if (fx.uses_functors) {
    parts.push_back("Functors &functors");
  }
  auto num_indexes = 0u;
  for (DataTable table : program.Tables()) {
    if (!fx.tables.contains(table.Id())) {
      continue;
    }
    parts.push_back(std::string("::hyde::rt::") +
                    (table.IsDifferential() ? "DiffTable<" : "Table<") +
                    row_type[table.Id()] + "> &" + table_member[table.Id()]);
    for (DataIndex index : table.Indices()) {
      if (fx.indexes.contains(index.Id())) {
        parts.push_back("::hyde::rt::Index<" + key_type[index.Id()] + "> &" +
                        index_member[index.Id()]);
        ++num_indexes;
      }
    }
  }

  // Every used index rides with its table (index reads imply table reads).
  assert(num_indexes == fx.indexes.size());
  (void) num_indexes;

  for (DataVariable var : program.GlobalVariables()) {
    if (fx.globals.contains(var.Id())) {
      parts.push_back(TypeName(module, var.Type()) + " &" + VarName(var));
    }
  }
  // R3: StateCell stores used by this procedure ride as reference params.
  for (const ProgramStateCellInfo &cell : program.StateCells()) {
    if (fx.statecells.contains(cell.Id())) {
      parts.push_back(StateCellStoreType(cell) + " &statecell_" +
                      std::to_string(cell.Id()));
    }
  }
  return JoinExprs(parts, ", ");
}

std::string Generator::StateCellStoreType(const ProgramStateCellInfo &cell) {
  const auto id = std::to_string(cell.Id());
  const std::string algebra = cell.IsInvertible()
      ? "::hyde::rt::Invertible<Reduce_" + id + ">"
      : "::hyde::rt::Recompute<Reduce_" + id + ">";
  return "::hyde::rt::StateCellStore<Key_" + id + ", " + algebra + ">";
}

std::string Generator::DetailStateArgs(ProgramProcedure proc,
                                       const char *prefix) {
  const auto &fx = EffectsOf(proc);
  std::vector<std::string> parts;
  parts.push_back(std::string(prefix) + "allocator");
  if (fx.uses_log) {
    parts.push_back("log");
  }
  if (fx.uses_functors) {
    parts.push_back("functors");
  }
  for (DataTable table : program.Tables()) {
    if (!fx.tables.contains(table.Id())) {
      continue;
    }
    parts.push_back(std::string(prefix) + table_member[table.Id()]);
    for (DataIndex index : table.Indices()) {
      if (fx.indexes.contains(index.Id())) {
        parts.push_back(std::string(prefix) + index_member[index.Id()]);
      }
    }
  }
  for (DataVariable var : program.GlobalVariables()) {
    if (fx.globals.contains(var.Id())) {
      parts.push_back(std::string(prefix) + VarName(var));
    }
  }
  // R3: forward the used StateCell stores by name.
  for (const ProgramStateCellInfo &cell : program.StateCells()) {
    if (fx.statecells.contains(cell.Id())) {
      parts.push_back(std::string(prefix) + "statecell_" +
                      std::to_string(cell.Id()));
    }
  }
  return JoinExprs(parts, ", ");
}

std::string Generator::ProcValueParams(ProgramProcedure proc) {
  std::vector<std::string> parts;
  const auto by_value = TakesVectorsByValue(proc);
  for (DataVector vec : proc.VectorParameters()) {
    parts.push_back("::hyde::rt::Vec<" + VecType(vec) + "> " +
                    (by_value ? "" : "&") + VecName(vec));
  }
  for (DataVariable var : proc.VariableParameters()) {
    parts.push_back(TypeName(module, var.Type()) + " " + VarName(var));
  }
  return JoinExprs(parts, ", ");
}

// Forward declarations for every detail function, ahead of any caller:
// calls with no dependent argument bind at template-definition time, so
// these must precede the class and its hidden friends.
void Generator::EmitDetailDecls(OutputStream &os) {
  os << "// Internal flow procedures. Not driver-facing. Each signature\n"
     << "// names exactly the state the procedure reads and writes.\n";
  for (ProgramProcedure proc : program.Procedures()) {
    const auto &fx = EffectsOf(proc);
    os << DetailTemplateHeader(fx);
    if (!fx.uses_log && !fx.uses_functors) {
      os << "inline ";
    }
    os << "bool " << DetailName(proc) << "(" << DetailStateParams(proc);
    if (const auto tail = ProcValueParams(proc); !tail.empty()) {
      os << ", " << tail;
    }
    os << ");\n";
  }
  os << "\n";
}

// -----------------------------------------------------------------------
// Header emission.

void Generator::EmitInlines(OutputStream &os, const char *stage) {
  for (ParsedInline code : Inlines(module, Language::kCxx)) {
    if (code.Stage() == stage) {
      os << code.CodeToInline() << "\n";
    }
  }
}

void Generator::EmitEnums(void) {
  for (ParsedEnumType type : module.EnumTypes()) {
    hh << "enum class " << type.NameAsString() << " : "
       << TypeName(module, type.UnderlyingType()) << " {\n";
    hh.PushIndent();
    for (ParsedForeignConstant enumerator : type.Enumerators()) {
      hh << hh.Indent() << enumerator.NameAsString();
      if (auto code = enumerator.Constructor(); !code.empty()) {
        hh << " = " << code;
      }
      hh << ",\n";
    }
    hh.PopIndent();
    hh << "};\n\n";
  }
}

// Emits a plain aggregate with a `Hash` method and defaulted equality.
void Generator::EmitHashStruct(
    const std::string &name,
    const std::vector<std::pair<std::string, std::string>> &typed_fields) {
  hh << "struct " << name << " {\n";
  hh.PushIndent();
  std::vector<std::string> field_names;
  for (const auto &[type, field] : typed_fields) {
    hh << hh.Indent() << type << " " << field << ";\n";
    field_names.push_back(field);
  }
  hh << hh.Indent() << "uint64_t Hash(void) const noexcept {\n";
  hh.PushIndent();
  hh << hh.Indent() << "return ::hyde::rt::HashRow("
     << JoinExprs(field_names, ", ") << ");\n";
  hh.PopIndent();
  hh << hh.Indent() << "}\n";
  hh << hh.Indent() << "bool operator==(const " << name
     << " &) const noexcept = default;\n";
  hh.PopIndent();
  hh << "};\n\n";
}

void Generator::EmitShapeStructs(void) {
  for (const auto &[mangled, shape] : shapes) {
    const auto &[name, types] = shape;
    hh << "struct " << name << " {\n";
    hh.PushIndent();
    auto i = 0u;
    for (TypeLoc t : types) {
      hh << hh.Indent() << TypeName(module, t) << " c" << (i++) << ";\n";
    }
    hh << hh.Indent() << "auto operator<=>(const " << name
       << " &) const noexcept = default;\n";
    hh.PopIndent();
    hh << "};\n\n";
  }
}

void Generator::EmitRowStructs(void) {
  for (DataTable table : program.Tables()) {
    const auto id = table.Id();
    const auto &fields = col_field[id];

    hh << "// Rows of `" << table_member[id] << "`";
    if (auto decl = TableDecl(table)) {
      hh << " (" << decl->Name() << "/" << decl->Arity() << ")";
    }
    hh << ".\n";

    std::vector<std::pair<std::string, std::string>> typed_fields;
    auto i = 0u;
    for (DataColumn col : table.Columns()) {
      typed_fields.emplace_back(TypeName(module, col.Type()), fields[i++]);
    }
    EmitHashStruct(row_type[id], typed_fields);

    for (DataIndex index : table.Indices()) {
      if (!key_type.contains(index.Id())) {
        continue;
      }
      hh << "// Key of `" << index_member[index.Id()] << "` over `"
         << table_member[id] << "`.\n";
      std::vector<std::pair<std::string, std::string>> key_fields;
      for (DataColumn col : index.KeyColumns()) {
        key_fields.emplace_back(TypeName(module, col.Type()),
                                fields[col.Index()]);
      }
      EmitHashStruct(key_type[index.Id()], key_fields);
    }
  }
}

// R3 (v3-spec-statecell.md §1.3/§C-5): emit, per StateCell store, a `Key_<id>`
// hash struct (group ++ config columns) and a `Reduce_<id>` policy that bridges
// the runtime Algebra interface (Invertible<Reduce> / Recompute<Reduce>, see
// Runtime/StateCell.h) to the DRIVER-SUPPLIED reduction free functions. The
// C-5 ABI (resolved by unqualified/ADL call from the generated template
// context, no DatabaseFunctors membership) for a reduction functor named `F`:
//   @invertible:  Summary  F_identity();
//                 Summary  F_combine(Summary working, Summary value);
//                 Summary  F_uncombine(Summary working, Summary value);
//     (Working == Summary for the corpus abelian reductions SUM/COUNT; the
//      finalizer is identity. AVG-style Working != Summary is a later ABI.)
//   @recompute:   Summary  F_reduce(const Summary *values, const int32_t
//                                   *counts, size_t n);
//     (the from-scratch rescan over the live multiset; MIN/MAX/opaque merge.)
// The driver defines these before its message calls (new-driver review gate).
void Generator::EmitStateCellStructs(void) {
  for (const ProgramStateCellInfo &cell : program.StateCells()) {
    const auto id = std::to_string(cell.Id());
    const std::string fname = Sanitize(ToString(cell.Functor().Name()));

    // Key_<id>: a hash struct over the group ++ config column types.
    std::vector<std::pair<std::string, std::string>> key_fields;
    {
      auto i = 0u;
      for (TypeLoc t : cell.KeyTypes()) {
        key_fields.emplace_back(TypeName(module, t), "c" + std::to_string(i++));
      }
    }
    hh << "// StateCell #" << id << " group key.\n";
    EmitHashStruct("Key_" + id, key_fields);

    // The summary C++ type. Single-column summary for the corpus (SUM/COUNT/
    // merge value); a multi-column summary would be a std::tuple (later ABI).
    std::string summary_type = "int64_t";
    if (cell.SummaryTypes().size() == 1u) {
      summary_type = TypeName(module, cell.SummaryTypes()[0]);
    } else if (cell.SummaryTypes().size() > 1u) {
      summary_type = "std::tuple<";
      auto sep = "";
      for (TypeLoc t : cell.SummaryTypes()) {
        summary_type += sep + TypeName(module, t);
        sep = ", ";
      }
      summary_type += ">";
    }

    // P2c: the CONFIGURATION column type suffix (the tail of KeyTypes). A
    // config-DEPENDENT reduction receives these as LEADING params on its C-5
    // free functions and policy methods; a config-FREE cell (num_config == 0)
    // emits exactly the pre-P2c signatures (byte-identical). @invertible takes
    // config at the Fold arm (Combine/Uncombine); @recompute takes it at the
    // reduce arm (ReduceLive) — the two algebra arms use config at disjoint
    // sites, never both.
    const unsigned num_config = cell.NumConfigTypes();
    const auto &key_types = cell.KeyTypes();
    // `cfg_decl_prefix`: the leading params for the C-5 free-function DECLS
    // (e.g. `int32_t cfg0, `); empty when config-free. `cfg_fwd_prefix`: the
    // matching forwarded actual args at the call site (`cfg0, `). `cfg_tuple`:
    // the std::tuple<...> config type list for the policy's ConfigTuple.
    std::string cfg_decl_prefix, cfg_fwd_prefix, cfg_tuple;
    for (unsigned k = 0u; k < num_config; ++k) {
      const std::string cty =
          TypeName(module, key_types[key_types.size() - num_config + k]);
      const std::string cname = "cfg" + std::to_string(k);
      cfg_decl_prefix += cty + " " + cname + ", ";
      cfg_fwd_prefix += cname + ", ";
      cfg_tuple += (k ? ", " : "") + cty;
    }

    // C-5 free-function forward declarations: DECLARED in the generated header,
    // DEFINED out-of-line by the driver (the DatabaseLog/DatabaseFunctors
    // declared-in-header/defined-by-driver idiom, but as free functions so the
    // engine owns the state layout). Ordinary unqualified lookup from the
    // Reduce_<id> template context finds these (no ADL on builtin summary
    // types).
    hh << "// StateCell #" << id << " reduction functions over `" << fname
       << "` (C-5 driver ABI; define these out-of-line in your driver TU).\n";
    if (cell.IsInvertible()) {
      hh << summary_type << " " << fname << "_identity();\n"
         << summary_type << " " << fname << "_combine(" << cfg_decl_prefix
         << summary_type << " working, " << summary_type << " value);\n"
         << summary_type << " " << fname << "_uncombine(" << cfg_decl_prefix
         << summary_type << " working, " << summary_type << " value);\n";
    } else {
      hh << summary_type << " " << fname << "_reduce(" << cfg_decl_prefix
         << "const " << summary_type
         << " *values, const int32_t *counts, ::std::size_t n);\n";
    }

    const std::string reduce_name = "Reduce_" + id;
    hh << "// StateCell #" << id << " reduction policy over `" << fname
       << "` (C-5 driver ABI).\n";
    hh << "struct " << reduce_name << " {\n";
    hh.PushIndent();
    hh << hh.Indent() << "using Summary = " << summary_type << ";\n";
    // P2c discriminator: config-bearing cells expose kHasConfig + ConfigTuple so
    // the runtime StateCellStore::DebugValidate can synthesize identity config
    // probes for the invertibility round-trip (option (i)). Config-free cells
    // omit these entirely (the HasConfigPolicy concept is unsatisfied and the
    // debug machinery compiles away byte-identically to pre-P2c).
    if (num_config) {
      hh << hh.Indent() << "static constexpr bool kHasConfig = true;\n"
         << hh.Indent() << "static constexpr unsigned num_config = "
         << num_config << "u;\n"
         << hh.Indent() << "using ConfigTuple = ::std::tuple<" << cfg_tuple
         << ">;\n";
    }
    if (cell.IsInvertible()) {
      // Working == Summary (abelian running reduction). The driver supplies the
      // step semantics; the engine owns the layout. Config (if any) is a leading
      // param on Combine/Uncombine, forwarded to the driver free function.
      hh << hh.Indent() << "using Working = Summary;\n"
         << hh.Indent() << "static void Identity(Working &w) { w = " << fname
         << "_identity(); }\n"
         << hh.Indent() << "static void Combine(Working &w, " << cfg_decl_prefix
         << "const Summary &v) { w = " << fname << "_combine(" << cfg_fwd_prefix
         << "w, v); }\n"
         << hh.Indent() << "static void Uncombine(Working &w, "
         << cfg_decl_prefix << "const Summary &v) { w = " << fname
         << "_uncombine(" << cfg_fwd_prefix << "w, v); }\n"
         << hh.Indent()
         << "static Summary Finalize(const Working &w) { return w; }\n";
    } else {
      // @recompute: rescan the live multiset from scratch (the driver merge).
      // Config (if any) is a leading param on ReduceLive, forwarded to the
      // driver free function; the store loads it from KeyAt(gid) at emit time.
      hh << hh.Indent() << "static Summary ReduceLive(" << cfg_decl_prefix
         << "const ::hyde::rt::Vec<Summary> &values, "
            "const ::hyde::rt::Vec<int32_t> &counts) {\n";
      hh.PushIndent();
      hh << hh.Indent() << "return " << fname << "_reduce(" << cfg_fwd_prefix
         << "values.begin(), counts.begin(), values.Size());\n";
      hh.PopIndent();
      hh << hh.Indent() << "}\n";
    }
    hh.PopIndent();
    hh << "};\n\n";
  }
}

void Generator::EmitFunctorsDecl(void) {
  EmitInlines(hh, "c++:database:functors:prologue");

  // P1 (ADL/functor-surface epoch): MAP functors are delivered as FREE
  // functions (declared here, defined out-of-line by the driver), the same
  // C-5 idiom EmitStateCellStructs uses for reduction bodies. Emitted
  // BEFORE `struct DatabaseFunctors` so the decls precede every detail
  // template body (two-phase lookup, E-19). The struct survives EMPTY so
  // the entry-point `Functors` template param still deduces
  // `DatabaseFunctors` and drivers still `DatabaseFunctors functors;`.
  //
  // The banners below are gated on a decl actually being emitted so that a
  // program with no MAP functor emits byte-identical text to the pre-P1
  // shape (the P1 functor-free byte-compare gate).
  auto emits_map_decl = [](ParsedFunctor func) -> bool {
    if (func.IsInline(Language::kCxx)) {
      return false;
    }
    for (ParsedParameter param : ParsedDeclaration(func).Parameters()) {
      const auto b = param.Binding();
      if (b == ParameterBinding::kAggregate ||
          b == ParameterBinding::kSummary) {
        return false;
      }
    }
    return true;
  };
  bool any_map_decl = false;
  for (ParsedFunctor func : Functors(module)) {
    if (emits_map_decl(func)) {
      any_map_decl = true;
      break;
    }
  }

  if (any_map_decl) {
    hh << "// User-provided MAP functors. Define these free functions in your\n"
       << "// own translation unit; the generated code calls them by\n"
       << "// unqualified name.\n";
  }
  for (ParsedFunctor func : Functors(module)) {
    if (func.IsInline(Language::kCxx)) {
      continue;
    }
    const ParsedDeclaration decl(func);

    // R3 (v3-spec-statecell.md §C-5): a reduction functor (one carrying
    // `aggregate`/`summary` parameters — an over() summary functor or a
    // mutable() merge functor) is NOT a DatabaseFunctors member. Under the C-5
    // ABI its reduction body is a DRIVER-SUPPLIED FREE FUNCTION over
    // driver-owned state (agg_init/agg_fold/agg_emit/agg_unfold), reached by
    // unqualified call from the StateCellStore template context — never through
    // the DatabaseFunctors surface (which has no `aggregate`/`summary` binding
    // pattern). Skip it here so the generated header stays valid C++.
    bool is_reduction = false;
    for (ParsedParameter param : decl.Parameters()) {
      const auto b = param.Binding();
      if (b == ParameterBinding::kAggregate ||
          b == ParameterBinding::kSummary) {
        is_reduction = true;
        break;
      }
    }
    if (is_reduction) {
      continue;
    }

    std::vector<ParsedParameter> free_params;
    for (ParsedParameter param : decl.Parameters()) {
      if (param.Binding() == ParameterBinding::kFree) {
        free_params.push_back(param);
      }
    }

    std::string val_type;
    if (free_params.size() == 1u) {
      val_type = TypeName(module, free_params[0].Type());
    } else if (free_params.size() > 1u) {
      val_type = "std::tuple<";
      auto sep = "";
      for (ParsedParameter param : free_params) {
        val_type += sep + TypeName(module, param.Type());
        sep = ", ";
      }
      val_type += ">";
    }

    // Return convention by range: filter -> bool; zero-or-one -> optional
    // (or the nullable type itself); one-to-one -> the value; zero-or-more
    // and one-or-more -> a materialized std::vector.
    std::string ret;
    switch (func.Range()) {
      case FunctorRange::kZeroOrOne:
        if (free_params.empty()) {
          ret = "bool";
        } else if (free_params.size() == 1u &&
                   free_params[0].Type().IsNullable(module, Language::kCxx)) {
          ret = val_type;
        } else {
          ret = "std::optional<" + val_type + ">";
        }
        break;
      case FunctorRange::kOneToOne: ret = val_type; break;
      case FunctorRange::kZeroOrMore:
      case FunctorRange::kOneOrMore:
        ret = "std::vector<" + val_type + ">";
        break;
    }

    hh << ret << " " << func.Name() << "_" << decl.BindingPattern() << "(";
    auto sep = "";
    for (ParsedParameter param : decl.Parameters()) {
      if (param.Binding() == ParameterBinding::kBound) {
        hh << sep << TypeName(module, param.Type()) << " "
           << Sanitize(ToString(param.Name()));
        sep = ", ";
      }
    }
    hh << ");\n";
  }
  // The (now vestigial) functor struct: empty, kept for `Functors`
  // deduction and driver construction (see above).
  if (any_map_decl) {
    hh << "// User-provided functors object (vestigial ABI seam; empty since\n"
       << "// the P1 MAP-functor free-function migration).\n";
  } else {
    hh << "// User-provided functors. Define the declared member functions in\n"
       << "// your own translation unit; the generated code calls them.\n";
  }
  hh << "struct DatabaseFunctors {\n";
  hh.PushIndent();
  EmitInlines(hh, "c++:database:functors:definition:prologue");
  EmitInlines(hh, "c++:database:functors:definition:epilogue");
  hh.PopIndent();
  hh << "};\n\n";
  EmitInlines(hh, "c++:database:functors:epilogue");
}

void Generator::EmitLogDecl(void) {
  EmitInlines(hh, "c++:database:log:prologue");
  hh << "// Receives published messages. The default methods do nothing.\n"
     << "// Entry points deduce the log's static type, so any type\n"
     << "// providing the same member signatures observes the published\n"
     << "// delta stream (a @differential message publishes one call per\n"
     << "// net presence change at each batch's commit sweep) with no\n"
     << "// virtual dispatch.\n"
     << "struct DatabaseLog {\n";
  hh.PushIndent();
  EmitInlines(hh, "c++:database:log:definition:prologue");
  for (ParsedMessage message : Messages(module)) {
    if (!message.IsPublished()) {
      continue;
    }
    hh << hh.Indent() << "void " << message.Name() << "_"
       << message.Arity() << "(";
    for (ParsedParameter param : ParsedDeclaration(message).Parameters()) {
      hh << TypeName(module, param.Type()) << " "
         << Sanitize(ToString(param.Name())) << ", ";
    }
    hh << "bool added) {}\n";
  }
  EmitInlines(hh, "c++:database:log:definition:epilogue");
  hh.PopIndent();
  hh << "};\n\n";
  EmitInlines(hh, "c++:database:log:epilogue");
}

void Generator::EmitDatabaseDecl(void) {
  hh << "// The sealed database state: tables, indices, and epoch counters.\n"
     << "// Construction allocates empty tables and cannot fail; epoch 0\n"
     << "// (the empty-program fixpoint) runs when the driver calls\n"
     << "// `init(db, log, functors)`, and every entry point asserts it has\n"
     << "// run. All driver-facing functions are hidden friends: reach them\n"
     << "// by unqualified call with the database as an argument.\n"
     << "struct Database {\n"
     << " public:\n";
  hh.PushIndent();

  // In-class constructor: member-init only, no callbacks, cannot fail.
  hh << hh.Indent()
     << "explicit Database(::hyde::rt::Allocator allocator_)\n";
  hh.PushIndent();
  hh << hh.Indent() << ": allocator(allocator_)";
  for (DataTable table : program.Tables()) {
    hh << ",\n" << hh.Indent() << "  " << table_member[table.Id()]
       << "(allocator_)";
    for (DataIndex index : table.Indices()) {
      if (auto it = index_member.find(index.Id()); it != index_member.end()) {
        hh << ",\n" << hh.Indent() << "  " << it->second << "(allocator_)";
      }
    }
  }
  // R3: StateCell stores, constructed with just the allocator.
  for (const ProgramStateCellInfo &cell : program.StateCells()) {
    hh << ",\n" << hh.Indent() << "  statecell_" << cell.Id() << "(allocator_)";
  }
  hh << " {}\n\n";
  hh.PopIndent();

  // Epoch 0: the zeroth entry point.
  for (ProgramProcedure proc : program.Procedures()) {
    if (proc.Kind() != ProcedureKind::kInitializer) {
      continue;
    }
    const auto &fx = EffectsOf(proc);
    hh << hh.Indent() << "// Epoch 0: derives the empty-EDB least model "
          "and publishes its\n"
       << hh.Indent() << "// t=0 deltas to `log`. Call exactly once, "
          "before any message.\n"
       << hh.Indent() << "template <typename Log, typename Functors>\n"
       << hh.Indent() << "friend auto init(Database &db, Log &"
       << (fx.uses_log ? "log" : "") << ", Functors &"
       << (fx.uses_functors ? "functors" : "") << ") {\n";
    hh.PushIndent();
    hh << hh.Indent() << "assert(!db.initialized_);\n"
       << hh.Indent() << "db.initialized_ = true;\n"
       << hh.Indent() << "return " << DetailName(proc) << "("
       << DetailStateArgs(proc, "db.") << ");\n";
    hh.PopIndent();
    hh << hh.Indent() << "}\n\n";
    break;
  }

  // Message entry points: thin hidden-friend wrappers over the handler
  // detail twins.
  for (ProgramProcedure proc : program.Procedures()) {
    if (proc.Kind() != ProcedureKind::kMessageHandler) {
      continue;
    }
    const auto &fx = EffectsOf(proc);
    hh << hh.Indent() << "// Message `" << proc.Message()->Name() << "/"
       << proc.Message()->Arity() << "`.\n";
    hh << hh.Indent() << "template <typename Log, typename Functors>\n";
    hh << hh.Indent() << "friend auto " << ProcName(proc)
       << "(Database &db, Log &" << (fx.uses_log ? "log" : "")
       << ", Functors &" << (fx.uses_functors ? "functors" : "");
    for (DataVector vec : proc.VectorParameters()) {
      hh << ", ::hyde::rt::Vec<" << VecType(vec) << "> " << VecName(vec);
    }
    for (DataVariable var : proc.VariableParameters()) {
      hh << ", " << TypeName(module, var.Type()) << " " << VarName(var);
    }
    hh << ") {\n";
    hh.PushIndent();
    hh << hh.Indent() << "assert(db.initialized_);\n";
    hh << hh.Indent() << "return " << DetailName(proc) << "("
       << DetailStateArgs(proc, "db.");
    for (DataVector vec : proc.VectorParameters()) {
      hh << ", std::move(" << VecName(vec) << ")";
    }
    for (DataVariable var : proc.VariableParameters()) {
      hh << ", " << VarName(var);
    }
    hh << ");\n";
    hh.PopIndent();
    hh << hh.Indent() << "}\n\n";
  }

  // Query entry points and their cursors, defined in-class. A query
  // with a forcing function runs a flow first, so it deduces Log and
  // Functors like a message entry point; plain queries only read.
  for (const ProgramQuery &spec : program.Queries()) {
    EmitQueryFriends(spec);
  }

  hh.PopIndent();
  hh << " private:\n";
  hh.PushIndent();

  hh << hh.Indent() << "::hyde::rt::Allocator allocator;\n\n";

  // Tables and their indexes. Differential tables carry per-row derivation
  // counters; monotone tables are insert-only row logs.
  for (DataTable table : program.Tables()) {
    const auto id = table.Id();
    hh << hh.Indent() << "::hyde::rt::"
       << (table.IsDifferential() ? "DiffTable<" : "Table<") << row_type[id]
       << "> " << table_member[id] << ";\n";
    for (DataIndex index : table.Indices()) {
      if (auto it = index_member.find(index.Id()); it != index_member.end()) {
        hh << hh.Indent() << "::hyde::rt::Index<" << key_type[index.Id()]
           << "> " << it->second << ";\n";
      }
    }
  }
  hh << "\n";

  // R3: one StateCell store per aggregate / KV view (v3-spec-statecell.md §1).
  for (const ProgramStateCellInfo &cell : program.StateCells()) {
    const auto id = std::to_string(cell.Id());
    const std::string algebra = cell.IsInvertible()
        ? "::hyde::rt::Invertible<Reduce_" + id + ">"
        : "::hyde::rt::Recompute<Reduce_" + id + ">";
    hh << hh.Indent() << "::hyde::rt::StateCellStore<Key_" << id << ", "
       << algebra << "> statecell_" << id << ";\n";
  }
  if (!program.StateCells().empty()) {
    hh << "\n";
  }

  // Mutable globals (condition ref-counts, init guards, fixpoint depth).
  for (DataVariable var : program.GlobalVariables()) {
    hh << hh.Indent() << TypeName(module, var.Type()) << " " << VarName(var)
       << " = 0;\n";
  }
  hh << hh.Indent() << "bool initialized_ = false;\n";

  hh.PopIndent();
  hh << "};\n\n";
}

// One query's driver surface: an existence check, or a nested cursor
// struct (with its in-class `next`) plus the factory. All hidden
// friends; all assert epoch 0 has run.
void Generator::EmitQueryFriends(const ProgramQuery &spec) {
  const ParsedDeclaration decl(spec.query);
  const auto name =
      Sanitize(ToString(decl.Name())) + "_" + std::string(decl.BindingPattern());
  const auto table = spec.table;
  const auto member = table_member[table.Id()];
  const auto &fields = col_field[table.Id()];

  std::vector<ParsedParameter> params;
  std::vector<bool> is_bound;
  std::vector<std::string> param_names;
  for (ParsedParameter param : decl.Parameters()) {
    params.push_back(param);
    is_bound.push_back(param.Binding() == ParameterBinding::kBound);
    param_names.push_back(Sanitize(ToString(param.Name())));
  }
  const bool has_free =
      std::find(is_bound.begin(), is_bound.end(), false) != is_bound.end();
  const bool differential = table.IsDifferential();

  std::vector<std::string> bound_names, free_names;
  for (auto i = 0u; i < params.size(); ++i) {
    (is_bound[i] ? bound_names : free_names).push_back(param_names[i]);
  }

  hh << hh.Indent() << "// Query `" << decl.Name() << "/" << decl.Arity()
     << "` (" << decl.BindingPattern() << ").\n";

  // The forced form runs a flow before reading, so it deduces Log and
  // Functors; the plain form is a non-template read-only friend.
  const auto emit_signature_prefix = [&](void) {
    if (spec.forcing_function) {
      const auto &fx = EffectsOf(*spec.forcing_function);
      hh << hh.Indent() << "template <typename Log, typename Functors>\n";
      hh << hh.Indent() << "friend "
         << (has_free ? name + "_cursor" : std::string("bool")) << " "
         << name << "(Database &db, Log &" << (fx.uses_log ? "log" : "")
         << ", Functors &" << (fx.uses_functors ? "functors" : "");
    } else {
      hh << hh.Indent() << "friend "
         << (has_free ? name + "_cursor" : std::string("bool")) << " "
         << name << "(Database &db";
    }
    for (auto i = 0u; i < params.size(); ++i) {
      if (is_bound[i]) {
        hh << ", " << TypeName(module, params[i].Type()) << " "
           << param_names[i];
      }
    }
    hh << ") {\n";
  };

  const auto emit_forcing_call = [&](const std::vector<std::string> &args) {
    if (!spec.forcing_function) {
      return;
    }
    hh << hh.Indent() << DetailName(*spec.forcing_function) << "("
       << DetailStateArgs(*spec.forcing_function, "db.");
    for (const auto &arg : args) {
      hh << ", " << arg;
    }
    hh << ");\n";
  };

  if (!has_free) {
    // Existence check.
    emit_signature_prefix();
    hh.PushIndent();
    hh << hh.Indent() << "assert(db.initialized_);\n";
    emit_forcing_call(param_names);
    if (differential) {
      hh << hh.Indent() << "const uint32_t id = db." << member << ".Find("
         << RowExpr(param_names) << ");\n";
      hh << hh.Indent() << "return id != ::hyde::rt::kNoRow && db."
         << member << ".Present(id);\n";
    } else {
      hh << hh.Indent() << "return db." << member << ".Find("
         << RowExpr(param_names) << ") != ::hyde::rt::kNoRow;\n";
    }
    hh.PopIndent();
    hh << hh.Indent() << "}\n\n";
    return;
  }

  // The nested cursor, with `next` defined in-class (nested member
  // bodies compile in complete-class context, so the later-declared
  // table members are visible).
  const bool via_index = spec.index && index_member.contains(spec.index->Id());
  hh << hh.Indent() << "struct " << name << "_cursor {\n";
  hh.PushIndent();
  hh << hh.Indent() << "Database &db;\n";
  for (auto i = 0u; i < params.size(); ++i) {
    if (is_bound[i]) {
      hh << hh.Indent() << TypeName(module, params[i].Type()) << " "
         << param_names[i] << ";\n";
    }
  }
  hh << hh.Indent() << "uint32_t pos;\n";
  hh << hh.Indent() << "bool next(";
  auto sep = "";
  for (auto i = 0u; i < params.size(); ++i) {
    if (!is_bound[i]) {
      hh << sep << TypeName(module, params[i].Type()) << " &"
         << param_names[i];
      sep = ", ";
    }
  }
  hh << ") {\n";
  hh.PushIndent();
  if (via_index) {
    hh << hh.Indent() << "while (pos != ::hyde::rt::kNoRow) {\n";
    hh.PushIndent();
    hh << hh.Indent() << "const uint32_t id = pos;\n";
    hh << hh.Indent() << "pos = db." << index_member[spec.index->Id()]
       << ".Next(id);\n";
  } else {
    hh << hh.Indent() << "while (pos < db." << member << ".NumRows()) {\n";
    hh.PushIndent();
    hh << hh.Indent() << "const uint32_t id = pos++;\n";
  }
  hh << hh.Indent() << "const auto row = db." << member << ".RowAt(id);\n";
  if (differential) {
    hh << hh.Indent() << "if (!db." << member << ".Present(id)) {\n";
    hh.PushIndent();
    hh << hh.Indent() << "continue;\n";
    hh.PopIndent();
    hh << hh.Indent() << "}\n";
  }
  if (!via_index) {
    // Re-check bound columns (a full scan is unkeyed).
    for (auto i = 0u; i < params.size(); ++i) {
      if (is_bound[i]) {
        hh << hh.Indent() << "if (row." << fields[i]
           << " != " << param_names[i] << ") {\n";
        hh.PushIndent();
        hh << hh.Indent() << "continue;\n";
        hh.PopIndent();
        hh << hh.Indent() << "}\n";
      }
    }
  }
  for (auto i = 0u; i < params.size(); ++i) {
    if (!is_bound[i]) {
      hh << hh.Indent() << param_names[i] << " = row." << fields[i]
         << ";\n";
    }
  }
  hh << hh.Indent() << "return true;\n";
  hh.PopIndent();
  hh << hh.Indent() << "}\n";
  hh << hh.Indent() << "return false;\n";
  hh.PopIndent();
  hh << hh.Indent() << "}\n";
  hh.PopIndent();
  hh << hh.Indent() << "};\n";

  // The factory.
  emit_signature_prefix();
  hh.PushIndent();
  hh << hh.Indent() << "assert(db.initialized_);\n";
  emit_forcing_call(bound_names);
  hh << hh.Indent() << "return {db";
  for (const auto &p : bound_names) {
    hh << ", " << p;
  }
  if (via_index) {
    std::vector<std::string> key_exprs;
    for (DataColumn col : spec.index->KeyColumns()) {
      key_exprs.push_back(param_names[col.Index()]);
    }
    hh << ", db." << index_member[spec.index->Id()] << ".First("
       << RowExpr(key_exprs) << ")";
  } else {
    hh << ", 0";
  }
  hh << "};\n";
  hh.PopIndent();
  hh << hh.Indent() << "}\n\n";
}

// -----------------------------------------------------------------------
// Procedure emission.

void Generator::EmitProcedure(ProgramProcedure proc) {
  // The detail function carries the body; its parameter list is the
  // procedure's transitive read/write set plus the value parameters.
  const auto &fx = EffectsOf(proc);
  cc << DetailTemplateHeader(fx);
  if (!fx.uses_log && !fx.uses_functors) {
    cc << "inline ";
  }
  cc << "bool " << DetailName(proc) << "(" << DetailStateParams(proc);
  if (const auto tail = ProcValueParams(proc); !tail.empty()) {
    cc << ", " << tail;
  }
  cc << ") {\n";
  cc.PushIndent();

  for (DataVector vec : proc.DefinedVectors()) {
    cc << cc.Indent() << "::hyde::rt::Vec<" << VecType(vec) << "> "
       << VecName(vec) << "(allocator);\n";
  }

  const auto body = proc.Body();
  EmitRegion(body);
  if (!EndsWithReturn(body)) {
    cc << cc.Indent() << "return false;\n";
  }
  cc.PopIndent();
  cc << "}\n\n";
}

// -----------------------------------------------------------------------
// Regions.

void Generator::EmitRegion(ProgramRegion region) {
  if (region.IsSeries()) {
    for (auto sub : ProgramSeriesRegion::From(region).Regions()) {
      EmitRegion(sub);
    }
  } else if (region.IsParallel()) {
    // Data-independent children; emitted sequentially.
    for (auto sub : ProgramParallelRegion::From(region).Regions()) {
      EmitRegion(sub);
    }
  } else if (region.IsLetBinding()) {
    auto let = ProgramLetBindingRegion::From(region);
    EmitComment(region);
    auto defs = let.DefinedVariables();
    auto uses = let.UsedVariables();
    for (auto i = 0u; i < defs.size(); ++i) {
      cc << cc.Indent() << "const auto " << VarName(defs[i]) << " = "
         << VarName(uses[i]) << ";\n";
    }
    EmitOptional(let.Body());
  } else if (region.IsReturn()) {
    cc << cc.Indent() << "return "
       << (ProgramReturnRegion::From(region).ReturnsTrue() ? "true" : "false")
       << ";\n";
  } else if (region.IsVectorLoop()) {
    auto loop = ProgramVectorLoopRegion::From(region);
    auto body = loop.Body();
    if (!body) {
      return;
    }
    EmitComment(region);
    cc << cc.Indent() << "for (auto ["
       << JoinExprs(VarExprs(loop.TupleVariables()), ", ") << "] : "
       << VecName(loop.Vector()) << ") {\n";
    cc.PushIndent();
    EmitRegion(*body);
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  } else if (region.IsVectorAppend()) {
    auto append = ProgramVectorAppendRegion::From(region);
    cc << cc.Indent() << VecName(append.Vector()) << ".Add("
       << RowExpr(VarExprs(append.TupleVariables())) << ");\n";
  } else if (region.IsVectorClear()) {
    cc << cc.Indent()
       << VecName(ProgramVectorClearRegion::From(region).Vector())
       << ".Clear();\n";
  } else if (region.IsVectorUnique()) {
    cc << cc.Indent()
       << VecName(ProgramVectorUniqueRegion::From(region).Vector())
       << ".SortAndUnique();\n";
  } else if (region.IsVectorSwap()) {
    auto swap = ProgramVectorSwapRegion::From(region);
    cc << cc.Indent() << VecName(swap.LHS()) << ".Swap(" << VecName(swap.RHS())
       << ");\n";
  } else if (region.IsUpdateCount()) {
    EmitUpdateCount(ProgramUpdateCountRegion::From(region));
  } else if (region.IsCheckMember()) {
    EmitCheckMember(ProgramCheckMemberRegion::From(region));
  } else if (region.IsCommitSweep()) {
    EmitCommitSweep(ProgramCommitSweepRegion::From(region));
  } else if (region.IsGroupUpdate()) {
    EmitGroupUpdate(ProgramGroupUpdateRegion::From(region));
  } else if (region.IsClaim()) {
    EmitClaim(ProgramClaimRegion::From(region));
  } else if (region.IsRetire()) {
    EmitRetire(ProgramRetireRegion::From(region));
  } else if (region.IsNetBatch()) {
    EmitNetBatch(ProgramNetBatchRegion::From(region));
  } else if (region.IsTableJoin()) {
    EmitJoin(ProgramTableJoinRegion::From(region));
  } else if (region.IsTableProduct()) {
    EmitProduct(ProgramTableProductRegion::From(region));
  } else if (region.IsTableScan()) {
    EmitScan(ProgramTableScanRegion::From(region));
  } else if (region.IsTupleCompare()) {
    EmitCompare(ProgramTupleCompareRegion::From(region));
  } else if (region.IsGenerate()) {
    EmitGenerate(ProgramGenerateRegion::From(region));
  } else if (region.IsCall()) {
    EmitCall(ProgramCallRegion::From(region));
  } else if (region.IsInduction()) {
    EmitInduction(ProgramInductionRegion::From(region));
  } else if (region.IsPublish()) {
    auto publish = ProgramPublishRegion::From(region);
    const auto message = publish.Message();
    cc << cc.Indent() << "log." << message.Name() << "_" << message.Arity()
       << "(";
    for (DataVariable var : publish.VariableArguments()) {
      cc << VarName(var) << ", ";
    }
    cc << (publish.IsRemoval() ? "false" : "true") << ");\n";
  } else if (region.IsTestAndSet()) {
    auto tas = ProgramTestAndSetRegion::From(region);
    const auto acc = VarName(tas.Accumulator());
    if (auto body = tas.Body()) {
      cc << cc.Indent() << "if ((" << acc << " += 1) == 1) {\n";
      cc.PushIndent();
      EmitRegion(*body);
      cc.PopIndent();
      cc << cc.Indent() << "}\n";
    } else {
      cc << cc.Indent() << acc << " += 1;\n";
    }
  } else if (region.IsWorkerId()) {
    // Single-threaded runtime: the worker id is always zero.
    auto worker = ProgramWorkerIdRegion::From(region);
    cc << cc.Indent() << "const uint64_t " << VarName(worker.WorkerId())
       << " = 0;\n"
       << cc.Indent() << "(void) " << VarName(worker.WorkerId()) << ";\n";
    EmitOptional(worker.Body());
  } else if (region.IsChangeRecord() || region.IsCheckRecord()) {
    Unsupported(
        "record regions (the IR analysis pass that creates them is disabled)");
  } else {
    Unsupported("an unknown region kind");
  }
}

void Generator::EmitCall(ProgramCallRegion region) {
  EmitComment(region);
  const ProgramProcedure callee = region.CalledProcedure();
  const auto callee_by_value = TakesVectorsByValue(callee);

  // Detail-to-detail call: the caller's parameter set is a superset of
  // the callee's (transitive closure), so every state argument is in
  // scope under its member name.
  std::string call =
      DetailName(callee) + "(" + DetailStateArgs(callee, "");
  auto sep = ", ";
  for (DataVector vec : region.VectorArguments()) {
    call += sep;
    if (callee_by_value) {
      call += "std::move(" + VecName(vec) + ")";
    } else {
      call += VecName(vec);
    }
    sep = ", ";
  }
  for (DataVariable var : region.VariableArguments()) {
    call += sep + VarName(var);
    sep = ", ";
  }
  call += ")";

  const auto true_body = region.BodyIfTrue();
  const auto false_body = region.BodyIfFalse();
  if (!true_body && !false_body) {
    cc << cc.Indent() << call << ";\n";
    return;
  }
  if (!true_body && false_body) {
    cc << cc.Indent() << "if (!" << call << ") {\n";
    cc.PushIndent();
    EmitRegion(*false_body);
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
    return;
  }
  cc << cc.Indent() << "if (" << call << ") {\n";
  cc.PushIndent();
  EmitRegion(*true_body);
  cc.PopIndent();
  cc << cc.Indent() << "}";
  if (false_body) {
    cc << " else {\n";
    cc.PushIndent();
    EmitRegion(*false_body);
    cc.PopIndent();
    cc << cc.Indent() << "}";
  }
  cc << "\n";
}

void Generator::EmitIndexAdds(DataTable table, const std::string &ins,
                              const std::vector<std::string> &tuple_exprs) {
  for (DataIndex index : table.Indices()) {
    const auto it = index_member.find(index.Id());
    if (it == index_member.end()) {
      continue;
    }
    std::vector<std::string> key_exprs;
    for (DataColumn col : index.KeyColumns()) {
      key_exprs.push_back(tuple_exprs[col.Index()]);
    }
    cc << cc.Indent() << it->second << ".Add(" << RowExpr(key_exprs) << ", "
       << ins << ".id);\n";
  }
}

void Generator::EmitUpdateCount(ProgramUpdateCountRegion region) {
  EmitComment(region);
  const auto table = region.Table();
  const auto tuple_exprs = VarExprs(region.TupleVariables());
  const auto row = RowExpr(tuple_exprs);
  const auto member = table_member[table.Id()];
  const auto body = region.Body();

  bool has_indexes = false;
  for (DataIndex index : table.Indices()) {
    if (index_member.contains(index.Id())) {
      has_indexes = true;
      break;
    }
  }

  // Monotone table: the fold degenerates to an insert-if-new, whose
  // crossing is "the row is new". Retraction folds cannot target a
  // monotone table.
  if (!table.IsDifferential()) {
    if (!body && !has_indexes) {
      cc << cc.Indent() << member << ".TryAdd(" << row << ");\n";
      return;
    }
    const auto ins = "ins" + std::to_string(next_ins_id++);
    cc << cc.Indent() << "if (const auto " << ins << " = " << member
       << ".TryAdd(" << row << "); " << ins << ".added) {\n";
    cc.PushIndent();
    EmitIndexAdds(table, ins, tuple_exprs);
    if (body) {
      EmitRegion(*body);
    }
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
    return;
  }

  // Differential table: one signed counter fold; every fresh log entry is
  // linked into the indexes (liveness is filtered at read time), and the
  // crossed body runs only on a zero-crossing event. An explicit fold
  // toggles the row's set-semantics message-support bit instead of moving
  // a multiset derivation count.
  std::string call;
  if (region.IsExplicit()) {
    call = (region.IsAdd() ? "AddExplicit(" : "SubExplicit(") + row + ")";
  } else {
    const char *method = region.IsAdd() ? "AddDerivation" : "SubDerivation";
    const char *cls =
        region.DerivationClass() == DerivClass::kRecursive
            ? "::hyde::rt::DerivClass::kRecursive"
            : "::hyde::rt::DerivClass::kNonRecursive";
    call = std::string(method) + "(" + row + ", " + cls + ")";
  }

  if (!body && !has_indexes) {
    cc << cc.Indent() << member << "." << call << ";\n";
    return;
  }

  const auto d = "d" + std::to_string(next_ins_id++);
  cc << cc.Indent() << "{\n";
  cc.PushIndent();
  cc << cc.Indent() << "const auto " << d << " = " << member << "." << call
     << ";\n";
  if (has_indexes) {
    cc << cc.Indent() << "if (" << d << ".added_row) {\n";
    cc.PushIndent();
    EmitIndexAdds(table, d, tuple_exprs);
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  }
  if (body) {
    cc << cc.Indent() << "if (" << d << ".crossed) {\n";
    cc.PushIndent();
    EmitRegion(*body);
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  }
  cc.PopIndent();
  cc << cc.Indent() << "}\n";
}

// R3 GROUP_UPDATE codegen (v3-spec-statecell.md §2.2). Writes the whole
// two-band body: BAND (a) two fold loops over the input net removal / net
// addition frontiers (folding the projected (group, summary) into the store);
// BAND (b) an emit_touched loop over the store's sort-unique touched set,
// applying the occupancy-generalized one-net-pair guard (spec §C-1) and
// folding ± into the agg DiffTable's counters + appending to its del/add
// queues (E3 seed-before-drain; the agg table's own claim/frontier/commit tail
// drains them, identity to every acyclic table).
void Generator::EmitGroupUpdate(ProgramGroupUpdateRegion region) {
  EmitComment(region);
  const auto sc = "statecell_" + std::to_string(region.StateCellId());
  const auto &gpos = region.GroupPositions();
  const auto &spos = region.SummaryPositions();
  const auto key_type = "Key_" + std::to_string(region.StateCellId());
  // P2c: the config positions are the tail of `gpos` (gpos = group ++ config).
  // For an @invertible cell they are passed as LEADING args to Fold (the config
  // gate is applied incrementally at fold time). A @recompute cell suppresses
  // config at the Fold arm — its Working is a summary-only membership multiset
  // and config routes through Emit/ReduceLive instead — so the fold stays
  // config-free there. A config-free cell has num_config == 0 and emits the
  // pre-P2c `Fold(gid, sign, summary)` byte-identically.
  const unsigned num_config = region.NumConfigPositions();
  const bool fold_takes_config = region.IsInvertible() && num_config != 0u;

  const DataTable agg = region.AggTable();
  const auto agg_member = table_member[agg.Id()];

  // One fold arm: destructure the frontier row into positional locals, form the
  // Key from the group positions, and fold the summary position with `sign`.
  const auto emit_fold_arm = [&](DataVector frontier, int sign) {
    // The frontier row is the input view's columns in order. Destructure it
    // into positional locals f0..fn (arity == the vector's shape) and consume
    // the group/summary positions.
    const auto row_arity =
        static_cast<unsigned>(frontier.ColumnTypes().size());
    std::vector<std::string> binds;
    for (unsigned i = 0u; i < row_arity; ++i) {
      binds.push_back("f" + std::to_string(i));
    }
    cc << cc.Indent() << "for (const auto &[" << JoinExprs(binds, ", ")
       << "] : " << VecName(frontier) << ") {\n";
    cc.PushIndent();
    // Silence unused-binding warnings on positions we don't consume.
    for (unsigned i = 0u; i < row_arity; ++i) {
      bool used = false;
      for (auto p : gpos) { used |= (p == i); }
      for (auto p : spos) { used |= (p == i); }
      if (!used) {
        cc << cc.Indent() << "(void) f" << i << ";\n";
      }
    }
    std::vector<std::string> key_parts;
    for (auto p : gpos) { key_parts.push_back("f" + std::to_string(p)); }
    std::vector<std::string> sum_parts;
    for (auto p : spos) { sum_parts.push_back("f" + std::to_string(p)); }
    const std::string summary_expr =
        sum_parts.size() == 1u
            ? sum_parts[0]
            : ("{" + JoinExprs(sum_parts, ", ") + "}");
    cc << cc.Indent() << "const auto gid = " << sc << ".FindOrAddGroup("
       << key_type << "{" << JoinExprs(key_parts, ", ") << "});\n";
    // P2c @invertible config prefix: the last `num_config` group positions are
    // the config locals, passed AHEAD of the summary to Fold (the raw trailing
    // pack `(cfg0.., v)` lands on the emitted Reduce::Combine's named params).
    std::string cfg_prefix;
    if (fold_takes_config) {
      for (unsigned k = 0u; k < num_config; ++k) {
        cfg_prefix += "f" + std::to_string(gpos[gpos.size() - num_config + k]) +
                      ", ";
      }
    }
    cc << cc.Indent() << sc << ".Fold(gid, " << sign << ", " << cfg_prefix
       << summary_expr << ");\n";
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  };

  emit_fold_arm(region.NegFrontier(), -1);
  emit_fold_arm(region.PosFrontier(), +1);

  // The agg row from a key + summary value. The agg table row is (group ++
  // config ++ summary) in column order; the key contributes the leading
  // group/config fields (key.c0..) and `val` the trailing summary field(s).
  const auto agg_tuple_exprs = [&](const std::string &val)
      -> std::vector<std::string> {
    std::vector<std::string> parts;
    for (unsigned i = 0u; i < gpos.size(); ++i) {
      parts.push_back("key.c" + std::to_string(i));
    }
    parts.push_back(val);
    return parts;
  };
  const auto agg_row = [&](const std::string &val) -> std::string {
    return "{" + JoinExprs(agg_tuple_exprs(val), ", ") + "}";
  };

  // Whether the agg table carries any live index that needs maintenance on a
  // newly-inserted agg row (parity with EmitUpdateCount: an AddDerivation whose
  // `added_row` is set must be reflected into every index, else index-keyed
  // consumers — e.g. a JOIN on the summary output, average_weight's sum⋈count
  // — cannot Find the row). SubDerivation removes no index entry (rows persist
  // until compaction), exactly like the ordinary counter path.
  bool agg_has_indexes = false;
  for (DataIndex index : agg.Indices()) {
    if (index_member.contains(index.Id())) {
      agg_has_indexes = true;
      break;
    }
  }

  // Emit an `AddDerivation` that also maintains the agg table's indices on the
  // zero-crossing that materializes a new row (mirror EmitUpdateCount's
  // has_indexes arm).
  const auto emit_add_deriv = [&](const std::string &val) {
    if (!agg_has_indexes) {
      cc << cc.Indent() << agg_member << ".AddDerivation(" << agg_row(val)
         << ", ::hyde::rt::DerivClass::kNonRecursive);\n";
      return;
    }
    const auto d = "gd" + std::to_string(next_ins_id++);
    cc << cc.Indent() << "const auto " << d << " = " << agg_member
       << ".AddDerivation(" << agg_row(val)
       << ", ::hyde::rt::DerivClass::kNonRecursive);\n";
    cc << cc.Indent() << "if (" << d << ".added_row) {\n";
    cc.PushIndent();
    EmitIndexAdds(agg, d, agg_tuple_exprs(val));
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  };

  // BAND (b): emit_touched, the occupancy-generalized one-net-pair (spec §C-1).
  cc << cc.Indent() << "for (const auto gid : " << sc << ".Touched()) {\n";
  cc.PushIndent();
  cc << cc.Indent() << "const auto &key = " << sc << ".KeyAt(gid);\n";
  cc << cc.Indent() << "const bool w_occ = " << sc << ".WorkingOccupied(gid);\n";
  cc << cc.Indent() << "const bool s_occ = " << sc << ".SealedOccupied(gid);\n";
  // P2c config-@recompute: config actuals for Emit ONLY, loaded from the key
  // (config is the trailing group positions key.c<first>..key.c<last>). Empty
  // for config-free / @invertible cells (they take config at the Fold arm, or
  // none), so `Emit(gid)` stays byte-identical there. Old(gid) NEVER takes
  // config — it reads the pre-reduced sealed scalar (Old is non-variadic;
  // Old(gid, cfg) would not compile), so the config actuals apply to the single
  // Emit site only.
  std::string emit_cfg;
  if (!region.IsInvertible() && num_config != 0u) {
    const unsigned first = static_cast<unsigned>(gpos.size()) - num_config;
    for (unsigned k = 0u; k < num_config; ++k) {
      emit_cfg += ", key.c" + std::to_string(first + k);
    }
  }
  cc << cc.Indent() << "if (w_occ) {\n";
  cc.PushIndent();
  cc << cc.Indent() << "const auto new_v = " << sc << ".Emit(gid" << emit_cfg
     << ");\n";
  cc << cc.Indent() << "if (s_occ) {\n";
  cc.PushIndent();
  cc << cc.Indent() << "const auto old_v = " << sc << ".Old(gid);\n";
  cc << cc.Indent() << "if (!(new_v == old_v)) {\n";
  cc.PushIndent();
  // change: -old, +new
  cc << cc.Indent() << agg_member << ".SubDerivation(" << agg_row("old_v")
     << ", ::hyde::rt::DerivClass::kNonRecursive);\n";
  cc << cc.Indent() << VecName(region.DelQueue()) << ".Add(" << agg_row("old_v")
     << ");\n";
  emit_add_deriv("new_v");
  cc << cc.Indent() << VecName(region.AddQueue()) << ".Add(" << agg_row("new_v")
     << ");\n";
  cc.PopIndent();
  cc << cc.Indent() << "}\n";  // if new != old
  cc.PopIndent();
  cc << cc.Indent() << "} else {\n";  // birth: +new only
  cc.PushIndent();
  emit_add_deriv("new_v");
  cc << cc.Indent() << VecName(region.AddQueue()) << ".Add(" << agg_row("new_v")
     << ");\n";
  cc.PopIndent();
  cc << cc.Indent() << "}\n";  // s_occ ? change : birth
  cc.PopIndent();
  cc << cc.Indent() << "} else if (s_occ) {\n";  // death: -old only
  cc.PushIndent();
  cc << cc.Indent() << "const auto old_v = " << sc << ".Old(gid);\n";
  cc << cc.Indent() << agg_member << ".SubDerivation(" << agg_row("old_v")
     << ", ::hyde::rt::DerivClass::kNonRecursive);\n";
  cc << cc.Indent() << VecName(region.DelQueue()) << ".Add(" << agg_row("old_v")
     << ");\n";
  cc.PopIndent();
  cc << cc.Indent() << "}\n";  // occupancy cases
  cc.PopIndent();
  cc << cc.Indent() << "}\n";  // for touched
}

// The runtime method reading one named membership predicate.
static const char *PredicateMethod(MembershipPredicate pred) {
  switch (pred) {
    case MembershipPredicate::kInI: return "InI";
    case MembershipPredicate::kInNew: return "InNew";
    case MembershipPredicate::kSurvivesSoFar: return "SurvivesSoFar";
    case MembershipPredicate::kAliveAtClaim: return "AliveAtClaim";
    case MembershipPredicate::kInNewWithFrontier: return "InNewWithFrontier";
    case MembershipPredicate::kInNewSansFrontier: return "InNewSansFrontier";
    case MembershipPredicate::kPresent: return "Present";
    case MembershipPredicate::kRecursivelySupported:
      return "RecursivelySupported";
    case MembershipPredicate::kNetDeleted: return "NetDeleted";
    case MembershipPredicate::kNetAdded: return "NetAdded";
  }
  Unsupported("an unknown membership predicate");
}

void Generator::EmitCheckMember(ProgramCheckMemberRegion region) {
  const auto present = region.IfPresent();
  const auto absent = region.IfAbsent();
  if (!present && !absent) {
    return;
  }
  EmitComment(region);
  const auto table = region.Table();
  const auto member = table_member[table.Id()];
  const auto exprs = VarExprs(region.TupleVariables());
  const auto row = RowExpr(exprs);

  // A live same-table row binding makes the value-keyed Find redundant:
  // the predicate reads the scan's cursor id directly, and a monotone
  // existence test degenerates to constant truth (the row was just read
  // from the append-only log). See RowScopeEntry for the preconditions.
  const RowScopeEntry *const scope = FindRowScope(table.Id(), exprs);

  // Differential table: the named predicate applied to the row's id (a
  // missing row satisfies no predicate). Monotone table: current-state
  // membership is row existence; the frozen and net-change predicates read
  // the sealed row-id watermark.
  std::string cond;
  bool scoped = true;
  const auto id = "m" + std::to_string(next_ins_id++);
  if (!table.IsDifferential()) {
    switch (region.Predicate()) {
      case MembershipPredicate::kPresent:
      case MembershipPredicate::kInNew:
        if (scope) {
          // Tautology: emit the surviving branch bare (an absent-only
          // gate emits nothing).
          if (present) {
            EmitRegion(*present);
          }
          return;
        }
        cond = member + ".Find(" + row + ") != ::hyde::rt::kNoRow";
        scoped = false;
        break;
      case MembershipPredicate::kInI:
      case MembershipPredicate::kNetAdded:
      case MembershipPredicate::kNetDeleted:
        // Watermark reads keep the scoped Find: rewriting them onto the
        // cursor is safe in principle but has no corpus witness.
        break;
      default:
        Unsupported("a fixpoint-round membership read on a monotone table");
    }
  } else if (scope) {
    cond = member + "." + PredicateMethod(region.Predicate()) + "(" +
           scope->cursor + ")";
    scoped = false;
  }
  if (scoped) {
    cc << cc.Indent() << "{\n";
    cc.PushIndent();
    cc << cc.Indent() << "const uint32_t " << id << " = " << member
       << ".Find(" << row << ");\n";
    cond = id + " != ::hyde::rt::kNoRow && " + member + "." +
           PredicateMethod(region.Predicate()) + "(" + id + ")";
  }

  if (present) {
    cc << cc.Indent() << "if (" << cond << ") {\n";
    cc.PushIndent();
    EmitRegion(*present);
    cc.PopIndent();
    cc << cc.Indent() << "}";
    if (absent) {
      cc << " else {\n";
      cc.PushIndent();
      EmitRegion(*absent);
      cc.PopIndent();
      cc << cc.Indent() << "}";
    }
    cc << "\n";
  } else {
    cc << cc.Indent() << "if (!(" << cond << ")) {\n";
    cc.PushIndent();
    EmitRegion(*absent);
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  }

  if (scoped) {
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  }
}

void Generator::EmitCommitSweep(ProgramCommitSweepRegion region) {
  EmitComment(region);
  const auto table = region.Table();
  const auto member = table_member[table.Id()];
  const auto &fields = col_field[table.Id()];

  // R3: STATE_SEAL rides the commit-sweep tail — sealed := Emit(working) for
  // this agg table's StateCell, AFTER emit_touched read working as `new`
  // (spec §2.3 E5). Emitted at EVERY exit of the sweep.
  //
  // P2c config-@recompute fork (i) (A-F3): a config-DEPENDENT @recompute cell
  // seals per-touched-group via SealOne(gid, key.c<cfg>..) so SealFrom ->
  // ReduceLive gets the group's config; every other cell class keeps the opaque
  // bulk `statecell_<id>.Seal();` (byte-identical). `SealStateCellId()` returns
  // only the id, so look up the cell's algebra + config bits from
  // program.StateCells() (the config slice keys off KeyTypes().size(), the same
  // base as the DECL-side slice — a GroupUpdate `gpos` is not in scope here).
  const auto emit_seal = [&]() {
    auto id = region.SealStateCellId();
    if (!id) {
      return;
    }
    // `program.StateCells()` returns a fresh vector BY VALUE; a `ProgramState-
    // CellInfo` is a cheap `const void *impl` handle, so copy the matched one
    // out (never hold a pointer into the temporary vector — it dangles).
    bool found = false;
    bool is_recompute = false;
    unsigned nc = 0u;
    unsigned key_types_size = 0u;
    for (const ProgramStateCellInfo &c : program.StateCells()) {
      if (c.Id() == *id) {
        found = true;
        is_recompute = !c.IsInvertible();
        nc = c.NumConfigTypes();
        key_types_size = static_cast<unsigned>(c.KeyTypes().size());
        break;
      }
    }
    assert(found);
    (void) found;
    if (is_recompute && nc != 0u) {
      const unsigned first = key_types_size - nc;
      std::string seal_cfg;
      for (unsigned k = 0u; k < nc; ++k) {
        seal_cfg += ", key.c" + std::to_string(first + k);
      }
      cc << cc.Indent() << "for (const auto gid : statecell_" << *id
         << ".Touched()) {\n";
      cc.PushIndent();
      cc << cc.Indent() << "const auto &key = statecell_" << *id
         << ".KeyAt(gid);\n";
      cc << cc.Indent() << "statecell_" << *id << ".SealOne(gid" << seal_cfg
         << ");\n";
      cc.PopIndent();
      cc << cc.Indent() << "}\n";
      cc << cc.Indent() << "statecell_" << *id << ".ClearTouched();\n";
    } else {
      cc << cc.Indent() << "statecell_" << *id << ".Seal();\n";
    }
    cc << "#ifndef NDEBUG\n";
    cc << cc.Indent() << "statecell_" << *id << ".DebugValidate();\n";
    cc << "#endif\n";
  };

  // Monotone table: the sweep advances the sealed row-id watermark so the
  // next epoch's frozen-state reads see this epoch's rows.
  if (!table.IsDifferential()) {
    assert(!region.Message());
    cc << cc.Indent() << member << ".Seal();\n";
    emit_seal();
    return;
  }

  if (auto message = region.Message()) {
    cc << cc.Indent() << member << ".Commit([&](const "
       << row_type[table.Id()] << " &row, bool added) {\n";
    cc.PushIndent();
    cc << cc.Indent() << "log." << message->Name() << "_" << message->Arity()
       << "(";
    for (const auto &field : fields) {
      cc << "row." << field << ", ";
    }
    cc << "added);\n";
    cc.PopIndent();
    cc << cc.Indent() << "});\n";
  } else {
    cc << cc.Indent() << member << ".Commit([](const "
       << row_type[table.Id()] << " &, bool) {});\n";
  }

  // The generated database self-checks its counters after every batch.
  cc << "#ifndef NDEBUG\n";
  cc << cc.Indent() << member << ".DebugValidateCounts();\n";
  cc << "#endif\n";

  // Dead-row compaction, strictly after the validator (it is written
  // against the pre-compaction id space). Renumbering invalidates every
  // index over the table; the rebuild walk re-Adds each surviving row
  // under the index's key projection — the projection lives only here,
  // the runtime stores no per-row key. Tables compact independently;
  // the trigger keeps every suite-sized program below threshold.
  std::vector<DataIndex> live_indices;
  for (DataIndex index : table.Indices()) {
    if (index_member.contains(index.Id())) {
      live_indices.push_back(index);
    }
  }
  if (live_indices.empty()) {
    cc << cc.Indent() << member << ".CompactDead();\n";
    emit_seal();
    return;
  }
  const auto cid = "cid" + std::to_string(next_ins_id++);
  const auto crow = "crow" + std::to_string(next_ins_id++);
  cc << cc.Indent() << "if (" << member << ".CompactDead()) {\n";
  cc.PushIndent();
  for (DataIndex index : live_indices) {
    cc << cc.Indent() << index_member[index.Id()] << ".Clear();\n";
  }
  cc << cc.Indent() << "for (uint32_t " << cid << " = 0u; " << cid << " < "
     << member << ".NumRows(); ++" << cid << ") {\n";
  cc.PushIndent();
  cc << cc.Indent() << "const auto &" << crow << " = " << member << ".RowAt("
     << cid << ");\n";
  for (DataIndex index : live_indices) {
    std::vector<std::string> key_exprs;
    for (DataColumn col : index.KeyColumns()) {
      key_exprs.push_back(crow + "." + fields[col.Index()]);
    }
    cc << cc.Indent() << index_member[index.Id()] << ".Add("
       << RowExpr(key_exprs) << ", " << cid << ");\n";
  }
  cc.PopIndent();
  cc << cc.Indent() << "}\n";
  cc.PopIndent();
  cc << cc.Indent() << "}\n";
  emit_seal();
}

void Generator::EmitClaim(ProgramClaimRegion region) {
  EmitComment(region);
  const auto table = region.Table();
  if (!table.IsDifferential()) {
    Unsupported("a row claim on a monotone table");
  }
  const auto member = table_member[table.Id()];
  const auto row = RowExpr(VarExprs(region.TupleVariables()));
  const auto id = "id" + std::to_string(next_ins_id++);
  const char *method = region.IsDelete() ? "TryClaimDel" : "TryClaimAdd";

  cc << cc.Indent() << "{\n";
  cc.PushIndent();
  cc << cc.Indent() << "const auto " << id << " = " << member << ".Find("
     << row << ");\n";
  cc << cc.Indent() << "if (" << id << " != ::hyde::rt::kNoRow && " << member
     << "." << method << "(" << id << ")) {\n";
  cc.PushIndent();
  EmitOptional(region.Body());
  cc.PopIndent();
  cc << cc.Indent() << "}\n";
  cc.PopIndent();
  cc << cc.Indent() << "}\n";
}

void Generator::EmitRetire(ProgramRetireRegion region) {
  EmitComment(region);
  const auto table = region.Table();
  if (!table.IsDifferential()) {
    Unsupported("a frontier retirement on a monotone table");
  }
  const auto member = table_member[table.Id()];
  const auto row = RowExpr(VarExprs(region.TupleVariables()));
  const auto id = "id" + std::to_string(next_ins_id++);
  const char *method = region.IsDelete() ? "RetireDel" : "RetireAdd";

  cc << cc.Indent() << "{\n";
  cc.PushIndent();
  cc << cc.Indent() << "const auto " << id << " = " << member << ".Find("
     << row << ");\n";
  cc << cc.Indent() << "if (" << id << " != ::hyde::rt::kNoRow) {\n";
  cc.PushIndent();
  cc << cc.Indent() << member << "." << method << "(" << id << ");\n";
  cc.PopIndent();
  cc << cc.Indent() << "}\n";
  cc.PopIndent();
  cc << cc.Indent() << "}\n";
}

void Generator::EmitNetBatch(ProgramNetBatchRegion region) {
  EmitComment(region);
  cc << cc.Indent() << "::hyde::rt::NetBatch(" << VecName(region.AddVector())
     << ", " << VecName(region.RemoveVector()) << ");\n";
}

void Generator::EmitJoin(ProgramTableJoinRegion region) {
  const auto body = region.Body();
  const auto added_body = region.AddedBody();
  const auto removed_body = region.RemovedBody();
  if (!body && !added_body && !removed_body) {
    return;
  }
  EmitComment(region);

  const auto id = region.Id();
  const auto pivot_vars = VarExprs(region.OutputPivotVariables());

  // Row-binding scope: one entry per scanned side, live for the body and
  // the delta sections (all emitted inside the scan loops), restored
  // before returning so no entry outlives its cursor.
  const auto saved_row_scope = row_scope.size();

  // Per-side membership reads for the delta sections, applied to the row
  // ids the scans below hold in scope.
  std::vector<std::string> side_reads;

  // Per-side pivot-equality re-checks for the delta sections: the index
  // scans are approximate, so a joined combination requires every side's
  // scanned key columns to equal the pivot. The body path re-checks through
  // its TUPLECMP; the sections conjoin the equality directly.
  std::vector<std::string> side_key_eqs;

  // Pivot loop.
  cc << cc.Indent() << "for (auto [" << JoinExprs(pivot_vars, ", ") << "] : "
     << VecName(region.PivotVector()) << ") {\n";
  cc.PushIndent();

  const auto tables = region.Tables();
  unsigned depth = 1u;  // The pivot loop.

  for (auto i = 0u; i < tables.size(); ++i) {
    const auto table = tables[i];
    const auto member = table_member[table.Id()];
    const auto &fields = col_field[table.Id()];
    const auto indexed_cols = region.IndexedColumns(i);

    // The pivot variable expression for a given indexed column.
    const auto pivot_for_col = [&](DataColumn col) -> std::string {
      auto j = 0u;
      for (DataColumn used_col : indexed_cols) {
        if (used_col == col) {
          return pivot_vars[j];
        }
        ++j;
      }
      Unsupported("a join key column with no pivot variable");
    };

    const auto row = "r" + std::to_string(id) + "_" + std::to_string(i);
    const auto cursor = "j" + std::to_string(id) + "_" + std::to_string(i);
    const auto maybe_index = region.Index(i);

    if (maybe_index && index_member.contains(maybe_index->Id())) {
      // Walk the index chain for this pivot key.
      const auto index = *maybe_index;
      std::vector<std::string> key_exprs;
      for (DataColumn col : index.KeyColumns()) {
        key_exprs.push_back(pivot_for_col(col));
      }
      cc << cc.Indent() << "for (uint32_t " << cursor << " = "
         << index_member[index.Id()] << ".First(" << RowExpr(key_exprs)
         << "); " << cursor << " != ::hyde::rt::kNoRow; " << cursor << " = "
         << index_member[index.Id()] << ".Next(" << cursor << ")) {\n";
    } else {
      // The key covers every column: probe for the single matching row.
      std::vector<std::string> all_cols;
      all_cols.resize(fields.size());
      for (DataColumn col : table.Columns()) {
        all_cols[col.Index()] = pivot_for_col(col);
      }
      cc << cc.Indent() << "if (const uint32_t " << cursor << " = " << member
         << ".Find(" << RowExpr(all_cols) << "); " << cursor
         << " != ::hyde::rt::kNoRow) {\n";
    }
    cc.PushIndent();
    ++depth;
    cc << cc.Indent() << "const auto " << row << " = " << member << ".RowAt("
       << cursor << ");\n";

    side_reads.push_back(member + ".InNew(" + cursor + ")");
    side_reads.push_back(member + ".NetAdded(" + cursor + ")");
    side_reads.push_back(member + ".InI(" + cursor + ")");
    side_reads.push_back(member + ".NetDeleted(" + cursor + ")");

    std::string key_eq;
    for (DataColumn col : indexed_cols) {
      if (!key_eq.empty()) {
        key_eq += " && ";
      }
      key_eq += row + "." + fields[col.Index()] + " == " + pivot_for_col(col);
    }
    side_key_eqs.push_back(key_eq.empty() ? "true" : key_eq);

    // Bind this table's output variables to the scanned row, positionally
    // in table column order (the IR's guard regions rely on this).
    const auto out_vars = region.OutputVariables(i);
    assert(out_vars.size() == region.SelectedColumns(i).size());
    std::vector<std::string> bound_names;
    bound_names.reserve(out_vars.size());
    for (auto k = 0u; k < out_vars.size(); ++k) {
      bound_names.push_back(VarName(out_vars[k]));
      cc << cc.Indent() << "const auto " << bound_names.back() << " = "
         << row << "." << fields[k] << ";\n";
    }

    // Dense full-column bindings only; a partial cover can never satisfy
    // a full-arity CHECKMEMBER tuple.
    if (bound_names.size() == fields.size()) {
      row_scope.push_back({table.Id(), std::move(bound_names), cursor});
    }
  }

  if (body) {
    EmitRegion(*body);
  }

  // Delta sections: each per-combination predicate is a conjunction of the
  // sides' pivot-equality re-checks and snapshot reads, and a disjunction
  // of their net-change reads — one-byte flag reads (differential sides) or
  // watermark id comparisons (monotone sides) on the ids already in scope.
  const auto num_sides = tables.size();
  const auto emit_section = [&](ProgramRegion section, unsigned all_of,
                                unsigned one_of) {
    cc << cc.Indent() << "if (";
    for (auto i = 0u; i < num_sides; ++i) {
      cc << side_key_eqs[i] << " && " << side_reads[i * 4u + all_of]
         << " && ";
    }
    auto sep = "(";
    for (auto i = 0u; i < num_sides; ++i) {
      cc << sep << side_reads[i * 4u + one_of];
      sep = " || ";
    }
    cc << ")) {\n";
    cc.PushIndent();
    EmitRegion(section);
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  };

  if (added_body) {
    emit_section(*added_body, 0u /* InNew */, 1u /* NetAdded */);
  }
  if (removed_body) {
    emit_section(*removed_body, 2u /* InI */, 3u /* NetDeleted */);
  }

  row_scope.resize(saved_row_scope);

  for (auto i = 0u; i < depth; ++i) {
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  }
}

void Generator::EmitProduct(ProgramTableProductRegion region) {
  const auto body = region.Body();
  if (!body) {
    return;
  }
  EmitComment(region);

  const auto id = region.Id();
  const auto tables = region.Tables();

  // Staging vector holding the concatenated output tuples; the body may
  // mutate the scanned tables, so tuples are collected first.
  std::vector<TypeLoc> types;
  std::vector<std::string> all_vars;
  for (auto i = 0u; i < tables.size(); ++i) {
    for (DataVariable var : region.OutputVariables(i)) {
      types.push_back(var.Type());
      all_vars.push_back(VarName(var));
    }
  }
  const auto stage = "stage" + std::to_string(id);
  cc << cc.Indent() << "::hyde::rt::Vec<" << ShapeName(types) << "> " << stage
     << "(allocator);\n";

  for (auto i = 0u; i < tables.size(); ++i) {
    // Proposer i's new tuples...
    cc << cc.Indent() << "for (auto ["
       << JoinExprs(VarExprs(region.OutputVariables(i)), ", ") << "] : "
       << VecName(region.Vector(i)) << ") {\n";
    cc.PushIndent();
    unsigned depth = 1u;

    // ... crossed with the full contents of every other table.
    for (auto j = 0u; j < tables.size(); ++j) {
      if (i == j) {
        continue;
      }
      const auto member = table_member[tables[j].Id()];
      const auto &fields = col_field[tables[j].Id()];
      const auto cursor = "p" + std::to_string(id) + "_" + std::to_string(j);
      cc << cc.Indent() << "for (uint32_t " << cursor << " = 0; " << cursor
         << " < " << member << ".NumRows(); ++" << cursor << ") {\n";
      cc.PushIndent();
      ++depth;
      const auto row = "r" + std::to_string(id) + "_" + std::to_string(j);
      cc << cc.Indent() << "const auto " << row << " = " << member
         << ".RowAt(" << cursor << ");\n";
      auto k = 0u;
      for (DataVariable var : region.OutputVariables(j)) {
        cc << cc.Indent() << "const auto " << VarName(var) << " = " << row
           << "." << fields[k++] << ";\n";
      }
    }

    cc << cc.Indent() << stage << ".Add(" << RowExpr(all_vars) << ");\n";

    for (auto d = 0u; d < depth; ++d) {
      cc.PopIndent();
      cc << cc.Indent() << "}\n";
    }
  }

  // Run the body over the staged tuples.
  cc << cc.Indent() << "for (auto [" << JoinExprs(all_vars, ", ") << "] : "
     << stage << ") {\n";
  cc.PushIndent();
  EmitRegion(*body);
  cc.PopIndent();
  cc << cc.Indent() << "}\n";
}

void Generator::EmitScan(ProgramTableScanRegion region) {
  const auto body = region.Body();
  if (!body) {
    return;
  }
  EmitComment(region);

  const auto id = region.Id();
  const auto table = region.Table();
  const auto member = table_member[table.Id()];
  const auto &fields = col_field[table.Id()];
  const auto out_vars = region.OutputVariables();
  const auto cursor = "s" + std::to_string(id);
  const auto row = "r" + std::to_string(id);

  const auto bind_outputs = [&] {
    // One output variable per table column, bound in column order.
    auto k = 0u;
    for (DataVariable var : out_vars) {
      cc << cc.Indent() << "const auto " << VarName(var) << " = " << row
         << "." << fields[k++] << ";\n";
    }
  };

  // Row-binding scope: live for the scan body, restored before returning.
  // (Consulted only when a CHECKMEMBER is emitted, i.e. inside the body.)
  const auto saved_row_scope = row_scope.size();
  {
    std::vector<std::string> bound_names;
    bound_names.reserve(out_vars.size());
    for (DataVariable var : out_vars) {
      bound_names.push_back(VarName(var));
    }
    if (bound_names.size() == fields.size()) {
      row_scope.push_back({table.Id(), std::move(bound_names), cursor});
    }
  }

  // A keyed scan is only possible when the region binds a value for every
  // key column; a rescan with no (or partial) inputs walks the whole table.
  const auto input_vars = VarExprs(region.InputVariables());
  const auto maybe_index = region.Index();
  const bool keyed_chain = maybe_index &&
                           index_member.contains(maybe_index->Id()) &&
                           input_vars.size() == maybe_index->KeyColumns().size();
  const bool keyed_probe = maybe_index && !keyed_chain &&
                           input_vars.size() == fields.size();
  if (keyed_chain) {
    // Keyed scan via the index chain.
    cc << cc.Indent() << "for (uint32_t " << cursor << " = "
       << index_member[maybe_index->Id()] << ".First(" << RowExpr(input_vars)
       << "); " << cursor << " != ::hyde::rt::kNoRow; " << cursor << " = "
       << index_member[maybe_index->Id()] << ".Next(" << cursor << ")) {\n";
  } else if (keyed_probe) {
    // The key covers every column: probe for the one matching row.
    cc << cc.Indent() << "if (const uint32_t " << cursor << " = " << member
       << ".Find(" << RowExpr(input_vars) << "); " << cursor
       << " != ::hyde::rt::kNoRow) {\n";
  } else {
    // Full table scan.
    cc << cc.Indent() << "for (uint32_t " << cursor << " = 0; " << cursor
       << " < " << member << ".NumRows(); ++" << cursor << ") {\n";
  }
  cc.PushIndent();
  cc << cc.Indent() << "const auto " << row << " = " << member << ".RowAt("
     << cursor << ");\n";

  // A full scan standing in for a keyed scan re-checks the constrained
  // columns itself.
  if (!keyed_chain && !keyed_probe && !input_vars.empty()) {
    const auto indexed_cols = region.IndexedColumns();
    assert(indexed_cols.size() == input_vars.size());
    std::string cond;
    auto sep = "";
    for (auto k = 0u; k < input_vars.size(); ++k) {
      cond += sep + row + "." + fields[indexed_cols[k].Index()] +
              " == " + input_vars[k];
      sep = " && ";
    }
    cc << cc.Indent() << "if (" << cond << ") {\n";
    cc.PushIndent();
    bind_outputs();
    EmitRegion(*body);
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  } else {
    bind_outputs();
    EmitRegion(*body);
  }
  row_scope.resize(saved_row_scope);
  cc.PopIndent();
  cc << cc.Indent() << "}\n";
}

void Generator::EmitCompare(ProgramTupleCompareRegion region) {
  const auto true_body = region.BodyIfTrue();
  const auto false_body = region.BodyIfFalse();
  if (!true_body && !false_body) {
    return;
  }
  EmitComment(region);

  const auto lhs = VarExprs(region.LHS());
  const auto rhs = VarExprs(region.RHS());
  const auto op = region.Operator();

  std::string cond;
  if (op == ComparisonOperator::kEqual || op == ComparisonOperator::kNotEqual) {
    // Field-wise comparison, skipping trivially-equal pairs.
    std::vector<std::string> terms;
    for (auto i = 0u; i < lhs.size(); ++i) {
      if (lhs[i] != rhs[i]) {
        terms.push_back(lhs[i] + " == " + rhs[i]);
      }
    }
    if (terms.empty()) {
      // Trivially equal.
      if (op == ComparisonOperator::kEqual) {
        if (true_body) {
          EmitRegion(*true_body);
        }
      } else if (false_body) {
        EmitRegion(*false_body);
      }
      return;
    }
    cond = JoinExprs(terms, " && ");
    if (op == ComparisonOperator::kNotEqual) {
      cond = "!(" + cond + ")";
    }
  } else {
    // Lexicographic ordering. Operands may be constants, which render as
    // prvalue literals, so the tuples are built by value.
    cond = "std::make_tuple(" + JoinExprs(lhs, ", ") + ") ";
    cond += OperatorString(op);
    cond += " std::make_tuple(" + JoinExprs(rhs, ", ") + ")";
  }

  if (!true_body && false_body) {
    cc << cc.Indent() << "if (!(" << cond << ")) {\n";
    cc.PushIndent();
    EmitRegion(*false_body);
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
    return;
  }
  cc << cc.Indent() << "if (" << cond << ") {\n";
  cc.PushIndent();
  EmitRegion(*true_body);
  cc.PopIndent();
  cc << cc.Indent() << "}";
  if (false_body) {
    cc << " else {\n";
    cc.PushIndent();
    EmitRegion(*false_body);
    cc.PopIndent();
    cc << cc.Indent() << "}";
  }
  cc << "\n";
}

void Generator::EmitGenerate(ProgramGenerateRegion region) {
  EmitComment(region);
  const auto functor = region.Functor();
  const auto id = region.Id();
  const auto out_vars = region.OutputVariables();
  const auto results_body = region.BodyIfResults();
  const auto empty_body = region.BodyIfEmpty();

  std::string call;
  if (auto inline_name = functor.InlineName(Language::kCxx); inline_name) {
    call = *inline_name;
  } else {
    // P1 (ADL/functor-surface epoch): the MAP-delivery call is a FREE
    // function (unqualified lookup, bound at template-definition point —
    // builtin args have no associated namespace so this is ordinary
    // lookup, not ADL; E-19), matching the C-5 reduction free-fn ABI.
    // The free forward-decl is emitted at the EmitFunctorsDecl slot,
    // before every detail template body (two-phase lookup).
    call = Sanitize(ToString(functor.Name())) + "_" +
           std::string(ParsedDeclaration(functor).BindingPattern());
  }
  call += "(" + JoinExprs(VarExprs(region.InputVariables()), ", ") + ")";

  const auto counter = "n" + std::to_string(id);
  if (empty_body) {
    cc << cc.Indent() << "size_t " << counter << " = 0;\n";
  }

  const auto emit_results = [&] {
    if (empty_body) {
      cc << cc.Indent() << "++" << counter << ";\n";
    }
    if (results_body) {
      EmitRegion(*results_body);
    }
  };

  const auto tmp = "t" + std::to_string(id);
  const auto bind_from = [&](const std::string &expr) {
    if (out_vars.size() == 1u) {
      cc << cc.Indent() << "const auto " << VarName(out_vars[0]) << " = "
         << expr << ";\n";
    } else {
      auto k = 0u;
      for (DataVariable var : out_vars) {
        cc << cc.Indent() << "const auto " << VarName(var) << " = std::get<"
           << (k++) << ">(" << expr << ");\n";
      }
    }
  };

  switch (functor.Range()) {
    case FunctorRange::kZeroOrOne:
      if (out_vars.empty()) {
        // Filter functor.
        cc << cc.Indent() << "if (" << call << ") {\n";
        cc.PushIndent();
        emit_results();
        cc.PopIndent();
        cc << cc.Indent() << "}\n";
      } else {
        const bool nullable =
            out_vars.size() == 1u &&
            out_vars[0].Type().IsNullable(module, Language::kCxx);
        cc << cc.Indent() << "if (auto " << tmp << " = " << call << "; "
           << tmp << ") {\n";
        cc.PushIndent();
        if (nullable) {
          cc << cc.Indent() << "const auto " << VarName(out_vars[0]) << " = "
             << tmp << ";\n";
        } else if (out_vars.size() == 1u) {
          cc << cc.Indent() << "const auto " << VarName(out_vars[0]) << " = *"
             << tmp << ";\n";
        } else {
          bind_from("*" + tmp);
        }
        emit_results();
        cc.PopIndent();
        cc << cc.Indent() << "}\n";
      }
      break;

    case FunctorRange::kOneToOne: {
      cc << cc.Indent() << "const auto " << tmp << " = " << call << ";\n";
      bind_from(tmp);
      emit_results();
      break;
    }

    case FunctorRange::kZeroOrMore:
    case FunctorRange::kOneOrMore: {
      cc << cc.Indent() << "for (const auto &" << tmp << " : " << call
         << ") {\n";
      cc.PushIndent();
      bind_from(tmp);
      emit_results();
      if (!results_body && empty_body) {
        // One witness is enough for the emptiness test.
        cc << cc.Indent() << "break;\n";
      }
      cc.PopIndent();
      cc << cc.Indent() << "}\n";
      break;
    }
  }

  if (empty_body) {
    cc << cc.Indent() << "if (!" << counter << ") {\n";
    cc.PushIndent();
    EmitRegion(*empty_body);
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  }
}

void Generator::EmitInduction(ProgramInductionRegion region) {
  EmitComment(region);
  if (auto init = region.Initializer()) {
    EmitRegion(*init);
  }

  const auto vectors_empty = [&](void) {
    cc << "(true";
    for (DataVector vec : region.Vectors()) {
      cc << " && " << VecName(vec) << ".Empty()";
    }
    cc << ")";
  };

  const auto changed = "changed" + std::to_string(region.Id());
  cc << cc.Indent() << "for (bool " << changed << " = true; " << changed
     << "; " << changed << " = !";
  vectors_empty();
  cc << ") {\n";
  cc.PushIndent();
  EmitRegion(region.FixpointLoop());
  cc.PopIndent();
  cc << cc.Indent() << "}\n";

  if (auto output = region.Output()) {
    EmitRegion(*output);
  }
}

// -----------------------------------------------------------------------

void Generator::Run(void) {
  ComputeNames();
  CollectVectorShapes();

  // ---- Header: the whole artifact. `cc` aliases the same stream (the
  // anchor TU is written by GenerateDatabaseCode), so the procedure
  // definitions land here, after the class.
  hh << "// Auto-generated file; do not edit.\n\n"
     << "#pragma once\n\n"
     << "#include <drlojekyll/Runtime/Allocator.h>\n"
     << "#include <drlojekyll/Runtime/Hash.h>\n"
     << "#include <drlojekyll/Runtime/Table.h>\n";
  // R3: the StateCell store header is emitted ONLY when a program instantiates
  // a store (a byte-identity guard for aggregate-free programs — the LOW note
  // in the r3cd critique).
  if (!program.StateCells().empty()) {
    hh << "#include <drlojekyll/Runtime/StateCell.h>\n";
  }
  hh << "#include <drlojekyll/Runtime/Vec.h>\n\n"
     << "#include <cassert>\n"
     << "#include <cstdint>\n"
     << "#include <optional>\n"
     << "#include <tuple>\n"
     << "#include <utility>\n"
     << "#include <vector>\n\n";

  EmitInlines(hh, "c++:database:prologue");

  if (!ns_name.empty()) {
    hh << "namespace " << ns_name << " {\n\n";
  }
  EmitInlines(hh, "c++:database:prologue:namespace");

  EmitInlines(hh, "c++:database:enums:prologue");
  EmitEnums();
  EmitInlines(hh, "c++:database:enums:epilogue");

  EmitShapeStructs();

  // Friendly aliases for each message's input-tuple shape.
  for (ProgramProcedure proc : program.Procedures()) {
    if (proc.Kind() != ProcedureKind::kMessageHandler) {
      continue;
    }
    auto vec_params = proc.VectorParameters();
    if (vec_params.size() == 1u) {
      hh << "using " << Sanitize(ToString(proc.Message()->Name()))
         << "_input = " << VecType(vec_params[0]) << ";\n";
    }
  }
  hh << "\n";

  EmitRowStructs();
  EmitStateCellStructs();  // R3: Key_<id> + Reduce_<id> per state cell.
  EmitFunctorsDecl();
  EmitLogDecl();

  // Forward declarations for every detail function BEFORE the class:
  // hidden-friend bodies whose intra-detail calls carry no dependent
  // argument bind at template-definition time (and non-template query
  // friends see only pre-class names).
  EmitDetailDecls(hh);

  EmitDatabaseDecl();

  // The detail definitions (via the aliased `cc`).
  for (ProgramProcedure proc : program.Procedures()) {
    EmitProcedure(proc);
  }

  EmitInlines(hh, "c++:database:epilogue:namespace");
  if (!ns_name.empty()) {
    hh << "}  // namespace " << ns_name << "\n";
  }
  EmitInlines(hh, "c++:database:epilogue");
}

}  // namespace

void GenerateDatabaseCode(const Program &program, OutputStream &os_h,
                          OutputStream &os_cc, std::string_view header_name) {
  // The generated database is header-only: the Generator writes the
  // whole artifact into the header stream (passed as both `hh` and
  // `cc`). The source file survives as an anchor translation unit so
  // build systems that compile one TU per database (static libraries,
  // explicit compile lines) keep working unchanged.
  Generator(program, os_h, os_h, header_name).Run();
  os_cc << "// Auto-generated file; do not edit.\n"
        << "//\n"
        << "// Anchor translation unit: the generated database is "
           "header-only; this\n"
        << "// file exists so build systems that compile one TU per "
           "database (static\n"
        << "// libraries, explicit compile lines) keep working "
           "unchanged.\n\n"
        << "#include \"" << header_name << "\"\n";
}

}  // namespace cxx
}  // namespace hyde
