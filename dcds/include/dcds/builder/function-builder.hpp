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

#ifndef DCDS_FUNCTION_BUILDER_HPP
#define DCDS_FUNCTION_BUILDER_HPP

#include <concepts>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dcds/common/types.hpp"

namespace dcds {
class FunctionBuilder {
 public:
  FunctionBuilder(std::string functionName) : name(std::move(functionName)), doesReturn(false), hasArguments(false) {}

  FunctionBuilder(std::string functionName, dcds::valueType returnType)
      : name(std::move(functionName)), doesReturn(true), returnValueType(returnType), hasArguments(false) {}

  FunctionBuilder(std::string functionName, dcds::valueType returnType, std::same_as<dcds::valueType> auto... args)
      : name(std::move(functionName)), doesReturn(true), returnValueType(returnType), hasArguments(true) {
    addArguments(args...);
  }

  FunctionBuilder(std::string functionName, std::same_as<dcds::valueType> auto... args)
      : name(std::move(functionName)), doesReturn(false), hasArguments(true) {
    addArguments(args...);
  }

  // variableType createLocalVariable(name, type)

  // addOperation

  // CRUD on DS-Attributes
  // Function calls on DS-Attributes (composed attribute, its gonna provide functions also)

 public:
  void finalize() { isFinalized = true; }
  auto isFinal() { return isFinalized; }

  auto getName() { return name; }

 private:
  void addArguments(std::same_as<dcds::valueType> auto &...args) {
    // const std::size_t num_args = sizeof...(args);
    for (auto i : {args...}) {
      functionArguments.emplace_back(i);
    }
  }

 private:
  const std::string name;
  const bool doesReturn;
  const bool hasArguments;
  dcds::valueType returnValueType;

  std::vector<dcds::valueType> functionArguments;

  bool isFinalized;
};

}  // namespace dcds

#endif  // DCDS_FUNCTION_BUILDER_HPP
