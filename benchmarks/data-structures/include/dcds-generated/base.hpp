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

#ifndef DCDS_BASE_HPP
#define DCDS_BASE_HPP

#include <dcds/dcds.hpp>

class dcds_generated_ds {
 protected:
  explicit dcds_generated_ds(const std::string& name) : builder(std::make_shared<dcds::Builder>(name)) {}

 public:
  auto getBuilder() { return builder; }
  void dump() { builder->dump(); }

  void build(bool inject_cc = true, bool optimize = false) {
    if (optimize) {
      dcds::BuilderOptPasses buildOptimizer(builder);
      buildOptimizer.runAll();
    }
    if (inject_cc) {
      builder->injectCC();
    }

    builder->build();
  }

  auto createInstance() { return builder->createInstance(); }

 protected:
  std::shared_ptr<dcds::Builder> builder;
};

#endif  // DCDS_BASE_HPP
