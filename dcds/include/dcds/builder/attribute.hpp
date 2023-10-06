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

#ifndef DCDS_ATTRIBUTE_HPP
#define DCDS_ATTRIBUTE_HPP

#include <any>
#include <concepts>
#include <dcds/common/types.hpp>
#include <dcds/util/locks/lock.hpp>
#include <dcds/util/locks/spin-lock.hpp>
#include <dcds/util/logging.hpp>
#include <deque>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

namespace dcds {

/// Class representing a normal attribute in DCDS
class SimpleAttribute {
 public:
  ///
  /// \param attribute_name  Name of the attribute to be constructed
  /// \param attribute_type  Type of the attribute to be constructed
  /// \param initialValue    Initial value of the attribute to be constructed
  //  SimpleAttribute(std::string attribute_name, dcds::valueType attribute_type, std::variant<int64_t, void *>
  //  initialValue)
  //      : name(std::move(attribute_name)), type(attribute_type), initVal(initialValue) {
  //    defaultValue
  //  }

  SimpleAttribute(std::string attribute_name, dcds::valueType attribute_type, std::any default_value)
      : name(std::move(attribute_name)), type(attribute_type), defaultValue(std::move(default_value)) {}
  SimpleAttribute(std::string attribute_name, dcds::valueType attribute_type)
      : name(std::move(attribute_name)), type(attribute_type) {
    defaultValue.reset();
  }

  auto getDefaultValue() const {
    //    if(defaultValue.has_value()){
    //
    //    }

    return defaultValue;
  }

  // Transactional Variables.
  // GET/ INSERT/ DELETE/ UPDATE

  ///
  /// \param theLLVMContext LLVM context from the codegen engine
  /// \param block          LLVM block in which the attribute should be added
  /// \param attrPos        Position in the block where the attribute should be added
  /// \return               Generated IR for attribute allocation
  auto addAttributeCodegen(std::unique_ptr<llvm::LLVMContext> &theLLVMContext, llvm::BasicBlock *block,
                           llvm::BasicBlock::iterator attrPos) {
    auto allocaBuilder = llvm::IRBuilder<>(block, attrPos);

    if (this->type == dcds::valueType::INTEGER)
      return allocaBuilder.CreateAlloca(llvm::Type::getInt64Ty(*theLLVMContext), nullptr, name);
    else if (this->type == dcds::valueType::RECORD_PTR) {
      return allocaBuilder.CreateAlloca(llvm::Type::getInt8PtrTy(*theLLVMContext), nullptr, name);
    }

    return allocaBuilder.CreateAlloca(llvm::Type::getVoidTy(*theLLVMContext), nullptr, name);
  }

  ///
  /// \param llvmBuilder LLVM builder from the codegen engine
  /// \param val         LLVM value which should be used for setting the attribute
  /// \param varAlloc    Attribute allocation IR
  /// \return            Generated store instruction for setting the attribute
  auto setAttributeCodegen(std::unique_ptr<llvm::IRBuilder<>> &llvmBuilder, llvm::Value *val, llvm::Value *varAlloc) {
    return llvmBuilder->CreateStore(val, varAlloc);
  }

  ///
  /// \param llvmBuilder     LLVM builder from the codegen engine
  /// \param theLLVMContext  LLVM context from the codegen engine
  /// \param val             Generated code for the attribute which should be loaded
  /// \return                Generated load instruction for getting the attribute
  auto getAttributeCodegen(std::unique_ptr<llvm::IRBuilder<>> &llvmBuilder,
                           std::unique_ptr<llvm::LLVMContext> &theLLVMContext, llvm::Value *val) {
    if (type == dcds::valueType::INTEGER)
      return llvmBuilder->CreateLoad(llvm::Type::getInt64Ty(*theLLVMContext), val);
    else if (type == dcds::valueType::RECORD_PTR)
      return llvmBuilder->CreateLoad(llvm::Type::getInt8PtrTy(*theLLVMContext), val);

    return llvmBuilder->CreateLoad(llvm::Type::getVoidTy(*theLLVMContext), val);
  }

  ///
  /// \param llvmBuilder LLVM builder from the codegen engine
  /// \param val1        Generated code for first value to be added
  /// \param val2        Generated code for second value to be added
  /// \return            Generated add instruction for the two values
  auto addTwoVarsCodegen(std::unique_ptr<llvm::IRBuilder<>> &llvmBuilder, llvm::Value *val1, llvm::Value *val2) {
    if (type == dcds::valueType::INTEGER) return llvmBuilder->CreateAdd(val1, val2);

    return llvmBuilder->CreateAdd(val1, val2);  // Dummy return for now to stop the compiler from complaining.
  }

  /// Variable for storing attribute name
  const std::string name;
  /// Variable for storing attribute type
  const dcds::valueType type;
  /// Variable for storing initial value of the attribute
  std::variant<int64_t, void *> initVal;

  std::any defaultValue;

 private:
};
}  // namespace dcds
#endif  // DCDS_ATTRIBUTE_HPP
