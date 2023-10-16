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
class LLVMCodegen;
}

namespace dcds::expressions {

class LLVMExpressionVisitor : public ExpressionVisitor {
 public:
  explicit LLVMExpressionVisitor(LLVMCodegen* codegen_engine,
                                 dcds::LLVMCodegen::function_build_context* function_context)
      : codegenEngine(codegen_engine), fnCtx(function_context) {}

  ~LLVMExpressionVisitor() override = default;

 public:
  // Unary expressions
  void* visit(const expressions::IsNullExpression& expr) override;
  void* visit(const expressions::IsNotNullExpression& expr) override;
  void* visit(const expressions::IsEvenExpression& expr) override;

  // Binary expressions
  void* visit(const expressions::AddExpression& expr) override;

  // Temporary variables / Function Arguments
  void* visit(const expressions::LocalVariableExpression& localVariableExpr) override;
  void* visit(const expressions::FunctionArgumentExpression& functionArgumentExpr) override;
  void* visit(const expressions::TemporaryVariableExpression& expr) override;

 private:
  dcds::LLVMCodegen* codegenEngine;
  dcds::LLVMCodegen::function_build_context* fnCtx;
};

}  // namespace dcds::expressions

#endif  // DCDS_LLVM_EXPRESSION_VISITOR_HPP
