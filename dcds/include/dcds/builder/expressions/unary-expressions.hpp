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

#ifndef DCDS_UNARY_EXPRESSIONS_HPP
#define DCDS_UNARY_EXPRESSIONS_HPP

#include <any>
#include <cassert>
#include <iostream>
#include <utility>

#include "dcds/builder/builder.hpp"
#include "dcds/builder/expressions/expressions.hpp"
#include "dcds/common/types.hpp"
#include "dcds/util/logging.hpp"

namespace dcds::expressions {

class UnaryExpression : public Expression {
 public:
  UnaryExpression() = default;
  [[nodiscard]] int getNumOperands() const final { return 1; }
  [[nodiscard]] bool isUnaryExpression() const final { return true; }
  [[nodiscard]] bool isBinaryExpression() const final { return false; }

  [[nodiscard]] virtual Expression* getExpression() const = 0;
};

class IsEvenExpression : public UnaryExpression {
 public:
  explicit IsEvenExpression(Expression* _expr) : expr(_expr) { assert(_expr->getResultType() == valueType::INT64); }
  explicit IsEvenExpression(const std::shared_ptr<Expression>& _expr) : expr(_expr.get()) {
    assert(_expr->getResultType() == valueType::INT64);
  }

  [[nodiscard]] valueType getResultType() const override { return valueType::BOOL; }

  [[nodiscard]] Expression* getExpression() const override { return expr; }

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] std::string toString() const override;

 private:
  Expression* expr;
};

class IsNullExpression : public UnaryExpression {
 public:
  explicit IsNullExpression(Expression* _expr) : expr(_expr) {}
  explicit IsNullExpression(const std::shared_ptr<Expression>& _expr) : expr(_expr.get()) {}

  [[nodiscard]] valueType getResultType() const override { return valueType::BOOL; }

  [[nodiscard]] Expression* getExpression() const override { return expr; }

  void* accept(ExpressionVisitor* v) override;
  [[nodiscard]] std::string toString() const override;

 private:
  Expression* expr;
};

class IsNotNullExpression : public UnaryExpression {
 public:
  explicit IsNotNullExpression(Expression* _expr) : expr(_expr) {}
  explicit IsNotNullExpression(const std::shared_ptr<Expression>& _expr) : expr(_expr.get()) {}

  [[nodiscard]] valueType getResultType() const override { return valueType::BOOL; }

  [[nodiscard]] Expression* getExpression() const override { return expr; }

  void* accept(ExpressionVisitor* v) override;
  [[nodiscard]] std::string toString() const override;

 private:
  Expression* expr;
};

class LocalVariableExpression : public UnaryExpression {
 protected:
  explicit LocalVariableExpression(std::string variable_name, dcds::valueType variable_type,
                                   VAR_SOURCE_TYPE source_type, bool is_writable)
      : var_name(std::move(variable_name)),
        var_type(variable_type),
        var_src_type(source_type),
        is_var_writable(is_writable) {}

 public:
  [[nodiscard]] auto getName() { return var_name; }
  [[nodiscard]] auto getType() const { return var_type; }
  [[nodiscard]] auto getSourceType() const { return var_src_type; }

  [[nodiscard]] valueType getResultType() const override { return var_type; }

  [[nodiscard]] Expression* getExpression() const override { assert(false && "how come here?"); }
  [[nodiscard]] std::string toString() const override;

 public:
  const std::string var_name;
  const dcds::valueType var_type;
  const VAR_SOURCE_TYPE var_src_type;
  const bool is_var_writable;
};

class TemporaryVariableExpression : public LocalVariableExpression {
 public:
  explicit TemporaryVariableExpression(std::string variable_name, dcds::valueType variable_type, std::any default_value)
      : LocalVariableExpression(std::move(variable_name), variable_type, VAR_SOURCE_TYPE::TEMPORARY_VARIABLE, true),
        var_default_value(std::move(default_value)) {}

  explicit TemporaryVariableExpression(std::string variable_name, dcds::valueType variable_type)
      : TemporaryVariableExpression(std::move(variable_name), variable_type, {}) {}

  // for composite-types.
  explicit TemporaryVariableExpression(std::string variable_name, std::shared_ptr<Builder> type)
      : LocalVariableExpression(std::move(variable_name), valueType::RECORD_PTR, VAR_SOURCE_TYPE::TEMPORARY_VARIABLE,
                                true),
        objectType(std::move(type)) {}

  void* accept(ExpressionVisitor* v) override;
  [[nodiscard]] std::string toString() const override;

 public:
  const std::any var_default_value{};
  const std::shared_ptr<Builder> objectType{};
};

class FunctionArgumentExpression : public LocalVariableExpression {
 public:
  explicit FunctionArgumentExpression(std::string variable_name, dcds::valueType variable_type, bool pass_by_reference)
      : LocalVariableExpression(std::move(variable_name), variable_type, VAR_SOURCE_TYPE::FUNCTION_ARGUMENT,
                                pass_by_reference),
        is_reference_type(pass_by_reference) {}

  void* accept(ExpressionVisitor* v) override;
  [[nodiscard]] std::string toString() const override;

 public:
  const bool is_reference_type;
};

}  // namespace dcds::expressions

#endif  // DCDS_UNARY_EXPRESSIONS_HPP
