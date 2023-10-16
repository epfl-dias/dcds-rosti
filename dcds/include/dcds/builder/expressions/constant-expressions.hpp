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

#include "dcds/builder/expressions/expressions.hpp"

namespace dcds::expressions {

// class Constant : public Expression {
//  public:
//   enum ConstantType { INT, INT64, BOOL, FLOAT, STRING, DSTRING, DATE };
//   Constant(const ExpressionType *type) : Expression(type) {}
//
//   [[nodiscard]] virtual ConstantType getConstantType() const = 0;
// };
//
// class Int64Constant
//     : public TConstant<int64_t, Int64Type, Constant::ConstantType::INT64,
//                        Int64Constant> {
//  public:
//   using TConstant<int64_t, Int64Type, Constant::ConstantType::INT64,
//                   Int64Constant>::TConstant;
// };

}

#endif  // DCDS_CONSTANT_EXPRESSIONS_HPP
