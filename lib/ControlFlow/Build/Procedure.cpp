// Copyright 2020, Trail of Bits. All rights reserved.

#include "Build.h"

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
  (void) message;

  VECTOR *removal_vec = nullptr;
  const auto vec =
      proc->VectorFor(impl, VectorKind::kParameter, receives[0].Columns());
  vec->added_message.emplace(message);

  // Loop over the receives for adding.
  for (auto receive : receives) {

    // A differential message keeps its removal parameter vector (the
    // generated message-handler API takes an add vector and a remove
    // vector), even though no region consumes it yet: differential removal
    // ingest lands with the per-stratum delete fixpoints.
    if (!removal_vec && receive.CanReceiveDeletions()) {
      removal_vec =
          proc->VectorFor(impl, VectorKind::kParameter, receive.Columns());
      removal_vec->removed_message.emplace(message);
    }

    const auto loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
        impl->next_id++, parent, ProgramOperation::kLoopOverInputVector);
    parent->AddRegion(loop);
    loop->vector.Emplace(loop, vec);
    OP *next_parent = loop;

    DataModel *model = impl->view_to_model[receive]->FindAs<DataModel>();
    TABLE *table = model->table;
    UPDATECOUNT *insert = nullptr;

    // If this message receive has a corresponding table, then persist the
    // received tuple with a nonrecursive counter fold (message receipt is
    // explicit support).
    if (table) {
      insert = impl->operation_regions.CreateDerived<UPDATECOUNT>(
          loop, true /* is_add */, DerivClass::kNonRecursive,
          false /* is_explicit */);
      insert->table.Emplace(insert, table);
      loop->body.Emplace(loop, insert);

      next_parent = insert;
    }

    for (auto col : receive.Columns()) {
      VAR * const var = loop->defined_vars.Create(
          impl->next_id++, VariableRole::kVectorVariable);
      var->query_column = col;
      loop->col_id_to_var.emplace(col.Id(), var);

      if (insert) {
        insert->col_values.AddUse(var);
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

  // The end-of-batch commit sweeps: one per differential table, sealing the
  // batch-start snapshot and clearing the batch-scratch flags. A table that
  // backs a `@differential` transmit publishes its net presence changes
  // through its sweep.
  std::unordered_map<TABLE *, ParsedMessage> table_to_message;
  for (const auto &[message, transmit] : context.commit_published_view) {
    const auto pred = transmit.Predecessors()[0];
    DataModel *const pred_model =
        impl->view_to_model[pred]->FindAs<DataModel>();
    assert(pred_model->table != nullptr);
    table_to_message.emplace(pred_model->table, message);
  }

  for (TABLE *table : impl->tables) {
    auto is_differential = false;
    for (const QueryView &table_view : table->views) {
      if (table_view.CanProduceDeletions()) {
        is_differential = true;
        break;
      }
    }
    if (!is_differential) {
      continue;
    }

    COMMITSWEEP *const sweep =
        impl->operation_regions.CreateDerived<COMMITSWEEP>(seq);
    sweep->table.Emplace(sweep, table);
    if (auto it = table_to_message.find(table);
        it != table_to_message.end()) {
      sweep->message.emplace(it->second);
    }
    seq->AddRegion(sweep);
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
      bool all_const = true;
      for (auto in_col : tuple.InputColumns()) {
        if (!in_col.IsConstant()) {
          all_const = false;
        }
      }

      if (!all_const) {
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

  // NOTE(pag): This adds in a `return-true` to `proc`.
  PublishDifferentialMessageVectors(impl, proc, context);

  return proc;
}

}  // namespace hyde
