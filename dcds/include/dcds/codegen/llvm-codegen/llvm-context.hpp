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

#ifndef DCDS_LLVM_CONTEXT_HPP
#define DCDS_LLVM_CONTEXT_HPP

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/EPCIndirectionUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutorProcessControl.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>

#include <iostream>

#include "dcds/util/logging.hpp"

// Adapted from Proteus context.hpp:
// (https://github.com/epfl-dias/proteus/blob/main/core/olap/include/olap/util/context.hpp)

std::string getFunctionName(void *f);

class LLVMCodegenContext {
 private:
  const std::string moduleName;

 protected:
  explicit LLVMCodegenContext(std::string moduleName);
  void registerAllFunctions();

 public:
  virtual ~LLVMCodegenContext() {
    LOG(WARNING) << "[LLVMCodegenContext: ] Destructor";
    // XXX Has to be done in an appropriate sequence - segfaults otherwise
    //        delete Builder;
    //            delete TheFPM;
    //            delete TheExecutionEngine;
    //            delete TheFunction;
    //            delete llvmContext;
    //            delete TheFunction;
  }

  [[nodiscard]] std::string getModuleName() const { return moduleName; }
  [[nodiscard]] const char *getName() const;

  [[nodiscard]] llvm::LLVMContext &getLLVMContext() const { return getModule()->getContext(); }

  [[nodiscard]] virtual llvm::Module *getModule() const = 0;
  [[nodiscard]] virtual llvm::IRBuilder<> *getBuilder() const = 0;
  [[nodiscard]] virtual llvm::Function *getFunction(const std::string &funcName) const;
  virtual void registerFunction(const char *, llvm::Function *);
  void registerFunction(const std::string &function_name, llvm::Type *returnType,
                        const std::vector<llvm::Type *> &args = {}, bool always_inline = false);
  void registerFunction(const std::string &func, std::initializer_list<llvm::Value *> args, llvm::Type *ret);

  static void llvmVerifyFunction(llvm::Function *f);

  [[nodiscard]] llvm::ConstantInt *createInt8(char val) const;
  [[nodiscard]] llvm::ConstantInt *createInt32(int val) const;
  [[nodiscard]] llvm::ConstantInt *createInt64(int val) const;
  [[nodiscard]] llvm::ConstantInt *createInt64(unsigned int val) const;
  [[nodiscard]] llvm::ConstantInt *createInt64(size_t val) const;
  [[nodiscard]] llvm::ConstantInt *createInt64(int64_t val) const;
  [[nodiscard]] llvm::ConstantInt *createSizeT(size_t val) const;
  [[nodiscard]] llvm::ConstantInt *createUintptr(uintptr_t val) const;
  [[nodiscard]] llvm::ConstantInt *createTrue() const;
  [[nodiscard]] llvm::ConstantInt *createFalse() const;
  [[nodiscard]] llvm::IntegerType *createSizeType() const;
  [[nodiscard]] llvm::Value *createStringConstant(const std::string &value, const std::string &var_name) const;

  llvm::StructType *getVaListStructType();
  llvm::Value *createVaListStart();
  void createVaListEnd(llvm::Value *va_list_ptr) const;
  llvm::Value *getVAArg(llvm::Value *va_list_ptr, llvm::Type *type) const;
  std::vector<llvm::Value *> getVAArgs(llvm::Value *va_list_ptr, const std::vector<llvm::Type *> &types) const;

  virtual size_t getSizeOf(llvm::Type *type) const;
  virtual size_t getSizeOf(llvm::Value *val) const;

  static llvm::StructType *CreateCustomStruct(llvm::LLVMContext &, const std::vector<llvm::Type *> &innerTypes);
  [[nodiscard]] llvm::StructType *CreateCustomStruct(const std::vector<llvm::Type *> &innerTypes) const;

  void createPrintString(const std::string &str);

  /**
   * Does not involve AllocaInst, but still is a memory position
   * NOTE: 1st elem of Struct is 0!!
   */
  llvm::Value *getStructElem(llvm::Value *mem_struct, int elemNo) const;
  llvm::Value *getStructElem(llvm::AllocaInst *mem_struct, int elemNo) const;
  void updateStructElem(llvm::Value *toStore, llvm::Value *mem_struct, int elemNo) const;
  llvm::Value *getStructElemMem(llvm::Value *mem_struct, int elemNo) const;
  static llvm::PointerType *getPointerType(llvm::Type *type);

  static llvm::AllocaInst *createAlloca(llvm::BasicBlock *InsertAtBB, const std::string &VarName, llvm::Type *varType);

  void CreateIfElseBlocks(llvm::Function *fn, const std::string &if_name, const std::string &else_name,
                          llvm::BasicBlock **if_block, llvm::BasicBlock **else_block,
                          llvm::BasicBlock *insert_before = nullptr) const;
  void CreateIfBlock(llvm::Function *fn, const std::string &if_name, llvm::BasicBlock **if_block,
                     llvm::BasicBlock *insert_before = nullptr) const;

  llvm::BasicBlock *CreateIfBlock(llvm::Function *fn, const std::string &if_label,
                                  llvm::BasicBlock *insert_before = nullptr) const;

  llvm::Value *CastPtrToLlvmPtr(llvm::PointerType *type, const void *ptr) const;
  llvm::Value *getArrayElem(llvm::AllocaInst *mem_ptr, llvm::Value *offset) const;
  llvm::Value *getArrayElem(llvm::Value *val_ptr, llvm::Value *offset) const;
  /**
   * Does not involve AllocaInst, but still returns a memory position
   */
  llvm::Value *getArrayElemMem(llvm::Value *val_ptr, llvm::Value *offset) const;

  [[nodiscard]] llvm::Value *createStringArray(const std::vector<std::string> &string_array,
                                               const std::string &name_prefix) const;

  template <typename R, typename... Args>
  llvm::Value *gen_call(R (*func)(Args...), std::initializer_list<llvm::Value *> args, llvm::Type *ret) {
    auto function_name = getFunctionName((void *)func);
    return gen_call(function_name, args, ret);
  }

  template <typename R, typename... Args>
  llvm::Value *gen_call(R (*func)(Args...), std::initializer_list<llvm::Value *> args) {
    return gen_call(func, args, toLLVM<std::remove_cv_t<R>>());
  }

  /*virtual*/ llvm::Value *gen_call(const std::string &func, std::initializer_list<llvm::Value *> args,
                                    llvm::Type *ret = nullptr);

  /*virtual*/ llvm::Value *gen_call(llvm::Function *func, std::initializer_list<llvm::Value *> args) const;

  /*virtual*/ llvm::Value *gen_call(llvm::Function *func, const std::vector<llvm::Value *> &args) const;

  template <typename, typename = void>
  static constexpr bool is_type_complete_v = false;

  template <typename T>
  llvm::Type *toLLVM() {
    if constexpr (std::is_void_v<T>) {
      //      LOG(INFO) << "voidTy";
      return llvm::Type::getVoidTy(getLLVMContext());
    } else if constexpr (std::is_pointer_v<T>) {
      //      LOG(INFO) << "Void*";
      if constexpr (std::is_void_v<std::remove_cv_t<std::remove_pointer_t<T>>>) {
        // No void ptr type in llvm ir
        return llvm::PointerType::getUnqual(toLLVM<char>());
      } else if (!is_type_complete_v<std::remove_pointer_t<T>>) {
        // No void ptr type in llvm ir
        return llvm::PointerType::getUnqual(toLLVM<char>());
      } else {
        return llvm::PointerType::getUnqual(toLLVM<std::remove_cv_t<std::remove_pointer_t<T>>>());
      }
    } else if constexpr (std::is_integral_v<T>) {
      //      LOG(INFO) << "Integral";
      return llvm::Type::getIntNTy(getLLVMContext(), sizeof(T) * 8);
    } else if constexpr (std::is_same_v<T, double>) {
      //      LOG(INFO) << "Double";
      return llvm::Type::getDoubleTy(getLLVMContext());
    } else if constexpr (std::is_same_v<T, float>) {
      //      LOG(INFO) << "Float";
      return llvm::Type::getFloatTy(getLLVMContext());
    }
    //    else if constexpr (std::is_same_v<T, std::string>) {
    //      return CreateStringStruct();
    //    }
    //    else if constexpr (std::is_same_v<T, pb>) {
    //      auto charPtrType = toLLVM<char *>();
    //      return llvm::StructType::get(getLLVMContext(),
    //                                   {charPtrType, charPtrType});
    //    } else if constexpr (!std::is_same_v<T, std::stringstream> &&
    //                         std::is_aggregate_v<T>) {
    //      if constexpr (num_aggregate_fields_v<T> == 2) {
    //        return llvm::StructType::get(getLLVMContext(),
    //                                     {toLLVM<decltype(extract<T, 0>())>(),
    //                                      toLLVM<decltype(extract<T, 1>())>()});
    //      } else {
    //        return toLLVMUnknown<T>();
    //      }
    //    }
    else {
      LOG(INFO) << "Unknown";
      return toLLVMUnknown<T>();
    }
  }

 protected:
  std::map<std::string, llvm::Function *> availableFunctions;

 private:
  llvm::StructType *variadicArgs_vaList_struct_t = nullptr;
};

#endif  // DCDS_LLVM_CONTEXT_HPP
