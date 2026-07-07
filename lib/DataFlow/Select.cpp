// Copyright 2020, Trail of Bits. All rights reserved.

#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Util/EqualitySet.h>

#include "Query.h"

namespace hyde {

QuerySelectImpl::~QuerySelectImpl(void) {}

QuerySelectImpl::QuerySelectImpl(QueryRelationImpl *relation_,
                                 ParsedPredicate pred_)
    : pred(pred_),
      position(pred_.SpellingRange().From()),
      relation(this, relation_),
      inserts(this) {}

QuerySelectImpl::QuerySelectImpl(QueryStreamImpl *stream_,
                                 ParsedPredicate pred_)
    : pred(pred_),
      position(pred_.SpellingRange().From()),
      stream(this, stream_),
      inserts(this) {
  if (auto input_stream = stream->AsIO(); input_stream) {
    this->can_receive_deletions =
        ParsedMessage::From(input_stream->declaration).IsDifferential();
    this->can_produce_deletions = this->can_receive_deletions;
  }
}

QuerySelectImpl::QuerySelectImpl(QueryStreamImpl *stream_,
                                 DisplayRange spelling_range)
    : pred(std::nullopt),
      position(spelling_range.From()),
      stream(this, stream_),
      inserts(this) {
  if (auto input_stream = stream->AsIO(); input_stream) {
    this->can_receive_deletions =
        ParsedMessage::From(input_stream->declaration).IsDifferential();
    this->can_produce_deletions = this->can_receive_deletions;
  }
}

const char *QuerySelectImpl::KindName(void) const noexcept {
  if (relation) {
    return "PUSH";
  } else if (auto s = stream.get(); s) {
    if (s->AsConstant()) {
      return "CONST";
    } else if (s->AsIO()) {
      return "RECEIVE";
    } else {
      assert(false);
      return "STREAM";
    }
  } else {
    return "SELECT";
  }
}

QuerySelectImpl *QuerySelectImpl::AsSelect(void) noexcept {
  return this;
}

uint64_t QuerySelectImpl::Hash(void) noexcept {
  if (hash) {
    return hash;
  }

  hash = HashInit();
  assert(hash != 0);
  const auto hash_ror = RotateRight64(hash, 33u);

  if (relation) {
    hash ^= hash_ror * relation->declaration.Id();

  } else if (stream) {
    if (auto const_tag = stream->AsTag()) {
      hash ^= hash_ror * (const_tag->val + 1ull);

    } else if (auto const_stream = stream->AsConstant()) {
      if (const_stream->literal->IsConstant()) {
        hash ^= hash_ror * const_stream->literal->Literal().IdentifierId();
      } else {
        hash ^= hash_ror *
                std::hash<std::string_view>()(
                    *const_stream->literal->Spelling(Language::kUnknown));
      }

    } else if (auto input_stream = stream->AsIO()) {
      hash ^= hash_ror * input_stream->declaration.Id();

    } else {
      assert(is_dead);
    }
  } else {
    assert(is_dead);
  }
  return hash;
}

// Return a number that can be used to help sort this node. The idea here
// is that we often want to try to merge together two different instances
// of the same underlying node when we can.
uint64_t QuerySelectImpl::Sort(void) noexcept {
  return position.Index();
}

unsigned QuerySelectImpl::Depth(void) noexcept {
  if (depth) {
    return depth;
  }

  auto estimate = EstimateDepth(input_columns, 0u);
  estimate = EstimateDepth(positive_conditions, estimate);
  estimate = EstimateDepth(negative_conditions, estimate);
  depth = estimate + 1u;  // Base case if there are cycles.

  auto real = GetDepth(input_columns, 0u);
  real = GetDepth(positive_conditions, real);
  real = GetDepth(negative_conditions, real);

  if (relation) {
    for (auto insert : relation->inserts) {
      real = std::max(real, insert->Depth());
    }
  }

  depth = real + 1u;

  return depth;
}

// Put this view into a canonical form. Returns `true` if changes were made
// beyond the scope of this view.
//
// A SELECT is a dataflow source: it pushes rows out of a RELATION (PUSH),
// receives tuples of an input message (RECEIVE), or yields a constant
// (CONST). Canonicalizing a SELECT is dead-source elimination: when no
// downstream view consumes any of its columns, the node is detached from the
// query and marked dead, so no receive/scan machinery is ever generated for
// data that cannot flow anywhere. The owning RELATION or IO stream always
// holds a reference back to the SELECT, which makes the generic
// `VIEW::IsUsed` test report every SELECT as used; this pass therefore runs
// its own usage scan, first over the output columns and then over the actual
// VIEW-to-VIEW use edges (`average_weight.dr` produces an orphaned SELECT
// that only this scan detects). A SELECT that sets a zero-argument CONDition
// is kept as-is, because condition testers depend on it without consuming
// any of its columns. The sole remaining RECEIVE of a message is also kept
// as-is: the message is part of the program's external interface, and its
// handler must exist even when optimization proves that the received data
// can never flow anywhere (the handler simply discards it).
//
//     if dead or sets a CONDition:           keep as-is
//     if any output column has a user:       already canonical
//     if any VIEW uses this SELECT:          already canonical
//     // only the RELATION/IO back-references remain
//     if this is the sole RECEIVE of its message: keep as-is
//     detach from the query and mark dead    (non-local change)
//
// Unused source elimination (`A`, `B` have no users):
//
//     RECEIVE msg              (node deleted, unless it is the last
//       A  B          ==>       RECEIVE of `msg`, which stays as the
//     (no users)                message's interface)
//
// Condition-setting SELECT (kept although no column is used):
//
//     RECEIVE msg              RECEIVE msg
//       A  B                     A  B
//     sets COND c     ==>      sets COND c        (unchanged)
//         :                        :
//     testers of c             testers of c
bool QuerySelectImpl::Canonicalize(QueryImpl *query,
                                     const OptimizationContext &opt,
                                     const ErrorLog &err) {

  if (is_dead || sets_condition) {
    return false;
  }

  if (sets_condition && 0u < (sets_condition->positive_users.Size() +
                              sets_condition->negative_users.Size())) {
    return true;
  }

  for (auto col : columns) {
    if (col->IsUsedIgnoreMerges()) {
      return false;
    }
  }

  auto is_really_used = false;
  ForEachUse<VIEW>(
      [&is_really_used](VIEW *, VIEW *) { is_really_used = true; });

  if (!is_really_used) {

    // The last `RECEIVE` of a message is the message's interface with the
    // outside world; it stays even when its data is proven to flow nowhere.
    if (stream) {
      if (auto io = stream->AsIO(); io && io->receives.Size() == 1u) {
        return false;
      }
    }

    PrepareToDelete();
    return true;

  } else {
    return false;
  }
}

// Equality over SELECTs is a mix of structural and pointer-based.
bool QuerySelectImpl::Equals(EqualitySet &eq,
                               QueryViewImpl *that_) noexcept {
  const auto that = that_->AsSelect();
  if (!that || can_receive_deletions != that->can_receive_deletions ||
      can_produce_deletions != that->can_produce_deletions ||
      positive_conditions != that->positive_conditions ||
      negative_conditions != that->negative_conditions ||
      columns.Size() != that->columns.Size() ||
      input_columns.Size() != that->input_columns.Size()) {
    return false;
  }

  if (eq.Contains(this, that)) {
    return true;
  }

  if (stream) {
    if (stream.get() != that->stream.get()) {
      return false;
    }

    if (stream->AsConstant()) {
      return true;
    }

  } else if (relation) {
    if (!that->relation ||
        relation->declaration.Id() != that->relation->declaration.Id()) {
      return false;
    }
  }

  if (InsertSetsOverlap(this, that)) {
    return false;
  }

  eq.Insert(this, that);
  return true;
}

}  // namespace hyde
