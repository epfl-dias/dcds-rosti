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

#include <dcds/dcds.hpp>
#include <random>

#include "ycsb.hpp"

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "YCSB";

  // dcds::ScopedAffinityManager scopedAffinity(dcds::Core{0});

  constexpr auto num_columns = 1;
  constexpr size_t num_threads = 16;
  constexpr size_t num_runs = 8;

  //  for(auto c = 1; c < num_columns ; c++){
  //    for(size_t i = 1 ; i <= num_threads; i++){
  //      for(size_t r = 1 ; r <= num_runs; r++){
  //        auto ycsb = YCSB(1, i * 1_M);
  //        ycsb.test_MT_lookup_sequential(i);
  //      }
  //    }
  //  }

  for (auto c = 1; c <= num_columns; c++) {
    for (size_t i = 8; i <= num_threads; i++) {
      for (size_t r = 0; r < num_runs; r++) {
        auto ycsb = YCSB(1, i * 1_M);
        if (r == 0 || r == 7) dcds::profiling::Profile::resume();
        ycsb.test_MT_lookup_random(i);

        if (r == 0 || r == 7) dcds::profiling::Profile::pause();

        dcds::storage::TableRegistry::getInstance().clear();
      }
    }
  }

  //  auto builder = std::make_shared<dcds::Builder>("YCSB");
  //  //   builder->addHint(dcds::hints::BuilderHints::SINGLE_THREADED);
  //  auto ycsb_item = generateYCSB_Item(builder);
  //
  //  // Attribute array is integer-indexed.
  //  // Attribute IndexedList is key-based index.
  //
  //  builder->addAttributeArray("records", ycsb_item, num_records);
  //
  //  generateLookupOneFunction(builder);
  //  generateLookupNFunction(builder);
  //  generateUpdateFunction(builder);
  //
  //  //  builder->dump();
  //  builder->injectCC();
  //  LOG(INFO) << "######################";
  //  builder->dump();
  //  LOG(INFO) << "######################";
  //
  //  builder->build();
  //
  //  auto instance = builder->createInstance();
  //  instance->listAllAvailableFunctions();
  //
  //  test_MT_lookup_sequential(instance, 8);  // warm-up
  //
  //  LOG(INFO) << "-----test_MT_lookup_sequential (1 op/txn)----";
  //  dcds::profiling::Profile::resume();
  //  for (size_t i = 1; i <= num_threads; i *= 2) {
  //    test_MT_lookup_sequential(instance, i);
  //  }
  //  dcds::profiling::Profile::pause();
  //
  //  LOG(INFO) << "-----test_MT_lookup_random (1 op/txn)----";
  //
  //  dcds::profiling::Profile::resume();
  //  for (size_t i = 1; i <= num_threads; i *= 2) {
  //    test_MT_lookup_random(instance, i);
  //  }
  //  dcds::profiling::Profile::pause();

  //  LOG(INFO) << "-----test_MT_lookup_sequential-N  (" << num_ops_per_txn << " op/txn)---";
  //  dcds::profiling::Profile::resume();
  //  for (size_t i = 1; i <= num_threads; i *= 2) {
  //    test_MT_lookup_sequential_n(instance, i);
  //  }
  //  dcds::profiling::Profile::pause();

  return 0;
}
