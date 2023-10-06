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

#include "dcds/codegen/llvm-codegen/llvm-jit.hpp"

using namespace llvm;
using namespace dcds;

// bool print_generated_code = true;

llvm::Expected<llvm::orc::ThreadSafeModule> LLVMJIT::printIR(llvm::orc::ThreadSafeModule module,
                                                             const std::string &suffix) {
  module.withModuleDo([&suffix](llvm::Module &m) {
    std::error_code EC;
    llvm::raw_fd_ostream out("generated_code/" + m.getName().str() + suffix + ".ll", EC,
                             llvm::sys::fs::OpenFlags::OF_None);
    m.print(out, nullptr, false, true);
  });
  return std::move(module);
}

llvm::Expected<llvm::orc::ThreadSafeModule> LLVMJIT::optimizeModule(llvm::orc::ThreadSafeModule TSM,
                                                                    const llvm::orc::MaterializationResponsibility &R) {
  TSM.withModuleDo([](Module &M) {
    // Create a function pass manager.
    auto FPM = std::make_unique<legacy::FunctionPassManager>(&M);

    // Add some optimizations.
    FPM->add(createInstructionCombiningPass());
    FPM->add(createReassociatePass());
    FPM->add(createGVNPass());
    FPM->add(createCFGSimplificationPass());
    FPM->add(createDeadCodeEliminationPass());
    FPM->doInitialization();

    // Run the optimizations over all functions in the module being added to
    // the JIT.
    for (auto &F : M) FPM->run(F);
  });

  return std::move(TSM);
}

void *LLVMJIT::getCompiledFunction(llvm::Function *function_ptr) {
  assert(function_ptr);
  return getCompiledFunction(function_ptr->getName().str());
}
void *LLVMJIT::getCompiledFunction(const std::string &function_name) {
  return (void *)this->lookup(function_name).getAddress();
}