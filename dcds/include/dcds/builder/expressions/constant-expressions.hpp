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

#ifndef DCDS_CONSTANT_EXPRESSIONS_HPP
#define DCDS_CONSTANT_EXPRESSIONS_HPP

#include "dcds/builder/expressions/unary-expressions.hpp"

namespace dcds::expressions {

class Constant : public UnaryExpression {
 public:
  enum class ConstantType { INT, INT64, BOOL, FLOAT, DOUBLE, NULLPTR };
  explicit Constant(ConstantType constantType) : type(constantType) {}

  [[nodiscard]] virtual ConstantType getConstantType() const { return type; }

  [[nodiscard]] Expression* getExpression() const override { assert(false); }

 public:
  const ConstantType type;
};

class Int64Constant : public Constant {
 public:
  explicit Int64Constant(int64_t _val) : Constant(Constant::ConstantType::INT64), val(_val) {}

  [[nodiscard]] valueType getResultType() const override { return valueType::INT64; }

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] auto getValue() const { return val; }

 private:
  int64_t val;
};

class BoolConstant : public Constant {
 public:
  explicit BoolConstant(bool _val) : Constant(Constant::ConstantType::BOOL), val(_val) {}

  [[nodiscard]] valueType getResultType() const override { return valueType::BOOL; }

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] auto getValue() const { return val; }

 private:
  bool val;
};

class FloatConstant : public Constant {
 public:
  explicit FloatConstant(float _val) : Constant(Constant::ConstantType::FLOAT), val(_val) {}

  [[nodiscard]] valueType getResultType() const override { return valueType::FLOAT; }

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] auto getValue() const { return val; }

 private:
  float val;
};

class DoubleConstant : public Constant {
 public:
  explicit DoubleConstant(double _val) : Constant(Constant::ConstantType::DOUBLE), val(_val) {}

  [[nodiscard]] valueType getResultType() const override { return valueType::DOUBLE; }

  void* accept(ExpressionVisitor* v) override;

  [[nodiscard]] auto getValue() const { return val; }

 private:
  double val;
};

class NullPtrConstant : public Constant {
 public:
  explicit NullPtrConstant() : Constant(Constant::ConstantType::NULLPTR) {}

  [[nodiscard]] valueType getResultType() const override { return valueType::RECORD_PTR; }

  void* accept(ExpressionVisitor* v) override;

  //  [[nodiscard]] auto getValue() const { return 0; }
};

}  // namespace dcds::expressions

#endif  // DCDS_CONSTANT_EXPRESSIONS_HPP
