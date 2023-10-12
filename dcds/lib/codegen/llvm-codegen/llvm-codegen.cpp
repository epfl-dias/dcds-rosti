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

#include "dcds/builder/function-builder.hpp"
#include "dcds/codegen/llvm-codegen/functions.hpp"
#include "dcds/codegen/llvm-codegen/llvm-jit.hpp"

namespace dcds {

void LLVMCodegen::saveToFile(const std::string &filename) {
  std::error_code errorCode;
  llvm::raw_fd_ostream outLL(filename, errorCode);
  theLLVMModule->print(outLL, nullptr);
}

void LLVMCodegen::printIR() { theLLVMModule->print(llvm::outs(), nullptr); };

llvm::Type *toLLVMType(LLVMContext &context, dcds::valueType dcds_type) {
  switch (dcds_type) {
    case INTEGER:
    case RECORD_ID:
    case RECORD_PTR: {
      return llvm::Type::getInt64Ty(context);
    }
    case FLOAT: {
      return llvm::Type::getFloatTy(context);
    }
    case CHAR:
    case VOID:
    default:
      assert(false && "valueTypeNotSupportedYet");
      break;
  }
}

void LLVMCodegen::initializeLLVMModule(const std::string &name) {
  LOG(INFO) << "[LLVMCodegen] Initializing LLVM module: " << name;
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  theLLVMContext = std::make_unique<LLVMContext>();

  theLLVMModule = std::make_unique<Module>(name, *theLLVMContext);
  llvmBuilder = std::make_unique<IRBuilder<>>(*theLLVMContext);
}

LLVMCodegen::LLVMCodegen(Builder &builder) : Codegen(builder), LLVMCodegenContext(builder.getName()) {
  initializeLLVMModule(builder.getName());
  initializePassManager();
  this->registerAllFunctions();
}

llvm::Module *LLVMCodegen::getModule() const { return theLLVMModule.get(); }
llvm::IRBuilder<> *LLVMCodegen::getBuilder() const { return llvmBuilder.get(); }

void LLVMCodegen::initializePassManager() {
  LOG(INFO) << "[LLVMCodegen] Initializing LLVM pass manager";
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

  /// TODO: Eliminate unnecessary code generation at its source (wherever it makes sense to do so).
  theLLVMFPM->add(createDeadCodeEliminationPass());

  theLLVMFPM->doInitialization();
}

void LLVMCodegen::build() {
  LOG(INFO) << "[LLVMCodegen] Building: " << builder.getName();

  codegenHelloWorld();

  LOG(INFO) << "[LLVMCodegen::build()] createDsContainerStruct";
  this->createDsContainerStruct();
  LOG(INFO) << "[LLVMCodegen::build()] createDsStructType";
  this->createDsStructType();
  LOG(INFO) << "[LLVMCodegen::build()] buildConstructor";
  this->buildConstructor();

  LOG(INFO) << "[LLVMCodegen::build()] BuildFunctions";
  this->buildFunctions();
  LOG(INFO) << "[LLVMCodegen::build()] DONE";
}

void LLVMCodegen::buildFunctions() {
  LOG(INFO) << "[LLVMCodegen] buildFunctions: # of functions: " << builder.functions.size();

  // later: parallelize this loop, but do take care of the context insertion points in parallel function build.
  for (auto &fb : builder.functions) {
    this->buildOneFunction(fb.second);
  }
}

// void LLVMCodegen::wrapOne(){
//
//   auto ptrType = IntegerType::getInt8PtrTy(getLLVMContext());
//   auto i32Type = IntegerType::getInt32Ty(getLLVMContext());
//   auto uintPtrType = IntegerType::getInt64Ty(getLLVMContext());
//
//   std::string function_name = "test_va_function";
//   // First three args are stable: txnManager*, table*, mainRecord
//   std::vector<llvm::Type *> argTypes = {
//       ptrType,     //     arg1 <void*>: txnManager*
//       ptrType,     //     arg2 <void*>: table*
//       uintPtrType  //  arg3 <uintptr_t>: mainRecord
//   };
//
// }

llvm::Function *LLVMCodegen::wrapFunctionVariadicArgs(llvm::Function *inner_function,
                                                      std::vector<llvm::Type *> position_args,
                                                      std::vector<llvm::Type *> expected_vargs,
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

void LLVMCodegen::buildOneFunction(std::shared_ptr<FunctionBuilder> &fb) {
  LOG(INFO) << "[LLVMCodegen] buildOneFunction: " << fb->_name;

  // FIXME: How to ensure that this function returns in all paths? and returns with correct type in all paths?
  // FIXME: what about scoping of temporary variables??
  // FIXME: this would create nested transactions! we need to make sure we dont call txn if there is recursive function.

  auto ptrType = IntegerType::getInt8PtrTy(getLLVMContext());
  auto uintPtrType = IntegerType::getInt64Ty(getLLVMContext());

  //  argTypes.push_back(ptrType);      // txnManager*
  //  argTypes.push_back(ptrType);      // table*
  //  argTypes.push_back(uintPtrType);  // mainRecord

  // 1- generate function signatures
  auto fn_name_prefix = builder.getName() + "_";

  // pre_args: { txnManager*, table*, mainRecord }
  std::vector<llvm::Type *> fn_outer_args{ptrType, ptrType, uintPtrType};
  auto fn_outer = this->genFunctionSignature(fb, fn_outer_args, fn_name_prefix);
  // pre_args: { txnManager*, table*, mainRecord, txnPtr }
  auto fn_inner = this->genFunctionSignature(fb, {ptrType, ptrType, uintPtrType, ptrType}, fn_name_prefix, "_inner");

  userFunctions.push_back(fn_outer);

  // generate inner function first.

  // 2- set insertion point at the beginning of the function.
  auto fn_inner_BB = llvm::BasicBlock::Create(getLLVMContext(), "entry", fn_inner);
  llvmBuilder->SetInsertPoint(fn_inner_BB);

  // 3- allocate temporary variables.
  LOG(INFO) << "codegen- temporary variables";
  auto variableCodeMap = this->allocateTemporaryVariables(fb, fn_inner_BB);

  // 4- codegen all statements
  LOG(INFO) << "codegen-statements";

  for (auto &s : fb->statements) {
    // check if statement does not return internally else we call endTxn inside.
    this->buildStatement(s, fb, fn_inner, fn_inner_BB, variableCodeMap);
  }
  dcds::LLVMCodegen::llvmVerifyFunction(fn_inner);

  // NOTE: as we dont know the function inner return paths, lets create a wrapper function which will do the txn stuff,
  //  and then call the inner function with txn ptr.

  // CODEGEN outer function.
  auto fn_outer_BB = llvm::BasicBlock::Create(getLLVMContext(), "entry", fn_outer);
  llvmBuilder->SetInsertPoint(fn_outer_BB);

  // ###### BeginTxn
  auto txnManager = fn_outer->getArg(0);
  auto storageTable = fn_outer->getArg(1);
  auto mainRecord = fn_outer->getArg(2);
  Value *fn_res_beginTxn = this->gen_call(beginTxn, {fn_outer->getArg(0)});

  // create call to inner_function here.
  // start from 3, as 0-2 are already taken.
  std::vector<llvm::Value *> inner_args{txnManager, storageTable, mainRecord, fn_res_beginTxn};
  for (auto i = 3; i < fn_outer->arg_size(); i++) {
    inner_args.push_back(fn_outer->getArg(i));
  }

  auto inner_ret = getBuilder()->CreateCall(fn_inner, inner_args);

  // ###### Commit Txn
  this->gen_call(commitTxn, {txnManager, fn_res_beginTxn});

  if (fn_inner->getReturnType()->getTypeID() == llvm::Type::getVoidTy(getLLVMContext())->getTypeID()) {
    getBuilder()->CreateRetVoid();
  } else {
    getBuilder()->CreateRet(inner_ret);
  }

  dcds::LLVMCodegen::llvmVerifyFunction(fn_outer);

  // so we have a function R f(void*,void*,void*, A,B,C)
  // we want it wrapped to R f(void*,void*,void*,...)

  std::vector<llvm::Type *> expected_vargs;
  for (const auto &arg : fb->function_arguments) {
    expected_vargs.push_back(toLLVMType(getLLVMContext(), arg.second));
  }

  this->wrapFunctionVariadicArgs(fn_outer, fn_outer_args, expected_vargs, fn_outer->getName().str() + "_vargs");

  // Optimizations:
  //    easy: if an attribute is only accessed in a single function across DS, then you dont need CC either on that one.
  //    hard: if the group-sequence of attribute is common across all functions,
  //    then encapsulate them as a single CC-variable.

  // rw_set = fb->getReadWriteSet();
  // CcBuilder::injectCC(rw_set)
  // CcBuilder::injectTxnBegin

  // ALSO, start a txn here, so that it could be passed everywhere!!

  // CcBuilder::injectTxnEnd

  // 4- Add return statement.
  // FIXME: add void return if there is not return in the statements.
  //  LOG(INFO) << "codegen-return";
  //  if (fb->returnValueType == dcds::valueType::VOID) {
  //    llvmBuilder->CreateRetVoid();
  //  }

  // later: mark this function as done?
}

void LLVMCodegen::buildStatement(std::shared_ptr<StatementBuilder> &sb, std::shared_ptr<FunctionBuilder> &fb,
                                 llvm::Function *fn, llvm::BasicBlock *basicBlock,
                                 std::map<std::string, llvm::Value *> &tempVariableMap) {
  // FIXME: what about scoping of temporary variables??
  LOG(INFO) << "[LLVMCodegen] buildStatement";

  auto txnManager = fn->getArg(0);
  auto storageTable = fn->getArg(1);
  auto mainRecord = fn->getArg(2);
  auto txn = fn->getArg(3);
  size_t fn_arg_idx_st = 4;

  // READ, UPDATE, YIELD, TEMP_VAR_ADD, CONDITIONAL_STATEMENT, CALL
  if (sb->stType == dcds::statementType::READ) {
    LOG(INFO) << "StatementType: read";
    // actionVar -> txnVariable to read from
    // referenceVar -> destination.

    if (!builder.hasAttribute(sb->actionVarName)) {
      assert(false && "read attribute does not exists");
    }

    // auto readFromAttribute = builder.getAttribute(sb->actionVarName);
    assert(builder.hasAttribute(sb->actionVarName));
    assert(tempVariableMap.contains(sb->refVarName));
    auto readDst = tempVariableMap[sb->refVarName];

    //    extern "C" void table_read_attribute(
    //    void* _txnManager, void* _storageTable, uintptr_t _mainRecord,
    //    void* txn, void* dst, uint attributeIdx);

    // TODO: what about type mismatch in read/write attributes?
    this->gen_call(table_read_attribute,
                   {txnManager, storageTable, mainRecord, txn, readDst,
                    this->createSizeT(builder.getAttributeIndex(sb->actionVarName))},
                   Type::getVoidTy(getLLVMContext()));

  } else if (sb->stType == dcds::statementType::UPDATE) {
    LOG(INFO) << "StatementType: update";

    // actionVar -> txnVariable to write to
    // referenceVar -> source.

    auto updateSb = std::static_pointer_cast<UpdateStatementBuilder>(sb);

    // auto writeToAttribute = builder.getAttribute(sb->actionVarName);
    assert(builder.hasAttribute(sb->actionVarName));

    llvm::Value *updateSource;
    if (updateSb->source_type == TEMPORARY_VARIABLE) {
      assert(tempVariableMap.contains(sb->refVarName));

      // FIXME: check and verify that if we need the same for the above? the variable itself is from alloca, and has the
      // store, so maybe we just need bit cast. to-be verified.
      //  for now, doing the bit-cast, if it works, then works, else remove it.
      // updateSource = tempVariableMap[updateSb->refVarName];

      updateSource = getBuilder()->CreateBitCast(tempVariableMap[updateSb->refVarName],
                                                 llvm::Type::getInt8PtrTy(getLLVMContext()));
    } else if (updateSb->source_type == FUNCTION_ARGUMENT) {
      // NOTE: because are update function expects a void* to the src, we need to create a temporary allocation.
      auto source_arg = fn->getArg(fb->getArgumentIndex(updateSb->refVarName) + fn_arg_idx_st);

      llvm::AllocaInst *allocaInst = getBuilder()->CreateAlloca(source_arg->getType());
      getBuilder()->CreateStore(source_arg, allocaInst);

      // Bit-cast the value to i8*
      updateSource = getBuilder()->CreateBitCast(allocaInst, llvm::Type::getInt8PtrTy(getLLVMContext()));

    } else {
      assert(false && "what?");
    }

    //    extern "C" void table_write_attribute(void* _txnManager, void* _storageTable, uintptr_t _mainRecord, void*
    //    txnPtr, void* src, uint attributeIdx);

    this->gen_call(table_write_attribute,
                   {txnManager, storageTable, mainRecord, txn, updateSource,
                    this->createSizeT(builder.getAttributeIndex(sb->actionVarName))},
                   Type::getVoidTy(getLLVMContext()));

  } else if (sb->stType == dcds::statementType::YIELD) {
    LOG(INFO) << "StatementType: return";
    if (sb->refVarName.empty()) {
      LOG(INFO) << "void return because sb->refVarName.empty()";
      getBuilder()->CreateRetVoid();
    } else {
      if (tempVariableMap.contains(sb->refVarName)) {
        // We know temporary variables are allocated through llvm, hence, do a CreateLoad on it.

        llvm::Value *retValue = getBuilder()->CreateLoad(fn->getReturnType(), tempVariableMap[sb->refVarName]);
        getBuilder()->CreateRet(retValue);
      } else {
        assert(false && "reference variable not allocated?");
      }
    }

  } else {
    assert(false);
  }
}

// What about CC? nothing about CC yet, essentially,
// it should be injected in starting and ending of each function.
// We would need the read/write set, that should be easy though.

// For latching, each statementBuild will take care of that, actually.

llvm::Function *LLVMCodegen::genFunctionSignature(std::shared_ptr<FunctionBuilder> &fb,
                                                  const std::vector<llvm::Type *> &pre_args,
                                                  const std::string &name_prefix, const std::string &name_suffix) {
  std::vector<llvm::Type *> argTypes(pre_args);
  llvm::Type *returnType;

  // NOTE: hardcode first argument: to be the mainStructPtr (pseudo this)
  //  argTypes.push_back(PointerType::get(dsContainerStructType, 0));

  //  struct dcds_jit_container_t {
  //    // txnManager, table, mainRecord
  //    dcds::txn::TransactionManager *txnManager;
  //    dcds::storage::Table *storageTable;
  //    uintptr_t mainRecord;  // can it be directly record_reference_t?
  //  };
  // Happens in pre-args now.
  //  auto ptrType = IntegerType::getInt8PtrTy(getLLVMContext());
  //  auto uintPtrType = IntegerType::getInt64Ty(getLLVMContext());
  //  argTypes.push_back(ptrType);      // txnManager*
  //  argTypes.push_back(ptrType);      // table*
  //  argTypes.push_back(uintPtrType);  // mainRecord

  for (const auto &arg : fb->function_arguments) {
    argTypes.push_back(toLLVMType(getLLVMContext(), arg.second));
  }

  if (fb->returnValueType == VOID) {
    returnType = llvm::Type::getVoidTy(getLLVMContext());
  } else {
    returnType = toLLVMType(getLLVMContext(), fb->returnValueType);
  }

  auto fn_type = llvm::FunctionType::get(returnType, argTypes, false);
  auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                   name_prefix + fb->_name + name_suffix, theLLVMModule.get());

  if (fb->isAlwaysInline()) {
    fn->addFnAttr(llvm::Attribute::AlwaysInline);
  }
  return fn;
}

std::map<std::string, llvm::Value *> LLVMCodegen::allocateTemporaryVariables(std::shared_ptr<FunctionBuilder> &fb,
                                                                             llvm::BasicBlock *basicBlock) {
  std::map<std::string, llvm::Value *> variableCodeMap;

  auto allocaBuilder = llvm::IRBuilder<>(basicBlock, basicBlock->end());

  for (auto &v : fb->temp_variables) {
    auto var_name = v.first;
    auto var_type = v.second.first;
    auto init_value = v.second.second;
    bool has_value = init_value.has_value();

    switch (var_type) {
      case INTEGER:
      case RECORD_PTR: {
        auto int64_type = llvm::Type::getInt64Ty(getLLVMContext());
        auto vr = allocaBuilder.CreateAlloca(int64_type, nullptr, var_name);
        if (has_value) {
          auto value = createInt64(std::any_cast<uint64_t>(init_value));
          allocaBuilder.CreateStore(value, vr);
        }

        //        LOG(INFO) << "Allocating temp variable: " << var_name << " | " << vr->getType()->getTypeID() << " | "
        //        << int64_type->getTypeID();
        variableCodeMap.emplace(var_name, vr);
        break;
      }
        //      case VOID:{
        //        auto vr = allocaBuilder.CreateAlloca(llvm::Type::getVoidTy(getLLVMContext()), nullptr, var_name);
        //        variableCodeMap.emplace(var_name, vr);
        //        break;
        //      }
      case VOID:
      case FLOAT:
      case RECORD_ID:
      case CHAR:

      default:
        assert(false && "[allocateTemporaryVariables] valueTypeNotSupportedYet");
        break;
    }
  }

  return variableCodeMap;
}

void LLVMCodegen::createDsContainerStruct() {
  // NOTE: do not change the format of the struct, otherwise,
  // also change in the jit-container (and maybe code-exporter to match it).
  std::string ds_struct_name = "struct_container_" + builder.getName() + "_t";
  bool is_packed = false;
  auto ptrType = IntegerType::getInt8PtrTy(getLLVMContext());
  auto sizeTType = IntegerType::getInt64Ty(getLLVMContext());

  std::vector<llvm::Type *> struct_vars = {ptrType, ptrType, sizeTType};

  // basically having, txnManager, table, mainRecord.

  dsContainerStructType = StructType::create(getLLVMContext(), struct_vars, ds_struct_name, is_packed);
}

void LLVMCodegen::createDsStructType() {
  std::string ds_struct_name = builder.getName() + "_t";
  bool is_packed = true;  // Important!! as the insertRecord takes and directly copies the memory.

  // for each attribute, add it to struct.
  // Later, also add this struct to the output .hpp file.

  std::vector<llvm::Type *> struct_vars;
  for (const auto &a : builder.attributes) {
    // simpleType to type, else have a ptr to it.
    struct_vars.push_back(toLLVMType(getLLVMContext(), a.second->type));
  }

  dsRecordValueStructType = StructType::create(getLLVMContext(), struct_vars, ds_struct_name, is_packed);
}

Value *LLVMCodegen::initializeDsContainerStruct(Value *txnManager, Value *storageTable, Value *mainRecord) {
  LOG(INFO) << "dsContainerStructType: " << dsContainerStructType->getName().data();
  Value *structValue = llvm::UndefValue::get(dsContainerStructType);
  LOG(INFO) << "dsContainerStructType--1";

  // txnManager, table, mainRecord
  structValue = getBuilder()->CreateInsertValue(structValue, txnManager, {0});
  LOG(INFO) << "dsContainerStructType--2";
  structValue = getBuilder()->CreateInsertValue(structValue, storageTable, {1});
  LOG(INFO) << "dsContainerStructType--3";
  structValue = getBuilder()->CreateInsertValue(structValue, mainRecord, {2});
  LOG(INFO) << "dsContainerStructType--4";

  llvm::AllocaInst *structPtr = getBuilder()->CreateAlloca(dsContainerStructType);
  getBuilder()->CreateStore(structValue, structPtr);

  return structPtr;
}

Value *LLVMCodegen::initializeDsValueStructDefault() {
  llvm::Value *structValue = UndefValue::get(dsRecordValueStructType);
  auto *irBuilder = getBuilder();

  uint i = 0;
  for (const auto &a : builder.attributes) {
    auto defaultValue = a.second->getDefaultValue();
    auto hasValue = defaultValue.has_value();
    LOG(INFO) << "[initializeDsValueStructDefault] Processing: " << a.first
              << " | has_value: " << defaultValue.has_value();

    llvm::Value *fieldValue;

    switch (a.second->type) {
      case INTEGER: {
        auto value = hasValue ? std::any_cast<uint64_t>(defaultValue) : 0;
        fieldValue = createInt64(value);
        break;
      }
      case RECORD_PTR: {
        // RECORD_PTR is a uintptr underlying.
        fieldValue = createUintptr(0);
        //        fieldValue = llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(getLLVMContext()));
        //        // fixme: isn't record_ptr will be int? as internally its integer.
        //        if(hasValue){
        //          LOG(INFO) << "has_value:";
        //          CastPtrToLlvmPtr(llvm::Type::getInt8PtrTy(getLLVMContext()), std::any_cast<void*>(fieldValue));
        //        } else {
        //          fieldValue = llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(getLLVMContext()));
        //        }
        break;
      }
      case VOID:
      case FLOAT:
      case RECORD_ID:
      case CHAR:
      default:
        assert(false && "[allocateTemporaryVariables] valueTypeNotSupportedYet");
        break;
    }

    LOG(INFO) << "CreateInsertValue";
    fieldValue->dump();
    structValue = irBuilder->CreateInsertValue(structValue, fieldValue, {i});

    // fixme: they should be simple type.
    // get default values for them.
    //    a.second->getDefaultValue();
    //    structValue = llvmBuilder->CreateInsertValue(structValue, field1, {0});
    i++;
  }

  structValue->dump();
  structValue->getType()->dump();
  LOG(INFO) << "Done";

  llvm::AllocaInst *structPtr = irBuilder->CreateAlloca(dsRecordValueStructType);
  irBuilder->CreateStore(structValue, structPtr);

  return structPtr;
  //  return structValue;
}

llvm::Function *LLVMCodegen::buildInitTablesFn(llvm::Value *table_name) {
  auto returnType = llvm::Type::getInt8PtrTy(getLLVMContext());
  auto fn_type = llvm::FunctionType::get(returnType, std::vector<llvm::Type *>{}, false);
  auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                   builder.getName() + "_init_storage", getModule());
  userFunctions.push_back(fn);

  BasicBlock *entryBlock = BasicBlock::Create(getLLVMContext(), "entry", fn);
  llvmBuilder->SetInsertPoint(entryBlock);

  Value *numAttributes = ConstantInt::get(getLLVMContext(), APInt(32, builder.attributes.size()));

  llvm::Value *tableNameCharPtr = llvmBuilder->CreateBitCast(table_name, llvm::Type::getInt8PtrTy(getLLVMContext()));

  //  Value *c1Res = this->gen_call(c1, {tableNameCharPtr});
  //  Value *c2Res = this->gen_call(c2, {numAttributes});

  Type *cppEnumType = Type::getInt32Ty(getLLVMContext());
  ArrayType *enumArrayType = ArrayType::get(cppEnumType, builder.attributes.size());

  std::vector<Constant *> valueTypeArray;
  std::vector<std::string> names;

  for (const auto &a : builder.attributes) {
    valueTypeArray.push_back(ConstantInt::get(cppEnumType, a.second->type));
    names.push_back(a.first);
  }

  Constant *constAttributeTypeArray = ConstantArray::get(enumArrayType, valueTypeArray);
  AllocaInst *allocaInstAttributeTypes = llvmBuilder->CreateAlloca(enumArrayType, nullptr, "attributeTypeArray");
  llvmBuilder->CreateStore(constAttributeTypeArray, allocaInstAttributeTypes);
  Value *gepIndices[] = {ConstantInt::get(Type::getInt32Ty(getLLVMContext()), 0),
                         ConstantInt::get(Type::getInt32Ty(getLLVMContext()), 0)};
  Value *elementPtrAttributeType = llvmBuilder->CreateGEP(enumArrayType, allocaInstAttributeTypes, gepIndices);

  // Value *c3Res = this->gen_call(c3, {elementPtrAttributeType});

  llvm::Value *attributeNames = createStringArray(names, builder.getName() + "_attr_");
  // attributeNames->getType()->dump();
  llvm::Value *attributeNamesFirstCharPtr = llvmBuilder->CreateExtractValue(attributeNames, {0});

  // Value *c4Res = this->gen_call(c4, {attributeNamesFirstCharPtr});
  llvm::Value *resultPtr = this->gen_call(
      createTablesInternal, {tableNameCharPtr, elementPtrAttributeType, attributeNamesFirstCharPtr, numAttributes});

  // return the table*
  llvmBuilder->CreateRet(resultPtr);

  dcds::LLVMCodegen::llvmVerifyFunction(fn);
  return fn;
}

void LLVMCodegen::buildDestructor() {
  // TODO:
  //  delete records
  //  what about user-defined cleanups, if any.
}

void LLVMCodegen::buildConstructor() {
  // FIXME: get namespace prefix from context? buildContext or runtimeContext?
  //  maybe take it from runtime context to enable cross-DS or intra-DS txns.
  std::string namespace_prefix = "default_namespace";
  // FIXME: do we need namespace prefix on the table name? i dont think so. check and confirm!
  std::string table_name = namespace_prefix + "_" + builder.getName();

  auto namespaceLlvmConstant = this->createStringConstant(namespace_prefix, "txn_namespace_prefix");
  auto tableNameLlvmConstant = this->createStringConstant(table_name, "ds_table_name");
  auto fn_initTables = this->buildInitTablesFn(tableNameLlvmConstant);

  auto returnType = PointerType::getUnqual(Type::getInt8Ty(getLLVMContext()));
  auto fn_type = llvm::FunctionType::get(returnType, std::vector<llvm::Type *>{}, false);

  auto fn = llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                   builder.getName() + "_constructor", theLLVMModule.get());
  userFunctions.push_back(fn);

  auto fn_bb = llvm::BasicBlock::Create(getLLVMContext(), "entry", fn);
  llvmBuilder->SetInsertPoint(fn_bb);

  Value *fn_res_getTxnManger = this->gen_call(getTxnManager, {namespaceLlvmConstant});

  Value *fn_res_doesTableExists =
      this->gen_call(doesTableExists, {tableNameLlvmConstant}, Type::getInt1Ty(getLLVMContext()));

  //  // Create a conditional branch based on the external function's result
  llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(getLLVMContext(), "then", fn);
  llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(getLLVMContext(), "else", fn);
  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(getLLVMContext(), "merge", fn);
  llvmBuilder->CreateCondBr(fn_res_doesTableExists, thenBlock, elseBlock);

  llvmBuilder->SetInsertPoint(thenBlock);
  // Add logic for thenBlock
  //  if: table exits, get pointer to it.
  Value *thenValue = this->gen_call(getTable, {tableNameLlvmConstant});
  llvmBuilder->CreateBr(mergeBlock);

  // Add logic for elseBlock
  //  else : create tables which return the table*.
  llvmBuilder->SetInsertPoint(elseBlock);
  Value *elseValue = gen_call(fn_initTables, {});
  llvmBuilder->CreateBr(mergeBlock);

  // Set insertion point to the merge block
  llvmBuilder->SetInsertPoint(mergeBlock);

  // Phi node to merge the values from "then" and "else" blocks
  llvm::PHINode *phiNode = llvmBuilder->CreatePHI(llvm::Type::getInt8PtrTy(getLLVMContext()), 2);
  phiNode->addIncoming(thenValue, thenBlock);
  phiNode->addIncoming(elseValue, elseBlock);
  llvm::Value *tablePtrValue = phiNode;

  this->createPrintString("tablePtrValue Got done");

  // Begin Txn
  Value *fn_res_beginTxn = this->gen_call(beginTxn, {fn_res_getTxnManger});
  this->createPrintString("fn_res_beginTxn done");

  // insert a record in table here.
  llvm::Value *defaultInsValues = this->initializeDsValueStructDefault();
  this->createPrintString("defaultInsValues done");
  Value *fn_res_insertMainRecord = this->gen_call(insertMainRecord, {tablePtrValue, fn_res_beginTxn, defaultInsValues});

  // Commit Txn
  this->gen_call(commitTxn, {fn_res_getTxnManger, fn_res_beginTxn});

  //--
  llvm::Value *returnDs =
      this->gen_call(createDsContainer, {fn_res_getTxnManger, tablePtrValue, fn_res_insertMainRecord});

  //  this->createPrintString("returning now:");
  //  this->gen_call(printPtr, {returnDs});

  llvmBuilder->CreateRet(returnDs);
  //--

  llvmVerifyFunction(fn);
  LOG(INFO) << "buildConstructor DONE";
}

void LLVMCodegen::codegenHelloWorld() {
  // Create a function named "main" with return type void
  llvm::FunctionType *funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(getLLVMContext()), false);
  llvm::Function *mainFunction =
      llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "helloworld", getModule());

  // Create a basic block in the function
  llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(getLLVMContext(), "entry", mainFunction);

  llvmBuilder->SetInsertPoint(entryBlock);

  // Create a constant string for "Hello, World!"
  // llvm::Value *helloWorldStr = llvmBuilder->CreateGlobalString("Hello, World!");

  auto helloWorldCharPtr = this->createStringConstant("Hello, World!", "");

  this->gen_call(prints, {helloWorldCharPtr});

  //  llvm::Value *helloWorldStr2 = llvmBuilder->CreateGlobalString("Hello, World2!");
  //  this->gen_call(prints, {helloWorldStr2});

  llvmBuilder->CreateRetVoid();

  // Verify the function for correctness
  llvm::verifyFunction(*mainFunction, &(llvm::outs()));
}

void LLVMCodegen::testHelloWorld() {
  assert(this->jitter);
  LOG(INFO) << "getRawAddress(\"helloworld\")";
  LOG(INFO) << this->jitter->getRawAddress("helloworld");

  LOG(INFO) << "test helloWorld";
  reinterpret_cast<void (*)()>(this->jitter->getRawAddress("helloworld"))();
}

void LLVMCodegen::runOptimizationPasses() {
  for (auto &F : *theLLVMModule) theLLVMFPM->run(F);
}

void *LLVMCodegen::getFunction(const std::string &name) { return this->jitter->getRawAddress(name); }
void *LLVMCodegen::getFunctionPrefixed(const std::string &name) {
  return this->jitter->getRawAddress(builder.getName() + "_" + name);
}

void LLVMCodegen::buildFunctionDictionary() {
  assert(this->is_jit_done);
  for (auto &fb : builder.functions) {
    LOG(INFO) << "Resolving address: " << fb.first;
    auto *address = getFunctionPrefixed(fb.first);
    auto name = fb.first;
    auto return_type = fb.second->returnValueType;
    auto args = fb.second->getArguments();
    available_jit_functions.emplace(name, new jit_function_t{name, address, return_type, args});
  }
}

void LLVMCodegen::jitCompileAndLoad() {
  LOG(INFO) << "[LLVMCodegen::jit()] IR- before passes: ";
  this->printIR();
  //  LOG(INFO) << "[LLVMCodegen::jit()] IR- after passes: ";
  //  runOptimizationPasses();
  //  this->printIR();
  LOG(INFO) << "[LLVMCodegen::jit()] Passes done";

  this->jitter = std::make_unique<LLVMJIT>();
  this->theLLVMModule->setDataLayout(jitter->getDataLayout());
  this->theLLVMModule->setTargetTriple(jitter->getTargetTriple().str());

  auto modName = getModule()->getName();
  LOG(INFO) << "Module name: " << modName.str();

  // llvm::orc::ThreadSafeContext NewTSCtx(std::make_unique<LLVMContext>());
  // auto TSM = llvm::orc::ThreadSafeModule(std::move(theLLVMModule), std::move(NewTSCtx));

  auto TSM = llvm::orc::ThreadSafeModule(std::move(theLLVMModule), std::move(theLLVMContext));

  this->jitter->addModule(std::move(TSM));
  this->jitter->dump();
  this->testHelloWorld();
  this->jitter->dump();

  this->is_jit_done = true;
  this->buildFunctionDictionary();
}

}  // namespace dcds
