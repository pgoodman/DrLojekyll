// Copyright 2020, Trail of Bits. All rights reserved.

#include <drlojekyll/ControlFlow/Format.h>
#include <drlojekyll/DataFlow/Format.h>
#include <drlojekyll/Display/Format.h>
#include <drlojekyll/Lex/Format.h>
#include <drlojekyll/Parse/Format.h>

#include "Program.h"

namespace hyde {
namespace {

static OutputStream &Type(OutputStream &os, ParsedModule module,
                          TypeLoc kind) {
  if (auto type = module.ForeignType(kind); type) {
    os << type->Name();
  } else {
    os << kind;
  }
  return os;
}

static OutputStream &Type(OutputStream &os, ParsedModule module,
                          DataColumn col) {
  return Type(os, module, col.Type());
}


static OutputStream &Type(OutputStream &os, ParsedModule module,
                          DataVariable var) {
  return Type(os, module, var.Type().Kind());
}

static void DefineTable(OutputStream &os, ParsedModule module,
                        DataTable table) {
  os << os.Indent() << "create " << table;
  os.PushIndent();
  for (auto col : table.Columns()) {
    os << '\n';
    os << os.Indent() << Type(os, module, col) << '\t' << col;
    auto sep = "\t; ";
    for (auto name : col.PossibleNames()) {
      os << sep << name;
      sep = ", ";
    }
  }

  auto index_sep = "\n\n";
  for (auto index : table.Indices()) {
    os << index_sep << os.Indent() << index;
    auto sep = " on ";
    for (auto col : index.KeyColumns()) {
      os << sep << col;
      sep = ", ";
    }
    index_sep = "\n";
  }

  os.PopIndent();
}

}  // namespace

OutputStream &operator<<(OutputStream &os, DataColumn col) {
  os << "%col:" << col.Id();
  return os;
}

OutputStream &operator<<(OutputStream &os, DataIndex index) {
  os << "%index:" << index.Id();
  auto sep = "[";
  auto i = 0u;
  for (auto col : index.KeyColumns()) {
    for (; i < col.Index(); ++i) {
      os << sep << '_';
      sep = ",";
    }
    os << sep << col.Type();
    i = col.Index() + 1u;
    sep = ",";
  }
  auto table = DataTable::Backing(index);
  for (auto max_i = table.Columns().size(); i < max_i; ++i) {
    os << sep << '_';
    sep = ",";
  }
  os << ']';
  return os;
}

OutputStream &operator<<(OutputStream &os, DataTable table) {
  os << "%table:" << table.Id();
  auto sep = "[";
  for (auto col : table.Columns()) {
    os << sep << col.Type();
    sep = ",";
  }
  os << ']';
  return os;
}

OutputStream &operator<<(OutputStream &os, DataVector vec) {

  switch (vec.Kind()) {
    case VectorKind::kParameter: os << "$param"; break;
    case VectorKind::kInductionInputs: os << "$induction_in"; break;
    case VectorKind::kInductionSwaps: os << "$induction_swap"; break;
    case VectorKind::kInductionOutputs: os << "$induction_out"; break;
    case VectorKind::kJoinPivots: os << "$pivots"; break;
    case VectorKind::kInductiveJoinPivots: os << "$induction_pivots"; break;
    case VectorKind::kInductiveJoinPivotSwaps:
      os << "$induction_pivots_swap";
      break;
    case VectorKind::kProductInput: os << "$product"; break;
    case VectorKind::kInductiveProductInput: os << "$induction_product"; break;
    case VectorKind::kInductiveProductSwaps:
      os << "$induction_product_swap";
      break;
    case VectorKind::kTableScan: os << "$scan"; break;
    case VectorKind::kMessageOutputs: os << "$publish"; break;
    case VectorKind::kEmpty: os << "$empty"; break;
    case VectorKind::kDeleteQueue: os << "$delete_queue"; break;
    case VectorKind::kAddQueue: os << "$add_queue"; break;
    case VectorKind::kOverdeleteSet: os << "$overdelete"; break;
    case VectorKind::kAdditionSet: os << "$addition"; break;
    case VectorKind::kNetRemovals: os << "$net_removals"; break;
    case VectorKind::kNetAdditions: os << "$net_additions"; break;
  }

  os << ':' << vec.Id();
  auto sep = "<";
  for (auto type_kind : vec.ColumnTypes()) {
    os << sep << type_kind;
    sep = ",";
  }
  os << '>';
  return os;
}

OutputStream &operator<<(OutputStream &os, DataVariable var) {
  os << '@';
  if (auto name = var.Name(); name.Lexeme() == Lexeme::kIdentifierAtom ||
                              name.Lexeme() == Lexeme::kIdentifierVariable ||
                              name.Lexeme() == Lexeme::kIdentifierConstant) {
    os << name << ':';
  } else if (auto const_val = var.Value()) {
    if (const_val->IsTag()) {
      auto tag = QueryTag::From(*const_val);
      os << "TAG:" << tag.Value() << ':';
    } else if (auto lit = const_val->Literal(); lit && lit->IsConstant()) {
      os << *lit << ':';
    }
  }
  os << var.Id();
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramPublishRegion region) {
  auto message = region.Message();
  os << os.Indent() << "publish ";

  if (message.IsDifferential()) {
    if (region.IsRemoval()) {
      os << "remove ";
    } else {
      os << "add ";
    }
  }

  os << message.Name() << '/' << message.Arity();
  auto sep = "(";
  auto end = "";
  for (auto arg : region.VariableArguments()) {
    os << sep << arg;
    sep = ", ";
    end = ")";
  }
  os << end;
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramCallRegion region) {
  os << os.Indent();

  os << "call " << region.CalledProcedure();
  auto sep = "(";
  auto end = "";
  for (auto arg : region.VectorArguments()) {
    os << sep << arg;
    sep = ", ";
    end = ")";
  }
  for (auto arg : region.VariableArguments()) {
    os << sep << arg;
    sep = ", ";
    end = ")";
  }
  os << end;

  os.PushIndent();

  if (auto true_body = region.BodyIfTrue(); true_body) {
    os << '\n' << os.Indent() << "if-true\n";
    os.PushIndent();
    os << *true_body;
    os.PopIndent();
  }

  if (auto false_body = region.BodyIfFalse(); false_body) {
    os << '\n' << os.Indent() << "if-false\n";
    os.PushIndent();
    os << *false_body;
    os.PopIndent();
  }

  os.PopIndent();

  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramReturnRegion region) {
  auto ret = "return-false";
  if (region.ReturnsTrue()) {
    ret = "return-true";
  }
  os << os.Indent() << ret;
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramTupleCompareRegion region) {
  os << os.Indent() << "if-compare {";
  auto sep = "";
  for (auto var : region.LHS()) {
    os << sep << var;
    sep = ", ";
  }

  os << "} " << region.Operator() << " {";

  sep = "";
  for (auto var : region.RHS()) {
    os << sep << var;
    sep = ", ";
  }
  os << "}";
  auto has_body = false;

  if (auto maybe_true_body = region.BodyIfTrue(); maybe_true_body) {
    os << '\n';
    os.PushIndent();
    os << os.Indent() << "if-true\n";
    os.PushIndent();
    os << (*maybe_true_body);
    os.PopIndent();
    os.PopIndent();
    has_body = true;
  }

  if (auto maybe_false_body = region.BodyIfFalse(); maybe_false_body) {
    os << '\n';
    os.PushIndent();
    os << os.Indent() << "if-false\n";
    os.PushIndent();
    os << (*maybe_false_body);
    os.PopIndent();
    os.PopIndent();
    has_body = true;
  }

  if (!has_body) {
    os << '\n' << os.Indent() << "empty-compare";
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramTestAndSetRegion region) {
  os << os.Indent();

  const auto acc = region.Accumulator();

  if (auto maybe_body = region.Body(); maybe_body) {
    os << "if (" << acc << " += 1) == 1\n";
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();

  } else {
    os << acc << " += 1";
  }

  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramGenerateRegion region) {
  auto functor = region.Functor();
  os << os.Indent();
  if (!region.IsFilter()) {
    const char *sep = "assign {";
    for (auto var : region.OutputVariables()) {
      os << sep << var;
      sep = ", ";
    }
    os << "} from ";
  }

  os << functor.Name();

  auto sep = "(";
  auto i = 0u;
  auto input_vars = region.InputVariables();

  ParsedDeclaration decl(functor);
  for (ParsedParameter param : decl.Parameters()) {
    if (param.Binding() == ParameterBinding::kFree) {
      os << sep << '_';
    } else {
      os << sep << input_vars[i++];
    }
    sep = ", ";
  }

  os << ')';

  auto has_body = false;
  os.PushIndent();

  if (auto true_body = region.BodyIfResults(); true_body) {
    os << '\n' << os.Indent() << "if-results\n";
    os.PushIndent();
    os << *true_body;
    os.PopIndent();
    has_body = true;
  }

  if (auto false_body = region.BodyIfEmpty(); false_body) {
    os << '\n' << os.Indent() << "if-empty\n";
    os.PushIndent();
    os << *false_body;
    os.PopIndent();
    has_body = true;
  }

  if (!has_body) {
    os << os.Indent() << "\n" << os.Indent() << "empty?!\n";
  }

  os.PopIndent();

  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramLetBindingRegion region) {
  if (auto maybe_body = region.Body(); maybe_body) {
    if (region.DefinedVariables().empty()) {
      os << (*maybe_body);
      return os;
    } else {
      os << os.Indent();
      auto sep = "let {";
      for (auto var : region.DefinedVariables()) {
        os << sep << var;
        sep = ", ";
      }
      sep = "} = {";
      for (auto var : region.UsedVariables()) {
        os << sep << var;
        sep = ", ";
      }
      os << "}\n";
      os.PushIndent();
      os << (*maybe_body);
      os.PopIndent();
    }
  } else {
    os << os.Indent() << "empty-let";
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramVectorLoopRegion region) {
  if (auto maybe_body = region.Body(); maybe_body) {
    os << os.Indent() << "vector-loop {";
    auto sep = "";
    for (auto var : region.TupleVariables()) {
      os << sep << var;
      sep = ", ";
    }
    os << "} over " << region.Vector() << '\n';
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();
  } else {
    os << os.Indent() << "empty-vector-loop";
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramVectorAppendRegion region) {
  os << os.Indent() << "vector-append {";
  auto sep = "";
  for (auto var : region.TupleVariables()) {
    os << sep << var;
    sep = ", ";
  }
  os << "} into " << region.Vector();
  if (auto worker_id = region.WorkerId(); worker_id) {
    os << " of-worker " << *worker_id;
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramWorkerIdRegion region) {
  if (auto maybe_body = region.Body(); maybe_body) {
    os << os.Indent() << "hash {";
    auto sep = "";
    for (auto var : region.HashedVariables()) {
      os << sep << var;
      sep = ", ";
    }
    os << "} into " << region.WorkerId() << '\n';
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();
  } else {
    os << os.Indent() << "empty-hash";
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramVectorClearRegion region) {
  os << os.Indent() << "vector-clear " << region.Vector();
  if (auto worker_id = region.WorkerId(); worker_id) {
    os << " of-worker " << *worker_id;
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramVectorUniqueRegion region) {
  os << os.Indent() << "vector-unique " << region.Vector();
  if (auto worker_id = region.WorkerId(); worker_id) {
    os << " of-worker " << *worker_id;
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramVectorSwapRegion region) {
  os << os.Indent() << "vector-swap " << region.LHS() << ", " << region.RHS();
  return os;
}

// Print the derivation class suffix of a counter fold.
static OutputStream &PrintDerivClass(OutputStream &os, DerivClass c) {
  switch (c) {
    case DerivClass::kNonRecursive: os << "nonrecursive"; break;
    case DerivClass::kRecursive: os << "recursive"; break;
  }
  return os;
}

// Print the name of a membership predicate.
static OutputStream &PrintPredicate(OutputStream &os,
                                    MembershipPredicate pred) {
  switch (pred) {
    case MembershipPredicate::kInI: os << "in-I"; break;
    case MembershipPredicate::kInNew: os << "in-new"; break;
    case MembershipPredicate::kSurvivesSoFar: os << "survives-so-far"; break;
    case MembershipPredicate::kAliveAtClaim: os << "alive-at-claim"; break;
    case MembershipPredicate::kInNewWithFrontier:
      os << "in-new-with-frontier";
      break;
    case MembershipPredicate::kInNewSansFrontier:
      os << "in-new-sans-frontier";
      break;
    case MembershipPredicate::kPresent: os << "present"; break;
    case MembershipPredicate::kRecursivelySupported:
      os << "recursively-supported";
      break;
    case MembershipPredicate::kNetDeleted: os << "net-deleted"; break;
    case MembershipPredicate::kNetAdded: os << "net-added"; break;
  }
  return os;
}

OutputStream &operator<<(OutputStream &os,
                         ProgramUpdateCountRegion region) {

  os << os.Indent() << "update-count";
  if (region.IsExplicit()) {
    os << "-explicit";
  }
  os << ' ' << (region.IsAdd() ? '+' : '-');
  PrintDerivClass(os, region.DerivationClass());
  os << " {";

  auto sep = "";
  for (auto var : region.TupleVariables()) {
    os << sep << var;
    sep = ", ";
  }
  os << "} in " << region.Table();

  if (auto maybe_body = region.Body(); maybe_body) {
    os << '\n';
    os.PushIndent();
    os << os.Indent() << "if-crossed\n";
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();
    os.PopIndent();
  }
  return os;
}

OutputStream &operator<<(OutputStream &os,
                         ProgramChangeRecordRegion region) {

  os << os.Indent();
  auto sep = "{";
  for (auto var : region.RecordVariables()) {
    os << sep << var;
    sep = ", ";
  }
  os << "} = update-count-record " << (region.IsAdd() ? '+' : '-');
  PrintDerivClass(os, region.DerivationClass());
  os << " {";

  sep = "";
  for (auto var : region.TupleVariables()) {
    os << sep << var;
    sep = ", ";
  }
  os << "} in " << region.Table();

  if (auto maybe_body = region.Body(); maybe_body) {
    os << '\n';
    os.PushIndent();
    os << os.Indent() << "if-crossed\n";
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();
    os.PopIndent();
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramCheckMemberRegion region) {
  os << os.Indent() << "check-member ";
  PrintPredicate(os, region.Predicate());
  os << " {";
  auto sep = "";
  for (auto var : region.TupleVariables()) {
    os << sep << var;
    sep = ", ";
  }
  os << "} in " << region.Table();

  os.PushIndent();
  if (auto maybe_body = region.IfPresent(); maybe_body) {
    os << '\n';
    os << os.Indent() << "if-member\n";
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();
  }
  if (auto maybe_body = region.IfAbsent(); maybe_body) {
    os << '\n';
    os << os.Indent() << "if-absent\n";
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();
  }
  os.PopIndent();
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramCheckRecordRegion region) {
  auto sep = "{";
  os << os.Indent();
  for (auto var : region.RecordVariables()) {
    os << sep << var;
    sep = ", ";
  }
  os << "} = check-member-record ";
  PrintPredicate(os, region.Predicate());
  os << " {";
  sep = "";
  for (auto var : region.TupleVariables()) {
    os << sep << var;
    sep = ", ";
  }
  os << "} in " << region.Table();

  os.PushIndent();
  if (auto maybe_body = region.IfPresent(); maybe_body) {
    os << '\n';
    os << os.Indent() << "if-member\n";
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();
  }
  if (auto maybe_body = region.IfAbsent(); maybe_body) {
    os << '\n';
    os << os.Indent() << "if-absent\n";
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();
  }
  os.PopIndent();
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramCommitSweepRegion region) {
  os << os.Indent() << "commit-sweep " << region.Table();
  if (auto message = region.Message(); message) {
    os << " publishing " << message->Name() << '/' << message->Arity();
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramClaimRegion region) {
  os << os.Indent() << (region.IsDelete() ? "claim-del" : "claim-add")
     << " {";
  auto sep = "";
  for (auto var : region.TupleVariables()) {
    os << sep << var;
    sep = ", ";
  }
  os << "} in " << region.Table();

  if (auto maybe_body = region.Body(); maybe_body) {
    os << '\n';
    os.PushIndent();
    os << os.Indent() << "if-claimed\n";
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();
    os.PopIndent();
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramRetireRegion region) {
  os << os.Indent() << (region.IsDelete() ? "retire-del" : "retire-add")
     << " {";
  auto sep = "";
  for (auto var : region.TupleVariables()) {
    os << sep << var;
    sep = ", ";
  }
  os << "} in " << region.Table();
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramNetBatchRegion region) {
  os << os.Indent() << "net-batch " << region.AddVector() << ' '
     << region.RemoveVector();
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramTableJoinRegion region) {
  const auto maybe_body = region.Body();
  const auto maybe_added = region.AddedBody();
  const auto maybe_removed = region.RemovedBody();
  if (maybe_body || maybe_added || maybe_removed) {
    os << os.Indent() << "join-tables\n";
    os.PushIndent();
    os << os.Indent();
    auto sep = "vector-loop {";
    const auto pivot_vars = region.OutputPivotVariables();
    for (auto var : pivot_vars) {
      os << sep << var;
      sep = ", ";
    }
    os << "} over " << region.PivotVector() << '\n';

    const auto tables = region.Tables();
    for (auto i = 0u; i < tables.size(); ++i) {
      auto table = tables[i];
      os << os.Indent();

      sep = "select {";
      auto end = "select {} from ";
      auto cols = table.Columns();
      auto j = 0u;
      for (auto var : region.OutputVariables(i)) {
        os << sep << cols[j++] << " as " << var;
        sep = ", ";
        end = "} from ";
      }
      os << end << table;

      if (auto maybe_index = region.Index(i)) {
        os << " using " << (*maybe_index);
      }

      sep = " where ";
      j = 0u;
      for (auto col : region.IndexedColumns(i)) {
        os << sep << col << " = " << pivot_vars[j++];
        sep = " and ";
      }

      os << '\n';
    }

    // A join with only the plain body prints it directly; a join carrying
    // delta sections names each section's read discipline.
    if (maybe_body && !maybe_added && !maybe_removed) {
      os.PushIndent();
      os << (*maybe_body);
      os.PopIndent();
    } else {
      if (maybe_body) {
        os << os.Indent() << "current:\n";
        os.PushIndent();
        os << (*maybe_body);
        os.PopIndent();
        os << '\n';
      }
      if (maybe_added) {
        os << os.Indent() << "added: all-in-new, some-net-added\n";
        os.PushIndent();
        os << (*maybe_added);
        os.PopIndent();
        if (maybe_removed) {
          os << '\n';
        }
      }
      if (maybe_removed) {
        os << os.Indent() << "removed: all-in-i, some-net-deleted\n";
        os.PushIndent();
        os << (*maybe_removed);
        os.PopIndent();
      }
    }
    os.PopIndent();
  } else {
    os << os.Indent() << "empty-join";
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramTableProductRegion region) {
  if (auto maybe_body = region.Body(); maybe_body) {
    os << os.Indent() << "cross-product\n";
    os.PushIndent();
    const auto tables = region.Tables();
    for (auto i = 0u; i < tables.size(); ++i) {
      auto table = tables[i];
      auto vector = region.Vector(i);
      auto outputs = region.OutputVariables(i);
      os << os.Indent() << "from " << table << " and " << vector;
      auto sep = " select {";
      for (auto var : outputs) {
        os << sep << var;
        sep = ", ";
      }
      os << "}\n";
    }
    os.PushIndent();
    os << (*maybe_body);
    os.PopIndent();
    os.PopIndent();
  } else {
    os << os.Indent() << "empty-cross-product";
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramTableScanRegion region) {
  if (auto body = region.Body(); body) {
    os << os.Indent() << "scan";
    if (region.Index()) {
      os << "-index";
    } else {
      os << "-table";
    }

    auto sep = "";
    os << " select {";
    for (auto var : region.OutputVariables()) {
      os << sep << var;
      sep = ", ";
    }

    os << "} from " << region.Table();

    if (auto index = region.Index(); index) {
      os << " where {";
      sep = "";
      for (auto var : region.InputVariables()) {
        os << sep << var;
        sep = ", ";
      }
      os << "} in " << *index;
    }

    os << '\n';
    os.PushIndent();
    os << (*body);
    os.PopIndent();
  } else {
    os << os.Indent() << "empty-scan";
  }

  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramInductionRegion region) {
  os << os.Indent() << "induction\n";
  os.PushIndent();
  if (auto init = region.Initializer(); init) {
    os << os.Indent() << "init\n";
    os.PushIndent();
    os << *init << '\n';
    os.PopIndent();
  } else {
    os << os.Indent() << "empty-init\n";
  }

  os << os.Indent() << "fixpoint-loop testing ";

  auto sep = "";
  for (auto vec : region.Vectors()) {
    os << sep << vec;
    sep = ", ";
  }
  os << '\n';

  os.PushIndent();
  os << region.FixpointLoop();
  os.PopIndent();
  if (auto output = region.Output(); output) {
    os << '\n' << os.Indent() << "output\n";
    os.PushIndent();
    os << (*output);
    os.PopIndent();
  }
  os.PopIndent();
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramSeriesRegion region) {
  if (auto regions = region.Regions(); !regions.empty()) {
    auto sep = "";
    os << os.Indent() << "seq\n";
    os.PushIndent();
    for (auto sub_region : regions) {
      os << sep << sub_region;
      sep = "\n";
    }
    os.PopIndent();
  } else {
    os << os.Indent() << "empty-seq";
  }
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramParallelRegion region) {
  if (auto regions = region.Regions(); !regions.empty()) {
    auto sep = "";
    os << os.Indent() << "par\n";
    os.PushIndent();
    for (auto sub_region : regions) {
      os << sep << sub_region;
      sep = "\n";
    }
    os.PopIndent();
  } else {
    os << os.Indent() << "empty-par";
  }
  return os;
}

namespace {

class FormatDispatcher final : public ProgramVisitor {
 public:
  virtual ~FormatDispatcher(void) = default;

  explicit FormatDispatcher(OutputStream &os_) : os(os_) {}

#define MAKE_VISITOR(cls) \
  void Visit(cls region) override { \
    os << region; \
  }

  MAKE_VISITOR(ProgramCallRegion)
  MAKE_VISITOR(ProgramReturnRegion)
  MAKE_VISITOR(ProgramTestAndSetRegion)
  MAKE_VISITOR(ProgramGenerateRegion)
  MAKE_VISITOR(ProgramInductionRegion)
  MAKE_VISITOR(ProgramLetBindingRegion)
  MAKE_VISITOR(ProgramParallelRegion)
  MAKE_VISITOR(ProgramProcedure)
  MAKE_VISITOR(ProgramPublishRegion)
  MAKE_VISITOR(ProgramSeriesRegion)
  MAKE_VISITOR(ProgramVectorAppendRegion)
  MAKE_VISITOR(ProgramVectorClearRegion)
  MAKE_VISITOR(ProgramVectorLoopRegion)
  MAKE_VISITOR(ProgramVectorSwapRegion)
  MAKE_VISITOR(ProgramVectorUniqueRegion)
  MAKE_VISITOR(ProgramUpdateCountRegion)
  MAKE_VISITOR(ProgramCheckMemberRegion)
  MAKE_VISITOR(ProgramChangeRecordRegion)
  MAKE_VISITOR(ProgramCheckRecordRegion)
  MAKE_VISITOR(ProgramCommitSweepRegion)
  MAKE_VISITOR(ProgramClaimRegion)
  MAKE_VISITOR(ProgramRetireRegion)
  MAKE_VISITOR(ProgramNetBatchRegion)
  MAKE_VISITOR(ProgramTableJoinRegion)
  MAKE_VISITOR(ProgramTableProductRegion)
  MAKE_VISITOR(ProgramTableScanRegion)
  MAKE_VISITOR(ProgramTupleCompareRegion)
  MAKE_VISITOR(ProgramWorkerIdRegion)

 private:
  OutputStream &os;
};

}  // namespace

OutputStream &operator<<(OutputStream &os, ProgramRegion region) {
  FormatDispatcher dispatcher(os);
  if (!region.Comment().empty()) {
    os << os.Indent() << "; " << region.Comment() << '\n';
  }
  region.Accept(dispatcher);
  return os;
}

OutputStream &operator<<(OutputStream &os, ProgramProcedure proc) {
  switch (proc.Kind()) {
    case ProcedureKind::kInitializer: os << "^init:"; break;
    case ProcedureKind::kEntryDataFlowFunc: os << "^entry:"; break;
    case ProcedureKind::kPrimaryDataFlowFunc: os << "^flow:"; break;
    case ProcedureKind::kMessageHandler:
      os << "^receive:";
      if (auto message = proc.Message(); message) {
        os << message->Name() << '/' << message->Arity() << ':';
      }
      break;
    case ProcedureKind::kQueryMessageInjector: os << "^inject:"; break;
  }
  os << proc.Id();

  return os;
}

OutputStream &operator<<(OutputStream &os, Program program) {
  auto sep = "";

  const auto module = program.ParsedModule();
  for (auto var : program.Constants()) {
    os << sep << os.Indent() << "const " << Type(os, module, var) << ' ' << var;
    switch (var.DefiningRole()) {
      case VariableRole::kConstantZero: os << " = 0"; break;
      case VariableRole::kConstantFalse: os << " = false"; break;
      case VariableRole::kConstantTrue: os << " = true"; break;
      default:
        if (auto const_val = var.Value()) {
          if (const_val->IsTag()) {
            os << " = " << QueryTag::From(*const_val).Value()
               << " ; Optimization tag";

          } else if (auto lit = const_val->Literal(); lit) {
            os << " = " << *lit;
          }
        }
        break;
    }
    sep = "\n\n";
  }
  for (auto var : program.GlobalVariables()) {
    os << sep << os.Indent() << "global " << Type(os, module, var) << ' '
       << var;
    sep = "\n\n";
  }

  for (auto table : program.Tables()) {
    os << sep;
    DefineTable(os, module, table);
    sep = "\n\n";
  }

  for (auto proc : program.Procedures()) {
    os << sep << os.Indent();
    if (proc.Kind() == ProcedureKind::kInitializer) {
      os << "init ";
    }

    os << "proc " << proc;

    os << '(';
    auto param_sep = "";
    for (auto vec : proc.VectorParameters()) {
      os << param_sep << vec;
      param_sep = ", ";
    }
    for (auto var : proc.VariableParameters()) {
      os << param_sep << var;
      param_sep = ", ";
    }
    os << ")\n";
    os.PushIndent();

    for (auto vec : proc.DefinedVectors()) {
      os << os.Indent() << "vector-define " << vec << '\n';
    }

    os << proc.Body();
    os.PopIndent();

    sep = "\n\n";
  }
  os << sep;
  return os;
}

}  // namespace hyde
