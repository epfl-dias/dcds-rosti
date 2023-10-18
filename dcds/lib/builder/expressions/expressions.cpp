/*
                              Copyright (c) 2023.
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

#include "dcds/builder/expressions/expressions.hpp"

#include "dcds/builder/expressions/binary-expressions.hpp"
#include "dcds/builder/expressions/expression-visitor.hpp"
#include "dcds/builder/expressions/unary-expressions.hpp"

using namespace dcds::expressions;

// Temporary variables / Function Arguments
void* FunctionArgumentExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* TemporaryVariableExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }

// Unary expressions
void* IsNullExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* IsNotNullExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* IsEvenExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }

// Constant expressions
void* Int64Constant::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* BoolConstant::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* FloatConstant::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* DoubleConstant::accept(ExpressionVisitor* v) { return v->visit(*this); }

// Binary expressions
void* AddExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* SubtractExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
