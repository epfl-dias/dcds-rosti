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

#ifndef DCDS_LLVM_CODEGEN_FUNCTION_HPP
#define DCDS_LLVM_CODEGEN_FUNCTION_HPP

#include "dcds/builder/function-builder.hpp"
#include "dcds/codegen/llvm-codegen/llvm-codegen.hpp"

namespace dcds {
using namespace llvm;

class LLVMCodegenFunction {
 public:
  //  friend class LLVMCodegenStatement;

  // it can take either statements to codegen (user-facing/defined functions).
  // or it can return something or set the IR to point to current function.

  static auto gen(LLVMCodegen *_codegen, dcds::Builder *builder, std::shared_ptr<FunctionBuilder> &_fb,
                  const std::string &fn_name_prefix = "", const std::string &fn_name_suffix = "",
                  llvm::GlobalValue::LinkageTypes linkageType = llvm::GlobalValue::LinkageTypes::PrivateLinkage) {
    // this is meant for sb/fb based functions directly.
    LLVMCodegenFunction build_fn(_codegen, builder, _fb, fn_name_prefix, fn_name_suffix, linkageType);
    return build_fn.get();
  }

 private:
  LLVMCodegenFunction(LLVMCodegen *_codegen, dcds::Builder *builder, std::shared_ptr<FunctionBuilder> &_fb,
                      const std::string &fn_name_prefix = "", const std::string &fn_name_suffix = "",
                      llvm::GlobalValue::LinkageTypes linkageType = llvm::GlobalValue::LinkageTypes::PrivateLinkage);

  // TODO: for non-builder functions, mainly meant for automatic restore-points in the BB.
  // LLVMCodegenFunction(LLVMCodegen *_codegen, std::string function_name);

  ~LLVMCodegenFunction() {
    LOG(INFO) << "Destructing ~LLVMCodegenFunction";
    dcds::LLVMCodegenFunction::LLVM_verifyFunction(fn);
  }

 public:
  // Utilities
  inline auto getFunctionPointer() {
    CHECK(fn != nullptr) << "Accessing incomplete function";
    return fn;
  }

  inline auto getFunctionName() { return fn_name; }

  inline std::pair<std::string, llvm::Function *> get() { return std::pair{getFunctionName(), getFunctionPointer()}; }

  llvm::Value *getArgumentByName(const std::string &name);
  llvm::Value *getArgumentByPosition(uint32_t index);

  llvm::Value *getVariable(const std::string &name);

  bool doesReturn() { return fb->getReturnValueType() != valueType::VOID; }

  [[nodiscard]] llvm::BasicBlock *GetReturnBlock() const { return returnBB; }
  [[nodiscard]] llvm::BasicBlock *GetEntryBlock() const { return entryBB; }
  llvm::Value *getReturnVariable() { return retval_variable; }

 private:
  llvm::Function *wrapFunctionVariadicArgs(llvm::Function *inner_function,
                                           const std::vector<llvm::Type *> &position_args,
                                           const std::vector<llvm::Type *> &expected_vargs,
                                           std::string wrapped_function_name);

  void allocateFunctionVariables();
  void genFunctionSignature(const std::vector<std::pair<std::string, llvm::Type *>> &pre_args,
                            llvm::GlobalValue::LinkageTypes linkageType, llvm::Type *override_return_type);

 private:
  inline auto &ctx() { return codegen->getLLVMContext(); }
  inline auto IRBuilder() { return codegen->getBuilder(); }
  inline auto Module() { return codegen->getModule(); }
  static inline void LLVM_verifyFunction(llvm::Function *f) { dcds::LLVMCodegen::llvmVerifyFunction(f); }

 private:
  LLVMCodegen *codegen;

 private:
  // bool is_gen_completed; ??

 private:
  std::shared_ptr<FunctionBuilder> fb;
  // std::shared_ptr<StatementBuilder> sb;

  // created function
  llvm::Function *fn;
  std::string fn_name;
  // entry block
  llvm::BasicBlock *entryBB;
  // return block
  llvm::BasicBlock *returnBB;
  // allocated variables at function-level
  std::unordered_map<std::string, llvm::Value *> allocated_vars;

  // Restore points.
  //  LLVMCodegenFunction *previous_fn;
  llvm::BasicBlock *previous_insertBB;

  std::string retval_variable_name;  // ??
  llvm::Value *retval_variable;
};

}  // namespace dcds

#endif  // DCDS_LLVM_CODEGEN_FUNCTION_HPP
