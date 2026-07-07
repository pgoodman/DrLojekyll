// Copyright 2020, Trail of Bits. All rights reserved.

#include <drlojekyll/Util/EqualitySet.h>

#include "Optimize.h"
#include "Query.h"

namespace hyde {

QueryMapImpl::~QueryMapImpl(void) {}

QueryMapImpl::QueryMapImpl(ParsedFunctor functor_, DisplayRange range_,
                           bool is_positive_)
    : range(range_),
      functor(functor_),
      is_positive(is_positive_) {
  this->can_produce_deletions = !functor.IsPure();
  ParsedDeclaration decl(functor);
  for (ParsedParameter param : decl.Parameters()) {
    if (ParameterBinding::kFree == param.Binding()) {
      ++num_free_params;
    }
  }
}

QueryMapImpl *QueryMapImpl::AsMap(void) noexcept {
  return this;
}

const char *QueryMapImpl::KindName(void) const noexcept {
  if (num_free_params) {
    if (functor.IsPure()) {
      return "MAP";
    } else {
      return "FUNCTION";
    }
  } else {
    if (functor.IsPure()) {
      return "PREDICATE";
    } else {
      return "FILTER";
    }
  }
}

uint64_t QueryMapImpl::Sort(void) noexcept {
  return range.From().Index();
}

static const std::hash<std::string_view> kStringViewHasher;

uint64_t QueryMapImpl::Hash(void) noexcept {
  if (hash) {
    return hash;
  }

  const auto binding_pattern = ParsedDeclaration(functor).BindingPattern();

  hash = RotateRight64(HashInit(), 43) ^ functor.Id();
  assert(hash != 0);

  if (!is_positive) {
    hash = ~hash;
  }
  hash ^= RotateRight64(hash, 33) * kStringViewHasher(binding_pattern);

  auto local_hash = hash;

  // Mix in the hashes of the merged views and columns;
  for (auto input_col : input_columns) {
    local_hash ^= RotateRight64(local_hash, 23) * input_col->Hash();
  }

  for (auto input_col : attached_columns) {
    local_hash ^= RotateRight64(local_hash, 13) * input_col->Hash();
  }

  hash = local_hash;
  return local_hash;
}

// Put this map into a canonical form, which makes comparisons and
// replacements easier. Maps correspond to functors with inputs: the
// `input_columns` feed the functor's `bound` parameters, the first
// `functor.Arity()` output columns mirror the functor's parameters in
// declaration order (both `bound` copies and `free` results), and the
// `attached_columns` are extra columns copied through alongside each output
// row. Canonicalization pulls inputs from beyond trivial forwarding TUPLEs
// and single-source unconditional UNIONs, propagates unsatisfiability from
// the incoming view, propagates constant inputs into constant-ref outputs,
// deduplicates uses of outputs that repeat another output, drops unused
// attached outputs (and their inputs), and, when a constant-valued or
// duplicated output would otherwise flow to every user, interposes an
// optimized TUPLE between this MAP and its users so only the needed data
// propagates forward. This shrinks the MAP's width, exposes constants to
// downstream passes, and makes structurally identical MAPs hash and compare
// equal for CSE. Every `bound` input is always retained -- even when
// constant or duplicated -- because the functor needs all of its arguments
// at application time; that is why duplicate discoveries among the bound
// inputs are muted before processing the attached columns.
//
//    canonicalize(MAP):
//      if dead, unsat, or invalid:                     nothing to do
//      if input+attached columns span multiple views:  mark invalid
//      bypass trivial TUPLEs/UNIONs feeding input+attached columns
//      if incoming view is unsatisfiable:              mark unsat; done
//      for each (in_col, out_col) over bound params, then attached:
//        record in_to_out; copy constants in->out; note unused,
//        duplicated, and directly used outputs
//      (duplicates among bound params are ignored: all must be kept)
//      if nothing changed: done
//      if some output is a guardable constant or a duplicate, and no
//         user requires the raw MAP view itself:
//        interpose optimized TUPLE; users now read from the TUPLE
//      rebuild columns:
//        keep all functor-parameter outputs
//        drop unused attached outputs and their input columns
//        resolve each remaining input to its constant when possible
//        keep-last-edge rule: if the rebuilt lists would no longer
//          reference the incoming view, one representative column keeps
//          reading from it raw (a bound-parameter input when possible,
//          else an attached column retained though its output is unused),
//          so the MAP's row-presence dependency stays a column edge
//
// Bypassing a trivial forwarding TUPLE (same for a 1-source UNION):
//
//        V                          V
//        |                         /|
//      TUPLE            =>    TUPLE |    (TUPLE dies later if unused)
//        |                          |
//       MAP                        MAP
//
// Guarding with an optimized TUPLE, for functor f(bound B, free F) with
// attached columns [A, A'] fed by the same column A of V, and B fed the
// constant 7:
//
//        V                           V
//        |                           |
//   MAP f [B F A A']            MAP f [B F A A']
//      /       \         =>          |
//   user1     user2             TUPLE [7 F A A]
//                                  /       \
//                               user1     user2
//
// Keep-last-edge, for a MAP whose only input is a constant-ref (=3) column
// of V: the input keeps reading V's column raw instead of resolving to the
// constant, so the MAP still fires only for rows of V:
//
//        V[a=3]                   V[a=3]
//        |a                       |a
//     MAP f(a)          =>     MAP f(a)   (a stays; not folded to 3)
bool QueryMapImpl::Canonicalize(QueryImpl *query,
                                  const OptimizationContext &opt,
                                  const ErrorLog &) {

  if (is_dead || is_unsat || valid != VIEW::kValid) {
    is_canonical = true;
    return false;
  }

  if (valid == VIEW::kValid &&
      !CheckIncomingViewsMatch(input_columns, attached_columns)) {
    valid = VIEW::kInvalidBeforeCanonicalize;
    is_canonical = true;
    return false;
  }

  const auto arity = functor.Arity();
  const auto num_cols = columns.Size();
  const auto first_attached_col = arity;
  assert(arity <= num_cols);

  is_canonical = true;  // Updated by `CanonicalizeColumn`.
  in_to_out.clear();  // Filled in by `CanonicalizeColumn`.
  Discoveries has = {};

  // NOTE(pag): This may update `is_canonical`.
  const auto incoming_view = PullDataFromBeyondTrivialTuples(
      GetIncomingView(input_columns, attached_columns), input_columns,
      attached_columns);

  if (incoming_view && incoming_view->is_unsat) {
    MarkAsUnsatisfiable();
    return true;
  }

  auto i = 0u;
  for (auto j = 0u; i < arity; ++i) {
    if (functor.NthParameter(i).Binding() == ParameterBinding::kFree) {
      continue;  // It's an output column.
    }

    const auto out_col = columns[i];
    const auto in_col = input_columns[j++];
    has = CanonicalizeColumn(opt, in_col, out_col, false, has);
  }

  // NOTE(pag): Mute this, as we always need to maintain the `input_columns`
  //            and so we don't want to infinitely rewrite this map if
  //            there is a duplicate column in `input_columns`.
  has.duplicated_input_column = false;

  assert(arity <= i);
  for (auto j = 0u; i < num_cols; ++i, ++j) {
    has = CanonicalizeColumn(opt, attached_columns[j], columns[i], true, has);
  }

  // Nothing changed.
  if (is_canonical) {
    return has.non_local_changes;
  }

  // There is at least one output of our map that is a constant and that
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

  // The keep-last-edge rule: resolving inputs to constants and dropping
  // unused attached columns must not sever the final column edge to
  // `incoming_view`, because that edge is what expresses this MAP's
  // row-presence dependency on its predecessor. When no surviving column
  // would read from `incoming_view`, one representative keeps reading from
  // it raw: a bound-parameter input when one comes from `incoming_view`,
  // else an attached column that is retained even though its output is
  // unused. An unused MAP produces rows for nobody, so it has no presence
  // to preserve and keeps nothing.
  const auto is_used = IsUsed();
  const auto num_input_cols = input_columns.Size();
  const auto num_attached_cols = attached_columns.Size();
  auto keep_input_index = num_input_cols;
  auto keep_attached_index = num_attached_cols;
  if (incoming_view && is_used) {
    auto retains_edge = false;
    for (auto j = 0u; j < num_input_cols && !retains_edge; ++j) {
      const auto in_col = input_columns[j];
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
      for (auto j = 0u; j < num_input_cols; ++j) {
        const auto in_col = input_columns[j];
        if (!in_col->IsConstant() && in_col->view == incoming_view) {
          keep_input_index = j;
          break;
        }
      }
      for (auto j = 0u; keep_input_index == num_input_cols &&
                        j < num_attached_cols; ++j) {
        const auto in_col = attached_columns[j];
        if (!in_col->IsConstant() && in_col->view == incoming_view) {
          keep_attached_index = j;
          break;
        }
      }
      assert(keep_input_index < num_input_cols ||
             keep_attached_index < num_attached_cols);
    }
  }

  // Fixpoint check: the rebuild below drops unused attached columns and
  // resolves constant-ref inputs to constants. When the keep-last-edge
  // representative is the only unused attached column and every other
  // input is already fully resolved, the rebuild is an identity rewrite,
  // so the MAP is already in its final shape and the rebuild is skipped.
  auto rebuild_changes_shape = false;
  for (auto j = 0u; j < num_input_cols && !rebuild_changes_shape; ++j) {
    rebuild_changes_shape =
        j != keep_input_index &&
        input_columns[j]->TryResolveToConstant() != input_columns[j];
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

  i = 0u;
  for (auto j = 0u; i < arity; ++i) {
    const auto old_col = columns[i];
    const auto new_col =
        new_columns.Create(old_col->var, old_col->type, this, old_col->id, i);
    old_col->ReplaceAllUsesWith(new_col);

    // It's an input column.
    if (functor.NthParameter(i).Binding() != ParameterBinding::kFree) {
      new_input_columns.AddUse(
          j == keep_input_index ? input_columns[j]
                                : input_columns[j]->TryResolveToConstant());
      ++j;
    }
  }

  assert(arity <= i);
  for (auto j = 0u; i < num_cols; ++i, ++j) {
    const auto old_col = columns[i];
    if (old_col->IsUsed() || j == keep_attached_index) {
      const auto new_col = new_columns.Create(old_col->var, old_col->type, this,
                                              old_col->id, new_columns.Size());
      old_col->ReplaceAllUsesWith(new_col);
      new_attached_columns.AddUse(
          j == keep_attached_index
              ? attached_columns[j]
              : attached_columns[j]->TryResolveToConstant());
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

// Equality over maps is pointer-based.
bool QueryMapImpl::Equals(EqualitySet &eq, QueryViewImpl *that_) noexcept {
  if (eq.Contains(this, that_)) {
    return true;
  }

  const auto that = that_->AsMap();
  if (!that || is_positive != that->is_positive ||
      num_free_params != that->num_free_params ||
      columns.Size() != that->columns.Size() ||
      attached_columns.Size() != that->attached_columns.Size() ||
      functor.Id() != that->functor.Id() ||
      (ParsedDeclaration(functor).BindingPattern() !=
       ParsedDeclaration(that->functor).BindingPattern()) ||
      positive_conditions != that->positive_conditions ||
      negative_conditions != that->negative_conditions ||
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

}  // namespace hyde
