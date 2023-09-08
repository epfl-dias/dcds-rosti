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
// Created by prathamesh on 27/7/23.
//

#ifndef DCDS_CODEGEN_DCDSJIT_HPP
#define DCDS_CODEGEN_DCDSJIT_HPP

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
//#include "llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
//#include "llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h"
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
//#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include <dcds/codegen/utils.hpp>

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

namespace dcds {
using namespace llvm;
using namespace orc;

class DCDSJIT {
 public:
  static llvm::Expected<std::unique_ptr<DCDSJIT>> Create() {
    auto EPC = SelfExecutorProcessControl::Create();
    if (!EPC) return EPC.takeError();

    auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));
    JITTargetMachineBuilder JTMB(ES->getExecutorProcessControl().getTargetTriple());

    auto DL = JTMB.getDefaultDataLayoutForTarget();
    if (!DL) return DL.takeError();

    return std::make_unique<DCDSJIT>(std::move(JTMB), std::move(ES), std::move(*DL));
  }

  DCDSJIT(llvm::orc::JITTargetMachineBuilder tmb, std::unique_ptr<ExecutionSession> ES, DataLayout DL)
      : theES(std::move(ES)),
        dataLayout(std::move(DL)),
        targetMachine(dcds::llvmutil::unwrap(tmb.createTargetMachine())),
        mangle(*this->theES, this->dataLayout),
        objectLayer(*this->theES, [] { return std::make_unique<llvm::SectionMemoryManager>(); }),
        compileLayer(*this->theES, objectLayer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(tmb))),
        optimizeLayer(*this->theES, compileLayer, optimizeModule),
        mainJD(this->theES->createBareJITDylib("<main>")) {
    mainJD.addGenerator(cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())));
  }

  ~DCDSJIT() {
    if (auto Err = theES->endSession()) theES->reportError(std::move(Err));
  }

  const DataLayout &getDataLayout() const { return dataLayout; }

  JITDylib &getMainJITDylib() { return mainJD; }

  Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr) {
    if (!RT) RT = mainJD.getDefaultResourceTracker();

    return optimizeLayer.add(RT, std::move(TSM));
  }

  auto lookup(StringRef Name) { return dcds::llvmutil::unwrap(theES->lookup({&mainJD}, mangle(Name.str()))); }

  static Expected<ThreadSafeModule> optimizeModule(ThreadSafeModule TSM, const MaterializationResponsibility &R) {
    TSM.withModuleDo([](Module &M) {
      // Create a function pass manager.
      auto FPM = std::make_unique<legacy::FunctionPassManager>(&M);

      // Add some optimizations.
//      FPM->add(createInstructionCombiningPass());
//      FPM->add(createReassociatePass());
//      FPM->add(createGVNPass());
//      FPM->add(createCFGSimplificationPass());
//      FPM->add(createDeadCodeEliminationPass());
      FPM->doInitialization();

      // Run the optimizations over all functions in the module being added to
      // the JIT.
      for (auto &F : M) FPM->run(F);
    });

    return std::move(TSM);
  }

  void *getRawAddress(std::string const &name) {
    auto address = dcds::llvmutil::unwrap(theES->lookup({&mainJD}, mangle(name))).getAddress();
    return reinterpret_cast<void *>(address);
  }

  //  template<typename ReturnType, typename... Arguments>
  //  auto getAddress(auto fn) {
  //    return reinterpret_cast<ReturnType (*)(Arguments...)>(getRawAddress("update_and_read_fn"));
  ////    return reinterpret_cast<int32_t (*)()>(getRawAddress("update_and_read_fn"));
  //  }

  template <typename ReturnType, typename... Arguments>
  auto getAddress(dcds::llvmutil::function_ref<ReturnType, Arguments...> const &fn) {
    //      return reinterpret_cast<ReturnType (*)(Arguments...)>(getRawAddress(fn.getName()));
    return reinterpret_cast<int32_t (*)()>(getRawAddress(fn.getName()));
  }

  std::unique_ptr<ExecutionSession> theES;
  llvm::DataLayout dataLayout;
  std::unique_ptr<llvm::TargetMachine> targetMachine;
  llvm::orc::MangleAndInterner mangle;
  llvm::orc::RTDyldObjectLinkingLayer objectLayer;
  llvm::orc::IRCompileLayer compileLayer;
  llvm::orc::IRTransformLayer optimizeLayer;
  llvm::orc::JITDylib &mainJD;
};
}  // namespace dcds

#endif  // DCDS_CODEGEN_DCDSJIT_HPP
