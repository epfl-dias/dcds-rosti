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

#ifndef DCDS_EXPRESSION_VISITOR_HPP
#define DCDS_EXPRESSION_VISITOR_HPP

#include "dcds/builder/expressions/binary-expressions.hpp"
#include "dcds/builder/expressions/constant-expressions.hpp"
#include "dcds/builder/expressions/expressions.hpp"
#include "dcds/builder/expressions/unary-expressions.hpp"

namespace dcds::expressions {

// Add visit methods for other expression types

class ExpressionVisitor {
 public:
  // Unary expressions
  virtual void* visit(const expressions::IsNullExpression& expr) = 0;
  virtual void* visit(const expressions::IsNotNullExpression& expr) = 0;
  virtual void* visit(const expressions::IsEvenExpression& expr) = 0;

  // Constant expressions
  virtual void* visit(const expressions::Int64Constant& expr) = 0;
  virtual void* visit(const expressions::Int32Constant& expr) = 0;
  virtual void* visit(const expressions::BoolConstant& expr) = 0;
  virtual void* visit(const expressions::FloatConstant& expr) = 0;
  virtual void* visit(const expressions::DoubleConstant& expr) = 0;
  virtual void* visit(const expressions::NullPtrConstant& expr) = 0;

  // Binary expressions
  virtual void* visit(const expressions::AddExpression& expr) = 0;
  virtual void* visit(const expressions::SubtractExpression& expr) = 0;
  virtual void* visit(const expressions::EqualExpression& expr) = 0;
  virtual void* visit(const expressions::NotEqualExpression& expr) = 0;
  virtual void* visit(const expressions::GreaterThanExpression& expr) = 0;
  virtual void* visit(const expressions::GreaterThanOrEqualToExpression& expr) = 0;
  virtual void* visit(const expressions::LessThanExpression& expr) = 0;
  virtual void* visit(const expressions::LessThanOrEqualToExpression& expr) = 0;

  // Temporary variables / Function Arguments
  virtual void* visit(const expressions::LocalVariableExpression& expr) = 0;
  virtual void* visit(const expressions::FunctionArgumentExpression& expr) = 0;
  virtual void* visit(const expressions::TemporaryVariableExpression& expr) = 0;

  virtual ~ExpressionVisitor() = default;
};

}  // namespace dcds::expressions

#endif  // DCDS_EXPRESSION_VISITOR_HPP
