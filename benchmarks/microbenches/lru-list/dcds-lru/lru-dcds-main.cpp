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

#include "baseline/list/lru-list-global-lock.hpp"
#include "baseline/list/lru-list-tbb.hpp"
#include "dcds-generated/list/lru-list.hpp"
#include "dcds/util/bench-utils/zipf-generator.hpp"

constexpr size_t num_op_per_thread = 100000;
// constexpr size_t domain_min = 0;
constexpr size_t domain_max = 1_M;  // 1000;
constexpr size_t lru_capacity = 1000;

static void printThroughput(size_t runtime_ms, size_t n_threads, const std::string& prefix = "") {
  auto runtime_s = ((static_cast<double>(runtime_ms)) / 1000);
  auto throughput = ((num_op_per_thread * n_threads) / runtime_s);
  auto avg_per_thread = (throughput / n_threads) / 1000000;
  LOG(INFO) << "Threads: " << n_threads << prefix
            << ": Throughput: " << ((num_op_per_thread * n_threads) / runtime_s) / 1000000 << " MTPS"
            << " | avg-per-thread: " << avg_per_thread << " MTPS"
            << " | total_time: " << runtime_ms << "ms";
}
static auto initWorkload(size_t n_threads, double zipf_theta = 0.0) {
  std::vector<std::vector<size_t>> thread_keys;
  constexpr size_t seed = 42;
  std::mt19937 gen(seed);

  thread_keys.resize(n_threads);
  for (auto& per_thread_keys : thread_keys) {
    per_thread_keys.resize(num_op_per_thread, 42);
  }

  if (zipf_theta == 0.0) {
    std::uniform_int_distribution<> distrib(0, domain_max);
    for (size_t thread_idx = 0; thread_idx < n_threads; thread_idx++) {
      for (size_t i = 0; i < num_op_per_thread; i++) {
        thread_keys.at(thread_idx)[i] = distrib(gen);
      }
    }
  } else {
    absl::zipf_distribution<size_t> distrib(domain_max, 2.0, zipf_theta);
    for (size_t thread_idx = 0; thread_idx < n_threads; thread_idx++) {
      for (size_t i = 0; i < num_op_per_thread; i++) {
        thread_keys.at(thread_idx)[i] = distrib(gen);
      }
    }
  }

  return thread_keys;
}

static auto test_tbb_MT_rw_zipf(size_t n_threads, double zipf_theta = 0, bool print_res = true) {
  using tbb_lru_t = dcds::baseline::LruListTbb<size_t, size_t>;
  tbb_lru_t lru(lru_capacity);

  auto thr = dcds::ThreadRunner(n_threads);
  std::vector<std::vector<size_t>> thread_keys = initWorkload(n_threads, zipf_theta);

  auto runtime_ms = thr(
      [thread_keys](const uint64_t _tid, tbb_lru_t* _instance, const size_t _nr) {
        static thread_local auto thr_keys = thread_keys[_tid];

        for (size_t i = 0; i < num_op_per_thread; i++) {
          // auto c = i%domain_max;
          auto c = thr_keys[i];
          _instance->insert(c, c);
        }
      },
      &lru, domain_max);

  if (print_res)
    printThroughput(
        runtime_ms, n_threads,
        " OneAPI TBB (zipf: " + std::to_string(zipf_theta) + ")(key_max : " + std::to_string(domain_max) + " )");
  return runtime_ms;
}

static auto test_global_lock_MT_rw_zipf(size_t n_threads, double zipf_theta = 0, bool print_res = true) {
  using std_lru_t = dcds::baseline::LruListStd<size_t, size_t>;
  std_lru_t lru(lru_capacity);

  auto thr = dcds::ThreadRunner(n_threads);
  std::vector<std::vector<size_t>> thread_keys = initWorkload(n_threads, zipf_theta);

  auto runtime_ms = thr(
      [thread_keys](const uint64_t _tid, std_lru_t* _instance, const size_t _nr) {
        static thread_local auto thr_keys = thread_keys[_tid];

        for (size_t i = 0; i < num_op_per_thread; i++) {
          // auto c = i%domain_max;
          auto c = thr_keys[i];
          _instance->insert(c, c);
        }
      },
      &lru, domain_max);

  if (print_res)
    printThroughput(
        runtime_ms, n_threads,
        " GlobalLock (zipf: " + std::to_string(zipf_theta) + ")(key_max : " + std::to_string(domain_max) + " )");
  return runtime_ms;
}

static auto test_dcds_MT_rw_zipf(size_t n_threads, double zipf_theta = 0, bool print_res = true) {
  static bool build = false;
  static dcds::datastructures::LruList2* lru = nullptr;
  if (!build) {
    lru = new dcds::datastructures::LruList2(lru_capacity);
    lru->build(true, false);
    build = true;
  }

  auto instance = lru->createInstance();

  auto thr = dcds::ThreadRunner(n_threads);
  std::vector<std::vector<size_t>> thread_keys = initWorkload(n_threads, zipf_theta);

  auto runtime_ms = thr(
      [thread_keys](const uint64_t _tid, dcds::JitContainer* _instance, const size_t _nr) {
        static thread_local auto thr_keys = thread_keys[_tid];

        for (size_t i = 0; i < num_op_per_thread; i++) {
          // auto c = i%domain_max;
          auto c = thr_keys[i];
          auto v = _instance->op("insert", c, c);
        }
      },
      instance, domain_max);

  dcds::storage::TableRegistry::getInstance().clear();

  if (print_res)
    printThroughput(runtime_ms, n_threads,
                    " DCDS (zipf: " + std::to_string(zipf_theta) + ")(key_max : " + std::to_string(domain_max) + " )");
  return runtime_ms;
}

template <class Function, class... Args>
static void test_runner_l(Function&& f, Args&&... args) {
  constexpr size_t num_runs = 5;
  std::vector<size_t> cores{24, 36, 48};
  // std::vector<size_t> cores{1, 2, 4, 8, 12, 16, 24, 36, 48};
  //   std::vector<size_t> cores{1, 2, 4, 8, 16, 18, 24,
  //                             /*32, 36, 48, 64, 72, 84, 108, 120, 128, 144*/};
  //   std::vector<size_t> cores{16, 18, 24, 32, 36, 48, 64, 72, 84, 108, 120, 128, 144};

  //  std::vector<double> zipf_theta{0, 0.1, 0.2, 0.4, 0.60, 0.80, 0.90, 0.99};
  std::vector<double> zipf_theta{0};
  std::reverse(zipf_theta.begin(), zipf_theta.end());

  for (auto zipf : zipf_theta) {
    for (auto t : cores) {
      for (size_t r = 0; r < num_runs; r++) {
        if (t == cores.front() || t == cores.back()) dcds::profiling::Profile::resume();
        f(t, zipf, args...);
        if (t == cores.front() || t == cores.back()) dcds::profiling::Profile::pause();
      }
    }
  }
}

int main(int argc, char** argv) {
  dcds::InitializeLog(argc, argv);
  LOG(INFO) << "LRU";

  // dcds::ScopedAffinityManager scopedAffinity(dcds::Core{0});

  // test_runner_l([](size_t n_threads, double zipf_theta) { test_global_lock_MT_rw_zipf(n_threads, zipf_theta); });
  // test_runner_l([](size_t n_threads, double zipf_theta) { test_tbb_MT_rw_zipf(n_threads, zipf_theta); });
  test_runner_l([](size_t n_threads, double zipf_theta) { test_dcds_MT_rw_zipf(n_threads, zipf_theta); });

  return 0;
}
