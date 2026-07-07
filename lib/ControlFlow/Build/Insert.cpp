// Copyright 2020, Trail of Bits. All rights reserved.

#include "Build.h"

namespace hyde {

// Build an eager region for publishing data, or inserting it. This might end
// up passing things through if this isn't actually a message publication.
void BuildEagerInsertRegion(ProgramImpl *impl, QueryView pred_view,
                            QueryInsert insert, Context &context, OP *parent,
                            TABLE *last_table) {
  const auto view = QueryView(insert);

  // Only the stored (input) columns are persisted and published. Attached
  // witness columns are read-only edges to the incoming view; their
  // variables are already in scope at the insert site and nothing about
  // them is emitted.
  const auto cols = insert.InputColumns();

  DataModel *const model = impl->view_to_model[view]->FindAs<DataModel>();
  TABLE *const table = model->table;

  if (table) {
    if (table != last_table) {
      TupleState from_state = TupleState::kAbsent;
      if (view.CanProduceDeletions()) {
        from_state = TupleState::kAbsentOrUnknown;
      }

      const auto table_insert =
          impl->operation_regions.CreateDerived<CHANGETUPLE>(
              parent, from_state, TupleState::kPresent);

      for (auto col : cols) {
        const auto var = parent->VariableFor(impl, col);
        table_insert->col_values.AddUse(var);
      }

      table_insert->table.Emplace(table_insert, table);
      parent->body.Emplace(parent, table_insert);
      parent = table_insert;
      last_table = table;
    }
  }

  // This insert represents a message publication.
  if (insert.IsStream()) {
    auto io = QueryIO::From(insert.Stream());
    auto message = ParsedMessage::From(io.Declaration());

    // There's an accumulation vector, add it in.
    if (const auto pub_vec = context.publish_vecs[message];
        pub_vec != nullptr) {

      auto append = impl->operation_regions.CreateDerived<VECTORAPPEND>(
          parent, ProgramOperation::kAppendToMessageOutputVector);
      parent->body.Emplace(parent, append);
      append->vector.Emplace(append, pub_vec);

      for (auto col : insert.InputColumns()) {
        append->tuple_vars.AddUse(append->VariableFor(impl, col));
      }

    // No accumulation vector, publish right now.
    } else {
      const auto message_publish =
          impl->operation_regions.CreateDerived<PUBLISH>(
              parent, message, impl->next_id++);
      parent->body.Emplace(parent, message_publish);

      for (auto col : cols) {
        const auto var = parent->VariableFor(impl, col);
        message_publish->arg_vars.AddUse(var);
      }
    }

  // Inserting into a relation.
  } else if (insert.IsRelation()) {
    BuildEagerInsertionRegions(impl, view, context, parent, view.Successors(),
                               last_table);

  } else {
    assert(false);
  }
}

// A bottom-up insert remover is not a DELETE; instead it is that the relation
// that backs this INSERT is somehow subject to differential updates, e.g.
// because it is downstream from an aggregate or kvindex.
void CreateBottomUpInsertRemover(ProgramImpl *impl, Context &context,
                                 QueryView view, OP *parent_,
                                 TABLE *already_removed_) {
  auto [parent, table, already_removed] =
      InTryMarkUnknown(impl, context, view, parent_, already_removed_);

  const auto insert = QueryInsert::From(view);
  const auto insert_cols = insert.InputColumns();
  (void) insert_cols;

  // If were doing a removal to a stream, then we want to defer publication
  // of the removal until later, when we know the thing is truly gone.
  if (insert.IsStream()) {
    auto io = QueryIO::From(insert.Stream());
    auto message = ParsedMessage::From(io.Declaration());

    const auto pub_vec = context.publish_vecs[message];
    assert(pub_vec != nullptr);

    auto append = impl->operation_regions.CreateDerived<VECTORAPPEND>(
        parent, ProgramOperation::kAppendToMessageOutputVector);
    parent->body.Emplace(parent, append);
    append->vector.Emplace(append, pub_vec);

    for (auto col : insert.InputColumns()) {
      append->tuple_vars.AddUse(append->VariableFor(impl, col));
    }

  // Otherwise, call our successor removal functions. In this case, we're trying
  // to call the removers associated with every `QuerySelect` node.
  } else {

    const auto par = impl->parallel_regions.Create(parent);
    parent->body.Emplace(parent, par);

    // Make sure that we've already done the checking for these nodes.
    assert(table != nullptr);
    assert(already_removed == table);

    for (auto succ_view : view.Successors()) {
      assert(succ_view.IsSelect());

      assert(succ_view.Columns().size() == insert_cols.size());

      auto let = impl->operation_regions.CreateDerived<LET>(par);
      par->AddRegion(let);

      // Bind the SELECT's columns to the INSERT's stored columns, whose
      // variables are in scope.
      auto i = 0u;
      for (auto col : succ_view.Columns()) {
        const auto in_col = insert.NthInputColumn(i++);
        let->col_id_to_var[col.Id()] = let->VariableFor(impl, in_col);
      }

      BuildEagerRemovalRegions(impl, succ_view, context, let,
                               succ_view.Successors(), already_removed);
    }
  }
}

}  // namespace hyde
