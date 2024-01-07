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
void* Int32Constant::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* BoolConstant::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* FloatConstant::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* DoubleConstant::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* NullPtrConstant::accept(ExpressionVisitor* v) { return v->visit(*this); }

// Binary expressions
void* AddExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* SubtractExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* EqualExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* NotEqualExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* GreaterThanExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* GreaterThanOrEqualToExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* LessThanExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }
void* LessThanOrEqualToExpression::accept(ExpressionVisitor* v) { return v->visit(*this); }

std::string TemporaryVariableExpression::toString() const {
  std::stringstream out;

  out << this->var_type << " " << this->var_name;
//  if (this->var_default_value.has_value()) {
//    out << " - val-type: " << this->var_default_value.type().name();
//  }
  return out.str();
}
std::string FunctionArgumentExpression::toString() const {
  std::stringstream out;

  out << this->var_type << (this->is_reference_type ? "* " : " ") << this->var_name;

  return out.str();
}
std::string LocalVariableExpression::toString() const {
  if (this->var_src_type == VAR_SOURCE_TYPE::FUNCTION_ARGUMENT) {
    return dynamic_cast<const FunctionArgumentExpression*>(this)->toString();
  } else if (this->var_src_type == VAR_SOURCE_TYPE::TEMPORARY_VARIABLE) {
    return dynamic_cast<const TemporaryVariableExpression*>(this)->toString();
  } else {
    assert(false);
  }
}
std::string IsEvenExpression::toString() const { return std::string{"IS_EVEN( " + this->expr->toString() + " )"}; }
std::string IsNullExpression::toString() const { return std::string{"IS_NULL( " + this->expr->toString() + " )"}; }
std::string IsNotNullExpression::toString() const {
  return std::string{"IS_NOT_NULL( " + this->expr->toString() + " )"};
}

std::string EqualExpression::toString() const {
  return std::string{" (" + this->getLeft()->toString() + " == " + this->getRight()->toString() + " )"};
}