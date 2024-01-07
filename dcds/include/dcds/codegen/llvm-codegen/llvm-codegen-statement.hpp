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

#ifndef DCDS_LLVM_CODEGEN_STATEMENT_HPP
#define DCDS_LLVM_CODEGEN_STATEMENT_HPP

#include "dcds/codegen/llvm-codegen/llvm-codegen-function.hpp"
#include "dcds/codegen/llvm-codegen/llvm-codegen.hpp"
#include "dcds/codegen/llvm-codegen/llvm-scoped-context.hpp"

namespace dcds {
using namespace llvm;

// class LLVMScopedContext;

class LLVMCodegenStatement {
 public:
  //  static void gen(LLVMCodegen *_codegen, Statement *stmt) {
  //    LLVMCodegenStatement statementGenerator(_codegen);
  //    statementGenerator.buildStatement(builder, fnCtx, stmt);
  //  }
  //
  //  static void gen(std::shared_ptr<StatementBuilder> &sb) {
  //    LLVMCodegenStatement statementGenerator(fnCtx->codegen);
  //    for (auto &s : sb->statements) {
  //      statementGenerator.buildStatement(builder, fnCtx, s);
  //    }
  //  }

  static void gen(LLVMScopedContext *build_ctx) {
    LLVMCodegenStatement statementGenerator(build_ctx);
    for (auto &s : build_ctx->current_sb->statements) {
      statementGenerator.buildStatement(s);
    }
  }

  // LLVMCodegenFunction::gen(this, builder, fb->entryPoint);

 public:
  explicit LLVMCodegenStatement(LLVMScopedContext *_build_ctx) : build_ctx(_build_ctx) {}

  void buildStatement(Statement *stmt);

 private:
  void buildStatement_ConditionalStatement(Statement *stmt);
  void buildStatement_Read(Statement *stmt);
  void buildStatement_ReadIndexed(Statement *stmt);
  void buildStatement_Update(Statement *stmt);
  void buildStatement_LogString(Statement *stmt);
  void buildStatement_Yield(Statement *stmt);
  void buildStatement_Create(Statement *stmt);
  void buildStatement_MethodCall(Statement *stmt);
  void buildStatement_CC_Lock(Statement *stmt);

  void buildStatement_ForLoop(dcds::Statement *stmt);
  void buildStatement_WhileLoop(dcds::Statement *stmt);
  void buildStatement_DoWhileLoop(dcds::Statement *stmt);

  void gen_conditional_abort(llvm::Value *do_continue);

 private:
  inline auto &ctx() { return build_ctx->codegen->getLLVMContext(); }
  inline auto IRBuilder() { return build_ctx->codegen->getBuilder(); }

 private:
  llvm::Value *getArg_txnManager();
  llvm::Value *getArg_mainRecord();
  llvm::Value *getArg_txn();

 private:
  LLVMScopedContext *build_ctx;

 private:
  llvm::Value *call_index_find(valueType key_type, llvm::Value *base_record_ptr, llvm::Value *index_key);
};

}  // namespace dcds

#endif  // DCDS_LLVM_CODEGEN_STATEMENT_HPP
