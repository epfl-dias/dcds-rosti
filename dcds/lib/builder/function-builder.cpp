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

#include "dcds/builder/function-builder.hpp"

#include "dcds/builder/statement-builder.hpp"

using namespace dcds;

FunctionBuilder::FunctionBuilder(dcds::Builder *ds_builder, std::string functionName)
    : FunctionBuilder(ds_builder, std::move(functionName), dcds::valueType::VOID) {}

FunctionBuilder::FunctionBuilder(dcds::Builder *ds_builder, std::string functionName, dcds::valueType returnType)
    : builder(ds_builder), _name(std::move(functionName)), returnValueType(returnType) {
  // TODO: function name should not start with get_/set_

  this->entryPoint = std::make_shared<StatementBuilder>(*this);
}
