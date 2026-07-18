// Copyright 2026, Peter Goodman. All rights reserved.
//
// THE LIVE DEMAND TRANSFORM (magic-sets / SLDMagic as a Query-graph rewrite).
//
// Design of record:
//   docs/proposals/DemandSeeds.artifacts/d4s3-recipe.md  (Road-G; precedence
//     AMENDMENTS ROUND 3 > AMENDMENTS > body)
//   docs/proposals/DemandSeeds.artifacts/d1-demand-seed-mechanism.md (A0-A7)
//   docs/proposals/SubgraphsDemand.artifacts/p3-demand-argument.md   (§1-§2)
//
// ALGORITHM (single-adornment slice; anything outside the slice cleanly
// rejects under `-demand` — the 166-case net never runs this pass):
//
//   For the ONE bound `#query` q (bound columns = the adornment α of the
//   relation p that q projects from):
//     1. TRACE q's projection chain to its read of p (a full-width reader
//        TUPLE over p's post-Connect MERGE) — this yields α as positions in
//        p's columns.
//     2. SIP over p's MERGE members (its rule bodies): per member, locate the
//        guard site (recipe N3) —
//          - a recursive read of p whose bound cols carry α (the member's
//            bound output col traces INTO the read at α's position): the
//            guard JOINs THERE (the JOIN-18 push-down; its restored output
//            REPLACES the read as input to the body's remaining joins);
//          - no interior read of p (a base rule): the guard JOINs at the
//            body's bound-column SOURCE atom (the JOIN-20 shape).
//        A read at a different position (left-linear / a second adornment),
//        a self-join (two reads of p in one body), a NEGATE/AGG on the
//        demand path, or any un-witnessed shape → clean diagnostic.
//     3. FABRICATE (Option D', A7/G1 display-buffer naming): a real demand
//        message `demand__<q>_<α>` (its receive is the ROOT SEED) and a
//        `#local` decl for the demand relation (recipe A1/F5).
//     4. MINT the demand relation d_p: root member = TUPLE chain over the
//        fabricated receive; one propagation member per demanding subgoal
//        read (a TUPLE projecting α off that read); the d_p MERGE minted BY
//        THE PASS (recipe A2 resolution (ii) — ConnectInsertsToSelects never
//        re-runs); every derived-d_p read is a TUPLE over that MERGE (N2 —
//        no relation SELECT, no fabricated ParsedPredicate anywhere).
//     5. GUARD each member at its located site: the E-32 1→N-pivot JOIN
//        (`ApplyPositiveConditionTest` generalized — a REAL column edge,
//        never group_ids).
//     6. QUERY-PROJECTION GUARD (recipe §3.7/N1): a FRESH receive-projection
//        TUPLE (never the d_p root member) joined against q's own read of p
//        on α — the raw-seed guard (JOIN 7 / TABLE 23 in the spike).
//     7. TRIPWIRE (obligation (f), always-on): every minted d_p has ≥1
//        producing source; else fprintf+abort.
//     8. REGISTER the {query, fabricated message, bound-param binding} in the
//        demand-forcing registry (consumed by the ControlFlow injector
//        builder and by codegen's public-entry suppression — recipe F2).
//
// BEFORE (a bound query reads the FULL relation, scan-indexed on the bound
// columns — every instance of `p` materializes):
//
//     edge --> path (full closure) --> reachable_from (scan-index on From)
//
// AFTER (`-demand`): the caller's bound tuple seeds d_path via the fabricated
// message's injector; every rule body of path is guarded at its SIP site;
// the query projection is guarded on the raw seed:
//
//     demand__reachable_from_bf (fabricated msg) --> RECEIVE --+
//        root member <---------------------------------------- +
//     d_path MERGE --> d_reader TUPLE          (prop member <-- path read)
//        base body:      d_path JOIN edge_2 on From      (the JOIN-20 shape)
//        recursive body: edge_2 JOIN (d_path JOIN path)  (JOIN-22/T19/JOIN-18)
//     query projection:  raw-seed JOIN path on From      (JOIN-7 / TABLE-23)
//
// The whole body runs ONLY when `demand_mode` is true. With it false the
// function returns immediately having minted nothing and mutated no module
// state — the 166-case golden net is byte-for-bit identical (the hard
// containment gate). See d1 §6.1 / A2 for the off-mode byte-identity argument.

#include "Query.h"

#include <drlojekyll/Parse/ErrorLog.h>
#include <drlojekyll/Parse/Parse.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <vector>

namespace hyde {
namespace {

// Is `v` a full-width, positional reader TUPLE over `src` — the post-Connect
// `ProxySelects` shape (input i is exactly `src`'s column i)?
static bool IsFullWidthReaderOf(VIEW *v, VIEW *src) {
  TUPLE *const t = v->AsTuple();
  if (!t) {
    return false;
  }
  const auto width = src->columns.Size();
  if (t->input_columns.Size() != width || t->columns.Size() != width) {
    return false;
  }
  for (auto i = 0u; i < width; ++i) {
    if (t->input_columns[i] != src->columns[i]) {
      return false;
    }
  }
  return true;
}

// If `v` is a TUPLE all of whose inputs come from one message-receive SELECT,
// return that SELECT; else nullptr. (The base rule's bound-column source
// atom — the JOIN-20 guard site.)
static SELECT *AtomReceiveReadOf(VIEW *v) {
  TUPLE *const t = v->AsTuple();
  if (!t || t->input_columns.Empty()) {
    return nullptr;
  }
  SELECT *const sel = t->input_columns[0]->view->AsSelect();
  if (!sel || !sel->stream || !sel->stream->AsIO()) {
    return nullptr;
  }
  for (COL *c : t->input_columns) {
    if (c->view != sel) {
      return nullptr;
    }
  }
  return sel;
}

// One located guard site (recipe N3).
struct GuardSite {
  enum Kind {
    kReadAtTuple,  // Consumer TUPLE/INSERT directly reads the p-reader.
    kPushDown,     // Consumer JOIN reads the p-reader (recursive subgoal).
    kBaseAtom,     // Consumer TUPLE/INSERT reads a message-receive atom.
  } kind{kReadAtTuple};

  VIEW *consumer{nullptr};  // The view whose uses of `read` get rewired.
  TUPLE *read{nullptr};     // The read the guard JOIN consumes.

  // Pivot positions within `read`'s columns, one per bound column of the
  // adornment, in adornment order.
  std::vector<unsigned> pivot_pos;
};

// Mint the E-32 guard JOIN: `demand_side ⋈ read` pivoting `read`'s
// `pivot_pos` columns against `demand_side`'s columns 0..k-1. Fills
// `out_for_read_pos[j]` = the JOIN output column carrying `read`'s column j
// (pivot outputs for pivot positions, pass-throughs for the rest). The pivot
// is a REAL column edge (`out_to_in` uses BOTH inputs —
// `ApplyPositiveConditionTest`, Build.cpp:1483-1489); structural
// distinctness (num_pivots >= 1, the extra demand child) keeps the guarded
// body from CSE-collapsing onto an unguarded twin (E-32/§3(b)).
static JOIN *MintGuardJoin(QueryImpl *query, TUPLE *read, VIEW *demand_side,
                           const std::vector<unsigned> &pivot_pos,
                           std::vector<COL *> &out_for_read_pos) {
  JOIN *const join = query->joins.Create();
  join->joined_views.AddUse(demand_side);
  join->joined_views.AddUse(read);
  join->num_pivots = static_cast<unsigned>(pivot_pos.size());

#ifndef NDEBUG
  join->producer = "DEMAND-GUARD";
#endif

  const auto width = read->columns.Size();
  out_for_read_pos.assign(width, nullptr);

  auto col_index = 0u;

  // Pivot outputs first (the C7 shape: pivot columns lead). The pivot's
  // input-use order matches `joined_views` order ({demand_side, read}).
  auto k = 0u;
  for (unsigned pos : pivot_pos) {
    COL *const rc = read->columns[pos];
    COL *const out =
        join->columns.Create(rc->var, rc->type, join, rc->id, col_index++);
    auto [it, added] = join->out_to_in.emplace(out, join);
    assert(added);
    (void) added;
    it->second.AddUse(demand_side->columns[k]);
    it->second.AddUse(rc);
    out_for_read_pos[pos] = out;
    ++k;
  }

  // Non-pivot pass-throughs, in `read` column order.
  for (auto pos = 0u; pos < width; ++pos) {
    if (out_for_read_pos[pos]) {
      continue;  // A pivot.
    }
    COL *const rc = read->columns[pos];
    COL *const out =
        join->columns.Create(rc->var, rc->type, join, rc->id, col_index++);
    join->out_to_in.emplace(out, join).first->second.AddUse(rc);
    out_for_read_pos[pos] = out;
  }

  return join;
}

// Mint a restoring TUPLE re-establishing the guarded read's original column
// order (recipe N3 step 3 — the TABLE-19 shape).
static TUPLE *MintRestoringTuple(QueryImpl *query, TUPLE *read,
                                 const std::vector<COL *> &cols_for_pos) {
  TUPLE *const proj = query->tuples.Create();
#ifndef NDEBUG
  proj->producer = "DEMAND-RESTORE";
#endif
  const auto width = read->columns.Size();
  for (auto pos = 0u; pos < width; ++pos) {
    COL *const rc = read->columns[pos];
    COL *const in = cols_for_pos[pos];
    proj->input_columns.AddUse(in);
    (void) proj->columns.Create(rc->var, rc->type, proj, rc->id, pos);
  }
  return proj;
}

// Rebuild `consumer`'s uses of `read`'s columns, substituting position-wise
// via `repl_for_pos`, and (for a JOIN) swapping the `joined_views` entry to
// `replacement_view`. Targeted — the read's OTHER consumers keep their uses
// (the shared-reader shape: one reader TUPLE can feed several guard sites).
static void RewireConsumer(VIEW *consumer, TUPLE *read,
                           const std::vector<COL *> &repl_for_pos,
                           VIEW *replacement_view) {
  const auto subst = [&](COL *c) -> COL * {
    if (c->view == read) {
      return repl_for_pos[c->Index()];
    }
    return c;
  };

  if (TUPLE *t = consumer->AsTuple(); t) {
    std::vector<COL *> ins;
    for (COL *c : t->input_columns) {
      ins.push_back(subst(c));
    }
    t->input_columns.Clear();
    for (COL *c : ins) {
      t->input_columns.AddUse(c);
    }

  } else if (QueryInsertImpl *iv = consumer->AsInsert(); iv) {
    std::vector<COL *> ins;
    for (COL *c : iv->input_columns) {
      ins.push_back(subst(c));
    }
    iv->input_columns.Clear();
    for (COL *c : ins) {
      iv->input_columns.AddUse(c);
    }

  } else if (JOIN *j = consumer->AsJoin(); j) {

    // Rebuild each out column's input-use list, preserving order.
    for (COL *out : j->columns) {
      auto it = j->out_to_in.find(out);
      assert(it != j->out_to_in.end());
      std::vector<COL *> ins;
      for (COL *c : it->second) {
        ins.push_back(subst(c));
      }
      it->second.Clear();
      for (COL *c : ins) {
        it->second.AddUse(c);
      }
    }

    // Swap the `joined_views` entry, preserving order.
    std::vector<VIEW *> views;
    for (VIEW *v : j->joined_views) {
      views.push_back(v == static_cast<VIEW *>(read) ? replacement_view : v);
    }
    j->joined_views.Clear();
    for (VIEW *v : views) {
      j->joined_views.AddUse(v);
    }

  } else {
    assert(false && "unsupported demand-guard consumer shape");
  }
}

}  // namespace

const std::vector<QueryDemandForcing> &Query::DemandForcings(
    void) const noexcept {
  return impl->demand_forcings;
}

bool Query::IsDemandMessage(ParsedMessage m) const noexcept {
  for (const QueryDemandForcing &f : impl->demand_forcings) {
    if (f.message == m) {
      return true;
    }
  }
  return false;
}

// Enumerate every view in this query that uses a column of `producer` (or
// `producer` itself at the view level — a MERGE member, a JOIN input, a
// negated view). A full scan; cheap at this graph size. Used for the slice's
// stray-consumer accounting: a consumer of p we did not trace (an all-free
// sibling query, another relation's rule) makes p demand-INERT or
// un-witnessed — reject.
static std::vector<VIEW *> CollectColUsers(QueryImpl *query, VIEW *producer) {
  std::vector<VIEW *> users;
  const auto uses_col_of = [&](const UseList<COL> &cols) -> bool {
    for (COL *c : cols) {
      if (c->view == producer) {
        return true;
      }
    }
    return false;
  };

  query->ForEachView([&](VIEW *v) {
    if (v == producer) {
      return;
    }
    bool uses =
        uses_col_of(v->input_columns) || uses_col_of(v->attached_columns);

    if (JOIN *j = v->AsJoin(); j && !uses) {
      for (auto &[out, ins] : j->out_to_in) {
        (void) out;
        if (uses_col_of(ins)) {
          uses = true;
          break;
        }
      }
      if (!uses) {
        for (VIEW *jv : j->joined_views) {
          if (jv == producer) {
            uses = true;
            break;
          }
        }
      }
    }
    if (MERGE *m = v->AsMerge(); m && !uses) {
      for (VIEW *mv : m->merged_views) {
        if (mv == producer) {
          uses = true;
          break;
        }
      }
    }
    if (QueryNegateImpl *n = v->AsNegate(); n && !uses) {
      if (n->negated_view && n->negated_view.get() == producer) {
        uses = true;
      }
    }
    if (AGG *a = v->AsAggregate(); a && !uses) {
      uses = uses_col_of(a->group_by_columns) ||
             uses_col_of(a->config_columns) ||
             uses_col_of(a->aggregated_columns);
    }
    if (uses) {
      users.push_back(v);
    }
  });
  return users;
}

bool QueryImpl::ApplyDemandTransform(const ParsedModule &module,
                                     const ErrorLog &log, bool demand_mode) {

  // MODE GATE. When the `-demand` flag is off (the default), this pass is a
  // total no-op: nothing is minted, no module state is mutated, the id-stream
  // is untouched, and the QueryImpl graph is byte-identical to today. This is
  // the hard containment gate for the existing corpus.
  if (!demand_mode) {
    return true;
  }

  // G2: `Query::Build` is at-most-once per module instance. The fabrication
  // mutates the shared module; a re-entry would fabricate stale demand decls.
  if (module.DemandMessagesFabricated()) {
    log.Append(module.SpellingRange())
        << "Internal error: the demand transform was re-entered on a module "
        << "that already carries fabricated demand messages (Query::Build is "
        << "at-most-once per module instance)";
    return false;
  }

  const auto reject = [&](const char *what) -> bool {
    log.Append(module.SpellingRange())
        << what << "; recompile without -demand";
    return false;
  };

  // ---------------------------------------------------------------------
  // 1. Collect the demanded queries: bound `#query` decls. All-free queries
  //    are DEMAND-INERT (d1 §2.3) and skipped.
  // ---------------------------------------------------------------------
  std::vector<REL *> bound_queries;
  for (REL *rel : relations) {
    const ParsedDeclaration decl(rel->declaration);
    if (!decl.IsQuery() || !decl.Arity()) {
      continue;
    }
    for (ParsedParameter param : decl.Parameters()) {
      if (param.Binding() == ParameterBinding::kBound) {
        bound_queries.push_back(rel);
        break;
      }
    }
  }

  if (bound_queries.empty()) {
    return true;  // Nothing to demand-transform; a benign no-op.
  }

  if (bound_queries.size() > 1u) {
    return reject(
        "Multiple demanded (bound) queries are not yet supported under "
        "-demand");
  }

  REL *const q_rel = bound_queries[0];
  const ParsedDeclaration q_decl(q_rel->declaration);

  // MULTI-ADORNMENT reject (the FIRST belt of the adornment cross-wire fix):
  // a query NAME may be redeclared at several binding patterns (adornments)
  // sharing ONE DeclarationContext — a relation is per (name, arity), so the
  // >1-bound-query fence above does not see them. The ControlFlow entry-point
  // builder emits one entry per unique redeclaration; only the transformed
  // adornment has a valid demand seeder, and `ParsedQuery::operator==` is
  // context-keyed, so a sibling adornment could otherwise inherit the wrong
  // injector (the registry match's binding-pattern check is the second belt).
  {
    std::unordered_set<std::string> patterns;
    for (ParsedDeclaration redecl : q_decl.UniqueRedeclarations()) {
      patterns.insert(std::string(redecl.BindingPattern()));
    }
    if (patterns.size() != 1u) {
      return reject(
          "Multi-adornment demand is not yet supported (a demanded query "
          "name with more than one binding pattern) under -demand");
    }
  }

  std::vector<unsigned> bound_indices;  // Bound param indices, decl order.
  for (ParsedParameter param : q_decl.Parameters()) {
    if (param.Binding() == ParameterBinding::kBound) {
      bound_indices.push_back(param.Index());
    }
  }

  // ---------------------------------------------------------------------
  // 2. Trace the query's projection chain to its read of p. Post-Connect the
  //    query relation keeps exactly one MATERIALIZE INSERT (Connect.cpp
  //    :275-282); its bound input column descends through forwarding TUPLEs
  //    to a full-width reader TUPLE over p's MERGE.
  // ---------------------------------------------------------------------
  if (q_rel->inserts.Size() != 1u) {
    return reject(
        "A demanded query must have exactly one materialization under "
        "-demand");
  }
  VIEW *const q_insert = q_rel->inserts[0];

  VIEW *q_consumer = nullptr;     // The view whose read of p gets guarded.
  TUPLE *q_read = nullptr;        // The query's reader TUPLE over p's MERGE.
  MERGE *p_merge = nullptr;       // p's post-Connect MERGE.
  std::vector<unsigned> p_bound;  // The adornment: bound positions in p.

  for (unsigned bi : bound_indices) {
    if (bi >= q_insert->input_columns.Size()) {
      return reject("Unsupported demanded-query shape under -demand");
    }
    VIEW *parent = q_insert;
    COL *in_col = q_insert->input_columns[bi];

    for (auto steps = 0u;; ++steps) {
      if (steps > 64u) {
        return reject("Unsupported demanded-query shape under -demand");
      }
      VIEW *const pv = in_col->view;

      // The query relation's own post-Connect MERGE: descend through it into
      // its single member (Connect's `CreateProxyOfInserts` reads the
      // swapped-empty list for `has_one_insert`, so EVERY relation gets a
      // MERGE, even single-clause — the empirically real Connect shape).
      // Multiple members = multiple query clauses: un-witnessed, reject.
      if (MERGE *pm = pv->AsMerge(); pm && pm != p_merge) {
        if (pm->merged_views.Size() != 1u) {
          return reject(
              "A demanded query with multiple clauses is not yet supported "
              "under -demand");
        }
        VIEW *const only = pm->merged_views[0];
        if (in_col->Index() >= only->columns.Size()) {
          return reject("Unsupported demanded-query shape under -demand");
        }
        parent = pm;
        in_col = only->columns[in_col->Index()];
        continue;
      }

      TUPLE *const pt = pv->AsTuple();
      if (!pt) {
        return reject(
            "The demanded query must project from a single derived relation "
            "under -demand");
      }

      // Is `pt` a full-width reader over some MERGE? Then it is the read.
      if (!pt->input_columns.Empty()) {
        MERGE *const m = pt->input_columns[0]->view->AsMerge();
        if (m && IsFullWidthReaderOf(pt, m)) {
          if (p_merge && (m != p_merge || pt != q_read)) {
            return reject(
                "The demanded query's bound columns must all read one "
                "relation under -demand");
          }

          // Every bound column must reach the read through the SAME
          // consumer view: step 8 rewires exactly one consumer, so a
          // second consumer would be left reading the un-guarded read
          // (over-answering). Not reachable on today's slice (adjacent
          // rejects fire first); guarded locally so the invariant is not
          // merely emergent.
          if (q_consumer && q_consumer != parent) {
            return reject(
                "The demanded query's bound columns must share one "
                "projection under -demand");
          }
          p_merge = m;
          q_read = pt;
          q_consumer = parent;
          p_bound.push_back(in_col->Index());
          break;
        }
      }

      // A forwarding TUPLE: descend.
      if (in_col->Index() >= pt->input_columns.Size()) {
        return reject("Unsupported demanded-query shape under -demand");
      }
      parent = pt;
      in_col = pt->input_columns[in_col->Index()];
    }
  }

  assert(p_merge && q_read && q_consumer);

  // Distinct bound positions (each bound param its own p column).
  {
    std::unordered_set<unsigned> seen(p_bound.begin(), p_bound.end());
    if (seen.size() != p_bound.size()) {
      return reject(
          "The demanded query's bound columns must map to distinct relation "
          "columns under -demand");
    }
  }

  // ---------------------------------------------------------------------
  // 3. Locate the guard site of every rule body (MERGE member) of p — the
  //    recipe-N3 SIP walk. Also collect the demanding-subgoal reads (the
  //    propagation sources) and enforce the slice's honesty rejects.
  // ---------------------------------------------------------------------
  std::vector<GuardSite> sites;
  std::vector<TUPLE *> pushdown_reads;  // Demanding subgoal reads of p.

  for (VIEW *member_v : p_merge->merged_views) {
    TUPLE *const member = member_v->AsTuple();
    if (!member) {
      return reject("Unsupported rule-body shape under -demand");
    }

    // Walk the member tree: count reads of p (the self-join reject — a
    // second read would need its own propagation rule, un-witnessed; recipe
    // F6's self-join caveat) and reject NEGATE/AGG on the demand path (the
    // demand sink) and any un-witnessed view kind.
    {
      auto p_reads = 0u;
      std::vector<VIEW *> work{member};
      std::unordered_set<VIEW *> seen;
      while (!work.empty()) {
        VIEW *const v = work.back();
        work.pop_back();
        if (!seen.insert(v).second) {
          continue;
        }

        if (TUPLE *t = v->AsTuple(); t) {
          if (IsFullWidthReaderOf(t, p_merge)) {
            ++p_reads;
            continue;  // Do not descend past the read.
          }
          for (COL *c : t->input_columns) {
            work.push_back(c->view);
          }
        } else if (JOIN *j = v->AsJoin(); j) {
          for (VIEW *jv : j->joined_views) {
            work.push_back(jv);
          }
        } else if (SELECT *s = v->AsSelect(); s) {
          if (!s->stream || !s->stream->AsIO()) {
            return reject("Unsupported rule-body shape under -demand");
          }
          // A message receive: a leaf.
        } else if (v->AsNegate() || v->AsAggregate()) {
          return reject(
              "Demand does not propagate through a negation or aggregate "
              "(the demand sink); this shape is not supported under -demand");
        } else {
          return reject("Unsupported rule-body shape under -demand");
        }
      }
      if (p_reads > 1u) {
        return reject(
            "A rule body reading its own relation more than once (a "
            "self-join) is not yet supported under -demand");
      }
    }

    // Locate the guard site by tracing each bound output column.
    GuardSite site;
    bool have_site = false;

    for (auto k = 0u; k < p_bound.size(); ++k) {
      const unsigned pos = p_bound[k];
      if (pos >= member->input_columns.Size()) {
        return reject("Unsupported rule-body shape under -demand");
      }

      VIEW *parent = member;
      COL *in_col = member->input_columns[pos];
      GuardSite this_site;
      bool located = false;

      for (auto steps = 0u; !located; ++steps) {
        if (steps > 64u) {
          return reject("Unsupported rule-body shape under -demand");
        }
        VIEW *const pv = in_col->view;

        if (TUPLE *pt = pv->AsTuple(); pt) {
          if (IsFullWidthReaderOf(pt, p_merge)) {

            // The bound value comes straight off a read of p: the read is
            // the demanded subgoal; the guard joins here. The bound value
            // must sit at α's own position within the read (the
            // From-preserving check); anything else is a second adornment.
            if (in_col->Index() != pos) {
              return reject(
                  "Multi-adornment demand is not yet supported under "
                  "-demand");
            }
            this_site.kind = GuardSite::kReadAtTuple;
            this_site.consumer = parent;
            this_site.read = pt;
            this_site.pivot_pos.push_back(in_col->Index());
            located = true;

          } else if (AtomReceiveReadOf(pt)) {

            // A base body: the bound value's source atom is a message read.
            this_site.kind = GuardSite::kBaseAtom;
            this_site.consumer = parent;
            this_site.read = pt;
            this_site.pivot_pos.push_back(in_col->Index());
            located = true;

          } else {

            // A forwarding TUPLE: descend.
            if (in_col->Index() >= pt->input_columns.Size()) {
              return reject("Unsupported rule-body shape under -demand");
            }
            parent = pt;
            in_col = pt->input_columns[in_col->Index()];
          }

        } else if (JOIN *pj = pv->AsJoin(); pj) {

          // The bound value comes out of the body's join tree: find the
          // join input carrying it. A read of p supplying it at α's own
          // position is the push-down site (recipe N3 / JOIN 18).
          auto it = pj->out_to_in.find(in_col);
          if (it == pj->out_to_in.end()) {
            return reject("Unsupported rule-body shape under -demand");
          }
          TUPLE *found_read = nullptr;
          COL *found_col = nullptr;
          for (COL *u : it->second) {
            TUPLE *const ut = u->view->AsTuple();
            if (ut && IsFullWidthReaderOf(ut, p_merge)) {
              found_read = ut;
              found_col = u;
              break;
            }
          }
          if (!found_read) {
            return reject(
                "The demanded relation's bound columns do not trace to a "
                "recursive read or a source atom (unsupported demand "
                "propagation shape) under -demand");
          }
          if (found_col->Index() != pos) {
            return reject(
                "Multi-adornment demand is not yet supported under -demand");
          }
          this_site.kind = GuardSite::kPushDown;
          this_site.consumer = pj;
          this_site.read = found_read;
          this_site.pivot_pos.push_back(found_col->Index());
          located = true;

        } else {
          return reject("Unsupported rule-body shape under -demand");
        }
      }

      if (!have_site) {
        site = this_site;
        have_site = true;
      } else {
        if (site.kind != this_site.kind ||
            site.consumer != this_site.consumer ||
            site.read != this_site.read) {
          return reject(
              "The demanded relation's bound columns must share one guard "
              "site per rule body under -demand");
        }
        site.pivot_pos.push_back(this_site.pivot_pos[0]);
      }
    }

    assert(have_site);
    if (site.kind == GuardSite::kPushDown ||
        site.kind == GuardSite::kReadAtTuple) {
      pushdown_reads.push_back(site.read);
    }
    sites.push_back(site);
  }

  // ---------------------------------------------------------------------
  // 4. Stray-consumer accounting: every reader of p must be one we traced
  //    (the query's read or a rule-body read), and every consumer of a
  //    reader must be a guard-site consumer. An untraced consumer (an
  //    all-free sibling query, another relation's rule) would read the now-
  //    pruned p and silently under-derive — the d1 §2.3 inertness terrain,
  //    un-witnessed, so reject.
  // ---------------------------------------------------------------------
  {
    std::unordered_set<VIEW *> known_consumers;
    known_consumers.insert(q_consumer);
    for (const GuardSite &site : sites) {
      known_consumers.insert(site.consumer);
    }

    for (VIEW *user : CollectColUsers(this, p_merge)) {
      TUPLE *const t = user->AsTuple();
      if (!t || !IsFullWidthReaderOf(t, p_merge)) {
        return reject(
            "The demanded relation has a consumer shape not yet supported "
            "under -demand");
      }
      for (VIEW *ruser : CollectColUsers(this, t)) {
        if (!known_consumers.count(ruser)) {
          return reject(
              "The demanded relation is read by a consumer demand cannot "
              "guard (a sibling query or another rule) under -demand");
        }
      }
    }
  }

  // ---------------------------------------------------------------------
  // 5. FABRICATE the demand message + the demand relation's #local decl
  //    (Option D' / recipe A1; the single-shot flag is owned HERE, N5).
  // ---------------------------------------------------------------------
  std::string adorn;
  for (ParsedParameter param : q_decl.Parameters()) {
    adorn += (param.Binding() == ParameterBinding::kBound) ? 'b' : 'f';
  }

  // The reserved name prefix (G3). NOTE — a forced deviation from the d1
  // A7/G3 spelling `__demand_`: a LEADING underscore lexes as a VARIABLE,
  // not an atom, so the fabricated name must start lowercase. `demand__` is
  // the lexable reserved prefix; the G3 uniquing scan + clean diagnostic
  // carry over unchanged.
  std::string base_name("demand__");
  base_name += q_decl.NameAsString();
  base_name += '_';
  base_name += adorn;

  std::vector<TypeLoc> bound_types;
  for (unsigned bi : bound_indices) {
    bound_types.push_back(q_decl.NthParameter(bi).Type());
  }

  // BOTH G3 collision checks run BEFORE anything is fabricated, so a
  // collision rejects without mutating the module (no orphaned demand
  // message on the local-collision path). The primitives keep their own
  // scans as belt-and-suspenders.
  if (module.DemandFabricationWouldCollide(base_name, base_name + "_local",
                                           bound_types.size())) {
    log.Append(q_decl.SpellingRange())
        << "Cannot fabricate the demand declarations for '" << base_name
        << "': a user declaration collides with the reserved demand__ "
        << "prefix; rename it or recompile without -demand";
    return false;
  }

  const auto msg_opt = module.FabricateDemandMessage(base_name, bound_types);
  if (!msg_opt) {
    log.Append(q_decl.SpellingRange())
        << "Cannot fabricate the demand message '" << base_name
        << "': a user declaration collides with the reserved demand__ "
        << "prefix; rename it or recompile without -demand";
    return false;
  }
  const ParsedMessage d_msg = *msg_opt;

  const auto local_opt =
      module.FabricateDemandLocal(base_name + "_local", bound_types);
  if (!local_opt) {
    log.Append(q_decl.SpellingRange())
        << "Cannot fabricate the demand relation '" << base_name
        << "_local': a user declaration collides with the reserved demand__ "
        << "prefix; rename it or recompile without -demand";
    return false;
  }

  // ---------------------------------------------------------------------
  // 6. MINT the demand seed: the QueryIO + receive SELECT for the fabricated
  //    message (the pred-less stream ctor — the landed KEY SIMPLIFICATION;
  //    `BuildIOProcedure` reads only the declaration and the receive's
  //    columns, so the handler wiring fires unchanged at ControlFlow), the
  //    demand relation, and the d_p MERGE + shared reader (recipe A2 (ii)).
  // ---------------------------------------------------------------------
  const ParsedDeclaration d_msg_decl(d_msg);
  QueryIOImpl *&io_slot = decl_to_input[d_msg_decl];
  assert(!io_slot);
  IO *const d_io = ios.Create(d_msg_decl);
  io_slot = d_io;

  SELECT *const recv = selects.Create(d_io, DisplayRange());
  d_io->receives.AddUse(recv);
#ifndef NDEBUG
  recv->producer = "DEMAND-RECEIVE";
#endif
  const auto arity = static_cast<unsigned>(bound_indices.size());
  for (auto i = 0u; i < arity; ++i) {
    (void) recv->columns.Create(bound_types[i], recv, i, i);
  }

  // The demand relation object (recipe A1): registered so the graph carries
  // the fabricated decl exactly as a post-Connect #local does (its
  // inserts/selects stay EMPTY — the pass mints the readable MERGE
  // structure directly per A2 (ii)/N4; `ConnectInsertsToSelects` clears a
  // normal local's lists the same way).
  const ParsedDeclaration d_local_decl(*local_opt);
  QueryRelationImpl *&rel_slot = decl_to_relation[d_local_decl];
  assert(!rel_slot);
  rel_slot = relations.Create(d_local_decl);

  // Root member: TUPLE chain over the receive (the BuildClause+Connect
  // two-tuple shape: the clause-head TUPLE, then the member proxy).
  TUPLE *const root_head = tuples.Create();
#ifndef NDEBUG
  root_head->producer = "DEMAND-SEED";
#endif
  for (auto i = 0u; i < arity; ++i) {
    COL *const rc = recv->columns[i];
    root_head->input_columns.AddUse(rc);
    (void) root_head->columns.Create(rc->var, rc->type, root_head, rc->id, i);
  }

  TUPLE *const root_member = tuples.Create();
#ifndef NDEBUG
  root_member->producer = "INSERT";
#endif
  for (auto i = 0u; i < arity; ++i) {
    COL *const rc = root_head->columns[i];
    root_member->input_columns.AddUse(rc);
    (void) root_member->columns.Create(rc->var, rc->type, root_member, rc->id,
                                       i);
  }

  // Propagation member(s): one per demanding-subgoal read — a TUPLE
  // projecting α off THAT read (the spike's projection chain), then the
  // member proxy. Same-α duplicates share the frontier by feeding the same
  // MERGE (recipe F7: MERGE-member dedup, not CSE producer fusion).
  std::vector<VIEW *> d_members{root_member};
  {
    // NOTE: a rule-body read may be the SAME node as the query's read (the
    // post-Simplify shared-reader shape) — the propagation projection still
    // applies for its rule-body role (the spike mints it off the shared
    // reader).
    std::unordered_set<TUPLE *> seen_reads;
    for (TUPLE *read : pushdown_reads) {
      if (!seen_reads.insert(read).second) {
        continue;
      }
      TUPLE *const proj = tuples.Create();
#ifndef NDEBUG
      proj->producer = "DEMAND-PROP";
#endif
      for (auto i = 0u; i < arity; ++i) {
        COL *const rc = read->columns[p_bound[i]];
        proj->input_columns.AddUse(rc);
        (void) proj->columns.Create(rc->var, rc->type, proj, rc->id, i);
      }
      TUPLE *const prop_member = tuples.Create();
#ifndef NDEBUG
      prop_member->producer = "INSERT";
#endif
      for (auto i = 0u; i < arity; ++i) {
        COL *const pc = proj->columns[i];
        prop_member->input_columns.AddUse(pc);
        (void) prop_member->columns.Create(pc->var, pc->type, prop_member,
                                           pc->id, i);
      }
      d_members.push_back(prop_member);
    }
  }

  // The d_p MERGE (recipe A2 resolution (ii)). ALWAYS a MERGE, matching the
  // empirically real Connect shape: `CreateProxyOfInserts` computes
  // `has_one_insert` AFTER swapping the insert list empty (Connect.cpp:16),
  // so its bare-proxy single-insert branch is dead code and EVERY relation
  // gets a MERGE — including single-clause ones (the base dumps' one-member
  // `reachable_from` UNION). The recipe's N4 note to "match Connect's
  // single-insert MERGE-less behavior" reads the dead branch; the dumps are
  // the ground truth.
  MERGE *const d_merge = merges.Create();
#ifndef NDEBUG
  d_merge->producer = "MERGE-INSERT";
#endif
  for (auto i = 0u; i < arity; ++i) {
    COL *const rc = root_member->columns[i];
    (void) d_merge->columns.Create(rc->var, rc->type, d_merge, rc->id, i);
  }
  for (VIEW *m : d_members) {
    d_merge->merged_views.AddUse(m);
  }
  VIEW *const d_top = d_merge;

  // The shared derived-d_p reader (recipe N2: a TUPLE over the pass-minted
  // MERGE is the ONLY derived-d_p read construction — no relation SELECT).
  TUPLE *const d_reader = tuples.Create();
#ifndef NDEBUG
  d_reader->producer = "SELECT";
#endif
  for (auto i = 0u; i < arity; ++i) {
    COL *const dc = d_top->columns[i];
    d_reader->input_columns.AddUse(dc);
    (void) d_reader->columns.Create(dc->var, dc->type, d_reader, dc->id, i);
  }

  // ---------------------------------------------------------------------
  // 7. GUARD each rule body at its located site (recipe N3).
  // ---------------------------------------------------------------------
  for (const GuardSite &site : sites) {
    std::vector<COL *> out_for_pos;
    JOIN *const guard =
        MintGuardJoin(this, site.read, d_reader, site.pivot_pos, out_for_pos);

    if (site.kind == GuardSite::kReadAtTuple) {

      // The consumer TUPLE reads the guarded read directly: rewire it to
      // the JOIN's outputs (no restoring TUPLE — the consumer restores
      // order itself).
      RewireConsumer(site.consumer, site.read, out_for_pos, guard);

    } else {

      // Push-down / base atom: a restoring TUPLE re-establishes the read's
      // column order (the TABLE-19 / JOIN-20 restore), then the consumer's
      // uses of the read swap to it.
      TUPLE *const restore = MintRestoringTuple(this, site.read, out_for_pos);
      std::vector<COL *> restore_for_pos;
      for (COL *c : restore->columns) {
        restore_for_pos.push_back(c);
      }
      RewireConsumer(site.consumer, site.read, restore_for_pos, restore);
    }
  }

  // ---------------------------------------------------------------------
  // 8. THE QUERY-PROJECTION GUARD (recipe §3.7/N1): a FRESH receive
  //    projection (the TABLE-23 node — never the d_p root member; both must
  //    survive as distinct nodes) joined against the query's own read of p
  //    on the RAW seed.
  // ---------------------------------------------------------------------
  {
    TUPLE *const raw_seed = tuples.Create();
#ifndef NDEBUG
    raw_seed->producer = "DEMAND-RAW-SEED";
#endif
    for (auto i = 0u; i < arity; ++i) {
      COL *const rc = recv->columns[i];
      raw_seed->input_columns.AddUse(rc);
      (void) raw_seed->columns.Create(rc->var, rc->type, raw_seed, rc->id, i);
    }

    std::vector<COL *> out_for_pos;
    JOIN *const guard =
        MintGuardJoin(this, q_read, raw_seed, p_bound, out_for_pos);
    RewireConsumer(q_consumer, q_read, out_for_pos, guard);
  }

  // ---------------------------------------------------------------------
  // 9. TRIPWIRE (obligation (f), always-on under the flag): the demand
  //    relation must have a ROOT SEED — at least one of its MERGE members
  //    must trace (through its TUPLE chain) to the fabricated message's
  //    receive, or nothing can ever seed demand at runtime and the guarded
  //    relation silently under-derives. This walks the ACTUAL minted
  //    structure (not construction-order tautologies): a future edit that
  //    re-points, drops, or mis-chains the root member fires it.
  // ---------------------------------------------------------------------
  {
    bool root_reaches_receive = false;
    for (VIEW *member : d_merge->merged_views) {
      VIEW *v = member;
      for (auto steps = 0u; v && steps < 64u; ++steps) {
        if (v == static_cast<VIEW *>(recv)) {
          root_reaches_receive = true;
          break;
        }
        TUPLE *const t = v->AsTuple();
        if (!t || t->input_columns.Empty()) {
          break;
        }
        v = t->input_columns[0]->view;
      }
      if (root_reaches_receive) {
        break;
      }
    }
    if (d_io->receives.Empty() || !root_reaches_receive) {
      fprintf(stderr,
              "DEMAND-TRIPWIRE: fabricated demand relation for query '%.*s' "
              "has no root seed tracing to the fabricated receive\n",
              static_cast<int>(q_decl.NameAsString().size()),
              q_decl.NameAsString().data());
      abort();
    }
  }

  // ---------------------------------------------------------------------
  // 10. REGISTER the forcing entry (recipe F2: the ControlFlow injector
  //     builder consumes it — `ForcingMessage()` stays nullopt; codegen
  //     consumes it for the public-entry suppression) + close the
  //     single-shot fabrication window (N5).
  // ---------------------------------------------------------------------
  QueryDemandForcing forcing{ParsedQuery::From(q_decl), d_msg, bound_indices};
  demand_forcings.emplace_back(std::move(forcing));

  module.MarkDemandFabricated();
  return true;
}

}  // namespace hyde
