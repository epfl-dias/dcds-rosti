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

#include "dcds/codegen/llvm-codegen/expression-codegen/llvm-expression-visitor.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "dcds/builder/function-builder.hpp"
#include "dcds/codegen/llvm-codegen/llvm-context.hpp"

using namespace dcds::expressions;

void* LLVMExpressionVisitor::visit(const expressions::IsNullExpression& expr) {
  LOG(INFO) << "LLVMExpressionVisitor::IsNullExpression::visit";

  auto builder = codegenEngine->getBuilder();
  auto subExprEval = static_cast<llvm::Value*>(expr.getExpression()->accept(this));

  // NOTE: this is assuming a lot that isNull will be only of type RECORD_PTR?
  // ALSO, test it for functionArg as base, maybe it will be different subtype.

  llvm::Value* loadedVal = builder->CreateLoad(llvm::Type::getInt64Ty(builder->getContext()), subExprEval);
  return builder->CreateICmpEQ(loadedVal, codegenEngine->createInt64(0));
}

void* LLVMExpressionVisitor::visit(const expressions::IsNotNullExpression& expr) {
  LOG(INFO) << "LLVMExpressionVisitor::IsNotNullExpression::visit";

  auto builder = codegenEngine->getBuilder();
  auto subExprEval = static_cast<llvm::Value*>(expr.getExpression()->accept(this));

  // NOTE: this is assuming a lot that isNotNull will be only of type RECORD_PTR?
  // ALSO, test it for functionArg as base, maybe it will be different subtype.

  // just check if it is a ptr or not, and then choose to load it or not.
  //  llvm::Type* requiredType;
  //
  //  if (llvm::isa<llvm::PointerType>(subExprEval->getType())) {
  //    // It's a pointer type
  //    llvm::PointerType* ptrType = llvm::cast<llvm::PointerType>(subExprEval->getType());
  //    llvm::Type* pointedType = ptrType->getPointerElementType();
  //    // Now, pointedType contains the type the pointer points to
  //  } else {
  //    // It's not a pointer type
  //  }

  llvm::Value* loadedVal = builder->CreateLoad(llvm::Type::getInt64Ty(builder->getContext()), subExprEval);
  auto isNotNull = builder->CreateICmpNE(loadedVal, codegenEngine->createInt64(0));

  // Following is not needed i think.
  // Convert the result to a boolean (i1)
  //  llvm::Value* isNullAsBool = builder->CreateZExtOrTrunc(isNull, llvm::Type::getInt1Ty(builder->getContext()));

  return isNotNull;
}

llvm::Value* LLVMExpressionVisitor::loadValueIfRequired(llvm::Value* in, dcds::valueType dcds_value_type) {
  auto* ret = in;
  if (ret->getType()->isPointerTy()) {
    ret = codegenEngine->getBuilder()->CreateLoad(codegenEngine->DcdsToLLVMType(dcds_value_type), in);
  }
  return ret;
}

void* LLVMExpressionVisitor::visit(const expressions::AddExpression& expr) {
  LOG(INFO) << "LLVMExpressionVisitor::AddExpression::visit";
  auto builder = codegenEngine->getBuilder();

  auto leftValue = static_cast<llvm::Value*>(expr.getLeft()->accept(this));
  auto rightValue = static_cast<llvm::Value*>(expr.getRight()->accept(this));

  leftValue = loadValueIfRequired(leftValue, expr.getLeft()->getResultType());
  rightValue = loadValueIfRequired(rightValue, expr.getRight()->getResultType());

  if (leftValue->getType()->isIntegerTy() && rightValue->getType()->isIntegerTy()) {
    return builder->CreateAdd(leftValue, rightValue);
  } else if (leftValue->getType()->isFloatingPointTy() && rightValue->getType()->isFloatingPointTy()) {
    return builder->CreateFAdd(leftValue, rightValue);
  } else {
    assert(false && "what is the type??");
  }
}

void* LLVMExpressionVisitor::visit(const expressions::SubtractExpression& expr) {
  LOG(INFO) << "LLVMExpressionVisitor::SubtractExpression::visit";
  auto builder = codegenEngine->getBuilder();

  auto leftValue = static_cast<llvm::Value*>(expr.getLeft()->accept(this));
  auto rightValue = static_cast<llvm::Value*>(expr.getRight()->accept(this));

  leftValue = loadValueIfRequired(leftValue, expr.getLeft()->getResultType());
  rightValue = loadValueIfRequired(rightValue, expr.getRight()->getResultType());

  if (leftValue->getType()->isIntegerTy() && rightValue->getType()->isIntegerTy()) {
    return builder->CreateSub(leftValue, rightValue);
  } else if (leftValue->getType()->isFloatingPointTy() && rightValue->getType()->isFloatingPointTy()) {
    return builder->CreateFSub(leftValue, rightValue);
  } else {
    assert(false && "what is the type??");
  }
}

void* LLVMExpressionVisitor::visit(const expressions::IsEvenExpression& isEven) {
  LOG(INFO) << "LLVMExpressionVisitor::IsEvenExpression::visit";
  auto builder = codegenEngine->getBuilder();
  auto subExprEval = static_cast<llvm::Value*>(isEven.getExpression()->accept(this));

  if (!(llvm::isa<llvm::IntegerType>(subExprEval->getType()))) {
    subExprEval->getType()->dump();
    LOG(FATAL) << "sub-expression does not return integer type: " << subExprEval->getType()->getStructName().str();
  }

  auto bitWidth = llvm::cast<llvm::IntegerType>(subExprEval->getType())->getBitWidth();
  auto int0 = builder->getIntN(bitWidth, 0);
  auto int1 = builder->getIntN(bitWidth, 1);

  // 1 - builder->CreateAnd(arg, builder.getInt32(1)): This part of the code takes the input integer value arg (the
  // function argument) and performs a bitwise AND operation with the constant integer 1. In binary, the constant 1 is
  // represented as 0001, and the result of this AND operation will either be 0 or 1. If the input value is even, the
  // least significant bit (LSB) is 0, and the result will be 0. If the input value is odd, the LSB is 1, and the result
  // will be 1.

  // 2 - builder->CreateICmpEQ(...): This part checks if the result of the AND operation is equal to 0. It creates an
  // integer comparison operation (ICmpEQ) that compares the result of the AND operation with 0. If the result is 0, it
  // means the input value is even, and the comparison result is true (1); otherwise, it's false (0).

  // 3 -builder->CreateAnd(..., builder.getInt1(true)): Finally, the result of the comparison operation is combined with
  // builder.getInt1(true), which is a constant true value of type i1 (boolean). This step ensures that the final result
  // of the function is a boolean value (i1) that represents whether the input value is even (true) or not (false).

  llvm::Value* isEvenResult =
      builder->CreateAnd(builder->CreateICmpEQ(builder->CreateAnd(subExprEval, int1), int0), builder->getInt1(true));

  return isEvenResult;
}

void* LLVMExpressionVisitor::visit(const expressions::FunctionArgumentExpression& expr) {
  LOG(INFO) << "LLVMExpressionVisitor::FunctionArgumentExpression::visit";
  assert(expr.var_src_type == VAR_SOURCE_TYPE::FUNCTION_ARGUMENT);

  bool doesReturn = fnCtx->fb->getReturnValueType() != valueType::VOID;
  size_t fn_arg_idx_st = doesReturn ? 4 : 3;  // if it does return, then 3 is the retVal ptr.

  auto source_arg = fnCtx->fn->getArg(fnCtx->fb->getArgumentIndex(expr.var_name) + fn_arg_idx_st);
  return source_arg;
}

void* LLVMExpressionVisitor::visit(const expressions::TemporaryVariableExpression& expr) {
  LOG(INFO) << "LLVMExpressionVisitor::TemporaryVariableExpression::visit";
  assert(expr.var_src_type == VAR_SOURCE_TYPE::TEMPORARY_VARIABLE);
  assert(this->fnCtx->tempVariableMap->contains(expr.var_name));
  return fnCtx->tempVariableMap->operator[](expr.var_name);
}

void* LLVMExpressionVisitor::visit(const expressions::LocalVariableExpression& localVariableExpr) {
  LOG(INFO) << "LLVMExpressionVisitor::LocalVariableExpression::visit";
  if (localVariableExpr.var_src_type == VAR_SOURCE_TYPE::TEMPORARY_VARIABLE) {
    return visit(dynamic_cast<const TemporaryVariableExpression&>(localVariableExpr));
  } else if (localVariableExpr.var_src_type == VAR_SOURCE_TYPE::FUNCTION_ARGUMENT) {
    return visit(dynamic_cast<const FunctionArgumentExpression&>(localVariableExpr));
  } else {
    assert(false && "what type of VAR_SOURCE_TYPE in LocalVariableExpression ?");
  }
}

void* LLVMExpressionVisitor::visit(const expressions::Int64Constant& expr) {
  return codegenEngine->createInt64(expr.getValue());
}

void* LLVMExpressionVisitor::visit(const expressions::FloatConstant& expr) {
  return codegenEngine->createFloat(expr.getValue());
}

void* LLVMExpressionVisitor::visit(const expressions::DoubleConstant& expr) {
  return codegenEngine->createDouble(expr.getValue());
}

void* LLVMExpressionVisitor::visit(const expressions::BoolConstant& expr) {
  if (expr.getValue()) {
    return codegenEngine->createTrue();
  } else {
    return codegenEngine->createFalse();
  }
}
void* LLVMExpressionVisitor::visit(const expressions::NullPtrConstant& expr) {
  // is this really a nullptr?
  // because we have record_ptr mostly, which are 0, so return 0 i think.
  //  return llvm::ConstantPointerNull::get(Type::getInt8PtrTy(this->codegenEngine->getLLVMContext()));
  return codegenEngine->createInt64(0);
}

void* LLVMExpressionVisitor::visit(const expressions::EqualExpression& expr) {
  LOG(INFO) << "LLVMExpressionVisitor::EqualExpression::visit";
  auto builder = codegenEngine->getBuilder();

  auto leftValue = static_cast<llvm::Value*>(expr.getLeft()->accept(this));
  auto rightValue = static_cast<llvm::Value*>(expr.getRight()->accept(this));
  CHECK(leftValue->getType()->getTypeID() == rightValue->getType()->getTypeID()) << "Type mismatch";

  return builder->CreateICmpEQ(leftValue, rightValue);
}
void* LLVMExpressionVisitor::visit(const expressions::NotEqualExpression& expr) {
  LOG(INFO) << "LLVMExpressionVisitor::NotEqualExpression::visit";
  auto builder = codegenEngine->getBuilder();

  auto leftValue = static_cast<llvm::Value*>(expr.getLeft()->accept(this));
  auto rightValue = static_cast<llvm::Value*>(expr.getRight()->accept(this));
  CHECK(leftValue->getType()->getTypeID() == rightValue->getType()->getTypeID()) << "Type mismatch";

  return builder->CreateICmpNE(leftValue, rightValue);
}