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

#include "dcds/codegen/llvm-codegen/llvm-context.hpp"

#include <dlfcn.h>

#include <utility>
#include <vector>

#include "dcds/codegen/llvm-codegen/functions.hpp"

using namespace llvm;

const char *LLVMCodegenContext::getName() const { return (new std::string{getModule()->getName().str()})->c_str(); }

LLVMCodegenContext::LLVMCodegenContext(std::string moduleName) : moduleName(std::move(moduleName)) {}

void LLVMCodegenContext::registerAllFunctions() {
  LLVMContext &ctx = getModule()->getContext();
  Module *TheModule = getModule();
  assert(TheModule != nullptr);

  // Types
  [[maybe_unused]] Type *int1_bool_type = Type::getInt1Ty(ctx);
  [[maybe_unused]] Type *int8_type = Type::getInt8Ty(ctx);
  [[maybe_unused]] Type *int16_type = Type::getInt16Ty(ctx);
  [[maybe_unused]] Type *int32_type = Type::getInt32Ty(ctx);
  [[maybe_unused]] Type *int64_type = Type::getInt64Ty(ctx);
  // not 100% portable, but we only run on 64bit architectures
  [[maybe_unused]] Type *uintptr_type = Type::getInt64Ty(ctx);
  [[maybe_unused]] Type *void_type = Type::getVoidTy(ctx);
  [[maybe_unused]] Type *double_type = Type::getDoubleTy(ctx);

  [[maybe_unused]] PointerType *void_ptr_type = PointerType::get(int8_type, 0);
  [[maybe_unused]] PointerType *char_ptr_type = PointerType::get(int8_type, 0);
  [[maybe_unused]] PointerType *int32_ptr_type = PointerType::get(int32_type, 0);

  size_t i = 0;
  LOG(INFO) << i++;
  // int printc(char *X);
  registerFunction("printc", int32_type, {char_ptr_type});
  // int prints(char *X);
  registerFunction("prints", int32_type, {char_ptr_type});

  // void printPtr(void* X);
  registerFunction("printPtr", void_type, {void_ptr_type});

  // void printUInt64(uint64_t* X);
  registerFunction("printUInt64", void_type, {int64_type});

  //  void* createTablesInternal(char* table_name, const dcds::valueType attributeTypes[], char* attributeNames[],
  //                             int num_attributes)

  registerFunction("createTablesInternal", void_ptr_type, {char_ptr_type, int32_ptr_type, char_ptr_type, int32_type});

  registerFunction("c1", void_ptr_type, {char_ptr_type});
  registerFunction("c2", void_ptr_type, {int32_type});
  registerFunction("c3", void_ptr_type, {int32_ptr_type});
  registerFunction("c4", void_ptr_type, {char_ptr_type});

  //  LOG(INFO) << i++;

  //
  //  LOG(INFO) << i++;
  //  // void *getTableRegistry();
  //  assert(int8_type);
  //  registerFunction("getTableRegistry", void_ptr_type);
  //
  //  LOG(INFO) << i++;
  //  // void* getTable(const char* table_name);
  //  registerFunction("getTable", void_ptr_type, {char_ptr_type});
  //
  //  LOG(INFO) << i++;
  //  // uint doesTableExists(const char* table_name);
  //  registerFunction("doesTableExists", int1_bool_type, {char_ptr_type});
  //
  //
  //  // TO TEST IF IT CAN BE AUTO-REGISTERED.
  ////  // void *createTablesInternal(const char *table_name, const dcds::valueType attributeTypes[],
  ////  //                                      char *attributeNames[], int num_attributes)
  ////  registerFunction("createTablesInternal", void_ptr_type, {char_ptr_type});
  //
  //
  //  LOG(INFO) << i++;
  //  //  void *getTxnManager(const char* txn_namespace = "default_namespace");
  //  registerFunction("getTxnManager", void_ptr_type, {char_ptr_type});
  //
  //  LOG(INFO) << i++;
  //  //  extern "C" void *beginTxn(void *txnManager);
  //  registerFunction("beginTxn", void_ptr_type, {void_ptr_type});
  //
  //  LOG(INFO) << i++;
  //  //  extern "C" bool commitTxn(void *txnManager, void* txnPtr);
  //  registerFunction("commitTxn", int1_bool_type, {void_ptr_type, void_ptr_type});
  //
  //  LOG(INFO) << i++;
  //  //  extern "C" uintptr_t insertMainRecord(void* table, void* txn, void* data);
  //  registerFunction("insertMainRecord", uintptr_type, {void_ptr_type, void_ptr_type, void_ptr_type});
  //  LOG(INFO) << i++;
}

void LLVMCodegenContext::createPrintString(const std::string &str) {
  auto stringPtr = this->createStringConstant(str, "");
  this->gen_call(prints, {stringPtr});
}

// void createBooleanLockVariable(){
//     auto* lock = new llvm::GlobalVariable(*(theLLVMModule),
//                                               Type::getInt1Ty(getLLVMContext()),
//                                               false,
//                                               GlobalValue::InternalLinkage,
//                                               ConstantInt::getFalse(getLLVMContext()),
//                                               builder.getName() + "_constructor_internal_lock");
// }

llvm::Value *LLVMCodegenContext::createStringConstant(const std::string &value, const std::string &var_name) const {
  // FIXME: Currently a hack, no idea if it is good thing to do like this or do it the proper way.
  llvm::Value *str = getBuilder()->CreateGlobalString(value);
  llvm::Value *charPtrCast = getBuilder()->CreateBitCast(str, llvm::Type::getInt8PtrTy(getLLVMContext()));
  return charPtrCast;

  //  Constant *strConstant = ConstantDataArray::getString(getLLVMContext(), value, true);
  //  //  LOG(INFO) << "[createStringConstant] 3";
  //  //  AllocaInst *strAlloca = irBuilder->CreateAlloca(type, nullptr, var_name);
  //  //  LOG(INFO) << "[createStringConstant] 4";
  //  //  irBuilder->CreateStore(strConstant, strAlloca);
  //  //  LOG(INFO) << "[createStringConstant] 5";
  //
  //  //  llvm::Value* stringPtr = irBuilder->CreateGlobalStringPtr(value.c_str(), var_name);
  //  // return stringPtr;
  //
  //  auto *strGlobal =
  //      new llvm::GlobalVariable(strConstant->getType(), true, llvm::GlobalValue::PrivateLinkage, strConstant,
  //      var_name);
  //  llvm::Value *strPtr = getBuilder()->CreateBitCast(strGlobal, llvm::Type::getInt8PtrTy(getLLVMContext()));
  //  return strPtr;

  // can be of type llvm::Value* too.
  //  auto allocaString = irBuilder->CreateAlloca(llvm::Type::getInt8Ty(getLLVMContext()),
  //                                                      llvm::ConstantInt::get(llvm::Type::getInt64Ty(getLLVMContext()),
  //                                                                             value.length() + 1),
  //                                                      var_name);
  //
  //  // Store the string in the allocated memory
  //  irBuilder->CreateMemCpy(allocaString,
  //                          llvm::MaybeAlign{}
  //                          llvm::ConstantDataArray::getString(getLLVMContext(), value.c_str()),
  //                          value.length() + 1,
  //                          1);

  //  return allocaString;
}

llvm::StructType *LLVMCodegenContext::getVaListStructType() {
  if (variadicArgs_vaList_struct_t == nullptr) {
    static_assert(
        sizeof(std::va_list) == 24,
        "variadic list (va_list) is platform dependent, and mismatches current implementation of x86_64 only.");
    // NOTE: the other one should be merely a ptr in a struct, but not sure until experiment on such a machine.
    //  reference: https://llvm.org/docs/LangRef.html#int-varargs

    auto ptrType = IntegerType::getInt8PtrTy(getLLVMContext());
    auto i32Type = IntegerType::getInt32Ty(getLLVMContext());
    // auto uintPtrType = IntegerType::getInt64Ty(getLLVMContext());

    variadicArgs_vaList_struct_t =
        StructType::create(getLLVMContext(), {i32Type, i32Type, ptrType, ptrType}, "struct.va_list", false);
  }

  return variadicArgs_vaList_struct_t;
}

llvm::Value *LLVMCodegenContext::createVaListStart() {
  llvm::AllocaInst *structAlloca =
      getBuilder()->CreateAlloca(this->getVaListStructType(), ConstantInt::get(getLLVMContext(), APInt(32, 1)));
  auto *ValuePtr = getBuilder()->CreateBitOrPointerCast(structAlloca, IntegerType::getInt8PtrTy(getLLVMContext()));
  llvm::Function *vaStartFunc = llvm::Intrinsic::getDeclaration(getModule(), llvm::Intrinsic::vastart);
  getBuilder()->CreateCall(vaStartFunc, {ValuePtr});
  return ValuePtr;
}

void LLVMCodegenContext::createVaListEnd(llvm::Value *va_list_ptr) {
  llvm::Function *vaEndFunc = llvm::Intrinsic::getDeclaration(getModule(), llvm::Intrinsic::vaend);
  getBuilder()->CreateCall(vaEndFunc, {va_list_ptr});
}

llvm::Value *LLVMCodegenContext::getVAArg(llvm::Value *va_list_ptr, llvm::Type *type) {
  return getBuilder()->CreateVAArg(va_list_ptr, type);
}

std::vector<llvm::Value *> LLVMCodegenContext::getVAArgs(llvm::Value *va_list_ptr, std::vector<llvm::Type *> types) {
  std::vector<llvm::Value *> ret;
  const auto builder = getBuilder();
  for (const auto &t : types) {
    ret.push_back(builder->CreateVAArg(va_list_ptr, t));
  }
  return ret;
}

Value *LLVMCodegenContext::CastPtrToLlvmPtr(PointerType *type, const void *ptr) const {
  Constant *const_int = createInt64((uint64_t)ptr);
  Value *llvmPtr = ConstantExpr::getIntToPtr(const_int, type);
  return llvmPtr;
}

void LLVMCodegenContext::llvmVerifyFunction(llvm::Function *f) {
  // Verify the function and capture errors in an error stream
  assert(f);
  llvm::raw_ostream &errorStream = llvm::errs();  // Standard error stream
  bool hasErrors = llvm::verifyFunction(*f, &errorStream);
  if (hasErrors) {
    llvm::outs() << "Function has issues: " << f->getName() << "\n";
  }
}

ConstantInt *LLVMCodegenContext::createInt8(char val) const {
  return ConstantInt::get(getLLVMContext(), APInt(8, val));
}

ConstantInt *LLVMCodegenContext::createInt32(int val) const {
  return ConstantInt::get(getLLVMContext(), APInt(32, val));
}

ConstantInt *LLVMCodegenContext::createInt64(int val) const {
  return ConstantInt::get(getLLVMContext(), APInt(64, val));
}

ConstantInt *LLVMCodegenContext::createInt64(unsigned int val) const {
  return ConstantInt::get(getLLVMContext(), APInt(64, val));
}

ConstantInt *LLVMCodegenContext::createInt64(size_t val) const {
  return ConstantInt::get(getLLVMContext(), APInt(64, val));
}

ConstantInt *LLVMCodegenContext::createInt64(int64_t val) const {
  return ConstantInt::get(getLLVMContext(), APInt(64, val));
}

ConstantInt *LLVMCodegenContext::createUintptr(uintptr_t val) const {
  return ConstantInt::get(getLLVMContext(), APInt(64, val));
}

ConstantInt *LLVMCodegenContext::createSizeT(size_t val) const { return ConstantInt::get(createSizeType(), val); }

IntegerType *LLVMCodegenContext::createSizeType() const {
  return Type::getIntNTy(getLLVMContext(), sizeof(size_t) * 8);
}

ConstantInt *LLVMCodegenContext::createTrue() const { return ConstantInt::get(getLLVMContext(), APInt(1, 1)); }

ConstantInt *LLVMCodegenContext::createFalse() const { return ConstantInt::get(getLLVMContext(), APInt(1, 0)); }

size_t LLVMCodegenContext::getSizeOf(llvm::Type *type) const {
  return getModule()->getDataLayout().getTypeAllocSize(type);
}

size_t LLVMCodegenContext::getSizeOf(llvm::Value *val) const { return getSizeOf(val->getType()); }

StructType *LLVMCodegenContext::CreateCustomStruct(LLVMContext &ctx, const std::vector<Type *> &innerTypes) {
  return llvm::StructType::get(ctx, innerTypes);
}

StructType *LLVMCodegenContext::CreateCustomStruct(const std::vector<Type *> &innerTypes) const {
  return CreateCustomStruct(getLLVMContext(), innerTypes);
}

Value *LLVMCodegenContext::getStructElem(Value *mem_struct, int elemNo) const {
  std::vector<Value *> idxList = std::vector<Value *>();
  idxList.push_back(createInt32(0));
  idxList.push_back(createInt32(elemNo));
  // Shift in struct ptr
  Value *mem_struct_shifted =
      getBuilder()->CreateGEP(mem_struct->getType()->getNonOpaquePointerElementType(), mem_struct, idxList);
  Value *val_struct_shifted =
      getBuilder()->CreateLoad(mem_struct_shifted->getType()->getPointerElementType(), mem_struct_shifted);
  return val_struct_shifted;
}

Value *LLVMCodegenContext::getStructElemMem(Value *mem_struct, int elemNo) const {
  std::vector<Value *> idxList = std::vector<Value *>();
  idxList.push_back(createInt32(0));
  idxList.push_back(createInt32(elemNo));
  // Shift in struct ptr
  Value *mem_struct_shifted =
      getBuilder()->CreateGEP(mem_struct->getType()->getNonOpaquePointerElementType(), mem_struct, idxList);
  return mem_struct_shifted;
}

Value *LLVMCodegenContext::getStructElem(AllocaInst *mem_struct, int elemNo) const {
  std::vector<Value *> idxList = std::vector<Value *>();
  idxList.push_back(createInt32(0));
  idxList.push_back(createInt32(elemNo));
  // Shift in struct ptr
  Value *mem_struct_shifted =
      getBuilder()->CreateGEP(mem_struct->getType()->getNonOpaquePointerElementType(), mem_struct, idxList);
  Value *val_struct_shifted =
      getBuilder()->CreateLoad(mem_struct_shifted->getType()->getPointerElementType(), mem_struct_shifted);
  return val_struct_shifted;
}

void LLVMCodegenContext::updateStructElem(Value *toStore, Value *mem_struct, int elemNo) const {
  std::vector<Value *> idxList = std::vector<Value *>();
  idxList.push_back(createInt32(0));
  idxList.push_back(createInt32(elemNo));
  // Shift in struct ptr
  Value *structPtr =
      getBuilder()->CreateGEP(mem_struct->getType()->getNonOpaquePointerElementType(), mem_struct, idxList);
  getBuilder()->CreateStore(toStore, structPtr);
}

AllocaInst *LLVMCodegenContext::createAlloca(BasicBlock *InsertAtBB, const std::string &VarName, Type *varType) {
  IRBuilder<> TmpBuilder(InsertAtBB, InsertAtBB->begin());
  return TmpBuilder.CreateAlloca(varType, nullptr, VarName);
}

void LLVMCodegenContext::CreateIfElseBlocks(Function *fn, const std::string &if_label, const std::string &else_label,
                                            BasicBlock **if_block, BasicBlock **else_block,
                                            BasicBlock *insert_before) const {
  LLVMContext &ctx = getLLVMContext();
  *if_block = BasicBlock::Create(ctx, if_label, fn, insert_before);
  *else_block = BasicBlock::Create(ctx, else_label, fn, insert_before);
}

BasicBlock *LLVMCodegenContext::CreateIfBlock(Function *fn, const std::string &if_label,
                                              BasicBlock *insert_before) const {
  return BasicBlock::Create(getLLVMContext(), if_label, fn, insert_before);
}

void LLVMCodegenContext::CreateIfBlock(Function *fn, const std::string &if_label, BasicBlock **if_block,
                                       BasicBlock *insert_before) const {
  *if_block = CreateIfBlock(fn, if_label, insert_before);
}

Value *LLVMCodegenContext::getArrayElem(AllocaInst *mem_ptr, Value *offset) const {
  Value *val_ptr = getBuilder()->CreateLoad(mem_ptr->getType()->getPointerElementType(), mem_ptr, "mem_ptr");
  Value *shiftedPtr =
      getBuilder()->CreateInBoundsGEP(val_ptr->getType()->getNonOpaquePointerElementType(), val_ptr, offset);
  Value *val_shifted =
      getBuilder()->CreateLoad(shiftedPtr->getType()->getPointerElementType(), shiftedPtr, "val_shifted");
  return val_shifted;
}

Value *LLVMCodegenContext::getArrayElem(Value *val_ptr, Value *offset) const {
  Value *shiftedPtr =
      getBuilder()->CreateInBoundsGEP(val_ptr->getType()->getNonOpaquePointerElementType(), val_ptr, offset);
  Value *val_shifted =
      getBuilder()->CreateLoad(shiftedPtr->getType()->getPointerElementType(), shiftedPtr, "val_shifted");
  return val_shifted;
}

Value *LLVMCodegenContext::getArrayElemMem(Value *val_ptr, Value *offset) const {
  Value *shiftedPtr =
      getBuilder()->CreateInBoundsGEP(val_ptr->getType()->getNonOpaquePointerElementType(), val_ptr, offset);
  return shiftedPtr;
}

PointerType *LLVMCodegenContext::getPointerType(Type *type) { return PointerType::get(type, 0); }

void LLVMCodegenContext::registerFunction(const char *funcName, Function *func) { availableFunctions[funcName] = func; }

void LLVMCodegenContext::registerFunction(const std::string &function_name, llvm::Type *returnType,
                                          const std::vector<llvm::Type *> &args, bool always_inline) {
  //  auto fn_type = llvm::FunctionType::get(returnType, argTypes, false);
  //  auto fn =llvm::Function::Create(fn_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage, fb->_name,
  //  theLLVMModule.get()); fn->addFnAttr(llvm::Attribute::AlwaysInline);
  LOG(INFO) << "registerFunction: ver2: " << function_name;

  assert(returnType);

  FunctionType *FT = nullptr;
  if (args.empty()) {
    FunctionType::get(returnType, false);
  } else {
    FT = FunctionType::get(returnType, args, false);
  }
  Function *FN = Function::Create(FT, Function::ExternalLinkage, function_name, getModule());

  if (always_inline) FN->addFnAttr(llvm::Attribute::AlwaysInline);

  registerFunction(function_name.c_str(), FN);
}

void LLVMCodegenContext::registerFunction(const std::string &func, std::initializer_list<llvm::Value *> args,
                                          llvm::Type *ret) {
  LOG(INFO) << "registerFunction: ver1: " << func;

  std::vector<llvm::Type *> v;
  v.reserve(args.size());
  for (const auto &arg : args) {
    v.emplace_back(arg->getType());
    // arg->getType()->dump();
  }
  auto FT = llvm::FunctionType::get(ret, v, false);

  llvm::Function *f = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, func, getModule());

  registerFunction((new std::string{func})->c_str(), f);
}

llvm::Function *LLVMCodegenContext::getFunction(const std::string &funcName) const {
  std::map<std::string, Function *>::const_iterator it;
  it = availableFunctions.find(funcName);
  if (it == availableFunctions.end()) {
    throw std::runtime_error(std::string("Unknown function name: ") + funcName);
  }
  return it->second;
}

llvm::Value *LLVMCodegenContext::gen_call(llvm::Function *f, std::initializer_list<llvm::Value *> args) {
  return getBuilder()->CreateCall(f, args);
}

llvm::Value *LLVMCodegenContext::gen_call(llvm::Function *f, std::vector<llvm::Value *> args) {
  return getBuilder()->CreateCall(f, args);
}

llvm::Value *LLVMCodegenContext::gen_call(const std::string &func, std::initializer_list<llvm::Value *> args,
                                          llvm::Type *ret) {
  LOG(INFO) << "[LLVMCodegenContext][gen_call][2] "
            << "function_name: " << func;
  llvm::Function *f;
  try {
    f = getFunction(func);
    assert(f);
    // LOG(INFO) << "Found function: " << func;
    for (const auto &arg : args) {
      // LOG(INFO) << "argDump[1]";
      // arg->getType()->dump();
    }

    assert(!ret || ret == f->getReturnType());
  } catch (std::runtime_error &) {
    LOG(INFO) << "[LLVMCodegenContext][gen_call] "
              << "registering a new function: " << func;
    assert(ret);
    std::vector<llvm::Type *> v;
    v.reserve(args.size());
    for (const auto &arg : args) {
      v.emplace_back(arg->getType());
      //      LOG(INFO) << "argDump[2]";
      //      arg->getType()->dump();
    }
    auto FT = llvm::FunctionType::get(ret, v, false);

    f = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, func, getModule());

    registerFunction((new std::string{func})->c_str(), f);
  }

  return gen_call(f, args);
}

llvm::Value *LLVMCodegenContext::createStringArray(const std::vector<std::string> &string_array,
                                                   const std::string &name_prefix) const {
  std::vector<llvm::GlobalVariable *> strGlobals;

  for (const auto &s : string_array) {
    //    llvm::Constant* strConstant = llvm::ConstantDataArray::getString(getLLVMContext(),
    //    string_array.at(i).c_str()); strGlobals[i] = new llvm::GlobalVariable(
    //        strConstant->getType(),
    //        true,
    //        llvm::GlobalValue::PrivateLinkage,
    //        strConstant,
    //        (name_prefix + std::to_string(i)).c_str()
    //    );

    llvm::Constant *strConstant = llvm::ConstantDataArray::getString(getLLVMContext(), s);
    auto *strGlobal =
        new llvm::GlobalVariable(*getModule(), strConstant->getType(), true, llvm::GlobalValue::PrivateLinkage,
                                 strConstant, name_prefix + std::to_string(strGlobals.size()));
    strGlobals.push_back(strGlobal);
  }

  std::vector<llvm::Value *> stringPointers;
  for (llvm::GlobalVariable *strGlobal : strGlobals) {
    llvm::Value *strPtr = getBuilder()->CreateBitCast(strGlobal, llvm::Type::getInt8PtrTy(getLLVMContext()));
    stringPointers.push_back(strPtr);
  }

  // Convert the array of llvm::Value* to an array of llvm::Constant*
  std::vector<llvm::Constant *> stringConstants;
  for (llvm::Value *strPtr : stringPointers) {
    stringConstants.push_back(llvm::dyn_cast<llvm::Constant>(strPtr));
  }

  // Create a constant array of llvm::Constant* representing pointers to strings
  llvm::ArrayType *arrayType = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(getLLVMContext()), string_array.size());
  llvm::Constant *stringArray = llvm::ConstantArray::get(arrayType, stringConstants);
  // stringArray->getType()->dump();

  return stringArray;
}

std::string getFunctionName(void *f) {
  Dl_info info{};
#ifndef NDEBUG
  int ret =
#endif
      dladdr(f, &info);
  assert(ret && "Looking for function failed");
  assert(info.dli_saddr == (decltype(Dl_info::dli_saddr))f);
  assert(info.dli_sname);
  return info.dli_sname;
}