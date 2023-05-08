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

#include <iostream>

#include <benchmark/benchmark.h>
#include <libcuckoo/cuckoohash_map.hh>
#include <dcds/util/logging.hpp>

#include "un-generated/queue.hpp"


int main(int argc, char** argv) {
    LOG(INFO) << "listBasedDS";
    UnGeneratedQueue defaultNSQueue;
    LOG(INFO) << "[main] init-done";
    LOG(INFO) << "defaultNSQueue: size: " << defaultNSQueue.size();
    LOG(INFO) << "-------";
//    UnGeneratedQueue nSQueue("separated");
//    LOG(INFO) << "nSQueue: size: " << nSQueue.size();

    LOG(INFO) << "###########";
    defaultNSQueue.printQueue();
    defaultNSQueue.push(10);
    LOG(INFO) << "###########";
    defaultNSQueue.printQueue();
    defaultNSQueue.push(11);
    LOG(INFO) << "###########";
    defaultNSQueue.printQueue();
    defaultNSQueue.push(10);
    LOG(INFO) << "###########";
    defaultNSQueue.printQueue();
    defaultNSQueue.push(12);
    LOG(INFO) << "###########";
    defaultNSQueue.printQueue();

    LOG(INFO) << "------- DONE";
    return 0;
}
