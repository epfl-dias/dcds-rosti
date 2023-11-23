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

#include "dcds/codegen/llvm-codegen/llvm-codegen-function.hpp"

#include "dcds/builder/function-builder.hpp"
#include "dcds/codegen/llvm-codegen/expression-codegen/llvm-expression-visitor.hpp"
#include "dcds/codegen/llvm-codegen/functions.hpp"
#include "dcds/codegen/llvm-codegen/llvm-codegen-statement.hpp"
#include "dcds/codegen/llvm-codegen/utils/conditionals.hpp"
#include "dcds/codegen/llvm-codegen/utils/loops.hpp"
#include "dcds/codegen/llvm-codegen/utils/phi-node.hpp"
#include "dcds/util/logging.hpp"

static constexpr bool print_debug_log = false;

namespace dcds {

LLVMCodegenFunction::LLVMCodegenFunction(LLVMCodegen *_codegen, dcds::Builder *builder,
                                         std::shared_ptr<FunctionBuilder> &_fb, const std::string &fn_name_prefix,
                                         const std::string &fn_name_suffix, llvm::GlobalValue::LinkageTypes linkageType)
    : codegen(_codegen), fb(_fb) {
  auto ptrType = IntegerType::getInt8PtrTy(ctx());
  auto uintPtrType = IntegerType::getInt64Ty(ctx());
  auto boolType = IntegerType::getInt1Ty(ctx());
  // auto fn_name_prefix = builder->getName() + "_";

  this->retval_variable_name = builder->getName() + "_" + fb->getName() + "_retval";

  this->fn_name = fn_name_prefix + fb->_name + fn_name_suffix;

  // pre_args: { txnManager*, mainRecord, txnPtr, ^^retValPtr^^ }
  std::vector<std::pair<std::string, llvm::Type *>> pre_args;  // {ptrType, uintPtrType, ptrType};
  pre_args.emplace_back("txnManager", ptrType);
  pre_args.emplace_back("mainRecord", uintPtrType);
  pre_args.emplace_back("txnPtr", ptrType);

  if (fb->returnValueType != valueType::VOID) {
    pre_args.emplace_back(retval_variable_name, codegen->DcdsToLLVMType(fb->getReturnValueType(), true));
  }

  this->genFunctionSignature(pre_args, linkageType, boolType);

  // 2- set insertion point at the beginning of the function.
  entryBB = llvm::BasicBlock::Create(ctx(), "entry", fn);
  returnBB = llvm::BasicBlock::Create(ctx(), "return", fn);
  IRBuilder()->SetInsertPoint(entryBB);

  // 3- allocate temporary variables.
  LOG_IF(INFO, print_debug_log) << "codegen-temporary variables";
  allocateFunctionVariables();

  // Allocate the return variable: success-state: not_abort
  CHECK(!(allocated_vars.contains(this->retval_variable_name))) << "Return variable name already taken?";

  retval_variable = codegen->allocateOneVar(retval_variable_name, valueType::BOOL, true);
  allocated_vars.emplace(retval_variable_name, retval_variable);

  LLVMScopedContext build_ctx(this->codegen, builder, this->fb, this->fb->entryPoint, this);

  // 4- codegen all statements
  LOG_IF(INFO, print_debug_log) << "codegen-statements";
  LLVMCodegenStatement::gen(&build_ctx);

  // ----- GEN RETURN BLOCK
  // always return if the inner was successful or not.
  // WAIT, if it is single-threaded, we don't need this!
  IRBuilder()->SetInsertPoint(returnBB);

  llvm::Value *retValue = IRBuilder()->CreateLoad(fn->getReturnType(), this->retval_variable);
  IRBuilder()->CreateRet(retValue);

  returnBB->moveAfter(&(fn->back()));
  // ----- GEN RETURN BLOCK END

  LOG_IF(INFO, print_debug_log) << "verifying function";
  dcds::LLVMCodegenFunction::LLVM_verifyFunction(fn);
}

// LLVMCodegenFunction::LLVMCodegenFunction(LLVMCodegen *_codegen, std::string function_name) : codegen(_codegen) {
//   entryBB = llvm::BasicBlock::Create(ctx(), "entry", fn);
//   returnBB = llvm::BasicBlock::Create(ctx(), "return", fn);
//
//   IRBuilder()->SetInsertPoint(entryBB);
//
//   auto fnCtx = FunctionBuildContext(fb, sb, fn, &tempVariableMap, returnBB);
//
//   // Allocate the return variable: success-state: not_abort
//   fnCtx.retval_variable_name = builder->getName() + "_" + fb->getName() + "_retval";
//
//   auto retValAlloc = allocateOneVar(fnCtx.retval_variable_name, valueType::BOOL, true);
//   tempVariableMap.emplace(fnCtx.retval_variable_name, retValAlloc);
//
//   for (auto &s : sb->statements) {
//     LLVMCodegenStatement::gen(this, builder, fnCtx, s);
//     // this->buildStatement(builder, fnCtx, s);
//   }
//
//   // ----- GEN RETURN BLOCK
//   // always return if the inner was successful or not.
//   // WAIT, if it is single-threaded, we don't need this!
//   getBuilder()->SetInsertPoint(returnBB);
//   llvm::Value *retValue = getBuilder()->CreateLoad(fnCtx.fn->getReturnType(),
//                                                    fnCtx.tempVariableMap->operator[](fnCtx.retval_variable_name));
//   getBuilder()->CreateRet(retValue);
//
//   returnBB->moveAfter(&(fn->back()));
// }

llvm::Function *LLVMCodegenFunction::wrapFunctionVariadicArgs(llvm::Function *inner_function,
                                                              const std::vector<llvm::Type *> &position_args,
                                                              const std::vector<llvm::Type *> &expected_vargs,
                                                              std::string wrapped_function_name) {
  assert(inner_function->isVarArg() == false && "already variadic args!");

  if (wrapped_function_name.empty()) {
    wrapped_function_name = inner_function->getName().str() + "_vargs";
  }

  llvm::FunctionType *funcType = llvm::FunctionType::get(inner_function->getReturnType(), position_args, true);
  llvm::Function *func =
      llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, wrapped_function_name, Module());
  llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(ctx(), "entry", func);
  IRBuilder()->SetInsertPoint(entryBlock);

  auto va_list = codegen->createVaListStart();
  auto vargs = codegen->getVAArgs(va_list, expected_vargs);
  codegen->createVaListEnd(va_list);

  std::vector<llvm::Value *> args_materialized;
  for (size_t i = 0; i < position_args.size(); i++) {
    args_materialized.push_back(func->getArg(i));
  }

  for (auto &v : vargs) {
    args_materialized.push_back(v);
  }

  auto ret = codegen->gen_call(inner_function, args_materialized);

  if (inner_function->getReturnType()->getTypeID() == llvm::Type::getVoidTy(ctx())->getTypeID()) {
    IRBuilder()->CreateRetVoid();
  } else {
    IRBuilder()->CreateRet(ret);
  }

  this->LLVM_verifyFunction(func);

  return func;
}

void LLVMCodegenFunction::allocateFunctionVariables() {
  // auto allocaBuilder = llvm::IRBuilder<>(basicBlock, basicBlock->end());

  LOG_IF(INFO, print_debug_log) << "Allocating function's temp vars: " << fb->temp_variables.size();

  for (auto &v : fb->temp_variables) {
    auto var_name = v.first;
    auto var_type = v.second->var_type;
    CHECK(!(allocated_vars.contains(var_name)))
        << "Variable already allocated: " << var_name << " in function: " << fb->getName();
    auto vr = codegen->allocateOneVar(var_name, var_type, v.second->var_default_value);
    allocated_vars.emplace(var_name, vr);
  }
}

void LLVMCodegenFunction::genFunctionSignature(const std::vector<std::pair<std::string, llvm::Type *>> &pre_args,
                                               llvm::GlobalValue::LinkageTypes linkageType,
                                               llvm::Type *override_return_type) {
  std::vector<llvm::Type *> arg_types;
  std::vector<std::string> arg_names;
  for (auto &a : pre_args) {
    arg_names.push_back(a.first);
    arg_types.push_back(a.second);
  }

  llvm::Type *returnType = override_return_type;

  for (const auto &arg : fb->function_args) {
    arg_types.push_back(codegen->DcdsToLLVMType(arg->getType(), arg->is_reference_type));
    arg_names.push_back(arg->var_name);
  }

  if (!override_return_type) {
    if (fb->returnValueType == dcds::valueType::VOID) {
      returnType = llvm::Type::getVoidTy(ctx());
    } else {
      returnType = codegen->DcdsToLLVMType(fb->returnValueType);
    }
  }

  auto fn_type = llvm::FunctionType::get(returnType, arg_types, false);
  this->fn = llvm::Function::Create(fn_type, linkageType, fn_name, Module());

  if (fb->isAlwaysInline()) {
    fn->addFnAttr(llvm::Attribute::AlwaysInline);
  }

  // Set the argument names
  assert(fn->arg_size() == arg_names.size());
  size_t i = 0;
  for (auto &a : fn->args()) {
    a.setName(arg_names[i]);
    i++;
  }
}

llvm::Value *LLVMCodegenFunction::getArgumentByName(const std::string &name) {
  // LOG_IF(INFO, print_debug_log) << "getArgumentByName: " << name;
  for (auto &arg : fn->args()) {
    // LOG_IF(INFO, print_debug_log) << "\targ.getName(): " << arg.getName().str();
    if (arg.getName().equals(name)) {
      return &arg;
    }
  }
  return nullptr;
}

llvm::Value *LLVMCodegenFunction::getArgumentByPosition(uint32_t index) {
  CHECK(index < fn->arg_size()) << "Argument index out of bounds";
  uint32_t pos = 0;
  for (auto arg_iter = fn->arg_begin(), end = fn->arg_end(); arg_iter != end; ++arg_iter, ++pos) {
    if (pos == index) {
      return &*arg_iter;
    }
  }
  return nullptr;
}

llvm::Value *LLVMCodegenFunction::getVariable(const std::string &name) {
  if (allocated_vars.contains(name))
    return allocated_vars[name];
  else
    return nullptr;
}

}  // namespace dcds
