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

#include "dcds/util/affinity-manager.hpp"
#include "dcds/util/timing.hpp"

namespace dcds {

const auto processor_count = std::thread::hardware_concurrency();

// template <typename TDuration = std::chrono::milliseconds, typename TClock = std::chrono::system_clock>
class ThreadRunner {
 public:
  using TDuration = std::chrono::milliseconds;
  using TClock = std::chrono::system_clock;
  using clock = TClock;
  using dur = typename TClock::duration;
  static_assert(dur{1} <= TDuration{1}, "clock not precise enough");

 public:
  explicit ThreadRunner(std::ptrdiff_t _n_threads = 1) : n_threads(_n_threads), sync_point(_n_threads, []() {}) {}

  template <class pre_runner, class post_runner, class Function, class... Args>
  size_t operator()(pre_runner&& pre_run, post_runner&& post_run, Function&& f, Args&&... args) {
    std::barrier<void (*)()> sync_point_pre(n_threads, []() {});
    std::vector<std::thread> runners;

    for (uint64_t _ti = 0; _ti < n_threads; _ti++) {
      runners.emplace_back(
          [&](uint64_t _tid) {
            dcds::ScopedAffinityManager scopedAffinity(dcds::Core(_tid % processor_count));

            // PRE-RUN
            pre_run(_tid);
            sync_point_pre.arrive_and_wait();

            // Main-function
            sync_point.arrive_and_wait();  // mark the start
            if (_tid == 0) start = clock ::now();
            f(args...);
            sync_point.arrive_and_wait();  // mark the end
            if (_tid == 0) end = clock ::now();

            // POST-RUN
            post_run(_tid);
          },
          _ti);
    }

    for (auto& th : runners) {
      th.join();
    }

    auto d = std::chrono::duration_cast<TDuration>(end - start);
    return d.count();
  }

  template <class Function, class... Args>
  size_t operator()(Function&& f, Args&&... args) {
    return this->operator()([](uint64_t) {}, [](uint64_t) {}, f, args...);
  }

 public:
  const size_t n_threads{};

 private:
  std::barrier<void (*)()> sync_point;
  std::chrono::time_point<clock> start;
  std::chrono::time_point<clock> end;
};

}  // namespace dcds

#endif  // DCDS_THREAD_RUNNER_HPP
