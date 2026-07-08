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
      const auto table_insert =
          BuildUpdateCount(impl, table, parent, cols, true /* is_add */,
                           EmissionDerivClass(impl, context, view));
      parent->body.Emplace(parent, table_insert);
      parent = table_insert;
      last_table = table;
    }
  }

  // This insert represents a message publication.
  if (insert.IsStream()) {
    auto io = QueryIO::From(insert.Stream());
    auto message = ParsedMessage::From(io.Declaration());

    // A differential-table-backed `@differential` message: publication is
    // owned by the backing table's end-of-batch commit sweep, which reports
    // net 0/1 presence changes; nothing publishes eagerly.
    if (context.commit_published_view.count(message)) {
      return;
    }

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

}  // namespace hyde
