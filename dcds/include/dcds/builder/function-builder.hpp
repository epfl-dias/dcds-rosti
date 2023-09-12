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

/// Class representing a function in DCDS
class FunctionBuilder {
  friend class Visitor;

 public:
  ///
  /// \param functionName Name of the function
  FunctionBuilder(std::string functionName) : name(std::move(functionName)), hasArguments(false) {}

  ///
  /// \param functionName Name of the function
  /// \param returnType Return type of the function
  FunctionBuilder(std::string functionName, dcds::valueType returnType)
      : name(std::move(functionName)), returnValueType(returnType), hasArguments(false) {}

  ///
  /// \param functionName Name of the function
  /// \param returnType Return type of the function
  /// \param argsTypes Argument types of the function
  FunctionBuilder(std::string functionName, dcds::valueType returnType, std::same_as<dcds::valueType> auto... argsTypes)
      : name(std::move(functionName)), returnValueType(returnType), hasArguments(true) {
    addArguments(argsTypes...);
  }

  ///
  /// \return Function name
  auto getName() { return name; }
  ///
  /// \return Function return value type
  dcds::valueType getReturnValueType() { return returnValueType; }
  ///
  /// \return Check if the function has arguments
  bool getHasArgumentsStatus() { return hasArguments; }

  ///
  /// \param theLLVMContext LLVM context from the codegen engine
  /// \param theLLVMModule  LLVM module from the codegen engine
  /// \param hasAttr        Flag to check if the data structure has attributes, if it has them, take a void pointer as
  ///                       first argument
  /// \return               Generated code for the function signature
  auto codegenFunctionSignature(std::unique_ptr<llvm::LLVMContext> &theLLVMContext,
                                std::unique_ptr<llvm::Module> &theLLVMModule, bool hasAttr) {
    std::vector<llvm::Type *> argTypes;
    llvm::Type *returnType = llvm::Type::getVoidTy(*theLLVMContext);

    if (hasAttr) argTypes.push_back(llvm::Type::getInt8PtrTy(*theLLVMContext));

    for (auto arg : functionArgumentsTypes) {
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
    auto fn =
        llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, name, theLLVMModule.get());

    return fn;
  }

 private:
  ///
  /// \param args  Function argument types which are to be recorded
  void addArguments(std::same_as<dcds::valueType> auto &... argsTypes) {
    for (auto i : {argsTypes...}) {
      functionArgumentsTypes.emplace_back(i);
    }
  }

  /// Record function name
  const std::string name;
  /// Record function return value type
  dcds::valueType returnValueType = dcds::valueType::VOID;
  /// Check if the function has arguments
  const bool hasArguments;

  /// Record function argument types
  std::vector<dcds::valueType> functionArgumentsTypes;
};

}  // namespace dcds

#endif  // DCDS_FUNCTION_BUILDER_HPP
