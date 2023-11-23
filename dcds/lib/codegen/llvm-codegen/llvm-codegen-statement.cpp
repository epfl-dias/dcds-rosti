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

static constexpr bool print_debug_log = false;

namespace dcds {
using namespace dcds::expressions;

void LLVMCodegenStatement::buildStatement(Statement *stmt) {
  // FIXME: what about scoping of temporary variables??
  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen] buildStatement: " << stmt->stType;

  // READ, UPDATE, YIELD, TEMP_VAR_ADD, CONDITIONAL_STATEMENT, CALL
  if (stmt->stType == dcds::statementType::READ) {
    buildStatement_Read(stmt);
  } else if (stmt->stType == dcds::statementType::READ_INDEXED) {
    buildStatement_ReadIndexed(stmt);
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

  } else if (stmt->stType == dcds::statementType::CC_LOCK_SHARED ||
             stmt->stType == dcds::statementType::CC_LOCK_EXCLUSIVE) {
    buildStatement_CC_Lock(stmt);

  } else {
    LOG(FATAL) << "unknown statement type: " << stmt->stType;
  }
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

  // now we have the pointer to actual table in record_ptr
  if (readStmt->integer_indexed) {
    auto attributeArray = std::static_pointer_cast<AttributeArray>(attributeList);
    llvm::Value *array_offset = LLVMExpressionVisitor::gen(build_ctx, readStmt->index_expr);

    if (attributeArray->is_primitive_type) {
      assert(false);  // directly read the thing in the nth index.
    } else {
      auto *base_record_ptr =
          IRBuilder()->CreateLoad(build_ctx->codegen->DcdsToLLVMType(valueType::RECORD_PTR), base_record);
      auto *record_ptr = build_ctx->codegen->gen_call(
          table_get_nth_record, {txnManager, base_record_ptr, txn, array_offset}, Type::getInt64Ty(ctx()));
      IRBuilder()->CreateStore(record_ptr, destination);
    }

    // extern "C" void table_read_attribute_offset(
    //    void* _txnManager,
    //    uintptr_t _mainRecord,
    //    void* txnPtr, void* dst,
    //    size_t attributeIdx,
    //    size_t record_offset)

    // Wrong mainRecord here.
    //    build_ctx->codegen->gen_call(table_read_attribute_offset,
    //                      {txnManager, mainRecord, txn, destination, attribute_idx, array_offset},
    //                      Type::getVoidTy(ctx()));
  } else {
    // TODO: for indexed-types.
    assert(false);
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

  // {ds_name}_{function_name}_inner
  // 'inner' because txn is already here from wrapped outer function.

  std::string function_name =
      methodStmt->function_instance->builder->getName() + "_" + methodStmt->function_instance->getName() + "_inner";

  assert(build_ctx->codegen->userFunctions.contains(function_name));

  // NOTE: (methodStmt->refVarName.empty() && doesReturn):  function returns but value is unused

  // construct arguments. every inner function expects:
  // ptrType, uintPtrType, ptrType (txnManager, mainRecord:subtype, txnPtr)
  // mainRecord is the one supplied in callStatement
  // optionally 4th arg, if the function returns, then a returnValue pointer.

  std::vector<llvm::Value *> callArgs;
  callArgs.push_back(txnManager);

  assert(build_ctx->getFunctionContext()->getVariable(methodStmt->referenced_type_variable));

  // assert(build_ctx->getFunctionContext()->getArgumentByName(methodStmt->referenced_type_variable));

  llvm::Value *referencedRecord =
      IRBuilder()->CreateLoad(llvm::Type::getInt64Ty(ctx()),
                              build_ctx->getFunctionContext()->getVariable(methodStmt->referenced_type_variable));
  callArgs.push_back(referencedRecord);
  callArgs.push_back(txn);

  // --- call statement return value
  bool doesReturn = true;
  llvm::Value *returnValueArg;
  llvm::Value *ret_dest_expr;

  if (methodStmt->function_instance->getReturnValueType() == valueType::VOID) {
    doesReturn = false;
  }

  CHECK((!(methodStmt->has_return_dest)) || (methodStmt->has_return_dest && doesReturn))
      << "callee(" << build_ctx->current_fb->getName() << ") expects return but function("
      << methodStmt->function_instance->getName() << ") does not return";

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

  for (auto &fArg : methodStmt->function_arguments) {
    // FIXME: there might be issue if the function expects pass by reference?
    llvm::Value *exprResult = LLVMExpressionVisitor::gen(build_ctx, fArg);

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

  auto lockStmt = reinterpret_cast<LockStatement *>(stmt);

  // FIXME: is the mainRecord the record we want to lock?

  // void* _txnManager, void* txnPtr, uintptr_t record, size_t attributeIdx
  llvm::Value *ret = build_ctx->codegen->gen_call(
      lockStmt->stType == dcds::statementType::CC_LOCK_SHARED ? lock_shared : lock_exclusive,
      {txnManager, txn,
       mainRecord /*, this->createSizeT(build_ctx->current_builder->getAttributeIndex(lockStmt->attribute))*/},
      build_ctx->codegen->DcdsToLLVMType(valueType::BOOL));

  // (ret == false) goto returnBB;
  auto genIf = build_ctx->codegen->gen_if(IRBuilder()->CreateNot(ret))([&]() {
    IRBuilder()->CreateStore(build_ctx->codegen->createFalse(), build_ctx->getFunctionContext()->getReturnVariable());

    IRBuilder()->CreateBr(build_ctx->getFunctionContext()->GetReturnBlock());
  });
}

}  // namespace dcds
