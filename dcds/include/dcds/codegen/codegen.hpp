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

#ifndef DCDS_CODEGEN_HPP
#define DCDS_CODEGEN_HPP

#include "dcds/builder/builder.hpp"

namespace dcds {

class Codegen {
 public:
  virtual void build(dcds::Builder* _builder, bool is_nested_type = false) = 0;
  virtual void jitCompileAndLoad() = 0;
  virtual void runOptimizationPasses() = 0;

  virtual void printIR() = 0;
  virtual void saveToFile(const std::string& filename) = 0;

  virtual void* getFunction(const std::string& name) = 0;
  virtual void* getFunctionPrefixed(const std::string& name) = 0;

  inline auto& getAvailableFunctions() {
    assert(is_jit_done);
    return available_jit_functions;
  }
  inline const jit_function_t* getJitFunction(const std::string& name) {
    assert(is_jit_done);
    return available_jit_functions[name];
  }

  virtual ~Codegen();

 protected:
  explicit Codegen(dcds::Builder* _builder) : top_level_builder(_builder) {}

 protected:
  dcds::Builder* top_level_builder;
  std::map<std::string, jit_function_t*> available_jit_functions;
  bool is_jit_done = false;
};

}  // namespace dcds

#endif  // DCDS_CODEGEN_HPP
