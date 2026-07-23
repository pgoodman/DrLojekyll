// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2019, Trail of Bits. All rights reserved.

#include <drlojekyll/DataFlow/Format.h>
#include <drlojekyll/DataFlow/Query.h>
#include <drlojekyll/Display/Format.h>
#include <drlojekyll/Lex/Format.h>
#include <drlojekyll/Parse/Format.h>

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "EquivalenceSet.h"
#include "Query.h"  // For the raw QueryViewImpl::det_seq read (the -df-out
                    // bijection witness bypasses DeterministicOrder()'s debug
                    // assert so the witness reports its own failure) and the
                    // is_dead skip in the dump traversal.

#define DEBUG(...)

namespace hyde {
namespace {

static const char *kBeginTable =
    "<TABLE cellpadding=\"0\" cellspacing=\"0\" border=\"1\" bgcolor=\"white\"><TR>";
static const char *kEndTable = "</TR></TABLE>";

static const char *kColors[] = {
    "antiquewhite", "aquamarine",   "cadetblue1", "chartreuse1",
    "chocolate1",   "deepskyblue2", "goldenrod1", "beige",
    "cadetblue3",   "floralwhite",  "gainsboro",  "darkseagreen1",
};

static constexpr auto kNumColors = sizeof(kColors)/sizeof(kColors[0]);

}  // namespace

OutputStream &operator<<(OutputStream &os, Query query) {
  os << "digraph {\n"
     << "bgcolor=\"#f0f4f7\";\n"
     << "node [shape=none margin=0 nojustify=false labeljust=l font=courier];\n";

  auto do_table = [&](int row_span, QueryView view) {
    std::optional<unsigned> induction_id = view.InductionGroupId();
    std::optional<unsigned> induction_depth = view.InductionDepth();
    os << "<TD rowspan=\"" << row_span << "\">";
    auto sep = "";
    if (auto table_id = view.TableId()) {
      os << sep << "TABLE " << *table_id;
      sep = "<BR />";
    }

    if (induction_id && induction_depth) {
      os << sep << "SET " << *induction_id << " DEPTH " << *induction_depth;
      sep = "<BR />";
    }

    if (auto stratum = view.Stratum()) {
      os << sep << "STRATUM " << *stratum;
      sep = "<BR />";
    }

    os << sep << "EQ SET " << view.EquivalenceSetId() << "</TD>";
  };

  auto do_const = [&](QueryConstant const_col) {
    if (const_col.IsTag()) {
      os << "<B>TAG-" << QueryTag::From(const_col).Value() << "</B>";
    } else if (auto lit = const_col.Literal(); lit) {
      os << "<B>" << *lit << "</B>";
    } else {
      os << "<B>TRUE</B>";  // The unit-relation token.
    }
  };

  auto do_col = [&](QueryColumn col) -> OutputStream & {
    if (col.IsConstantOrConstantRef()) {
      do_const(QueryConstant::From(col));

    } else {
      os << col.Variable(); // << ":" << *(col.Index());
    }
//    os << ", Id:" << col.Id();
//    NOTE(pag): the per-column taint-set debug dump used to live here; the
//    taint passes (lib/DataFlow/Taint.cpp) were deleted as dead code at the
//    delta-relational-IR epoch — recover both from git history if needed.
    return os;
  };

  auto do_color = [&](QueryView view) -> OutputStream & {
    if (auto color = view.Color(); color) {
      os << " color=\"#";
      for (auto i = 0; i < 6; ++i) {
        os << "0123456789abcdef"[color & 0xf];
        color >>= 4u;
      }
      os << "\" ";
    }
    return os;
  };

  for (auto relation : query.Relations()) {
    const auto decl = relation.Declaration();
    const auto arity = decl.Arity();
    os << "t" << relation.UniqueId() << " [ label=<" << kBeginTable
       << "<TD>RELATION ";

    if (relation.IsCondition()) {
      os << "(unit) ";
    }

    os << ParsedDeclarationName(decl) << "</TD>";
    for (auto i = 0u; i < arity; ++i) {
      auto param = decl.NthParameter(i);
      os << "<TD port=\"p" << i << "\">" << param.Name() << "</TD>";
    }
    if (relation.IsCondition()) {
      os << "<TD port=\"p0\">true</TD>";
    }

    os << kEndTable << ">];\n";

    for (auto select : relation.Selects()) {
      auto color = QueryView::From(select).CanReceiveDeletions()
                       ? " [color=purple]"
                       : "";
      os << "v" << select.UniqueId() << " -> t" << relation.UniqueId() << color
         << ";\n";
    }

    for (auto insert : relation.Inserts()) {
      os << "t" << relation.UniqueId() << " -> v" << insert.UniqueId();

      if (insert.CanProduceDeletions()) {
        os << " [color=purple]";
      }

      os << ";\n";
    }
  }

  for (auto io : query.IOs()) {
    const auto decl = io.Declaration();
    const auto arity = decl.Arity();
    os << "t" << io.UniqueId() << " [label=<" << kBeginTable << "<TD>"
       << QueryStream(io).KindName() << ' ' << ParsedDeclarationName(decl)
       << "</TD>";
    for (auto i = 0u; i < arity; ++i) {
      auto param = decl.NthParameter(i);
      os << "<TD port=\"p" << i << "\">" << param.Name() << "</TD>";
    }
    os << kEndTable << ">];\n";

    for (auto select : io.Receives()) {
      auto color = QueryView::From(select).CanReceiveDeletions()
                       ? " [color=purple]"
                       : "";
      os << "v" << select.UniqueId() << " -> t" << io.UniqueId() << color
         << ";\n";
    }

    for (auto insert : io.Transmits()) {
      auto color = QueryView::From(insert).CanReceiveDeletions()
                       ? " [color=purple]"
                       : "";
      os << "t" << io.UniqueId() << " -> v" << insert.UniqueId() << color
         << ";\n";
    }
  }

//  for (auto constant : query.Constants()) {
//    os << "t" << constant.UniqueId() << " [label=<" << kBeginTable
//       << "<TD port=\"p0\">";
//    do_const(constant);
//    os << "</TD>" << kEndTable << ">];\n";
//  }

  for (auto tag : query.Tags()) {
    os << "t" << tag.UniqueId() << " [label=<" << kBeginTable
       << "<TD port=\"p0\">";
    do_const(tag);
    os << "</TD>" << kEndTable << ">];\n";
  }

  for (auto select : query.Selects()) {
    os << "v" << select.UniqueId() << " [" << do_color(select) << "label=<"
       << kBeginTable;
    do_table(2, select);
    os << "<TD>" << QueryView(select).KindName() << "</TD>";

    auto i = 0u;
    for (auto col : select.Columns()) {
      os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
      ++i;
    }

    DEBUG(os << "</TR><TR><TD colspan=\"10\">" << select.DebugString(os)
             << "</TD>";)

    os << kEndTable << ">];\n";

  }

  for (auto constraint : query.Compares()) {
    os << "v" << constraint.UniqueId() << " [" << do_color(constraint)
       << "label=<" << kBeginTable;
    do_table(2, constraint);

    const auto out_copied_cols = constraint.CopiedColumns();
    const auto in_copied_cols = constraint.InputCopiedColumns();
    if (!out_copied_cols.empty()) {
      os << "<TD rowspan=\"2\">COPY</TD>";
      for (auto col : out_copied_cols) {
        os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
      }
    }

    os << "<TD rowspan=\"2\">" << QueryView(constraint).KindName() << ' ';
    switch (constraint.Operator()) {
      case ComparisonOperator::kEqual: os << "eq"; break;
      case ComparisonOperator::kGreaterThan: os << "gt"; break;
      case ComparisonOperator::kLessThan: os << "lt"; break;
      case ComparisonOperator::kNotEqual: os << "neq"; break;
    }
    const auto lhs = constraint.LHS();
    const auto rhs = constraint.RHS();
    const auto input_lhs = constraint.InputLHS();
    const auto input_rhs = constraint.InputRHS();
    const auto input_lhs_view = QueryView::Containing(input_lhs);
    const auto input_rhs_view = QueryView::Containing(input_rhs);

    if (lhs == rhs) {
      os << "</TD><TD port=\"c" << lhs.Id() << "\" colspan=\"2\">"
         << do_col(lhs);
    } else {
      os << "</TD><TD port=\"c" << lhs.Id() << "\">" << do_col(lhs)
         << "</TD><TD port=\"c" << rhs.Id() << "\">" << do_col(rhs);
    }

    os << "</TD></TR><TR>";

    if (!in_copied_cols.empty()) {
      auto i = 0u;
      for (auto col : in_copied_cols) {
        os << "<TD port=\"g" << i << "\">" << do_col(col) << "</TD>";
        ++i;
      }
    }

    os << "<TD port=\"p0\">" << do_col(input_lhs) << "</TD><TD port=\"p1\">"
       << do_col(input_rhs) << "</TD>";

    DEBUG(os << "</TR><TR><TD colspan=\"10\">" << constraint.DebugString(os)
             << "</TD>";)

    auto color = QueryView::From(constraint).CanReceiveDeletions()
                     ? " [color=purple]"
                     : "";

    os << kEndTable << ">];\n"
       << "v" << constraint.UniqueId() << ":p0 -> v"
       << input_lhs_view.UniqueId() << ":c" << input_lhs.Id() << color << ";\n"
       << "v" << constraint.UniqueId() << ":p1 -> v"
       << input_rhs_view.UniqueId() << ":c" << input_rhs.Id() << color << ";\n";

    for (auto i = 0u; i < in_copied_cols.size(); ++i) {
      const auto col = in_copied_cols[i];
      const auto view = QueryView::Containing(col);
      os << "v" << constraint.UniqueId() << ":g" << i << " -> v"
         << view.UniqueId() << ":c" << col.Id() << color << ";\n";
    }

  }

  for (auto kv : query.KVIndices()) {
    os << "v" << kv.UniqueId() << " [" << do_color(kv) << "label=<"
       << kBeginTable;
    do_table(2, kv);
    os << "<TD rowspan=\"2\">KEYS</TD>";
    for (auto col : kv.KeyColumns()) {
      os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
    }
    os << "<TD rowspan=\"2\">VALS</TD>";
    auto i = 0u;
    for (auto col : kv.ValueColumns()) {
      os << "<TD port=\"c" << col.Id() << "\">"
         << kv.NthValueMergeFunctor(i).Name() << "(" << do_col(col) << ")</TD>";
    }
    os << "</TR><TR>";
    i = 0u;
    for (auto col : kv.InputKeyColumns()) {
      os << "<TD port=\"g" << (i++) << "\">" << do_col(col) << "</TD>";
    }
    for (auto col : kv.InputValueColumns()) {
      os << "<TD port=\"g" << (i++) << "\">" << do_col(col) << "</TD>";
    }
    DEBUG(os << "</TR><TR><TD colspan=\"10\">" << kv.DebugString(os)
             << "</TD>";)

    os << kEndTable << ">];\n";

    auto color =
        QueryView::From(kv).CanReceiveDeletions() ? " [color=purple]" : "";

    i = 0u;
    for (auto col : kv.InputKeyColumns()) {
      const auto view = QueryView::Containing(col);
      os << "v" << kv.UniqueId() << ":g" << (i++) << " -> v" << view.UniqueId()
         << ":c" << col.Id() << color << ";\n";
    }
    for (auto col : kv.InputValueColumns()) {
      const auto view = QueryView::Containing(col);
      os << "v" << kv.UniqueId() << ":g" << (i++) << " -> v" << view.UniqueId()
         << ":c" << col.Id() << color << ";\n";
    }
  }

  for (auto join : query.Joins()) {
    os << "v" << join.UniqueId() << " [" << do_color(join) << "label=<"
       << kBeginTable;
    do_table(2, join);

    //    auto num_pivot_inputs = 0u;
    //    auto num_pivots = join.NumPivotSets();
    //    for (auto i = 0u; i < join.NumPivotSets(); ++i) {
    //      num_pivot_inputs += join.NthPivotSet(i).size();
    //    }
    //
    //    for (auto i = 0u; i < join.NumPivotSets(); ++i) {
    //      auto color = kColors[i];
    //      os << "<TD colspan=\"" << join.NthPivotSet(i).size() << "\" bgcolor=\""
    //         << color << "\">" << join.NthPivotColumn(i).Variable() << "</TD>";
    //    }

    const auto num_pivots = join.NumPivotColumns();
    const auto num_outputs = join.NumMergedColumns();
    auto i = 0u;
    for (; i < num_pivots; ++i) {
      const auto pivot_set_size = join.NthInputPivotSet(i).size();
      const auto col = join.NthOutputPivotColumn(i);
      const auto color = kColors[i % kNumColors];
      os << "<TD port=\"c" << col.Id() << "\" colspan=\"" << pivot_set_size
         << "\" bgcolor=\"" << color << "\">" << do_col(col) << "</TD>";
    }

    os << "<TD rowspan=\"2\">" << QueryView(join).KindName() << "</TD>";

    for (i = 0u; i < num_outputs; ++i) {
      const auto col = join.NthOutputMergedColumn(i);
      os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
    }

    os << "</TR><TR>";

    auto j = 0u;
    for (i = 0u; i < num_pivots; ++i) {
      auto color = kColors[i % kNumColors];
      for (auto col : join.NthInputPivotSet(i)) {
        os << "<TD bgcolor=\"" << color << "\" port=\"p" << j << "\">"
           << do_col(col) << "</TD>";
        j++;
      }
    }

    for (i = 0u; i < num_outputs; ++i) {
      const auto col = join.NthInputMergedColumn(i);
      os << "<TD port=\"p" << j << "\">" << do_col(col) << "</TD>";
      j++;
    }

    DEBUG(os << "</TR><TR><TD colspan=\"10\">" << join.DebugString(os)
             << "</TD>";)

    os << kEndTable << ">];\n";


    // Link the joined columns to their sources.

    j = 0u;
    for (i = 0u; i < num_pivots; ++i) {
      for (auto col : join.NthInputPivotSet(i)) {
        const auto view = QueryView::Containing(col);
        auto color = view.CanProduceDeletions() ? " [color=purple]" : "";
        os << "v" << join.UniqueId() << ":p" << j << " -> v" << view.UniqueId()
           << ":c" << col.Id() << color << ";\n";
        j++;
      }
    }

    for (i = 0u; i < num_outputs; ++i) {
      const auto col = join.NthInputMergedColumn(i);
      const auto view = QueryView::Containing(col);
      auto color = view.CanProduceDeletions() ? " [color=purple]" : "";
      os << "v" << join.UniqueId() << ":p" << j << " -> v" << view.UniqueId()
         << ":c" << col.Id() << color << ";\n";
      j++;
    }

  }

  for (auto map : query.Maps()) {
    os << "v" << map.UniqueId() << " [" << do_color(map) << "label=<"
       << kBeginTable;
    do_table(2, map);

    auto num_copied = map.NumCopiedColumns();
    if (num_copied) {
      os << "<TD rowspan=\"2\">COPY</TD>";
      for (auto col : map.CopiedColumns()) {
        os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
      }
    }

    os << "<TD rowspan=\"2\">" << QueryView(map).KindName() << ' ';
    if (!map.IsPositive()) {
      os << '!';
    }
    os << ParsedDeclarationName(map.Functor()) << "</TD>";

    ParsedDeclaration map_functor_decl(map.Functor());
    for (ParsedParameter param : map_functor_decl.Parameters()) {
      auto col = map.MappedColumns()[param.Index()];
      os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
    }

    const auto num_inputs = map.NumInputColumns();
    os << "</TR><TR>";

    for (auto i = 0u; i < num_copied; ++i) {
      const auto col = map.NthInputCopiedColumn(i);
      os << "<TD port=\"g" << i << "\">" << do_col(col) << "</TD>";
    }

    auto i = 0u;
    for (ParsedParameter param : map_functor_decl.Parameters()) {
      if (param.Binding() == ParameterBinding::kBound) {
        auto in_col = map.NthInputColumn(i);
        os << "<TD port=\"p" << i << "\">" << do_col(in_col) << "</TD>";
        ++i;
      } else {
        os << "<TD> </TD>";
      }
    }

    //    // Empty space.
    //    if (auto diff = (map.Arity() - map.NumInputColumns())) {
    //      os << "<TD colspan=\"" << diff << "\"></TD>";
    //    }

    DEBUG(os << "</TR><TR><TD colspan=\"10\">" << map.DebugString(os)
             << "</TD>";)

    os << kEndTable << ">];\n";

    auto color = QueryView(map).CanReceiveDeletions() ? " [color=purple]" : "";

    for (auto i = 0u; i < num_copied; ++i) {
      auto col = map.NthInputCopiedColumn(i);
      auto view = QueryView::Containing(col);
      os << "v" << map.UniqueId() << ":g" << i << " -> v" << view.UniqueId()
         << ":c" << col.Id() << color << ";\n";
    }

    // Link the input columns to their sources.
    for (auto i = 0u; i < num_inputs; ++i) {
      auto col = map.NthInputColumn(i);
      auto view = QueryView::Containing(col);
      os << "v" << map.UniqueId() << ":p" << i << " -> v" << view.UniqueId()
         << ":c" << col.Id() << color << ";\n";
    }

  }

  for (auto agg : query.Aggregates()) {
    os << "v" << agg.UniqueId() << " [" << do_color(agg) << "label=<"
       << kBeginTable;
    do_table(3, agg);
    os << "<TD rowspan=\"3\">" << QueryView(agg).KindName() << ' '
       << ParsedDeclarationName(agg.Functor()) << "</TD>";
    for (auto col : agg.Columns()) {
      os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
    }
    os << "</TR><TR>";
    auto num_group = agg.NumGroupColumns();
    if (num_group) {
      os << "<TD colspan=\"" << num_group << "\">GROUP</TD>";
    }
    auto num_config = agg.NumConfigurationColumns();
    if (num_config) {
      os << "<TD colspan=\"" << num_config << "\">CONFIG</TD>";
    }
    auto num_summ = agg.NumAggregateColumns();
    if (num_summ) {
      os << "<TD colspan=\"" << num_summ << "\">SUMMARIZE</TD>";
    }
    os << "</TR><TR>";
    for (auto i = 0u; i < num_group; ++i) {
      auto col = agg.NthInputGroupColumn(i);
      os << "<TD port=\"g1_" << i << "\">" << do_col(col) << "</TD>";
    }
    for (auto i = 0u; i < num_config; ++i) {
      auto col = agg.NthInputConfigurationColumn(i);
      os << "<TD port=\"g2_" << i << "\">" << do_col(col) << "</TD>";
    }
    for (auto i = 0u; i < num_summ; ++i) {
      auto col = agg.NthInputAggregateColumn(i);
      os << "<TD port=\"s" << i << "\">" << do_col(col) << "</TD>";
    }

    DEBUG(os << "</TR><TR><TD colspan=\"10\">" << agg.DebugString(os)
             << "</TD>";)

    os << kEndTable << ">];\n";

    auto color =
        QueryView::From(agg).CanReceiveDeletions() ? " [color=purple]" : "";

    for (auto i = 0u; i < num_group; ++i) {
      auto col = agg.NthInputGroupColumn(i);
      auto view = QueryView::Containing(col);
      os << "v" << agg.UniqueId() << ":g1_" << i << " -> v" << view.UniqueId()
         << ":c" << col.Id() << color << ";\n";
    }
    for (auto i = 0u; i < num_config; ++i) {
      auto col = agg.NthInputConfigurationColumn(i);
      auto view = QueryView::Containing(col);
      os << "v" << agg.UniqueId() << ":g2_" << i << " -> v" << view.UniqueId()
         << ":c" << col.Id() << color << ";\n";
    }
    for (auto i = 0u; i < num_summ; ++i) {
      auto col = agg.NthInputAggregateColumn(i);
      auto view = QueryView::Containing(col);
      os << "v" << agg.UniqueId() << ":s" << i << " -> v" << view.UniqueId()
         << ":c" << col.Id() << color << ";\n";
    }

  }

  for (auto neg : query.Negations()) {
    os << "v" << neg.UniqueId() << " [" << do_color(neg) << "label=<"
       << kBeginTable;
    do_table(2, neg);

    auto num_group = neg.NumCopiedColumns();
    if (num_group) {
      os << "<TD rowspan=\"2\">COPY</TD>";
      for (auto col : neg.CopiedColumns()) {
        os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
      }
    }

    os << "<TD rowspan=\"2\">" << QueryView(neg).KindName() << "</TD>";
    for (auto col : neg.NegatedColumns()) {
      os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
    }

    const auto num_inputs = neg.NumInputColumns();
    os << "</TR><TR>";

    for (auto i = 0u; i < num_group; ++i) {
      const auto col = neg.NthInputCopiedColumn(i);
      os << "<TD port=\"g" << i << "\">" << do_col(col) << "</TD>";
    }

    for (auto i = 0u; i < num_inputs; ++i) {
      const auto col = neg.NthInputColumn(i);
      os << "<TD port=\"p" << i << "\">" << do_col(col) << "</TD>";
    }

    DEBUG(os << "</TR><TR><TD colspan=\"10\">" << neg.DebugString(os)
             << "</TD>";)

    os << kEndTable << ">];\n";

    auto color = " [color=purple]";

    for (auto i = 0u; i < num_group; ++i) {
      auto col = neg.NthInputCopiedColumn(i);
      auto view = QueryView::Containing(col);
      os << "v" << neg.UniqueId() << ":g" << i << " -> v" << view.UniqueId()
         << ":c" << col.Id() << color << ";\n";
    }

    // Link the input columns to their sources.
    for (auto i = 0u; i < num_inputs; ++i) {
      auto col = neg.NthInputColumn(i);
      auto view = QueryView::Containing(col);
      os << "v" << neg.UniqueId() << ":p" << i << " -> v" << view.UniqueId()
         << ":c" << col.Id() << color << ";\n";
    }

    os << "v" << neg.UniqueId() << " -> v" << neg.NegatedView().UniqueId()
       << color << ";\n";

  }

  for (auto insert : query.Inserts()) {
    const auto decl = insert.Declaration();
    os << "v" << insert.UniqueId() << " [" << do_color(insert) << "label=<"
       << kBeginTable;
    do_table(2, insert);
    os << "<TD>" << QueryView(insert).KindName() << ' '
       << ParsedDeclarationName(decl) << "</TD>";

    for (auto i = 0u, max_i = insert.NumInputColumns(); i < max_i; ++i) {
      os << "<TD port=\"c" << i << "\">" << do_col(insert.NthInputColumn(i))
         << "</TD>";
    }

    // Attached witness columns: read from the incoming view, never stored.
    for (auto i = 0u, max_i = insert.NumAttachedColumns(); i < max_i; ++i) {
      os << "<TD port=\"a" << i << "\">attached "
         << do_col(insert.NthAttachedColumn(i)) << "</TD>";
    }

    DEBUG(os << "</TR><TR><TD colspan=\"10\">" << insert.DebugString(os)
             << "</TD>";)

    os << kEndTable << ">];\n";

    auto color =
        QueryView::From(insert).CanReceiveDeletions() ? " [color=purple]" : "";

    for (auto i = 0u, max_i = insert.NumInputColumns(); i < max_i; ++i) {
      const auto col = insert.NthInputColumn(i);
      const auto view = QueryView::Containing(col);
      os << "v" << insert.UniqueId() << ":c" << i << " -> "
         << "v" << view.UniqueId() << ":c" << col.Id() << color << ";\n";
    }

    for (auto i = 0u, max_i = insert.NumAttachedColumns(); i < max_i; ++i) {
      const auto col = insert.NthAttachedColumn(i);
      const auto view = QueryView::Containing(col);
      os << "v" << insert.UniqueId() << ":a" << i << " -> "
         << "v" << view.UniqueId() << ":c" << col.Id() << color << ";\n";
    }

  }

  for (auto view : query.Tuples()) {
    os << "v" << view.UniqueId() << " [" << do_color(view) << "label=<"
       << kBeginTable;
    do_table(2, view);
    os << "<TD rowspan=\"2\">" << QueryView(view).KindName() << "</TD>";
    for (auto col : view.Columns()) {
      os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
    }
    os << "</TR><TR>";

    for (auto i = 0u; i < view.NumInputColumns(); ++i) {
      os << "<TD port=\"p" << i << "\">" << do_col(view.NthInputColumn(i))
         << "</TD>";
    }

    DEBUG(os << "</TR><TR><TD colspan=\"10\">" << view.DebugString(os)
             << "</TD>";)

    os << kEndTable << ">];\n";

    auto color =
        QueryView::From(view).CanReceiveDeletions() ? " [color=purple]" : "";

    // Link the input columns to their sources.
    for (auto i = 0u; i < view.NumInputColumns(); ++i) {
      auto col = view.NthInputColumn(i);
      auto input_view = QueryView::Containing(col);
      os << "v" << view.UniqueId() << ":p" << i << " -> v"
         << input_view.UniqueId() << ":c" << col.Id() << color << ";\n";
    }

  }

  for (auto merge : query.Merges()) {
    os << "v" << merge.UniqueId() << " [" << do_color(merge) << "label=<"
       << kBeginTable;
    do_table(2, merge);
    auto num_cols = 1;
    os << "<TD rowspan=\"2\">" << QueryView(merge).KindName() << "</TD>";
    for (auto col : merge.Columns()) {
      ++num_cols;
      os << "<TD port=\"c" << col.Id() << "\">" << do_col(col) << "</TD>";
    }
    DEBUG(os << "</TR><TR><TD colspan=\"10\">" << merge.DebugString(os)
             << "</TD>";)

    std::optional<ParsedDeclaration> decl;
    auto conflict = false;
    for (auto col : merge.Columns()) {
      if (auto var = col.Variable()) {
        auto clause = ParsedClause::Containing(*var);
        if (var->Order() >= clause.Arity()) {
          continue;
        }
        auto clause_decl = ParsedDeclaration::Of(clause);
        if (!decl) {
          decl = clause_decl;
        } else if (*decl != clause_decl) {
          conflict = true;
          break;
        }
      }
    }

    if (decl && !conflict) {
      os << "</TR><TR><TD colspan=\"" << num_cols << "\">"
         << decl->Name() << "</TD>";
    }

    os << kEndTable << ">];\n";

    for (auto view : merge.MergedViews()) {
      auto color = view.CanProduceDeletions() ? " [color=purple]" : "";
      os << "v" << merge.UniqueId() << " -> v" << view.UniqueId() << color
         << ";\n";
    }

  }

  os << "}\n";
  return os;
}

// The `-df-out` BB-with-arguments text dump. Grammar: t2-dump-spec.md §1 with
// the session-pinned rulings (p1)-(p9); the byte-contracts are the §1 blocks
// of the t2-desired-df-* artifacts. Blocks are emitted in det_seq order BY
// CONSTRUCTION: det_seq is stamped in QueryImpl::ForEachView per-kind DefList
// order (selects, tuples, kv_indices, joins, maps, aggregates, merges,
// negations, compares, inserts) at the last stamp (IdentifyInductions), every
// later pass is view-neutral, and the traversal below walks the SAME kind
// sequence. NEVER use the public Query::ForEachView here — it walks JOINS
// FIRST and would silently break the ascending-det_seq property.
OutputStream &operator<<(OutputStream &os, QueryDF df) {
  const Query query = df.query;

  // Kind-tagged det_seq-order traversal over LIVE views. The public per-kind
  // DefList iterators do NOT skip dead views (DefinedNodeIterator::operator++
  // has no is_dead filter); the impl stamp walk does. RemoveUnusedViews
  // leaves the lists physically dead-free at this drain today, but the skip
  // below makes the traversal set UNCONDITIONALLY identical to the stamp set
  // — the bijection witness's s >= N check is the always-on tripwire if that
  // ever regresses.
  enum : unsigned {
    kDFSelect, kDFTuple, kDFKVIndex, kDFJoin, kDFMap,
    kDFAggregate, kDFMerge, kDFNegate, kDFCompare, kDFInsert
  };
  const auto for_each_df_view = [&query](auto cb) {
    const auto wrap = [&cb](QueryView v, unsigned kind, const char *tag) {
      if (!v.impl->is_dead) {
        cb(v, kind, tag);
      }
    };
    for (auto v : query.Selects())    wrap(QueryView::From(v), kDFSelect, "select");
    for (auto v : query.Tuples())     wrap(QueryView::From(v), kDFTuple, "tuple");
    for (auto v : query.KVIndices())  wrap(QueryView::From(v), kDFKVIndex, "kv_index");
    for (auto v : query.Joins())      wrap(QueryView::From(v), kDFJoin, "join");
    for (auto v : query.Maps())       wrap(QueryView::From(v), kDFMap, "map");
    for (auto v : query.Aggregates()) wrap(QueryView::From(v), kDFAggregate, "aggregate");
    for (auto v : query.Merges())     wrap(QueryView::From(v), kDFMerge, "merge");
    for (auto v : query.Negations())  wrap(QueryView::From(v), kDFNegate, "negate");
    for (auto v : query.Compares())   wrap(QueryView::From(v), kDFCompare, "compare");
    for (auto v : query.Inserts())    wrap(QueryView::From(v), kDFInsert, "insert");
  };

  // PASS 1 — the bijection witness: the raw det_seq stamps over the live
  // traversal must be exactly a bijection onto {0..N-1}. Always-on
  // (fprintf+abort survives NDEBUG) — the accessor's stamped-ness assert
  // compiles out in release, so this is the release authority for the dump
  // path. max==count-1 is insufficient (duplicate-with-gap passes it); the
  // seen-bitset with s < N is exact by pigeonhole, and N==0-safe.
  unsigned num_views = 0u;
  for_each_df_view([&num_views](QueryView, unsigned, const char *) {
    ++num_views;
  });

  std::vector<bool> seen(num_views, false);
  for_each_df_view([&seen, num_views](QueryView v, unsigned, const char *) {
    const unsigned s = v.impl->det_seq;  // Raw read: our diagnostic, not the
                                         // accessor's debug assert.
    if (s >= num_views) {  // Subsumes the ~0u unstamped sentinel.
      fprintf(stderr,
              "DF-BIJECTION: view det_seq %u out of range [0,%u) "
              "(unstamped or gapped view reached the -df-out drain)\n",
              s, num_views);
      abort();
    }
    if (seen[s]) {
      fprintf(stderr,
              "DF-BIJECTION: duplicate det_seq %u over %u live views "
              "(a view-killing pass gapped the sequence)\n",
              s, num_views);
      abort();
    }
    seen[s] = true;
  });

  // Token rendering goes through a nested OutputStream over a string buffer
  // so lines can be composed and padded (pin p6) before hitting `os`.
  std::ostringstream buf;
  OutputStream bos(os.display_manager, buf);
  const auto take = [&buf]() {
    auto s = buf.str();
    buf.str("");
    return s;
  };

  // Column NAME token: finalized variable (streams AutoVar_<n> for unnamed
  // variables via the ParsedVariable operator<<), else c<id>. Never
  // _MissingVar (the std::optional operator is never used). Constant columns
  // also render c<id> — their token form is UNPINNED (PIN-1: no contract
  // witnesses one; pin before blessing any constant-carrying dump).
  const auto name_tok = [&](QueryColumn c) -> std::string {
    if (!c.IsConstantOrConstantRef()) {
      if (auto var = c.Variable()) {
        bos << *var;
        return take();
      }
    }
    bos << 'c' << c.Id();
    return take();
  };
  const auto typed_tok = [&](QueryColumn c) -> std::string {
    auto s = name_tok(c);
    bos << c.Type();
    return s + ":" + take();
  };

  // Pin p6: trailing comments start at byte 52 — content padded through byte
  // 51; content already >= 51 bytes gets exactly one space.
  const auto with_comment = [](std::string content, const std::string &c) {
    if (c.empty()) {
      return content;
    }
    if (content.size() < 51) {
      content.append(51 - content.size(), ' ');
    } else {
      content.push_back(' ');
    }
    return content + "; " + c;
  };

  // Kind tags for rendering `^<kind>.<det_seq>` references to other blocks.
  std::unordered_map<QueryView, const char *> kind_of;
  for_each_df_view([&kind_of](QueryView v, unsigned, const char *tag) {
    kind_of[v] = tag;
  });

  // Every rendered reference must resolve inside the live traversal set —
  // a referenced view outside it (e.g. a dead merged view) would otherwise
  // surface as an opaque out_of_range; abort with the validator idiom.
  const auto ref = [&kind_of](QueryView v) -> std::string {
    const auto it = kind_of.find(v);
    if (it == kind_of.end()) {
      fprintf(stderr,
              "DF-REF: referenced view is outside the live dump traversal\n");
      abort();
    }
    return std::string("^") + it->second + "." +
           std::to_string(v.DeterministicOrder());
  };

  // ------------------------------------------------------------------
  // The `=>` edge model: for each producer, one line per (user view [, .in<K>
  // port group for JOIN users]), entries in user input-port order. Built by
  // walking every USER's input columns (the inverse of the DOT's use->def
  // edges). Also the forward adjacency for the cycle SCC below — the SCC runs
  // over THIS edge set, never QueryView::Successors(), which additionally
  // carries INSERT->SELECT materialization edges the `=>` grammar never
  // renders (marking those would over-mark materialization-closed recursion).
  struct EdgeLine {
    QueryView user;
    int in_k;  // -1 for non-JOIN users; else the producer's JoinedViews()
               // position (rendered `.in<K>`).
    std::vector<std::pair<unsigned, std::string>> entries;  // (port, text)
  };
  std::unordered_map<QueryView, std::vector<EdgeLine>> edges_of;
  std::unordered_map<QueryView, std::vector<QueryView>> succ_of;

  const auto find_line = [&edges_of, &succ_of](QueryView producer,
                                               QueryView user,
                                               int in_k) -> EdgeLine & {
    auto &lines = edges_of[producer];
    for (auto &l : lines) {
      if (l.user == user && l.in_k == in_k) {
        return l;
      }
    }
    succ_of[producer].push_back(user);
    lines.push_back(EdgeLine{user, in_k, {}});
    return lines.back();
  };

  // Non-JOIN user edge: entry is the bare producer token on identity (pin
  // p2), else `dst=src` (dst = the user's own finalized column token). A
  // missing dst (INSERT and summarized AGG inputs own no positional output
  // column) renders bare by construction. Constant inputs have no producer
  // block — skipped (PIN-1 territory).
  const auto add_edge = [&](QueryView user, unsigned port,
                            std::optional<QueryColumn> dst, QueryColumn src) {
    if (src.IsConstantOrConstantRef()) {
      return;
    }
    const auto producer = QueryView::Containing(src);
    auto &line = find_line(producer, user, -1);
    const auto s = name_tok(src);
    auto text = s;
    if (dst) {
      const auto d = name_tok(*dst);
      if (d != s) {
        text = d + "=" + s;
      }
    }
    line.entries.emplace_back(port, std::move(text));
  };

  // JOIN user edge (pin p3): pure producer tokens under a `.in<K>` tag; K =
  // the producer's JoinedViews() position; entry order = the join's
  // output-column-position port order restricted to .in<K> (p3-order). The
  // join block owns the role mapping.
  const auto add_join_edge = [&](QueryView user_join, unsigned k,
                                 unsigned port, QueryColumn src) {
    if (src.IsConstantOrConstantRef()) {
      return;
    }
    const auto producer = QueryView::Containing(src);
    auto &line = find_line(producer, user_join, static_cast<int>(k));
    line.entries.emplace_back(port, name_tok(src));
  };

  const auto cols_of = [](auto range) {
    std::vector<QueryColumn> cols;
    for (auto c : range) {
      cols.push_back(c);
    }
    return cols;
  };

  // JoinedViews() position of the view producing `col`. A self-join with the
  // same view on two sides would make this ambiguous — the dataflow proxies
  // such joins through distinct tuples, so first-match is exact.
  const auto join_k = [](QueryJoin j, QueryColumn col) -> unsigned {
    const auto producer = QueryView::Containing(col);
    auto k = 0u;
    for (auto jv : j.JoinedViews()) {
      if (jv == producer) {
        return k;
      }
      ++k;
    }
    fprintf(stderr,
            "DF-JOIN: input column's producer not in JoinedViews()\n");
    abort();
  };

  for_each_df_view([&](QueryView v, unsigned kind, const char *) {
    switch (kind) {
      case kDFSelect: {  // INSERT->SELECT materialization is never a `=>`.
        break;
      }
      case kDFTuple: {
        const auto tuple = QueryTuple::From(v);
        const auto outs = cols_of(tuple.Columns());
        for (auto i = 0u; i < tuple.NumInputColumns(); ++i) {
          add_edge(v, i, outs[i], tuple.NthInputColumn(i));
        }
        break;
      }
      case kDFKVIndex: {
        const auto kv = QueryKVIndex::From(v);
        const auto keys = cols_of(kv.KeyColumns());
        const auto vals = cols_of(kv.ValueColumns());
        auto port = 0u;
        auto i = 0u;
        for (auto c : kv.InputKeyColumns()) {
          add_edge(v, port++, keys[i++], c);
        }
        i = 0u;
        for (auto c : kv.InputValueColumns()) {
          add_edge(v, port++, vals[i++], c);
        }
        break;
      }
      case kDFJoin: {
        const auto join = QueryJoin::From(v);
        auto port = 0u;
        for (auto i = 0u; i < join.NumPivotColumns(); ++i) {
          for (auto c : join.NthInputPivotSet(i)) {
            add_join_edge(v, join_k(join, c), port++, c);
          }
        }
        for (auto i = 0u; i < join.NumMergedColumns(); ++i) {
          const auto c = join.NthInputMergedColumn(i);
          add_join_edge(v, join_k(join, c), port++, c);
        }
        break;
      }
      case kDFMap: {  // Copied inputs, then bound inputs mapped to their
                      // parameter's column.
        const auto map = QueryMap::From(v);
        const auto copied = cols_of(map.CopiedColumns());
        const auto mapped = cols_of(map.MappedColumns());
        auto port = 0u;
        for (auto i = 0u; i < map.NumCopiedColumns(); ++i) {
          add_edge(v, port++, copied[i], map.NthInputCopiedColumn(i));
        }
        const ParsedDeclaration decl(map.Functor());
        auto i = 0u;
        for (ParsedParameter param : decl.Parameters()) {
          if (param.Binding() == ParameterBinding::kBound) {
            add_edge(v, port++, mapped[param.Index()],
                     map.NthInputColumn(i++));
          }
        }
        break;
      }
      case kDFMerge: {  // Positional edges from every merged view.
        const auto merge = QueryMerge::From(v);
        const auto outs = cols_of(merge.Columns());
        for (auto mv : merge.MergedViews()) {
          const auto ins = cols_of(mv.Columns());
          for (auto i = 0u; i < outs.size(); ++i) {
            add_edge(v, i, outs[i], ins[i]);
          }
        }
        break;
      }
      case kDFAggregate: {  // group/config inputs map positionally onto the
                   // leading output columns; summarized inputs have no
                   // positional output (rendered bare). PROVISIONAL until a
                   // contract carries an aggregate block.
        const auto agg = QueryAggregate::From(v);
        const auto outs = cols_of(agg.Columns());
        const auto num_group = agg.NumGroupColumns();
        const auto num_config = agg.NumConfigurationColumns();
        auto port = 0u;
        for (auto i = 0u; i < num_group; ++i) {
          add_edge(v, port++, outs[i], agg.NthInputGroupColumn(i));
        }
        for (auto i = 0u; i < num_config; ++i) {
          add_edge(v, port++, outs[num_group + i],
                   agg.NthInputConfigurationColumn(i));
        }
        for (auto i = 0u; i < agg.NumAggregateColumns(); ++i) {
          add_edge(v, port++, std::nullopt, agg.NthInputAggregateColumn(i));
        }
        break;
      }
      case kDFNegate: {  // Copied inputs, then the checked inputs against
                         // the negated columns. PROVISIONAL until a contract
                         // carries a negate block.
        const auto neg = QueryNegate::From(v);
        const auto copied = cols_of(neg.CopiedColumns());
        const auto negated = cols_of(neg.NegatedColumns());
        auto port = 0u;
        for (auto i = 0u; i < neg.NumCopiedColumns(); ++i) {
          add_edge(v, port++, copied[i], neg.NthInputCopiedColumn(i));
        }
        for (auto i = 0u; i < neg.NumInputColumns(); ++i) {
          add_edge(v, port++, negated[i], neg.NthInputColumn(i));
        }
        break;
      }
      case kDFCompare: {  // Ports follow the finalized column order:
                          // LHS(,RHS), then copied (CopiedColumns starts at
                          // columns.begin()+1/+2 — the review's F1 catch).
        const auto cmp = QueryCompare::From(v);
        const auto copied = cols_of(cmp.CopiedColumns());
        const auto in_copied = cols_of(cmp.InputCopiedColumns());
        auto port = 0u;
        add_edge(v, port++, cmp.LHS(), cmp.InputLHS());
        add_edge(v, port++, cmp.RHS(), cmp.InputRHS());
        for (auto i = 0u; i < copied.size(); ++i) {
          add_edge(v, port++, copied[i], in_copied[i]);
        }
        break;
      }
      case kDFInsert: {  // No finalized output columns -> bare entries.
        const auto insert = QueryInsert::From(v);
        for (auto i = 0u; i < insert.NumInputColumns(); ++i) {
          add_edge(v, i, std::nullopt, insert.NthInputColumn(i));
        }
        break;
      }
    }
  });

  // ------------------------------------------------------------------
  // `; cycle` (decision a2-i): an edge def->user is marked iff def is
  // reachable from user over the `=>` graph — i.e. iff both endpoints share
  // an SCC of that graph (the edge itself supplies def->user). One iterative
  // Tarjan pass, seeded in det_seq order so component numbering (unobserved
  // beyond equality) is deterministic anyway.
  std::unordered_map<QueryView, unsigned> tarjan_index, tarjan_low, scc_id;
  std::unordered_map<QueryView, bool> on_stack;
  std::vector<QueryView> scc_stack;
  unsigned next_index = 0u, next_scc = 0u;

  struct Frame {
    QueryView v;
    size_t next_child;
  };
  const std::vector<QueryView> no_children;

  for_each_df_view([&](QueryView root, unsigned, const char *) {
    if (tarjan_index.count(root)) {
      return;
    }
    std::vector<Frame> work;
    tarjan_index[root] = tarjan_low[root] = next_index++;
    scc_stack.push_back(root);
    on_stack[root] = true;
    work.push_back(Frame{root, 0u});

    while (!work.empty()) {
      auto &frame = work.back();
      const auto it = succ_of.find(frame.v);
      const auto &children = (it != succ_of.end()) ? it->second : no_children;
      if (frame.next_child < children.size()) {
        const auto child = children[frame.next_child++];
        if (!tarjan_index.count(child)) {
          tarjan_index[child] = tarjan_low[child] = next_index++;
          scc_stack.push_back(child);
          on_stack[child] = true;
          work.push_back(Frame{child, 0u});
        } else if (on_stack[child]) {
          tarjan_low[frame.v] =
              std::min(tarjan_low[frame.v], tarjan_index[child]);
        }
      } else {
        const auto finished = frame.v;
        if (tarjan_low[finished] == tarjan_index[finished]) {
          while (true) {
            const auto w = scc_stack.back();
            scc_stack.pop_back();
            on_stack[w] = false;
            scc_id[w] = next_scc;
            if (w == finished) {
              break;
            }
          }
          ++next_scc;
        }
        work.pop_back();
        if (!work.empty()) {  // Propagate the child's lowlink to its parent.
          tarjan_low[work.back().v] =
              std::min(tarjan_low[work.back().v], tarjan_low[finished]);
        }
      }
    }
  });

  // ------------------------------------------------------------------
  // PASS 2 — emission.

  // PIN-3 (discharged): class= is a TABLE property. Aggregate each
  // %table:N's deletion-capability once, over its live members, so every
  // block sharing a table renders one consistent class (a producer view —
  // a non-@never NEGATE, an AGGREGATE / KVINDEX, an impure MAP — makes
  // its whole table differential even though it does not RECEIVE
  // deletions). CanReceiveDeletions() implies CanProduceDeletions()
  // (Differential.cpp), so the explicit disjunction below equals bare
  // CanProduceDeletions(); the self-documenting form is kept.
  std::unordered_map<unsigned, bool> table_is_differential;
  for_each_df_view([&](QueryView v, unsigned, const char *) {
    const auto tid = v.TableId();
    if (!tid) {
      return;  // Table-less views contribute nothing.
    }
    // OR-fold: any member ⇒ table diff (operator[] value-initializes the
    // accumulator to false; insertion IS the job here — the checked-lookup
    // discipline applies on the emission side below, where an insert would
    // mask a live/dead skew).
    table_is_differential[*tid] |=
        v.CanReceiveDeletions() || v.CanProduceDeletions();
  });

  const auto attrs_line = [&](QueryView v, bool omit_table) -> std::string {
    std::string r = "  ATTRIBUTES";
    const auto table_id = v.TableId();
    if (table_id && !omit_table) {
      r += " table=%table:" + std::to_string(*table_id);
    }
    r += " class=";
    if (!table_id) {
      r += "table-less";
    } else {
      // Checked lookup: this view HAS table_id and the pre-pass enrolled
      // every table-bearing live view, so the key is guaranteed present.
      // A miss means emission reached a view the pre-pass skipped — a
      // live/dead skew; abort loudly (the DF-REF/DF-JOIN idiom), never a
      // silent operator[] insert or an opaque .at() throw.
      const auto it = table_is_differential.find(*table_id);
      if (it == table_is_differential.end()) {
        fprintf(stderr,
                "DF-CLASS: view on %%table:%u absent from the class "
                "aggregate (live/dead skew in the .df drain)\n",
                *table_id);
        abort();
      }
      r += it->second ? "differential" : "monotone";
    }
    if (auto stratum = v.Stratum()) {
      r += " stratum=" + std::to_string(*stratum);
    }
    const auto set = v.InductionGroupId();
    const auto depth = v.InductionDepth();
    if (set && depth) {
      r += " set=" + std::to_string(*set) + " depth=" + std::to_string(*depth);
    }
    return r;
  };

  const auto emit_edges = [&](QueryView producer) {
    const auto it = edges_of.find(producer);
    if (it == edges_of.end()) {
      return;
    }
    auto &lines = it->second;
    // (user det_seq, .in<K>) is total: det_seq is a bijection, and lines to
    // the same user are already merged per (user, K) by find_line.
    std::sort(lines.begin(), lines.end(),
              [](const EdgeLine &a, const EdgeLine &b) {
                const auto ad = a.user.DeterministicOrder();
                const auto bd = b.user.DeterministicOrder();
                if (ad != bd) {
                  return ad < bd;
                }
                return a.in_k < b.in_k;
              });
    for (auto &line : lines) {
      std::sort(line.entries.begin(), line.entries.end(),
                [](const auto &a, const auto &b) { return a.first < b.first; });
      std::string content = "  => " + ref(line.user);
      if (line.in_k >= 0) {
        content += " .in" + std::to_string(line.in_k);
        content += "(";
      } else {
        content += " (";
      }
      auto sep = "";
      for (auto &[port, text] : line.entries) {
        content += sep + text;
        sep = ", ";
      }
      content += ")";
      const auto cyclic = scc_id.at(producer) == scc_id.at(line.user);
      os << with_comment(std::move(content), cyclic ? "cycle" : "") << "\n";
    }
  };

  const auto header = [&](QueryView v, const char *kind_tag,
                          const std::vector<QueryColumn> &cols,
                          const std::string &comment) -> std::string {
    std::string content = std::string(kind_tag) + " ^" + kind_tag + "." +
                          std::to_string(v.DeterministicOrder()) + " (";
    auto sep = "";
    for (auto c : cols) {
      content += sep + typed_tok(c);
      sep = ", ";
    }
    content += ")";
    return with_comment(std::move(content), comment);
  };

  os << "dataflow\n";

  for_each_df_view([&](QueryView v, unsigned kind, const char *kind_tag) {
    os << "\n";
    switch (kind) {
      case kDFSelect: {  // Pin p1: keyword is the id kind; provenance in
                         // the trailing comment, pin p7 spelling.
        const auto select = QuerySelect::From(v);
        std::string prov;
        if (select.IsStream() && select.Stream().IsIO()) {
          // A non-IO stream is a CONSTANT stream (clause literals, the
          // unit-relation TrueColumn, tags) — its provenance form is
          // unpinned (PIN-1); no comment until a contract pins it. The
          // unguarded QueryIO::From asserted/UB'd on those (review catch).
          const auto io = QueryIO::From(select.Stream());
          bos << ParsedDeclarationName(io.Declaration());
          prov = "recv #message " + take() + "/" +
                 std::to_string(io.Declaration().Arity());
        } else if (select.IsRelation()) {
          const auto decl = select.Relation().Declaration();
          bos << ParsedDeclarationName(decl);
          prov = "relation " + take() + "/" + std::to_string(decl.Arity());
        }
        os << header(v, kind_tag, cols_of(select.Columns()), prov) << "\n"
           << attrs_line(v, false) << "\n";
        emit_edges(v);
        break;
      }
      case kDFJoin: {  // Spec §1.3 grammar + p9 body form.
        const auto join = QueryJoin::From(v);
        std::vector<QueryColumn> outs;
        for (auto i = 0u; i < join.NumPivotColumns(); ++i) {
          outs.push_back(join.NthOutputPivotColumn(i));
        }
        for (auto i = 0u; i < join.NumMergedColumns(); ++i) {
          outs.push_back(join.NthOutputMergedColumn(i));
        }
        os << header(v, kind_tag, outs, "") << " {\n";
        for (auto i = 0u; i < join.NumPivotColumns(); ++i) {
          std::string line =
              "  pivot " + typed_tok(join.NthOutputPivotColumn(i)) + " <- ";
          auto sep = "";
          for (auto c : join.NthInputPivotSet(i)) {
            line += sep + std::string(".in") +
                    std::to_string(join_k(join, c)) + "." + name_tok(c);
            sep = ", ";
          }
          os << line << "\n";
        }
        for (auto i = 0u; i < join.NumMergedColumns(); ++i) {
          const auto c = join.NthInputMergedColumn(i);
          os << "  out " << typed_tok(join.NthOutputMergedColumn(i)) << " <- "
             << ".in" << join_k(join, c) << "." << name_tok(c) << "\n";
        }
        os << "}\n" << attrs_line(v, false) << "\n";
        emit_edges(v);
        break;
      }
      case kDFMap: {  // Functor provenance comment.
        const auto map = QueryMap::From(v);
        bos << ParsedDeclarationName(map.Functor());
        const auto prov = (map.IsPositive() ? "" : std::string("!")) + take();
        os << header(v, kind_tag, cols_of(map.Columns()), prov) << "\n"
           << attrs_line(v, false) << "\n";
        emit_edges(v);
        break;
      }
      case kDFMerge: {  // `; callers:` ascending det_seq (spec §1.3).
        const auto merge = QueryMerge::From(v);
        std::vector<QueryView> callers;
        for (auto mv : merge.MergedViews()) {
          callers.push_back(mv);
        }
        std::sort(callers.begin(), callers.end(),
                  [](QueryView a, QueryView b) {
                    return a.DeterministicOrder() < b.DeterministicOrder();
                  });
        std::string comment = "callers:";
        auto sep = " ";
        for (auto c : callers) {
          comment += sep + ref(c);
          sep = ", ";
        }
        os << header(v, kind_tag, cols_of(merge.Columns()), comment) << "\n"
           << attrs_line(v, false) << "\n";
        emit_edges(v);
        break;
      }
      case kDFAggregate: {  // Functor provenance comment.
        const auto agg = QueryAggregate::From(v);
        bos << ParsedDeclarationName(agg.Functor());
        os << header(v, kind_tag, cols_of(agg.Columns()), take()) << "\n"
           << attrs_line(v, false) << "\n";
        emit_edges(v);
        break;
      }
      case kDFNegate: {  // `; [never] negates ^<target>`.
        const auto neg = QueryNegate::From(v);
        std::string prov = neg.HasNeverHint() ? "never negates " : "negates ";
        prov += ref(neg.NegatedView());
        os << header(v, kind_tag, cols_of(neg.Columns()), prov) << "\n"
           << attrs_line(v, false) << "\n";
        emit_edges(v);
        break;
      }
      case kDFCompare: {  // `; <op>` provenance. Header columns in the
                          // view's FINALIZED order — LHS(,RHS), then copied
                          // (the review's F1 catch; QueryView::Columns() is
                          // exactly that order).
        const auto cmp = QueryCompare::From(v);
        const char *op = "";
        switch (cmp.Operator()) {
          case ComparisonOperator::kEqual: op = "eq"; break;
          case ComparisonOperator::kGreaterThan: op = "gt"; break;
          case ComparisonOperator::kLessThan: op = "lt"; break;
          case ComparisonOperator::kNotEqual: op = "neq"; break;
        }
        os << header(v, kind_tag, cols_of(v.Columns()), op) << "\n"
           << attrs_line(v, false) << "\n";
        emit_edges(v);
        break;
      }
      case kDFInsert: {  // Terminal; `into %table:` in the header, table=
                         // omitted from ATTRIBUTES (redundant).
        const auto insert = QueryInsert::From(v);
        std::vector<QueryColumn> ins;
        for (auto i = 0u; i < insert.NumInputColumns(); ++i) {
          ins.push_back(insert.NthInputColumn(i));
        }
        auto content = header(v, kind_tag, ins, "");  // No comment -> the
                                                      // raw header content.
        if (auto table_id = v.TableId()) {
          content += " into %table:" + std::to_string(*table_id);
        }
        os << content << "\n" << attrs_line(v, true) << "\n";
        break;
      }
      default: {  // tuple / kv_index share the plain form.
        const auto cols = (kind == kDFTuple)
                              ? cols_of(QueryTuple::From(v).Columns())
                              : cols_of(QueryKVIndex::From(v).Columns());
        os << header(v, kind_tag, cols, "") << "\n"
           << attrs_line(v, false) << "\n";
        emit_edges(v);
        break;
      }
    }
  });

  return os;
}

}  // namespace hyde
