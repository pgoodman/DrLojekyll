// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2020, Trail of Bits. All rights reserved.

#include <cstdio>
#include <cstdlib>

#include "Build.h"
#include "Rel.h"

namespace hyde {
namespace {

// Create a procedure for a view.
static void ExtendEagerProcedure(ProgramImpl *impl, QueryIO io,
                                 Context &context, PROC *proc,
                                 PARALLEL *parent) {
  const auto receives = io.Receives();
  if (receives.empty()) {
    return;
  }

  assert(io.Declaration().IsMessage());
  const auto message = ParsedMessage::From(io.Declaration());

  const auto vec =
      proc->VectorFor(impl, VectorKind::kParameter, receives[0].Columns());
  vec->added_message.emplace(message);

  VECTOR *removal_vec = nullptr;
  for (auto receive : receives) {
    if (receive.CanReceiveDeletions()) {
      removal_vec =
          proc->VectorFor(impl, VectorKind::kParameter, receive.Columns());
      removal_vec->removed_message.emplace(message);
      break;
    }
  }

  for (auto receive : receives) {

    // A deletion-capable receive: both polarities park in the receive
    // table's queues (one explicit loop per polarity, +add before -del; the
    // message handler nets the two parameter vectors first, so each surviving
    // row folds exactly one explicit crossing — the message-support bit —
    // whose zero crossing parks the row); its consumers run in the table's
    // stratum phases. P2 CUTOVER: the fold ops are the DR-IR's stage-1
    // kIngestFold pair (the same MakeStageOneIngestFolds payload the flow
    // enrolls and the census counts), lowered here — at the original walk
    // position, for id-stream identity — by LowerIngestFold (Stratum.cpp).
    if (receive.CanReceiveDeletions()) {
      assert(removal_vec != nullptr);
      DataModel *const model =
          impl->view_to_model[receive]->FindAs<DataModel>();
      for (const DROp &op :
           MakeStageOneIngestFolds(message, receive, model->table)) {
        LowerIngestFold(impl, context, op, parent,
                        (0 < op.ingest_sign) ? vec : removal_vec);
      }
      continue;
    }

    // A MONOTONE receive. §6 (subgraphs/demand P1): a table-BEARING monotone
    // fold lowers from the DR-IR at the ORIGINAL walk position (id-stream
    // identity) via LowerIngestFold — token-equivalent to the deleted
    // hand-coded fold. The returned UPDATECOUNT is the descent cursor
    // (E-34 (iii); the pre-§6 arm set `next_parent = insert`). A table-LESS
    // monotone receive mints no counter fold (its head is induction-owned; the
    // descent's InTryInsert emits the fold under an induction) — NOT an ingest
    // fold (DR.cpp). Since R-E42 it is MODELED by a kIngestLoop op (the last
    // emission surface with no DR representation retires): LowerIngestLoop
    // (Stratum.cpp) mints the VECTORLOOP shim in place.
    DataModel *const model = impl->view_to_model[receive]->FindAs<DataModel>();
    TABLE *const table = model->table;

    OP *next_parent;
    if (table) {
      const DROp op =
          MakeMonotoneIngestFold(impl, context, message, receive, table);
      next_parent = LowerIngestFold(impl, context, op, parent, vec);

      // §6 INGEST-CURSOR-SHAPE validator (subgraphs/demand P1, amended §3).
      // LowerIngestFold must return the table-bearing monotone fold's
      // UPDATECOUNT (its table == the receive's model table), NOT the
      // VECTORLOOP and NOT a fold over a different table — the descent will
      // Emplace the publish/net-additions subtree INTO fold->body, and a
      // wrong-cursor hand-off (R-CURSOR) would silently mis-parent it or abort
      // inside Emplace with no context. The idiom is Induction.cpp's
      // downcast-and-table-compare; ALWAYS-ON (fprintf+abort, survives NDEBUG).
      UPDATECOUNT *const fold = next_parent->AsUpdateCount();
      if (!fold || fold->table.get() != table) {
        std::fprintf(stderr,
                     "error: §6 INGEST-CURSOR: LowerIngestFold returned a "
                     "non-fold or wrong-table cursor for a monotone ingest\n");
        std::abort();
      }
    } else {
      // R-E42: the table-less monotone receive is NOW modeled — a kIngestLoop
      // op, lowered IN PLACE by LowerIngestLoop (the byte-move of the old
      // hand-minted shim, minting 1+arity ids at THIS walk position and
      // returning the VECTORLOOP as the descent cursor). The op's provenance
      // replaces "no op"; the emission is unchanged.
      const DROp op = MakeIngestLoopOp(message, receive);
      next_parent = LowerIngestLoop(impl, context, op, parent, vec);

      // §6 INGEST-LOOP-SHAPE validator (R-E42; symmetry with the Arm-B
      // INGEST-CURSOR-SHAPE guard above). LowerIngestLoop must return a
      // VECTORLOOP over the message add-vector — the descent will Emplace the
      // insertion subtree INTO loop->body, and a wrong-cursor hand-off would
      // silently mis-parent it or abort inside Emplace with no context.
      // ALWAYS-ON (fprintf+abort, survives NDEBUG).
      VECTORLOOP *const chk = next_parent->AsVectorLoop();
      if (!chk || chk->vector.get() != vec) {
        std::fprintf(stderr,
                     "error: §6 INGEST-LOOP-SHAPE: LowerIngestLoop returned a "
                     "non-loop or wrong-vector cursor for a table-less "
                     "monotone ingest\n");
        std::abort();
      }
    }

    BuildEagerInsertionRegions(impl, receive, context, next_parent,
                               receive.Successors(), table);
  }
}

struct CompareVectors {
 public:
  inline bool operator()(VECTOR *a, VECTOR *b) const noexcept {
    return a->id < b->id;
  }
};

// Classifies usage of a vector into "read" or "written" (or both) by `region`.
static void ClassifyVector(VECTOR *vec, REGION *region,
                           std::set<VECTOR *, CompareVectors> &read,
                           std::set<VECTOR *, CompareVectors> &written) {
  if (region->AsInduction()) {
    read.insert(vec);

  } else if (auto op = region->AsOperation(); op) {
    switch (op->op) {
      case ProgramOperation::kAppendQueryParamsToMessageInjectVector:
      case ProgramOperation::kAppendToInductionVector:
      case ProgramOperation::kClearInductionVector:
      case ProgramOperation::kAppendUnionInputToVector:
      case ProgramOperation::kClearUnionInputVector:
      case ProgramOperation::kAppendJoinPivotsToVector:
      case ProgramOperation::kClearJoinPivotVector:
      case ProgramOperation::kAppendToProductInputVector:
      case ProgramOperation::kClearProductInputVector:
      case ProgramOperation::kScanTable:
      case ProgramOperation::kClearScanVector:
      case ProgramOperation::kAppendToMessageOutputVector:
      case ProgramOperation::kClearMessageOutputVector:
        written.insert(vec);
        break;

      // TODO(pag): Should we bother considering these to be reads?
      case ProgramOperation::kSwapInductionVector:
      case ProgramOperation::kSortAndUniqueInductionVector:
      case ProgramOperation::kSortAndUniquePivotVector:
      case ProgramOperation::kSortAndUniqueProductInputVector:
      case ProgramOperation::kSortAndUniqueMessageOutputVector:
      case ProgramOperation::kNetBatchVectors:
        read.insert(vec);
        written.insert(vec);
        break;

      case ProgramOperation::kLoopOverInductionVector:
      case ProgramOperation::kLoopOverUnionInputVector:
      case ProgramOperation::kJoinTables:
      case ProgramOperation::kCrossProduct:
      case ProgramOperation::kLoopOverScanVector:
      case ProgramOperation::kLoopOverInputVector:
      case ProgramOperation::kLoopOverMessageOutputVector:
        read.insert(vec);
        break;

      // R3 GROUP_UPDATE: it DRAINS the input net frontiers (read) and APPENDS
      // the emit_touched one-net-pair to the agg table's del/add queues
      // (write). Classify by which of the op's vecs `vec` matches.
      case ProgramOperation::kGroupUpdate: {
        auto *gu = op->AsGroupUpdate();
        if (vec == gu->neg_frontier.get() || vec == gu->pos_frontier.get()) {
          read.insert(vec);
        }
        if (vec == gu->del_queue.get() || vec == gu->add_queue.get()) {
          written.insert(vec);
        }
        break;
      }

      // D2.b SUBGRAPH_INSTANCE: it DRAINS the demand net-additions frontier
      // (read) — the birth keys. No vector writes (it publishes into the pub
      // table + the instance store, not a vector).
      case ProgramOperation::kSubgraphInstance: {
        auto *si = op->AsSubgraphInstance();
        if (vec == si->demand_frontier.get()) {
          read.insert(vec);
        }
        if (vec == si->input_frontier.get()) {
          read.insert(vec);  // [R-REBUILD-a2] band-(a2) drains it (read-only)
        }
        if (vec == si->input_removal_frontier.get()) {
          read.insert(vec);  // [D3.a.2 a2'] input net-removals drain (read-only)
        }
        if (vec == si->removal_frontier.get()) {
          read.insert(vec);   // D3.a.1 band-(a0) death drain (read-only)
        }
        if (vec == si->del_queue.get() || vec == si->add_queue.get()) {
          written.insert(vec);  // D3.a.1 band-(b) signed publish appends
        }
        break;
      }

      default: assert(false);
    }
  // Parameter; by construction, neither the entry nor the primary procedures
  // have inout parameters.
  } else if (region->AsProcedure()) {
    read.insert(vec);

  } else {
    assert(false);
  }
}

// Create vectors for each published message that is marked as `@differential`.
// We de-duplicate these, then check that they actually are added/removed (as
// that can change over the course of some iterations), then publish.
static void CreateDifferentialMessageVectors(
    ProgramImpl *impl, Context &context, Query query, PROC *proc) {
  for (auto io : query.IOs()) {
    const auto transmits = io.Transmits();
    if (!transmits.empty()) {
      const auto insert = QueryInsert::From(transmits[0]);
      assert(insert.IsStream());
      assert(transmits[0].AllColumnsOfSinglePredecessorAreUsed());

      // In the data flow representation, as a final step, we enforce that every
      // INSERT is preceded by a TUPLE, and the TUPLE passes exactly the inputs
      // needed by the INSERT, and only them, and in that order.
      auto pred = transmits[0].Predecessors()[0];
      assert(pred.IsTuple());

      const auto message = ParsedMessage::From(io.Declaration());
      assert(message.IsPublished());

      if (message.IsDifferential()) {

        // A deletion-capable flow publishes net presence changes through
        // the end-of-batch commit sweep of the table backing the
        // transmit's predecessor.
        if (QueryView(transmits[0]).CanReceiveDeletions()) {
          context.commit_published_view.emplace(message, transmits[0]);

        // A monotone flow accumulates rows into a vector that is
        // sort-uniqued and published at the end of the batch.
        } else {
          context.publish_vecs[message] = proc->VectorFor(
              impl, VectorKind::kMessageOutputs, pred.Columns());
          context.published_view.emplace(message, insert);
        }
      }
    }
  }
}

// D2.b/D3.a.1 keyed-instance lowering: build ONE SUBGRAPHINSTANCE region per
// kSubgraphInstantiate op, realizing the THREE-OP store protocol
// {death?, instantiate, seal} as one region whose internal band order
// (a0 death -> a1 birth -> a2 rebuild -> b publish -> Seal) is textual and
// unreorderable (R-1: the death is the seal's head-mirror — a DR op with no
// region of its own, self-lowered at the region HEAD). Runs in the
// (pre-split) flow proc so ExtractPrimaryProcedure threads the frontier
// vectors automatically. A kInstanceDeath exists only for a DIFFERENTIAL
// demand table (P-DEATH, the `-demand-retract` channel); under R-MONO the
// death band and the removal_frontier member stay absent.
static void LowerSubgraphInstances(ProgramImpl *impl, Context &context,
                                   const DRFlowGraph &dr_flow, SERIES *seq) {

  // D3.a.1 (M1): the death ops by store id. V-INST-PAIR guarantees at most
  // one death per sid, so a plain map is total.
  std::unordered_map<unsigned, const DROp *> death_by_sid;
  for (const DROp *dop : dr_flow.OpsOfKind(DROpKind::kInstanceDeath)) {
    death_by_sid.emplace(dop->instance_store_id, dop);
  }

  for (const DROp *op : dr_flow.SubgraphInstances()) {
    const unsigned sid = op->instance_store_id;
    if (sid >= dr_flow.instances.size()) {
      continue;
    }
    const DRInstance &inst = dr_flow.instances[sid];

    // V-INST-DIFF-COHERENCE: the stamped bit must equal the live
    // TableIsDifferential(pub) authority (the InstantiateEffects fork,
    // Rel.cpp). A future edit that desyncs the mint stamp from the live
    // predicate aborts here. Always-on; survives NDEBUG.
    if (inst.differential != TableIsDifferential(op->table_op_table)) {
      std::fprintf(stderr,
                   "error: SUBGRAPHINSTANCE store %u: stamped differential=%d "
                   "!= TableIsDifferential(pub)=%d\n",
                   sid, inst.differential,
                   TableIsDifferential(op->table_op_table));
      std::abort();
    }

    // Orphan-mint fence [ALWAYS-ON] (Fable review [B], the A2.6 symmetry):
    // under the differential regime the demand kNetAdditions frontier is a
    // commit-band product that must ALREADY be memoized here — and the
    // V-INST-DRAIN regime split no longer checks the ControlFlow-side vector
    // for a differential demand (XC-3), so a mint-on-miss below would hand
    // band-(a1) a producer-less always-empty frontier: zero births, silently.
    // (Monotone regime: the eager boundary append provisioned it, and the
    // validator's cf_ok arm still checks it — no fence needed.)
    if (inst.differential &&
        !HasTableDeltaVector(context, op->demand_table,
                             VectorKind::kNetAdditions)) {
      std::fprintf(stderr,
                   "error: orphan-mint fence: band-(a1) demand frontier not "
                   "pre-minted (store %u)\n", sid);
      std::abort();
    }
    VECTOR *const demand_front =
        TableDeltaVector(impl, context, op->demand_table,
                         VectorKind::kNetAdditions);
    // [R-REBUILD-a2] the memoized input net-additions frontier.
    //  - MONOTONE input: the eager cut-successor boundary append
    //    (Build.cpp:1110-1114) minted it during the walk — the mint-on-miss
    //    fetch resolves it and provisions nothing new.
    //  - DIFFERENTIAL input (input_diff): the eager append was SKIPPED
    //    (Build.cpp:1110 `!TableIsDifferential`), so both frontiers are
    //    commit-band products minted by LowerDRFlow's frontier-filter lowering
    //    BEFORE this pass (F-b1-3). A mint-on-miss here would hand band-(a2) a
    //    producer-less always-empty frontier — the silent-orphan hazard the
    //    fence catches (A2.6 idiom).
    const bool input_diff =
        op->input_table && TableIsDifferential(op->input_table);
    if (input_diff &&
        (!HasTableDeltaVector(context, op->input_table,
                              VectorKind::kNetAdditions) ||
         !HasTableDeltaVector(context, op->input_table,
                              VectorKind::kNetRemovals))) {
      std::fprintf(stderr,
                   "error: orphan-mint fence: differential input +/- frontier "
                   "not pre-minted (store %u)\n", sid);
      std::abort();
    }
    VECTOR *const input_front =
        TableDeltaVector(impl, context, op->input_table,
                         VectorKind::kNetAdditions);
    VECTOR *const input_removal_front =
        input_diff ? TableDeltaVector(impl, context, op->input_table,
                                      VectorKind::kNetRemovals)
                   : nullptr;

    SUBGRAPHINSTANCE *const si =
        impl->operation_regions.CreateDerived<SUBGRAPHINSTANCE>(
            seq, sid, inst.differential);
    seq->AddRegion(si);
    si->demand_frontier.Emplace(si, demand_front);
    si->input_frontier.Emplace(si, input_front);  // [R-REBUILD-a2]
    if (input_removal_front) {
      si->input_removal_frontier.Emplace(si, input_removal_front);  // a2'
    }

    // V-INST-INPUT-COHERENCE [ALWAYS-ON]: input_removal_frontier present iff
    // the summarized input is @differential — the invariant codegen's a2'/
    // Present selector relies on. Within THIS scope the equality is a
    // construction tautology (both sides derive from `input_diff` above); the
    // LIVE fence is the orphan-mint check at the band head. Kept as an
    // executable invariant statement (the V-INST-DIFF-COHERENCE mold).
    if ((si->input_removal_frontier.get() != nullptr) != input_diff) {
      std::fprintf(stderr,
                   "error: SUBGRAPHINSTANCE store %u: input_removal_frontier "
                   "presence (%d) != TableIsDifferential(input)=%d\n",
                   sid, si->input_removal_frontier.get() != nullptr,
                   input_diff);
      std::abort();
    }

    // D3.a.1: under the differential regime the band publishes SIGNED deltas
    // into pub's own machinery (OQ-PUBLISH-ORDER) — fetch pub's memoized
    // delete/add queues. Monotone stores keep null refs (TryAdd publish).
    if (inst.differential) {
      // Queue orphan-mint fence [ALWAYS-ON, §3.3 / A2.6 idiom]: the
      // (pub, kDeleteQueue/kAddQueue) entries must ALREADY be memoized
      // (minted by the step-1 claim-drain lowering) — TableDeltaVector mints
      // on miss, which would silently hand the band drainless orphan vectors
      // (+ id churn) for a differential pub the claim-drain mint skipped.
      {
        if (!HasTableDeltaVector(context, op->table_op_table,
                                 VectorKind::kDeleteQueue) ||
            !HasTableDeltaVector(context, op->table_op_table,
                                 VectorKind::kAddQueue)) {
          std::fprintf(stderr,
                       "error: orphan-mint fence: pub delete/add queue not "
                       "pre-minted (store %u)\n", sid);
          std::abort();
        }
      }
      si->del_queue.Emplace(
          si, TableDeltaVector(impl, context, op->table_op_table,
                               VectorKind::kDeleteQueue));
      si->add_queue.Emplace(
          si, TableDeltaVector(impl, context, op->table_op_table,
                               VectorKind::kAddQueue));
      // E8d (R-3): the band-(a2) demand-liveness gate probes the demand
      // table's Present membership. Hash/Equals untouched — like the queues,
      // demand_table is a pure function of the Equals key (V-INST-SOLE +
      // V-INST-PAIR pin one demand table per store).
      si->demand_table.Emplace(si, op->demand_table);
    }

    // D3.a.1 (M1/M2): the death band's drain source — the demand table's
    // NETTED net-removals frontier (the commit-band frontier-filter product;
    // the memoized fetch returns the SAME VECTOR that filter's lowering
    // already minted). Null under R-MONO: the band and the member stay
    // absent, and the emitter keys on presence (D-2 — the death is keyed by
    // the OP, never by the region diff bit; d2 ruling).
    if (auto dit = death_by_sid.find(sid); dit != death_by_sid.end()) {
      const DROp *const death = dit->second;
      // V-INST-DEATH-COHERENCE [ALWAYS-ON]: the death op must name the SAME
      // demand/pub tables as its instantiate — a drifted mint would drain the
      // wrong table's frontier or retract into the wrong pub. fprintf+abort,
      // survives NDEBUG (the V-INST-DIFF-COHERENCE mold).
      if (death->demand_table != op->demand_table ||
          death->table_op_table != op->table_op_table) {
        std::fprintf(stderr,
                     "error: SUBGRAPHINSTANCE store %u: kInstanceDeath tables "
                     "(demand/pub) disagree with its kSubgraphInstantiate\n",
                     sid);
        std::abort();
      }
      // A2.6 orphan-mint fence [ALWAYS-ON]: the (demand, kNetRemovals) entry
      // must ALREADY be memoized (minted by the step-1 EmitFrontierFilter
      // lowering) — TableDeltaVector mints on miss, which would silently
      // hand the death drain a producer-less orphan vector.
      {
        if (!HasTableDeltaVector(context, death->demand_table,
                                 VectorKind::kNetRemovals)) {
          std::fprintf(stderr,
                       "error: orphan-mint fence: death drain vector not "
                       "pre-minted (store %u)\n", sid);
          std::abort();
        }
      }
      si->removal_frontier.Emplace(
          si, TableDeltaVector(impl, context, death->demand_table,
                               VectorKind::kNetRemovals));
      context.emitted_instance_ops.push_back(
          {sid, static_cast<uint8_t>(DROpKind::kInstanceDeath)});
    }

    si->input_table.Emplace(si, op->input_table);
    si->pub_table.Emplace(si, op->table_op_table);  // HP-3: pub rides op_table
    si->key_positions = inst.key_cols;
    si->row_positions = inst.row_cols;
    // input_key_cols = the section-walk bound cols (the input columns equal to
    // the instance key), carried on the op's rescan spine (§3.2c).
    if (!op->arms.empty() && op->arms[0].body &&
        op->arms[0].body->kind == PlanKind::kAccess) {
      si->input_key_cols = op->arms[0].body->bound_cols;
    }

    // Band-(a2) keys FindInstance on input_key_cols; a shorter list would
    // aggregate-init the missing Key components to ZERO and compile — a
    // silently-wrong probe that re-opens edge-after-demand (the R-a2
    // Fable-review latent). Every recognized shape today binds the full
    // key; a future shape that doesn't must extend the plumbing, not
    // truncate the probe. ALWAYS-ON (fprintf+abort, survives NDEBUG).
    if (si->input_key_cols.size() != inst.key_cols.size()) {
      std::fprintf(stderr,
                   "error: SUBGRAPHINSTANCE store %u: rescan-spine bound "
                   "cols (%zu) != instance key arity (%zu)\n",
                   sid, si->input_key_cols.size(), inst.key_cols.size());
      std::abort();
    }
    // input_row_cols = the remaining input columns (in order) — the published
    // row payload (the single-monotone-hop shape: input = key ++ row cols).
    {
      std::unordered_set<unsigned> keyset(si->input_key_cols.begin(),
                                          si->input_key_cols.end());
      const unsigned arity =
          static_cast<unsigned>(op->input_table->columns.Size());
      for (unsigned c = 0u; c < arity; ++c) {
        if (!keyset.count(c)) {
          si->input_row_cols.push_back(c);
        }
      }
    }

    // V-INST-EMITTED (HP-1): enroll the instantiate AND the self-lowered seal
    // (the death, when present, enrolled above at its wiring site — the
    // three-op protocol {death?, instantiate, seal} is emitted by this ONE
    // region).
    context.emitted_instance_ops.push_back(
        {sid, static_cast<uint8_t>(DROpKind::kSubgraphInstantiate)});
    context.emitted_instance_ops.push_back(
        {sid, static_cast<uint8_t>(DROpKind::kInstanceSeal)});
  }
}

static void PublishDifferentialMessageVectors(ProgramImpl *impl, PROC *proc,
                                              Context &context) {

  // Place the body inside of a sequence.
  const auto seq = impl->series_regions.Create(proc);
  proc->body->parent = seq;
  seq->AddRegion(proc->body.get());
  proc->body.Emplace(proc, seq);

  // The first thing in the sequence will be a PARALLEL region for iterating
  // over the vectors to publish.
  const auto iter_par = impl->parallel_regions.Create(seq);
  seq->AddRegion(iter_par);

  for (auto [message, vec] : context.publish_vecs) {
    if (!vec) {
      continue;
    }

    const auto sub_seq = impl->series_regions.Create(iter_par);
    iter_par->AddRegion(sub_seq);

    VECTORUNIQUE *const sort =
        impl->operation_regions.CreateDerived<VECTORUNIQUE>(
            sub_seq, ProgramOperation::kSortAndUniqueMessageOutputVector);
    sort->vector.Emplace(sort, vec);
    sub_seq->AddRegion(sort);

    const QueryView view = context.published_view.find(message)->second;
    const QueryInsert insert = QueryInsert::From(view);

    // Create the vector loop over the publish vector.
    VECTORLOOP *const iter = impl->operation_regions.CreateDerived<VECTORLOOP>(
        impl->next_id++, sub_seq,
        ProgramOperation::kLoopOverMessageOutputVector);
    sub_seq->AddRegion(iter);
    iter->vector.Emplace(iter, vec);

    // Add in variable bindings.
    for (auto col : insert.InputColumns()) {
      const auto var = iter->defined_vars.Create(impl->next_id++,
                                                 VariableRole::kMessageOutput);

      var->query_column = col;
      if (col.IsConstantOrConstantRef()) {
        var->query_const = QueryConstant::From(col);
      }

      iter->col_id_to_var[col.Id()] = var;
    }

    // No flow into this transmit can produce deletions, so every vector row
    // is a fresh derivation from this epoch's eager insertion path: publish
    // each row as an addition.
    PUBLISH *const publish_add =
        impl->operation_regions.CreateDerived<PUBLISH>(
            iter, message, impl->next_id++,
            ProgramOperation::kPublishMessage);
    iter->body.Emplace(iter, publish_add);

    for (auto var : iter->defined_vars) {
      publish_add->arg_vars.AddUse(var);
    }

    // Finally, clear the vector; we're done.
    VECTORCLEAR *const clear =
        impl->operation_regions.CreateDerived<VECTORCLEAR>(
            sub_seq, ProgramOperation::kClearMessageOutputVector);
    sub_seq->AddRegion(clear);
    clear->vector.Emplace(clear, vec);
  }

  // The end-of-batch commit sweeps: one per differential table (sealing the
  // batch-start snapshot, clearing the batch-scratch flags, publishing net
  // presence changes for a `@differential`-transmit-backing table), one Seal per
  // monotone boundary table. R2 FAMILY #3: LOWERED from the DR-IR flow graph's
  // kCommitSweep ops (stashed on `context.dr_flow` by `BuildStratumPhases`),
  // replacing the hand-coded `impl->tables` loop that used to live here. When no
  // stratum phases ran (no differential tables), the graph is null and there are
  // no sweeps to emit.
  if (context.dr_flow) {
    // D2.b: emit the keyed-instance regions BEFORE the commit sweeps (band-(b)
    // publish precedes the store Seal; the frontier-table Seals are independent).
    LowerSubgraphInstances(impl, context, *context.dr_flow, seq);
    LowerCommitSweeps(impl, context, *context.dr_flow, seq);

    // V-INST-EMITTED (HP-1, the V-INGEST-XCHECK Site-5 mold): the (store_id,
    // kind) multiset of EMITTED instance regions must equal the flow's
    // {kSubgraphInstantiate, kInstanceDeath, kInstanceSeal} enrollment — a
    // minted-but-unlowered op (esp. the seal) aborts.
    {
      using Key = std::pair<unsigned, uint8_t>;
      std::vector<Key> emitted, enrolled;
      for (const auto &e : context.emitted_instance_ops) {
        emitted.emplace_back(e.store_id, e.kind);
      }
      for (const DROp &op : context.dr_flow->ops) {
        if (op.kind == DROpKind::kSubgraphInstantiate ||
            op.kind == DROpKind::kInstanceDeath ||
            op.kind == DROpKind::kInstanceSeal) {
          enrolled.emplace_back(op.instance_store_id,
                                static_cast<uint8_t>(op.kind));
        }
      }
      std::sort(emitted.begin(), emitted.end());
      std::sort(enrolled.begin(), enrolled.end());
      if (emitted != enrolled) {
        std::fprintf(stderr,
                     "error: V-INST-EMITTED failed: the emitted instance "
                     "regions' (store_id, kind) multiset disagrees with the "
                     "flow's instance-op enrollment (%zu emitted vs %zu "
                     "enrolled)\n",
                     emitted.size(), enrolled.size());
        std::abort();
      }
    }
  }

  // Finally, return from the data flow procedure.
  seq->AddRegion(impl->operation_regions.CreateDerived<RETURN>(
      seq, ProgramOperation::kReturnTrueFromProcedure));
}

// Recursively fix a region's containing procedure.
static void FixupContainingProcedure(REGION *region, REGION *parent) {
  if (!region) {
    return;
  }

  assert(region->parent == parent);
  region->cached_depth = 0;
  region->parent = parent;
  region->containing_procedure = parent->containing_procedure;

  if (auto op = region->AsOperation(); op) {
    if (auto gen = op->AsGenerate(); gen) {
      FixupContainingProcedure(gen->empty_body.get(), region);

    } else if (auto call = op->AsCall(); call) {
      FixupContainingProcedure(call->false_body.get(), region);

    } else if (auto check = op->AsCheckMember(); check) {
      FixupContainingProcedure(check->absent_body.get(), region);

    } else if (auto get = op->AsCheckRecord(); get) {
      FixupContainingProcedure(get->absent_body.get(), region);

    } else if (auto join = op->AsTableJoin(); join) {
      FixupContainingProcedure(join->added_body.get(), region);
      FixupContainingProcedure(join->removed_body.get(), region);

    } else if (auto cmp = op->AsTupleCompare(); cmp) {
      FixupContainingProcedure(cmp->false_body.get(), region);
    }

    FixupContainingProcedure(op->body.get(), region);

  } else if (auto induction = region->AsInduction(); induction) {
    FixupContainingProcedure(induction->init_region.get(), region);
    FixupContainingProcedure(induction->cyclic_region.get(), region);
    FixupContainingProcedure(induction->output_region.get(), region);

  } else if (auto par = region->AsParallel(); par) {
    for (auto sub_region : par->regions) {
      FixupContainingProcedure(sub_region, region);
    }
  } else if (auto series = region->AsSeries(); series) {
    for (auto sub_region : series->regions) {
      FixupContainingProcedure(sub_region, series);
    }
  }
}

}  // namespace

void FixupContainingProcedure(ProgramImpl *impl) {
  for (auto proc : impl->procedure_regions) {
    proc->containing_procedure = proc;
    proc->parent = proc;
    FixupContainingProcedure(proc->body.get(), proc);
  }
}

// Builds an I/O procedure, which goes and invokes the entry data flow
// procedure.
void BuildIOProcedure(ProgramImpl *impl, Query query, QueryIO io,
                      Context &context, PROC *proc) {
  const auto receives = io.Receives();
  if (receives.empty()) {
    return;
  }

  assert(io.Declaration().IsMessage());
  const auto message = ParsedMessage::From(io.Declaration());

  const auto io_proc = impl->procedure_regions.Create(
      impl->next_id++, ProcedureKind::kMessageHandler);
  io_proc->io = io;

  // Record for later if we have to internally inject a message from a
  // query's forcing procedure.
  context.messsage_handler.emplace(message, io_proc);

  const auto io_vec =
      io_proc->VectorFor(impl, VectorKind::kParameter, receives[0].Columns());
  io_vec->added_message.emplace(message);

  VECTOR *io_remove_vec = nullptr;
  if (message.IsDifferential()) {
    io_remove_vec =
        io_proc->VectorFor(impl, VectorKind::kParameter, receives[0].Columns());
    io_remove_vec->removed_message.emplace(message);
  }

  auto seq = impl->series_regions.Create(io_proc);
  io_proc->body.Emplace(io_proc, seq);

  // A differential message's explicit adds are netted against its explicit
  // removes before the data flow runs (one received batch is one epoch): a
  // row present in both vectors with net zero leaves no trace, and a row
  // netting positive or negative survives in exactly one vector.
  if (io_remove_vec != nullptr) {
    NETBATCH *const net = impl->operation_regions.CreateDerived<NETBATCH>(seq);
    net->add_vector.Emplace(net, io_vec);
    net->remove_vector.Emplace(net, io_remove_vec);
    seq->AddRegion(net);
  }

  auto call =
      impl->operation_regions.CreateDerived<CALL>(impl->next_id++, seq, proc);
  seq->AddRegion(call);

  auto ret = impl->operation_regions.CreateDerived<RETURN>(
      seq, ProgramOperation::kReturnTrueFromProcedure);
  seq->AddRegion(ret);

  for (auto other_io : query.IOs()) {
    const auto other_receives = other_io.Receives();
    if (other_receives.empty()) {
      continue;
    }

    // Pass in our input vector for additions, and possibly our input vector
    // for removals.
    if (io == other_io) {
      call->arg_vecs.AddUse(io_vec);
      if (io_remove_vec) {
        call->arg_vecs.AddUse(io_remove_vec);
      }

    // Pass in the empty vector once or twice for other messages.
    } else {
      const auto empty_vec = io_proc->VectorFor(impl, VectorKind::kEmpty,
                                                other_receives[0].Columns());
      call->arg_vecs.AddUse(empty_vec);
      if (other_receives[0].CanReceiveDeletions()) {
        call->arg_vecs.AddUse(empty_vec);
      }
    }
  }
}

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
void ExtractPrimaryProcedure(ProgramImpl *impl, PROC *entry_proc, Context &) {
  const auto primary_proc = impl->procedure_regions.Create(
      impl->next_id++, ProcedureKind::kPrimaryDataFlowFunc);

  std::vector<REGION *> regions_to_extract;
  std::unordered_set<REGION *> seen;

  // First, go find the regions leading to the uses of the message vectors.
  // We go up to the enclosing inductions so that we can also capture things
  // like JOINs that will happen before those inductions.
  for (auto message_vec : entry_proc->input_vecs) {
    message_vec->ForEachUse<REGION>([&](REGION *region, VECTOR *) {
      if (auto [it, added] = seen.insert(region); added) {
        regions_to_extract.push_back(region);
      }
    });
  }

  // Add the discovered regions into the entry function, replacing them with
  // LET expressions.
  auto entry_seq = impl->series_regions.Create(entry_proc);
  auto entry_par = impl->parallel_regions.Create(entry_seq);
  entry_seq->AddRegion(entry_par);

  // NOTE: a message vector can be legitimately unused: when dataflow
  // optimization proves that a message's received data can never flow
  // anywhere, the RECEIVE stays as the message's external interface, and its
  // vector is consumed by no region.

  for (auto region : regions_to_extract) {
    auto let = impl->operation_regions.CreateDerived<LET>(region->parent);
    region->ReplaceAllUsesWith(let);
    region->parent = entry_par;
    entry_par->AddRegion(region);
  }

  // Re-root the entry function body into the primary function, and link in the
  // extracted stuff into the entry body.
  entry_proc->body->parent = primary_proc;
  primary_proc->body.Swap(entry_proc->body);
  entry_proc->body.Emplace(entry_proc, entry_seq);

  // Now, go figure out which vectors are logically read and written by the
  // two procedures, so we can split them up. Our goal is to build up the
  // list of arguments that we need to pass into the primary function from
  // the entry function.
  std::set<VECTOR *, CompareVectors> read_by_entry;
  std::set<VECTOR *, CompareVectors> written_by_entry;
  std::set<VECTOR *, CompareVectors> read_by_primary;
  std::set<VECTOR *, CompareVectors> written_by_primary;

  for (auto vec : entry_proc->vectors) {
    vec->ForEachUse<REGION>([&](REGION *region, VECTOR *) {
      auto region_proc = region->Ancestor()->AsProcedure();
      assert(region_proc != nullptr);

      if (region_proc == entry_proc) {
        ClassifyVector(vec, region, read_by_entry, written_by_entry);

      } else if (region_proc == primary_proc) {
        ClassifyVector(vec, region, read_by_primary, written_by_primary);

      } else {
        assert(false);
      }
    });
  }

  std::vector<VECTOR *> primary_params;

  // The parameters we need are written by `entry` and `read` by `primary`.
  std::set_intersection(written_by_entry.begin(), written_by_entry.end(),
                        read_by_primary.begin(), read_by_primary.end(),
                        std::back_inserter(primary_params), CompareVectors());

  // Create the mapping between the vectors that need to be updated in the
  // primary data flow function that still point at the old function.
  std::unordered_map<VECTOR *, VECTOR *> replacements;

  for (auto vec : primary_params) {
    replacements[vec] = primary_proc->input_vecs.Create(vec);
  }

  for (auto vec : read_by_primary) {
    if (!replacements.count(vec)) {
      replacements[vec] = primary_proc->vectors.Create(vec);
    }
  }

  for (auto vec : written_by_primary) {
    if (!replacements.count(vec)) {
      replacements[vec] = primary_proc->vectors.Create(vec);
    }
  }

  for (auto vec : written_by_entry) {
    if (!replacements.count(vec)) {
      replacements[vec] = primary_proc->vectors.Create(vec);
    }
  }

  for (auto [old_vec, new_vec] : replacements) {
    old_vec->ReplaceUsesWithIf<REGION>(new_vec, [=](REGION *user, VECTOR *) {
      return user->Ancestor() == primary_proc;
    });
  }

  // Garbage collect the unneeded vectors from the entry proc.
  entry_proc->vectors.RemoveUnused();

  std::unordered_set<unsigned> needed_vecs;
  for (VECTOR *vec : primary_params) {
    needed_vecs.insert(vec->id);
  }

  // Try to clear out an uneeded vector.
  auto try_clear_vec = [&] (VECTOR *vec) {
    if (!needed_vecs.count(vec->id)) {
      VECTORCLEAR * const clear =
          impl->operation_regions.CreateDerived<VECTORCLEAR>(
              entry_seq,
              ProgramOperation::kClearVectorBeforePrimaryFlowFunction);
      entry_seq->AddRegion(clear);
      clear->vector.Emplace(clear, vec);
    }
  };

  // Go clear the memory of unneeded vectors prior to calling the primary
  // dataflow procedure.
  for (VECTOR *vec : entry_proc->input_vecs) {
    try_clear_vec(vec);
  }
  for (VECTOR *vec : entry_proc->vectors) {
    try_clear_vec(vec);
  }

  // Call the dataflow proc from the entry proc.
  auto call = impl->operation_regions.CreateDerived<CALL>(
      impl->next_id++, entry_seq, primary_proc,
      ProgramOperation::kCallProcedure);
  entry_seq->AddRegion(call);

  for (auto vec : primary_params) {
    call->arg_vecs.AddUse(vec);
  }

  // Terminate the entry proc.
  entry_seq->AddRegion(impl->operation_regions.CreateDerived<RETURN>(
      entry_seq, ProgramOperation::kReturnFalseFromProcedure));

  FixupContainingProcedure(impl);
}

// Build the primary and entry data flow procedures.
PROC *BuildEntryProcedure(ProgramImpl *impl, Context &context, Query query) {

  assert(context.work_list.empty());
  assert(context.view_to_join_action.empty());
  assert(context.view_to_product_action.empty());
  assert(context.view_to_induction_action.empty());

  const auto proc = impl->procedure_regions.Create(
      impl->next_id++, ProcedureKind::kEntryDataFlowFunc);

  context.entry_proc = proc;
  context.work_list.clear();

  //  context.view_to_work_item.clear();
  //  context.view_to_induction.clear();
  //  context.product_vector.clear();

  const auto proc_par = impl->parallel_regions.Create(proc);

  CreateDifferentialMessageVectors(impl, context, query, proc);

  // First, build up the initialization code for all constants.
  {
    const auto uncond_inserts_var =
        impl->global_vars.Create(impl->next_id++, VariableRole::kInitGuard);

    // Test that we haven't yet done an initialization: `(guard += 1) == 1`.
    const auto test_and_set = impl->operation_regions.CreateDerived<TESTANDSET>(
        proc_par, ProgramOperation::kTestAndAdd);
    proc_par->AddRegion(test_and_set);
    test_and_set->accumulator.Emplace(test_and_set, uncond_inserts_var);

    const auto init_par = impl->parallel_regions.Create(test_and_set);
    test_and_set->body.Emplace(test_and_set, init_par);

    // Go find all TUPLEs whose inputs are constants. We ignore constant refs,
    // as those are dataflow dependent.
    //
    // NOTE(pag): The dataflow builder ensures that TUPLEs are the only node types
    //            that can take all constants.
    for (auto tuple : impl->query.Tuples()) {
      const QueryView view(tuple);

      // The ONE all-constant spelling, shared with the DR derivation's
      // Root 1 (IsAllConstantTupleDR — the flip's SD-1 one-authority
      // pattern; a divergence here is an SD-4 abort on every compile).
      if (!IsAllConstantTupleDR(tuple)) {
        continue;
      }

      const auto let = impl->operation_regions.CreateDerived<LET>(init_par);
      init_par->AddRegion(let);

      // Add variable mappings.
      view.ForEachUse([&](QueryColumn in_col, InputColumnRole,
                          std::optional<QueryColumn> out_col) {
        const auto const_var = let->VariableFor(impl, in_col);
        if (out_col) {
          let->col_id_to_var[out_col->Id()] = const_var;
        }
      });

      BuildEagerRegion(impl, view, view, context, let, nullptr);
    }
  }

  for (auto io : query.IOs()) {
    const auto par = impl->parallel_regions.Create(proc);
    proc->body.Emplace(proc, par);
    ExtendEagerProcedure(impl, io, context, proc, par);

    auto curr_body = proc->body.get();
    proc->body.Clear();
    curr_body->parent = proc_par;
    proc_par->AddRegion(curr_body);
  }

  // TODO(pag): I think I have half-fixed the bug described below. Basically,
  //            I think I've "fixed" it for the first "level" of inductions,
  //            but none of the subsequent levels of inductions. It's possible
  //            that we'll need to break out work lists to separate joins and
  //            such, so that I can do this type of fixing up in phases.
  //
  // TODO(pag): Possible future bug lies here. So, right now we group everything
  //            into one PARALLEL, `par`, then build out from there. But maybe
  //            the right approach is to place them into independent parallel
  //            nodes, then somehow merge them. I think this will be critical
  //            when there are more than one message being received. Comment
  //            below, kept for posterity, relates to my thinking on this
  //            subject.
  //
  // This is subtle. We can't group all messages into a single PARALLEL node,
  // otherwise some messages will get "sucked into" an induction region reached
  // by a possibly unrelated message, and thus the logical ordering of
  // inductions will get totally screwed up. For example, one induction A might
  // be embedded in another induction B's init region, but B's cycle/output
  // regions will append to A's induction vector!
  //
  // Really, we need to pretend that all of messages are treated completely
  // independently at first, and then allow `CompleteProcedure` and the work
  // list, which partially uses depth for ordering, to figure the proper order
  // for regions. This is tricky because we need to place anything we find,
  // in terms of.
  proc->body.Emplace(proc, proc_par);

  CompleteProcedure(impl, proc, context, false /* add_return */);

  // The per-stratum differential phases: seed enumeration over lower
  // strata's frontiers, claim drains, and net-frontier construction. They
  // run after the ingest walk above (whose fold crossings park rows in the
  // queues the drains consume) and before the commit sweeps below.
  BuildStratumPhases(impl, context, query);

  // NOTE(pag): This adds in a `return-true` to `proc`.
  PublishDifferentialMessageVectors(impl, proc, context);

  return proc;
}

}  // namespace hyde
