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

#include <benchmark/benchmark.h>

#include <dcds/util/logging.hpp>
#include <iostream>
#include <libcuckoo/cuckoohash_map.hh>

#include "parser-generated/linked-list.hpp"
#include "un-generated/linked-list.hpp"

constexpr size_t total_items = 8_K;

constexpr size_t n_threads = 1;
constexpr size_t n_iter = total_items / n_threads;

static void test_linkedList_stack() {
  auto ll = dcds_generated::Stack();

  ll.build();
  // ll.optimize();
  ll.test();
}

static void test_linkedList_fifo() {
  auto ll = dcds_generated::FIFO();

  ll.build();
  //  ll.optimize();
  // ll.test();

  ll.testMT(n_threads, n_iter);
}

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "listBasedDS";
  test_linkedList_stack();
  test_linkedList_fifo();
  LOG(INFO) << "------- DONE";
  return 0;
}
