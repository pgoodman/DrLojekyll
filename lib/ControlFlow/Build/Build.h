// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2020, Trail of Bits. All rights reserved.

#pragma once

#include <drlojekyll/DataFlow/Query.h>
#include <drlojekyll/Util/DefUse.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../Program.h"

namespace hyde {

// Equivalence classes of `QueryMerge`s. Two or more `QueryMerge`s belong to
// the same equivalence class iff their cycles intersect.
class InductionSet : public DisjointSet {
 public:
  using DisjointSet::DisjointSet;

  // The set of non-dominated merges that need to have persistent backing.
  std::vector<QueryView> merges;

  // The set of all merges that belong to this co-inductive set.
  std::vector<QueryView> all_merges;
};

class Context;
class DRFlowGraph;  // lib/ControlFlow/Build/DR.h (build-internal DR-IR).

// Generic action for lifting to the control flow representation. Work item
// actions enable us to defer some liting to the control flow representation
// until `Run` is invoked.
class WorkItem {
 public:

  // The ordering is inscrutable, but we get roughly the following behavior:
  //
  //      continuing induction group 2 depth 1
  //      continuing join group 2 depth 1
  //      continuing join group 2 depth 1
  //      continuing join group 2 depth 1
  //      continuing join group 2 depth 1
  //      continuing join group 2 depth 1
  //      continuing join group 2 depth 1
  //      continuing join group 2 depth 1
  //      finalizing induction group 2 depth 1
  //      continuing induction group 3 depth 2
  //      continuing join group 3 depth 2
  //      continuing join group 3 depth 2
  //      continuing join group 3 depth 2
  //      continuing join group 3 depth 2
  //      finalizing induction group 3 depth 2
  //      continuing induction group 0 depth 3
  //      enclosed by depth 2
  //      continuing join group 0 depth 3
  //      continuing join group 0 depth 3
  //      finalizing induction group 0 depth 3
  //      continuing induction group 1 depth 4
  //      continuing join group 1 depth 4
  //      continuing join group 1 depth 4
  //      continuing join group 1 depth 4
  //      continuing join group 1 depth 4
  //      finalizing induction group 1 depth 4
  //
  // We have induction finalizations of one depth happen before induction
  // continues of the next depth. We also do a priority inversion between
  // induction and join continues within the same group, such that the join
  // continues follow the induction continues, but precede the induction
  // finalizations. This only happens for inductive joins/products, and not
  // for normal ones.

  static constexpr unsigned kContinueJoinOrder = 0u;
  static constexpr unsigned kInductionDepthShift = 17u;
  static constexpr unsigned kContinueInductionOrder = 1u << 30;
  static constexpr unsigned kFinalizeInductionOrder = 1u << 16u;

  virtual ~WorkItem(void);
  virtual void Run(ProgramImpl *program, Context &context) = 0;

  explicit WorkItem(Context &context, unsigned order_);

  const unsigned order;
};

using WorkItemPtr = std::unique_ptr<WorkItem>;

class ContinueInductionWorkItem;
class ContinueJoinWorkItem;
class ContinueProductWorkItem;

// General wrapper around data used when lifting from the data flow to the
// control flow representation.
class Context {
 public:
  PROC *init_proc{nullptr};
  PROC *entry_proc{nullptr};

  // Maps received messages to their handler procedures.
  std::unordered_map<ParsedMessage, PROC *> messsage_handler;

  // The demand-forcing registry (the live demand transform, `-demand`;
  // recipe F2): one entry per demand-transformed bound `#query`, carrying
  // the fabricated demand-seed message and the bound-parameter binding.
  // `BuildQueryForceProcedure` consults it FIRST — a demand-transformed
  // query has no parse-level forcing predicate (`ForcingMessage()` stays
  // nullopt), so its injector is built from the registry instead of the
  // clause-var re-derivation. Null/empty unless built under `-demand`.
  const std::vector<QueryDemandForcing> *demand_forcings{nullptr};

  // Vectors that are associated with `@differential` messages backed by
  // monotone flows. We unique the contents of these at the end of the data
  // flow procedure, then iterate and publish.
  std::unordered_map<ParsedMessage, VECTOR *> publish_vecs;
  std::unordered_map<ParsedMessage, QueryView> published_view;

  // `@differential` messages backed by differential (deletion-capable)
  // flows publish through the end-of-batch COMMITSWEEP of the table backing
  // the transmit's predecessor, not through an accumulation vector.
  std::unordered_map<ParsedMessage, QueryView> commit_published_view;

  // Map `QueryMerge`s to INDUCTIONs. One or more `QueryMerge`s might map to
  // the same INDUCTION if they belong to the same "inductive set". This happens
  // when two or more `QueryMerge`s are cyclic, and their cycles intersect.
  std::unordered_map<QueryView, INDUCTION *> view_to_induction;

  // The current "pending" induction. Consider the following:
  //
  //        UNION0        UNION1
  //           \            /
  //            '-- JOIN --'
  //                  |
  //
  // In this case, we don't want UNION0 to be nest inside UNION1 or vice
  // versa, they should both "activate" at the same time. The work list
  // operates in such a way that we exhaust all JOINs before any UNIONs, so in
  // this process, we want to discover the frontiers to as many inductive UNIONs
  // as possible, so that they can all share the same INDUCTION.
  std::unordered_map<unsigned, ContinueInductionWorkItem *>
      pending_induction_action;

  // Work list of actions to invoke to build the execution tree.
  std::vector<WorkItemPtr> work_list;

  std::unordered_map<QueryView, ContinueJoinWorkItem *> view_to_join_action;
  std::unordered_map<QueryView, ContinueProductWorkItem *>
      view_to_product_action;
  std::unordered_map<QueryView, ContinueInductionWorkItem *>
      view_to_induction_action;

  // The per-table vectors of the differential batch skeleton, all owned by
  // the entry procedure and keyed by (table, vector kind):
  //
  //   kDeleteQueue / kAddQueue    rows whose explicit or derived counter
  //                               fold crossed zero, awaiting their
  //                               stratum's claim drain (delQ / addQ);
  //   kOverdeleteSet / kAdditionSet
  //                               rows claimed this batch (D / A);
  //   kNetRemovals / kNetAdditions
  //                               the consolidated signed frontiers that
  //                               higher strata's seeds range over
  //                               (D\A / A\D; a monotone boundary table
  //                               has only kNetAdditions).
  //   kClaimedDeleteFrontier / kClaimedAddFrontier
  //                               a recursive stratum's per-round CLAIMED
  //                               subset of D / A (MD §5.2/§5.3 `Δ_D`/
  //                               `Δ_A`): what the OVERDELETE/INSERT
  //                               fixpoint's firing loop ranges over, its
  //                               break tests, and RETIRE clears.
  std::unordered_map<TABLE *, std::unordered_map<unsigned, VECTOR *>>
      table_delta_vecs;

  // The MONOTONE tables that are the negated view of at least one non-@never
  // negate (D2'). A monotone negated table has no per-batch frontier machinery
  // of its own, but the negation crossover needs its GAINED keys as a
  // net-additions source, and its InI gate (Negate.cpp) needs the table sealed
  // per batch. Both are provisioned by giving such a table a kNetAdditions
  // frontier: the eager fold into it appends its crossed rows (extending the
  // monotone-boundary idiom, Build.cpp), which auto-enrolls it in the Seal
  // commit-sweep (Procedure.cpp). Populated once by `FindMonotoneNegatedTables`
  // before the eager insertion walk runs; a @never negated table is excluded
  // (it never retracts, so it has no crossover and keeps its kPresent gate).
  std::unordered_set<TABLE *> monotone_negated_tables;

  // R2 FAMILY #3: the DR-IR flow graph built by `BuildStratumPhases`, kept
  // alive so `PublishDifferentialMessageVectors` (Procedure.cpp) can LOWER the
  // commit-sweep band from the graph's kCommitSweep ops rather than re-deriving
  // it by hand. A `shared_ptr` of an incomplete type is header-legal (the
  // deleter is type-erased at the reset site in Stratum.cpp, where DR.h is in
  // scope). Null when no stratum phases ran (no differential tables).
  std::shared_ptr<DRFlowGraph> dr_flow;

  // Keyed-instance D1.b gate (HP-17 semantic-predicate staging). The keyed-
  // instance mint (`BuildSubgraphInstanceOps`) and its census recount are
  // guarded on this bit. It is DEFAULT-FALSE and set true by NO code path at
  // D1.b (no `-demand-instance` flag exists yet — it arrives at D2.b), so the
  // mint is structurally unreachable and every RecognizedSubgraphs() handle is
  // never dereferenced (the §19(K) dangling-handle hazard is sidestepped). Not
  // a debug toggle: it is a real mode bit on default-off production code.
  bool demand_instance_enabled{false};

  // §6 V-INGEST-XCHECK Site 5 (subgraphs/demand P1): the PAYLOAD each
  // `LowerIngestFold` emission actually produced, recorded at emission time
  // (the eager walk runs BEFORE the flow is built at BuildStratumPhases, so
  // the flow does not exist at walk time — the §12.6 authority shape). A
  // closing check in BuildStratumPhases compares this multiset against the
  // flow's kIngestFold op multiset (coverage + payload). The key is the R1e
  // census 5-tuple + the consumed derivation class (R-KLASS):
  // (table, sign, is_explicit, role, klass, message-id).
  struct EmittedIngestFold {
    const void *table;   // the emitted UPDATECOUNT's table (read off the node)
    int sign;            // +1 / -1
    bool is_explicit;    // the message-support-bit toggle
    uint8_t role;        // VecRole cast to its underlying type
    uint8_t klass;       // the emitted UPDATECOUNT's deriv_class (R-KLASS)
    uint64_t message;    // ParsedMessage::Id()
  };
  std::vector<EmittedIngestFold> emitted_ingest_folds;

  // V-INST-EMITTED (D2.b, HP-1): the (store_id, kind) of every keyed-instance
  // region emitted, cross-checked against the flow's {kSubgraphInstantiate,
  // kInstanceDeath, kInstanceSeal} enrollment. Enrolls ALL THREE kinds (a
  // minted-but-unlowered seal aborts).
  struct EmittedInstanceOp {
    unsigned store_id;
    uint8_t kind;  // DROpKind cast (kSubgraphInstantiate/Death/Seal)
  };
  std::vector<EmittedInstanceOp> emitted_instance_ops;

  // R1 (design §A.4/§B.2): the eager-web dispatch stream recorded at walk time
  // (the eager walk runs BEFORE the flow is built at BuildStratumPhases, so the
  // flow does not exist at walk time — the §12.6 walk-authority shape, shared
  // with emitted_ingest_folds). `BuildDRInventory`'s EAGER_WEB block iterates
  // this vector in walk (DFS) order, re-invoking the single-authority ctor per
  // record. Enough to rebuild the marker op: the view (re-invokes the ctor),
  // the target table (nullable), the sink discriminant, and the stream message.
  struct EmittedEagerOp {
    uint8_t kind;                          // DROpKind cast (kEagerForward/Insert)
    std::optional<QueryView> view;         // eager_view (re-invokes the ctor)
    TABLE *table{nullptr};                 // table_op_table (nullable)
    uint8_t sink{0};                       // EagerSink cast (kNone for forwards)
    std::optional<ParsedMessage> message;  // kEagerInsert stream sinks only
  };
  std::vector<EmittedEagerOp> emitted_eager_ops;
};

// Populate `context.monotone_negated_tables` from `query.Negations()`: the
// model table of each non-@never negate's negated view that is MONOTONE
// (differential negated tables already carry both frontiers). Called once
// before the eager insertion walk.
void FindMonotoneNegatedTables(ProgramImpl *impl, Context &context,
                               Query query);

// A table is differential when any of its member views can produce
// deletions (the codegen table-flavor rule).
bool TableIsDifferential(TABLE *table);

// The lazily created per-table delta vector of `kind` (one of the eight
// batch-skeleton kinds documented on `Context::table_delta_vecs`).
VECTOR *TableDeltaVector(ProgramImpl *impl, Context &context, TABLE *table,
                         VectorKind kind);

// Append the tuple of `view` (its columns, or an INSERT's stored input
// columns) to `vec` inside `parent`.
VECTORAPPEND *AppendViewTupleToVector(ProgramImpl *impl, REGION *parent,
                                      QueryView view, VECTOR *vec);

OP *BuildStateCheckCaseReturnFalse(ProgramImpl *impl, REGION *parent);
OP *BuildStateCheckCaseReturnTrue(ProgramImpl *impl, REGION *parent);

bool NeedsInductionCycleVector(QueryView view);
bool NeedsInductionOutputVector(QueryView view);

// The derivation class of a counter fold emitted at the current build
// position for `view`'s table: `kRecursive` while the fold is being emitted
// inside the fixpoint cycle of `view`'s induction (a back-edge arrival),
// else `kNonRecursive` (a seed/init-position arrival).
DerivClass EmissionDerivClass(ProgramImpl *impl, Context &context,
                              QueryView view);

// Emit one signed derivation-counter fold persisting `cols` into `table`.
template <typename Cols>
static UPDATECOUNT *BuildUpdateCount(ProgramImpl *impl, TABLE *table,
                                     REGION *parent, Cols &&cols, bool is_add,
                                     DerivClass deriv_class) {

  const auto fold = impl->operation_regions.CreateDerived<UPDATECOUNT>(
      parent, is_add, deriv_class, false /* is_explicit */);

  fold->table.Emplace(fold, table);
  for (auto col : cols) {
    const auto var = parent->VariableFor(impl, col);
    fold->col_values.AddUse(var);
  }

  return fold;
}

// Emit a two-way membership gate over `cols` in `table`, reading `predicate`.
template <typename Cols, typename IfMember, typename IfAbsent>
static CHECKMEMBER *
BuildCheckMember(ProgramImpl *impl, REGION *parent, TABLE *table, Cols &&cols,
                 MembershipPredicate predicate, IfMember if_member_cb,
                 IfAbsent if_absent_cb) {

  const auto check =
      impl->operation_regions.CreateDerived<CHECKMEMBER>(parent, predicate);
  for (auto col : cols) {
    const auto var = check->VariableFor(impl, col);
    check->col_values.AddUse(var);
  }

  check->table.Emplace(check, table);

  if (REGION *member_op = if_member_cb(impl, check); member_op) {
    assert(member_op->parent == check);
    check->OP::body.Emplace(check, member_op);
  }

  if (REGION *absent_op = if_absent_cb(impl, check); absent_op) {
    assert(absent_op->parent == check);
    check->absent_body.Emplace(check, absent_op);
  }

  return check;
}

// We know that `view`s data is persistently backed by `table`, and that we
// want to enumerate rows matching values available in `view_cols`. The issue
// is that `view_cols` might represent a subset of the columns actually
// in `view`, and so we need a way to "complete" the other columns before we
// can act on full rows. Thus what we will do is if we have a strict subset
// of the columns, we'll perform an index-backed table scan (this is the
// index-request path used by negation crossover joins).
//
// NOTE(pag): This mutates `available_cols` in place, so that by the time that
//            `cb` is called, `available_cols` contains all columns.
template <typename ForEachTuple>
static bool BuildMaybeScanPartial(ProgramImpl *impl, QueryView view,
                                  std::vector<QueryColumn> &view_cols,
                                  TABLE *table, SERIES *seq, ForEachTuple cb) {

  // Sort and unique out the relevant columns.
  std::sort(view_cols.begin(), view_cols.end(),
            [](QueryColumn a, QueryColumn b) {
              assert(!a.IsConstant());
              assert(!b.IsConstant());
              assert(QueryView::Containing(a) == QueryView::Containing(b));
              return *(a.Index()) < *(b.Index());
            });

  auto it = std::unique(view_cols.begin(), view_cols.end());
  view_cols.erase(it, view_cols.end());

  // Figure out if we even need an index scan; if we've got all the columns
  // then we can just use `cb`, which assumes the availability of all columns.
  const auto num_cols = view.Columns().size();
  if (view_cols.size() == num_cols) {
    const auto ret = cb(seq, false);
    if (ret) {
      assert(ret->parent == seq);
      seq->AddRegion(ret);
    }
    return false;
  }

  std::vector<unsigned> in_col_indices;
  std::vector<bool> indexed_cols(num_cols);
  for (auto view_col : view_cols) {
    const unsigned in_col_index = *(view_col.Index());
    in_col_indices.push_back(in_col_index);
    indexed_cols[in_col_index] = true;
  }

  // Figure out what columns we're selecting.
  std::vector<QueryColumn> selected_cols;
  for (auto col : view.Columns()) {
    if (!indexed_cols[*(col.Index())]) {
      selected_cols.push_back(col);
    }
  }

  assert(0u < selected_cols.size());

  TABLEINDEX *index = nullptr;
  if (!in_col_indices.empty()) {
    index = table->GetOrCreateIndex(impl, std::move(in_col_indices));
  }

  // Scan an index, using the columns from the tuple to find the columns
  // from the tuple's predecessor.
  const auto scan = impl->operation_regions.CreateDerived<TABLESCAN>(
      impl->next_id++, seq);
  seq->AddRegion(scan);
  scan->table.Emplace(scan, table);
  if (index) {
    scan->index.Emplace(scan, index);
  }

  for (QueryColumn view_col : view_cols) {
    const auto in_var = seq->VariableFor(impl, view_col);
    scan->in_vars.AddUse(in_var);
  }

  // Scans are funny. Even though we're looking into an index, we permit the
  // index to be slightly faulty, and so we double check all results.
  TUPLECMP * const cmp = impl->operation_regions.CreateDerived<TUPLECMP>(
      scan, ComparisonOperator::kEqual);
  scan->body.Emplace(scan, cmp);

  auto i = 0u;
  auto j = 0u;
  for (TABLECOLUMN *table_col : table->columns) {

    VAR *out_var = scan->out_vars.Create(
        impl->next_id++, VariableRole::kScanOutput);

    QueryColumn view_col = view.NthColumn(i++);
    out_var->query_column = view_col;

    if (indexed_cols[table_col->index]) {
      assert(index != nullptr);
      scan->in_cols.AddUse(table_col);

      VAR *in_var = scan->in_vars[j++];
      cmp->lhs_vars.AddUse(in_var);
      cmp->rhs_vars.AddUse(out_var);

    } else {
      scan->out_cols.AddUse(table_col);
    }

    // NOTE(pag): This enables later-stage "recordization" of the IR, because
    //            the relationship back to a more recently pulled tuple is more
    //            clear in descendents. However, it also prevents this variable
    //            from being replaced by `in_var`, which we know has the same value.
    cmp->col_id_to_var[view_col.Id()] = out_var;
  }

  // Mutable in place so that `cb` can observe an updates set of available
  // columns.
  view_cols.clear();
  for (auto col : view.Columns()) {
    view_cols.push_back(col);
  }

  REGION * const in_loop = cb(cmp, true);
  assert(!cmp->body);
  if (in_loop) {
    assert(in_loop->parent == cmp);
    cmp->body.Emplace(cmp, in_loop);
  }
  return true;
}

// Map `view`'s output columns to the variables that `pred_view` has in
// scope at `parent`: the standard per-edge variable plumbing used by both
// the eager insertion walk and the delta-chain walk.
void MapVariablesInEagerRegion(ProgramImpl *impl, QueryView pred_view,
                               QueryView view, OP *parent);

// Build the comparison region for a CMP view. Returns the TUPLECMP and the
// region under which the comparison-passing continuation nests (they differ
// for a not-equals, whose continuation nests in the false body of the
// equality form).
std::pair<TUPLECMP *, OP *> CreateCompareRegion(ProgramImpl *impl,
                                                QueryCompare view,
                                                Context &context,
                                                REGION *parent);

// Build the generator call for a MAP view's functor application, binding
// its bound parameters (from the data flow when `bottom_up`, from the
// output columns otherwise) and defining its free parameters.
GENERATOR *CreateGeneratorCall(ProgramImpl *impl, QueryMap view,
                               ParsedFunctor functor, Context &context,
                               REGION *parent, bool bottom_up);

// Build a join region given a JOIN view and a pivot vector. In the monotone
// form (`for_delta` is `false`) the join's body is a TUPLECMP re-checking
// the approximately-indexed scans against the pivot, and unit (condition)
// sides contribute no scan arm. In the delta form the join has no body (the
// caller wires the `added_body`/`removed_body` sections, whose emission
// re-checks scanned keys against the pivot itself) and unit sides are
// ordinary scan arms, so that the sections' per-side membership reads see
// the unit row's id; the returned TUPLECMP is null.
std::pair<TABLEJOIN *, TUPLECMP *> BuildJoin(ProgramImpl *impl,
                                             QueryJoin join_view,
                                             VECTOR *pivot_vec, SERIES *seq,
                                             bool for_delta);

// Build the per-stratum differential phases into the entry procedure: for
// each stratum in ascending order, the seed enumeration over lower strata's
// consolidated frontiers (frontier-vector loops walking delta chains, and
// dual-section TABLEJOINs fed by pivot appends), the claim drains of the
// stratum's differential tables (delete/add queues into the batch
// overdeletion/addition sets), and the net-frontier construction that
// higher strata's seeds range over.
void BuildStratumPhases(ProgramImpl *impl, Context &context, Query query);

// Build an eager region for adding data.
void BuildEagerRegion(ProgramImpl *impl, QueryView pred_view, QueryView view,
                      Context &context, OP *parent, TABLE *last_model);

// Build an eager region for a `QueryMerge` that is part of an inductive
// loop. This is interesting because we use a WorkItem as a kind of "barrier"
// to accumulate everything leading into the inductions before proceeding.
void BuildEagerInductiveRegion(ProgramImpl *impl, QueryView pred_view,
                               QueryMerge view, Context &context, OP *parent,
                               TABLE *last_model);

// Build an eager region for a `QueryMerge` that is NOT part of an inductive
// loop, and thus passes on its data to the next thing down as long as that
// data is unique.
void BuildEagerUnionRegion(ProgramImpl *impl, QueryView pred_view,
                           QueryMerge view, Context &context, OP *parent,
                           TABLE *last_model);

// Build an eager region for publishing data, or inserting it. This might end
// up passing things through if this isn't actually a message publication.
void BuildEagerInsertRegion(ProgramImpl *impl, QueryView pred_view,
                            QueryInsert view, Context &context, OP *parent,
                            TABLE *last_model);

// Build an eager region for testing the absence of some data in another view.
void BuildEagerNegateRegion(ProgramImpl *impl, QueryView pred_view,
                            QueryNegate negate, Context &context, OP *parent_,
                            TABLE *last_table_);

// Build an eager region for a join.
void BuildEagerJoinRegion(ProgramImpl *impl, QueryView pred_view,
                          QueryJoin view, Context &context, OP *parent,
                          TABLE *last_model);

// Build an eager region for cross-product.
void BuildEagerProductRegion(ProgramImpl *impl, QueryView pred_view,
                             QueryJoin view, Context &context, OP *parent,
                             TABLE *last_model);

// Build an eager region for performing a comparison.
void BuildEagerCompareRegions(ProgramImpl *impl, QueryCompare view,
                              Context &context, OP *parent);

// Build an eager region for a `QueryMap`.
void BuildEagerGenerateRegion(ProgramImpl *impl, QueryView pred_view,
                              QueryMap view, Context &context, OP *parent,
                              TABLE *last_model);

// Build an eager region for tuple. If the tuple can receive differential
// updates then its data needs to be saved.
void BuildEagerTupleRegion(ProgramImpl *impl, QueryView pred_view,
                           QueryTuple tuple, Context &context, OP *parent,
                           TABLE *last_model);

// Builds an initialization function which does any work that depends purely
// on constants.
void BuildInitProcedure(ProgramImpl *impl, Context &context, Query query);

// Build the entry data flow procedures.
PROC *BuildEntryProcedure(ProgramImpl *impl, Context &context, Query query);

// Fix the `containing_procedure` and region `parent` pointers. Useful to find
// bugs as well.
void FixupContainingProcedure(ProgramImpl *impl);

// Builds an I/O procedure, which goes and invokes the entry data flow
// procedure.
void BuildIOProcedure(ProgramImpl *impl, Query query, QueryIO io,
                      Context &context, PROC *proc);

// From the initial procedure, "extract" the primary procedure. The entry
// procedure operates on vectors from message receipt, and then does everything.
// Our goal is to split it up into two procedures:
//
//    1) The simplified entry procedure, which will only read from the
//       message vectors, do some joins perhaps, and append to induction
//       vectors / output message vectors.
//
//    2) The primary data flow procedure, which takes as input the induction
//       vectors which do the remainder of the data flow.
void ExtractPrimaryProcedure(ProgramImpl *impl, PROC *entry_proc,
                             Context &context);

// Complete a procedure by exhausting the work list.
void CompleteProcedure(ProgramImpl *impl, PROC *proc, Context &context,
                       bool add_return = true);

// Returns `true` if all paths through `region` ends with a `return` region.
inline bool EndsWithReturn(REGION *region) {
  if (region) {
    return region->EndsWithReturn();
  } else {
    return false;
  }
}

// Possibly add a counter fold into `parent` persisting the tuple of `view`
// into its table. Returns the fold (or `parent` when no fold was needed),
// the table of `view`, and the updated `already_added`.
std::tuple<OP *, TABLE *, TABLE *>
InTryInsert(ProgramImpl *impl, Context &context, QueryView view, OP *parent,
            TABLE *already_added);

// Add in all of the successors of a view inside of `parent`, which is
// usually some kind of loop. The successors execute in parallel.
void BuildEagerInsertionRegionsImpl(ProgramImpl *impl, QueryView view,
                                    Context &context, OP *parent_,
                                    const std::vector<QueryView> &successors,
                                    TABLE *last_table_);

// Add in all of the successors of a view inside of `parent`, which is
// usually some kind of loop. The successors execute in parallel.
template <typename List>
static void BuildEagerInsertionRegions(ProgramImpl *impl, QueryView view,
                                       Context &context, OP *parent_,
                                       List &&successors_, TABLE *last_table) {
  std::vector<QueryView> successors;
  for (auto succ_view : successors_) {
    successors.push_back(succ_view);
  }

  BuildEagerInsertionRegionsImpl(impl, view, context, parent_, successors,
                                 last_table);
}

}  // namespace hyde
