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

#ifndef DCDS_LLVM_JIT_HPP
#define DCDS_LLVM_JIT_HPP

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/EPCIndirectionUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutorProcessControl.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

#include "dcds/util/logging.hpp"
#include "dcds/util/timing.hpp"

namespace dcds {

static bool print_generated_code = true;
static bool print_optimized_code = false;

class PassConfiguration {
 public:
  std::unique_ptr<llvm::TargetMachine> JTM;
  llvm::Triple ModuleTriple;
  llvm::TargetLibraryInfoImpl TLII;
  llvm::legacy::PassManager Passes;
  llvm::PassManagerBuilder Builder;
  llvm::ImmutablePass *TTIPass;
  llvm::FunctionPass *PrefetchPass;

 public:
  explicit PassConfiguration(std::unique_ptr<llvm::TargetMachine> TM)
      : JTM(std::move(TM)),
        ModuleTriple(JTM->getTargetTriple()),
        TLII(ModuleTriple),
        TTIPass(createTargetTransformInfoWrapperPass(JTM->getTargetIRAnalysis())),
        PrefetchPass(llvm::createLoopDataPrefetchPass()) {
    llvm::Pass *TPC = dynamic_cast<llvm::LLVMTargetMachine *>(JTM.get())->createPassConfig(Passes);
    Passes.add(TPC);

    Passes.add(new llvm::TargetLibraryInfoWrapperPass(TLII));

    // Add internal analysis passes from the target machine.
    Passes.add(createTargetTransformInfoWrapperPass(JTM->getTargetIRAnalysis()));

    Builder.OptLevel = 3;
    Builder.SizeLevel = 0;

    Builder.Inliner = llvm::createFunctionInliningPass(3, 0, false);

    Builder.DisableUnrollLoops = false;
    Builder.LoopVectorize = true;

    Builder.SLPVectorize = true;

    JTM->adjustPassManager(Builder);

    Builder.populateModulePassManager(Passes);
  }
};

class LLVMJIT {
 public:
  ~LLVMJIT() {
    LOG(INFO) << "[LLVMJIT] Destructor";
    if (auto Err = ES.endSession()) ES.reportError(std::move(Err));
  }

  explicit LLVMJIT(
      llvm::orc::JITTargetMachineBuilder JTMB = llvm::cantFail(llvm::orc::JITTargetMachineBuilder::detectHost())
                                                    .setCodeGenOptLevel(llvm::CodeGenOpt::Aggressive)
                                                    .setCodeModel(llvm::CodeModel::Model::Large))
      : DL(llvm::cantFail(JTMB.getDefaultDataLayoutForTarget())),
        Mangle(ES, this->DL),
        ObjectLayer(ES, []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
        CompileLayer(ES, ObjectLayer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(JTMB)),
        PrintOptimizedIRLayer(
            ES, CompileLayer,
            [](llvm::orc::ThreadSafeModule TSM,
               const llvm::orc::MaterializationResponsibility &R) -> llvm::Expected<llvm::orc::ThreadSafeModule> {
              if (print_optimized_code) return printIR(std::move(TSM), "_opt");
              return std::move(TSM);
            }),
        TransformLayer(ES, PrintOptimizedIRLayer,
                       [_JTMB = std::move(JTMB)](llvm::orc::ThreadSafeModule TSM,
                                                 const llvm::orc::MaterializationResponsibility &R) mutable
                       -> llvm::Expected<llvm::orc::ThreadSafeModule> {
                         auto TM = _JTMB.createTargetMachine();
                         if (!TM) return TM.takeError();
                         return optimizeModule2(std::move(TSM), std::move(TM.get()));
                         // return optimizeModule(std::move(TSM), R);
                       }),
        PrintGeneratedIRLayer(
            ES, TransformLayer,
            [](llvm::orc::ThreadSafeModule TSM,
               const llvm::orc::MaterializationResponsibility &R) -> llvm::Expected<llvm::orc::ThreadSafeModule> {
              if (print_generated_code) return printIR(std::move(TSM));
              return std::move(TSM);
            }),
        MainJD(llvm::cantFail(ES.createJITDylib("main"))),
        vtuneProfiler(llvm::JITEventListener::createIntelJITEventListener()) {
    LOG(INFO) << "[LLVMJIT] Constructing JITTER";

    if (vtuneProfiler == nullptr) {
      LOG(WARNING) << "Could not create VTune listener";
    } else {
      ObjectLayer.setNotifyLoaded([this](llvm::orc::MaterializationResponsibility &R,
                                         const llvm::object::ObjectFile &Obj,
                                         const llvm::RuntimeDyld::LoadedObjectInfo &loi) {
        std::scoped_lock<std::mutex> lock{vtuneLock};
        cantFail(
            R.withResourceKeyDo([&](llvm::orc::ResourceKey k) { vtuneProfiler->notifyObjectLoaded(k, Obj, loi); }));
      });
    }

    //    ObjectLayer.setNotifyEmitted(
    //        [](llvm::orc::VModuleKey k, std::unique_ptr<MemoryBuffer> mb) {
    //          LOG(INFO) << "Emitted " << k << " " << mb.get();
    //        });

    //    ES.setDispatchTask([&p = pool](std::unique_ptr<llvm::orc::Task> T) {
    //      p.enqueue([UnownedT = std::move(T)]() mutable { UnownedT->run(); });
    //    });

    MainJD.addGenerator(
        llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(this->DL.getGlobalPrefix())));
  }

 public:
  void *getCompiledFunction(llvm::Function *function_ptr);
  void *getCompiledFunction(const std::string &function_name);

 public:
  const llvm::DataLayout &getDataLayout() const { return DL; }
  llvm::orc::JITDylib &getMainJITDylib() { return MainJD; }

  void addModule(llvm::orc::ThreadSafeModule M) { llvm::cantFail(PrintGeneratedIRLayer.add(MainJD, std::move(M))); }

  void dump() { ES.dump(llvm::outs()); }

  llvm::JITEvaluatedSymbol lookup(llvm::StringRef Name) {
    return llvm::cantFail(ES.lookup({&MainJD}, Mangle(Name.str())));
  }

  void *getRawAddress(std::string const &name) {
    return reinterpret_cast<void *>(llvm::cantFail(ES.lookup({&MainJD}, Mangle(name))).getAddress());
  }

 private:
  static llvm::Expected<llvm::orc::ThreadSafeModule> printIR(llvm::orc::ThreadSafeModule module,
                                                             const std::string &suffix = "");

  //  static llvm::Expected<llvm::orc::ThreadSafeModule> optimizeModule(llvm::orc::ThreadSafeModule TSM,
  //                                                                    const llvm::orc::MaterializationResponsibility
  //                                                                    &R);
  static llvm::orc::ThreadSafeModule optimizeModule2(llvm::orc::ThreadSafeModule TSM,
                                                     std::unique_ptr<llvm::TargetMachine> TM);

 private:
  llvm::orc::ExecutionSession ES{llvm::cantFail(llvm::orc::SelfExecutorProcessControl::Create())};
  llvm::DataLayout DL;
  llvm::orc::MangleAndInterner Mangle;

  llvm::orc::RTDyldObjectLinkingLayer ObjectLayer;
  llvm::orc::IRCompileLayer CompileLayer;
  llvm::orc::IRTransformLayer PrintOptimizedIRLayer;
  llvm::orc::IRTransformLayer TransformLayer;
  llvm::orc::IRTransformLayer PrintGeneratedIRLayer;

  llvm::orc::JITDylib &MainJD;

  std::mutex vtuneLock;
  llvm::JITEventListener *vtuneProfiler;
};

}  // namespace dcds

#endif  // DCDS_LLVM_JIT_HPP
