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

#include "un-generated/linked-list.hpp"

#include <dcds/util/logging.hpp>
#include <dcds/util/thread-runner.hpp>
#include <dcds/util/timing.hpp>

namespace dcds_hardcoded {

void LinkedList::testMT(size_t n_threads, size_t iterations) {
  LOG(INFO) << "FIFO::testMT() -- start";

  std::vector<std::thread> runners;
  std::barrier<void (*)()> sync_point(n_threads, []() {});

  std::atomic<size_t> overall_sum = 0;
  size_t expected_sum = n_threads * iterations;

  std::vector<std::chrono::milliseconds> times;

  for (size_t x = 0; x < n_threads; x++) {
    runners.emplace_back([&, x]() {
      size_t local_sum = 0;
      int64_t val = 0;

      sync_point.arrive_and_wait();
      // we need to time the following block
      {
        // time_block t("Trad_clust_p1:");
        time_block t{[&](auto tms) {
          //          LOG(INFO) << '\t' << std::this_thread::get_id() << ": " << tms.count() << "ms";
          times.emplace_back(tms);
        }};

        // time_block t2("t2_: ");

        for (size_t i = 0; i < iterations; i++) {
          this->push(1);
        }

        for (size_t i = 0; i < iterations; i++) {
          this->pop(val);
          local_sum += val;
        }
      }

      sync_point.arrive_and_wait();  // not required i guess.

      overall_sum.fetch_add(local_sum);
    });
  }

  for (auto& th : runners) {
    th.join();
  }
  size_t total = 0;
  for (auto& t : times) {
    total += t.count();
  }
  LOG(INFO) << "Total time: " << total << "ms";

  LOG(INFO) << "Expected sum: " << expected_sum << " | calc_sum: " << overall_sum;
}
}  // namespace dcds_hardcoded
