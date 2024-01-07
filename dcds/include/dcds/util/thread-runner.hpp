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

 private:
  static auto getCoreMap() {
    const auto processor_count = std::thread::hardware_concurrency();

    // socket-first
    std::vector<size_t> cores;
    if (processor_count == 144) {
      constexpr size_t core_per_socket = 36;
      constexpr size_t num_socket = 4;
      for (size_t i = 0; i < num_socket; i++) {
        for (size_t j = i; j < (num_socket * core_per_socket); j += num_socket) {
          // LOG(INFO) << "Socket: " << i << " -- core: " << j;
          cores.emplace_back(j);
        }
      }
    } else if (processor_count == 48) {
      constexpr size_t core_per_socket = 24;
      constexpr size_t num_socket = 2;
      for (size_t i = 0; i < num_socket; i++) {
        for (size_t j = i; j < (num_socket * core_per_socket); j += num_socket) {
          // LOG(INFO) << "Socket: " << i << " -- core: " << j;
          cores.emplace_back(j);
        }
      }

    } else
      CHECK(false) << "which machine";
    return cores;
  }

  template <class pre_runner, class post_runner, class Function, class... Args>
  size_t runner(pre_runner&& pre_run, post_runner&& post_run, Function&& f, Args&&... args) {
    std::barrier<void (*)()> sync_point_pre(n_threads, []() {});
    std::vector<std::thread> runners;

    std::vector<size_t> cores = getCoreMap();

    for (uint64_t _ti = 0; _ti < n_threads; _ti++) {
      runners.emplace_back(
          [&](uint64_t _tid) {
            dcds::ScopedAffinityManager scopedAffinity((dcds::Core(cores[_tid])));

            // PRE-RUN
            pre_run(_tid);
            sync_point_pre.arrive_and_wait();

            // Main-function
            sync_point.arrive_and_wait();  // mark the start
            if (_tid == 0) start = clock ::now();
            f(_tid, args...);
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

 public:
  template <class Function, class... Args>
  size_t operator()(Function&& f, Args&&... args) {
    return this->runner([](uint64_t) {}, [](uint64_t) {}, f, args...);
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
