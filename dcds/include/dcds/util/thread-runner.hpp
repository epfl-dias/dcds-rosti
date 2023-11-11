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

#ifndef DCDS_THREAD_RUNNER_HPP
#define DCDS_THREAD_RUNNER_HPP

#include <barrier>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

class ThreadRunner {
 public:
  ThreadRunner(size_t _n_threads = 1) : n_threads(_n_threads), sync_point(_n_threads, []() {}) {}

  // TODO: Add timing utilities
  // TODO: Add pre- / post-run also.

  template <class Function, class... Args>
  void operator()(Function&& f, Args&&... args) {
    std::vector<std::thread> runners;

    for (uint64_t _ti = 0; _ti < n_threads; _ti++) {
      runners.emplace_back([&]() {
        sync_point.arrive_and_wait();
        f(args...);
      });
    }

    for (auto& th : runners) {
      th.join();
    }
  }

 private:
  const size_t n_threads{};
  std::barrier<void (*)()> sync_point;
};

#endif  // DCDS_THREAD_RUNNER_HPP
