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

//
// Created by prathamesh on 11/7/23.
//

#ifndef DCDS_CODEGEN_HPP
#define DCDS_CODEGEN_HPP

#include <dlfcn.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <dcds/builder/attribute.hpp>
#include <dcds/builder/function-builder.hpp>
#include <dcds/builder/statement.hpp>
#include <dcds/codegen/DCDSJIT.hpp>
#include <dcds/codegen/utils.hpp>
#include <dcds/util/logging.hpp>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/EPCIndirectionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

namespace dcds {
using namespace llvm;

extern "C" {
void read_data_structure_attribute(void *, int64_t, void *, int64_t, void *);

void write_data_structure_attribute(void *, int64_t, void *, int64_t, void *);

void *begin_txn(void *);

void end_txn(void *, void *);
}

class Visitor {
 public:
  void initializeModule() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    /// Open a new context and module.
    theLLVMContext = std::make_unique<LLVMContext>();
    theLLVMModule = std::make_unique<Module>(dsName, *theLLVMContext);

    /// Create a new builder for the module.
    llvmBuilder = std::make_unique<IRBuilder<>>(*theLLVMContext);
  }

  void initializePassManager() {
    /// Create a new pass manager attached to it.
    theLLVMFPM = std::make_unique<legacy::FunctionPassManager>(theLLVMModule.get());

    /// Promote allocas to registers.
    theLLVMFPM->add(createPromoteMemoryToRegisterPass());
    /// Do simple "peephole" optimizations and bit-twiddling optzns.
    theLLVMFPM->add(createInstructionCombiningPass());
    /// Reassociate expressions.
    theLLVMFPM->add(createReassociatePass());
    /// Eliminate Common SubExpressions.
    theLLVMFPM->add(createGVNPass());
    /// Simplify the control flow graph (deleting unreachable blocks, etc).
    theLLVMFPM->add(createCFGSimplificationPass());

    /// TODO: Eliminate unnecessary code generation at its source (wherever it makes sense to do so).
    theLLVMFPM->add(createDeadCodeEliminationPass());

    theLLVMFPM->doInitialization();
  }

  void runPasses() {
    /// TODO: Determine if we really require this function or if all of it is/(can be) automatically done by the JIT.

    /// Run the optimizations over all functions in the module being added to
    /// the JIT.
    for (auto &F : *theLLVMModule) theLLVMFPM->run(F);

    theLLVMModule->print(llvm::outs(), nullptr);
  }

  void saveModuleToFile(std::string fileName) {
    std::error_code errorCode;
    llvm::raw_fd_ostream outLL(fileName, errorCode);
    theLLVMModule->print(outLL, nullptr);
  }

  Visitor(auto &dsName_, auto &functions_, auto &attributes_, auto &tempVarsInfo_, auto &funcArgsInfo_,
          auto &orderedStatements_, auto &statementToConditionMap_, auto &tempVarsOpResName_, auto externalCallInfo_)
      : dsName(dsName_),
        functions(functions_),
        attributes(attributes_),
        tempVarsInfo(tempVarsInfo_),
        funcArgsInfo(funcArgsInfo_),
        orderedStatements(orderedStatements_),
        statementToConditionMap(statementToConditionMap_),
        tempVarsOpResName(tempVarsOpResName_),
        externalCallInfo(externalCallInfo_) {
    initializeModule();
    initializePassManager();
  }

  auto addReturnStatement(llvm::Value *returnVal) { return llvmBuilder->CreateRet(returnVal); }

  auto addTwoVars(dcds::valueType ty1, llvm::Value *val1, dcds::valueType ty2, llvm::Value *val2) {
    return llvmBuilder->CreateAdd(val1, val2);
  }

  void codegenStatement(std::shared_ptr<StatementBuilder> statement, llvm::BasicBlock *block) {
    llvm::Value *readVar, *writeVar, *tempVar1, *tempVar2, *tempVar3;
    switch (statement->stType) {
      case dcds::statementType::READ: {
        if (temporaryVariableIRMap[statement->refVarName]) {
          readVar = temporaryVariableIRMap[statement->refVarName];

          if (std::get<0>(tempVarsInfo[statement->refVarName]) == dcds::valueType::INTEGER) {
            readVar = llvmBuilder->CreateIntToPtr(readVar, llvm::Type::getInt64PtrTy(*theLLVMContext));
            readVar = llvmBuilder->CreateBitCast(readVar, llvm::Type::getInt8PtrTy(*theLLVMContext));
          } else if (std::get<0>(tempVarsInfo[statement->refVarName]) == dcds::valueType::RECORD_PTR) {
            readVar = llvmBuilder->CreateBitCast(readVar, llvm::Type::getInt8PtrTy(*theLLVMContext));
          }
        } else  /// Function argument is involved.
        {
          readVar = currentFunction->getArg(
              1 + dcds::llvmutil::findInVector(
                      funcArgsInfo[functions[static_cast<std::string>(currentFunction->getName())]],
                      statement->refVarName));
          readVar = llvmBuilder->CreateBitCast(readVar, llvm::Type::getInt8PtrTy(*theLLVMContext));
        }

        if (attributes.find(statement->sourceAttrName) != attributes.end()) {
          /// Read data structure attribute.
          auto argIndex = dcds::llvmutil::findInMap(attributes, statement->sourceAttrName);
          auto argIndexValue = llvm::ConstantInt::get(*theLLVMContext, llvm::APInt(64, argIndex));

          llvmBuilder->CreateCall(
              readFunction, {currentFunction->getArg(0), argIndexValue, readVar,
                             dcds::llvmutil::attrTypeMatching(attributes[statement->sourceAttrName], theLLVMContext)});
        } else {
          /// TODO: Throw appropriate error code from C++ standard.
          std::cout << "Error: Can only read from data structure attributes.\n";
        }
        break;
      }
      case dcds::statementType::UPDATE: {
        if (temporaryVariableIRMap[statement->refVarName]) {
          writeVar = temporaryVariableIRMap[statement->refVarName];

          if (std::get<0>(tempVarsInfo[statement->refVarName]) == dcds::valueType::INTEGER) {
            writeVar = llvmBuilder->CreateIntToPtr(writeVar, llvm::Type::getInt64PtrTy(*theLLVMContext));
            writeVar = llvmBuilder->CreateBitCast(writeVar, llvm::Type::getInt8PtrTy(*theLLVMContext));
          } else if (std::get<0>(tempVarsInfo[statement->refVarName]) == dcds::valueType::RECORD_PTR) {
            writeVar = llvmBuilder->CreateBitCast(writeVar, llvm::Type::getInt8PtrTy(*theLLVMContext));
          }
        } else {  /// Function argument is involved.
          writeVar = currentFunction->getArg(
              1 + dcds::llvmutil::findInVector(
                      funcArgsInfo[functions[static_cast<std::string>(currentFunction->getName())]],
                      statement->refVarName));
          writeVar = llvmBuilder->CreateBitCast(writeVar, llvm::Type::getInt8PtrTy(*theLLVMContext));
        }

        if (attributes.find(statement->sourceAttrName) != attributes.end()) {
          /// Modify data structure attribute.
          auto argIndex = dcds::llvmutil::findInMap(attributes, statement->sourceAttrName);
          auto argIndexValue = llvm::ConstantInt::get(*theLLVMContext, llvm::APInt(64, argIndex));

          llvmBuilder->CreateCall(
              writeFunction, {currentFunction->getArg(0), argIndexValue, writeVar,
                              dcds::llvmutil::attrTypeMatching(attributes[statement->sourceAttrName], theLLVMContext)});
        } else {
          /// TODO: Throw appropriate error code from C++ standard.
          std::cout << "Error: Can only write to data structure attributes.\n";
        }
        break;
      }
      case dcds::statementType::TEMP_VAR_ADD: {
        llvm::Value *getVal1, *getVal2;

        if (temporaryVariableIRMap[statement->sourceAttrName]) {
          dcds::Attribute tempAttr1(statement->sourceAttrName, dcds::valueType::INTEGER,
                                    std::get<1>(tempVarsInfo[statement->sourceAttrName]));

          getVal1 = tempAttr1.getAttributeCodegen(llvmBuilder, theLLVMContext,
                                                  temporaryVariableIRMap[statement->sourceAttrName]);
        } else {
          dcds::Attribute tempAttr1(statement->sourceAttrName, dcds::valueType::INTEGER, -1);

          tempVar1 = currentFunction->getArg(
              1 + dcds::llvmutil::findInVector(
                      funcArgsInfo[functions[static_cast<std::string>(currentFunction->getName())]],
                      statement->sourceAttrName));
          getVal1 = tempAttr1.getAttributeCodegen(llvmBuilder, theLLVMContext, tempVar1);
        }

        if (temporaryVariableIRMap[statement->refVarName]) {
          dcds::Attribute tempAttr2(statement->refVarName, dcds::valueType::INTEGER,
                                    std::get<1>(tempVarsInfo[statement->refVarName]));

          getVal2 =
              tempAttr2.getAttributeCodegen(llvmBuilder, theLLVMContext, temporaryVariableIRMap[statement->refVarName]);
        } else {
          dcds::Attribute tempAttr2(statement->refVarName, dcds::valueType::INTEGER, -1);

          tempVar2 = currentFunction->getArg(
              1 + dcds::llvmutil::findInVector(
                      funcArgsInfo[functions[static_cast<std::string>(currentFunction->getName())]],
                      statement->refVarName));
          getVal2 = tempAttr2.getAttributeCodegen(llvmBuilder, theLLVMContext, tempVar2);
        }

        dcds::Attribute tempAttr3(tempVarsOpResName[statement], dcds::valueType::INTEGER, -1);
        auto addRes = tempAttr3.addTwoVarsCodegen(llvmBuilder, getVal1, getVal2);

        if (temporaryVariableIRMap[tempVarsOpResName[statement]])
          tempVar3 = temporaryVariableIRMap[tempVarsOpResName[statement]];
        else
          tempVar3 = currentFunction->getArg(
              1 + dcds::llvmutil::findInVector(
                      funcArgsInfo[functions[static_cast<std::string>(currentFunction->getName())]],
                      tempVarsOpResName[statement]));

        tempAttr3.setAttributeCodegen(llvmBuilder, addRes,
                                      tempVar3);  /// Example/Reminder that Attr wrapper is redundant in such cases.
        break;
      }
      case dcds::statementType::CONDITIONAL_STATEMENT: {
        llvm::Value *condition = llvm::ConstantInt::get(
            *theLLVMContext, llvm::APInt(64, 1));  /// Dummy value to stop the compiler from complaining now.
        auto conditionStatement = statementToConditionMap[statement];
        if (temporaryVariableIRMap[conditionStatement->condition.conditionVariableAttr1Name])
          tempVar1 = temporaryVariableIRMap[conditionStatement->condition.conditionVariableAttr1Name];
        else
          tempVar1 = currentFunction->getArg(
              1 + dcds::llvmutil::findInVector(
                      funcArgsInfo[functions[static_cast<std::string>(currentFunction->getName())]],
                      conditionStatement->condition.conditionVariableAttr1Name));

        if (temporaryVariableIRMap[conditionStatement->condition.conditionVariableAttr2Name])
          tempVar2 = temporaryVariableIRMap[conditionStatement->condition.conditionVariableAttr2Name];
        else
          tempVar2 = currentFunction->getArg(
              1 + dcds::llvmutil::findInVector(
                      funcArgsInfo[functions[static_cast<std::string>(currentFunction->getName())]],
                      conditionStatement->condition.conditionVariableAttr2Name));

        if (conditionStatement->condition.pred == dcds::CmpIPredicate::neq) {
          if (tempVar1->getType()->isIntegerTy()) {
            condition = llvmBuilder->CreateICmpNE(tempVar1, tempVar2, "ifCond");
          } else {
            /// Assume pointers are to be compared.
            auto tempVar1Int = llvmBuilder->CreatePtrToInt(tempVar1, llvm::Type::getInt64Ty(*theLLVMContext));
            auto tempVar2Int = llvmBuilder->CreatePtrToInt(tempVar2, llvm::Type::getInt64Ty(*theLLVMContext));
            condition = llvmBuilder->CreateICmpNE(tempVar1Int, tempVar2Int, "ifCond");
          }
        }

        BasicBlock *thenBB;
        BasicBlock *elseBB;
        BasicBlock *mergeBB = BasicBlock::Create(*theLLVMContext, "ifcont", currentFunction);
        dcds::llvmutil::CreateIfElseBlocks(theLLVMContext, currentFunction, "then", "else", &thenBB, &elseBB, mergeBB);

        llvmBuilder->CreateCondBr(condition, thenBB, elseBB);

        /// Emit then block.
        llvmBuilder->SetInsertPoint(thenBB);
        for (auto singleStatement : conditionStatement->ifResStatements) codegenStatement(singleStatement, thenBB);
        llvmBuilder->CreateBr(mergeBB);

        /// Emit else block.
        llvmBuilder->SetInsertPoint(elseBB);
        for (auto singleStatement : conditionStatement->elseResStatements) codegenStatement(singleStatement, elseBB);
        llvmBuilder->CreateBr(mergeBB);

        llvmBuilder->SetInsertPoint(mergeBB);
        break;
      }
      case dcds::statementType::YIELD: {
        if (temporaryVariableIRMap.find(statement->refVarName) != temporaryVariableIRMap.end())
          tempVar1 = temporaryVariableIRMap[statement->refVarName];
        else
          tempVar1 = currentFunction->getArg(
              1 + dcds::llvmutil::findInVector(
                      funcArgsInfo[functions[static_cast<std::string>(currentFunction->getName())]],
                      statement->refVarName));

        if (std::get<0>(tempVarsInfo[statement->refVarName]) == dcds::valueType::INTEGER) {
          dcds::Attribute tempReturnAttr("tempReturnAttribute", dcds::valueType::INTEGER, 0);
          addReturnStatement(tempReturnAttr.getAttributeCodegen(llvmBuilder, theLLVMContext, tempVar1));
        } else if (std::get<0>(tempVarsInfo[statement->refVarName]) == dcds::valueType::RECORD_PTR) {
          dcds::Attribute tempReturnAttr("tempReturnAttribute", dcds::valueType::RECORD_PTR, nullptr);
          addReturnStatement(tempReturnAttr.getAttributeCodegen(llvmBuilder, theLLVMContext, tempVar1));
        }
        break;
      }
      case dcds::statementType::CALL: {
        if (temporaryVariableIRMap[externalCallInfo[statement->sourceAttrName][0]]) {
          tempVar1 = temporaryVariableIRMap[externalCallInfo[statement->sourceAttrName][0]];

          if (std::get<0>(tempVarsInfo[externalCallInfo[statement->sourceAttrName][0]]) == dcds::valueType::INTEGER) {
            tempVar1 = llvmBuilder->CreateIntToPtr(tempVar1, llvm::Type::getInt64PtrTy(*theLLVMContext));
            tempVar1 = llvmBuilder->CreateBitCast(tempVar1, llvm::Type::getInt8PtrTy(*theLLVMContext));
          } else if (std::get<0>(tempVarsInfo[externalCallInfo[statement->sourceAttrName][0]]) ==
                     dcds::valueType::RECORD_PTR) {
            tempVar1 = llvmBuilder->CreateBitCast(tempVar1, llvm::Type::getInt8PtrTy(*theLLVMContext));
          }
        } else {
          tempVar1 = currentFunction->getArg(
              1 + dcds::llvmutil::findInVector(
                      funcArgsInfo[functions[static_cast<std::string>(currentFunction->getName())]],
                      externalCallInfo[statement->sourceAttrName][0]));
          tempVar1 = llvmBuilder->CreateBitCast(tempVar1, llvm::Type::getInt8PtrTy(*theLLVMContext));
        }

        if (temporaryVariableIRMap[externalCallInfo[statement->sourceAttrName][1]]) {
          tempVar2 = temporaryVariableIRMap[externalCallInfo[statement->sourceAttrName][1]];

          if (std::get<0>(tempVarsInfo[externalCallInfo[statement->sourceAttrName][1]]) == dcds::valueType::INTEGER) {
            tempVar2 = llvmBuilder->CreateIntToPtr(tempVar2, llvm::Type::getInt64PtrTy(*theLLVMContext));
            tempVar2 = llvmBuilder->CreateBitCast(tempVar2, llvm::Type::getInt8PtrTy(*theLLVMContext));
          } else if (std::get<0>(tempVarsInfo[externalCallInfo[statement->sourceAttrName][1]]) ==
                     dcds::valueType::RECORD_PTR) {
            /// TODO: Bitcast temporary variables of RECORD_PTR type before mapping them with temporaryVariableIRMap.
            tempVar2 = llvmBuilder->CreateBitCast(tempVar2, llvm::Type::getInt8PtrTy(*theLLVMContext));
          }
        } else {
          tempVar2 = currentFunction->getArg(
              1 + dcds::llvmutil::findInVector(
                      funcArgsInfo[functions[static_cast<std::string>(currentFunction->getName())]],
                      externalCallInfo[statement->sourceAttrName][1]));
          tempVar2 = llvmBuilder->CreateBitCast(tempVar2, llvm::Type::getInt8PtrTy(*theLLVMContext));
        }

        llvmBuilder->CreateCall(externalFunctions[statement->sourceAttrName], {tempVar1, tempVar2});
        break;
      }
    }
  }

  void codegenUserFunctions() {
    uint32_t indexVar = 0;
    for (auto it = functions.begin(); it != functions.end(); ++it) {
      currentFunction = userFunctions[indexVar];
      indexVar++;

      /// Set insertion point at the beginning of the function.
      auto block1 = llvm::BasicBlock::Create(*theLLVMContext, "entry", currentFunction);
      llvmBuilder->SetInsertPoint(block1);

      for (auto itTempVar = tempVarsInfo.begin(); itTempVar != tempVarsInfo.end(); ++itTempVar) {
        dcds::Attribute tempAttr(itTempVar->first, std::get<0>(itTempVar->second), std::get<1>(itTempVar->second));
        if (std::get<2>(itTempVar->second)->getName() != currentFunction->getName()) {
          for (auto itStatement = orderedStatements.begin(); itStatement != orderedStatements.end(); ++itStatement) {
            if (itStatement->second->sourceAttrName == std::get<2>(itTempVar->second)->getName() ||
                itStatement->second->refVarName == std::get<2>(itTempVar->second)->getName()) {
              /// TODO: Throw appropriate logical error code (from c++ standard).
              std::cout << "Logical Error: Temporary variable not attached to this function.\n";
              break;
            }
          }
        }

        temporaryVariableIRMap[itTempVar->first] = tempAttr.addAttributeCodegen(theLLVMContext, block1, block1->end());

        llvm::Value *initVal = llvm::ConstantInt::get(
            *theLLVMContext, llvm::APInt(64, 0));  /// Dummy value to stop compiler from complaining.
        if (std::get<0>(itTempVar->second) == dcds::valueType::INTEGER)
          initVal = llvm::ConstantInt::get(*theLLVMContext,
                                           llvm::APInt(64, std::get<int64_t>(std::get<1>(itTempVar->second))));
        else if (std::get<0>(itTempVar->second) == dcds::valueType::RECORD_PTR)
          initVal = llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(*theLLVMContext));

        tempAttr.setAttributeCodegen(llvmBuilder, initVal, temporaryVariableIRMap[itTempVar->first]);
      }

      for (auto itStatement = orderedStatements.begin(); itStatement != orderedStatements.end(); ++itStatement) {
        /// Check whether statement belongs to this function
        if (itStatement->first.first == it->second) {
          codegenStatement(itStatement->second, block1);
        }
      }

      if (it->second->returnValueType == dcds::valueType::VOID) llvmBuilder->CreateRetVoid();
    }
  }

  void visit() {
    auto readFunctionName = dcds::llvmutil::getFunctionName(reinterpret_cast<void *>(read_data_structure_attribute));
    auto readFunctionType =
        llvm::FunctionType::get(llvm::Type::getVoidTy(*theLLVMContext),
                                {llvm::Type::getInt8PtrTy(*theLLVMContext), llvm::Type::getInt64Ty(*theLLVMContext),
                                 llvm::Type::getInt8PtrTy(*theLLVMContext), llvm::Type::getInt64Ty(*theLLVMContext)},
                                false);
    readFunction = llvm::Function::Create(readFunctionType, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                          readFunctionName, theLLVMModule.get());

    auto writeFunctionName = dcds::llvmutil::getFunctionName(reinterpret_cast<void *>(write_data_structure_attribute));
    auto writeFunctionType =
        llvm::FunctionType::get(llvm::Type::getVoidTy(*theLLVMContext),
                                {llvm::Type::getInt8PtrTy(*theLLVMContext), llvm::Type::getInt64Ty(*theLLVMContext),
                                 llvm::Type::getInt8PtrTy(*theLLVMContext), llvm::Type::getInt64Ty(*theLLVMContext)},
                                false);
    writeFunction = llvm::Function::Create(writeFunctionType, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                           writeFunctionName, theLLVMModule.get());

    llvm::FunctionType *externalFunctionType;
    for (auto fn : externalCallInfo) {
      externalFunctionType = llvm::FunctionType::get(
          llvm::Type::getVoidTy(*theLLVMContext),
          {llvm::Type::getInt8PtrTy(*theLLVMContext), llvm::Type::getInt8PtrTy(*theLLVMContext)}, false);
      externalFunctions.emplace(
          fn.first, llvm::Function::Create(externalFunctionType, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                           fn.first, theLLVMModule.get()));
    }

    bool hasAttr = attributes.size();
    /// Generate code for signature of user defined custom functions.
    for (auto it = functions.begin(); it != functions.end(); ++it) {
      userFunctions.push_back(it->second->codegenFunctionSignature(theLLVMContext, theLLVMModule, hasAttr));
    }

    /// Generate code for user defined custom functions.
    codegenUserFunctions();

    /// Save generated module and print it.
    saveModuleToFile("./Generated_IR.ll");
  }

  void build() {
    theLLVMJIT = ExitOnErr(DCDSJIT::Create());
    theLLVMModule->setDataLayout(theLLVMJIT->dataLayout);
    theLLVMModule->setTargetTriple(theLLVMJIT->targetMachine->getTargetTriple().str());

    /// Create a ResourceTracker to track JIT'd memory allocated to our
    /// anonymous expression -- that way we can free it after executing.
    auto RT = theLLVMJIT->getMainJITDylib().createResourceTracker();

    auto TSM = ThreadSafeModule(std::move(theLLVMModule), std::move(theLLVMContext));
    ExitOnErr(theLLVMJIT->addModule(std::move(TSM), RT));
  }

  auto getBuiltFunctionInt64ReturnType(const std::string &functionName) {
    auto rawFunc = theLLVMJIT->getRawAddress(functionName);
    return reinterpret_cast<int64_t (*)(void *)>(rawFunc);
  }

  auto getBuiltFunctionVoidReturnType(const std::string &functionName) {
    auto rawFunc = theLLVMJIT->getRawAddress(functionName);
    return reinterpret_cast<void (*)(void *)>(rawFunc);
  }

  auto getBuiltFunctionVoidPtrReturnType(const std::string &functionName) {
    auto rawFunc = theLLVMJIT->getRawAddress(functionName);
    return reinterpret_cast<void *(*)(void *)>(rawFunc);
  }

  auto getBuiltFunctionVoidReturnTypeVoidArg1(const std::string &functionName) {
    auto rawFunc = theLLVMJIT->getRawAddress(functionName);
    return reinterpret_cast<void (*)(void *, void *)>(rawFunc);
  }

  auto getBuiltFunctionVoidReturnTypeVoidArg2(const std::string &functionName) {
    auto rawFunc = theLLVMJIT->getRawAddress(functionName);
    return reinterpret_cast<void (*)(void *, void *, void *)>(rawFunc);
  }

  auto getBuiltFunctionVoidReturnTypeIntArg1(const std::string &functionName) {
    auto rawFunc = theLLVMJIT->getRawAddress(functionName);
    return reinterpret_cast<void (*)(void *, int64_t *)>(rawFunc);
  }

  auto getBuiltFunctionVoidReturnTypeIntArg2(const std::string &functionName) {
    auto rawFunc = theLLVMJIT->getRawAddress(functionName);
    return reinterpret_cast<void (*)(void *, int64_t *, int64_t *)>(rawFunc);
  }

  auto getBuiltFunctionVoidReturnTypeIntArg3(const std::string &functionName) {
    auto rawFunc = theLLVMJIT->getRawAddress(functionName);
    return reinterpret_cast<void (*)(void *, int64_t *, int64_t *, int64_t *)>(rawFunc);
  }

  auto getBuiltFunctionVoidReturnTypeIntArg1VoidPtr1(const std::string &functionName) {
    auto rawFunc = theLLVMJIT->getRawAddress(functionName);
    return reinterpret_cast<void (*)(void *, int64_t *, void *)>(rawFunc);
  }

  auto getBuiltFunctionVoidReturnTypeIntArg1VoidPtr2(const std::string &functionName) {
    auto rawFunc = theLLVMJIT->getRawAddress(functionName);
    return reinterpret_cast<void (*)(void *, int64_t *, void *, void *)>(rawFunc);
  }

 private:
  /// Store Data Structure Name
  const std::string dsName;
  /// Map function names with functions
  std::unordered_map<std::string, std::shared_ptr<FunctionBuilder>> functions;
  /// Map attribute names with attributes
  std::map<std::string, std::shared_ptr<Attribute>> attributes;
  /// Map temporary variable names with information related to them
  std::unordered_map<std::string,
                     std::tuple<dcds::valueType, std::variant<int64_t, void *>, std::shared_ptr<FunctionBuilder>>>
      tempVarsInfo;
  /// Map functions with function arguments info
  std::unordered_map<std::shared_ptr<FunctionBuilder>, std::vector<std::pair<std::string, dcds::valueType>>>
      funcArgsInfo;
  /// Ordered map for mapping functions with statements comprising them
  std::map<std::pair<std::shared_ptr<FunctionBuilder>, uint32_t>, std::shared_ptr<StatementBuilder>> orderedStatements;
  /// Map conditional block statements with conditional statements
  std::unordered_map<std::shared_ptr<StatementBuilder>, std::shared_ptr<ConditionStatementBuilder>>
      statementToConditionMap;
  /// Map statements with resultant temporary variables for arithmetic calculations
  std::unordered_map<std::shared_ptr<StatementBuilder>, std::string> tempVarsOpResName;
  /// Map externally called function with names of its expected arguments
  std::unordered_map<std::string, std::vector<std::string>> externalCallInfo;

  /// LLVM context for the codegen engine
  std::unique_ptr<LLVMContext> theLLVMContext;
  /// LLVM module for the codegen engine
  std::unique_ptr<Module> theLLVMModule;
  /// LLVM builder for the codegen engine
  std::unique_ptr<IRBuilder<>> llvmBuilder;
  /// LLVM function pass manager for the codegen engine
  std::unique_ptr<legacy::FunctionPassManager> theLLVMFPM;
  /// LLVM JIT instance for the codegen engine
  std::unique_ptr<DCDSJIT> theLLVMJIT;

  /// Keep track of the current function during codegen
  llvm::Function *currentFunction;
  /// Vector to store signature for all user functions
  std::vector<llvm::Function *> userFunctions;
  /// Map temporary variable name to code generated for it
  std::unordered_map<std::string, llvm::Value *> temporaryVariableIRMap;
  /// Exit on error mechanism
  ExitOnError ExitOnErr;

  /// Custom read function to read from storage layer
  llvm::Function *readFunction;
  /// Custom write function to write in storage layer
  llvm::Function *writeFunction;
  /// Map user provided external function names to their signatures
  std::map<std::string, llvm::Function *> externalFunctions;
};

}  // namespace dcds

#endif  // DCDS_CODEGEN_HPP
