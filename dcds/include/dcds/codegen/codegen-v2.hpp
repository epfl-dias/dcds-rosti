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

#ifndef DCDS_CODEGEN_V2_HPP
#define DCDS_CODEGEN_V2_HPP

#include "dcds/builder/builder.hpp"
// #include "dcds/exporter/jit-container.hpp"

namespace dcds {

class CodegenV2 {
 public:
  virtual void build() = 0;
  virtual void jitCompileAndLoad() = 0;
  virtual void runOptimizationPasses() = 0;

  virtual void printIR() = 0;
  virtual void saveToFile(const std::string &filename) = 0;

  virtual void* getFunction(const std::string& name) = 0;



 protected:
  explicit CodegenV2(dcds::Builder &_builder) : builder(_builder) {}

 protected:
  dcds::Builder &builder;
};

}  // namespace dcds

#endif  // DCDS_CODEGEN_V2_HPP