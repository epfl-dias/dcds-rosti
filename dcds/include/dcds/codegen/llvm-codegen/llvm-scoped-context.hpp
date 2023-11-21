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

#ifndef DCDS_LLVM_SCOPED_CONTEXT_HPP
#define DCDS_LLVM_SCOPED_CONTEXT_HPP

#include <utility>

#include "dcds/codegen/llvm-codegen/llvm-codegen.hpp"

namespace dcds {
using namespace llvm;

class LLVMCodegenFunction;
class LLVMCodegenStatement;

class LLVMScopedContext {
  friend class LLVMCodegenFunction;
  friend class LLVMCodegenStatement;

 public:
  LLVMScopedContext(LLVMCodegen *_codegen, dcds::Builder *_current_builder,
                    std::shared_ptr<FunctionBuilder> _current_fb, std::shared_ptr<StatementBuilder> _current_sb,
                    LLVMCodegenFunction *_current_function_ctx)
      : codegen(_codegen),
        current_builder(_current_builder),
        current_fb(std::move(_current_fb)),
        current_sb(std::move(_current_sb)),
        current_function_ctx(_current_function_ctx) {}

  LLVMScopedContext(LLVMScopedContext *other, std::shared_ptr<StatementBuilder> _current_sb)
      : codegen(other->codegen),
        current_builder(other->current_builder),
        current_fb(other->current_fb),
        current_sb(std::move(_current_sb)),
        current_function_ctx(other->current_function_ctx) {}

 public:
  LLVMCodegenFunction *getFunctionContext() { return current_function_ctx; }
  auto getCodegen() { return codegen; }

  // llvm::Value* getVariable(std::string name); // future for scoped-temp-variables.

 protected:
  LLVMCodegen *codegen;

  dcds::Builder *current_builder;
  std::shared_ptr<FunctionBuilder> current_fb;
  std::shared_ptr<StatementBuilder> current_sb;

  LLVMCodegenFunction *current_function_ctx;
  // LLVMCodegenStatement *current_statement_ctx;
};

}  // namespace dcds

#endif  // DCDS_LLVM_SCOPED_CONTEXT_HPP
