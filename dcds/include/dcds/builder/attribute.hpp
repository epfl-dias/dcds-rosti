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

// ToDo: Remove template as soon as we find a better way to model initial value's type.
// template <typename initValType>
class Attribute {
 public:
  Attribute(std::string attribute_name, dcds::valueType attribute_type, std::variant<int64_t, void *> initialValue)
      : name(std::move(attribute_name)), type(attribute_type), initVal(initialValue) {}

  // Transactional Variables.
  // GET/ INSERT/ DELETE/ UPDATE

  llvm::GlobalVariable *createGlob(std::unique_ptr<llvm::IRBuilder<>> &llvmBuilder,
                                   std::unique_ptr<llvm::LLVMContext> &theContext,
                                   std::unique_ptr<llvm::Module> &theModule, std::string Name) {
    if (this->type == dcds::valueType::INTEGER)
      theModule->getOrInsertGlobal(Name, llvmBuilder->getInt64Ty());
    else if (this->type == dcds::valueType::RECORD_PTR)
      theModule->getOrInsertGlobal(Name, llvmBuilder->getInt8PtrTy());

    //    theModule->getOrInsertGlobal(Name, llvmBuilder->getInt64Ty());
    llvm::GlobalVariable *gVar = theModule->getNamedGlobal(Name);
    gVar->setLinkage(llvm::GlobalValue::CommonLinkage);

    if (this->type == dcds::valueType::INTEGER)
      gVar->setAlignment(static_cast<llvm::MaybeAlign>(alignof(int64_t)));
    else if (this->type == dcds::valueType::RECORD_PTR)
      gVar->setAlignment(static_cast<llvm::MaybeAlign>(alignof(void *)));

    if (type == dcds::valueType::INTEGER) {
      llvm::Constant *initValCodegen = llvm::ConstantInt::get(*theContext, llvm::APInt(64, 0));
      gVar->setInitializer(initValCodegen);
    }

    return gVar;
  }

  auto addGlobalAttributeCodegen(std::unique_ptr<llvm::IRBuilder<>> &llvmBuilder,
                                 std::unique_ptr<llvm::LLVMContext> &theContext,
                                 std::unique_ptr<llvm::Module> &theModule) {
    return createGlob(llvmBuilder, theContext, theModule, name);
  }

  auto addAttributeCodegen(std::unique_ptr<llvm::LLVMContext> &theContext, llvm::BasicBlock *block,
                           llvm::BasicBlock::iterator attrPos) {
    auto allocaBuilder = llvm::IRBuilder<>(block, attrPos);

    if (this->type == dcds::valueType::INTEGER)
      return allocaBuilder.CreateAlloca(llvm::Type::getInt64Ty(*theContext), nullptr, name);
    else if (this->type == dcds::valueType::RECORD_PTR) {
      return allocaBuilder.CreateAlloca(llvm::Type::getInt8PtrTy(*theContext), nullptr, name);
    }

    return allocaBuilder.CreateAlloca(llvm::Type::getVoidTy(*theContext), nullptr, name);
  }

  auto setAttributeCodegen(std::unique_ptr<llvm::IRBuilder<>> &llvmBuilder, llvm::Value *val, llvm::Value *varAlloc) {
    return llvmBuilder->CreateStore(val, varAlloc);
  }

  auto getAttributeCodegen(std::unique_ptr<llvm::IRBuilder<>> &llvmBuilder,
                           std::unique_ptr<llvm::LLVMContext> &theContext, llvm::Value *val) {
    if (type == dcds::valueType::INTEGER)
      return llvmBuilder->CreateLoad(llvm::Type::getInt64Ty(*theContext), val);
    else if (type == dcds::valueType::RECORD_PTR)
      return llvmBuilder->CreateLoad(llvm::Type::getInt8PtrTy(*theContext), val);

    return llvmBuilder->CreateLoad(llvm::Type::getVoidTy(*theContext), val);
  }

  auto addTwoVarsCodegen(std::unique_ptr<llvm::IRBuilder<>> &llvmBuilder, llvm::Value *val1, llvm::Value *val2) {
    if (type == dcds::valueType::INTEGER) return llvmBuilder->CreateAdd(val1, val2);

    return llvmBuilder->CreateAdd(val1, val2);  // Dummy return for now to stop the compiler from complaining.
  }

  const std::string name;
  const dcds::valueType type;
  std::variant<int64_t, void *> initVal;

 private:
};

//
// class Uint64T_Attribute{
//
//
//  const dcds::valueType type = valueType::INTEGER;
//  uint64_t val_;
//};
//
// template <class T>
// class AttributeCRTP{
//
//};
//
// class [[nodiscard]] attribute_t : public AttributeCRTP<attribute_t>{
//
//};
//

//
// class BoolConstant
//    : public TConstant<bool, BoolType, Constant::ConstantType::BOOL,
//                       BoolConstant> {
// public:
//  using TConstant<bool, BoolType, Constant::ConstantType::BOOL,
//                  BoolConstant>::TConstant;
//};

}  // namespace dcds
#endif  // DCDS_ATTRIBUTE_HPP
