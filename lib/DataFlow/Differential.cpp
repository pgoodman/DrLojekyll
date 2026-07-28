// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2020, Trail of Bits. All rights reserved.

#include <cstdio>
#include <cstdlib>

#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Parse/Parse.h>

#include "Query.h"

namespace hyde {

// Enumerate the INSERT -> SELECT seams of the dataflow: pairs where a
// SELECT reads the same declaration (relation or message stream) that an
// INSERT writes. These are dataflow edges that are not expressed as column
// edges.
void QueryImpl::ForEachInsertToSelectSeam(
    std::function<void(QueryInsertImpl *, QuerySelectImpl *)> cb) const {

  std::unordered_map<ParsedDeclaration, std::vector<SELECT *>> decl_to_selects;

  for (auto select : selects) {
    if (auto rel = select->relation.get(); rel) {
      decl_to_selects[rel->declaration].push_back(select);
    } else if (auto stream = select->stream.get(); stream) {
      if (auto input = stream->AsIO(); input) {
        decl_to_selects[input->declaration].push_back(select);
      }
    }
  }

  for (auto insert : inserts) {
    if (auto it = decl_to_selects.find(insert->declaration);
        it != decl_to_selects.end()) {
      for (auto select : it->second) {
        cb(insert, select);
      }
    }
  }
}

// Identify which data flows can receive and produce deletions.
void QueryImpl::TrackDifferentialUpdates(const ErrorLog &log,
                                         bool report_message_errors) const {

  std::unordered_map<INSERT *, std::vector<SELECT *>> insert_to_selects;
  ForEachInsertToSelectSeam([&insert_to_selects](INSERT *insert,
                                                 SELECT *select) {
    insert_to_selects[insert].push_back(select);
  });

  const_cast<const QueryImpl *>(this)->ForEachView([](VIEW *v) {
    v->can_receive_deletions = false;
    v->can_produce_deletions = false;
  });

  // Receives of a `@differential` message are differential at the source.
  for (auto select : selects) {
    if (auto stream = select->stream.get(); stream) {
      if (auto input = stream->AsIO(); input) {
        if (ParsedMessage::From(input->declaration).IsDifferential()) {
          select->can_receive_deletions = true;
          select->can_produce_deletions = true;
        }
      }
    }
  }

  // R3 (v3-spec-statecell.md §C-4): an AGGREGATE or KVINDEX produces deletions
  // from VALUE CHURN — a group's summary changing retracts the old
  // (group, old-value) row and asserts the new one — so it is a deletion
  // PRODUCER unconditionally, seeding the downstream `can_receive` propagation.
  for (auto agg : aggregates) {
    agg->can_produce_deletions = true;
  }
  for (auto kv : kv_indices) {
    kv->can_produce_deletions = true;
  }

  for (auto changed = true; changed && log.IsEmpty();) {
    changed = false;
    ForEachView([&](VIEW *view) {
      if (!view->can_produce_deletions) {
        if (auto negate = view->AsNegate(); negate) {

          // Using `@never ...` means that we assume the negated side, once
          // found absent, is /always/ found absent.
          if (negate->is_never) {
            if (view->can_receive_deletions) {
              view->can_produce_deletions = true;
              changed = true;
            }

            // NOTE (D3.a.1 / DS-R4-10): the `@never`-over-differential reject
            // that used to live HERE was visit-order fragile (it only ran
            // while this negate's own can_produce_deletions was still false)
            // and mode-split under -disable-dataflow-opt; it moved to the
            // POST-fixpoint sweep below, which checks the final closure bits.
          } else {
            view->can_produce_deletions = true;
            changed = true;
          }
        }
      }

      if (view->can_receive_deletions && !view->can_produce_deletions) {
        view->can_produce_deletions = true;
        changed = true;
      }

      if (auto insert = view->AsInsert();
          insert && insert->can_produce_deletions) {
        for (auto select : insert_to_selects[insert]) {
          if (!select->can_receive_deletions) {
            changed = true;
            select->can_receive_deletions = true;
          }
        }
      }

      if (!view->can_produce_deletions) {
        return;
      }

      for (COL *col : view->columns) {
        col->ForEachUser([&](VIEW *user_view) {
          if (!user_view->can_receive_deletions) {
            user_view->can_receive_deletions = true;
            changed = true;
          }
        });
      }
    });
  }

  // DS-R4-10 (OQ-NEVER, ruled REJECT): '@never' assumes once-absent-always-
  // absent, so a negated view that can produce differential updates
  // invalidates the gate (its rows can appear AND retract). Checked POST-
  // fixpoint on the final closure bits: the in-fixpoint check this replaces
  // was visit-order fragile — a negate whose own can_produce flipped first
  // (e.g. from a differential non-negated input) skipped the arm forever,
  // and the uncanonicalized -disable-dataflow-opt shape slipped through
  // (mode-split accept of a latent wrong-answer program; FINDINGS.md F25).
  // The raw `negations` DefList retains dead defs (PrepareToDelete clears
  // negated_view), hence the is_dead/null guard (A4.1).
  if (log.IsEmpty()) {
    for (const auto &negate : negations) {
      if (negate->is_dead || !negate->negated_view || !negate->is_never ||
          !negate->negated_view->can_produce_deletions) {
        continue;
      }
      auto reported = false;
      for (ParsedPredicate pred : negate->negations) {
        if (pred.IsNegatedWithNever()) {
          reported = true;
          log.Append(pred.SpellingRange(), pred.Negation().SpellingRange())
              << "'@never' cannot operate on a predicate that can "
              << "produce differential updates";
        }
      }
      // [ALWAYS-ON] (Fable review [C]): every user-written @never carries an
      // @never-flagged ParsedPredicate, so `reported` holds on any parseable
      // program — a miss means a fabricated/rebuilt negate reached the fence
      // with no diagnostic anchor, and an assert would NDEBUG-strip into a
      // SILENT ACCEPT of exactly the shape DS-R4-10 rejects (the F18/F25
      // wrong-answer class). Hard-stop instead.
      if (!reported) {
        fprintf(stderr, "error: DS-R4-10 fence: a live @never negate over a "
                        "differential negated view carries no @never-flagged "
                        "predicate to diagnose\n");
        abort();
      }
    }
  }

  if (!report_message_errors) {
    return;
  }

  for (auto view : inserts) {
    if (!view->stream) {
      continue;
    }

    const auto io = view->stream->AsIO();
    if (!io) {
      continue;
    }

    const auto message = ParsedMessage::From(io->declaration);
    const auto range = message.SpellingRange();

    // Require that the source code be faithful to the data flow in terms of
    // what messages can receive and produce differentials.

    if (message.IsDifferential()) {
      if (!view->can_produce_deletions) {
        assert(!view->can_receive_deletions);

        //        log.Append(range, message.Differential().SpellingRange())
        //            << "Message '" << message.Name() << '/' << message.Arity()
        //            << "' is marked with the '@differential' attribute but cannot "
        //            << "produce deletions";
      }

    } else if (view->can_produce_deletions) {
      log.Append(range, range.To())
          << "Message '" << message.Name() << '/' << message.Arity()
          << "' can produce deletions but is not marked with the "
          << "'@differential' attribute";
    }
  }
}

}  // namespace hyde
