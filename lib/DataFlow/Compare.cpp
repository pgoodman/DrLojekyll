// Copyright 2020, Trail of Bits. All rights reserved.

#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Util/EqualitySet.h>

#include "Optimize.h"
#include "Query.h"

namespace hyde {

QueryCompareImpl::QueryCompareImpl(ComparisonOperator op_) : op(op_) {}

QueryCompareImpl::~QueryCompareImpl(void) {}

QueryCompareImpl *QueryCompareImpl::AsCompare(void) noexcept {
  return this;
}

const char *QueryCompareImpl::KindName(void) const noexcept {
  return "COMPARE";
}

uint64_t QueryCompareImpl::Hash(void) noexcept {
  if (hash) {
    return hash;
  }

  // Base case for recursion.
  hash = HashInit() ^ static_cast<unsigned>(op);
  assert(hash != 0);

  auto local_hash = hash;

  for (auto col : input_columns) {
    local_hash ^= RotateRight64(local_hash, 53) * col->Hash();
  }

  for (auto col : attached_columns) {
    local_hash ^= RotateRight64(local_hash, 43) * col->Hash();
  }

  hash = local_hash;
  return local_hash;
}

// Puts this comparison (CMP) node into a canonical form, which makes
// structural comparisons and replacements easier. A CMP filters the rows of
// its predecessor by comparing two input columns with `op`; an equality CMP
// fuses the compared pair into a single output column (outputs are
// `[eq, attached...]`), while an inequality CMP keeps both (outputs are
// `[lhs, rhs, attached...]`), and all attached columns are copied through
// unchanged. Canonicalization folds comparisons whose outcome is known at
// compile time: a trivially-true CMP is replaced by a TUPLE that simply
// forwards its inputs (deferring constant propagation to the TUPLE's own
// canonicalizer), and a trivially-false CMP marks itself unsatisfiable so
// that dead-code elimination can prune the dependent subgraph. It also
// propagates constants through input/output column pairs, drops unused
// attached outputs, and, when nothing else changes, tries to sink the CMP
// through a preceding MERGE or NEGATE so that filtering happens closer to the
// data sources. Constant folding of `=`/`!=` over two distinct constants
// requires both to be "unique" constants (tags, or foreign constants declared
// unique), since only then does node distinctness prove value distinctness.
//
//    if dead, unsatisfiable, or invalid:   nothing to do
//    pull inputs through trivial forwarding TUPLEs
//    if predecessor is unsatisfiable:      mark self unsatisfiable
//    if op is '=':
//      map both inputs to the single fused output; propagate constants
//      if lhs and rhs are the same column: replace self w/ forwarding TUPLE
//      if lhs, rhs distinct unique consts: mark self unsatisfiable
//    else:
//      map lhs -> out[0], rhs -> out[1]; propagate constants
//      if lhs is rhs, or both are the same constant: mark unsatisfiable
//      if op is '!=' and lhs, rhs are distinct unique constants:
//        replace self with forwarding TUPLE
//    propagate constants through the attached columns
//    if nothing changed:  try sinking self through a MERGE or NEGATE
//    else:
//      possibly guard users with a TUPLE that re-derives constant or
//        duplicated outputs, so this CMP can shed them
//      rebuild output columns: fused '=' output, then only the attached
//        columns that are used; resolve constant-ref inputs to constants
//      keep-last-edge rule: if the rebuilt lists would drop the last data
//        edge to the predecessor, one representative column keeps reading
//        from it raw (an input operand when possible, else an attached
//        column retained even though its output is unused)
//
// Trivially satisfiable comparison, e.g. `A = A` over the same column:
//
//        VIEW                        VIEW
//      A  X  Y                     A  X  Y
//      |  |  |          ==>        |  |  |
//     CMP(A = A)                   TUPLE
//      A' X' Y'                    A' X' Y'
//
// Unsatisfiable comparison, e.g. `A < A`, `A != A`, or `1 = 2` over unique
// constants:
//
//        VIEW
//      A  X  Y          ==>     CMP marked UNSAT; its users are
//      |  |  |                  pruned by later dead-code passes
//     CMP(A < A)
//
// Unused attached output removal (`Y'` has no users):
//
//        VIEW                        VIEW
//     A  B  X  Y                  A  B  X  Y
//     |  |  |  |        ==>       |  |  |
//     CMP(A < B)                  CMP(A < B)
//     A' B' X' Y'                 A' B' X'
bool QueryCompareImpl::Canonicalize(QueryImpl *query,
                                      const OptimizationContext &opt,
                                      const ErrorLog &log) {

  if (is_dead || is_unsat || valid != VIEW::kValid) {
    is_canonical = true;
    return false;
  }

  if (valid == VIEW::kValid &&
      !CheckIncomingViewsMatch(input_columns, attached_columns)) {
    valid = VIEW::kInvalidBeforeCanonicalize;
    return false;
  }

  const auto num_cols = columns.Size();
  auto first_attached_col = 1u;

  is_canonical = true;  // Updated by `CanonicalizeColumn`.
  in_to_out.clear();  // Filled in by `CanonicalizeColumn`.
  Discoveries has = {};

  // NOTE(pag): This may update `is_canonical`.
  const auto incoming_view = PullDataFromBeyondTrivialTuples(
      GetIncomingView(input_columns, attached_columns), input_columns,
      attached_columns);

  if (incoming_view && incoming_view->is_unsat) {
    MarkAsUnsatisfiable();
    is_canonical = true;
    return true;
  }

  COL * const c0 = input_columns[0]->AsConstant();
  COL * const c1 = input_columns[1]->AsConstant();

  // Equality comparisons are merged into a single output.
  if (op == ComparisonOperator::kEqual) {
    has = CanonicalizeColumn(opt, input_columns[0], columns[0], false, has);
    has = CanonicalizeColumn(opt, input_columns[1], columns[0], false, has);

    // This is trivially satisfiable, create a tuple that forwards all of
    // the columns. We'll defer to the tuple's canonicalizer to continue
    // constant propagation.
    if (input_columns[0] == input_columns[1]) {
      TUPLE * const tuple = query->tuples.Create();
      tuple->color = color;
#ifndef NDEBUG
      tuple->producer = "TRIVIAL-EQ-CMP:" + producer;
#endif
      (void) tuple->columns.Create(columns[0]->var, columns[0]->type, tuple,
                                   columns[0]->id, 0u);
      tuple->input_columns.AddUse(input_columns[0]);
      for (auto i = 1u; i < num_cols; ++i) {
        (void) tuple->columns.Create(columns[i]->var, columns[i]->type, tuple,
                                     columns[i]->id, i);
        tuple->input_columns.AddUse(attached_columns[i - 1u]);
      }

      // NOTE(pag): This will transfer/fixup conditions.
      this->ReplaceAllUsesWith(tuple);
      return true;

    // This equality is unsatisfiable.
    } else if (c0 && c1 && c0 != c1 &&
               c0->IsUniqueConstant() && c1->IsUniqueConstant()) {
      MarkAsUnsatisfiable();
      is_canonical = true;
      return true;
    }

  // Inequality comparisons go to separate outputs.
  } else {
    has = CanonicalizeColumn(opt, input_columns[0], columns[0], false, has);
    has = CanonicalizeColumn(opt, input_columns[1], columns[1], false, has);
    first_attached_col = 2u;

    // This condition is unsatisfiable.
    if (input_columns[0] == input_columns[1] ||
        (c0 && c1 && c0 == c1)) {

      MarkAsUnsatisfiable();
      is_canonical = true;
      return true;

    // This inequality is trivially satisfiable.
    } else if (ComparisonOperator::kNotEqual == op && c0 && c1 && c0 != c1 &&
               c0->IsUniqueConstant() && c1->IsUniqueConstant()) {
      TUPLE * const tuple = query->tuples.Create();
      tuple->color = color;
#ifndef NDEBUG
      tuple->producer = "TRIVIAL-NE-CMP:" + producer;
#endif
      (void) tuple->columns.Create(columns[0]->var, columns[0]->type, tuple,
                                   columns[0]->id, 0u);
      (void) tuple->columns.Create(columns[1]->var, columns[1]->type, tuple,
                                   columns[1]->id, 1u);
      tuple->input_columns.AddUse(input_columns[0]);
      tuple->input_columns.AddUse(input_columns[1]);
      for (auto i = 2u; i < num_cols; ++i) {
        (void) tuple->columns.Create(columns[i]->var, columns[i]->type, tuple,
                                     columns[i]->id, i);
        tuple->input_columns.AddUse(attached_columns[i - 2u]);
      }

      // NOTE(pag): This will transfer/fixup conditions.
      this->ReplaceAllUsesWith(tuple);
      return true;
    }
  }

  // Do constant propagation on the attached columns.
  for (auto i = first_attached_col, j = 0u; i < num_cols; ++i, ++j) {
    has = CanonicalizeColumn(opt, attached_columns[j], columns[i], true, has);
  }

  // Nothing changed.
  if (is_canonical) {
    if (TrySink(query)) {
      return true;
    }
    return has.non_local_changes;
  }

  // There is at least one output of our compare that is a constant and that
  // can be guarded, or one duplicated column. Go create a tuple that will
  // only propagate forward the needed data.
  if (has.guardable_constant_output || has.duplicated_input_column) {
    if (!IsUsedDirectly() && !(OnlyUser() && has.directly_used_column)) {
      GuardWithOptimizedTuple(query, first_attached_col);
      has.non_local_changes = true;
    }
  }

  DefList<COL> new_columns(this);
  UseList<COL> new_input_columns(this);
  UseList<COL> new_attached_columns(this);

  // The keep-last-edge rule: resolving the operands to constants and
  // dropping unused attached columns must not sever the final column edge
  // to `incoming_view`, because that edge is what expresses this CMP's
  // presence dependency on its predecessor. When no surviving column would
  // read from `incoming_view`, one representative keeps reading from it
  // raw: an input operand when one comes from `incoming_view`, else an
  // attached column that is retained even though its output is unused. An
  // unused CMP produces rows for nobody, so it has no presence to preserve
  // and keeps nothing.
  const auto is_used = IsUsed();
  const auto num_attached_cols = attached_columns.Size();
  auto keep_input_index = 2u;
  auto keep_attached_index = num_attached_cols;
  if (incoming_view && is_used) {
    auto retains_edge = false;
    for (auto k = 0u; k < 2u && !retains_edge; ++k) {
      const auto in_col = input_columns[k];
      retains_edge =
          !in_col->IsConstantOrConstantRef() && in_col->view == incoming_view;
    }
    for (auto j = 0u; j < num_attached_cols && !retains_edge; ++j) {
      const auto in_col = attached_columns[j];
      retains_edge = columns[first_attached_col + j]->IsUsed() &&
                     !in_col->IsConstantOrConstantRef() &&
                     in_col->view == incoming_view;
    }
    if (!retains_edge) {
      for (auto k = 0u; k < 2u; ++k) {
        const auto in_col = input_columns[k];
        if (!in_col->IsConstant() && in_col->view == incoming_view) {
          keep_input_index = k;
          break;
        }
      }
      for (auto j = 0u;
           keep_input_index == 2u && j < num_attached_cols; ++j) {
        const auto in_col = attached_columns[j];
        if (!in_col->IsConstant() && in_col->view == incoming_view) {
          keep_attached_index = j;
          break;
        }
      }
      assert(keep_input_index < 2u || keep_attached_index < num_attached_cols);
    }
  }

  // Fixpoint check: the rebuild below drops unused attached columns and
  // resolves constant-ref inputs to constants. When the keep-last-edge
  // representative is the only unused attached column and every other
  // input is already fully resolved, the rebuild is an identity rewrite,
  // so the CMP is already in its final shape and the rebuild is skipped.
  auto rebuild_changes_shape = false;
  for (auto k = 0u; k < 2u && !rebuild_changes_shape; ++k) {
    rebuild_changes_shape =
        k != keep_input_index &&
        input_columns[k]->TryResolveToConstant() != input_columns[k];
  }
  for (auto j = 0u; j < num_attached_cols && !rebuild_changes_shape; ++j) {
    if (columns[first_attached_col + j]->IsUsed() ||
        j == keep_attached_index) {
      rebuild_changes_shape =
          j != keep_attached_index &&
          attached_columns[j]->TryResolveToConstant() != attached_columns[j];
    } else {
      rebuild_changes_shape = true;  // This attached column gets dropped.
    }
  }
  if (!rebuild_changes_shape) {
    hash = 0;
    is_canonical = true;
    return has.non_local_changes;
  }

  COL *new_lhs_out = nullptr;
  COL *new_rhs_out = nullptr;

  // Create and keep the new versions of the output columns.
  if (op == ComparisonOperator::kEqual) {
    new_lhs_out = new_columns.Create(columns[0]->var, columns[0]->type, this,
                                     columns[0]->id, 0u);
    new_rhs_out = new_lhs_out;

    columns[0]->ReplaceAllUsesWith(new_lhs_out);
  } else {
    new_lhs_out = new_columns.Create(columns[0]->var, columns[0]->type, this,
                                     columns[0]->id, 0u);
    new_rhs_out = new_columns.Create(columns[1]->var, columns[1]->type, this,
                                     columns[1]->id, 1u);

    columns[0]->ReplaceAllUsesWith(new_lhs_out);
    columns[1]->ReplaceAllUsesWith(new_rhs_out);
  }

  new_input_columns.AddUse(keep_input_index == 0u
                               ? input_columns[0]
                               : input_columns[0]->TryResolveToConstant());
  new_input_columns.AddUse(keep_input_index == 1u
                               ? input_columns[1]
                               : input_columns[1]->TryResolveToConstant());

  // Now bring in the attached columns: those that are used, plus the
  // keep-last-edge representative.
  for (auto j = first_attached_col, i = 0u; j < num_cols; ++j, ++i) {
    const auto col = columns[j];
    if (col->IsUsed() || i == keep_attached_index) {
      const auto new_col = new_columns.Create(col->var, col->type, this,
                                              col->id, new_columns.Size());
      col->ReplaceAllUsesWith(new_col);
      new_attached_columns.AddUse(
          i == keep_attached_index
              ? attached_columns[i]
              : attached_columns[i]->TryResolveToConstant());

    } else {
      has.non_local_changes = true;
    }
  }

  assert(!incoming_view || !is_used ||
         RetainsEdgeTo(incoming_view, new_input_columns,
                       new_attached_columns));

  columns.Swap(new_columns);
  input_columns.Swap(new_input_columns);
  attached_columns.Swap(new_attached_columns);

  hash = 0;
  is_canonical = true;

  if (!CheckIncomingViewsMatch(input_columns, attached_columns)) {
    valid = VIEW::kInvalidAfterCanonicalize;
  }

  return has.non_local_changes;
}

// Equality over compares is structural.
//
// NOTE(pag): The two inputs to the comparison being tested aren't always
//            ordered; however, equality testing here assumes ordering.
bool QueryCompareImpl::Equals(EqualitySet &eq,
                                QueryViewImpl *that_) noexcept {

  if (eq.Contains(this, that_)) {
    return true;
  }

  const auto that = that_->AsCompare();
  if (!that || op != that->op ||
      can_receive_deletions != that->can_receive_deletions ||
      can_produce_deletions != that->can_produce_deletions ||
      columns.Size() != that_->columns.Size() ||
      InsertSetsOverlap(this, that)) {
    return false;
  }

  eq.Insert(this, that);

  if (!ColumnsEq(eq, input_columns, that->input_columns) ||
      !ColumnsEq(eq, attached_columns, that->attached_columns)) {
    eq.Remove(this, that);
    return false;
  }

  return true;
}

// Tries to sink this comparison through its predecessor, dispatching to the
// MERGE- or NEGATE-specific rewrites below. Sinking pushes the filter closer
// to the data sources, shrinking the volume of tuples that flow through the
// predecessor and exposing further folding opportunities upstream. It only
// applies when `can_sink` is set, when there is a single incoming view, and
// when that view is not tested by a negation (which pins the predecessor's
// output in place).
//
//    if not can_sink:                                 give up
//    pred := incoming view of inputs + attached
//    if no pred, or pred is used by a NEGATE:         give up
//    if pred is a MERGE:   sink through the MERGE
//    if pred is a NEGATE:  sink through the NEGATE
//    otherwise:            give up
bool QueryCompareImpl::TrySink(QueryImpl *query) {
  if (!can_sink) {
    return false;
  }

  VIEW *pred = VIEW::GetIncomingView(input_columns, attached_columns);
  if (!pred || pred->is_used_by_negation) {
    return false;
  }

  if (auto merge = pred->AsMerge()) {
    return TrySinkThroughMerge(query, merge);
  } else if (auto negate = pred->AsNegate()) {
    return TrySinkThroughNegate(query, negate);
  } else {
    return false;
  }
}

// Sinks this comparison through a MERGE (UNION) node: the CMP is replicated
// onto every merged view, and a fresh MERGE that unions the replicas replaces
// this CMP. Filtering each union arm independently means tuples failing the
// comparison never enter the union at all, and each replica sits directly on
// a source view where it can fold further (e.g. against per-arm constants) or
// merge with identical CMPs via CSE. Every input or attached column of this
// CMP either originates in `merge` (and is remapped to the corresponding
// column of each merged view) or is a constant (and is used as-is). Each
// replica is marked `created_from_sinking` so that any unsatisfiable
// comparison it later proves is not reported as a user error.
//
//    create lifted MERGE with this CMP's column shape
//    for each view V merged by `merge`:
//      create replica CMP(op) reading from V
//      remap lhs/rhs/attached: column of `merge` -> column of V,
//                              constant -> itself
//      add replica to the lifted MERGE
//    replace all uses of this CMP with the lifted MERGE
//
//      V1     V2                     V1        V2
//       \     /                      |         |
//        UNION          ==>       CMP(A<B)  CMP(A<B)
//          |                          \       /
//       CMP(A<B)                       UNION
//          |                             |
//        users                         users
bool QueryCompareImpl::TrySinkThroughMerge(QueryImpl *query, MERGE *merge) {

  const auto num_cols = columns.Size();
  (void) num_cols;

  MERGE *lifted_merge = query->merges.Create();
  lifted_merge->color = color;

#ifndef NDEBUG
  lifted_merge->producer = "LIFTED-MERGE(" + this->producer + ")";
#endif

  auto col_index = 0u;
  for (auto col : columns) {
    (void) lifted_merge->columns.Create(
        col->var, col->type, lifted_merge, col->id, col_index++);
  }

  const auto lhs = input_columns[0];
  const auto rhs = input_columns[1];

  assert(lifted_merge->columns.Size() == num_cols);
  assert(lhs->type.Kind() == rhs->type.Kind());

  for (VIEW *merged_view : merge->merged_views) {
    CMP *sunk_cmp = query->compares.Create(op);
    sunk_cmp->color = color;
    sunk_cmp->created_from_sinking = true;
#ifndef NDEBUG
    sunk_cmp->producer = "SUNK-CMP-MERGE:" + this->producer;
#endif
    lifted_merge->merged_views.AddUse(sunk_cmp);

    if (lhs->view == merge) {
      sunk_cmp->input_columns.AddUse(merged_view->columns[lhs->Index()]);
    } else {
      assert(lhs->IsConstant());
      sunk_cmp->input_columns.AddUse(lhs);
    }

    if (rhs->view == merge) {
      sunk_cmp->input_columns.AddUse(merged_view->columns[rhs->Index()]);
    } else {
      assert(rhs->IsConstant());
      sunk_cmp->input_columns.AddUse(rhs);
    }

    if (ComparisonOperator::kEqual == op) {
      COL * const c0 = columns[0];
      (void) sunk_cmp->columns.Create(
          c0->var, c0->type, sunk_cmp, c0->id, 0u);
    } else {
      COL * const c0 = sunk_cmp->input_columns[0];
      COL * const c1 = sunk_cmp->input_columns[1];

      assert(c0->type.Kind() == c1->type.Kind());

      (void) sunk_cmp->columns.Create(
          c0->var, c0->type, sunk_cmp, c0->id, 0u);
      (void) sunk_cmp->columns.Create(
          c1->var, c1->type, sunk_cmp, c1->id, 1u);
    }

    col_index = 2u;
    for (auto col : attached_columns) {
      if (col->view == merge) {
        COL *const in_col = merged_view->columns[col->Index()];
        sunk_cmp->attached_columns.AddUse(in_col);

        (void) sunk_cmp->columns.Create(
            in_col->var, in_col->type, sunk_cmp, in_col->id, col_index++);
      } else {
        assert(col->IsConstant());
        sunk_cmp->attached_columns.AddUse(col);

        (void) sunk_cmp->columns.Create(
            col->var, col->type, sunk_cmp, col->id, col_index++);
      }
    }

    assert(sunk_cmp->columns.Size() == num_cols);
  }

  ReplaceAllUsesWith(lifted_merge);
  return true;
}

// Sinks this comparison through a NEGATE node by re-ordering the two
// operations: a lowered CMP filters the negation's input data first, a lifted
// NEGATE re-checks absence in the same negated view, and a lifted TUPLE on
// top restores this CMP's exact output column order and shape for existing
// users. This is sound because the negation only tests for the absence of a
// tuple in `negated_view` and does not transform data, so filtering before or
// after it yields the same rows; it is beneficial because rows failing the
// comparison never reach the (expensive) absence check. The rewrite is
// demand-driven: the lifted TUPLE demands columns from the lifted NEGATE,
// which in turn demands columns from the lowered CMP, with column-translation
// maps (CMP out -> CMP in, NEGATE out -> NEGATE in, ...) used to rewire each
// demanded column back to the negation's original input or to a constant. If
// any of the rebuilt nodes fails to remain directly connected to its intended
// predecessor, the three new nodes are discarded, `can_sink` is cleared, and
// the CMP is left untouched.
//
//    build column maps: cmp out->in, negate out->in, negate out->cmp out
//    create lifted TUPLE mirroring this CMP's outputs
//    create lifted NEGATE mirroring the negated view's columns
//    create lowered CMP(op) whose lhs/rhs are the negation's inputs that
//      feed this CMP (or constants)
//    demand each lifted-TUPLE column from the lifted NEGATE, appending
//      NEGATE outputs as needed
//    demand each lifted-NEGATE input from the lowered CMP, appending CMP
//      attached columns as needed
//    wire lowered CMP's attached columns to the negation's original inputs
//    if any connectivity check fails: delete new nodes, clear can_sink
//    else: replace all uses of this CMP with the lifted TUPLE
//
//      PRED                            PRED
//        |                               |
//      NEGATE ---- NEG'D    ==>       CMP(A=B)
//        |                               |
//      CMP(A=B)                       NEGATE ---- NEG'D
//        |                               |
//      users                           TUPLE   (restores CMP's
//                                        |      output order)
//                                      users
bool QueryCompareImpl::TrySinkThroughNegate(
    QueryImpl *query, NEGATION *negate) {

  // Maintains the output ordering of the columns of the CMP.
  TUPLE * const lifted_tuple = query->tuples.Create();
  NEGATION * const lifted_negate = query->negations.Create();
  CMP * const lowered_cmp = query->compares.Create(op);
  VIEW * const negated_view = negate->negated_view.get();
  lifted_negate->negated_view.Emplace(lifted_negate, negated_view);
  negated_view->is_used_by_negation = true;

  lifted_tuple->color = negate->color;
  lifted_negate->color = negate->color;
  lowered_cmp->color = this->color;
  lowered_cmp->created_from_sinking = true;

#ifndef NDEBUG
  lifted_negate->producer = "LIFTED-NEG:" + lifted_negate->producer;
  lowered_cmp->producer = "SUNK-CMP-NEG:" + this->producer;
#endif

  auto col_index = 0u;
//  const auto num_cols = columns.Size();


  const UseList<COL> *cmp_input_col_lists[] = {&input_columns, &attached_columns};
//  UseList<COL> &neg_input_col_lists[] = {negate->input_columns,
//                                         negate->attached_columns};

  // Maps outputs of the negation to outputs of the comparison.
  std::unordered_map<COL *, COL *> negate_out_to_cmp_out;
  std::unordered_map<COL *, COL *> cmp_out_to_cmp_in;
  QueryCompare(this).ForEachUse(
      [&] (QueryColumn in_col, InputColumnRole role,
           std::optional<QueryColumn> out_col) {
        if (out_col && out_col->impl->view == this) {
          cmp_out_to_cmp_in[out_col->impl] = in_col.impl;

          if (in_col.impl->view == negate) {
            negate_out_to_cmp_out[in_col.impl] = out_col->impl;
          }
        }
      });

  // Maps inputs to the negate to the outputs of the negate.
  std::unordered_map<COL *, COL *> negate_out_to_negate_in;
  QueryNegate(negate).ForEachUse(
      [&] (QueryColumn in_col, InputColumnRole role,
           std::optional<QueryColumn> out_col) {
        if (out_col && InputColumnRole::kNegated != role) {
          negate_out_to_negate_in[out_col->impl] = in_col.impl;
        }
      });

  auto lookup_col =
      [] (COL *col, std::unordered_map<COL *, COL *> &map) -> COL * {
        auto ret = map[col];
        if (!ret) {
          assert(col->IsConstant());
          return col->AsConstant();
        } else {
          return ret;
        }
      };

  // Maps inputs to the comparison `this` to inputs to the negate. These are
  // our comparison's lowered inputs.
  std::unordered_map<COL *, COL *> cmp_in_to_negate_in;
  for (const UseList<COL> *cmp_input_cols : cmp_input_col_lists) {
    for (COL *cmp_in : *cmp_input_cols) {
      cmp_in_to_negate_in[cmp_in] = lookup_col(cmp_in, negate_out_to_negate_in);
    }
  }

  // Key issues, annoyances:
  //
  //    1)  We need a `lifted_tuple` to maintain the output column ordering
  //        and shape of `this`.
  //    2)  We need `lifted_neg` to take its `input_columns` in the order of
  //        the negated view. `lifted_cmp` also needs to provide all the columns
  //        the `this` would have provided, for the sake of `lifted_tuple`.
  //    3)  We need `lowered_cmp` to take in all the inputs of the original
  //        negation, and any other inputs it might have needed.


  // Maps new columns to columns in either of the CMP or of the NEGATE
  std::unordered_map<COL *, COL *> new_to_old;

  // Fill out the tuple that's going to maintain the output column order of
  // this CMP.
  col_index = 0u;
  for (COL *col : columns) {
    COL *out_col = lifted_tuple->columns.Create(
        col->var, col->type, lifted_tuple, col->id, col_index++);

    new_to_old[out_col] = col;
  }

  // Now start building out the negation, using the negated view as the initial
  // guide. The `input_columns` have to be in the same order as
  // `negated_view->columns`.
  col_index = 0u;
  for (COL *col : negated_view->columns) {
    COL *old_neg_out = negate->columns[col_index];
    COL *out_col = lifted_negate->columns.Create(
        col->var, col->type, lifted_negate, col->id, col_index++);

    new_to_old[out_col] = old_neg_out;
  }

  // Now, start building out the comparison, using the input columns to the
  // comparison as the initial guide.
  std::unordered_map<COL *, COL *> cmp_in_to_lifted_cmp_out;
  std::unordered_map<COL *, COL *> cmp_out_to_lifted_cmp_out;
  COL * const cmp_i0 = input_columns[0];
  COL * const cmp_i1 = input_columns[1];
  COL * const cmp_o0 = columns[0];
  COL * const lifted_cmp_o0 = lowered_cmp->columns.Create(
      cmp_o0->var, cmp_o0->type, lowered_cmp, cmp_o0->id, 0u);
  cmp_in_to_lifted_cmp_out[cmp_i0] = lifted_cmp_o0;
  cmp_out_to_lifted_cmp_out[cmp_o0] = lifted_cmp_o0;
  new_to_old[lifted_cmp_o0] = cmp_o0;

  if (ComparisonOperator::kEqual == op) {
    cmp_in_to_lifted_cmp_out[cmp_i1] = lifted_cmp_o0;

  } else {
    COL * const cmp_o1 = columns[1];
    COL * const lifted_cmp_o1 = lowered_cmp->columns.Create(
        cmp_o1->var, cmp_o1->type, lowered_cmp, cmp_o1->id, 1u);

    cmp_in_to_lifted_cmp_out[cmp_i1] = lifted_cmp_o1;
    cmp_out_to_lifted_cmp_out[cmp_o1] = lifted_cmp_o1;

    new_to_old[lifted_cmp_o1] = cmp_o1;
  }

  lowered_cmp->input_columns.AddUse(
      lookup_col(cmp_i0, negate_out_to_negate_in));

  lowered_cmp->input_columns.AddUse(
      lookup_col(cmp_i1, negate_out_to_negate_in));

  assert(lowered_cmp->input_columns.Size() == 2u);

  // At this point, we have the following:
  //
  //    1)  Lifted tuple: has outputs columns matching `this`, but no input
  //        columns.
  //    2)  Lifted negate: has outputs matching the negated_view, but no
  //        inputs. It likely misses additional needed outputs to match with
  //        (1).
  //    3)  Lowered compare: has outputs and inputs for its comparison, but
  //        is missing all other needed things.

  // We will start the fixup process in a demand-driven way: we will demand
  // outputs from the lifted negate, and we'll add them in to the lifted tuple
  // as inputs.
  COL *demanded_col = nullptr;
  for (COL *lifted_tuple_out : lifted_tuple->columns) {
    COL * const cmp_out = new_to_old[lifted_tuple_out];
    assert(cmp_out->view == this);

    if (cmp_out->IsConstantOrConstantRef()) {
      lifted_tuple->input_columns.AddUse(cmp_out->AsConstant());
      goto next_col;
    }

    // Scan through the existing columns of the lifted negate, and try to match
    // them up with the comparison.
    for (COL *lifted_neg_out : lifted_negate->columns) {
      COL *old_in_col = new_to_old[lifted_neg_out];
      if (!old_in_col) {
        continue;
      }

      // The old input was part of the negate; try to translate it to being
      // in the comparison.
      if (old_in_col->view == negate) {
        COL *neg_out = old_in_col;
        if (negate_out_to_cmp_out[neg_out] == cmp_out) {
          new_to_old[lifted_neg_out] = cmp_out;
          lifted_tuple->input_columns.AddUse(lifted_neg_out);
          goto next_col;
        }

      // The old version of this column was from this comparison, so see if it's
      // the one we're looking for.
      } else if (old_in_col->view == this) {
        if (old_in_col == cmp_out) {
          new_to_old[lifted_neg_out] = cmp_out;
          lifted_tuple->input_columns.AddUse(lifted_neg_out);
          goto next_col;
        }

      } else if (old_in_col->IsConstant()) {
        lifted_tuple->input_columns.AddUse(old_in_col);
        goto next_col;
      }
    }

    // We're missing the column; go and add it.
    demanded_col = lifted_negate->columns.Create(
        lifted_tuple_out->var, lifted_tuple_out->type, lifted_negate,
        lifted_tuple_out->id, lifted_negate->columns.Size());

    new_to_old[demanded_col] = cmp_out;

    lifted_tuple->input_columns.AddUse(demanded_col);

  next_col:
    continue;
  }

  assert(lifted_tuple->input_columns.Size() == lifted_tuple->columns.Size());

  // If we produced a lifted tuple that drops connections to its predecessor
  // then disable sinking and give up.
  if (GetIncomingView(lifted_tuple->input_columns) != lifted_negate) {
    assert(false);
    lifted_tuple->PrepareToDelete();
    lifted_negate->PrepareToDelete();
    lowered_cmp->PrepareToDelete();
    can_sink = false;
    return false;
  }

  auto add_input_to_negate = [=] (COL *col, unsigned col_index) {
    if (col_index < negated_view->columns.Size()) {
      lifted_negate->input_columns.AddUse(col);
    } else {
      lifted_negate->attached_columns.AddUse(col);
    }
  };

  // Now do the fixup process on the lifted negation.
  for (COL *lifted_neg_out : lifted_negate->columns) {
    col_index = lifted_neg_out->Index();

    COL *old_out_col = new_to_old[lifted_neg_out];
    assert(old_out_col != nullptr);

    if (old_out_col->view == negate) {
      COL *cmp_out = negate_out_to_cmp_out[old_out_col];
      if (cmp_out) {
        COL *lifted_cmp_out = cmp_out_to_lifted_cmp_out[cmp_out];
        if (lifted_cmp_out) {
          add_input_to_negate(lifted_cmp_out, col_index);
          continue;
        }
      }

    } else if (old_out_col->view == this) {
      COL *cmp_out = old_out_col;
      COL *lifted_cmp_out = cmp_out_to_lifted_cmp_out[cmp_out];
      if (lifted_cmp_out) {
        add_input_to_negate(lifted_cmp_out, col_index);
        continue;
      }

    } else if (old_out_col->IsConstant()) {
      add_input_to_negate(old_out_col, col_index);
      continue;

    } else {
      assert(false);
    }

    // We need to introduce a new column into the comparison.
    std::unordered_map<COL *, COL *> lifted_cmp_col_to_neg_in_col;
    demanded_col = lowered_cmp->columns.Create(
        lifted_neg_out->var, lifted_neg_out->type, lowered_cmp,
        lifted_neg_out->id, lowered_cmp->columns.Size());

    new_to_old[demanded_col] = old_out_col;

    add_input_to_negate(demanded_col, col_index);
  }

  // If we produced a lifted tuple that drops connections to its predecessor
  // then disable sinking and give up.
  if (GetIncomingView(lifted_negate->input_columns,
                      lifted_negate->attached_columns) != lowered_cmp) {
    assert(false);
    lifted_tuple->PrepareToDelete();
    lifted_negate->PrepareToDelete();
    lowered_cmp->PrepareToDelete();
    can_sink = false;
    return false;
  }

  // Finally, fixup the comparison.
  col_index = 2u;
  if (ComparisonOperator::kEqual == op) {
    col_index = 1u;
  }

  VIEW * const negate_incoming_view = GetIncomingView(
      negate->input_columns, negate->attached_columns);

  for (auto i = col_index, max_i = lowered_cmp->columns.Size();
       i < max_i; ++i) {

    COL * const lowered_cmp_col = lowered_cmp->columns[i];

    COL *old_out_col = new_to_old[lowered_cmp_col];
    assert(old_out_col != nullptr);

    if (old_out_col->view == negate) {
      COL *neg_in = negate_out_to_negate_in[old_out_col];
      assert(neg_in->view == negate_incoming_view);
      lowered_cmp->attached_columns.AddUse(neg_in);
      continue;

    } else if (old_out_col->view == this) {
      COL *cmp_in = cmp_out_to_cmp_in[old_out_col];
      if (cmp_in->view == negate) {
        COL *neg_in = negate_out_to_negate_in[cmp_in];
        lowered_cmp->attached_columns.AddUse(neg_in);

      } else if (cmp_in->IsConstant()) {
        lowered_cmp->attached_columns.AddUse(cmp_in);
        continue;

      } else {
        assert(false);
      }

    } else if (old_out_col->IsConstant()) {
      lowered_cmp->attached_columns.AddUse(old_out_col);
      continue;

    } else {
      assert(false);
    }
  }

  // Make sure we connect in the same way.
  if (negate_incoming_view !=
      GetIncomingView(lowered_cmp->input_columns,
                      lowered_cmp->attached_columns)) {
    assert(false);
    lifted_tuple->PrepareToDelete();
    lifted_negate->PrepareToDelete();
    lowered_cmp->PrepareToDelete();
    can_sink = false;
    return false;
  }

  this->ReplaceAllUsesWith(lifted_tuple);

  //throw 0;

  return true;
}

}  // namespace hyde
