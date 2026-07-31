// Copyright 2026, Peter Goodman. All rights reserved.
//
// Prov -- keyset (value-containment) provenance. See Prov.h.

#include "Prov.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "Query.h"

namespace hyde {
namespace {

// Merge `src` into the sorted-unique `dst` (set union).
static void UnionInto(ProvSet &dst, const ProvSet &src) {
  if (src.empty()) {
    return;
  }
  ProvSet merged;
  merged.reserve(dst.size() + src.size());
  std::set_union(dst.begin(), dst.end(), src.begin(), src.end(),
                 std::back_inserter(merged));
  dst.swap(merged);
}

// Sorted-set intersection.
static ProvSet Intersect(const ProvSet &a, const ProvSet &b) {
  ProvSet out;
  std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                        std::back_inserter(out));
  return out;
}

// The stable in-graph source identity of a SELECT (its relation or stream).
static const void *SelectSource(QuerySelectImpl *sel) {
  if (auto rel = sel->relation.get()) {
    return static_cast<const void *>(rel);
  }
  return static_cast<const void *>(sel->stream.get());
}

// The input column feeding output column `i` of a view whose outputs are laid
// out as `input_columns ++ attached_columns` (TUPLE/CMP/MAP -- View.cpp:851).
// Returns `nullptr` if `i` is out of range (e.g. a functor-free MAP output).
static QueryColumnImpl *InputForOutput(QueryViewImpl *view, unsigned i) {
  const auto n_in = view->input_columns.Size();
  if (i < n_in) {
    return view->input_columns[i];
  }
  const auto att = i - n_in;
  if (att < view->attached_columns.Size()) {
    return view->attached_columns[att];
  }
  return nullptr;
}

}  // namespace

bool ProvContains(const ProvSet &p, const ProjKey &key) {
  return std::binary_search(p.begin(), p.end(), key);
}

ProvMap ComputeColumnProvenance(QueryImpl *query) {
  ProvMap prov;
  static const ProvSet kEmpty;

  auto get = [&](QueryColumnImpl *c) -> const ProvSet & {
    if (!c) {
      return kEmpty;
    }
    auto it = prov.find(c);
    return it == prov.end() ? kEmpty : it->second;
  };

  // Single forward pass in depth order: a view's inputs are computed before it,
  // except across an inductive back-edge (where the predecessor is bot == empty,
  // the safe under-approximation).
  const_cast<const QueryImpl *>(query)->ForEachViewInDepthOrder(
      [&](QueryViewImpl *view) {
        // SELECT -- seed each output column with its own source projection
        // (this is where a fabricated `demand__q_a` receive roots the subset
        // engine). A SELECT is a pure read (no row-narrowing), so the seed is
        // EXACT.
        if (auto sel = view->AsSelect()) {
          const void *src = SelectSource(sel);
          if (!src) {
            return;
          }
          for (auto col : sel->columns) {
            prov[col] = ProvSet{ProjKey{src, col->Index()}};
          }
          return;
        }

        // TUPLE -- a pure 1:1 forward (no narrowing), columns[i] <- input i.
        // Carrying provenance is sound (the value-set is unchanged).
        if (auto tuple = view->AsTuple()) {
          for (auto i = 0u, n = tuple->columns.Size(); i < n; ++i) {
            if (auto in = InputForOutput(tuple, i); in) {
              const auto &p = get(in);
              if (!p.empty()) {
                prov[tuple->columns[i]] = p;
              }
            }
          }
          return;
        }

        // JOIN -- a pivot output's value equals EVERY unified input (join-
        // equated), so it is contained in whatever ANY of them is contained in:
        // Prov(out) = union over out_to_in[out]. A non-pivot output has a single
        // input and is a carry. `out_to_in` gives both uniformly. A zero-pivot
        // JOIN is a @product: its outputs are independent, so leave them bot
        // (the union below is over the single product-arm input, which is a
        // carry -- still an under-approximation, but a @product column is not a
        // containment we rely on; keep it bot for clarity).
        if (auto join = view->AsJoin()) {
          if (!join->num_pivots) {
            return;  // @product -> bot
          }
          for (auto out_col : join->columns) {
            auto it = join->out_to_in.find(out_col);
            if (it == join->out_to_in.end()) {
              continue;
            }
            ProvSet s;
            for (auto in_col : it->second) {
              UnionInto(s, get(in_col));
            }
            if (!s.empty()) {
              prov[out_col] = std::move(s);
            }
          }
          return;
        }

        // MERGE (union of arms) -- out[i]'s value comes from SOME arm, so it is
        // contained only in what ALL arms guarantee: Prov(out[i]) = intersection
        // over arms of Prov(arm.columns[i]). An arm on an inductive back-edge is
        // bot, collapsing the intersection to bot (safe).
        if (auto merge = view->AsMerge()) {
          const auto n_cols = merge->columns.Size();
          bool first = true;
          std::vector<ProvSet> acc(n_cols);
          for (auto arm : merge->merged_views) {
            for (auto i = 0u; i < n_cols; ++i) {
              QueryColumnImpl *arm_col =
                  i < arm->columns.Size() ? arm->columns[i] : nullptr;
              if (first) {
                acc[i] = get(arm_col);
              } else {
                acc[i] = Intersect(acc[i], get(arm_col));
              }
            }
            first = false;
          }
          if (!first) {
            for (auto i = 0u; i < n_cols; ++i) {
              if (!acc[i].empty()) {
                prov[merge->columns[i]] = std::move(acc[i]);
              }
            }
          }
          return;
        }

        // MAP (free outputs rewrite the value), NEGATE / AGG / KVINDEX
        // (negation / regroup break containment), INSERT -> bot (leave absent).
      });

  return prov;
}

std::optional<ProjKey> ExactProjection(QueryColumnImpl *col) {
  // Walk back through pure pass-throughs to a base SELECT. A SELECT column IS
  // its source projection (equality); a TUPLE column has the SAME value-set as
  // the input feeding it (a TUPLE never narrows rows), so equality is preserved.
  // Anything else (JOIN / MERGE / CMP / MAP) may change the value-set -> not
  // exact.
  for (auto guard = 0u; col && guard < 1u << 20u; ++guard) {
    QueryViewImpl *const view = col->view;
    if (auto sel = view->AsSelect()) {
      if (const void *src = SelectSource(sel); src) {
        return ProjKey{src, col->Index()};
      }
      return std::nullopt;
    }
    if (auto tuple = view->AsTuple()) {
      QueryColumnImpl *const in = InputForOutput(tuple, col->Index());
      if (!in) {
        return std::nullopt;
      }
      col = in;
      continue;
    }
    return std::nullopt;
  }
  return std::nullopt;
}

void ValidateProvenance(QueryImpl *query, const ProvMap &prov) {
  auto has_prov = [&](QueryColumnImpl *c) -> const ProvSet * {
    auto it = prov.find(c);
    return it == prov.end() ? nullptr : &it->second;
  };

  const_cast<const QueryImpl *>(query)->ForEachView([&](QueryViewImpl *view) {
    // V-PROV-BOT: negation / regroup / product / free-MAP outputs must be bot.
    // A propagation rule that wrongly raised one of these would be an over-
    // approximation -> a recognizer miscompile. This is the anti-fold teeth.
    const bool must_be_bot = view->AsNegate() || view->AsAggregate() ||
                             view->AsKVIndex() || view->AsMap() ||
                             (view->AsJoin() && !view->AsJoin()->num_pivots);
    if (must_be_bot) {
      for (auto col : view->columns) {
        if (const ProvSet *p = has_prov(col); p && !p->empty()) {
          fprintf(stderr,
                  "V-PROV-BOT: %s output column %u carries non-bot provenance "
                  "(%zu keys) -- containment must be bot for this node kind\n",
                  view->KindName(), col->Index(), p->size());
          abort();
        }
      }
      return;
    }

    // V-PROV-PIVOT: a JOIN pivot's provenance must not FABRICATE a key -- every
    // key it carries must come from one of the joined input columns (the union
    // draws only from inputs). This is the sound, cycle-safe direction: the pass
    // is a single forward sweep, so a pivot on an inductive back-edge legitimately
    // UNDER-approximates (a back-edge input is bot when the pivot is computed) --
    // that is conservative, not a bug. The miscompile risk is the OTHER way: a
    // key claimed with no witnessing input (an over-approximation the recognizer
    // would trust). This clause is the anti-fabrication teeth.
    if (auto join = view->AsJoin(); join && join->num_pivots) {
      for (auto out_col : join->columns) {
        const ProvSet *out_p = has_prov(out_col);
        if (!out_p || out_p->empty()) {
          continue;
        }
        auto it = join->out_to_in.find(out_col);
        if (it == join->out_to_in.end() || it->second.Size() < 2u) {
          continue;  // not a pivot (single input == a carry)
        }
        ProvSet input_union;
        for (auto in_col : it->second) {
          if (const ProvSet *in_p = has_prov(in_col); in_p) {
            ProvSet merged;
            std::set_union(input_union.begin(), input_union.end(),
                           in_p->begin(), in_p->end(),
                           std::back_inserter(merged));
            input_union.swap(merged);
          }
        }
        for (const auto &k : *out_p) {
          if (!ProvContains(input_union, k)) {
            fprintf(stderr,
                    "V-PROV-PIVOT: JOIN pivot output %u carries a provenance "
                    "key with no witnessing input -- fabricated containment\n",
                    out_col->Index());
            abort();
          }
        }
      }
    }
  });
}

}  // namespace hyde
