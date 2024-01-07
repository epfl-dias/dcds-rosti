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

#ifndef DCDS_EXPRESSIONS_HPP
#define DCDS_EXPRESSIONS_HPP

#include <sstream>
#include <string>

#include "dcds/common/common.hpp"
#include "dcds/common/types.hpp"

namespace dcds::expressions {

class ExpressionVisitor;  // forward declaration

class Expression {
 public:
  //  explicit Expression(const ExpressionType *type)
  //      : type(type), registered(false) {}

  Expression(const Expression &) = default;
  Expression(Expression &&) = default;
  Expression &operator=(const Expression &) = default;
  Expression &operator=(Expression &&) = default;

  Expression() = default;
  virtual ~Expression() = default;

 public:
  [[nodiscard]] virtual valueType getResultType() const = 0;  // Method to get the result type

  virtual void *accept(ExpressionVisitor *v) = 0;

  [[nodiscard]] virtual std::string toString() const = 0;

 public:
  // Utilities
  [[nodiscard]] virtual int getNumOperands() const = 0;
  [[nodiscard]] virtual bool isUnaryExpression() const = 0;
  [[nodiscard]] virtual bool isBinaryExpression() const = 0;
};

}  // namespace dcds::expressions

#endif  // DCDS_EXPRESSIONS_HPP
