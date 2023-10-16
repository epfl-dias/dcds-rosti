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

#ifndef DCDS_LLVM_CODEGEN_HPP
#define DCDS_LLVM_CODEGEN_HPP

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/EPCIndirectionUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutorProcessControl.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>

#include "dcds/codegen/codegen.hpp"
#include "dcds/codegen/llvm-codegen/llvm-context.hpp"
#include "dcds/codegen/llvm-codegen/llvm-jit.hpp"
#include "dcds/exporter/code-exporter.hpp"

namespace dcds {
using namespace llvm;

namespace expressions {
class LLVMExpressionVisitor;
}

class LLVMCodegen : public Codegen, public LLVMCodegenContext {
  friend class expressions::LLVMExpressionVisitor;

 private:
  struct function_build_context {
    std::shared_ptr<FunctionBuilder> fb;
    std::shared_ptr<StatementBuilder> sb;
    llvm::Function *fn;
    std::map<std::string, llvm::Value *> *tempVariableMap;

    explicit function_build_context(std::shared_ptr<FunctionBuilder> _fb, std::shared_ptr<StatementBuilder> _sb,
                                    llvm::Function *_fn, std::map<std::string, llvm::Value *> *_tempVariableMap)
        : fb(_fb), sb(_sb), fn(_fn), tempVariableMap(_tempVariableMap) {}
  };

 public:
  explicit LLVMCodegen(dcds::Builder *builder);
  ~LLVMCodegen() override {
    LOG(WARNING) << "[LLVMCodegen] Destructor";
    theLLVMFPM->doFinalization();
    theLLVMFPM.reset();

    temporaryVariableIRMap.clear();
    llvmBuilder.reset();
    userFunctions.clear();
    this->availableFunctions.clear();
  }

  void build(dcds::Builder *builder) override;
  void saveToFile(const std::string &filename) override;
  void jitCompileAndLoad() override;

  void printIR() override;
  void testHelloWorld();

  void *getFunction(const std::string &name) override;
  void *getFunctionPrefixed(const std::string &name) override;

 private:
  void runOptimizationPasses() override;

 private:
  [[nodiscard]] llvm::Module *getModule() const override;
  [[nodiscard]] llvm::IRBuilder<> *getBuilder() const override;

  llvm::Function *wrapFunctionVariadicArgs(llvm::Function *inner_function,
                                           const std::vector<llvm::Type *> &position_args,
                                           const std::vector<llvm::Type *> &expected_vargs,
                                           std::string wrapped_function_name);

 private:
  void initializeLLVMModule(const std::string &name);
  void initializePassManager();

  void codegenHelloWorld();

  void createDsContainerStruct(dcds::Builder *builder);
  Value *initializeDsContainerStruct(Value *txnManager, Value *storageTable, Value *mainRecord);

  void createDsStructType(dcds::Builder *builder);
  Value *initializeDsValueStructDefault(dcds::Builder &builder);

  void buildConstructor(dcds::Builder &builder);
  void buildDestructor();
  void buildFunctions(dcds::Builder *builder);
  void buildOneFunction(dcds::Builder *builder, std::shared_ptr<FunctionBuilder> &fb);
  void buildFunctionDictionary(dcds::Builder &builder);

  llvm::Function *buildInitTablesFn(dcds::Builder &builder, llvm::Value *table_name);
  llvm::Function *genFunctionSignature(std::shared_ptr<FunctionBuilder> &fb,
                                       const std::vector<llvm::Type *> &pre_args = {},
                                       const std::string &name_prefix = "", const std::string &name_suffix = "");
  std::map<std::string, llvm::Value *> allocateTemporaryVariables(std::shared_ptr<FunctionBuilder> &fb,
                                                                  llvm::BasicBlock *basicBlock);

  void buildFunctionBody(dcds::Builder *builder, std::shared_ptr<FunctionBuilder> &fb,
                         std::shared_ptr<StatementBuilder> &sb, llvm::Function *fn, llvm::BasicBlock *basicBlock,
                         std::map<std::string, llvm::Value *> &tempVariableMap);

  void buildStatement(dcds::Builder *builder, function_build_context &fnCtx, std::shared_ptr<Statement> &stmt);
  void buildStatement_ConditionalStatement(dcds::Builder *builder, function_build_context &fnCtx,
                                           std::shared_ptr<Statement> &stmt);

  llvm::Value *codegenExpression(dcds::Builder *builder, function_build_context &fnCtx,
                                 const dcds::expressions::Expression *expr);

 private:
  //  std::vector<llvm::Function *> userFunctions;
  std::map<std::string, llvm::Function *> userFunctions;
  std::unordered_map<std::string, llvm::Value *> temporaryVariableIRMap;

 private:
  std::unique_ptr<LLVMContext> theLLVMContext;
  std::unique_ptr<legacy::FunctionPassManager> theLLVMFPM;
  std::unique_ptr<IRBuilder<>> llvmBuilder;
  std::unique_ptr<Module> theLLVMModule;

 private:
  StructType *dsContainerStructType{};
  StructType *dsRecordValueStructType{};

 private:
  std::unique_ptr<LLVMJIT> jitter;

  //  struct function_builder_context {
  //    // temporary_variables
  //  };

  // private:
  //  CodeExporter raw_code_exporter;
};

}  // namespace dcds

#endif  // DCDS_LLVM_CODEGEN_HPP
