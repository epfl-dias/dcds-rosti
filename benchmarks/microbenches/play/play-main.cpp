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
#include <dcds/util/logging.hpp>

#include "dcds-generated/list/doubly-linked-list.hpp"
#include "dcds/dcds.hpp"

// CHECKs with an any cast wrapper
static void EXPECT_TRUE(std::any v) { CHECK(std::any_cast<bool>(v)); }
static void EXPECT_FALSE(std::any v) { assert(std::any_cast<bool>(v) == false); }

static void fnUsage(std::shared_ptr<dcds::Builder>& builder) {
  dcds::BuilderOptPasses buildOptimizer(builder);
  buildOptimizer.runAll();
}

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "main() -- start";

  DoublyLinkedList dl;
  // dl.dump();
  dl.build(true, false);
  // dl.dump();

  auto instance = dl.createInstance();
  instance->listAllAvailableFunctions();

  LOG(INFO) << "DoublyLinkedList::testFrontBackMixedOps() -- start";

  uint64_t val = 0;

  EXPECT_TRUE(instance->op("empty"));

  instance->op("push_front", 11);
  auto head_11 = instance->op("head_ptr");

  instance->op("push_front", 22);
  // 22->11
  instance->op("push_back", 23);

  // 22->11->23

  //  auto x = instance->op("tail_ptr");
  //  LOG(INFO) << "xxx: " << std::any_cast<uintptr_t>(x);
  //  instance->op("touch", std::any_cast<uintptr_t>(x));
  //  LOG(INFO) << "touch-done";
  //  EXPECT_TRUE(instance->op("pop_front", &val));
  //  assert(val == 23);

  LOG(INFO) << "xxx: " << std::any_cast<uintptr_t>(head_11);
  instance->op("touch", std::any_cast<uintptr_t>(head_11));
  EXPECT_TRUE(instance->op("pop_front", &val));
  assert(val == 11);

  //
  //  //  EXPECT_TRUE(instance->op("pop_back", &val));
  //  //  assert(val == 23);
  //  //  LOG(INFO) << "here.";
  //  //  EXPECT_TRUE(instance->op("pop_back", &val));
  //  //  assert(val == 11);
  //  //  LOG(INFO) << "here..";
  //  EXPECT_FALSE(instance->op("empty"));

  LOG(INFO) << "main() -- done";
  return 0;
}
