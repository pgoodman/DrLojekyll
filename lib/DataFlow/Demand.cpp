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
#include <drlojekyll/Parse/ModuleIterator.h>
#include <drlojekyll/Parse/Parse.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <set>
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

// The public GuardAnnotation::Kind mirrors GuardSite::Kind by VALUE (the
// STEP-7 stamp static_casts between them); a reorder/insert on either enum
// would silently mis-kind every body stamp with no diagnostic.
static_assert(
    static_cast<int>(GuardAnnotation::kReadAtTuple) ==
            static_cast<int>(GuardSite::kReadAtTuple) &&
        static_cast<int>(GuardAnnotation::kPushDown) ==
            static_cast<int>(GuardSite::kPushDown) &&
        static_cast<int>(GuardAnnotation::kBaseAtom) ==
            static_cast<int>(GuardSite::kBaseAtom),
    "GuardAnnotation::Kind must stay value-aligned with GuardSite::Kind");

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
  JOIN *const join = Mint(query->joins, "demand/guard-join");
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
        Mint(join->columns, "demand/guard-join", rc->var, rc->type, join, rc->id, col_index++);
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
        Mint(join->columns, "demand/guard-join", rc->var, rc->type, join, rc->id, col_index++);
    join->out_to_in.emplace(out, join).first->second.AddUse(rc);
    out_for_read_pos[pos] = out;
  }

  return join;
}

// Mint a restoring TUPLE re-establishing the guarded read's original column
// order (recipe N3 step 3 — the TABLE-19 shape).
static TUPLE *MintRestoringTuple(QueryImpl *query, TUPLE *read,
                                 const std::vector<COL *> &cols_for_pos) {
  TUPLE *const proj = Mint(query->tuples, "demand/guard-restore");
#ifndef NDEBUG
  proj->producer = "DEMAND-RESTORE";
#endif
  const auto width = read->columns.Size();
  for (auto pos = 0u; pos < width; ++pos) {
    COL *const rc = read->columns[pos];
    COL *const in = cols_for_pos[pos];
    proj->input_columns.AddUse(in);
    (void) Mint(proj->columns, "demand/guard-restore", rc->var, rc->type, proj, rc->id, pos);
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

const std::vector<GuardAnnotation> &Query::GuardAnnotations(
    void) const noexcept {
  return impl->guard_annotations;
}

const std::vector<RecognizedSubgraph> &Query::RecognizedSubgraphs(
    void) const noexcept {
  return impl->recognized_subgraphs;
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

bool QueryImpl::ApplyDemandTransform(
    const ParsedModule &module, const ErrorLog &log, bool demand_mode,
    bool demand_retract, bool suppress_demand,
    const std::unordered_map<VIEW *, ParsedDeclaration> &proxy_view_to_decl) {

  // DEMAND-BLIND consumer override (bin/Oracle): the definitional referee
  // evaluates the FULL closure — a pragma-activated build inside it would
  // demand-gate a graph nothing seeds and referee nothing. Overrides both
  // the flag AND the pragmas; the compiler proper never sets it.
  if (suppress_demand) {
    return true;
  }

  // ACTIVATION GATE (S1a flat-`-demand` slice). The transform runs ONLY under
  // the global `-demand` flag. A module built without the flag short-circuits
  // before any walk: nothing is minted, no module state is mutated, the
  // id-stream is untouched, and the QueryImpl graph is byte-identical — the
  // hard containment gate for the flag-off corpus.
  //
  // RP-6 flagless `@key` FORCE-OPT-IN is DEFERRED: at this greenfield tip
  // `@key` is inert parsed metadata (parse-surface rejects still fire; no
  // semantic/activation effect), and the RP-6 pragma-activation layer belongs
  // to the DIFF-R3 semantic slice (multi-adornment / keyed lowering), NOT the
  // flat-`-demand` S1a slice. So `@key` alone never activates demand here; a
  // `#local rel(...) @key(A).` module stays byte-identical unless `-demand` is
  // also passed. Under `-demand`, an `@key`'d demanded relation is still
  // reconciled by V-DECLARED-KEY below (the scan collects the pragma decls for
  // that check + the pragma-appropriate reject advice). The scan is over the
  // PARSED module (a #local whose flows were proxied away by Connect no longer
  // lives in `relations` — the decl is the durable carrier), deduped by decl
  // Id across sub-modules, decl order (deterministic).
  if (!demand_mode) {
    return true;
  }
  std::vector<ParsedDeclaration> demand_key_decls;
  {
    std::unordered_set<uint64_t> seen_decl_ids;
    for (ParsedModule sub_module : ParsedModuleIterator(module)) {
      for (ParsedLocal l : sub_module.Locals()) {
        const ParsedDeclaration d(l);
        if (d.HasInstanceKey() && seen_decl_ids.insert(d.Id()).second) {
          demand_key_decls.push_back(d);
        }
      }
      for (ParsedExport e : sub_module.Exports()) {
        const ParsedDeclaration d(e);
        if (d.HasInstanceKey() && seen_decl_ids.insert(d.Id()).second) {
          demand_key_decls.push_back(d);
        }
      }
    }
  }
  const bool pragma_activated = !demand_key_decls.empty();

  // G2: `Query::Build` is at-most-once per module instance. The fabrication
  // mutates the shared module; a re-entry would fabricate stale demand decls.
  if (module.DemandMessagesFabricated()) {
    log.Append(module.SpellingRange())
        << "Internal error: the demand transform was re-entered on a module "
        << "that already carries fabricated demand messages (Query::Build is "
        << "at-most-once per module instance)";
    return false;
  }

  // Pragma-activated programs get pragma-appropriate advice: "recompile
  // without -demand" would be the NEC-2 silent-lie (dropping the flag does
  // not deactivate an explicit @key).
  const auto reject = [&](const char *what) -> bool {
    log.Append(module.SpellingRange())
        << what
        << (pragma_activated ? "; fix or remove the @key pragma"
                             : "; recompile without -demand");
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
    // Under pure flag activation this is a benign no-op — but an explicit
    // `@key` with nothing to seed it is UNREALIZABLE, and an inert
    // pragma would be a silent lie (RP-6: unprovable/unrealizable rejects).
    if (pragma_activated) {
      const ParsedDeclaration d = demand_key_decls[0];
      const auto &d_ranges = d.InstanceKeyRanges();
      log.Append(d_ranges.empty() ? d.SpellingRange() : d_ranges[0])
          << "'" << d.NameAsString() << "' declares an instance key but no "
          << "bound #query exists to seed demand; remove the @key pragma "
          << "or add a bound query";
      return false;
    }
    return true;  // Nothing to demand-transform; a benign no-op.
  }

  if (bound_queries.size() > 1u) {
    return reject(
        "Multiple demanded (bound) queries are not yet supported under "
        "-demand");
  }

  REL *const q_rel = bound_queries[0];
  const ParsedDeclaration q_decl(q_rel->declaration);

  // The query projection is one clause / one materialization (per name+arity),
  // shared by every adornment; computed ONCE above both loops.
  if (q_rel->inserts.Size() != 1u) {
    return reject(
        "A demanded query must have exactly one materialization under "
        "-demand");
  }
  VIEW *const q_insert = q_rel->inserts[0];

  // D3.a.3 g3: one traced-but-not-yet-minted adornment. Loop 1 (Phase 1) does
  // Steps 1b+2+3 per adornment with NO minting, so Step 4 (stray-consumer) can
  // run ONCE over the pre-mint union (ADV-3); Loop 2 (Phase 2) mints Steps 5-10.
  struct PerAdornment {
    ParsedDeclaration redecl;             // the declared binding pattern
    std::vector<unsigned> bound_indices;  // Step-1 (redecl-derived)
    std::vector<unsigned> p_bound;        // the adornment as p-column positions
    TUPLE *q_read{nullptr};               // Step-2 outputs
    VIEW *q_consumer{nullptr};
    MERGE *p_merge{nullptr};
    std::vector<GuardSite> sites;         // Step-3 outputs
    std::vector<TUPLE *> pushdown_reads;
  };

  std::unordered_set<VIEW *> known_consumers;    // ADV-3 pass-level union
  std::vector<PerAdornment> plan;
  std::unordered_set<std::string> seen_variants;  // mirror Build.cpp:678

  // Loop 1 (Phase 1): Steps 1b+2+3 per adornment, NO minting. The multi-
  // adornment fence at :457 is LIFTED — this loop enumerates the adornments it
  // used to forbid. The `seen_variants` dedup mirrors Build.cpp:678:
  // `UniqueRedeclarations()` can return duplicate-pattern redecls (fabricating
  // `demand__q_<adorn>` twice hits `assert(!io_slot)`), so the old
  // `patterns`-SET belt existed; this dedup is its faithful inversion.
  for (ParsedDeclaration redecl : q_decl.UniqueRedeclarations()) {
    std::string binding(redecl.BindingPattern());
    if (!seen_variants.insert(std::move(binding)).second) {
      continue;  // duplicate redecl of an already-planned adornment.
    }

    std::vector<unsigned> bound_indices;  // Bound param indices, decl order.
    for (ParsedParameter param : redecl.Parameters()) {
      if (param.Binding() == ParameterBinding::kBound) {
        bound_indices.push_back(param.Index());
      }
    }
    if (bound_indices.empty()) {
      // An all-free SIBLING adornment of a demanded query name shares the ONE
      // query materialization (Connect.cpp) with its bound siblings, so its
      // cursor reads the demand-GUARDED pub and would silently under-answer
      // (only the demanded rows). The pre-D3.a.3 patterns.size()!=1u belt
      // rejected the whole name; keep it a clean diagnostic. (An all-free-ONLY
      // name never reaches this loop — `bound_queries` excludes it, so this
      // fires only on a bound+all-free mix.)
      return reject("A demanded query name with an all-free sibling adornment "
                    "is not yet supported under -demand");
    }

  // ---------------------------------------------------------------------
  // 2. Trace the query's projection chain to its read of p. Post-Connect the
  //    query relation keeps exactly one MATERIALIZE INSERT (Connect.cpp
  //    :275-282); its bound input column descends through forwarding TUPLEs
  //    to a full-width reader TUPLE over p's MERGE.
  // ---------------------------------------------------------------------
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
      // its single member. Connect's `CreateProxyOfInserts` gives EVERY
      // relation a MERGE, even single-clause — a documented load-bearing
      // invariant since the 2026-08-05 dead-branch deletion (this AsMerge()
      // is one of its named dependents).
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
                  "Sideways (non-From-preserving) demand propagation is not "
                  "yet supported under -demand");
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
                "Sideways (non-From-preserving) demand propagation is not yet "
                "supported under -demand");
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

    // Loop-1 tail: accumulate the pass-level consumer union, assert the
    // one-relation-p invariant, and record the traced adornment (no minting).
    known_consumers.insert(q_consumer);
    for (const GuardSite &site : sites) {
      known_consumers.insert(site.consumer);
    }
    assert(plan.empty() || p_merge == plan.front().p_merge);  // one relation p
    plan.push_back(PerAdornment{redecl, std::move(bound_indices),
                                std::move(p_bound), q_read, q_consumer, p_merge,
                                std::move(sites), std::move(pushdown_reads)});
  }  // Loop 1 (Phase 1)

  // Tier-1 naming lift: resolve the demanded relation p's declaration ONCE
  // from the Connect-time correlation map (all adornments share the one
  // p_merge, asserted above; p_merge IS the Connect `insert_proxy`). The map
  // is Build-scoped and pre-Optimize-valid HERE and nowhere later. A miss is
  // a broken compiler invariant (Step 2 only lands on post-Connect proxy
  // MERGEs on this slice) — abort loudly, never name-guess.
  const auto p_decl_it = proxy_view_to_decl.find(plan.front().p_merge);
  if (p_decl_it == proxy_view_to_decl.end()) {
    fprintf(stderr,
            "T1-DECL-MISS: demanded relation's post-Connect MERGE has no "
            "Connect-time declaration record\n");
    abort();
  }
  const ParsedDeclaration p_demanded_decl = p_decl_it->second;

  // ---------------------------------------------------------------------
  // RP-6 realization check: every `@key` relation must BE the demanded
  // target the walk located. An @key on any OTHER relation is inert-by-
  // construction (nothing demands it), and an inert pragma is a silent lie
  // — reject, anchored at the offending declaration.
  // ---------------------------------------------------------------------
  for (const ParsedDeclaration &d : demand_key_decls) {
    if (d.Id() != p_demanded_decl.Id()) {
      const auto &d_ranges = d.InstanceKeyRanges();
      log.Append(d_ranges.empty() ? d.SpellingRange() : d_ranges[0])
          << "'" << d.NameAsString() << "' declares an instance key but is not "
          << "the demanded relation ('" << p_demanded_decl.NameAsString()
          << "' is); remove the @key pragma";
      return false;
    }
  }

  // ---------------------------------------------------------------------
  // Step 2b (V-DECLARED-KEY; RES-1 placement): the declared-demand-key
  // checks, at the FIRST site the complete adornment count exists (Loop-1
  // Step-3 fences have already run per adornment — fence-first diagnostic
  // order; `demand_forcings` is still empty here, populated only in Loop 2).
  // RES-6: these are USER-DECLARATION errors, anchored at the offending
  // declaration, with NO flag/pragma-advice suffix beyond the fix itself.
  // ---------------------------------------------------------------------
  if (p_demanded_decl.HasInstanceKey()) {

    // RP-10 TOTAL BIJECTION (V-DECLARED-KEY, multi-set): the declared
    // set-of-sets must EQUAL the SIP-inferred set-of-sets, order-free. Both
    // sides are duplicate-free here (declared by the ADJ-K1-A parse dup-set
    // reject; inferred by the seen_variants BindingPattern dedup), so
    // set-of-sets equality IS a bijection. `plan.size()` is the forcing count
    // (R-1BOUND: one bound query name; demand_forcings still empty here).
    //
    // Canonical form: each set sorted into a std::vector<unsigned>; membership
    // via std::set<std::vector<unsigned>>. A key-set is rendered by column name
    // for the diagnostics (RES-6: anchored at the decl, no flag/pragma suffix
    // beyond the fix advice).
    auto canon = [](std::vector<unsigned> v) {
      std::sort(v.begin(), v.end());
      return v;
    };
    auto names = [&](const std::vector<unsigned> &s) {
      std::string out;
      auto sep = "";
      for (unsigned pos : s) {
        out += sep;
        out += p_demanded_decl.NthParameter(pos).NameAsString();
        sep = ", ";
      }
      return out;
    };

    std::set<std::vector<unsigned>> declared_sets;
    for (const InstanceKeySet &s : p_demanded_decl.InstanceKeys()) {
      declared_sets.insert(canon(s));
    }
    std::set<std::vector<unsigned>> inferred_sets;
    for (const PerAdornment &a : plan) {
      inferred_sets.insert(canon(a.p_bound));
    }

    // Arm A — a declared @key set with no matching demanded adornment
    // (over-declaration; witness key_over_adorn_1). K6-3: index-preserving
    // iteration so each set's OWN pragma range anchors the diagnostic (the
    // canonicalized `declared_sets` element carries no index; iterate the
    // parallel `InstanceKeys()`/`InstanceKeyRanges()` by index).
    const auto &decl_ranges = p_demanded_decl.InstanceKeyRanges();
    for (unsigned j = 0u; j < p_demanded_decl.InstanceKeys().size(); ++j) {
      InstanceKeySet d = canon(p_demanded_decl.InstanceKeys()[j]);
      if (!inferred_sets.count(d)) {
        log.Append(j < decl_ranges.size() ? decl_ranges[j]
                                          : p_demanded_decl.SpellingRange())
            << "Declared instance key (" << names(d) << ") on "
            << p_demanded_decl.KindName() << " '"
            << p_demanded_decl.NameAsString() << "' has no matching demanded "
            << "query adornment; fix or remove the @key pragma";
        return false;
      }
    }
    // Arm B — a demanded adornment with no matching @key set (partial
    // declaration; witness key_multi_adorn_1, repurposed).
    for (const std::vector<unsigned> &i : inferred_sets) {
      if (!declared_sets.count(i)) {
        log.Append(p_demanded_decl.SpellingRange())
            << "The demanded query adornment binding (" << names(i) << ") on "
            << p_demanded_decl.KindName() << " '"
            << p_demanded_decl.NameAsString() << "' has no matching @key "
            << "instance key; declare @key(" << names(i) << ") or remove the "
            << "@key pragma";
        return false;
      }
    }
  }

  // ---------------------------------------------------------------------
  // 4. Stray-consumer accounting (ONCE, between the loops, on the PRE-MINT
  //    graph): every reader of p must be one we traced (the query's read or a
  //    rule-body read), and every consumer of a reader must be a guard-site
  //    consumer. An untraced consumer (an all-free sibling query, another
  //    relation's rule) would read the now-pruned p and silently under-derive
  //    — the d1 §2.3 inertness terrain, un-witnessed, so reject. Both
  //    adornments demand the SAME p (one p_merge, asserted above), so the
  //    union is checked ONCE before any guard is minted (ADV-3): minting A's
  //    guards first makes them untraced consumers of the shared reader,
  //    false-rejecting B.
  // ---------------------------------------------------------------------
  {
    MERGE *const p_merge = plan.front().p_merge;
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

  // D3.a.3 g3/R-DUP: guard rewires are DEFERRED and grouped by (consumer, read).
  // Two adornments sharing one (consumer, read) (ALWAYS for N>=2 on a shared
  // reader) would double-rewire: `RewireConsumer` substitutes only `c->view ==
  // read`, so the first rewire consumes the read-uses and the second orphans its
  // guard (dead-flow-eliminated -> under-answer, HP-5). A SINGLETON group (always
  // at |plan|==1) rewires DIRECTLY (today's exact bytes); a MULTI-guard group
  // mints a MERGE union of the guards' restored (read-schema) outputs and rewires
  // the consumer ONCE (the flat-arm realization of the reference-counted-union
  // pub; the nested arm unions via band-(b)'s reference-counted publish).
  struct PendingRewire {
    VIEW *consumer;
    TUPLE *read;
    std::vector<COL *> out_for_pos;  // read-pos-indexed JOIN output columns
    JOIN *guard;
    TUPLE *restore;  // MintRestoringTuple result; nullptr for a kReadAtTuple
                     //   guard (direct rewire needs none)
    GuardSite::Kind kind;
  };
  std::vector<PendingRewire> pending;

  // Loop 2 (Phase 2): Steps 5-10 per adornment (fabricate/mint/guard/register).
  for (PerAdornment &a : plan) {
    const std::vector<unsigned> &bound_indices = a.bound_indices;
    const std::vector<unsigned> &p_bound = a.p_bound;
    MERGE *const p_merge = a.p_merge;
    TUPLE *const q_read = a.q_read;
    VIEW *const q_consumer = a.q_consumer;
    const std::vector<GuardSite> &sites = a.sites;
    const std::vector<TUPLE *> &pushdown_reads = a.pushdown_reads;

  // ---------------------------------------------------------------------
  // 5. FABRICATE the demand message + the demand relation's #local decl
  //    (Option D' / recipe A1; the single-shot flag is owned HERE, N5).
  // ---------------------------------------------------------------------
  std::string adorn;
  for (ParsedParameter param : a.redecl.Parameters()) {
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
    bound_types.push_back(a.redecl.NthParameter(bi).Type());
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
        << "prefix; rename it"
        << (pragma_activated ? " or remove the @key pragma"
                             : " or recompile without -demand");
    return false;
  }

  const auto msg_opt =
      module.FabricateDemandMessage(base_name, bound_types, demand_retract);
  if (!msg_opt) {
    log.Append(q_decl.SpellingRange())
        << "Cannot fabricate the demand message '" << base_name
        << "': a user declaration collides with the reserved demand__ "
        << "prefix; rename it"
        << (pragma_activated ? " or remove the @key pragma"
                             : " or recompile without -demand");
    return false;
  }
  const ParsedMessage d_msg = *msg_opt;

  const auto local_opt =
      module.FabricateDemandLocal(base_name + "_local", bound_types);
  if (!local_opt) {
    log.Append(q_decl.SpellingRange())
        << "Cannot fabricate the demand relation '" << base_name
        << "_local': a user declaration collides with the reserved demand__ "
        << "prefix; rename it"
        << (pragma_activated ? " or remove the @key pragma"
                             : " or recompile without -demand");
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
  IO *const d_io = Mint(ios, "demand/seed-io", d_msg_decl);
  io_slot = d_io;

  SELECT *const recv = Mint(selects, "demand/seed-receive", d_io, DisplayRange());
  d_io->receives.AddUse(recv);
#ifndef NDEBUG
  recv->producer = "DEMAND-RECEIVE";
#endif
  const auto arity = static_cast<unsigned>(bound_indices.size());
  for (auto i = 0u; i < arity; ++i) {
    (void) Mint(recv->columns, "demand/seed-receive", bound_types[i], recv, i, i);
  }

  // The demand relation object (recipe A1): registered so the graph carries
  // the fabricated decl exactly as a post-Connect #local does (its
  // inserts/selects stay EMPTY — the pass mints the readable MERGE
  // structure directly per A2 (ii)/N4; `ConnectInsertsToSelects` clears a
  // normal local's lists the same way).
  const ParsedDeclaration d_local_decl(*local_opt);
  QueryRelationImpl *&rel_slot = decl_to_relation[d_local_decl];
  assert(!rel_slot);
  rel_slot = Mint(relations, "demand/relation", d_local_decl);

  // Root member: TUPLE chain over the receive (the BuildClause+Connect
  // two-tuple shape: the clause-head TUPLE, then the member proxy).
  TUPLE *const root_head = Mint(tuples, "demand/seed-head");
#ifndef NDEBUG
  root_head->producer = "DEMAND-SEED";
#endif
  for (auto i = 0u; i < arity; ++i) {
    COL *const rc = recv->columns[i];
    root_head->input_columns.AddUse(rc);
    (void) Mint(root_head->columns, "demand/seed-head", rc->var, rc->type, root_head, rc->id, i);
  }

  TUPLE *const root_member = Mint(tuples, "demand/seed-member");
#ifndef NDEBUG
  root_member->producer = "INSERT";
#endif
  for (auto i = 0u; i < arity; ++i) {
    COL *const rc = root_head->columns[i];
    root_member->input_columns.AddUse(rc);
    (void) Mint(root_member->columns, "demand/seed-member", rc->var, rc->type, root_member, rc->id,
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
      TUPLE *const proj = Mint(tuples, "demand/prop-projection");
#ifndef NDEBUG
      proj->producer = "DEMAND-PROP";
#endif
      for (auto i = 0u; i < arity; ++i) {
        COL *const rc = read->columns[p_bound[i]];
        proj->input_columns.AddUse(rc);
        (void) Mint(proj->columns, "demand/prop-projection", rc->var, rc->type, proj, rc->id, i);
      }
      TUPLE *const prop_member = Mint(tuples, "demand/prop-member");
#ifndef NDEBUG
      prop_member->producer = "INSERT";
#endif
      for (auto i = 0u; i < arity; ++i) {
        COL *const pc = proj->columns[i];
        prop_member->input_columns.AddUse(pc);
        (void) Mint(prop_member->columns, "demand/prop-member", pc->var, pc->type, prop_member,
                                           pc->id, i);
      }
      d_members.push_back(prop_member);
    }
  }

  // The d_p MERGE (recipe A2 resolution (ii)). ALWAYS a MERGE, matching
  // Connect's shape: `CreateProxyOfInserts` gives EVERY relation a MERGE —
  // including single-clause ones (the base dumps' one-member
  // `reachable_from` UNION). Deliberate and documented load-bearing since
  // the 2026-08-05 dead-branch deletion (the recipe's N4 note to "match
  // Connect's single-insert MERGE-less behavior" read the long-dead branch;
  // the dumps are the ground truth).
  MERGE *const d_merge = Mint(merges, "demand/relation-union");
#ifndef NDEBUG
  d_merge->producer = "MERGE-INSERT";
#endif
  for (auto i = 0u; i < arity; ++i) {
    COL *const rc = root_member->columns[i];
    (void) Mint(d_merge->columns, "demand/relation-union", rc->var, rc->type, d_merge, rc->id, i);
  }
  for (VIEW *m : d_members) {
    d_merge->merged_views.AddUse(m);
  }
  VIEW *const d_top = d_merge;

  // The shared derived-d_p reader (recipe N2: a TUPLE over the pass-minted
  // MERGE is the ONLY derived-d_p read construction — no relation SELECT).
  TUPLE *const d_reader = Mint(tuples, "demand/relation-reader");
#ifndef NDEBUG
  d_reader->producer = "SELECT";
#endif
  for (auto i = 0u; i < arity; ++i) {
    COL *const dc = d_top->columns[i];
    d_reader->input_columns.AddUse(dc);
    (void) Mint(d_reader->columns, "demand/relation-reader", dc->var, dc->type, d_reader, dc->id, i);
  }

  // The forcing this pass registers at STEP 10. Exactly one in the
  // single-adornment slice (the >1-bound-query reject and the single-shot
  // re-entry guard above enforce it); recorded on every stamp so the census
  // identity `recognized_subgraphs.size() == demand_forcings.size()` is
  // grounded.
  const unsigned forcing_index = static_cast<unsigned>(demand_forcings.size());
  const unsigned first_annotation =
      static_cast<unsigned>(guard_annotations.size());

  // ---------------------------------------------------------------------
  // 7. GUARD each rule body at its located site (recipe N3).
  // ---------------------------------------------------------------------
  for (const GuardSite &site : sites) {
    std::vector<COL *> out_for_pos;
    JOIN *const guard =
        MintGuardJoin(this, site.read, d_reader, site.pivot_pos, out_for_pos);

    // D1.a stamp (pre-CSE; release-surviving): keyed on the guard JOIN —
    // the guarded read is SHARED across sites and the demand-side child may
    // CSE-fold, so neither can carry the record.
    guard->guard_annotation_index =
        static_cast<unsigned>(guard_annotations.size());
    guard->query = this;  // INV-OWN3-Q: stamped beside the index
    guard_annotations.push_back(GuardAnnotation{
        static_cast<GuardAnnotation::Kind>(site.kind),
        GuardAnnotation::kDReader, GuardAnnotation::kBody,
        false /* is_instance_key: D3 recursive-subgoal marker only */,
        site.pivot_pos, QueryView(site.read), QueryView(d_reader),
        forcing_index});

    // DEFER the rewire (R-DUP): the grouped rewire below decides direct vs
    // union. For kReadAtTuple the consumer restores order itself (no restoring
    // TUPLE); for push-down / base atom a restoring TUPLE re-establishes the
    // read's column order (the TABLE-19 / JOIN-20 restore) and is minted HERE
    // so the singleton id-stream is byte-identical to today.
    TUPLE *restore = nullptr;
    if (site.kind != GuardSite::kReadAtTuple) {
      restore = MintRestoringTuple(this, site.read, out_for_pos);
    }
    pending.push_back(PendingRewire{site.consumer, site.read, out_for_pos,
                                    guard, restore, site.kind});
  }

  // ---------------------------------------------------------------------
  // 8. THE QUERY-PROJECTION GUARD (recipe §3.7/N1): a FRESH receive
  //    projection (the TABLE-23 node — never the d_p root member; both must
  //    survive as distinct nodes) joined against the query's own read of p
  //    on the RAW seed.
  // ---------------------------------------------------------------------
  {
    TUPLE *const raw_seed = Mint(tuples, "demand/raw-seed");
#ifndef NDEBUG
    raw_seed->producer = "DEMAND-RAW-SEED";
#endif
    for (auto i = 0u; i < arity; ++i) {
      COL *const rc = recv->columns[i];
      raw_seed->input_columns.AddUse(rc);
      (void) Mint(raw_seed->columns, "demand/raw-seed", rc->var, rc->type, raw_seed, rc->id, i);
    }

    std::vector<COL *> out_for_pos;
    JOIN *const guard =
        MintGuardJoin(this, q_read, raw_seed, p_bound, out_for_pos);

    // D1.a stamp: the query-projection guard has NO GuardSite (no
    // classifier reaches it); it is stamped kReadAtTuple — the direct-read
    // shape (q_consumer reads q_read directly) — distinguished from body
    // sites by role/demand_side. demand_side is recorded PRE-CSE: on
    // non-recursive witnesses CSE folds raw_seed into d_reader (GT-3) and
    // the graph alone can no longer tell the two sides apart.
    guard->guard_annotation_index =
        static_cast<unsigned>(guard_annotations.size());
    guard->query = this;  // INV-OWN3-Q: stamped beside the index
    guard_annotations.push_back(GuardAnnotation{
        GuardAnnotation::kReadAtTuple, GuardAnnotation::kRawSeed,
        GuardAnnotation::kQueryProjection,
        false /* is_instance_key */, p_bound, QueryView(q_read),
        QueryView(raw_seed), forcing_index});

    // DEFER the rewire (R-DUP): kReadAtTuple, no restoring TUPLE. Under N>=2
    // this shares (q_consumer, q_read) with every sibling adornment's
    // query-projection guard -> the grouped rewire unions them.
    pending.push_back(PendingRewire{q_consumer, q_read, out_for_pos, guard,
                                    nullptr, GuardSite::kReadAtTuple});
  }

  // ---------------------------------------------------------------------
  // 8b. REGISTER the recognized subgraph (X-9: the recognition unit is the
  //     FORCING — one entry per DemandForcings() entry, regardless of
  //     whether the demanded body is recursive). The keyed-instance census
  //     recount reads this registry, never the DR mint loop's own output.
  // ---------------------------------------------------------------------
  {
    std::vector<unsigned> guard_indices;
    for (auto i = first_annotation;
         i < static_cast<unsigned>(guard_annotations.size()); ++i) {
      guard_indices.push_back(i);
    }
    recognized_subgraphs.push_back(
        RecognizedSubgraph{forcing_index, QueryView(p_merge), p_bound,
                           QueryView(q_insert), std::move(guard_indices),
                           p_demanded_decl});
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
  QueryDemandForcing forcing{ParsedQuery::From(a.redecl), d_msg, bound_indices};
  demand_forcings.emplace_back(std::move(forcing));

  }  // Loop 2 (Phase 2)

  // ---------------------------------------------------------------------
  // R-DUP: the grouped rewire. Group the deferred guards by (consumer, read);
  // a SINGLETON group rewires directly (today's exact call — [BYTE] for
  // |plan|==1); a MULTI-guard group mints a MERGE union of the guards' restored
  // (read-schema) outputs and rewires the consumer ONCE. guard_bf and guard_fb
  // read DISTINCT fabricated demand relations -> distinct joined_views[0] ->
  // never CSE-fold; the MERGE is a fresh sink, so the fold arm stays DORMANT.
  // ---------------------------------------------------------------------
  {
    std::vector<bool> done(pending.size(), false);
    for (size_t i = 0u; i < pending.size(); ++i) {
      if (done[i]) {
        continue;
      }
      std::vector<size_t> group;
      for (size_t j = i; j < pending.size(); ++j) {
        if (!done[j] && pending[j].consumer == pending[i].consumer &&
            pending[j].read == pending[i].read) {
          group.push_back(j);
          done[j] = true;
        }
      }

      PendingRewire &g0 = pending[group.front()];
      if (group.size() == 1u) {
        // SINGLETON (always at |plan|==1): today's exact direct rewire.
        if (g0.kind == GuardSite::kReadAtTuple) {
          RewireConsumer(g0.consumer, g0.read, g0.out_for_pos, g0.guard);
        } else {
          std::vector<COL *> restore_for_pos;
          for (COL *c : g0.restore->columns) {
            restore_for_pos.push_back(c);
          }
          RewireConsumer(g0.consumer, g0.read, restore_for_pos, g0.restore);
        }
        continue;
      }

      // MULTI-guard (R-DUP): every guard over one `read`, once restored,
      // presents READ's schema and is union-compatible. Union the read-schema
      // members and rewire the consumer once.
      std::vector<VIEW *> members;
      for (size_t idx : group) {
        PendingRewire &pr = pending[idx];
        TUPLE *member = pr.restore;
        if (!member) {  // kReadAtTuple: mint a read-schema restore over the JOIN.
          member = MintRestoringTuple(this, pr.read, pr.out_for_pos);
        }
        members.push_back(member);
      }

      MERGE *const um = Mint(merges, "demand/guard-union");
#ifndef NDEBUG
      um->producer = "DEMAND-GUARD-UNION";
#endif
      const auto width = g0.read->columns.Size();
      for (auto pos = 0u; pos < width; ++pos) {
        COL *const rc = g0.read->columns[pos];
        (void) Mint(um->columns, "demand/guard-union", rc->var, rc->type, um, rc->id, pos);
      }
      for (VIEW *m : members) {
        um->merged_views.AddUse(m);
      }
      std::vector<COL *> merge_for_pos;
      for (COL *c : um->columns) {
        merge_for_pos.push_back(c);
      }
      RewireConsumer(g0.consumer, g0.read, merge_for_pos, um);
    }
  }

  // -------------------------------------------------------------------------
  // 11. ANNOTATION CENSUS (order-free counts; ALWAYS-ON, PRE-Optimize ONLY --
  //     dead-flow elimination (the Build.cpp Optimize call) deletes annotated
  //     views outright with no orphan bucket, so the equation is sound ONLY on
  //     this freshly-stamped graph; runs exactly once, here; NEVER relocate/
  //     duplicate post-Optimize without orphan accounting. OWN-3 (ruled):
  //     promoted always-on as the D3 multi-guard precondition.
  // -------------------------------------------------------------------------
  {
    auto n_stamped = 0u;
    ForEachView([&n_stamped](VIEW *v) {
      if (v->guard_annotation_index != QueryView::kNoGuardAnnotation) {
        ++n_stamped;
      }
    });
    if (n_stamped + guard_annotation_folded_count != guard_annotations.size()) {
      fprintf(stderr,
              "OWN-3: guard-annotation census mismatch: %u live-stamped + %u "
              "folded != %zu total stamped (pre-Optimize; a stamp/fold "
              "accounting bug)\n",
              n_stamped, guard_annotation_folded_count,
              guard_annotations.size());
      abort();
    }
    if (recognized_subgraphs.size() != demand_forcings.size()) {
      fprintf(stderr,
              "OWN-3: recognized-subgraph/forcing count mismatch: %zu "
              "subgraphs != %zu forcings\n",
              recognized_subgraphs.size(), demand_forcings.size());
      abort();
    }

    // g8 BELT (D3.a.3, PRE-Optimize, STAMP-TIME): every forcing that stamped ANY
    // guard MUST have stamped >=1 kBody guard. ResolveLiveRecognition
    // (Rel.cpp:977) AND the nested recursive-content fence (Build.cpp:1452)
    // derive/gate ONLY off a role==kBody stamp; a forcing minted with zero body
    // guards resolves input_table==null -> the instance is SILENTLY skipped at
    // mint (Rel.cpp:1053) -> missing answer rows. This is the STAMP-TIME half of
    // the survivor-policy invariant. Silence chain: Step 3's one-site-per-body
    // loop (every non-tail path `return reject`, the tail `sites.push_back`) +
    // Step 7's per-site kBody mint (:1001-1006) => >=1 kBody per compiling
    // forcing, so this is silent on the corpus. HONESTY (L15): the belt is
    // STAMP-TIME (pre-Optimize; CSE runs AFTER) -> it catches a MINT bug, NOT a
    // fold regression; the fold-time teeth is PromoteSurvivorToBody (View.cpp) +
    // its directed unit. ALWAYS-ON, NDEBUG-safe.
    std::unordered_set<unsigned> forcings_with_guard, forcings_with_body;
    ForEachView([&](VIEW *v) {
      if (v->guard_annotation_index == QueryView::kNoGuardAnnotation) {
        return;
      }
      const GuardAnnotation &g = guard_annotations[v->guard_annotation_index];
      forcings_with_guard.insert(g.forcing_index);
      if (g.role == GuardAnnotation::kBody) {
        forcings_with_body.insert(g.forcing_index);
      }
    });
    for (unsigned fidx : forcings_with_guard) {
      if (!forcings_with_body.count(fidx)) {
        fprintf(stderr,
                "OWN-3/g8: forcing %u stamped guards but NONE is kBody -- "
                "ResolveLiveRecognition resolves a null input_table and "
                "silently skips the instance (a Step-7 guard-mint bug)\n",
                fidx);
        abort();
      }
    }
  }

  module.MarkDemandFabricated();
  return true;
}

}  // namespace hyde
