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

#include "dcds/codegen/llvm-codegen/llvm-codegen.hpp"

#include <llvm/IR/Instructions.h>

#include <utility>

#include "dcds/builder/function-builder.hpp"
#include "dcds/builder/statement-builder.hpp"
#include "dcds/codegen/llvm-codegen/expression-codegen/llvm-expression-visitor.hpp"
#include "dcds/codegen/llvm-codegen/functions.hpp"
#include "dcds/codegen/llvm-codegen/llvm-codegen-statement.hpp"
#include "dcds/codegen/llvm-codegen/llvm-jit.hpp"
#include "dcds/codegen/llvm-codegen/utils/conditionals.hpp"
#include "dcds/codegen/llvm-codegen/utils/loops.hpp"
#include "dcds/codegen/llvm-codegen/utils/phi-node.hpp"
#include "dcds/indexes/index-functions.hpp"

static constexpr bool print_debug_log = false;

namespace dcds {

void LLVMCodegen::saveToFile(const std::string &filename) {
  std::error_code errorCode;
  llvm::raw_fd_ostream outLL(filename, errorCode);
  theLLVMModule->print(outLL, nullptr);
}

void LLVMCodegen::printIR() { theLLVMModule->print(llvm::outs(), nullptr); }

llvm::Type *LLVMCodegen::DcdsToLLVMType(dcds::valueType dcds_type, bool is_reference) {
  llvm::Type *ty;
  switch (dcds_type) {
    case dcds::valueType::INT64:
    case dcds::valueType::RECORD_PTR:
      ty = llvm::Type::getInt64Ty(getLLVMContext());
      break;
    case dcds::valueType::INT32:
      ty = llvm::Type::getInt32Ty(getLLVMContext());
      break;
    case dcds::valueType::DOUBLE:
      ty = llvm::Type::getDoubleTy(getLLVMContext());
      break;
    case dcds::valueType::FLOAT:
      ty = llvm::Type::getFloatTy(getLLVMContext());
      break;
    case dcds::valueType::BOOL:
      ty = llvm::Type::getInt1Ty(getLLVMContext());
      break;
    case dcds::valueType::VOID:
      ty = llvm::Type::getVoidTy(getLLVMContext());
      break;
  }
  assert(ty != nullptr);

  if (is_reference) {
    return ty->getPointerTo();
  }
  return ty;
}

void LLVMCodegen::initializeLLVMModule(const std::string &name) {
  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen] Initializing LLVM module: " << name;
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  theLLVMContext = std::make_unique<LLVMContext>();

  theLLVMModule = std::make_unique<Module>(name, *theLLVMContext);
  assert(theLLVMModule.get());
  llvmBuilder = std::make_unique<IRBuilder<>>(*theLLVMContext);
  assert(llvmBuilder.get());
}

LLVMCodegen::LLVMCodegen(Builder *builder) : Codegen(builder), LLVMCodegenContext(builder->getName()) {
  // LOG_IF(INFO, print_debug_log) << "[LLVMCodegen] constructor: " << this->moduleName;
  initializeLLVMModule(builder->getName());
  initializePassManager();
  this->registerAllFunctions();
}

llvm::Module *LLVMCodegen::getModule() const { return theLLVMModule.get(); }
llvm::IRBuilder<> *LLVMCodegen::getBuilder() const { return llvmBuilder.get(); }

void LLVMCodegen::initializePassManager() {
  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen] Initializing LLVM pass manager";
  assert(theLLVMModule.get());
  /// Create a new pass manager.
  theLLVMFPM = std::make_unique<legacy::FunctionPassManager>(theLLVMModule.get());

  /// Promote allocas to registers.
  theLLVMFPM->add(createPromoteMemoryToRegisterPass());
  /// Do simple "peephole" optimizations and bit-twiddling optimizations.
  theLLVMFPM->add(createInstructionCombiningPass());
  /// Re-associate expressions.
  theLLVMFPM->add(createReassociatePass());
  /// Eliminate Common SubExpressions.
  theLLVMFPM->add(createGVNPass());
  /// Simplify the control flow graph (deleting unreachable blocks, etc).
  theLLVMFPM->add(createCFGSimplificationPass());

  theLLVMFPM->add(createDeadCodeEliminationPass());

  theLLVMFPM->doInitialization();
}

void LLVMCodegen::build(dcds::Builder *builder, bool is_nested_type) {
  if (!builder) {
    builder = this->top_level_builder;
  }
  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen] Building: " << builder->getName();

  // codegenHelloWorld();

  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen::build()] createDsStructType";
  this->createDsStructType(builder);

  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen::build()] buildConstructor";
  this->buildConstructor(*builder, is_nested_type);

  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen::build()] BuildFunctions";
  this->buildFunctions(builder, is_nested_type);
  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen::build()] DONE";
}

void LLVMCodegen::buildFunctions(dcds::Builder *builder, bool is_nested_type) {
  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen] buildFunctions: # of functions: " << builder->functions.size();

  // later: parallelize this loop, but do take care of the context insertion points in parallel function build.
  for (auto &fb : builder->functions) {
    this->buildOneFunction(builder, fb.second, is_nested_type);
  }
}

llvm::Function *LLVMCodegen::wrapFunctionVariadicArgs(llvm::Function *inner_function,
                                                      const std::vector<llvm::Type *> &position_args,
                                                      const std::vector<llvm::Type *> &expected_vargs,
                                                      std::string wrapped_function_name) {
  assert(inner_function->isVarArg() == false && "already variadic args!");

  if (wrapped_function_name.empty()) {
    wrapped_function_name = inner_function->getName().str() + "_vargs";
  }

  llvm::FunctionType *funcType = llvm::FunctionType::get(inner_function->getReturnType(), position_args, true);
  llvm::Function *func =
      llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, wrapped_function_name, getModule());
  llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(getLLVMContext(), "entry", func);
  getBuilder()->SetInsertPoint(entryBlock);

  auto va_list = this->createVaListStart();
  auto vargs = this->getVAArgs(va_list, expected_vargs);
  this->createVaListEnd(va_list);

  std::vector<llvm::Value *> args_materialized;
  for (auto i = 0; i < position_args.size(); i++) {
    args_materialized.push_back(func->getArg(i));
  }

  for (auto &v : vargs) {
    args_materialized.push_back(v);
  }

  auto ret = this->gen_call(inner_function, args_materialized);

  if (inner_function->getReturnType()->getTypeID() == llvm::Type::getVoidTy(getLLVMContext())->getTypeID()) {
    getBuilder()->CreateRetVoid();
  } else {
    getBuilder()->CreateRet(ret);
  }
  llvmVerifyFunction(func);

  return func;
}

// llvm::Function *LLVMCodegen::buildOneFunction_inner(dcds::Builder *builder, std::shared_ptr<FunctionBuilder> &fb) {
//   auto ptrType = IntegerType::getInt8PtrTy(getLLVMContext());
//   auto uintPtrType = IntegerType::getInt64Ty(getLLVMContext());
//   auto boolType = IntegerType::getInt1Ty(getLLVMContext());
//
//   auto fn_name_prefix = builder->getName() + "_";
//
//   // pre_args: { txnManager*, mainRecord, txnPtr, ^^retValPtr^^ }
//   std::vector<llvm::Type *> pre_args{ptrType, uintPtrType, ptrType};
//   if (fb->returnValueType != valueType::VOID) {
//     pre_args.push_back(this->DcdsToLLVMType(fb->getReturnValueType(), true));
//   }
//
//   auto fn_inner = this->genFunctionSignature(fb, pre_args, fn_name_prefix, "_inner",
//                                              llvm::GlobalValue::LinkageTypes::PrivateLinkage, boolType);
//
//   userFunctions.emplace(fn_inner->getName().str(), fn_inner);
//
//   // generate inner function first.
//
//   // 2- set insertion point at the beginning of the function.
//   auto fn_inner_BB = llvm::BasicBlock::Create(getLLVMContext(), "entry", fn_inner);
//   getBuilder()->SetInsertPoint(fn_inner_BB);
//
//   // 3- allocate temporary variables.
//   LOG_IF(INFO, print_debug_log) << "codegen-temporary variables";
//   auto variableCodeMap = this->allocateTemporaryVariables(fb, fn_inner_BB);
//
//   // 4- codegen all statements
//   LOG_IF(INFO, print_debug_log) << "codegen-statements";
//
//   this->buildFunctionBody(builder, fb, fb->entryPoint, fn_inner, fn_inner_BB, variableCodeMap);
//
//   dcds::LLVMCodegen::llvmVerifyFunction(fn_inner);
//
//   return fn_inner;
// }

llvm::Function *LLVMCodegen::buildOneFunction_outer(dcds::Builder *builder, std::shared_ptr<FunctionBuilder> &fb,
                                                    llvm::Function *fn_inner) {
  // It is the mainDS, else there would be no outer function.
  auto ptrType = IntegerType::getInt8PtrTy(getLLVMContext());
  auto uintPtrType = IntegerType::getInt64Ty(getLLVMContext());
  auto fn_name_prefix = builder->getName() + "_";
  bool genCC = top_level_builder->is_multi_threaded;

  // pre_args: { txnManager*, mainRecord }
  std::vector<llvm::Type *> fn_outer_args{ptrType, uintPtrType};
  auto fn_outer = this->genFunctionSignature(fb, fn_outer_args, fn_name_prefix, "",
                                             llvm::GlobalValue::LinkageTypes::ExternalLinkage);
  userFunctions.emplace(fn_outer->getName().str(), fn_outer);

  // CODEGEN outer function.
  auto fn_outer_BB = llvm::BasicBlock::Create(getLLVMContext(), "entry", fn_outer);
  getBuilder()->SetInsertPoint(fn_outer_BB);

  auto txnManager = fn_outer->getArg(0);
  auto mainRecord = fn_outer->getArg(1);
  Value *txnPtr;

  auto arg_is_readOnly = fb->isReadOnly() ? this->createTrue() : this->createFalse();
  llvm::Value *function_ret_value_arg;
  llvm::Value *is_success;

  if (fb->returnValueType != valueType::VOID) {
    function_ret_value_arg = allocateOneVar("function_ret_value_arg", fb->getReturnValueType(), {});
  }

  this->gen_do([&]() {
        if (genCC) {
          txnPtr = this->gen_call(beginTxn, {fn_outer->getArg(0), arg_is_readOnly});
        } else {
          txnPtr = llvm::ConstantPointerNull::get(ptrType);
        }

        // create call to inner_function here.
        // start from 2, as 0-1 are already taken (outer.txnManager, out.mainRecord).
        std::vector<llvm::Value *> inner_args{txnManager, mainRecord, txnPtr};
        if (fb->returnValueType != valueType::VOID) {
          inner_args.push_back(function_ret_value_arg);
        }
        for (auto i = 2; i < fn_outer->arg_size(); i++) {
          inner_args.push_back(fn_outer->getArg(i));
        }

        // this function returns true/false for success/false, but here we anyway call endTxn, so depend on that.
        getBuilder()->CreateCall(fn_inner, inner_args);

        if (genCC) {
          is_success = this->gen_call(endTxn, {txnManager, txnPtr}, Type::getInt1Ty(getLLVMContext()));
        }
      })
      .gen_while([&]() {
        if (genCC)
          return getBuilder()->CreateNot(is_success);
        else {
          return static_cast<llvm::Value *>(this->createFalse());
        }
      });

  if (fb->returnValueType == valueType::VOID) {
    getBuilder()->CreateRetVoid();
  } else {
    auto retLoadInst = getBuilder()->CreateLoad(this->DcdsToLLVMType(fb->getReturnValueType()), function_ret_value_arg);
    getBuilder()->CreateRet(retLoadInst);
  }

  dcds::LLVMCodegen::llvmVerifyFunction(fn_outer);

  return fn_outer;

  // Optimizations:
  //   easy: if an attribute is only accessed in a single function across DS, then you don't need CC either on that one.
}

void LLVMCodegen::buildOneFunction(dcds::Builder *builder, std::shared_ptr<FunctionBuilder> &fb, bool is_nested_type) {
  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen] buildOneFunction: " << fb->_name;

  // FIXME: scoping of temporary variables??

  // NOTE: as we don't know the function inner return paths,
  //  lets create a wrapper function which will do the txn stuff,
  //  and then call the inner function with txn ptr.

  // LLVMCodegenFunction::gen()
  auto inner_fn = LLVMCodegenFunction::gen(this, builder, fb, builder->getName() + "_", "_inner",
                                           llvm::GlobalValue::LinkageTypes::PrivateLinkage);
  userFunctions.emplace(inner_fn);
  //  LLVMCodegenFunction inner_fn(this, builder, fb, builder->getName() + "_", "_inner",
  //  llvm::GlobalValue::LinkageTypes::PrivateLinkage); userFunctions.emplace(inner_fn.get());

  // auto fn_inner = buildOneFunction_inner(builder, fb);

  if (!is_nested_type) {
    // ------ GEN FN_OUTER ------
    // build outer function which would be exposed in symbol table
    [[maybe_unused]] auto fn_outer = buildOneFunction_outer(builder, fb, inner_fn.second);

    // ------ GEN FN_OUTER END------

    /*
    // ------ GEN VA ARGS WRAPPER ------
    // pre_args: { txnManager*, mainRecord }
    auto ptrType = IntegerType::getInt8PtrTy(getLLVMContext());
    auto uintPtrType = IntegerType::getInt64Ty(getLLVMContext());
    std::vector<llvm::Type *> fn_outer_args{ptrType, uintPtrType};

    // Only wrap va-args for the exposed functions as it is purely used by jit-container only.
    // so we have a function R f(void*,void*,void*, A,B,C)
    // we want it wrapped to R f(void*,void*,void*,...)

    std::vector<llvm::Type *> expected_vargs;
    for (const auto &arg : fb->function_args) {
      expected_vargs.push_back(DcdsToLLVMType(arg->getType(), arg->is_reference_type));
    }
    this->wrapFunctionVariadicArgs(fn_outer, fn_outer_args, expected_vargs, fn_outer->getName().str() + "_vargs");

    // ------ GEN VA ARGS WRAPPER END ------
     */
  }

  // later: mark this function as done?
}

// void LLVMCodegen::buildFunctionBody(dcds::Builder *builder, std::shared_ptr<FunctionBuilder> &fb,
//                                     std::shared_ptr<StatementBuilder> &sb, llvm::Function *fn,
//                                     llvm::BasicBlock *basicBlock,
//                                     std::map<std::string, llvm::Value *> &tempVariableMap) {
//   auto returnBB = llvm::BasicBlock::Create(getLLVMContext(), "return", fn);
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
//   // ----- GEN RETURN BLOCK END
// }

llvm::Function *LLVMCodegen::genFunctionSignature(std::shared_ptr<FunctionBuilder> &fb,
                                                  const std::vector<llvm::Type *> &pre_args,
                                                  const std::string &name_prefix, const std::string &name_suffix,
                                                  llvm::GlobalValue::LinkageTypes linkageType,
                                                  llvm::Type *override_return_type) {
  std::vector<llvm::Type *> argTypes(pre_args);
  llvm::Type *returnType = override_return_type;

  for (const auto &arg : fb->function_args) {
    argTypes.push_back(DcdsToLLVMType(arg->getType(), arg->is_reference_type));
  }

  if (!override_return_type) {
    if (fb->returnValueType == dcds::valueType::VOID) {
      returnType = llvm::Type::getVoidTy(getLLVMContext());
    } else {
      returnType = DcdsToLLVMType(fb->returnValueType);
    }
  }

  auto fn_type = llvm::FunctionType::get(returnType, argTypes, false);
  auto fn = llvm::Function::Create(fn_type, linkageType, name_prefix + fb->_name + name_suffix, theLLVMModule.get());

  if (fb->isAlwaysInline()) {
    fn->addFnAttr(llvm::Attribute::AlwaysInline);
  }
  return fn;
}

llvm::Value *LLVMCodegen::allocateOneVar(const std::string &var_name, dcds::valueType var_type, std::any init_value) {
  bool has_value = init_value.has_value();
  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen] allocateOneVar temp-var: " << var_name << "::" << var_type
                                << " | has_value: " << has_value;

  switch (var_type) {
    case dcds::valueType::INT64: {
      auto vr = getBuilder()->CreateAlloca(llvm::Type::getInt64Ty(getLLVMContext()), nullptr, var_name);
      if (has_value) {
        auto value = createInt64(std::any_cast<uint64_t>(init_value));
        getBuilder()->CreateStore(value, vr);
      }
      return vr;
    }
    case dcds::valueType::RECORD_PTR: {
      auto vr = getBuilder()->CreateAlloca(llvm::Type::getInt64Ty(getLLVMContext()), nullptr, var_name);
      // Initialize all pointers to zero for safe size!
      getBuilder()->CreateStore(createInt64(UINT64_C(0)), vr);
      return vr;
    }
    case dcds::valueType::INT32: {
      auto vr = getBuilder()->CreateAlloca(llvm::Type::getInt32Ty(getLLVMContext()), nullptr, var_name);
      if (has_value) {
        auto value = createInt32(std::any_cast<int32_t>(init_value));
        getBuilder()->CreateStore(value, vr);
      }
      return vr;
    }
    case dcds::valueType::BOOL: {
      auto bool_type = llvm::Type::getInt1Ty(getLLVMContext());
      auto vr = getBuilder()->CreateAlloca(bool_type, nullptr, var_name);
      if (has_value) {
        auto value = std::any_cast<bool>(init_value) ? createTrue() : createFalse();
        getBuilder()->CreateStore(value, vr);
      }
      return vr;
    }
    case dcds::valueType::FLOAT: {
      auto vr = getBuilder()->CreateAlloca(llvm::Type::getFloatTy(getLLVMContext()), nullptr, var_name);
      if (has_value) {
        auto value = createFloat(std::any_cast<float>(init_value));
        getBuilder()->CreateStore(value, vr);
      }
      return vr;
    }
    case dcds::valueType::DOUBLE: {
      auto vr = getBuilder()->CreateAlloca(llvm::Type::getDoubleTy(getLLVMContext()), nullptr, var_name);
      if (has_value) {
        auto value = createDouble(std::any_cast<double>(init_value));
        getBuilder()->CreateStore(value, vr);
      }
      return vr;
    }
    case dcds::valueType::VOID:
      assert(false && "[allocateOneVar] cannot allocate a variable of type 'void'");
  }
}

std::map<std::string, llvm::Value *> LLVMCodegen::allocateTemporaryVariables(std::shared_ptr<FunctionBuilder> &fb,
                                                                             llvm::BasicBlock *basicBlock) {
  std::map<std::string, llvm::Value *> variableCodeMap;
  auto allocaBuilder = llvm::IRBuilder<>(basicBlock, basicBlock->end());

  LOG_IF(INFO, print_debug_log) << "Allocating function's temp vars: " << fb->temp_variables.size();

  for (auto &v : fb->temp_variables) {
    auto var_name = v.first;
    auto var_type = v.second->var_type;
    auto vr = allocateOneVar(var_name, var_type, v.second->var_default_value);
    variableCodeMap.emplace(var_name, vr);
  }

  return variableCodeMap;
}

void LLVMCodegen::createDsStructType(dcds::Builder *builder) {
  std::string ds_struct_name = builder->getName() + "_t";
  bool is_packed = true;  // Important!! as the insertRecord takes and directly copies the memory.

  // for each attribute, add it to struct.
  // Later, also add this struct to the output .hpp file.
  LOG_IF(INFO, print_debug_log) << "createDsStructType : " << builder->getName();
  std::vector<llvm::Type *> struct_vars;
  for (const auto &a : builder->attributes) {
    // simpleType to type, else have a ptr to it.
    LOG_IF(INFO, print_debug_log) << "attr: " << a.first << "::" << a.second->type;
    struct_vars.push_back(DcdsToLLVMType(a.second->type));
  }

  auto *structType = StructType::create(getLLVMContext(), struct_vars, ds_struct_name, is_packed);
  CHECK(!(record_value_struct_types.contains(builder->getName())))
      << "This has been already built? " << builder->getName();
  record_value_struct_types.insert_or_assign(builder->getName(), structType);
}

llvm::Value *LLVMCodegen::createDefaultValue(const std::shared_ptr<Attribute> &attribute) {
  CHECK(attribute->type_category == ATTRIBUTE_TYPE_CATEGORY::PRIMITIVE || attribute->type == valueType::RECORD_PTR)
      << "Cannot create default value for complex type which is not a RECORD_PTR: " << attribute->type_category;

  auto hasValue = false;
  std::any defaultValue;

  if (attribute->type_category == ATTRIBUTE_TYPE_CATEGORY::PRIMITIVE) {
    defaultValue = std::static_pointer_cast<SimpleAttribute>(attribute)->getDefaultValue();
    hasValue = defaultValue.has_value();
  }

  switch (attribute->type) {
    case dcds::valueType::INT64: {
      auto value = hasValue ? std::any_cast<uint64_t>(defaultValue) : 0;
      return createInt64(value);
    }
    case dcds::valueType::INT32: {
      auto value = hasValue ? std::any_cast<int32_t>(defaultValue) : 0;
      return createInt32(value);
    }
    case dcds::valueType::RECORD_PTR: {
      return createUintptr(0);  // RECORD_PTR is a uintptr underlying.
    }
    case dcds::valueType::DOUBLE: {
      auto value = hasValue ? std::any_cast<double>(defaultValue) : 0;
      return createDouble(value);
    }
    case dcds::valueType::FLOAT: {
      auto value = hasValue ? std::any_cast<float>(defaultValue) : 0;
      return createFloat(value);
    }
    case dcds::valueType::BOOL: {
      auto value = hasValue && std::any_cast<bool>(defaultValue);
      return value ? createTrue() : createFalse();
    }
    case dcds::valueType::VOID:
      assert(false && "[createDefaultValue] cannot create value of type VOID");
      break;
  }

  CHECK(false) << "Unknown attribute type: " << attribute->type;
}

Value *LLVMCodegen::initializeDsValueStructDefault(dcds::Builder &builder) {
  CHECK(record_value_struct_types.contains(builder.getName())) << "default struct not found: " << builder.getName();

  llvm::Value *structValue = UndefValue::get(record_value_struct_types[builder.getName()]);
  auto *irBuilder = getBuilder();

  uint i = 0;
  for (const auto &a : builder.attributes) {
    llvm::Value *fieldValue = LLVMCodegen::createDefaultValue(a.second);
    structValue = irBuilder->CreateInsertValue(structValue, fieldValue, {i});
    i++;
  }

  llvm::AllocaInst *structPtr = irBuilder->CreateAlloca(record_value_struct_types[builder.getName()]);
  irBuilder->CreateStore(structValue, structPtr);

  return getBuilder()->CreateBitCast(structPtr, llvm::Type::getInt8PtrTy(getLLVMContext()));
}

llvm::Function *LLVMCodegen::genInitStorageFn(const std::string &function_name_prefix, llvm::Value *table_name,
                                              const std::map<std::string, std::shared_ptr<Attribute>> &attributes) {
  auto function_name = function_name_prefix + "_init_storage";
  auto fn_type =
      llvm::FunctionType::get(llvm::Type::getInt8PtrTy(getLLVMContext()), std::vector<llvm::Type *>{}, false);
  auto fn =
      llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::PrivateLinkage, function_name, getModule());
  userFunctions.emplace(function_name, fn);

  BasicBlock *entryBlock = BasicBlock::Create(getLLVMContext(), "entry", fn);
  getBuilder()->SetInsertPoint(entryBlock);

  Value *numAttributes = ConstantInt::get(getLLVMContext(), APInt(32, attributes.size()));

  llvm::Value *tableNameCharPtr = getBuilder()->CreateBitCast(table_name, llvm::Type::getInt8PtrTy(getLLVMContext()));

  Type *cppEnumType = Type::getInt32Ty(getLLVMContext());
  ArrayType *enumArrayType = ArrayType::get(cppEnumType, attributes.size());

  std::vector<Constant *> valueTypeArray;
  std::vector<std::string> names;

  for (const auto &a : attributes) {
    valueTypeArray.push_back(ConstantInt::get(cppEnumType, std::to_underlying(a.second->type)));
    names.push_back(a.first);
  }

  Constant *constAttributeTypeArray = ConstantArray::get(enumArrayType, valueTypeArray);
  AllocaInst *allocaInstAttributeTypes = getBuilder()->CreateAlloca(enumArrayType, nullptr, "attributeTypeArray");
  getBuilder()->CreateStore(constAttributeTypeArray, allocaInstAttributeTypes);
  Value *gepIndices[] = {ConstantInt::get(Type::getInt32Ty(getLLVMContext()), 0),
                         ConstantInt::get(Type::getInt32Ty(getLLVMContext()), 0)};
  Value *elementPtrAttributeType = getBuilder()->CreateGEP(enumArrayType, allocaInstAttributeTypes, gepIndices);

  llvm::Value *attributeNames = createStringArray(names, function_name_prefix + "_attr_");
  llvm::Value *attributeNamesFirstCharPtr = getBuilder()->CreateExtractValue(attributeNames, {0});

  llvm::Value *resultPtr = this->gen_call(
      createTablesInternal, {tableNameCharPtr, elementPtrAttributeType, attributeNamesFirstCharPtr, numAttributes});

  // return the table*
  getBuilder()->CreateRet(resultPtr);

  dcds::LLVMCodegen::llvmVerifyFunction(fn);
  return fn;
}

llvm::Function *LLVMCodegen::buildInitTablesFn(dcds::Builder &builder, llvm::Value *table_name) {
  auto function_name = builder.getName() + "_init_storage";
  auto returnType = llvm::Type::getInt8PtrTy(getLLVMContext());
  auto fn_type = llvm::FunctionType::get(returnType, std::vector<llvm::Type *>{}, false);
  auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                                   builder.getName() + "_init_storage", getModule());
  userFunctions.emplace(function_name, fn);

  BasicBlock *entryBlock = BasicBlock::Create(getLLVMContext(), "entry", fn);
  getBuilder()->SetInsertPoint(entryBlock);

  Value *numAttributes = ConstantInt::get(getLLVMContext(), APInt(32, builder.attributes.size()));

  llvm::Value *tableNameCharPtr = getBuilder()->CreateBitCast(table_name, llvm::Type::getInt8PtrTy(getLLVMContext()));

  Type *cppEnumType = Type::getInt32Ty(getLLVMContext());
  ArrayType *enumArrayType = ArrayType::get(cppEnumType, builder.attributes.size());

  std::vector<Constant *> valueTypeArray;
  std::vector<std::string> names;

  for (const auto &a : builder.attributes) {
    valueTypeArray.push_back(ConstantInt::get(cppEnumType, std::to_underlying(a.second->type)));
    names.push_back(a.first);
  }

  Constant *constAttributeTypeArray = ConstantArray::get(enumArrayType, valueTypeArray);
  AllocaInst *allocaInstAttributeTypes = getBuilder()->CreateAlloca(enumArrayType, nullptr, "attributeTypeArray");
  getBuilder()->CreateStore(constAttributeTypeArray, allocaInstAttributeTypes);
  Value *gepIndices[] = {ConstantInt::get(Type::getInt32Ty(getLLVMContext()), 0),
                         ConstantInt::get(Type::getInt32Ty(getLLVMContext()), 0)};
  Value *elementPtrAttributeType = getBuilder()->CreateGEP(enumArrayType, allocaInstAttributeTypes, gepIndices);

  llvm::Value *attributeNames = createStringArray(names, builder.getName() + "_attr_");
  llvm::Value *attributeNamesFirstCharPtr = getBuilder()->CreateExtractValue(attributeNames, {0});

  llvm::Value *resultPtr = this->gen_call(
      createTablesInternal, {tableNameCharPtr, elementPtrAttributeType, attributeNamesFirstCharPtr, numAttributes});

  // return the table*
  getBuilder()->CreateRet(resultPtr);

  dcds::LLVMCodegen::llvmVerifyFunction(fn);
  return fn;
}

void LLVMCodegen::buildDestructor() {
  // TODO:
  //  delete records
  //  what about user-defined cleanups, if any.
}

void LLVMCodegen::initializeArrayAttributes(dcds::Builder &builder,
                                            std::map<std::string, llvm::Function *> &fn_init_sub_tables,
                                            llvm::Value *txn_manager, llvm::Value *main_record, llvm::Value *txn) {
  for (auto &[name, at] : builder.attributes) {
    // if we have an array list, we need to create child tables and set the pointer accordingly.
    //  also, make sure, nobody tries to change this pointer. it should be const!

    if (at->type_category == ATTRIBUTE_TYPE_CATEGORY::ARRAY_LIST) {
      auto attributeList = std::static_pointer_cast<AttributeList>(at);
      llvm::Value *index_ptr;
      llvm::Value *array_table;
      {
        // create-mapped-value-table

        std::string sub_table_name;
        llvm::Value *sub_table_name_llvm_const;
        llvm::Function *init_sub_table_fn;

        if (attributeList->is_primitive_type) {
          std::map<std::string, std::shared_ptr<Attribute>> tbl_attributes;
          tbl_attributes[attributeList->name] = attributeList->simple_type;
          std::string sub_table_name_prefix = builder.getName() + "_" + attributeList->name;

          sub_table_name = sub_table_name_prefix + "_tbl";
          sub_table_name_llvm_const = this->createStringConstant(sub_table_name, sub_table_name_prefix);
          // FIXME: Now out function generator will insert the IR in the middle, fix that.
          //  fix is simple, use/build the functionGenerator, so it can restore the entry point in IRBuilder.
          init_sub_table_fn =
              this->genInitStorageFn(sub_table_name_prefix, sub_table_name_llvm_const, builder.attributes);

        } else {
          sub_table_name = attributeList->composite_type->getName() + "_tbl";
          sub_table_name_llvm_const =
              this->createStringConstant(sub_table_name, attributeList->composite_type->getName());
          CHECK(fn_init_sub_tables.contains(attributeList->name))
              << "Storage init does not exists for child type: " << attributeList->composite_type->getName();
          init_sub_table_fn = fn_init_sub_tables[attributeList->name];
        }

        Value *fn_res_doesTableExists_sub =
            this->gen_call(doesTableExists, {sub_table_name_llvm_const}, Type::getInt1Ty(getLLVMContext()));
        auto phi_subTable = gen_phi();

        this->gen_if(fn_res_doesTableExists_sub)(
                [&]() { phi_subTable.emplace(this->gen_call(getTable, {sub_table_name_llvm_const})); })
            .gen_else([&]() { phi_subTable.emplace(this->gen_call(init_sub_table_fn, {})); });

        array_table = phi_subTable.get();
      }

      if (attributeList->is_fixed_size) {
        // fixed-size array
        auto attributeArray = std::static_pointer_cast<AttributeArray>(attributeList);

        // Until here, if the table was not created, we created the table for the array-attribute.
        // now we want to insert N records in the table, get the ptr to the base.

        // NOW, either this is a simple or complex type.
        llvm::Value *defaultValue;
        if (attributeArray->is_primitive_type) {
          defaultValue = this->createDefaultValue(attributeArray->simple_type);
        } else {
          defaultValue = this->initializeDsValueStructDefault(*(attributeArray->composite_type));
        }
        auto *len = this->createSizeT(attributeArray->len);
        index_ptr =
            this->gen_call(insertNRecords, {array_table, txn, defaultValue, len}, Type::getInt64Ty(getLLVMContext()));

      } else {
        // indexed-map : cuckoo, or whatever.

        // we have the table to mapped type. but we don't need to insert anything there.
        // but somehow, either have a name generator function or save the table name back in the attribute?

        auto indexedList = std::static_pointer_cast<AttributeIndexedList>(attributeList);
        CHECK(indexedList->is_primitive_type == false) << "IndexedList can only be of complex type";

        // create a cuckoo-map, or index_t with type<key_t, record_ptr>
        auto key_attribute = indexedList->composite_type->getAttribute(indexedList->key_attribute);

        // FIXME: createIndexMap is a fixed function which creates cuckooMap. we need it to be switch-able to
        //  cuckoo (hash), or some other range, or specific types.
        // ----> also add it to destructor.
        auto key_type = ConstantInt::get(Type::getInt32Ty(getLLVMContext()), std::to_underlying(key_attribute->type));
        index_ptr = this->gen_call(createIndexMap, {key_type}, Type::getInt64Ty(getLLVMContext()));
      }

      llvm::AllocaInst *allocaInst = getBuilder()->CreateAlloca(index_ptr->getType());
      getBuilder()->CreateStore(index_ptr, allocaInst);
      Value *indexPtrT = getBuilder()->CreateBitCast(allocaInst, llvm::Type::getInt8PtrTy(getLLVMContext()));

      this->gen_call(table_write_attribute,
                     {txn_manager, main_record, txn, indexPtrT, this->createSizeT(builder.getAttributeIndex(name))},
                     Type::getVoidTy(getLLVMContext()));
    }
  }
}

llvm::Function *LLVMCodegen::buildConstructorInner(dcds::Builder &builder) {
  std::string table_name = builder.getName() + "_tbl";
  LOG_IF(INFO, print_debug_log) << "[LLVMCodegen][buildConstructorInner] table_name: " << table_name;

  auto tableNameLlvmConstant = this->createStringConstant(table_name, "ds_table_name");

  bool hasAttributes = !(builder.attributes.empty());
  auto void_ptr_type = PointerType::get(Type::getInt8Ty(getLLVMContext()), 0);

  //  llvm::Function *fn_initTables = hasAttributes ? this->buildInitTablesFn(builder, tableNameLlvmConstant) : nullptr;
  llvm::Function *fn_initTables =
      hasAttributes ? this->genInitStorageFn(builder.getName(), tableNameLlvmConstant, builder.attributes) : nullptr;

  // mainly for indexed attribute which causes another table to be created.
  std::map<std::string, llvm::Function *> fn_init_sub_tables;
  for (auto &[name, at] : builder.attributes) {
    if (at->type_category == ATTRIBUTE_TYPE_CATEGORY::ARRAY_LIST) {
      LOG_IF(INFO, print_debug_log) << "ARRAY_LIST: " << name;
      auto attributeList = std::static_pointer_cast<AttributeList>(at);
      // gen tables or whatever to get them started
      if (attributeList->is_primitive_type) {
        assert(false);
      } else {
        auto initSubTableFnName = attributeList->composite_type->getName() + "_init_storage";

        CHECK(userFunctions.contains(initSubTableFnName)) << "Sub-type has no _init_storage?? :" << initSubTableFnName;
        LOG_IF(INFO, print_debug_log) << "initSubTableFnName: " << initSubTableFnName << " | "
                                      << userFunctions.at(initSubTableFnName);
        fn_init_sub_tables.insert_or_assign(attributeList->name, userFunctions.at(initSubTableFnName));
      }
    }
  }

  // inner args: { txnManager*, txn* }
  auto returnType = PointerType::getUnqual(Type::getInt8Ty(getLLVMContext()));
  auto fn_type = llvm::FunctionType::get(returnType, std::vector<llvm::Type *>{void_ptr_type, void_ptr_type}, false);

  auto function_name = builder.getName() + "_constructor_inner";
  auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::PrivateLinkage, function_name,
                                   theLLVMModule.get());
  userFunctions.emplace(function_name, fn);

  auto fn_bb = llvm::BasicBlock::Create(getLLVMContext(), "entry", fn);
  getBuilder()->SetInsertPoint(fn_bb);

  Value *arg_txnManger = fn->getArg(0);
  Value *arg_txn = fn->getArg(1);
  llvm::Value *mainRecordRef;

  // Only create table and related logic when DS actually has concurrent attributes.
  if (hasAttributes) {
    Value *fn_res_doesTableExists =
        this->gen_call(doesTableExists, {tableNameLlvmConstant}, Type::getInt1Ty(getLLVMContext()));

    auto phiNode = gen_phi();

    this->gen_if(fn_res_doesTableExists)([&]() { phiNode.emplace(this->gen_call(getTable, {tableNameLlvmConstant})); })
        .gen_else([&]() { phiNode.emplace(this->gen_call(fn_initTables, {})); });

    llvm::Value *tablePtrValue = phiNode.get();

    // insert a record in table here.
    llvm::Value *defaultInsValues = this->initializeDsValueStructDefault(builder);
    mainRecordRef = this->gen_call(insertMainRecord, {tablePtrValue, arg_txn, defaultInsValues});

    this->initializeArrayAttributes(builder, fn_init_sub_tables, arg_txnManger, mainRecordRef, arg_txn);

    //    for (auto &at : builder.attributes) {
    //      if (at.second->type_category == ATTRIBUTE_TYPE_CATEGORY::UNORDERED_MAP) {
    //        // initialize it.
    //
    //        auto at_ty = std::static_pointer_cast<UnOrderedMapAttribute>(at.second);
    //
    //        auto keyType = ConstantInt::get(Type::getInt32Ty(getLLVMContext()), std::to_underlying(at_ty->key_type));
    //        auto cuckooPtr = this->gen_call(getCuckooMap, {keyType});
    //
    //        this->gen_call(
    //            table_write_attribute,
    //            {arg_txnManger, mainRecordRef, arg_txn, cuckooPtr,
    //            this->createSizeT(builder.getAttributeIndex(at.first))}, Type::getVoidTy(getLLVMContext()));
    //      }
    //    }

  } else {
    mainRecordRef = this->createInt64(UINT64_C(0));
  }
  llvm::Value *returnDs = this->gen_call(createDsContainer, {arg_txnManger, mainRecordRef});

  getBuilder()->CreateRet(returnDs);

  llvmVerifyFunction(fn);
  LOG_IF(INFO, print_debug_log) << "buildConstructorInner DONE";
  return fn;
}

void LLVMCodegen::buildConstructor(dcds::Builder &builder, bool is_nested_type) {
  // FIXME: get namespace prefix from context? buildContext or runtimeContext?
  //  maybe take it from runtime context to enable cross-DS or intra-DS txns.
  std::string namespace_prefix = "default";  //"default_namespace";

  auto linkageType = is_nested_type ? llvm::GlobalValue::LinkageTypes::PrivateLinkage
                                    : llvm::GlobalValue::LinkageTypes::ExternalLinkage;

  auto namespaceLlvmConstant = this->createStringConstant(namespace_prefix, "txn_namespace_prefix");

  auto *fn_constructor_inner = buildConstructorInner(builder);

  if (!is_nested_type) {
    auto fn_type = llvm::FunctionType::get(fn_constructor_inner->getReturnType(), std::vector<llvm::Type *>{}, false);
    auto function_name = builder.getName() + "_constructor";
    auto fn = llvm::Function::Create(fn_type, linkageType, function_name, theLLVMModule.get());
    userFunctions.emplace(function_name, fn);

    auto fn_bb = llvm::BasicBlock::Create(getLLVMContext(), "entry", fn);
    getBuilder()->SetInsertPoint(fn_bb);

    Value *fn_res_getTxnManger = this->gen_call(getTxnManager, {namespaceLlvmConstant});

    // Begin Txn
    Value *fn_res_beginTxn = this->gen_call(beginTxn, {fn_res_getTxnManger, this->createFalse()});

    // Call constructor_inner
    llvm::Value *inner_fn_res = this->gen_call(fn_constructor_inner, {fn_res_getTxnManger, fn_res_beginTxn});

    // Commit Txn
    this->gen_call(endTxn, {fn_res_getTxnManger, fn_res_beginTxn}, Type::getInt1Ty(getLLVMContext()));

    //  this->gen_call(printPtr, {returnDs});

    getBuilder()->CreateRet(inner_fn_res);

    llvmVerifyFunction(fn);
  }

  LOG_IF(INFO, print_debug_log) << "buildConstructor DONE";
}

void LLVMCodegen::codegenHelloWorld() {
  // Create a function named "main" with return type void
  llvm::FunctionType *funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(getLLVMContext()), false);
  llvm::Function *mainFunction =
      llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "helloworld", getModule());

  // Create a basic block in the function
  llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(getLLVMContext(), "entry", mainFunction);

  getBuilder()->SetInsertPoint(entryBlock);

  auto helloWorldCharPtr = this->createStringConstant("Hello, World!", "");

  this->gen_call(prints, {helloWorldCharPtr});

  getBuilder()->CreateRetVoid();

  // Verify the function for correctness
  llvmVerifyFunction(mainFunction);
}

void LLVMCodegen::testHelloWorld() {
  assert(this->jitter);
  LOG_IF(INFO, print_debug_log) << "[testHelloWorld] GetRawAddress(\"helloworld\")"
                                << this->jitter->getRawAddress("helloworld");

  LOG_IF(INFO, print_debug_log) << "[testHelloWorld] Test helloWorld: ";
  reinterpret_cast<void (*)()>(this->jitter->getRawAddress("helloworld"))();
}

void LLVMCodegen::runOptimizationPasses() {
  for (auto &F : *theLLVMModule) theLLVMFPM->run(F);
}

void *LLVMCodegen::getFunction(const std::string &name) { return this->jitter->getRawAddress(name); }
void *LLVMCodegen::getFunctionPrefixed(const std::string &name) {
  return this->jitter->getRawAddress(top_level_builder->getName() + "_" + name);
}

void LLVMCodegen::buildFunctionDictionary(dcds::Builder &builder) {
  assert(this->is_jit_done);
  for (auto &fb : builder.functions) {
    // LOG_IF(INFO, print_debug_log) << "Resolving address: " << fb.first;
    auto *address = getFunctionPrefixed(fb.first);
    auto name = fb.first;
    auto return_type = fb.second->returnValueType;
    auto args_expr = fb.second->getArguments();
    std::vector<std::pair<std::string, dcds::valueType>> args;
    for (auto &fa : args_expr) {
      args.emplace_back(fa->getName(), fa->getType());
    }
    LOG_IF(INFO, print_debug_log) << "Resolving address: " << fb.first << " | " << address;
    available_jit_functions.emplace(name, new jit_function_t{name, address, return_type, args});
  }
}

void LLVMCodegen::jitCompileAndLoad() {
  this->jitter = std::make_unique<LLVMJIT>();
  this->theLLVMModule->setDataLayout(jitter->getDataLayout());

  LOG_IF(INFO, print_debug_log) << "Module name: " << getModule()->getName().str();

  // llvm::orc::ThreadSafeContext NewTSCtx(std::make_unique<LLVMContext>());
  // auto TSM = llvm::orc::ThreadSafeModule(std::move(theLLVMModule), std::move(NewTSCtx));

  auto TSM = llvm::orc::ThreadSafeModule(std::move(theLLVMModule), std::move(theLLVMContext));

  this->jitter->addModule(std::move(TSM));
  // this->jitter->dump();

  this->is_jit_done = true;
  this->buildFunctionDictionary(*top_level_builder);
}

}  // namespace dcds
