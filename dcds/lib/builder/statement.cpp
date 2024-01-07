/*
                              Copyright (c) 2024.
          Data Intensive Applications and Systems Laboratory (DIAS)
                  École Polytechnique Fédérale de Lausanne

                              All Rights Reserved.

      Permission to use, copy, modify and distribute this software and
      its documentation is hereby granted, provided that both the
      copyright notice and this permission notice appear in all copies of
      the software, derivative works or modified versions, and any
      portions thereof, and that both notices appear in supporting
      documentation.

      This code is distributed in the hope that it will be useful, but
      WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
      DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
      RESULTING FROM THE USE OF THIS SOFTWARE.
 */

#include "dcds/builder/statement.hpp"

#include "dcds/builder/statement-builder.hpp"

using namespace dcds;

Statement *ConditionalStatement::clone() const {
  return new ConditionalStatement(const_cast<dcds::expressions::Expression *>(this->expr), this->ifBlock->clone_deep(),
                                  this->elseBLock ? this->elseBLock->clone_deep() : nullptr);
}

Statement *ForLoopStatement::clone() const {
  return new ForLoopStatement(const_cast<dcds::expressions::LocalVariableExpression *>(this->loop_var),
                              const_cast<dcds::expressions::Expression *>(this->cond_expr),
                              const_cast<dcds::expressions::Expression *>(this->iteration_expr),
                              this->body->clone_deep());
}

Statement *WhileLoopStatement::clone() const {
  return new WhileLoopStatement(const_cast<dcds::expressions::Expression *>(this->cond_expr), this->body->clone_deep());
}

Statement *DoWhileLoopStatement::clone() const {
  return new DoWhileLoopStatement(const_cast<dcds::expressions::Expression *>(this->cond_expr),
                                  this->body->clone_deep());
}