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

#ifndef DCDS_LRU_BENCH_UTIL_HPP
#define DCDS_LRU_BENCH_UTIL_HPP
#include <benchmark/benchmark.h>

/**
 * Fixture to generate data with a uniform distribution for lru benching
 *
 *  state.range(0) is the total number of elements to insert/get. The number of elements per thread is the
 * total/count_threads, thus total must be divisible by count_threads exactly. state.range(1) is the maximum key value
 * for the uniform distribution (0, state.range(1))
 */
class UniformRandomLru : public benchmark::Fixture {
 public:
  UniformRandomLru() : thread_keys() {}

  void SetUp(const ::benchmark::State& state) override;

  void TearDown(const ::benchmark::State& state) override;
  // a vector of keys for each thread to use
  std::vector<std::vector<int>> thread_keys;
};

/**
 * Fixture to generate data with a zipfian distribution for lru benching
 *
 *  state.range(0) is the total number of elements to insert/get. The number of elements per thread is the
 * total/count_threads, thus total must be divisible by count_threads exactly. state.range(1) is the maximum key value
 * for the uniform distribution (0, state.range(1))
 * state.range(2) is the zipfian parameter * 100. * 100 because parameters are all size_t in googlebenchmark.
 * This uses the abseil implementation of the zipfian
 */
class ZipfianRandomLru : public benchmark::Fixture {
 public:
  ZipfianRandomLru() : thread_keys() {}

  void SetUp(const ::benchmark::State& state) override;

  void TearDown(const ::benchmark::State& state) override;
  // a vector of keys for each thread to use
  std::vector<std::vector<int>> thread_keys;
};

#endif  // DCDS_LRU_BENCH_UTIL_HPP
