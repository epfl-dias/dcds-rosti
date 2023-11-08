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

#ifndef DCDS_LLVM_CODEGEN_UTILS_LOOPS_HPP
#define DCDS_LLVM_CODEGEN_UTILS_LOOPS_HPP

#include "dcds/codegen/llvm-codegen/llvm-context.hpp"

using namespace llvm;

class While {
 private:
  std::function<value_t()> cond;
  LLVMCodegenContext *context;

 private:
  While(std::function<value_t()> cond, LLVMCodegenContext *context) : cond(std::move(cond)), context(context) {}

  friend class LLVMCodegenContext;

 public:
  template <typename Fbody>
  void operator()(Fbody body) &&;
};

class DoWhile {
 private:
  std::function<void()> body;
  LLVMCodegenContext *context;

 private:
  DoWhile(std::function<void()> body, LLVMCodegenContext *context) : body(std::move(body)), context(context) {
    assert(context);
  }

  friend class LLVMCodegenContext;

 public:
  ~DoWhile() { assert(!context && "gen_do without body?"); }

  void gen_while(const std::function<value_t()> &cond) &&;
};

template <typename Fbody>
void While::operator()(Fbody body) && {
  auto &llvmContext = context->getLLVMContext();
  auto F = context->getBuilder()->GetInsertBlock()->getParent();

  auto CondBB = llvm::BasicBlock::Create(llvmContext, "cond", F);
  auto BodyBB = llvm::BasicBlock::Create(llvmContext, "body", F);
  auto AfterBB = llvm::BasicBlock::Create(llvmContext, "after", F);

  auto Builder = context->getBuilder();
  Builder->CreateBr(CondBB);
  Builder->SetInsertPoint(CondBB);

  auto conditionValue = cond();

  auto loop_cond = Builder->CreateCondBr(conditionValue, BodyBB, AfterBB);

  Builder->SetInsertPoint(BodyBB);

  body(loop_cond);

  Builder->CreateBr(CondBB);

  Builder->SetInsertPoint(AfterBB);
}

#endif  // DCDS_LLVM_CODEGEN_UTILS_LOOPS_HPP
