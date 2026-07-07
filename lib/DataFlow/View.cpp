// Copyright 2020, Trail of Bits. All rights reserved.

#include <drlojekyll/DataFlow/Query.h>
#include <drlojekyll/Display/Format.h>
#include <drlojekyll/Parse/Format.h>
#include <drlojekyll/Util/DefUse.h>

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "EquivalenceSet.h"
#include "Optimize.h"
#include "Query.h"

namespace hyde {

QueryViewImpl::~QueryViewImpl(void) {}

QueryViewImpl::QueryViewImpl(void)
    : Def<QueryViewImpl>(this),
      User(this),
      columns(this),
      input_columns(this),
      attached_columns(this),
      predecessors(this),
      successors(this) {
  assert(reinterpret_cast<uintptr_t>(static_cast<User *>(this)) ==
         reinterpret_cast<uintptr_t>(this));
}

QuerySelectImpl *QueryViewImpl::AsSelect(void) noexcept {
  return nullptr;
}

QueryTupleImpl *QueryViewImpl::AsTuple(void) noexcept {
  return nullptr;
}

QueryKVIndexImpl *QueryViewImpl::AsKVIndex(void) noexcept {
  return nullptr;
}

QueryJoinImpl *QueryViewImpl::AsJoin(void) noexcept {
  return nullptr;
}

QueryMapImpl *QueryViewImpl::AsMap(void) noexcept {
  return nullptr;
}

QueryAggregateImpl *QueryViewImpl::AsAggregate(void) noexcept {
  return nullptr;
}

QueryMergeImpl *QueryViewImpl::AsMerge(void) noexcept {
  return nullptr;
}

QueryNegateImpl *QueryViewImpl::AsNegate(void) noexcept {
  return nullptr;
}

QueryCompareImpl *QueryViewImpl::AsCompare(void) noexcept {
  return nullptr;
}

QueryInsertImpl *QueryViewImpl::AsInsert(void) noexcept {
  return nullptr;
}

// Useful for communicating low-level debug info back to the formatter.
OutputStream &QueryViewImpl::DebugString(OutputStream &ss) noexcept {
  if (!group_ids.empty()) {
    auto sep = "group-ids(";
    for (auto group_id : group_ids) {
      ss << sep << group_id;
      sep = ", ";
    }
    ss << ") ";
  }

  ss << "depth=" << Depth();
  if (is_dead) {
    ss << " dead=1";
  }
  if (is_locked) {
    ss << " locked=1";
  }
  ss << " hash=" << std::hex << this->Hash() << std::dec;
  switch (valid) {
    case kValid: break;
    case kInvalidBeforeCanonicalize:
      ss << "<B><FONT COLOR=\"RED\">BEFORE";
      if (invalid_var) {
        ss << ' ' << invalid_var->SpellingRange();
      }
      ss << "</FONT></B>";
      break;
    case kInvalidAfterCanonicalize:
      ss << "<B><FONT COLOR=\"RED\">AFTER";
      if (invalid_var) {
        ss << ' ' << invalid_var->SpellingRange();
      }
      ss << "</FONT></B>";
      break;
  }
#ifndef NDEBUG
  if (!producer.empty()) {
    ss << ' ' << producer;
  }
#endif
  return ss;
}

// Return a number that can be used to help sort this node. The idea here
// is that we often want to try to merge together two different instances
// of the same underlying node when we can.
uint64_t QueryViewImpl::Sort(void) noexcept {
  return Hash();
}

// Is this view directly being used? This does not check columns, but does
// check view-level uses (e.g. by MERGEs).
bool QueryViewImpl::IsUsedDirectly(void) const noexcept {
  if (this->Def<QueryViewImpl>::IsUsed()) {
    if (is_dead) {
#ifndef NDEBUG
      ForEachUse<QueryViewImpl>([](QueryViewImpl *user_view, QueryViewImpl *) {
        assert(user_view->is_dead || user_view->AsMerge());
      });
#endif
      return false;

    } else {
      return true;
    }
  }

  return false;
}

// Returns `true` if this view is being used. This is defined in terms of
// whether or not the view is used in a merge, or whether or not any of its
// columns are used.
bool QueryViewImpl::IsUsed(void) const noexcept {
  if (IsUsedDirectly()) {
    return true;
  }

  for (auto col : columns) {
    if (col->Def<QueryColumnImpl>::IsUsed()) {
      if (is_dead) {
#ifndef NDEBUG
        col->ForEachUse<QueryViewImpl>(
            [](QueryViewImpl *user_view, COL *) { assert(user_view->is_dead); });
#endif
        continue;
      }
      return true;
    }
  }

  return false;
}

// Invoked any time time that any of the columns used by this view are
// modified.
void QueryViewImpl::Update(uint64_t next_timestamp) {
  if (is_canonical) {
    is_canonical = false;
    for (auto col : columns) {
      col->ForEachUse<QueryViewImpl>(
          [=](QueryViewImpl *user, COL *) { user->is_canonical = false; });
    }
  }

  //  if (timestamp >= next_timestamp) {
  //    return;
  //  }
  //
  //  timestamp = next_timestamp;
  //  hash = 0;
  //  depth = 0;
  //

  //
  //  // Update merges.
  //  ForEachUse<QueryViewImpl>([=] (QueryViewImpl *user, QueryViewImpl *) {
  //    user->Update(next_timestamp);
  //  });
}

// Record the mapping between `in_col` and `out_col` into `this->in_to_out`,
// do constant propagation, and de-duplicate repeated input columns. This is
// the per-column workhorse invoked by node-specific `Canonicalize`
// implementations as they walk their (input, output) column pairs in order.
// It performs two rewrites directly -- marking an output column as a
// constant reference when its input is constant, and redirecting all uses
// of an output column whose input repeats onto the output of the first
// occurrence (column-level CSE) -- and accumulates discovery bits in `has`
// that tell the caller which structural rewrites (dropping unused columns,
// guarding constant outputs behind a tuple) are worth performing next.
// Sets `is_canonical = false` if anything is changed or should be changed.
//
//    (prev, added) = in_to_out.emplace(in_col, out_col)
//    if in_col is constant/const-ref and out_col is not a const-ref:
//      out_col.CopyConstantFrom(in_col)          // constant propagation
//      has.non_local_changes = true
//    if out_col is used ignoring MERGEs: has.directly_used_column = true
//    if out_col is unused:
//      has.unused_column = true
//      if is_attached: is_canonical = false      // droppable pass-through
//      if out_col is a const-ref: has.guardable_constant_output = true
//    if not added:                               // in_col seen before
//      replace all uses of out_col with prev's out_col
//      has.duplicated_input_column = true
//
//  Constant propagation:            Duplicate input de-duplication:
//
//    V[a=7]                           V[a]
//     |                                |  \
//    MAP(in: a -> out: x)             CMP(in: a, a -> out: x, y)
//     |                                |         \
//    users(x)                         users(x)   users(y)
//        ==>                              ==>
//    V[a=7]                           V[a]
//     |                                |  \
//    MAP(in: a -> out: x=7)           CMP(in: a, a -> out: x, y)
//     |                                |
//    users(x) see constant 7          users(x + former users of y);
//                                     y unused -> removal candidate
QueryViewImpl::Discoveries
QueryViewImpl::CanonicalizeColumn(
    const OptimizationContext &, QueryColumnImpl *in_col,
    QueryColumnImpl *out_col, bool is_attached,
    QueryViewImpl::Discoveries has) {
  auto [it, added] = in_to_out.emplace(in_col, out_col);

  const auto in_col_is_constant = in_col->IsConstantOrConstantRef();
  auto out_col_is_constant_ref = out_col->IsConstantRef();
  if (in_col_is_constant && !out_col_is_constant_ref) {

    // Mark it as a constant.
    is_canonical = false;
    has.non_local_changes = true;
    out_col->CopyConstantFrom(in_col);
    out_col_is_constant_ref = true;
  }

  const auto is_directly_used = out_col->IsUsedIgnoreMerges();
  if (is_directly_used) {
    has.directly_used_column = true;
  }

  if (!out_col->IsUsed()) {
    has.unused_column = true;

    if (is_attached) {
      is_canonical = false;
    }
    if (out_col_is_constant_ref) {
      has.guardable_constant_output = true;
    }
  }

  if (!added) {
    if (is_directly_used) {
      out_col->ReplaceAllUsesWith(it->second);
      has.non_local_changes = true;
      is_canonical = false;
    }

    has.duplicated_input_column = true;
  }

  return has;
}

// Canonicalizes an input/output column pair. Returns `true` in the first
// element if non-local changes are made, and `true` in the second element
// if the column pair can be removed. Constant-ness flows from the input to
// the output: an output fed by a constant (or by a column proven to hold a
// constant) becomes a constant reference, so every downstream user of the
// output observes the constant without evaluating this view. When the input
// is itself a constant reference and the output already carries the
// constant, a non-local change is reported if `opt` permits replacing
// inputs with constants, signalling the caller to substitute the input use
// with the constant itself and thereby sever a dataflow edge. A pair whose
// output is unused is flagged as removable when `opt` allows unused-column
// removal.
//
//    non_local = false
//    if in_col is a literal constant and out_col is not a const-ref:
//      out_col.CopyConstantFrom(in_col); non_local = true
//    elif in_col is a const-ref:
//      if out_col is not a const-ref:
//        out_col.CopyConstantFrom(in_col); non_local = true
//      elif opt.can_replace_inputs_with_constants:
//        non_local = true                 // caller swaps in the constant
//    removable = opt.can_remove_unused_columns and out_col unused
//    return {non_local, removable}
//
//    V[a=7]                            V[a=7]
//     |                       ==>       |
//    TUPLE(in: a -> out: x)            TUPLE(in: a -> out: x=7)
//     |                                 |
//    users(x)                          users(x) see constant 7
std::pair<bool, bool> QueryViewImpl::CanonicalizeColumnPair(
    QueryColumnImpl *in_col, QueryColumnImpl *out_col,
    const OptimizationContext &opt) noexcept {

  const auto out_col_is_constref = out_col->IsConstantRef();

  //  const auto out_col_is_directly_used = out_col->IsUsedIgnoreMerges();
  auto non_local_changes = false;

  if (in_col->IsConstant()) {
    if (!out_col_is_constref) {
      non_local_changes = true;
      out_col->CopyConstantFrom(in_col);
    }
  } else if (in_col->IsConstantRef()) {
    if (!out_col_is_constref) {
      non_local_changes = true;
      out_col->CopyConstantFrom(in_col);

    } else if (opt.can_replace_inputs_with_constants) {
      non_local_changes = true;
    }
  }

  auto can_remove = false;
  if (opt.can_remove_unused_columns && !out_col->IsUsed()) {
    can_remove = true;
  }

  return {non_local_changes, can_remove};
}

// Put this view into a canonical form.
bool QueryViewImpl::Canonicalize(QueryImpl *, const OptimizationContext &,
                                 const ErrorLog &) {
  is_canonical = true;
  return false;
}

unsigned QueryViewImpl::Depth(void) noexcept {
  if (depth) {
    return depth;
  }

  auto estimate = EstimateDepth(input_columns, 1u);
  estimate = EstimateDepth(attached_columns, depth);
  depth = estimate + 1u;

  auto real = GetDepth(input_columns, 1u);
  real = GetDepth(attached_columns, real);
  depth = real + 1u;

  return depth;
}

unsigned QueryViewImpl::EstimateDepth(const UseList<QueryColumnImpl> &cols,
                                      unsigned depth) {
  for (const auto input_col : cols) {
    const auto input_depth = input_col->view->depth;
    if (input_depth >= depth) {
      depth = input_depth;
    }
  }
  return depth;
}

unsigned QueryViewImpl::GetDepth(
    const UseList<QueryColumnImpl> &cols, unsigned depth) {
  for (const auto input_col : cols) {
    const auto input_depth = input_col->view->Depth();
    if (input_depth >= depth) {
      depth = input_depth;
    }
  }
  return depth;
}

// Return the number of uses of this view.
unsigned QueryViewImpl::NumUses(void) const noexcept {
  std::vector<QueryViewImpl *> users;
  users.reserve(columns.Size() * 2);

  for (auto col : columns) {
    col->ForEachUser([&users](QueryViewImpl *user) { users.push_back(user); });
  }

  std::sort(users.begin(), users.end());
  auto it = std::unique(users.begin(), users.end());
  users.erase(it, users.end());
  return static_cast<unsigned>(users.size());
}

static const std::hash<const char *> kCStrHasher;

// Initializer for an updated hash value.
uint64_t QueryViewImpl::HashInit(void) const noexcept {
  uint64_t init_hash = kCStrHasher(this->KindName());
  init_hash <<= 1u;
  init_hash |= can_receive_deletions;
  init_hash <<= 1u;
  init_hash |= can_produce_deletions;

  init_hash ^= RotateRight64(init_hash, 33) * (columns.Size() + 7u);

  return init_hash;
}

// Upward facing hash. The idea here is that we sometimes have multiple nodes
// that have the same hash, and thus are candidates for CSE, and we want to
// decide: among those candidates, which nodes /should/ be merged. We decide
// this by looking up the dataflow graph (to some limited depth) and creating
// a rough hash of how this node gets used.
uint64_t QueryViewImpl::UpHash(unsigned depth) const noexcept {

  auto up_hash = HashInit();

  if (!depth) {
    return up_hash;
  }

  unsigned i = 0u;
  for (auto col : columns) {
    col->ForEachUse<QueryViewImpl>([=, &up_hash](QueryViewImpl *user, COL *) {
      up_hash ^=
          RotateRight64(up_hash, (i + 7u) % 64u) * user->UpHash(depth - 1u);
    });
    ++i;
  }

  return up_hash;
}

// Prepare to delete this node. This tries to drop all dependencies and
// unlink this node from the dataflow graph. It returns `true` if successful
// and `false` if it has already been performed.
bool QueryViewImpl::PrepareToDelete(void) {
  if (is_dead) {
    return false;
  }

  hash = 0;
  is_canonical = true;
  is_dead = true;

  input_columns.Clear();
  attached_columns.Clear();

  const auto is_this_view = [this](QueryViewImpl *v) { return v == this; };

  if (auto merge = AsMerge(); merge) {
    merge->merged_views.Clear();

  } else if (auto agg = AsAggregate(); agg) {
    agg->group_by_columns.Clear();
    agg->config_columns.Clear();
    agg->aggregated_columns.Clear();

  } else if (auto join = AsJoin(); join) {
    join->out_to_in.clear();
    join->joined_views.Clear();
    join->num_pivots = 0u;

  } else if (auto select = AsSelect(); select) {
    if (auto stream = select->stream.get(); stream) {
      select->stream.Clear();
      if (auto io = stream->AsIO(); io) {
        io->receives.RemoveIf(is_this_view);
      } else {
        assert(stream->AsConstant());
      }

    } else if (auto rel = select->relation.get(); rel) {
      select->relation.Clear();
      rel->selects.RemoveIf(is_this_view);
    }

  } else if (auto insert = AsInsert(); insert) {
    if (auto stream = insert->stream.get(); stream) {
      insert->stream.Clear();
      if (auto io = stream->AsIO(); io) {
        io->transmits.RemoveIf(is_this_view);
      } else {
        assert(false);
      }

    } else if (auto rel = insert->relation.get(); rel) {
      insert->relation.Clear();
      rel->inserts.RemoveIf(is_this_view);
    }

  } else if (auto negate = AsNegate(); negate) {
    negate->negated_view.Clear();
  }

  // Data can never flow out of a deleted view. A non-MERGE view reading this
  // view's columns can therefore never receive data, and dies with it. A
  // MERGE merely loses this view as one of its sources, and dies only when
  // this view was the last one. View-level users other than MERGEs (a NEGATE
  // holding this as its negated view, a SELECT holding an INSERT into its
  // relation) do not consume this view's data and are left alone.
  std::vector<QueryViewImpl *> column_users;
  for (QueryColumnImpl *col : columns) {
    col->ForEachUse<QueryViewImpl>([&](QueryViewImpl *user, COL *) {
      column_users.push_back(user);
    });
  }

  std::vector<QueryMergeImpl *> merge_users;
  ForEachUse<QueryViewImpl>([&](QueryViewImpl *user, QueryViewImpl *) {
    if (auto merge = user->AsMerge()) {
      merge_users.push_back(merge);
    }
  });

  for (QueryMergeImpl *merge : merge_users) {
    if (merge == this || merge->is_dead) {
      continue;
    }
    merge->merged_views.RemoveIf(is_this_view);
    merge->is_canonical = false;
    if (merge->merged_views.Empty()) {
      merge->PrepareToDelete();
    }
  }

  for (QueryViewImpl *user : column_users) {
    if (user != this && !user->is_dead && !user->AsMerge()) {
      user->PrepareToDelete();
    }
  }

  return true;
}

// Copy the group IDs and the receive/produce deletions from `this` to `that`.
void QueryViewImpl::CopyDifferentialAndGroupIdsTo(QueryViewImpl *that) {

  // Maintain the set of group IDs, to prevent over-merging.
  that->group_ids.insert(that->group_ids.end(), group_ids.begin(),
                         group_ids.end());
  std::sort(that->group_ids.begin(), that->group_ids.end());

  if (can_receive_deletions) {
    that->can_receive_deletions = true;
  }

  if (can_produce_deletions) {
    that->can_produce_deletions = true;
  }
}

// Replace all uses of `this` with `that`. The semantic here is that `this`
// remains valid and used.
void QueryViewImpl::SubstituteAllUsesWith(QueryViewImpl *that) {
  if (is_used_by_negation) {
    that->is_used_by_negation = true;
    is_used_by_negation = false;
  }

  unsigned i = 0u;
  for (auto col : columns) {
    col->ReplaceAllUsesWith(that->columns[i++]);
  }

  this->Def<QueryViewImpl>::ReplaceAllUsesWith(that);

  CopyDifferentialAndGroupIdsTo(that);

  if (color && that->color) {
    if (color != that->color) {
      that->color ^= RotateRight32(color, (color % 13) + 1u);
    }

  } else if (!that->color) {
    that->color = color;
  }
}

// Replace all uses of `this` with `that`. The semantic here is that `this`
// is completely subsumed/replaced by `that`.
void QueryViewImpl::ReplaceAllUsesWith(QueryViewImpl *that) {
  SubstituteAllUsesWith(that);
  PrepareToDelete();
}

// Does this view introduce a control dependency? If a node introduces a
// control dependency then it generally needs to be kept around.
bool QueryViewImpl::IntroducesControlDependency(void) const noexcept {
//  if (this->AsMap()) {
//    return true;
//  }

  // TODO(pag): Think about whether or not 1:1 MAPs are control dependencies.

  std::unordered_map<QueryViewImpl *, bool> is_conditional;
  return QueryViewImpl::IsConditional(const_cast<QueryViewImpl *>(this),
                                      is_conditional);
}

// Returns `true` if all output columns are used.
bool QueryViewImpl::AllColumnsAreUsed(void) const noexcept {
  if (IsUsedDirectly()) {
    return true;  // Used in a MERGE.
  }

  for (auto col : columns) {
    if (!col->IsUsedIgnoreMerges()) {
      return false;
    }
  }

  return true;
}
// Insert a pass-through TUPLE between `this` and its users, and return that
// tuple, or `nullptr` when no guard is needed. A view that is used directly
// -- as an operand of a UNION (MERGE) -- exposes its exact column arity and
// ordering to those users, so its own canonicalization must not re-order,
// de-duplicate, or drop columns. The guard tuple absorbs that external
// interface: every direct use (including group IDs and differential flags)
// moves onto the tuple, which forwards each column of `this` in order,
// leaving `this` free to restructure its columns beneath a stable facade.
// With `force`, a guard is inserted even when `this` is not directly used.
//
//    if not force and not IsUsedDirectly(): return nullptr
//    tuple = new TUPLE with one output per column of this, constants copied
//    SubstituteAllUsesWith(tuple)   // users, group ids move over
//    tuple.input_columns = this->columns
//    return tuple
//
//   before:                          after:
//
//    JOIN(out: A, B, C)               JOIN(out: A, B, C)  <- may now
//      |                                |               reorder/drop cols
//    UNION                            TUPLE(in: A,B,C -> out: A,B,C)
//                                       |
//                                     UNION
QueryTupleImpl *QueryViewImpl::GuardWithTuple(QueryImpl *query,
                                              bool force) {

  if (!force && !IsUsedDirectly()) {
    return nullptr;
  }

  const auto tuple = query->tuples.Create();
  tuple->color = color;

  assert(!AsInsert());  // INSERTs don't have output columns.

  auto col_index = 0u;
  for (auto col : columns) {
    auto out_col = tuple->columns.Create(
        col->var, col->type, tuple, col->id, col_index++);
    out_col->CopyConstantFrom(col);
  }

  // Make any merges use the tuple.
  SubstituteAllUsesWith(tuple);

  for (QueryColumnImpl *col : columns) {
    tuple->input_columns.AddUse(col);
  }

#ifndef NDEBUG
  std::stringstream ss;
  ss << "GUARD(" << KindName();
  if (!producer.empty()) {
    ss << ": " << producer;
  }
  ss << ')';
  tuple->producer = ss.str();
#endif

  return tuple;
}

// Insert a guard TUPLE between `this` and its users that forwards each
// output column from its best available source: constant outputs feed the
// tuple from the constant itself, and non-constant outputs route through
// `in_to_out`, so repeated inputs collapse onto a single column (covering
// both `input_columns` and the attached columns starting at
// `first_attached_col`). All uses of `this` move onto the tuple. The tuple
// preserves the externally visible column shape while letting downstream
// users consume constants and de-duplicated columns directly. The
// keep-last-edge rule applies: the tuple must keep at least one input
// column produced by `this`, because that column edge is what expresses
// the tuple's presence dependency on `this` holding data. When every
// output would otherwise feed from a constant, one representative
// position reads its (constant-valued) source column of `this` instead of
// the constant; the tuple's corresponding output column still carries the
// constant, so downstream constant propagation is unaffected.
//
// NOTE(pag): This assumes `in_to_out` is filled up, and operates on
//            `input_columns` and `attached_columns` to find the best version
//            of a column from `in_to_out`. MAP output columns are forwarded
//            positionally rather than through `in_to_out`: `bound`- and
//            `free`-attributed functor parameters interleave, so there is
//            no alignment between a MAP's input and output columns.
//
//    tuple = new TUPLE mirroring this->columns
//    SubstituteAllUsesWith(tuple)
//    keep_rep = every column of this is a constant or constant ref
//    for i, col in this->columns:
//      if col is a constant ref and not (keep_rep and i == 0):
//                                  tuple.inputs += the constant
//      elif this is a MAP:         tuple.inputs += col
//      elif i < first_attached:    tuple.inputs += in_to_out[input[i]]
//      else:                       tuple.inputs += in_to_out[attached[...]]
//
//   before:                          after:
//
//    V                                V
//    |                                |
//    CMP(out: X, Y=7, X)              CMP(out: X, Y, Z)
//      |         \                     |
//    UNION      users                 TUPLE(in: X, 7, X ->
//                                           out: X', Y'=7, Z')
//                                       |         \
//                                     UNION      users
//
//   all-constant case (position 0 keeps the edge to `this`):
//
//    V(out: X=1, Y=2)                 V(out: X=1, Y=2)
//      |                               |X       CONST 2
//    users                            TUPLE(in: X, 2 -> out: X'=1, Y'=2)
//                                       |
//                                     users
QueryTupleImpl *
QueryViewImpl::GuardWithOptimizedTuple(QueryImpl *query,
                                       unsigned first_attached_col) {

  QueryTupleImpl *tuple = query->tuples.Create();
  tuple->color = color;

#ifndef NDEBUG
  std::stringstream ss;
  ss << "OPT-GUARD(" << KindName();
  if (!producer.empty()) {
    ss << ": " << producer;
  }
  ss << ')';
  tuple->producer = ss.str();
#endif

  const auto num_cols = columns.Size();
  for (auto i = 0u; i < num_cols; ++i) {
    const auto col = columns[i];
    const auto new_col =
        tuple->columns.Create(col->var, col->type, tuple, col->id, i);
    new_col->CopyConstantFrom(col);
  }

  SubstituteAllUsesWith(tuple);
  const auto is_map = !!this->AsMap();

  // The keep-last-edge rule: if every column of `this` resolved to a
  // constant, the guard tuple would read nothing from `this` and its
  // presence dependency on `this` would vanish, so one representative
  // position (the first) reads its source column of `this` instead of the
  // constant.
  auto keep_representative = true;
  for (auto i = 0u; i < num_cols; ++i) {
    if (!columns[i]->AsConstant()) {
      keep_representative = false;
      break;
    }
  }

  for (auto i = 0u; i < num_cols; ++i) {
    QueryColumnImpl *const col = columns[i];
    if (auto const_col = col->AsConstant();
        const_col && !(keep_representative && !i)) {
      tuple->input_columns.AddUse(const_col);


    // If it's not an attached column then map through
    } else if (i < first_attached_col) {

      // Maps follow non-traditional rules for input-to-output mappings for
      // columns; there isn't alignment (or even shifted alignment) between
      // input and output columns because `bound`- and `free`-attributed
      // parameters can be intermixed, and the output columns follow the same
      // order as the functor parameters.
      if (is_map) {
        tuple->input_columns.AddUse(col);

      } else {
        tuple->input_columns.AddUse(in_to_out[input_columns[i]]);
      }

    // Drop duplicates if we have them.
    } else {
      tuple->input_columns.AddUse(
          in_to_out[attached_columns[i - first_attached_col]]);
    }
  }

  assert(QueryViewImpl::GetIncomingView(tuple->input_columns) == this);

  return tuple;
}

// Proxy this node with a comparison of `lhs_col` and `rhs_col`, where
// `lhs_col` and `rhs_col` either belong to `this->columns` or are constants.
// This materializes a constraint between two columns as an explicit CMP
// node fed by `this`, then restores the original column interface with a
// pass-through TUPLE, which is returned so that the caller can redirect
// existing users of `this` onto it. For `kEqual` the CMP fuses both inputs
// into a single output column -- downstream, the two columns are provably
// one value -- and the trailing TUPLE re-emits that fused column in both
// original positions. All other columns of `this` ride along as attached
// columns of the CMP. For symmetric operators (equal/not-equal) with only
// the right operand constant, the operands swap so the constant sits on
// the left. `in_to_out` is rebuilt to map each original column of `this`
// to its corresponding CMP output.
//
//    if op in {=, !=} and rhs is constant and lhs is not: swap operands
//    cmp = CMP(op; in: lhs, rhs; attached: remaining columns of this)
//      kEqual: one fused output for lhs and rhs
//      else:   separate outputs for lhs and rhs
//    in_to_out = original column -> cmp output column
//    tuple = TUPLE re-emitting this->columns' order from cmp outputs
//    return tuple
//
//   before:                          after (op is A = B):
//
//    VIEW(out: A, B, C)               VIEW(out: A, B, C)
//      |                                |
//    users                             CMP A=B (in: A, B; attached: C
//                                                -> out: AB, C)
//                                        |
//                                      TUPLE(out: AB, AB, C)
//                                        |
//                                      users (moved here by caller)
QueryTupleImpl *
QueryViewImpl::ProxyWithComparison(QueryImpl *query, ComparisonOperator op,
                                   QueryColumnImpl *lhs_col,
                                   QueryColumnImpl *rhs_col) {

  // Prefer to have the constant first.
  if ((ComparisonOperator::kEqual == op ||
       ComparisonOperator::kNotEqual == op) &&
      rhs_col->IsConstant() && !lhs_col->IsConstant()) {
    return ProxyWithComparison(query, op, rhs_col, lhs_col);
  }

  // Now fill in the tuple to use a CMP that takes its input from `this`.

  in_to_out.clear();

  auto col_index = 0u;
  QueryCompareImpl *cmp = query->compares.Create(op);
  cmp->color = color;

  cmp->input_columns.AddUse(lhs_col);
  auto lhs_out_col = cmp->columns.Create(lhs_col->var, lhs_col->type, cmp,
                                         lhs_col->id, col_index++);

  lhs_out_col->CopyConstantFrom(lhs_col);
  in_to_out.emplace(lhs_col, lhs_out_col);

  cmp->input_columns.AddUse(rhs_col);
  if (ComparisonOperator::kEqual == op) {
    lhs_out_col->CopyConstantFrom(rhs_col);
    in_to_out.emplace(rhs_col, lhs_out_col);

  } else {
    auto rhs_out_col = cmp->columns.Create(rhs_col->var, rhs_col->type, cmp,
                                           rhs_col->id, col_index++);
    rhs_out_col->CopyConstantFrom(rhs_col);
    in_to_out.emplace(rhs_col, rhs_out_col);
  }

  assert(cmp->input_columns.Size() == 2);

  // Add in the other columns.
  for (QueryColumnImpl *col : columns) {
    if (col != lhs_col && col != rhs_col) {
      cmp->attached_columns.AddUse(col);
      const auto attached_col =
          cmp->columns.Create(col->var, col->type, cmp, col->id, col_index++);
      attached_col->CopyConstantFrom(col);
      in_to_out.emplace(col, attached_col);
    }
  }

  // Create a tuple that re-orders the output of the CMP to preserve it.
  QueryTupleImpl *tuple = query->tuples.Create();
  tuple->color = color;

  col_index = 0u;
  for (auto orig_col : columns) {
    QueryColumnImpl *const in_col = in_to_out[orig_col];
    auto out_col = tuple->columns.Create(orig_col->var, orig_col->type, tuple,
                                         orig_col->id, col_index++);
    tuple->input_columns.AddUse(in_col);
    out_col->CopyConstantFrom(in_col);
  }

#ifndef NDEBUG
  std::stringstream ss;
  ss << "PROXY-CMP(" << KindName();
  if (!producer.empty()) {
    ss << ": " << producer;
  }
  ss << ')';
  cmp->producer = ss.str();
#endif

  return tuple;
}

// Utility for comparing use lists.
bool QueryViewImpl::ColumnsEq(EqualitySet &eq,
                              const UseList<QueryColumnImpl> &c1s,
                              const UseList<QueryColumnImpl> &c2s) {

  const auto num_cols = c1s.Size();
  if (num_cols != c2s.Size()) {
    return false;
  }
  for (auto i = 0u; i < num_cols; ++i) {
    auto a = c1s[i];
    auto b = c2s[i];
    if (a == b) {
      continue;

    } else if (a->view == b->view) {
      return false;

    } else if (a->type.Kind() != b->type.Kind() ||
               !a->view->Equals(eq, b->view) || a->Index() != b->Index()) {
      return false;
    }
  }
  return true;
}

// If `cols1:cols2` pull their data from a forwarding tuple, then update
// `cols1:cols2` to point at the source of the data of those tuples. This
// retargets the input use lists of `this` through arbitrarily long chains
// of forwarding TUPLEs (and, via `PullDataFromBeyondTrivialUnions`,
// single-source UNIONs) so that `this` reads directly from the ultimate
// producer. Skipping a forwarding view is sound because a TUPLE only
// forwards values and cannot filter data. Constant columns in the lists
// pass through unchanged. Shortening the dependency chains reduces dataflow
// depth, and bypassed tuples that lose their last user become dead and are
// removed later.
//
// The keep-last-edge rule bounds the chase: when every retargeted column
// resolves to a constant while the hopped-over tuple itself still reads
// from a predecessor, the hop would sever the last column edge expressing
// that the rows' presence depends on that predecessor holding data, so the
// chase stops at the tuple instead.
//
// Takes in the `incoming_view` pulled from by `cols1:cols2` and returns the
// updated `incoming_view`. Recursion stops when the incoming view stops
// changing, which is the fixpoint reached by self-recursive dataflows.
//
// NOTE(pag): This updates `is_canonical = false` if it changes anything.
//
//    if incoming_view is null, or is `this`: return as-is
//    if incoming_view is not a TUPLE:
//      return PullDataFromBeyondTrivialUnions(incoming_view, cols1, cols2)
//    for col in cols1 and cols2:
//      if col.view == tuple: col = tuple.input_columns[col.Index()]
//      else:                 keep col (a constant)
//    if new cols are all constants and tuple has an incoming view:
//      return tuple                        // keep-last-edge: do not sever
//    recurse on GetIncomingView(cols1, cols2)
//
//   before:                          after:
//
//    SELECT(out: s0, s1)              SELECT(out: s0, s1)
//      |                                |
//    TUPLE(out: t0, t1)  <- trivial     |   TUPLE (unused -> dead)
//      |                                |
//    this(in: t0, c, t1)              this(in: s0, c, s1)
QueryViewImpl *QueryViewImpl::PullDataFromBeyondTrivialTuples(
    QueryViewImpl *incoming_view, UseList<QueryColumnImpl> &cols1,
    UseList<QueryColumnImpl> &cols2) {
  std::vector<QueryViewImpl *> visited;
  return PullDataFromBeyondTrivialTuplesImpl(visited, incoming_view, cols1,
                                             cols2);
}

QueryViewImpl *QueryViewImpl::PullDataFromBeyondTrivialTuplesImpl(
    std::vector<QueryViewImpl *> &visited, QueryViewImpl *incoming_view,
    UseList<QueryColumnImpl> &cols1, UseList<QueryColumnImpl> &cols2) {

  if (!incoming_view || this == incoming_view) {
    return incoming_view;
  }

  // The chase has come back around a forwarding cycle (one with no external
  // data source, e.g. the dataflow of mutually forwarding clauses); the
  // incoming view repeating is the cycle analogue of the incoming view no
  // longer changing, so stop here and let dead-flow elimination collect the
  // source-less cycle.
  if (std::find(visited.begin(), visited.end(), incoming_view) !=
      visited.end()) {
    return incoming_view;
  }
  visited.push_back(incoming_view);

  const auto tuple = incoming_view->AsTuple();
  if (!tuple) {
    return PullDataFromBeyondTrivialUnions(visited, incoming_view, cols1,
                                           cols2);
  }

  // Pre-compute the view the hop would leave `this` reading from: the view
  // of the first retargeted column that is not a constant. This runs before
  // any use list is materialized so that the stop cases below cost nothing.
  QueryViewImpl *hopped_view = nullptr;
  const UseList<QueryColumnImpl> *col_lists[] = {&cols1, &cols2};
  for (const auto *col_list : col_lists) {
    for (QueryColumnImpl *col : *col_list) {
      QueryColumnImpl *new_col = col;
      if (col->view == tuple) {
        new_col = tuple->input_columns[col->Index()];
      } else {
        assert(col->IsConstant());
      }
      if (!new_col->IsConstant()) {
        hopped_view = new_col->view;
        break;
      }
    }
    if (hopped_view) {
      break;
    }
  }

  // If this hop would leave `this` reading its own output columns then the
  // forwarding chain is a cycle with no external data source (e.g. the
  // dataflow of `p(A) : p(A).`). A view must never be its own user, so stop
  // the chase one hop early and keep reading through the last forwarding
  // tuple; dead-flow elimination is responsible for collecting the
  // source-less cycle.
  if (hopped_view == this) {
    return incoming_view;
  }

  // The keep-last-edge rule: if every retargeted column is a constant while
  // the tuple itself reads from a predecessor, then the hop would sever the
  // last column edge expressing that the rows' presence depends on that
  // predecessor holding data (e.g. the tuple's only non-constant input is a
  // retained representative witness that `this` does not read). Stop the
  // chase at the tuple.
  if (!hopped_view && GetIncomingView(tuple->input_columns)) {
    return incoming_view;
  }

  UseList<QueryColumnImpl> new_cols1(this);
  UseList<QueryColumnImpl> new_cols2(this);

  for (auto col : cols1) {
    if (col->view == tuple) {
      new_cols1.AddUse(tuple->input_columns[col->Index()]);
    } else {
      new_cols1.AddUse(col);
    }
  }

  for (auto col : cols2) {
    if (col->view == tuple) {
      new_cols2.AddUse(tuple->input_columns[col->Index()]);
    } else {
      new_cols2.AddUse(col);
    }
  }

  is_canonical = false;

  cols1.Swap(new_cols1);
  cols2.Swap(new_cols2);

  // Recursion stops at the fixpoint of a self-recursive dataflow.
  const auto next_incoming_view = GetIncomingView(cols1, cols2);
  if (next_incoming_view == incoming_view) {
    return next_incoming_view;
  }

  return PullDataFromBeyondTrivialTuplesImpl(visited, next_incoming_view,
                                             cols1, cols2);
}

// Companion to `PullDataFromBeyondTrivialTuples` for UNIONs (MERGEs): when
// `maybe_merge` is a trivially conditional UNION whose merged views reduce
// to exactly one distinct real data source, retarget `cols1:cols2` to read
// that source's columns directly, bypassing the UNION. Merged entries that
// are `this` (a direct recursion back through the reader), the merge
// itself, or repeats of the already-chosen source do not count as extra
// sources; if a second distinct source exists, the UNION genuinely merges
// data and cannot be skipped. After the rewrite, tuple-skipping resumes
// from the discovered source.
//
//    if maybe_merge is not a UNION: return as-is
//    source = unique view in merged_views excluding {this, merge, dups}
//    if no source, or more than one: return as-is
//    for col in cols1 and cols2:
//      if col.view == merge: col = source.columns[col.Index()]
//      else:                 keep col (a constant)
//    is_canonical = false
//    return PullDataFromBeyondTrivialTuples(source, cols1, cols2)
//
//   before:                          after:
//
//    V(out: v0, v1)                   V(out: v0, v1)
//      |                                |
//    UNION(V, this)(out: u0, u1)        |   UNION (may become dead)
//      |                                |
//    this(in: u0, u1)                 this(in: v0, v1)
QueryViewImpl *QueryViewImpl::PullDataFromBeyondTrivialUnions(
    std::vector<QueryViewImpl *> &visited, QueryViewImpl *maybe_merge,
    UseList<QueryColumnImpl> &cols1, UseList<QueryColumnImpl> &cols2) {

  QueryMergeImpl *const merge = maybe_merge->AsMerge();
  if (!merge) {
    return maybe_merge;
  }

  QueryViewImpl *incoming_view = nullptr;
  for (QueryViewImpl *merged_view : merge->merged_views) {

    if (merged_view == this || merged_view == merge ||
        merged_view == incoming_view) {
      continue;

    // This is the second non-trivial data source to the merge, thus it's not
    // a trivial union.
    } else if (incoming_view) {
      return maybe_merge;

    } else {
      incoming_view = merged_view;
    }
  }

  if (!incoming_view) {
    return maybe_merge;
  }

  UseList<QueryColumnImpl> new_cols1(this);
  UseList<QueryColumnImpl> new_cols2(this);

  for (QueryColumnImpl *col : cols1) {
    if (col->view == merge) {
      new_cols1.AddUse(incoming_view->columns[col->Index()]);
    } else {
      assert(col->IsConstant());
      new_cols1.AddUse(col);
    }
  }

  for (QueryColumnImpl *col : cols2) {
    if (col->view == merge) {
      new_cols2.AddUse(incoming_view->columns[col->Index()]);
    } else {
      assert(col->IsConstant());
      new_cols2.AddUse(col);
    }
  }

  is_canonical = false;

  cols1.Swap(new_cols1);
  cols2.Swap(new_cols2);

  return PullDataFromBeyondTrivialTuplesImpl(visited, incoming_view, cols1,
                                             cols2);
}

// Figure out what the incoming view to `cols1` is: the view producing the
// first non-constant column, or `nullptr` when every column is a constant.
// Canonicalizers use this to discover the unique predecessor that a use
// list reads from; a well-formed use list references at most one
// non-constant view (`CheckIncomingViewsMatch` asserts this), so the first
// non-constant column determines it. A `nullptr` result means the view
// consumes only constants and has no dataflow dependency.
//
//    for col in cols1:
//      if col is not a constant: return col.view
//    return nullptr
//
//    V(out: a, b)   c = constant
//    [c, a, b] -> V        [c, c] -> nullptr
QueryViewImpl *QueryViewImpl::GetIncomingView(
    const UseList<QueryColumnImpl> &cols1) {
  for (QueryColumnImpl *col : cols1) {
    if (!col->IsConstant()) {
      return col->view;
    }
  }
  return nullptr;
}

// Figure out what the incoming view to `cols1` and/or `cols2` is: the view
// producing the first non-constant column across both lists (`cols1`
// checked first), or `nullptr` when every column in both lists is a
// constant. This is the two-list form used by views with both
// `input_columns` and `attached_columns`; both lists draw from the same
// single predecessor.
//
//    for col in cols1 then cols2:
//      if col is not a constant: return col.view
//    return nullptr
QueryViewImpl *QueryViewImpl::GetIncomingView(
    const UseList<QueryColumnImpl> &cols1,
    const UseList<QueryColumnImpl> &cols2) {

  for (auto col : cols1) {
    if (!col->IsConstant()) {
      return col->view;
    }
  }
  for (auto col : cols2) {
    if (!col->IsConstant()) {
      return col->view;
    }
  }
  return nullptr;
}

// The keep-last-edge rule: a canonicalization step may drop a column edge
// to `incoming_view` only if at least one input-or-attached column edge to
// `incoming_view` remains afterward, because a column edge is what
// expresses a view's presence dependency on its predecessor in the
// dataflow. Returns `true` if at least one column in `cols1` or `cols2` is
// produced by `incoming_view`, i.e. the candidate rewritten lists retain
// the edge.
//
//    for col in cols1 then cols2:
//      if col is not a constant and col.view == incoming_view: return true
//    return false
bool QueryViewImpl::RetainsEdgeTo(const QueryViewImpl *incoming_view,
                                  const UseList<QueryColumnImpl> &cols1,
                                  const UseList<QueryColumnImpl> &cols2) {
  for (QueryColumnImpl *col : cols1) {
    if (!col->IsConstant() && col->view == incoming_view) {
      return true;
    }
  }
  for (QueryColumnImpl *col : cols2) {
    if (!col->IsConstant() && col->view == incoming_view) {
      return true;
    }
  }
  return false;
}

// Try to figure out if `view` is conditional. That could mean that it
// depends directly on a condition, or that it depends on something that
// may be present or may be absent (e.g. the output of a `JOIN`).
//
// Conditional in this case means: does `view` always have data after
// initialization? Unconditional views derive purely from constants, which
// are inserted at init time.
//
// A view whose evaluation is in progress when it is queried again depends
// on itself through a cycle. A cycle contributes no data of its own, so it
// cannot prove that data is always present; the in-progress entry is
// therefore pessimistically `true` (conditional), and every path that
// proves unconditionality stores `false` explicitly.
bool QueryViewImpl::IsConditional(
    QueryViewImpl *view,
    std::unordered_map<QueryViewImpl *, bool> &conditional_views) {

  if (conditional_views.count(view)) {
    return conditional_views[view];
  }

  auto &is_cond = conditional_views[view];
  is_cond = true;  // Pessimistic answer for cyclic re-entry.

  // An unsatisfiable or dead view never has data, so it certainly does not
  // always have data.
  if (view->is_unsat || view->is_dead) {
    is_cond = true;
    return true;
  }

  // These all introduce control dependencies. It's too annoying to truly
  // detect if the effective tests (e.g. compare `1=1`) actually are conditional
  // so we just assume these things are conditional.
  if (view->AsJoin() || view->AsCompare() || view->AsNegate() ||
      view->AsAggregate() || view->AsKVIndex()) {

    is_cond = true;
    return true;

  // Maps are not conditional iff their input view is not conditional and the
  // functor's range is one-to-one.
  } else if (QueryMapImpl *map = view->AsMap()) {
    if (FunctorRange::kOneToOne != map->functor.Range()) {
      is_cond = true;
      return true;
    }

    QueryViewImpl *incoming_view = QueryViewImpl::GetIncomingView(
        view->input_columns, view->attached_columns);
    if (!incoming_view) {
      is_cond = false;
      return false;
    } else {
      is_cond = IsConditional(incoming_view, conditional_views);
      return is_cond;
    }

  } else if (QueryMergeImpl *merge = view->AsMerge()) {

    // A MERGE with no merged views never has data.
    if (merge->merged_views.Empty()) {
      is_cond = true;
      return true;
    }
    for (QueryViewImpl *merged_view : merge->merged_views) {
      if (IsConditional(merged_view, conditional_views)) {
        is_cond = true;
        return true;
      }
    }
    is_cond = false;
    return false;

  } else if (QuerySelectImpl *sel = view->AsSelect()) {
    if (auto stream = sel->stream.get()) {
      if (stream->AsIO()) {
        is_cond = true;
        return true;
      } else {
        is_cond = false;
        return false;
      }
    } else if (QueryRelationImpl *rel = sel->relation.get()) {
      for (QueryViewImpl *insert : rel->inserts) {
        if (IsConditional(insert, conditional_views)) {
          is_cond = true;
          return true;
        }
      }
    }

    is_cond = false;
    return false;

  } else if (view->AsTuple() || view->AsInsert()) {
    if (QueryViewImpl *incoming_view = QueryViewImpl::GetIncomingView(
            view->input_columns, view->attached_columns)) {
      is_cond = IsConditional(incoming_view, conditional_views);
    } else {
      is_cond = false;
    }
    return is_cond;

  } else {
    assert(false);
    return true;
  }
}

// Returns a pointer to the only user of this node, or nullptr if there are
// zero users, or more than one users.
QueryViewImpl *QueryViewImpl::OnlyUser(void) const noexcept {
  QueryViewImpl *only_user = nullptr;
  bool fail = false;
  for (QueryColumnImpl *col : columns) {
    col->ForEachUser([&](QueryViewImpl *user) {
      if (!only_user) {
        only_user = user;
      } else if (only_user != user) {
        fail = true;
      }
    });
    if (fail) {
      return nullptr;
    }
  }
  this->ForEachUse<QueryViewImpl>([&](QueryViewImpl *user, QueryViewImpl *) {
    if (!only_user) {
      only_user = user;
    } else if (only_user != user) {
      fail = true;
    }
  });

  return fail ? nullptr : only_user;
}

// Check that all non-constant views in `cols1` match.
bool QueryViewImpl::CheckIncomingViewsMatch(
    const UseList<QueryColumnImpl> &cols1) const {
#ifndef NDEBUG
  QueryViewImpl *prev_view = nullptr;
  for (QueryColumnImpl *col : cols1) {
    if (!col->IsConstant()) {
      if (prev_view) {
        if (prev_view != col->view) {
          invalid_var = col->var;
          return false;
        }
      } else {
        prev_view = col->view;
        if (prev_view == this) {
          return false;
        }
      }
    }
  }
#else
  (void) cols1;
#endif
  return true;
}

// Check that all non-constant views in `cols1` and `cols2` match.
//
// NOTE(pag): This isn't a pairwise matching; instead it checks that all
//            columns in both of the lists independently reference the same
//            view.
bool QueryViewImpl::CheckIncomingViewsMatch(
    const UseList<QueryColumnImpl> &cols1,
    const UseList<QueryColumnImpl> &cols2) const {
#ifndef NDEBUG
  QueryViewImpl *prev_view = nullptr;

  auto do_cols = [this, &prev_view](const auto &cols) -> bool {
    for (QueryColumnImpl *col : cols) {
      if (!col->IsConstant()) {
        if (prev_view) {
          if (prev_view != col->view) {
            this->invalid_var = col->var;
            return false;
          }
        } else {
          prev_view = col->view;
          if (prev_view == this) {
            return false;
          }
        }
      }
    }
    return true;
  };
  return do_cols(cols1) && do_cols(cols2);
#else
  (void) cols1;
  (void) cols2;
  return true;
#endif
}

// Check if the `group_ids` of two views have any overlaps.
//
// Two selects in the same logical clause are not allowed to be merged,
// except in rare cases like constant streams. For example, consider the
// following:
//
//    node_pairs(A, B) : node(A), node(B).
//
// `node_pairs` is the cross-product of `node`. The two selects associated
// with each invocation of `node` are structurally the same, but cannot
// be merged because otherwise we would not get the cross product.
//
// NOTE(pag): The `group_ids` are sorted.
bool QueryViewImpl::InsertSetsOverlap(QueryViewImpl *a, QueryViewImpl *b) {

  //  if (a->check_group_ids != b->check_group_ids) {
  //    return true;
  //  }
  //
  //  if (!a->check_group_ids) {
  //    return false;
  //  }

  if (a->group_ids.empty() || b->group_ids.empty()) {
    return false;
  }

  for (auto i = 0u, j = 0u;
       i < a->group_ids.size() && j < b->group_ids.size();) {

    if (a->group_ids[i] == b->group_ids[j]) {
      return true;

    } else if (a->group_ids[i] < b->group_ids[j]) {
      ++i;

    } else {
      ++j;
    }
  }

  return false;

  //double_check:
  //  auto a_used_by_join = false;
  //  a->ForEachUse<JOIN>([&a_used_by_join] (JOIN *, QueryViewImpl *) {
  //    a_used_by_join = true;
  //  });
  //
  //  auto b_used_by_join = false;
  //  b->ForEachUse<JOIN>([&b_used_by_join] (JOIN *, QueryViewImpl *) {
  //    b_used_by_join = true;
  //  });
  //
  //  return a_used_by_join || b_used_by_join;
}

// Mark this node as being unsatisfiable.
void QueryViewImpl::MarkAsUnsatisfiable(void) {
  is_unsat = true;
  ForEachUse<QueryViewImpl>([](QueryViewImpl *user_view, QueryViewImpl *) {
    user_view->is_canonical = false;
  });
}

}  // namespace hyde
