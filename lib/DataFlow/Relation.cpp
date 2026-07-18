// Copyright 2022, Peter Goodman. All rights reserved.

#include "Query.h"

namespace hyde {

QueryRelationImpl::~QueryRelationImpl(void) {}

QueryRelationImpl::QueryRelationImpl(ParsedDeclaration decl_)
    : Def<QueryRelationImpl>(this),
      User(this),
      declaration(decl_),
      is_condition(decl_.Arity() == 0u),
      inserts(this),
      selects(this) {}

}  // namespace hyde
