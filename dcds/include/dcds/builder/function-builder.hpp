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
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

namespace dcds {
class FunctionBuilder {
  friend class Visitor;

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

  void finalize() { isFinalized = true; }
  auto isFinal() { return isFinalized; }
  auto getName() { return name; }
  bool getDoesReturnStatus() { return doesReturn; }
  dcds::valueType getReturnValueType() { return returnValueType; }
  bool getHasArgumentsStatus() { return hasArguments; }

  auto codegen(std::unique_ptr<llvm::LLVMContext> &theLLVMContext, std::unique_ptr<llvm::Module> &theModule,
               bool hasAttr) {
    std::vector<llvm::Type *> argTypes;
    llvm::Type *returnType = llvm::Type::getVoidTy(*theLLVMContext);

    if (hasAttr) argTypes.push_back(llvm::Type::getInt8PtrTy(*theLLVMContext));

    for (auto arg : functionArguments) {
      if (arg == dcds::valueType::INTEGER)
        argTypes.push_back(llvm::Type::getInt64PtrTy(*theLLVMContext));
      else if (arg == dcds::valueType::RECORD_PTR)
        argTypes.push_back(llvm::Type::getInt8PtrTy(*theLLVMContext));
    }

    if (returnValueType == dcds::valueType::INTEGER)
      returnType = llvm::Type::getInt64Ty(*theLLVMContext);
    else if (returnValueType == dcds::valueType::RECORD_PTR)
      returnType = llvm::Type::getInt8PtrTy(*theLLVMContext);

    auto fn_type = llvm::FunctionType::get(returnType, argTypes, false);
    auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, name, theModule.get());

    return fn;
  }

 private:
  void addArguments(std::same_as<dcds::valueType> auto &... args) {
    // const std::size_t num_args = sizeof...(args);
    for (auto i : {args...}) {
      functionArguments.emplace_back(i);
    }
  }

  const std::string name;
  const bool doesReturn;
  dcds::valueType returnValueType = dcds::valueType::VOID;
  const bool hasArguments;

  std::vector<dcds::valueType> functionArguments;

  bool isFinalized;
};

}  // namespace dcds

#endif  // DCDS_FUNCTION_BUILDER_HPP
