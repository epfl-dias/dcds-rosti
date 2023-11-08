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

#include <llvm/IR/IRBuilder.h>

#include "dcds/codegen/llvm-codegen/utils/conditionals.hpp"

void if_then::openCase(const value_t &cond) {
  auto Builder = context->getBuilder();
  llvm::LLVMContext &llvmContext = context->getLLVMContext();
  llvm::Function *F = Builder->GetInsertBlock()->getParent();

  auto ThenBB = llvm::BasicBlock::Create(llvmContext, "IfThen", F);
  ElseBB = llvm::BasicBlock::Create(llvmContext, "IfElse", F);
  if (!AfterBB) {
    AfterBB = llvm::BasicBlock::Create(llvmContext, "IfAfter", F);
  }

  Builder->CreateCondBr(cond, ThenBB, ElseBB);

  Builder->SetInsertPoint(ThenBB);
}

void if_then::closeCase() {
  if (!(context->getBuilder()->GetInsertBlock()->getTerminator())) {
    context->getBuilder()->CreateBr(AfterBB);
  }
}

void if_then::openElseCase() { context->getBuilder()->SetInsertPoint(ElseBB); }

void if_then::closeElseCase() {
  auto Builder = context->getBuilder();
  if (!(context->getBuilder()->GetInsertBlock()->getTerminator())) {
    Builder->CreateBr(AfterBB);
    Builder->SetInsertPoint(AfterBB);
  }
  ElseBB = nullptr;
}
