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

#include "dcds/codegen/llvm-codegen/llvm-codegen-statement.hpp"

#include "dcds/builder/function-builder.hpp"
#include "dcds/codegen/llvm-codegen/expression-codegen/llvm-expression-visitor.hpp"
#include "dcds/codegen/llvm-codegen/functions.hpp"
#include "dcds/codegen/llvm-codegen/utils/conditionals.hpp"
#include "dcds/codegen/llvm-codegen/utils/loops.hpp"
#include "dcds/codegen/llvm-codegen/utils/phi-node.hpp"
#include "dcds/indexes/index-functions.hpp"

static constexpr bool print_debug_log = false;

namespace dcds {
using namespace dcds::expressions;

void LLVMCodegenStatement::buildStatement(Statement *stmt) {
  // FIXME: what about scoping of temporary variables??
  LOG_IF(INFO, print_debug_log) << "[LLVMCodegenStatement] buildStatement: " << stmt->stType;

  // READ, UPDATE, YIELD, TEMP_VAR_ADD, CONDITIONAL_STATEMENT, CALL
  if (stmt->stType == dcds::statementType::READ) {
    buildStatement_Read(stmt);
  } else if (stmt->stType == dcds::statementType::READ_INDEXED) {
    buildStatement_ReadIndexed(stmt);
  } else if (stmt->stType == dcds::statementType::INSERT_INDEXED) {
    buildStatement_InsertIndexed(stmt);
  } else if (stmt->stType == dcds::statementType::REMOVE_INDEXED) {
    buildStatement_RemoveIndexed(stmt);
  } else if (stmt->stType == dcds::statementType::UPDATE) {
    buildStatement_Update(stmt);

  } else if (stmt->stType == dcds::statementType::TEMP_VAR_ASSIGN) {
    auto tempVarAssignSt = reinterpret_cast<TempVarAssignStatement *>(stmt);

    llvm::Value *source = LLVMExpressionVisitor::gen(build_ctx, tempVarAssignSt->source);
    llvm::Value *destination = LLVMExpressionVisitor::gen(build_ctx, tempVarAssignSt->dest);
    IRBuilder()->CreateStore(source, destination);

  } else if (stmt->stType == dcds::statementType::LOG_STRING) {
    buildStatement_LogString(stmt);

  } else if (stmt->stType == dcds::statementType::YIELD) {
    buildStatement_Yield(stmt);

  } else if (stmt->stType == dcds::statementType::CREATE) {
    buildStatement_Create(stmt);

  } else if (stmt->stType == dcds::statementType::METHOD_CALL) {
    buildStatement_MethodCall(stmt);

  } else if (stmt->stType == dcds::statementType::CONDITIONAL_STATEMENT) {
    this->buildStatement_ConditionalStatement(stmt);

  } else if (stmt->stType == dcds::statementType::FOR_LOOP) {
    this->buildStatement_ForLoop(stmt);

  } else if (stmt->stType == dcds::statementType::WHILE_LOOP) {
    this->buildStatement_WhileLoop(stmt);

  } else if (stmt->stType == dcds::statementType::DO_WHILE_LOOP) {
    this->buildStatement_DoWhileLoop(stmt);

  } else if (stmt->stType == dcds::statementType::CC_LOCK) {
    buildStatement_CC_Lock(stmt);

  } else {
    LOG(FATAL) << "unknown statement type: " << stmt->stType;
  }
}

void LLVMCodegenStatement::buildStatement_ForLoop(dcds::Statement *stmt) {
  auto loopStatement = reinterpret_cast<ForLoopStatement *>(stmt);
  CHECK(!loopStatement->body->statements.empty()) << "Build for loop requested but loop body is empty";

  auto isLastStatementInBlock = (build_ctx->current_sb->statements.back() == stmt);

  auto F = IRBuilder()->GetInsertBlock()->getParent();
  llvm::Value *loopVar = LLVMExpressionVisitor::gen(build_ctx, loopStatement->loop_var);

  BasicBlock *LoopCondBlock = BasicBlock::Create(ctx(), "for.loop.cond", F);
  BasicBlock *LoopBodyBlock = BasicBlock::Create(ctx(), "for.loop.body", F);
  //  BasicBlock *AfterLoopBlock = BasicBlock::Create(ctx(), "after.loop", F);
  BasicBlock *AfterLoopBlock = isLastStatementInBlock ? build_ctx->getFunctionContext()->GetReturnBlock()
                                                      : BasicBlock::Create(ctx(), "for.after.loop", F);

  IRBuilder()->CreateBr(LoopCondBlock);
  IRBuilder()->SetInsertPoint(LoopCondBlock);

  llvm::Value *condExpr = LLVMExpressionVisitor::gen(build_ctx, loopStatement->cond_expr);

  IRBuilder()->CreateCondBr(condExpr, LoopBodyBlock, AfterLoopBlock);
  IRBuilder()->SetInsertPoint(LoopBodyBlock);

  // Body of the loop:
  LLVMScopedContext loop_body_ctx(this->build_ctx, loopStatement->body);
  LLVMCodegenStatement::gen(&loop_body_ctx);

  // Increment loop variable
  llvm::Value *iterExpr = LLVMExpressionVisitor::gen(build_ctx, loopStatement->iteration_expr);
  IRBuilder()->CreateStore(iterExpr, loopVar);

  // Create unconditional branch back to the loop condition block
  IRBuilder()->CreateBr(LoopCondBlock);

  // Set insertion point to after the loop
  IRBuilder()->SetInsertPoint(AfterLoopBlock);
}

void LLVMCodegenStatement::buildStatement_WhileLoop(dcds::Statement *stmt) {
  auto loopStatement = reinterpret_cast<WhileLoopStatement *>(stmt);
  CHECK(!loopStatement->body->statements.empty()) << "Build for loop requested but loop body is empty";

  auto isLastStatementInBlock = (build_ctx->current_sb->statements.back() == stmt);

  auto F = IRBuilder()->GetInsertBlock()->getParent();

  BasicBlock *LoopCondBlock = BasicBlock::Create(ctx(), "while.loop.cond", F);
  BasicBlock *LoopBodyBlock = BasicBlock::Create(ctx(), "while.loop.body", F);
  BasicBlock *AfterLoopBlock = isLastStatementInBlock ? build_ctx->getFunctionContext()->GetReturnBlock()
                                                      : BasicBlock::Create(ctx(), "while.after.loop", F);

  IRBuilder()->CreateBr(LoopCondBlock);
  IRBuilder()->SetInsertPoint(LoopCondBlock);

  llvm::Value *condExpr = LLVMExpressionVisitor::gen(build_ctx, loopStatement->cond_expr);

  IRBuilder()->CreateCondBr(condExpr, LoopBodyBlock, AfterLoopBlock);
  IRBuilder()->SetInsertPoint(LoopBodyBlock);

  // Body of the loop:
  LLVMScopedContext loop_body_ctx(this->build_ctx, loopStatement->body);
  LLVMCodegenStatement::gen(&loop_body_ctx);

  // Create unconditional branch back to the loop condition block
  IRBuilder()->CreateBr(LoopCondBlock);

  // Set insertion point to after the loop
  IRBuilder()->SetInsertPoint(AfterLoopBlock);
}

void LLVMCodegenStatement::buildStatement_DoWhileLoop(dcds::Statement *stmt) {
  auto loopStatement = reinterpret_cast<DoWhileLoopStatement *>(stmt);
  CHECK(!loopStatement->body->statements.empty()) << "Build for loop requested but loop body is empty";

  auto isLastStatementInBlock = (build_ctx->current_sb->statements.back() == stmt);

  auto F = IRBuilder()->GetInsertBlock()->getParent();

  BasicBlock *LoopBodyBlock = BasicBlock::Create(ctx(), "doWhile.loop.body", F);
  BasicBlock *AfterLoopBlock = isLastStatementInBlock ? build_ctx->getFunctionContext()->GetReturnBlock()
                                                      : BasicBlock::Create(ctx(), "doWhile.after.loop", F);

  // Body of the loop:
  IRBuilder()->CreateBr(LoopBodyBlock);
  IRBuilder()->SetInsertPoint(LoopBodyBlock);

  LLVMScopedContext loop_body_ctx(this->build_ctx, loopStatement->body);
  LLVMCodegenStatement::gen(&loop_body_ctx);

  llvm::Value *condExpr = LLVMExpressionVisitor::gen(build_ctx, loopStatement->cond_expr);

  IRBuilder()->CreateCondBr(condExpr, LoopBodyBlock, AfterLoopBlock);

  // Set insertion point to after the loop
  IRBuilder()->SetInsertPoint(AfterLoopBlock);
}

void LLVMCodegenStatement::buildStatement_ConditionalStatement(Statement *stmt) {
  auto conditionalStatement = reinterpret_cast<ConditionalStatement *>(stmt);
  CHECK(!conditionalStatement->ifBlock->statements.empty())
      << "Build conditional statement requested but then block is empty";

  auto hasElseBlock = !(conditionalStatement->elseBLock->statements.empty());

  auto isLastStatementInBlock = (build_ctx->current_sb->statements.back() == stmt);

  llvm::Value *exprResult = LLVMExpressionVisitor::gen(build_ctx, conditionalStatement->expr);

  auto genIf = build_ctx->codegen->gen_if(
      exprResult, isLastStatementInBlock ? build_ctx->getFunctionContext()->GetReturnBlock() : nullptr)([&]() {
    // make a new scopedContext!

    LLVMScopedContext if_build_ctx(this->build_ctx, conditionalStatement->ifBlock);
    LLVMCodegenStatement::gen(&if_build_ctx);

    //    FunctionBuildContext fnCtx_IfBlock(fnCtx);
    //    fnCtx_IfBlock.sb = conditionalStatement->ifBlock;

    //    for (auto &if_stmt : conditionalStatement->ifBlock->statements) {
    //      this->buildStatement(builder, fnCtx_IfBlock, if_stmt);
    //    }
  });

  if (hasElseBlock) {
    genIf.gen_else([&]() {
      LLVMScopedContext else_build_ctx(this->build_ctx, conditionalStatement->elseBLock);
      LLVMCodegenStatement::gen(&else_build_ctx);
      //      FunctionBuildContext fnCtx_elseBlock(fnCtx);
      //      fnCtx_elseBlock.sb = conditionalStatement->elseBLock;

      //      for (auto &elseStmt : conditionalStatement->elseBLock->statements) {
      //        this->buildStatement(builder, fnCtx_elseBlock, elseStmt);
      //      }
    });
  }
}

llvm::Value *LLVMCodegenStatement::getArg_txnManager() {
  return build_ctx->current_function_ctx->getArgumentByPosition(0);
}
llvm::Value *LLVMCodegenStatement::getArg_mainRecord() {
  return build_ctx->current_function_ctx->getArgumentByPosition(1);
}
llvm::Value *LLVMCodegenStatement::getArg_txn() { return build_ctx->current_function_ctx->getArgumentByPosition(2); }

void LLVMCodegenStatement::buildStatement_Read(Statement *stmt) {
  auto readStmt = reinterpret_cast<ReadStatement *>(stmt);
  CHECK(build_ctx->current_builder->hasAttribute(readStmt->source_attr)) << "read attribute does not exists";

  auto txnManager = getArg_txnManager();
  auto mainRecord = getArg_mainRecord();
  auto txn = getArg_txn();

  llvm::Value *destination = LLVMExpressionVisitor::gen(build_ctx, readStmt->dest_expr);

  //    extern "C" void table_read_attribute(
  //    void* _txnManager, uintptr_t _mainRecord,
  //    void* txn, void* dst, uint attributeIdx);

  build_ctx->codegen->gen_call(
      table_read_attribute,
      {txnManager, mainRecord, txn, destination,
       build_ctx->codegen->createSizeT(build_ctx->current_builder->getAttributeIndex(readStmt->source_attr))},
      Type::getVoidTy(ctx()));
}

llvm::Value *LLVMCodegenStatement::call_index_find(valueType key_type, llvm::Value *base_record_ptr,
                                                   llvm::Value *index_key) {
  auto return_uintptr_type = Type::getInt64Ty(ctx());
  switch (key_type) {
    case valueType::INT64:
      return build_ctx->codegen->gen_call(index_find<int64_t>, {base_record_ptr, index_key}, return_uintptr_type);
    case valueType::INT32:
      return build_ctx->codegen->gen_call(index_find<int32_t>, {base_record_ptr, index_key}, return_uintptr_type);
    case valueType::FLOAT:
      return build_ctx->codegen->gen_call(index_find<float>, {base_record_ptr, index_key}, return_uintptr_type);
    case valueType::DOUBLE:
      return build_ctx->codegen->gen_call(index_find<double>, {base_record_ptr, index_key}, return_uintptr_type);
    case valueType::RECORD_PTR:
      return build_ctx->codegen->gen_call(index_find<uintptr_t>, {base_record_ptr, index_key}, return_uintptr_type);
    case valueType::VOID:
    case valueType::BOOL:
      assert(false);
      break;
  }
}

llvm::Value *LLVMCodegenStatement::call_index_insert(valueType key_type, llvm::Value *base_record_ptr,
                                                     llvm::Value *index_key, llvm::Value *index_value) {
  auto return_bool_type = Type::getInt1Ty(ctx());
  switch (key_type) {
    case valueType::INT64:
      return build_ctx->codegen->gen_call(index_insert<int64_t>, {base_record_ptr, index_key, index_value},
                                          return_bool_type);
    case valueType::INT32:
      return build_ctx->codegen->gen_call(index_insert<int32_t>, {base_record_ptr, index_key, index_value},
                                          return_bool_type);
    case valueType::FLOAT:
      return build_ctx->codegen->gen_call(index_insert<float>, {base_record_ptr, index_key, index_value},
                                          return_bool_type);
    case valueType::DOUBLE:
      return build_ctx->codegen->gen_call(index_insert<double>, {base_record_ptr, index_key, index_value},
                                          return_bool_type);
    case valueType::RECORD_PTR:
      return build_ctx->codegen->gen_call(index_insert<uintptr_t>, {base_record_ptr, index_key, index_value},
                                          return_bool_type);
    case valueType::VOID:
    case valueType::BOOL:
      assert(false);
      break;
  }
}

llvm::Value *LLVMCodegenStatement::call_index_remove(valueType key_type, llvm::Value *base_record_ptr,
                                                     llvm::Value *index_key) {
  auto *return_void_type = Type::getVoidTy(ctx());
  switch (key_type) {
    case valueType::INT64:
      return build_ctx->codegen->gen_call(index_remove<int64_t>, {base_record_ptr, index_key}, return_void_type);
    case valueType::INT32:
      return build_ctx->codegen->gen_call(index_remove<int32_t>, {base_record_ptr, index_key}, return_void_type);
    case valueType::FLOAT:
      return build_ctx->codegen->gen_call(index_remove<float>, {base_record_ptr, index_key}, return_void_type);
    case valueType::DOUBLE:
      return build_ctx->codegen->gen_call(index_remove<double>, {base_record_ptr, index_key}, return_void_type);
    case valueType::RECORD_PTR:
      return build_ctx->codegen->gen_call(index_remove<uintptr_t>, {base_record_ptr, index_key}, return_void_type);
    case valueType::VOID:
    case valueType::BOOL:
      assert(false);
      break;
  }
}

// RemoveIndexedStatement
void LLVMCodegenStatement::buildStatement_RemoveIndexed(Statement *stmt) {
  auto removeStmt = reinterpret_cast<RemoveIndexedStatement *>(stmt);
  CHECK(build_ctx->current_builder->hasAttribute(removeStmt->source_attr)) << "index attribute does not exists";

  auto sourceAttribute = build_ctx->current_builder->getAttribute(removeStmt->source_attr);
  CHECK(sourceAttribute->type_category == ATTRIBUTE_TYPE_CATEGORY::ARRAY_LIST);
  auto attributeList = std::static_pointer_cast<AttributeList>(sourceAttribute);

  auto txnManager = getArg_txnManager();
  auto mainRecord = getArg_mainRecord();
  auto txn = getArg_txn();

  llvm::Value *index_key = LLVMExpressionVisitor::gen(build_ctx, removeStmt->index_expr);
  if (index_key->getType()->isPointerTy()) {
    index_key =
        IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(removeStmt->index_expr->getResultType()), index_key);
  }

  auto *base_record = build_ctx->codegen->allocateOneVar("idx_ins_tmp_arTy", valueType::RECORD_PTR);

  build_ctx->codegen->gen_call(
      table_read_attribute,
      {txnManager, mainRecord, txn, base_record,
       build_ctx->codegen->createSizeT(build_ctx->current_builder->getAttributeIndex(removeStmt->source_attr))},
      Type::getVoidTy(ctx()));

  auto indexedList = std::static_pointer_cast<AttributeIndexedList>(attributeList);
  assert(!indexedList->is_primitive_type);

  auto *base_record_ptr =
      IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(valueType::RECORD_PTR), base_record);

  // FIXME: What about CC? removing key and it fails after?

  call_index_remove(indexedList->type, base_record_ptr, index_key);
  // auto *remove_success = call_index_remove(indexedList->type, base_record_ptr, index_key);
  // gen_conditional_abort(remove_success);
}

// InsertIndexedStatement
void LLVMCodegenStatement::buildStatement_InsertIndexed(Statement *stmt) {
  auto insStmt = reinterpret_cast<InsertIndexedStatement *>(stmt);
  CHECK(build_ctx->current_builder->hasAttribute(insStmt->source_attr)) << "index attribute does not exists";

  auto sourceAttribute = build_ctx->current_builder->getAttribute(insStmt->source_attr);
  CHECK(sourceAttribute->type_category == ATTRIBUTE_TYPE_CATEGORY::ARRAY_LIST);
  auto attributeList = std::static_pointer_cast<AttributeList>(sourceAttribute);

  auto txnManager = getArg_txnManager();
  auto mainRecord = getArg_mainRecord();
  auto txn = getArg_txn();

  llvm::Value *value_rec = LLVMExpressionVisitor::gen(build_ctx, insStmt->value_expr);
  llvm::Value *index_key = LLVMExpressionVisitor::gen(build_ctx, insStmt->index_expr);

  auto *base_record = build_ctx->codegen->allocateOneVar("idx_ins_tmp_arTy", valueType::RECORD_PTR);

  build_ctx->codegen->gen_call(
      table_read_attribute,
      {txnManager, mainRecord, txn, base_record,
       build_ctx->codegen->createSizeT(build_ctx->current_builder->getAttributeIndex(insStmt->source_attr))},
      Type::getVoidTy(ctx()));

  auto indexedList = std::static_pointer_cast<AttributeIndexedList>(attributeList);
  assert(!indexedList->is_primitive_type);

  auto *base_record_ptr =
      IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(valueType::RECORD_PTR), base_record);

  // Here, instead of find, do an insert. what if fails?
  //  auto *record_ptr = call_index_find(indexedList->type, base_record_ptr, index_key);
  //  IRBuilder()->CreateStore(record_ptr, destination);
  //  build_ctx->codegen->gen_call(printUInt64, {value_rec});

  auto *value_ptr =
      IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(insStmt->value_expr->getType()), value_rec);

  if (index_key->getType()->isPointerTy()) {
    index_key =
        IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(insStmt->index_expr->getResultType()), index_key);
  }

  auto *ins_success = call_index_insert(indexedList->type, base_record_ptr, index_key, value_ptr);
  gen_conditional_abort(ins_success);
}

void LLVMCodegenStatement::buildStatement_ReadIndexed(Statement *stmt) {
  auto readStmt = reinterpret_cast<ReadIndexedStatement *>(stmt);
  CHECK(build_ctx->current_builder->hasAttribute(readStmt->source_attr)) << "read attribute does not exists";
  auto sourceAttribute = build_ctx->current_builder->getAttribute(readStmt->source_attr);
  CHECK(sourceAttribute->type_category == ATTRIBUTE_TYPE_CATEGORY::ARRAY_LIST);
  auto attributeList = std::static_pointer_cast<AttributeList>(sourceAttribute);

  auto txnManager = getArg_txnManager();
  auto mainRecord = getArg_mainRecord();
  auto txn = getArg_txn();
  llvm::Value *destination = LLVMExpressionVisitor::gen(build_ctx, readStmt->dest_expr);

  auto *base_record = build_ctx->codegen->allocateOneVar("idx_read_tmp_arTy", valueType::RECORD_PTR);

  build_ctx->codegen->gen_call(
      table_read_attribute,
      {txnManager, mainRecord, txn, base_record,
       build_ctx->codegen->createSizeT(build_ctx->current_builder->getAttributeIndex(readStmt->source_attr))},
      Type::getVoidTy(ctx()));

  llvm::Value *index_key = LLVMExpressionVisitor::gen(build_ctx, readStmt->index_expr);
  if (index_key->getType()->isPointerTy()) {
    index_key =
        IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(readStmt->index_expr->getResultType()), index_key);
  }

  // now we have the pointer to actual table in record_ptr
  if (readStmt->integer_indexed) {
    auto attributeArray = std::static_pointer_cast<AttributeArray>(attributeList);

    if (attributeArray->is_primitive_type) {
      assert(false);  // directly read the thing in the nth index.
    } else {
      auto *base_record_ptr =
          IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(valueType::RECORD_PTR), base_record);
      auto *record_ptr = build_ctx->codegen->gen_call(
          table_get_nth_record, {txnManager, base_record_ptr, txn, index_key}, Type::getInt64Ty(ctx()));
      IRBuilder()->CreateStore(record_ptr, destination);
    }

  } else {
    // TODO: for indexed-types.
    auto indexedList = std::static_pointer_cast<AttributeIndexedList>(attributeList);
    assert(!indexedList->is_primitive_type);

    auto *base_record_ptr =
        IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(valueType::RECORD_PTR), base_record);

    auto *record_ptr = call_index_find(indexedList->type, base_record_ptr, index_key);

    IRBuilder()->CreateStore(record_ptr, destination);
  }
}

void LLVMCodegenStatement::buildStatement_Update(Statement *stmt) {
  auto txnManager = getArg_txnManager();
  auto mainRecord = getArg_mainRecord();
  auto txn = getArg_txn();

  auto updStmt = reinterpret_cast<UpdateStatement *>(stmt);
  CHECK(build_ctx->current_builder->hasAttribute(updStmt->destination_attr)) << "write attribute does not exists";

  llvm::Value *updateSource;
  llvm::Value *source = LLVMExpressionVisitor::gen(build_ctx, updStmt->source_expr);

  if (source->getType()->isPointerTy()) {
    updateSource = IRBuilder()->CreateBitCast(source, llvm::Type::getInt8PtrTy(ctx()));
  } else {
    // NOTE: because are update function expects a void* to the src, we need to create a temporary allocation.
    llvm::AllocaInst *allocaInst = IRBuilder()->CreateAlloca(source->getType());
    IRBuilder()->CreateStore(source, allocaInst);
    updateSource = IRBuilder()->CreateBitCast(allocaInst, llvm::Type::getInt8PtrTy(ctx()));
  }

  //    extern "C" void table_write_attribute(void* _txnManager, uintptr_t _mainRecord, void*
  //    txnPtr, void* src, uint attributeIdx);

  build_ctx->codegen->gen_call(
      table_write_attribute,
      {txnManager, mainRecord, txn, updateSource,
       build_ctx->codegen->createSizeT(build_ctx->current_builder->getAttributeIndex(updStmt->destination_attr))},
      Type::getVoidTy(ctx()));
}

void LLVMCodegenStatement::buildStatement_LogString(Statement *stmt) {
  auto logStmt = reinterpret_cast<LogStringStatement *>(stmt);
  auto stringPtr = build_ctx->codegen->createStringConstant(logStmt->log_string, "");
  std::vector<llvm::Value *> generated_expr{stringPtr};

  for (auto &e : logStmt->args) {
    auto v = LLVMExpressionVisitor::gen(build_ctx, e);
    if (v->getType()->isPointerTy()) {
      v = IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(e->getResultType()), v);
    }
    generated_expr.emplace_back(v);
  }

  IRBuilder()->CreateCall(build_ctx->codegen->getFunction_printf(), generated_expr);
}
void LLVMCodegenStatement::buildStatement_Yield([[maybe_unused]] Statement *stmt) {
  auto returnStmt = reinterpret_cast<ReturnStatement *>(stmt);

  if (returnStmt->expr) {
    llvm::Value *exprResult = LLVMExpressionVisitor::gen(build_ctx, returnStmt->expr);
    llvm::Value *retValue;

    if (exprResult->getType()->isPointerTy()) {
      retValue = IRBuilder()->CreateLoad(
          build_ctx->codegen->DcdsToLLVMType(build_ctx->current_fb->getReturnValueType()), exprResult);

    } else {
      retValue = exprResult;
    }
    // Option 2:
    // if (llvm::isa<llvm::AllocaInst>(exprResult)) {
    //   auto *allocaInst = llvm::cast<llvm::AllocaInst>(exprResult);
    //   retValue = IRbuilder()->CreateLoad(fnCtx.fn->getReturnType(), allocaInst);
    // }

    IRBuilder()->CreateStore(retValue, build_ctx->getFunctionContext()->getArgumentByPosition(3));
  }

  IRBuilder()->CreateBr(build_ctx->getFunctionContext()->GetReturnBlock());
}
void LLVMCodegenStatement::buildStatement_Create(Statement *stmt) {
  auto txnManager = getArg_txnManager();
  // auto mainRecord = getArg_mainRecord();
  auto txn = getArg_txn();

  auto insStmt = reinterpret_cast<InsertStatement *>(stmt);

  // we need to call the constructor of subType
  // should be `builder.getName() + "_constructor"`
  assert(build_ctx->current_builder->hasRegisteredType(insStmt->type_name));
  auto subtype = build_ctx->current_builder->getRegisteredType(insStmt->type_name);
  auto constructorName = subtype->getName() + "_constructor_inner";
  assert(build_ctx->codegen->userFunctions.contains(constructorName));
  auto dst = build_ctx->getFunctionContext()->getVariable(insStmt->destination_var);

  llvm::Value *dsContainer =
      build_ctx->codegen->gen_call(build_ctx->codegen->userFunctions[constructorName], {txnManager, txn});

  // extern "C" uintptr_t extractRecordFromDsContainer(void* container);
  llvm::Value *newRecord = build_ctx->codegen->gen_call(extractRecordFromDsContainer, {dsContainer});
  IRBuilder()->CreateStore(newRecord, dst);
}
void LLVMCodegenStatement::buildStatement_MethodCall(Statement *stmt) {
  auto txnManager = getArg_txnManager();
  // auto mainRecord = getArg_mainRecord();
  auto txn = getArg_txn();

  auto methodStmt = reinterpret_cast<MethodCallStatement *>(stmt);
  auto fn_instance = methodStmt->function_instance;

  // {ds_name}_{function_name}_inner
  // 'inner' because txn is already here from wrapped outer function.

  std::string function_name = fn_instance->builder->getName() + "_" + fn_instance->getName() + "_inner";

  assert(build_ctx->codegen->userFunctions.contains(function_name));

  // NOTE: (methodStmt->refVarName.empty() && doesReturn):  function returns but value is unused

  // construct arguments. every inner function expects:
  // ptrType, uintPtrType, ptrType (txnManager, mainRecord:subtype, txnPtr)
  // mainRecord is the one supplied in callStatement
  // optionally 4th arg, if the function returns, then a returnValue pointer.

  std::vector<llvm::Value *> callArgs;
  callArgs.push_back(txnManager);

  assert(build_ctx->getFunctionContext()->getVariable(methodStmt->referenced_type_variable));

  auto ref_var = build_ctx->getFunctionContext()->getVariable(methodStmt->referenced_type_variable);
  if (ref_var->getType()->isPointerTy()) {
    callArgs.push_back(IRBuilder()->CreateLoad(llvm::Type::getInt64Ty(ctx()), ref_var));
  } else {
    callArgs.push_back(ref_var);
  }

  callArgs.push_back(txn);

  // --- call statement return value
  bool doesReturn = true;
  llvm::Value *returnValueArg;
  llvm::Value *ret_dest_expr;

  if (fn_instance->getReturnValueType() == valueType::VOID) {
    doesReturn = false;
  }

  CHECK((!(methodStmt->has_return_dest)) || (methodStmt->has_return_dest && doesReturn))
      << "callee(" << build_ctx->current_fb->getName() << ") expects return but function(" << fn_instance->getName()
      << ") does not return";

  if (doesReturn) {
    ret_dest_expr = LLVMExpressionVisitor::gen(build_ctx, methodStmt->return_dest);
    if (isa<PointerType>(ret_dest_expr->getType())) {
      returnValueArg = ret_dest_expr;
    } else {
      returnValueArg = build_ctx->codegen->allocateOneVar(methodStmt->function_instance->getName() + "_return_dest",
                                                          methodStmt->function_instance->getReturnValueType());
    }
    callArgs.push_back(returnValueArg);
  }
  // ------

  for (auto i = 0; i < fn_instance->getArguments().size(); i++) {
    auto &fArg = methodStmt->function_arguments[i];
    llvm::Value *exprResult = LLVMExpressionVisitor::gen(build_ctx, fArg);

    if (fn_instance->getArguments()[i]->is_reference_type) {
      if (exprResult->getType()->isPointerTy()) {
        callArgs.push_back(exprResult);
      } else {
        if (llvm::isa<llvm::AllocaInst>(exprResult)) {
          callArgs.push_back(IRBuilder()->CreateBitCast(exprResult, exprResult->getType()->getPointerTo()));
        } else {
          assert(false);  // Isn't it same as isPointerTy()?
          // llvm::AllocaInst *allocaInst = IRBuilder()->CreateAlloca(exprResult->getType());
          // IRBuilder()->CreateStore(exprResult, allocaInst);
          // callArgs.push_back(IRBuilder()->CreateBitCast(allocaInst, exprResult->getType()->getPointerTo()));
        }
      }

    } else {
      if (llvm::isa<llvm::AllocaInst>(exprResult)) {
        // Temporary variables might be AllocaInst, and need to be loaded before use or passing to function by value.
        auto *allocaInst = llvm::cast<llvm::AllocaInst>(exprResult);
        llvm::Value *loadedTmpVar =
            IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(fArg->getResultType()), allocaInst);
        callArgs.push_back(loadedTmpVar);

      } else {
        // It's not an AllocaInst
        callArgs.push_back(exprResult);
      }
    }
  }

  llvm::Value *ret = build_ctx->codegen->gen_call(build_ctx->codegen->userFunctions[function_name], callArgs);
  auto genIf = build_ctx->codegen->gen_if(IRBuilder()->CreateNot(ret))([&]() {
    IRBuilder()->CreateStore(build_ctx->codegen->createFalse(), build_ctx->getFunctionContext()->getReturnVariable());
    IRBuilder()->CreateBr(build_ctx->getFunctionContext()->GetReturnBlock());
  });

  if (doesReturn) {
    if (!(isa<PointerType>(ret_dest_expr->getType()))) {
      auto retLoadIns = IRBuilder()->CreateLoad(
          build_ctx->codegen->DcdsToLLVMType(methodStmt->function_instance->getReturnValueType()), returnValueArg);
      IRBuilder()->CreateStore(retLoadIns, ret_dest_expr);
    }
  }
}

void LLVMCodegenStatement::buildStatement_CC_Lock(Statement *stmt) {
  auto txnManager = getArg_txnManager();
  auto mainRecord = getArg_mainRecord();
  auto txn = getArg_txn();

  auto lockStmt = reinterpret_cast<LockStatement2 *>(stmt);

  // FIXME: is the mainRecord the record we want to lock?

  // void* _txnManager, void* txnPtr, uintptr_t record, size_t attributeIdx
  llvm::Value *ret = build_ctx->codegen->gen_call(
      // lockStmt->stType == dcds::statementType::CC_LOCK_SHARED ? lock_shared : lock_exclusive,
      lockStmt->is_exclusive ? lock_exclusive : lock_shared,
      {txnManager, txn,
       mainRecord /*, this->createSizeT(build_ctx->current_builder->getAttributeIndex(lockStmt->attribute))*/},
      build_ctx->codegen->DcdsToLLVMType(valueType::BOOL));

  // (ret == false) goto returnBB;
  gen_conditional_abort(ret);
}

void LLVMCodegenStatement::gen_conditional_abort(llvm::Value *do_continue) {
  // (ret == false) goto returnBB;
  auto genIf = build_ctx->codegen->gen_if(IRBuilder()->CreateNot(do_continue))([&]() {
    IRBuilder()->CreateStore(build_ctx->codegen->createFalse(), build_ctx->getFunctionContext()->getReturnVariable());

    IRBuilder()->CreateBr(build_ctx->getFunctionContext()->GetReturnBlock());
  });
}

}  // namespace dcds
