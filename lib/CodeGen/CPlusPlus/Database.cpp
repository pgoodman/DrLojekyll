// Copyright 2026, Trail of Bits. All rights reserved.
//
// The C++ backend. Emits a concrete implementation of a Datalog program:
// a header with named row structs, enums, and a `Database` class, plus a
// source file with the procedure and query-cursor definitions. The emitted
// code contains no templates; storage is provided by the small concrete
// runtime (`hyde::rt::Table`, `hyde::rt::Index`, `hyde::rt::Vec`) and all
// allocation goes through an explicit `hyde::rt::Allocator`.

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
  // Header emission.

  void EmitInlines(OutputStream &os, const char *stage);
  void EmitEnums(void);
  void EmitShapeStructs(void);
  void EmitHashStruct(const std::string &name,
                      const std::vector<std::pair<std::string, std::string>>
                          &typed_fields);
  void EmitRowStructs(void);
  void EmitFunctorsDecl(void);
  void EmitLogDecl(void);
  void EmitDatabaseDecl(void);

  // ---------------------------------------------------------------------
  // Source emission.

  void EmitConstructor(void);
  void EmitProcedure(ProgramProcedure proc);
  void EmitQueries(void);

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

  unsigned next_ins_id{0};
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

void Generator::EmitFunctorsDecl(void) {
  EmitInlines(hh, "c++:database:functors:prologue");
  hh << "// User-provided functors. Define the declared member functions in\n"
     << "// your own translation unit; the generated code calls them.\n"
     << "struct DatabaseFunctors {\n";
  hh.PushIndent();
  EmitInlines(hh, "c++:database:functors:definition:prologue");
  for (ParsedFunctor func : Functors(module)) {
    if (func.IsInline(Language::kCxx)) {
      continue;
    }
    const ParsedDeclaration decl(func);

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

    hh << hh.Indent() << ret << " " << func.Name() << "_"
       << decl.BindingPattern() << "(";
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
  EmitInlines(hh, "c++:database:functors:definition:epilogue");
  hh.PopIndent();
  hh << "};\n\n";
  EmitInlines(hh, "c++:database:functors:epilogue");
}

void Generator::EmitLogDecl(void) {
  EmitInlines(hh, "c++:database:log:prologue");
  hh << "// Receives published messages. The default methods do nothing;\n"
     << "// they are virtual so a driver can observe the published delta\n"
     << "// stream (a @differential message publishes one call per net\n"
     << "// presence change at each batch's commit sweep).\n"
     << "struct DatabaseLog {\n";
  hh.PushIndent();
  hh << hh.Indent() << "virtual ~DatabaseLog(void) = default;\n";
  EmitInlines(hh, "c++:database:log:definition:prologue");
  for (ParsedMessage message : Messages(module)) {
    if (!message.IsPublished()) {
      continue;
    }
    hh << hh.Indent() << "virtual void " << message.Name() << "_"
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
  hh << "class Database {\n"
     << " public:\n";
  hh.PushIndent();
  hh << hh.Indent()
     << "explicit Database(::hyde::rt::Allocator allocator_, DatabaseLog "
        "&log_, DatabaseFunctors &functors_);\n\n";

  // Message entry points.
  for (ProgramProcedure proc : program.Procedures()) {
    if (proc.Kind() != ProcedureKind::kMessageHandler) {
      continue;
    }
    hh << hh.Indent() << "// Message `" << proc.Message()->Name() << "/"
       << proc.Message()->Arity() << "`.\n";
    hh << hh.Indent() << "bool " << ProcName(proc) << "(";
    auto sep = "";
    for (DataVector vec : proc.VectorParameters()) {
      hh << sep << "::hyde::rt::Vec<" << VecType(vec) << "> " << VecName(vec);
      sep = ", ";
    }
    for (DataVariable var : proc.VariableParameters()) {
      hh << sep << TypeName(module, var.Type()) << " " << VarName(var);
      sep = ", ";
    }
    hh << ");\n\n";
  }

  // Query entry points and their cursors.
  for (const ProgramQuery &spec : program.Queries()) {
    const ParsedDeclaration decl(spec.query);
    const auto name =
        Sanitize(ToString(decl.Name())) + "_" + std::string(decl.BindingPattern());

    std::vector<ParsedParameter> bound_params, free_params;
    for (ParsedParameter param : decl.Parameters()) {
      (param.Binding() == ParameterBinding::kBound ? bound_params
                                                   : free_params)
          .push_back(param);
    }

    hh << hh.Indent() << "// Query `" << decl.Name() << "/" << decl.Arity()
       << "` (" << decl.BindingPattern() << ").\n";

    if (free_params.empty()) {
      hh << hh.Indent() << "bool " << name << "(";
      auto sep = "";
      for (ParsedParameter param : bound_params) {
        hh << sep << TypeName(module, param.Type()) << " "
           << Sanitize(ToString(param.Name()));
        sep = ", ";
      }
      hh << ");\n\n";
      continue;
    }

    hh << hh.Indent() << "struct " << name << "_cursor {\n";
    hh.PushIndent();
    hh << hh.Indent() << "Database &db;\n";
    for (ParsedParameter param : bound_params) {
      hh << hh.Indent() << TypeName(module, param.Type()) << " "
         << Sanitize(ToString(param.Name())) << ";\n";
    }
    hh << hh.Indent() << "uint32_t pos;\n";
    hh << hh.Indent() << "bool next(";
    auto sep = "";
    for (ParsedParameter param : free_params) {
      hh << sep << TypeName(module, param.Type()) << " &"
         << Sanitize(ToString(param.Name()));
      sep = ", ";
    }
    hh << ");\n";
    hh.PopIndent();
    hh << hh.Indent() << "};\n";

    hh << hh.Indent() << name << "_cursor " << name << "(";
    sep = "";
    for (ParsedParameter param : bound_params) {
      hh << sep << TypeName(module, param.Type()) << " "
         << Sanitize(ToString(param.Name()));
      sep = ", ";
    }
    hh << ");\n\n";
  }

  hh.PopIndent();
  hh << " private:\n";
  hh.PushIndent();

  // Internal procedures.
  for (ProgramProcedure proc : program.Procedures()) {
    if (proc.Kind() == ProcedureKind::kMessageHandler) {
      continue;
    }
    hh << hh.Indent() << "bool " << ProcName(proc) << "(";
    auto sep = "";
    const auto by_value = TakesVectorsByValue(proc);
    for (DataVector vec : proc.VectorParameters()) {
      hh << sep << "::hyde::rt::Vec<" << VecType(vec) << "> "
         << (by_value ? "" : "&") << VecName(vec);
      sep = ", ";
    }
    for (DataVariable var : proc.VariableParameters()) {
      hh << sep << TypeName(module, var.Type()) << " " << VarName(var);
      sep = ", ";
    }
    hh << ");\n";
  }
  hh << "\n";

  hh << hh.Indent() << "::hyde::rt::Allocator allocator;\n"
     << hh.Indent() << "[[maybe_unused]] DatabaseLog &log;\n"
     << hh.Indent() << "[[maybe_unused]] DatabaseFunctors &functors;\n\n";

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

  // Mutable globals (condition ref-counts, init guards, fixpoint depth).
  for (DataVariable var : program.GlobalVariables()) {
    hh << hh.Indent() << TypeName(module, var.Type()) << " " << VarName(var)
       << " = 0;\n";
  }

  hh.PopIndent();
  hh << "};\n\n";
}

// -----------------------------------------------------------------------
// Source emission.

void Generator::EmitConstructor(void) {
  cc << "Database::Database(::hyde::rt::Allocator allocator_, DatabaseLog "
        "&log_, DatabaseFunctors &functors_)\n"
     << "    : allocator(allocator_),\n"
     << "      log(log_),\n"
     << "      functors(functors_)";
  for (DataTable table : program.Tables()) {
    cc << ",\n      " << table_member[table.Id()] << "(allocator_)";
    for (DataIndex index : table.Indices()) {
      if (auto it = index_member.find(index.Id()); it != index_member.end()) {
        cc << ",\n      " << it->second << "(allocator_)";
      }
    }
  }
  cc << " {\n";
  cc.PushIndent();
  for (ProgramProcedure proc : program.Procedures()) {
    if (proc.Kind() == ProcedureKind::kInitializer) {
      cc << cc.Indent() << ProcName(proc) << "();\n";
      break;
    }
  }
  cc.PopIndent();
  cc << "}\n\n";
}

void Generator::EmitProcedure(ProgramProcedure proc) {
  cc << "bool Database::" << ProcName(proc) << "(";
  auto sep = "";
  const auto by_value = TakesVectorsByValue(proc);
  for (DataVector vec : proc.VectorParameters()) {
    cc << sep << "::hyde::rt::Vec<" << VecType(vec) << "> "
       << (by_value ? "" : "&") << VecName(vec);
    sep = ", ";
  }
  for (DataVariable var : proc.VariableParameters()) {
    cc << sep << TypeName(module, var.Type()) << " " << VarName(var);
    sep = ", ";
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

  std::string call = ProcName(callee) + "(";
  auto sep = "";
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
  const auto row = RowExpr(VarExprs(region.TupleVariables()));

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
        cond = member + ".Find(" + row + ") != ::hyde::rt::kNoRow";
        scoped = false;
        break;
      case MembershipPredicate::kInI:
      case MembershipPredicate::kNetAdded:
      case MembershipPredicate::kNetDeleted:
        break;
      default:
        Unsupported("a fixpoint-round membership read on a monotone table");
    }
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

  // Monotone table: the sweep advances the sealed row-id watermark so the
  // next epoch's frozen-state reads see this epoch's rows.
  if (!table.IsDifferential()) {
    assert(!region.Message());
    cc << cc.Indent() << member << ".Seal();\n";
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
    for (auto k = 0u; k < out_vars.size(); ++k) {
      cc << cc.Indent() << "const auto " << VarName(out_vars[k]) << " = "
         << row << "." << fields[k] << ";\n";
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
    call = "functors." + Sanitize(ToString(functor.Name())) + "_" +
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
// Queries.

void Generator::EmitQueries(void) {
  for (const ProgramQuery &spec : program.Queries()) {
    const ParsedDeclaration decl(spec.query);
    const auto name =
        Sanitize(ToString(decl.Name())) + "_" + std::string(decl.BindingPattern());
    const auto table = spec.table;
    const auto member = table_member[table.Id()];
    const auto &fields = col_field[table.Id()];

    // Query parameters map 1:1, in order, to the backing table's columns.
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

    // The liveness filter applied to each candidate row: a count-based
    // presence read on a differential table; nothing on a monotone table,
    // whose stored rows are present forever.
    const bool differential = table.IsDifferential();
    const auto emit_row_filter = [&](const std::string &row,
                                     const std::string &id_expr) {
      (void) row;
      if (!differential) {
        return;
      }
      cc << cc.Indent() << "if (!db." << member << ".Present(" << id_expr
         << ")) {\n";
      cc.PushIndent();
      cc << cc.Indent() << "continue;\n";
      cc.PopIndent();
      cc << cc.Indent() << "}\n";
    };

    if (!has_free) {
      // Existence check.
      cc << "bool Database::" << name << "(";
      auto sep = "";
      for (auto i = 0u; i < params.size(); ++i) {
        cc << sep << TypeName(module, params[i].Type()) << " "
           << param_names[i];
        sep = ", ";
      }
      cc << ") {\n";
      cc.PushIndent();
      if (spec.forcing_function) {
        cc << cc.Indent() << ProcName(*spec.forcing_function) << "("
           << JoinExprs(param_names, ", ") << ");\n";
      }
      if (differential) {
        cc << cc.Indent() << "const uint32_t id = " << member << ".Find("
           << RowExpr(param_names) << ");\n";
        cc << cc.Indent()
           << "return id != ::hyde::rt::kNoRow && " << member
           << ".Present(id);\n";
      } else {
        cc << cc.Indent() << "return " << member << ".Find("
           << RowExpr(param_names) << ") != ::hyde::rt::kNoRow;\n";
      }
      cc.PopIndent();
      cc << "}\n\n";
      continue;
    }

    std::vector<std::string> bound_names, free_names;
    for (auto i = 0u; i < params.size(); ++i) {
      (is_bound[i] ? bound_names : free_names).push_back(param_names[i]);
    }

    // Factory.
    cc << "Database::" << name << "_cursor Database::" << name << "(";
    auto sep = "";
    for (auto i = 0u; i < params.size(); ++i) {
      if (is_bound[i]) {
        cc << sep << TypeName(module, params[i].Type()) << " "
           << param_names[i];
        sep = ", ";
      }
    }
    cc << ") {\n";
    cc.PushIndent();
    if (spec.forcing_function) {
      cc << cc.Indent() << ProcName(*spec.forcing_function) << "("
         << JoinExprs(bound_names, ", ") << ");\n";
    }
    cc << cc.Indent() << "return {*this";
    for (const auto &p : bound_names) {
      cc << ", " << p;
    }
    if (spec.index && index_member.contains(spec.index->Id())) {
      std::vector<std::string> key_exprs;
      for (DataColumn col : spec.index->KeyColumns()) {
        key_exprs.push_back(param_names[col.Index()]);
      }
      cc << ", " << index_member[spec.index->Id()] << ".First("
         << RowExpr(key_exprs) << ")";
    } else {
      cc << ", 0";
    }
    cc << "};\n";
    cc.PopIndent();
    cc << "}\n\n";

    // Cursor::next.
    cc << "bool Database::" << name << "_cursor::next(";
    sep = "";
    for (auto i = 0u; i < params.size(); ++i) {
      if (!is_bound[i]) {
        cc << sep << TypeName(module, params[i].Type()) << " &"
           << param_names[i];
        sep = ", ";
      }
    }
    cc << ") {\n";
    cc.PushIndent();

    const bool via_index =
        spec.index && index_member.contains(spec.index->Id());
    if (via_index) {
      cc << cc.Indent() << "while (pos != ::hyde::rt::kNoRow) {\n";
      cc.PushIndent();
      cc << cc.Indent() << "const uint32_t id = pos;\n";
      cc << cc.Indent() << "pos = db." << index_member[spec.index->Id()]
         << ".Next(id);\n";
    } else {
      cc << cc.Indent() << "while (pos < db." << member << ".NumRows()) {\n";
      cc.PushIndent();
      cc << cc.Indent() << "const uint32_t id = pos++;\n";
    }
    cc << cc.Indent() << "const auto row = db." << member << ".RowAt(id);\n";
    emit_row_filter("row", "id");
    if (!via_index) {
      // Re-check bound columns (a full scan is unkeyed).
      for (auto i = 0u; i < params.size(); ++i) {
        if (is_bound[i]) {
          cc << cc.Indent() << "if (row." << fields[i]
             << " != " << param_names[i] << ") {\n";
          cc.PushIndent();
          cc << cc.Indent() << "continue;\n";
          cc.PopIndent();
          cc << cc.Indent() << "}\n";
        }
      }
    }

    for (auto i = 0u; i < params.size(); ++i) {
      if (!is_bound[i]) {
        cc << cc.Indent() << param_names[i] << " = row." << fields[i]
           << ";\n";
      }
    }
    cc << cc.Indent() << "return true;\n";
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
    cc << cc.Indent() << "return false;\n";
    cc.PopIndent();
    cc << "}\n\n";
  }
}

// -----------------------------------------------------------------------

void Generator::Run(void) {
  ComputeNames();
  CollectVectorShapes();

  // ---- Header.
  hh << "// Auto-generated file; do not edit.\n\n"
     << "#pragma once\n\n"
     << "#include <drlojekyll/Runtime/Allocator.h>\n"
     << "#include <drlojekyll/Runtime/Hash.h>\n"
     << "#include <drlojekyll/Runtime/Table.h>\n"
     << "#include <drlojekyll/Runtime/Vec.h>\n\n"
     << "#include <cstdint>\n"
     << "#include <optional>\n"
     << "#include <tuple>\n"
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
  EmitFunctorsDecl();
  EmitLogDecl();
  EmitDatabaseDecl();

  EmitInlines(hh, "c++:database:epilogue:namespace");
  if (!ns_name.empty()) {
    hh << "}  // namespace " << ns_name << "\n";
  }
  EmitInlines(hh, "c++:database:epilogue");

  // ---- Source.
  cc << "// Auto-generated file; do not edit.\n\n"
     << "#include \"" << header_name << "\"\n\n"
     << "#include <tuple>\n"
     << "#include <utility>\n\n";
  if (!ns_name.empty()) {
    cc << "namespace " << ns_name << " {\n\n";
  }

  EmitConstructor();
  for (ProgramProcedure proc : program.Procedures()) {
    EmitProcedure(proc);
  }
  EmitQueries();

  if (!ns_name.empty()) {
    cc << "}  // namespace " << ns_name << "\n";
  }
}

}  // namespace

void GenerateDatabaseCode(const Program &program, OutputStream &os_h,
                          OutputStream &os_cc, std::string_view header_name) {
  Generator(program, os_h, os_cc, header_name).Run();
}

}  // namespace cxx
}  // namespace hyde
