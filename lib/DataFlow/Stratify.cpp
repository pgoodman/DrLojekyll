// Copyright 2026, Trail of Bits. All rights reserved.

#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Parse/Parse.h>

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "EquivalenceSet.h"
#include "Query.h"

namespace hyde {
namespace {

// Per-view bookkeeping for the iterative Tarjan SCC computation.
struct TarjanState {
  static constexpr unsigned kUnvisited = ~0u;
  static constexpr unsigned kNoStratum = ~0u;

  unsigned index{kUnvisited};
  unsigned lowlink{0u};
  unsigned stratum{kNoStratum};
  bool on_stack{false};
};

// One DFS frame of the iterative Tarjan walk: which view we're at, and how
// many of its source edges we've already followed.
struct TarjanFrame {
  unsigned view_index;
  unsigned next_edge;
};

}  // namespace

// Stratify the dataflow. This pass computes the strongly connected components
// of the complete view graph and orders them topologically; a stratum is one
// SCC, so a recursive fixpoint (an induction) is exactly a multi-view
// stratum, and every dataflow edge either stays within a stratum (a recursive
// back-edge) or crosses from a lower stratum to a higher one (a seed).
//
// The edge relation is every way data can flow between views:
//
//   - column edges, read off the `predecessors` lists that `LinkViews` built
//     (these include the unit-relation INSERT -> SELECT edges);
//   - `negated_view` edges of NEGATEs (an absence read is a dependency);
//   - the INSERT -> SELECT declaration seams that
//     `ForEachInsertToSelectSeam` enumerates. The seams that are not already
//     column edges are the message publish -> receive pairs of programs that
//     both transmit and receive one message; they order the receive's
//     stratum above the publisher's, matching the differential-capability
//     propagation in `TrackDifferentialUpdates`, but they are external to
//     the database (published rows reach the receive only through the
//     client), so `IdentifyInductions` does not traverse them.
//
// Outputs, stored on the IR:
//
//   - `view->stratum`: the topological index of the view's SCC (equal ids
//     <=> same SCC); `QueryImpl::num_strata` bounds them.
//   - `EquivalenceSet::stratum` (on the canonical set): the stratum owning
//     the data model's table = the highest stratum among the model's views.
//     A model whose views straddle strata is recorded in
//     `QueryImpl::stratum_straddling_models`; its lower-stratum views'
//     insertions are seeds of the owning stratum.
//   - the derivation class of any table-inserting edge is derivable from
//     these ids (`QueryView::DerivationClassInto`): recursive iff the
//     deriving view shares an SCC with any of the target model's views.
//
// The pass also enforces stratified negation: a NEGATE whose negated view
// lies in the NEGATE's own SCC reads the absence of data that the negation's
// own result can still produce, which has no order-independent meaning. Such
// negations are rejected with an error on the negated predicate;
// `tests/OptDiff/cases/evm_func_parse.dr` is the corpus witness of the
// shape. A negation of a *lower* stratum is the fully supported shape: the
// negated data is final before the reading stratum runs.
//
//     index views; sources(v) := predecessors(v) ∪ {negated_view(v)}
//                                ∪ {INSERT i : (i, v) is a decl seam}
//     run Tarjan over `sources`; an SCC pops only after every SCC it reads
//       from has popped, so `stratum := pop order` topologically sorts the
//       condensation with data sources first
//     stratum(model) := max stratum over member views; record straddlers
//     for each NEGATE n:
//       if stratum(negated_view(n)) == stratum(n): error (unstratified)
//
// Before (view graph):                  After (strata assigned):
//
//   RECV base   RECV edge                 s0        s1
//       |          |                       .         .
//     TUPLE      TUPLE                    s0        s1
//        \        /                        \        /
//        UNION t <---.                     UNION t: s2 --.
//           |        |                        |          |  (in-stratum
//         JOIN ------'                      JOIN: s2 ----'   back-edge)
//           |                                 |
//        INSERT t_out                       INSERT: s3
//
//   `t`'s SCC {UNION, JOIN} is stratum 2: it reads only strata 0/1 (seeds,
//   C_nr edges) and itself (back-edge, C_r edge); the INSERT below it is a
//   higher stratum. A NEGATE reading `t` from stratum 3+ is legal; a NEGATE
//   inside stratum 2 negating `t` draws the error.
//
// Cross-checks against `IdentifyInductions` (debug builds): that pass seeds
// only MERGE/JOIN/NEGATE views with `InductionInfo` and unions two views'
// merge sets exactly when they are mutually reachable, so on info-bearing
// views its structure must agree with the SCC condensation. Interior
// TUPLE/CMP/MAP views on a cycle never carry info, and JOIN/NEGATEs wholly
// inside back-edges have their info reset — both are legitimate divergences
// (membership is partial), as are SCCs closed only through message
// publish -> receive seams, which IdentifyInductions cannot see. Everywhere
// else:
//
//   - a predecessor/successor is masked inductive iff it shares the SCC;
//   - `merge_set_id` and SCC membership induce the same partition;
//   - every multi-view SCC contains an inductive MERGE (all dataflow cycles
//     pass through UNIONs).
//
// The first check is what grounds the derivation-class split: an insert edge
// classified recursive (same SCC) is exactly an inductive-predecessor edge,
// i.e. a fixpoint-body emission position in the control-flow build, and a
// non-recursive edge is a seed/init emission position.
void QueryImpl::Stratify(const ErrorLog &log) {

  // Index the (live) views.
  std::vector<VIEW *> views;
  std::unordered_map<VIEW *, unsigned> index_of;
  const_cast<const QueryImpl *>(this)->ForEachView([&](VIEW *view) {
    index_of.emplace(view, static_cast<unsigned>(views.size()));
    views.push_back(view);
  });

  const auto num_views = static_cast<unsigned>(views.size());

  // Build the source-edge adjacency. Duplicate edges are harmless to
  // Tarjan, so no deduplication is needed.
  std::vector<std::vector<unsigned>> sources(num_views);

  auto add_source = [&](unsigned view_index, VIEW *source_view) {
    const auto it = index_of.find(source_view);
    assert(it != index_of.end());
    if (it != index_of.end()) {
      sources[view_index].push_back(it->second);
    }
  };

  for (auto i = 0u; i < num_views; ++i) {
    VIEW *const view = views[i];
    for (VIEW *pred_view : view->predecessors) {
      add_source(i, pred_view);
    }
    if (NEGATION *negate = view->AsNegate()) {
      add_source(i, negate->negated_view.get());
    }
  }

  // Message publish -> receive seams (an INSERT to a stream): these edges
  // are invisible to `IdentifyInductions`, so remember them for the debug
  // cross-checks below.
  std::vector<std::pair<unsigned, unsigned>> io_seams;  // (select, insert)
  ForEachInsertToSelectSeam([&](INSERT *insert, SELECT *select) {
    const auto insert_it = index_of.find(insert);
    const auto select_it = index_of.find(select);
    if (insert_it == index_of.end() || select_it == index_of.end()) {
      return;  // A dead view's seam.
    }
    sources[select_it->second].push_back(insert_it->second);
    if (insert->stream) {
      io_seams.emplace_back(select_it->second, insert_it->second);
    }
  });

  // Iterative Tarjan. Because edges point from a view to its data sources,
  // an SCC is popped only after every SCC it (transitively) reads from has
  // been popped, so the pop order is a topological order of the condensation
  // with sources first: `stratum` ids increase along the dataflow.
  std::vector<TarjanState> state(num_views);
  std::vector<unsigned> scc_stack;
  std::vector<TarjanFrame> frames;
  unsigned next_index = 0u;
  unsigned next_stratum = 0u;

  for (auto root = 0u; root < num_views; ++root) {
    if (state[root].index != TarjanState::kUnvisited) {
      continue;
    }

    state[root].index = state[root].lowlink = next_index++;
    state[root].on_stack = true;
    scc_stack.push_back(root);
    frames.push_back({root, 0u});

    while (!frames.empty()) {
      TarjanFrame &frame = frames.back();
      const auto v = frame.view_index;

      if (frame.next_edge < sources[v].size()) {
        const auto w = sources[v][frame.next_edge++];
        if (state[w].index == TarjanState::kUnvisited) {
          state[w].index = state[w].lowlink = next_index++;
          state[w].on_stack = true;
          scc_stack.push_back(w);
          frames.push_back({w, 0u});

        } else if (state[w].on_stack) {
          state[v].lowlink = std::min(state[v].lowlink, state[w].index);
        }

      } else {
        frames.pop_back();
        if (!frames.empty()) {
          auto &parent = state[frames.back().view_index];
          parent.lowlink = std::min(parent.lowlink, state[v].lowlink);
        }

        // `v` is the root of an SCC: pop its members and assign a stratum.
        if (state[v].lowlink == state[v].index) {
          for (;;) {
            const auto w = scc_stack.back();
            scc_stack.pop_back();
            state[w].on_stack = false;
            state[w].stratum = next_stratum;
            if (w == v) {
              break;
            }
          }
          ++next_stratum;
        }
      }
    }
  }

  assert(scc_stack.empty());
  num_strata = next_stratum;

  for (auto i = 0u; i < num_views; ++i) {
    assert(state[i].stratum != TarjanState::kNoStratum);
    views[i]->stratum = state[i].stratum;
  }

  // Assign each data model the highest stratum among its views (that
  // stratum's phases own the model's table), and record the models whose
  // views straddle strata.
  std::unordered_map<EquivalenceSet *, std::pair<unsigned, unsigned>>
      model_strata;  // (min, max) stratum over the model's views.
  for (auto i = 0u; i < num_views; ++i) {
    assert(views[i]->equivalence_set);
    EquivalenceSet *const model = views[i]->equivalence_set->Find();
    const auto s = state[i].stratum;
    if (auto [it, added] = model_strata.emplace(model, std::make_pair(s, s));
        !added) {
      it->second.first = std::min(it->second.first, s);
      it->second.second = std::max(it->second.second, s);
    }
  }

  stratum_straddling_models.clear();
  for (const auto &[model, min_max] : model_strata) {
    model->stratum = min_max.second;
    if (min_max.first != min_max.second) {
      stratum_straddling_models.push_back(model);
    }
  }
  std::sort(stratum_straddling_models.begin(),
            stratum_straddling_models.end(),
            [](EquivalenceSet *a, EquivalenceSet *b) { return a->id < b->id; });

  // Reject unstratified negation: the negated view must be final before the
  // NEGATE's stratum runs, which is impossible when both share an SCC, so
  // no removal order through such a negation is well-defined.
  for (NEGATION *negate : negations) {
    if (negate->is_dead) {
      continue;
    }
    VIEW *const negated_view = negate->negated_view.get();
    if (negate->stratum != negated_view->stratum) {
      continue;
    }

    if (negate->negations.empty()) {
      log.Append() << "Negated predicate is recursively derived from the "
                      "negation's own result (unstratified negation)";
    } else {
      for (ParsedPredicate pred : negate->negations) {
        const ParsedClause clause = ParsedClause::Containing(pred);
        Error err =
            log.Append(clause.SpellingRange(), pred.SpellingRange());
        err << "Negated predicate is recursively derived from the "
               "negation's own result (unstratified negation)";
        err.Note(clause.SpellingRange(), pred.SpellingRange())
            << "The negated relation must be fully computable before any "
               "clause that negates it runs; break the recursion through "
               "the negation";
      }
    }
  }

  // Reject unstratified aggregation (the delta-relational-IR R3 sibling of the
  // negation reject above; v3-spec-statecell.md §5). An aggregate reads the
  // WHOLE of its summarized relation to fold a per-group summary value, so
  // that relation must be final before the aggregate's StateCell emits — an
  // aggregate whose summarized input shares its SCC (an aggregate over its own
  // recursive result) has no order-independent value and is rejected. The
  // summarized input is exactly the aggregate's predecessor(s) (Link.cpp binds
  // the group_by/config incoming view and the aggregated incoming view as
  // predecessors); an in-SCC aggregation is one whose predecessor sits in the
  // aggregate's own stratum. A KV index is the degenerate aggregate (group =
  // key, summary = merged value) and is stratified identically. No monotone-
  // over-monotone relaxation: aggregates are placed strictly-lower like
  // negation (spec §5 RELAXATION QUESTION).
  auto reject_in_scc_agg = [&](VIEW *agg, DisplayRange functor_range,
                               const char *kind) {
    for (VIEW *input_view : agg->predecessors) {
      if (agg->stratum != input_view->stratum) {
        continue;  // A strictly-lower summarized input: the supported shape.
      }

      Error err = log.Append(functor_range);
      err << "Aggregate is recursively derived from its own result "
             "(unstratified aggregation)";
      err.Note(functor_range)
          << "The " << kind << " relation must be fully computable before "
             "the aggregate runs; break the recursion through the aggregate";
      break;  // One diagnostic per aggregate, not one per input edge.
    }
  };

  for (AGG *agg : aggregates) {
    if (agg->is_dead) {
      continue;
    }
    reject_in_scc_agg(agg, agg->functor.SpellingRange(), "summarized");
  }

  for (KVINDEX *kv : kv_indices) {
    if (kv->is_dead) {
      continue;
    }
    const DisplayRange functor_range =
        kv->merge_functors.empty() ? DisplayRange()
                                   : kv->merge_functors.front().SpellingRange();
    reject_in_scc_agg(kv, functor_range, "keyed");
  }

#ifndef NDEBUG

  // Cross-check the SCC condensation against `IdentifyInductions` (see the
  // pass doc comment for the divergence rules).

  // Strata closed only through message publish -> receive seams are
  // invisible to IdentifyInductions; skip agreement checks inside them.
  std::vector<bool> stratum_has_io_seam(num_strata, false);
  for (const auto &[select_index, insert_index] : io_seams) {
    if (state[select_index].stratum == state[insert_index].stratum) {
      stratum_has_io_seam[state[select_index].stratum] = true;
    }
  }

  std::vector<unsigned> stratum_num_views(num_strata, 0u);
  std::vector<bool> stratum_has_inductive_merge(num_strata, false);
  for (auto i = 0u; i < num_views; ++i) {
    ++stratum_num_views[state[i].stratum];
    if (views[i]->AsMerge() && views[i]->induction_info) {
      stratum_has_inductive_merge[state[i].stratum] = true;
    }
  }

  std::unordered_map<unsigned, unsigned> stratum_to_merge_set;
  std::unordered_map<unsigned, unsigned> merge_set_to_stratum;

  for (auto i = 0u; i < num_views; ++i) {
    VIEW *const view = views[i];
    InductionInfo *const info = view->induction_info.get();
    if (!info) {
      continue;
    }

    const auto view_stratum = state[i].stratum;
    if (stratum_has_io_seam[view_stratum]) {
      continue;
    }

    // Every dataflow edge respects the topological order, and an edge is
    // masked inductive exactly when it stays inside the SCC. This is the
    // grounding of the derivation-class split: recursive (same SCC) insert
    // edges are exactly the inductive-predecessor edges.
    auto pred_i = 0u;
    for (VIEW *pred_view : info->predecessors) {
      const bool same_scc = (pred_view->stratum == view->stratum);
      const bool inductive = info->inductive_predecessors_mask[pred_i++];
      assert(inductive == same_scc);
      assert(*pred_view->stratum <= *view->stratum);
    }

    auto succ_i = 0u;
    for (VIEW *succ_view : info->successors) {
      const bool same_scc = (succ_view->stratum == view->stratum);
      const bool inductive = info->inductive_successors_mask[succ_i++];
      assert(inductive == same_scc);
      assert(*view->stratum <= *succ_view->stratum);
    }

    // `merge_set_id` (union-find over mutually reachable inductive views)
    // and SCC membership induce the same partition on info-bearing views.
    if (auto [it, added] =
            stratum_to_merge_set.emplace(view_stratum, info->merge_set_id);
        !added) {
      assert(it->second == info->merge_set_id);
    }
    if (auto [it, added] =
            merge_set_to_stratum.emplace(info->merge_set_id, view_stratum);
        !added) {
      assert(it->second == view_stratum);
    }
  }

  // All dataflow cycles pass through UNIONs, so every multi-view SCC that
  // isn't closed by an IO seam contains an inductive MERGE.
  for (auto s = 0u; s < num_strata; ++s) {
    assert(stratum_num_views[s] == 1u || stratum_has_inductive_merge[s] ||
           stratum_has_io_seam[s]);
  }

#endif  // NDEBUG
}

}  // namespace hyde
