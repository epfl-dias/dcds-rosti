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
  [[nodiscard]] int getNumOperands() const override { return 2; }
  [[nodiscard]] bool isUnaryExpression() const override { return false; }
  [[nodiscard]] bool isBinaryExpression() const override { return true; }

  [[nodiscard]] Expression* getLeft() const { return _left; }
  [[nodiscard]] Expression* getRight() const { return _right; }

 protected:
  Expression* _left;
  Expression* _right;
};

class AddExpression : public BinaryExpression {
 public:
  explicit AddExpression(Expression* left, Expression* right) : BinaryExpression(left, right) {}
  explicit AddExpression(const std::shared_ptr<Expression>& left, const std::shared_ptr<Expression>& right)
      : BinaryExpression(left.get(), right.get()) {}

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] valueType getResultType() const override { return this->_left->getResultType(); }

  [[nodiscard]] std::string toString() const override {
    return std::string{"( " + this->_left->toString() + " + " + this->_right->toString() + ")"};
  }
};

class SubtractExpression : public BinaryExpression {
 public:
  explicit SubtractExpression(Expression* left, Expression* right) : BinaryExpression(left, right) {}
  explicit SubtractExpression(const std::shared_ptr<Expression>& left, const std::shared_ptr<Expression>& right)
      : BinaryExpression(left.get(), right.get()) {
    LOG(INFO) << "Creating SubtractExpression:: " << right->toString();
  }

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] valueType getResultType() const override { return this->_left->getResultType(); }

  [[nodiscard]] std::string toString() const override {
    return std::string{"( " + this->_left->toString() + " - " + this->_right->toString() + ")"};
  }

  ~SubtractExpression() override { LOG(INFO) << "Destructing SubtractExpression"; }
};

class EqualExpression : public BinaryExpression {
 public:
  explicit EqualExpression(Expression* left, Expression* right) : BinaryExpression(left, right) {}
  explicit EqualExpression(const std::shared_ptr<Expression>& left, const std::shared_ptr<Expression>& right)
      : BinaryExpression(left.get(), right.get()) {}

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] std::string toString() const override;

  [[nodiscard]] valueType getResultType() const final { return dcds::valueType::BOOL; }
};
class NotEqualExpression : public BinaryExpression {
 public:
  explicit NotEqualExpression(Expression* left, Expression* right) : BinaryExpression(left, right) {}
  explicit NotEqualExpression(const std::shared_ptr<Expression>& left, const std::shared_ptr<Expression>& right)
      : BinaryExpression(left.get(), right.get()) {}

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] valueType getResultType() const final { return dcds::valueType::BOOL; }
};

class GreaterThanExpression : public BinaryExpression {
 public:
  explicit GreaterThanExpression(Expression* left, Expression* right) : BinaryExpression(left, right) {}
  explicit GreaterThanExpression(const std::shared_ptr<Expression>& left, const std::shared_ptr<Expression>& right)
      : BinaryExpression(left.get(), right.get()) {}

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] valueType getResultType() const final { return dcds::valueType::BOOL; }

  [[nodiscard]] std::string toString() const override {
    return std::string{"( " + this->_left->toString() + " > " + this->_right->toString() + ")"};
  }

  ~GreaterThanExpression() override { LOG(INFO) << "Destructing GreaterThanExpression"; }
};

class GreaterThanOrEqualToExpression : public BinaryExpression {
 public:
  explicit GreaterThanOrEqualToExpression(Expression* left, Expression* right) : BinaryExpression(left, right) {}
  explicit GreaterThanOrEqualToExpression(const std::shared_ptr<Expression>& left,
                                          const std::shared_ptr<Expression>& right)
      : BinaryExpression(left.get(), right.get()) {}

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] valueType getResultType() const final { return dcds::valueType::BOOL; }

  [[nodiscard]] std::string toString() const override {
    return std::string{"( " + this->_left->toString() + " > " + this->_right->toString() + ")"};
  }

  ~GreaterThanOrEqualToExpression() override { LOG(INFO) << "Destructing GreaterThanOrEqualToExpression"; }
};

class LessThanExpression : public BinaryExpression {
 public:
  explicit LessThanExpression(Expression* left, Expression* right) : BinaryExpression(left, right) {}
  explicit LessThanExpression(const std::shared_ptr<Expression>& left, const std::shared_ptr<Expression>& right)
      : BinaryExpression(left.get(), right.get()) {}

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] valueType getResultType() const final { return dcds::valueType::BOOL; }

  [[nodiscard]] std::string toString() const override {
    return std::string{"( " + this->_left->toString() + " > " + this->_right->toString() + ")"};
  }

  ~LessThanExpression() override { LOG(INFO) << "Destructing LessThanExpression"; }
};

class LessThanOrEqualToExpression : public BinaryExpression {
 public:
  explicit LessThanOrEqualToExpression(Expression* left, Expression* right) : BinaryExpression(left, right) {}
  explicit LessThanOrEqualToExpression(const std::shared_ptr<Expression>& left,
                                       const std::shared_ptr<Expression>& right)
      : BinaryExpression(left.get(), right.get()) {}

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] valueType getResultType() const final { return dcds::valueType::BOOL; }

  [[nodiscard]] std::string toString() const override {
    return std::string{"( " + this->_left->toString() + " > " + this->_right->toString() + ")"};
  }

  ~LessThanOrEqualToExpression() override { LOG(INFO) << "Destructing LessThanOrEqualToExpression"; }
};

}  // namespace dcds::expressions

#endif  // DCDS_BINARY_EXPRESSIONS_HPP
