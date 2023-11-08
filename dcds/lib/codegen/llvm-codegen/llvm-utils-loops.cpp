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

#include "dcds/codegen/llvm-codegen/utils/loops.hpp"

void DoWhile::gen_while(const std::function<value_t()> &cond) && {
  assert(context && "Double do while condition?");
  auto &llvmContext = context->getLLVMContext();
  auto F = context->getBuilder()->GetInsertBlock()->getParent();

  auto BodyBB = llvm::BasicBlock::Create(llvmContext, "body", F);
  auto AfterBB = llvm::BasicBlock::Create(llvmContext, "after", F);

  auto Builder = context->getBuilder();
  Builder->CreateBr(BodyBB);
  Builder->SetInsertPoint(BodyBB);

  body();

  auto conditionValue = cond();

  Builder->CreateCondBr(conditionValue, BodyBB, AfterBB);

  Builder->SetInsertPoint(AfterBB);

  context = nullptr;
}
