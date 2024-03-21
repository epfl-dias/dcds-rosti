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
#include <absl/log/check.h>

#include <any>
#include <dcds/dcds.hpp>
#include <dcds/util/logging.hpp>

// Data structures declared in benchmarks/data-structures
#include "dcds-generated/list/doubly-linked-list.hpp"
#include "dcds-generated/list/lru-list.hpp"

// CHECK with an any cast wrapper
static void EXPECT_TRUE(std::any v) { CHECK(std::any_cast<bool>(v)); }
static void EXPECT_FALSE(std::any v) { assert(std::any_cast<bool>(v) == false); }

static void fnUsage(std::shared_ptr<dcds::Builder>& builder) {
  dcds::BuilderOptPasses buildOptimizer(builder);
  buildOptimizer.runAll();
}

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "main() -- start";

  LOG(INFO) << "main() -- done";
  return 0;
}
