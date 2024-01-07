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

#ifndef DCDS_LLVM_EXPRESSION_VISITOR_HPP
#define DCDS_LLVM_EXPRESSION_VISITOR_HPP

#include <llvm/IR/Value.h>

#include "dcds/builder/expressions/expression-visitor.hpp"
#include "dcds/builder/expressions/expressions.hpp"
#include "dcds/codegen/llvm-codegen/llvm-codegen.hpp"

namespace dcds {
// class LLVMCodegen;
// class LLVMCodegenFunction;
class LLVMScopedContext;
}  // namespace dcds

namespace dcds::expressions {

class LLVMExpressionVisitor : public ExpressionVisitor {
 public:
  static llvm::Value* gen(LLVMScopedContext* build_ctx, const dcds::expressions::Expression* expr) {
    dcds::expressions::LLVMExpressionVisitor exprVisitor(build_ctx);
    auto val = const_cast<dcds::expressions::Expression*>(expr)->accept(&exprVisitor);
    return static_cast<llvm::Value*>(val);
  }

  static llvm::Value* gen(LLVMScopedContext* build_ctx, const std::shared_ptr<dcds::expressions::Expression>& expr) {
    dcds::expressions::LLVMExpressionVisitor exprVisitor(build_ctx);
    auto val = const_cast<dcds::expressions::Expression*>(expr.get())->accept(&exprVisitor);
    return static_cast<llvm::Value*>(val);
  }

  // std::shared_ptr

 private:
  explicit LLVMExpressionVisitor(LLVMScopedContext* function_context) : build_ctx(function_context) {}

  ~LLVMExpressionVisitor() override = default;

 public:
  // Unary expressions
  void* visit(const expressions::IsNullExpression& expr) override;
  void* visit(const expressions::IsNotNullExpression& expr) override;
  void* visit(const expressions::IsEvenExpression& expr) override;

  // Constant expressions
  void* visit(const expressions::Int64Constant& expr) override;
  void* visit(const expressions::Int32Constant& expr) override;
  void* visit(const expressions::BoolConstant& expr) override;
  void* visit(const expressions::FloatConstant& expr) override;
  void* visit(const expressions::DoubleConstant& expr) override;
  void* visit(const expressions::NullPtrConstant& expr) override;

  // Binary expressions
  void* visit(const expressions::AddExpression& expr) override;
  void* visit(const expressions::SubtractExpression& expr) override;
  void* visit(const expressions::EqualExpression& expr) override;
  void* visit(const expressions::NotEqualExpression& expr) override;
  void* visit(const expressions::GreaterThanExpression& expr) override;
  void* visit(const expressions::GreaterThanOrEqualToExpression& expr) override;
  void* visit(const expressions::LessThanExpression& expr) override;
  void* visit(const expressions::LessThanOrEqualToExpression& expr) override;

  // Temporary variables / Function Arguments
  void* visit(const expressions::LocalVariableExpression& localVariableExpr) override;
  void* visit(const expressions::FunctionArgumentExpression& functionArgumentExpr) override;
  void* visit(const expressions::TemporaryVariableExpression& expr) override;

 private:
  llvm::Value* loadValueIfRequired(llvm::Value* in, dcds::valueType dcds_value_type);

 private:
  LLVMScopedContext* build_ctx;
};

}  // namespace dcds::expressions

#endif  // DCDS_LLVM_EXPRESSION_VISITOR_HPP
