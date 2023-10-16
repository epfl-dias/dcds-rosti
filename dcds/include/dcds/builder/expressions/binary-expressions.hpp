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

#ifndef DCDS_BINARY_EXPRESSIONS_HPP
#define DCDS_BINARY_EXPRESSIONS_HPP

#include <cassert>

#include "dcds/builder/expressions/expressions.hpp"

namespace dcds::expressions {

class BinaryExpression : public Expression {
 public:
  BinaryExpression(Expression* left, Expression* right) : _left(left), _right(right) {}
  [[nodiscard]] int getNumOperands() const override { return 2; };
  [[nodiscard]] bool isUnaryExpression() const override { return false; }
  [[nodiscard]] bool isBinaryExpression() const override { return true; }

  Expression* getLeft() const { return _left; }
  Expression* getRight() const { return _right; }

 private:
  Expression* _left;
  Expression* _right;
};

class AddExpression : public BinaryExpression {
 public:
  explicit AddExpression(Expression* left, Expression* right) : BinaryExpression(left, right) {}
  explicit AddExpression(const std::shared_ptr<Expression>& left, const std::shared_ptr<Expression>& right)
      : BinaryExpression(left.get(), right.get()) {}

  void* accept(ExpressionVisitor* v) override;
};

}  // namespace dcds::expressions

#endif  // DCDS_BINARY_EXPRESSIONS_HPP
