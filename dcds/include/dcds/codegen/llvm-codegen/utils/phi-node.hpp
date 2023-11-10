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

#ifndef DCDS_GEN_PHI_NODE_HPP
#define DCDS_GEN_PHI_NODE_HPP

#include <llvm/IR/BasicBlock.h>

#include "dcds/codegen/llvm-codegen/llvm-context.hpp"

class PhiNode {
 public:
  inline explicit PhiNode(LLVMCodegenContext *context) : context(context), generated(false) {}

 public:
  void emplace(llvm::Value *incomingValue) {
    CHECK(generated == false) << "Cannot add more values to already generated phi node";
    context->getBuilder()->GetInsertBlock();
    if (!type) {
      type = incomingValue->getType();
    } else {
      CHECK(incomingValue->getType()->getTypeID() == this->type->getTypeID()) << "Type mismatch between phi values";
    }

    incoming_blocks.emplace_back(incomingValue, context->getBuilder()->GetInsertBlock());
  }

  inline llvm::Value *get() {
    if (!generated) generate();
    return node;
  }

 private:
  void generate() {
    node = context->getBuilder()->CreatePHI(type, incoming_blocks.size());
    for (auto &b : incoming_blocks) {
      node->addIncoming(b.first, b.second);
    }
  }

 private:
  LLVMCodegenContext *const context;
  bool generated;

 private:
  std::vector<std::pair<llvm::Value *, llvm::BasicBlock *>> incoming_blocks;
  llvm::Type *type{};
  llvm::PHINode *node{};
};

#endif  // DCDS_GEN_PHI_NODE_HPP
