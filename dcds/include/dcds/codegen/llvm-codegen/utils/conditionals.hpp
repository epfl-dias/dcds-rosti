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

#ifndef DCDS_LLVM_CODEGEN_UTILS_CONDITIONALS_HPP
#define DCDS_LLVM_CODEGEN_UTILS_CONDITIONALS_HPP

#include "dcds/codegen/llvm-codegen/llvm-context.hpp"

class if_then;

using namespace llvm;

class if_branch {
 private:
  value_t condition;
  LLVMCodegenContext *const context;
  [[maybe_unused]] llvm::BasicBlock *afterBB;

 public:
  inline constexpr if_branch(value_t condition, LLVMCodegenContext *context, llvm::BasicBlock *afterBB)
      : condition(condition), context(context), afterBB(afterBB) {}

 public:
  inline constexpr if_branch(value_t condition, LLVMCodegenContext *context) : if_branch(condition, context, nullptr) {}

 public:
  template <typename IfBlock>
  if_then operator()(IfBlock ifBlock) &&;

  friend class Context;
};

class if_then {
 private:
  LLVMCodegenContext *context;
  llvm::BasicBlock *ElseBB{};
  llvm::BasicBlock *AfterBB;

  void openCase(const value_t &cond);
  void closeCase();

  void openElseCase();
  void closeElseCase();

 public:
  template <typename IfBlock>
  if_then(value_t cond, IfBlock ifBlock, LLVMCodegenContext *context, llvm::BasicBlock *endBB = nullptr)
      : context(context), AfterBB(endBB) {
    openCase(cond);
    ifBlock();
    closeCase();
  }

  if_then(const if_then &) = delete;
  if_then(if_then &&) = delete;

  if_then &operator=(const if_then &) = delete;
  if_then &operator=(if_then &&) = delete;

  template <typename ElseBlock>
  void gen_else(ElseBlock elseBlock) {
    assert(ElseBB && "gen_else* called twice in same gen_if");
    openElseCase();
    elseBlock();
    closeElseCase();
  }

  template <typename ElseBlock>
  if_branch gen_else_if(value_t cond) {
    assert(ElseBB && "gen_else* called twice in same gen_if");
    openElseCase();
    ElseBB = nullptr;
    return {cond, context, AfterBB};
  }

  ~if_then() {
    if (ElseBB) gen_else([]() {});
  }
};

template <typename IfBlock>
if_then if_branch::operator()(IfBlock then) && {
  return {condition, then, context, afterBB};
}

#endif  // DCDS_LLVM_CODEGEN_UTILS_CONDITIONALS_HPP
