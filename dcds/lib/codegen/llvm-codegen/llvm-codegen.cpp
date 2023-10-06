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

void LLVMCodegen::initializeLLVMModule(const std::string &name) {
  LOG(INFO) << "[LLVMCodegen] Initializing LLVM module: " << name;
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  theLLVMContext = std::make_unique<LLVMContext>();

  theLLVMModule = std::make_unique<Module>(name, *theLLVMContext);
  llvmBuilder = std::make_unique<IRBuilder<>>(*theLLVMContext);
}

LLVMCodegen::LLVMCodegen(Builder &builder) : CodegenV2(builder), LLVMCodegenContext(builder.getName()) {
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

  //  // first we need a constructor or meta-function! which is gonna do the storage allocations and all.
  //
  //  LOG(INFO) << "[LLVMCodegen::build()] BuildFunctions";
  //  this->buildFunctions();
  //  LOG(INFO) << "[LLVMCodegen::build()] DONE";
}

void LLVMCodegen::buildFunctions() {
  LOG(INFO) << "[LLVMCodegen] buildFunctions: # of functions: " << builder.functions.size();

  // later: parallelize this loop.
  for (auto &fb : builder.functions) {
    this->buildOneFunction(fb.second);
  }
}

llvm::Type *toLLVMType(LLVMContext &context, dcds::valueType dcds_type) {
  switch (dcds_type) {
    case INTEGER:
    case RECORD_ID: {
      return llvm::Type::getInt64Ty(context);
    }
    case RECORD_PTR: {
      // FIXME: this should be size_t
      return llvm::Type::getInt8PtrTy(context);
    }
    case FLOAT: {
      return llvm::Type::getFloatTy(context);
    }
    case DATE: {
      return llvm::Type::getInt64Ty(context);
    }
    case CHAR:
    case VOID:
    default:
      assert(false && "valueTypeNotSupportedYet");
      break;
  }
}

llvm::Function *LLVMCodegen::genFunctionSignature(std::shared_ptr<FunctionBuilder> &fb) {
  std::vector<llvm::Type *> argTypes;
  llvm::Type *returnType;

  // NOTE: hardcode first argument: to be the mainStructPtr (pseudo this)
  argTypes.push_back(PointerType::get(dsContainerStructType, 0));

  for (const auto &arg : fb->function_arguments) {
    switch (arg.second) {
      case INTEGER:
      case RECORD_PTR: {
        argTypes.push_back(llvm::Type::getInt64PtrTy(getLLVMContext()));
        break;
      }
      case FLOAT:
      case DATE:
      case RECORD_ID:
      case CHAR:
      case VOID:
      default:
        assert(false && "valueTypeNotSupportedYet");
        break;
    }
  }

  switch (fb->returnValueType) {
    case INTEGER:
    case RECORD_PTR: {
      returnType = llvm::Type::getInt64Ty(getLLVMContext());
      break;
    }
    case VOID: {
      returnType = llvm::Type::getVoidTy(getLLVMContext());
      break;
    }
    case FLOAT:
    case DATE:
    case RECORD_ID:
    case CHAR:
    default:
      assert(false && "returnValueTypeNotSupportedYet");
      break;
  }

  auto fn_type = llvm::FunctionType::get(returnType, argTypes, false);
  auto fn =
      llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, fb->_name, theLLVMModule.get());

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
    auto var_type = std::get<0>(v.second);
    auto init_value = std::get<1>(v.second);

    // FIXME: if has a init value, then also assign that here!

    switch (var_type) {
      case INTEGER: {
        auto vr = allocaBuilder.CreateAlloca(llvm::Type::getInt64Ty(getLLVMContext()), nullptr, var_name);
        variableCodeMap.emplace(var_name, vr);
        break;
      }
      case RECORD_PTR: {
        auto vr = allocaBuilder.CreateAlloca(llvm::Type::getInt8PtrTy(getLLVMContext()), nullptr, var_name);
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
      case DATE:
      case RECORD_ID:
      case CHAR:

      default:
        assert(false && "[allocateTemporaryVariables] valueTypeNotSupportedYet");
        break;
    }
  }

  return variableCodeMap;
}

void LLVMCodegen::buildStatement(std::shared_ptr<StatementBuilder> &sb, llvm::BasicBlock *basicBlock) {
  LOG(INFO) << "[LLVMCodegen] buildStatement";

  // READ, UPDATE, YIELD, TEMP_VAR_ADD, CONDITIONAL_STATEMENT, CALL
  if (sb->stType == dcds::statementType::READ) {
    // actionVar -> txnVariable to read from
    // referenceVar -> destination.

    if (!builder.hasAttribute(sb->actionVarName)) {
      assert(false && "read attribute does not exists");
    }

    auto readFromAttribute = builder.getAttribute(sb->actionVarName);

  } else if (sb->stType == dcds::statementType::UPDATE) {
  }
}

// What about CC? nothing about CC yet, essentially,
// it should be injected in starting and ending of each function.
// We would need the read/write set, that should be easy though.

// For latching, each statementBuild will take care of that, actually.

void LLVMCodegen::createDsContainerStruct() {
  std::string ds_struct_name = "struct_" + builder.getName() + "_t";
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
  structValue = getBuilder()->CreateInsertValue(structValue, mainRecord, 2);
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
        fieldValue = ConstantInt::get(getLLVMContext(), APInt(64, value));
        break;
      }
      case RECORD_PTR: {
        fieldValue = llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(getLLVMContext()));
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
      case DATE:
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

void LLVMCodegen::buildConstructor() {
  // FIXME: get namespace prefix from context? buildContext or runtimeContext?
  //  maybe take it from runtime context to enable cross-DS or intra-DS txns.
  std::string namespace_prefix = "default_namespace";
  std::string table_name = namespace_prefix + "_" + builder.getName();

  auto namespaceLlvmConstant = this->createStringConstant(namespace_prefix, "txn_namespace_prefix");
  auto tableNameLlvmConstant = this->createStringConstant(table_name, "ds_table_name");
  auto fn_initTables = this->buildInitTablesFn(tableNameLlvmConstant);

  auto returnType = PointerType::getUnqual(dsContainerStructType);
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

  Value *returnStructContainer =
      this->initializeDsContainerStruct(fn_res_getTxnManger, tablePtrValue, fn_res_insertMainRecord);
  this->createPrintString("returning now");

  llvmBuilder->CreateRet(returnStructContainer);

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

void LLVMCodegen::buildOneFunction(std::shared_ptr<FunctionBuilder> &fb) {
  LOG(INFO) << "[LLVMCodegen] buildOneFunction: " << fb->_name;

  // 1- generate function signature
  auto fn = this->genFunctionSignature(fb);
  userFunctions.push_back(fn);

  // 2- set insertion point at the beginning of the function.
  auto fn_basic_block = llvm::BasicBlock::Create(getLLVMContext(), "entry", fn);
  llvmBuilder->SetInsertPoint(fn_basic_block);

  // 2- allocate temporary variables.
  LOG(INFO) << "codegen- temporary variables";
  auto variableCodeMap = this->allocateTemporaryVariables(fb, fn_basic_block);

  // Optimizations:
  //    easy: if an attribute is only accessed in a single function across DS, then you dont need CC either on that one.
  //    hard: if the group-sequence of attribute is common across all functions,
  //    then encapsulate them as a single CC-variable.

  // rw_set = fb->getReadWriteSet();
  // CcBuilder::injectCC(rw_set)
  // CcBuilder::injectTxnBegin

  // 3- codegen all statements
  LOG(INFO) << "codegen-statements";
  for (auto &s : fb->statements) {
    // check if statement does not return internally else we call endTxn inside.
    this->buildStatement(s, fn_basic_block);
  }

  // CcBuilder::injectTxnEnd

  // 4- Add return statement.
  LOG(INFO) << "codegen-return";
  if (fb->returnValueType == dcds::valueType::VOID) {
    llvmBuilder->CreateRetVoid();
  }

  // later: mark this function as done?
}

void LLVMCodegen::runOptimizationPasses() {
  for (auto &F : *theLLVMModule) theLLVMFPM->run(F);
}

void *LLVMCodegen::getFunction(const std::string &name) { return this->jitter->getRawAddress(name); }

void LLVMCodegen::jitCompileAndLoad() {
  LOG(INFO) << "[LLVMCodegen::jit()] IR- before passes: ";
  this->printIR();
  LOG(INFO) << "[LLVMCodegen::jit()] IR- after passes: ";
  runOptimizationPasses();
  this->printIR();
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
}

}  // namespace dcds
