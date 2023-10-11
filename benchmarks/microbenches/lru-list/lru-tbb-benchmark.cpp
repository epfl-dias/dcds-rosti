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
#define TBB_PREVIEW_CONCURRENT_LRU_CACHE 1
#include "absl/log/log.h"
#include "lru-bench-util.hpp"
#include "oneapi/tbb/concurrent_lru_cache.h"

static int ConstructValue([[maybe_unused]] int _) { return 42; }

// IMPORTANT use repetitions instead of iterations, i.e. only use one iteration
// each repetition will use a fresh concurrent_lru_cache. Iterations will re-use the same one
BENCHMARK_DEFINE_F(UniformRandomLru, UniformTbbLru)(benchmark::State& st) {
  //  only thread 0 constructs the lru cache
  static tbb::concurrent_lru_cache<int, int>* cache = nullptr;
  if (st.thread_index() == 0) {
    const size_t lru_size = st.range(2);
    cache = new tbb::concurrent_lru_cache<int, int>(&ConstructValue, lru_size);
  }
  for ([[maybe_unused]] auto _ : st) {
    auto& this_threads_keys = thread_keys[st.thread_index()];
    for (const auto& key : this_threads_keys) {
      // each call to [] gets the value with that key or constructs a new value with ConstructValue
      benchmark::DoNotOptimize((*cache)[key]);
    }
  }
  if (st.thread_index() == 0) {
    delete cache;
  }
  st.SetItemsProcessed(st.range(0) * st.iterations());
}

BENCHMARK_REGISTER_F(UniformRandomLru, UniformTbbLru)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseRealTime()
    ->ReportAggregatesOnly(true)
    ->ArgNames({"count_inserts", "max_key", "lru_size"})
    ->Args({10000, 10000, 1000})
    ->ThreadRange(1, 8);

// IMPORTANT use repetitions instead of iterations, i.e. only use one iteration
// each repetition will use a fresh concurrent_lru_cache. Iterations will re-use the same one
BENCHMARK_DEFINE_F(ZipfianRandomLru, ZipfianTbbLru)(benchmark::State& st) {
  //  only thread 0 constructs the lru cache
  static tbb::concurrent_lru_cache<int, int>* cache = nullptr;
  if (st.thread_index() == 0) {
    const size_t lru_size = st.range(2);
    cache = new tbb::concurrent_lru_cache<int, int>(&ConstructValue, lru_size);
  }
  for ([[maybe_unused]] auto _ : st) {
    auto& this_threads_keys = thread_keys[st.thread_index()];
    for (const auto& key : this_threads_keys) {
      // each call to [] gets the value with that key or constructs a new value with ConstructValue
      benchmark::DoNotOptimize((*cache)[key]);
    }
  }
  if (st.thread_index() == 0) {
    delete cache;
  }
  st.SetItemsProcessed(st.range(0) * st.iterations());
}

BENCHMARK_REGISTER_F(ZipfianRandomLru, ZipfianTbbLru)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1)
    ->Repetitions(10)
    ->UseRealTime()
    ->ReportAggregatesOnly(true)
    ->ArgNames({"count_inserts", "max_key", "lru_size", "zipf_param*100"})
    ->Args({10000, 10000, 1000, 10})
    ->Args({10000, 10000, 1000, 20})
    ->Args({10000, 10000, 1000, 40})
    ->Args({10000, 10000, 1000, 60})
    ->Args({10000, 10000, 1000, 80})
    ->Args({10000, 10000, 1000, 100})
    ->ThreadRange(1, 8);
