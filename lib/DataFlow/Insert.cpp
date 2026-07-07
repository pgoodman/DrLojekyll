// Copyright 2020, Trail of Bits. All rights reserved.

#include <drlojekyll/Util/EqualitySet.h>

#include "Query.h"

namespace hyde {

QueryInsertImpl::~QueryInsertImpl(void) {}

QueryInsertImpl::QueryInsertImpl(QueryRelationImpl *relation_, ParsedDeclaration decl_)
    : relation(this, relation_),
      declaration(decl_) {}

QueryInsertImpl::QueryInsertImpl(QueryStreamImpl *stream_, ParsedDeclaration decl_)
    : stream(this, stream_),
      declaration(decl_) {}

QueryInsertImpl *QueryInsertImpl::AsInsert(void) noexcept {
  return this;
}

const char *QueryInsertImpl::KindName(void) const noexcept {
  if (declaration.Kind() == DeclarationKind::kQuery) {
    return "MATERIALIZE";

  } else if (declaration.Kind() == DeclarationKind::kMessage) {
    return "TRANSMIT";

  } else {
    return "INSERT";
  }
}

uint64_t QueryInsertImpl::Hash(void) noexcept {
  if (hash) {
    return hash;
  }

  // Start with an initial hash just in case there's a cycle somewhere.
  hash = HashInit() ^ declaration.Id();
  assert(hash != 0);

  auto local_hash = hash;

  // Mix in the hashes of the input by columns; these are ordered.
  for (auto col : input_columns) {
    local_hash ^= RotateRight64(local_hash, 33) * col->Hash();
  }

  for (auto col : attached_columns) {
    local_hash ^= RotateRight64(local_hash, 53) * col->Hash();
  }

  hash = local_hash;
  return local_hash;
}

// Put this INSERT into a canonical form. An INSERT is a data sink: it writes
// its input columns into a relation (INSERT), a message stream (TRANSMIT),
// or a query result (MATERIALIZE), and produces no output columns of its
// own. Its
// `attached_columns` are read-only witness edges: they are read from the
// incoming view to keep this INSERT's dataflow dependency on it expressed
// as a column edge, but they are not stored. Its canonical form reads its
// inputs from as far up the dataflow as possible: the pass hops the input
// and attached columns over trivially forwarding TUPLEs and single-source
// UNIONs, so long as each hopped-over node is unconditional or guarded only
// by trivial positive conditions. This is beneficial because it makes the
// intervening forwarding nodes dead (hence deletable) and makes structurally
// identical INSERTs more likely to hash/compare as equal, enabling CSE in
// the optimizer driver. It is sound because a TUPLE only forwards values and
// a single-source UNION emits exactly its one source's data, so rewiring the
// reads upstream leaves the multiset of inserted tuples unchanged. If the
// view feeding the INSERT is unsatisfiable then the INSERT can never fire,
// so it is marked unsatisfiable and unlinked from the graph.
//
// Attached columns then shrink: a constant attached column reads nothing
// from the incoming view, and a duplicate attached column (one that repeats
// an input column or an earlier attached column) reads nothing new, so both
// are dropped — subject to the keep-last-edge rule: at least one
// input-or-attached column edge to the incoming view must remain, so if
// dropping would sever the final edge, the first attached column produced
// by the incoming view is retained as the representative witness.
//
//   is_canonical = true
//   if valid and the non-constant inputs read from multiple
//   views: mark self invalid (debug-only invariant check)
//   incoming = PullDataFromBeyondTrivialTuples(input_columns,
//                                              attached_columns)
//     // may clear `is_canonical` if it rewires any input
//   if self is not already unsatisfiable and incoming is:
//     mark self unsatisfiable  // dirties all users
//     unlink self from the dataflow graph
//     return changed
//   new_attached = [c for c in attached_columns
//                   if c is not constant and not a duplicate]
//   if incoming and not RetainsEdgeTo(incoming, input_columns, new_attached):
//     new_attached += first attached column produced by incoming
//   return whether any input was rewired or any attached column dropped
//
// Dropping a duplicate witness column (the edge through `a` already exists
// via the stored column; `w` stays):
//
//   Before:                          After:
//
//     VIEW V[a, w]                     VIEW V[a, w]
//      |a  |w,a(attached)               |a  |w(attached)
//     INSERT rel(a)                    INSERT rel(a)
//
// Hopping over an unconditional forwarding TUPLE:
//
//   Before:                          After:
//
//     VIEW V[a, b]                     VIEW V[a, b]
//      |a  |b                           |a  |b
//     TUPLE T[x=a, y=b]                INSERT rel(a, b)
//      |x  |y
//     INSERT rel(x, y)                 (T deleted once unused)
//
// Hopping over a UNION whose only distinct source is one view (its other
// arms are itself or this INSERT's own view):
//
//   Before:                          After:
//
//     VIEW V[a]                        VIEW V[a]
//      |a    \                          |a
//     UNION U[m]                       INSERT rel(a)
//      |m
//     INSERT rel(m)                    (U deleted once unused)
//
// Unsatisfiable predecessor:
//
//   Before:                          After:
//
//     VIEW V[a]  (unsat)               VIEW V[a]  (unsat)
//      |a
//     INSERT rel(a)                    (INSERT unsat, unlinked)
bool QueryInsertImpl::Canonicalize(QueryImpl *, const OptimizationContext &,
                                     const ErrorLog &) {
  is_canonical = true;
  if (valid == VIEW::kValid &&
      !CheckIncomingViewsMatch(input_columns, attached_columns)) {
    valid = VIEW::kInvalidBeforeCanonicalize;
  }

  assert(columns.Empty());

  // NOTE(pag): This may update `is_canonical`.
  auto incoming_view = PullDataFromBeyondTrivialTuples(
      GetIncomingView(input_columns, attached_columns), input_columns,
      attached_columns);

  // An unsatisfiable INSERT is dropped.
  if (!is_unsat && incoming_view && incoming_view->is_unsat) {
    MarkAsUnsatisfiable();
    PrepareToDelete();
    return true;
  }

  // Drop constant and duplicate attached columns: a constant reads nothing
  // from the incoming view, and a duplicate reads nothing new.
  if (!attached_columns.Empty()) {
    UseList<COL> new_attached_columns(this);

    for (COL *col : attached_columns) {
      auto is_duplicate = false;
      for (COL *in_col : input_columns) {
        if (in_col == col) {
          is_duplicate = true;
          break;
        }
      }
      if (!is_duplicate) {
        for (COL *prev_col : new_attached_columns) {
          if (prev_col == col) {
            is_duplicate = true;
            break;
          }
        }
      }

      if (!is_duplicate && !col->IsConstantOrConstantRef()) {
        new_attached_columns.AddUse(col);
      }
    }

    // The keep-last-edge rule: dropping attached columns must not sever the
    // final input-or-attached column edge to the incoming view, because
    // that edge is what expresses this INSERT's presence dependency on it.
    // Retain the first attached column produced by the incoming view as the
    // representative witness.
    if (incoming_view &&
        !RetainsEdgeTo(incoming_view, input_columns, new_attached_columns)) {
      for (COL *col : attached_columns) {
        if (!col->IsConstant() && col->view == incoming_view) {
          new_attached_columns.AddUse(col);
          break;
        }
      }
      assert(
          RetainsEdgeTo(incoming_view, input_columns, new_attached_columns));
    }

    if (new_attached_columns != attached_columns) {
      attached_columns.Swap(new_attached_columns);
      hash = 0;
      is_canonical = false;
    }
  }

  if (!is_canonical) {
    is_canonical = true;
    return true;
  }

  return false;
}

// Equality over inserts is structural.
bool QueryInsertImpl::Equals(EqualitySet &eq, VIEW *that_) noexcept {

  if (eq.Contains(this, that_)) {
    return true;
  }

  const auto that = that_->AsInsert();

  // NOTE(pag): Declaration (context) equality, not `Id()` equality: the
  //           `Id()`s of name-less auto-declared zero-arity exports collide.
  if (!that || can_produce_deletions != that->can_produce_deletions ||
      declaration != that->declaration ||
      columns.Size() != that->columns.Size() ||
      input_columns.Size() != that->input_columns.Size() ||
      attached_columns.Size() != that->attached_columns.Size()) {
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
