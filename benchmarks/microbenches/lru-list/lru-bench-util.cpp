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

#include "lru-bench-util.hpp"

#include <absl/log/check.h>
#include <absl/log/log.h>
#include <absl/random/zipf_distribution.h>

#include <random>

void UniformRandomLru::SetUp(const ::benchmark::State& state) {
  // setup is run by GoogleBenchmark once per thread
  // We specifically do all setup in one thread.
  if (state.thread_index() != 0) {
    return;
  }
  constexpr size_t seed = 42;
  std::mt19937 gen(seed);
  const size_t count_elements_to_insert = state.range(0);
  const size_t element_range = state.range(1);

  CHECK(count_elements_to_insert % state.threads() == 0)
      << "number of elements to insert must be divisible by the number of threads";
  const size_t elements_per_thread = count_elements_to_insert / state.threads();
  std::uniform_int_distribution<> distrib(0, element_range);

  thread_keys.clear();
  thread_keys.resize(state.threads());
  for (auto& per_thread_keys : thread_keys) {
    per_thread_keys.resize(elements_per_thread, 42);
  }

  for (int thread_idx = 0; thread_idx < state.threads(); thread_idx++) {
    for (size_t i = 0; i < elements_per_thread; i++) {
      thread_keys.at(thread_idx)[i] = distrib(gen);
    }
  }
}

void UniformRandomLru::TearDown(const ::benchmark::State& state) {
  if (state.thread_index() != 0) {
    return;
  }
  thread_keys = {};
}

void ZipfianRandomLru::SetUp(const ::benchmark::State& state) {
  // setup is run by GoogleBenchmark once per thread
  // We specifically do all setup in one thread.
  if (state.thread_index() != 0) {
    return;
  }
  constexpr size_t seed = 42;
  std::mt19937 gen(seed);
  const size_t count_elements_to_insert = state.range(0);
  const int element_range = state.range(1);

  CHECK(count_elements_to_insert % state.threads() == 0)
      << "number of elements to insert must be divisible by the number of threads";
  const size_t elements_per_thread = count_elements_to_insert / state.threads();
  double zipf_param = static_cast<double>(state.range(3)) / 100.0;
  absl::zipf_distribution<int> distrib(element_range, 2.0, zipf_param);

  thread_keys.clear();
  thread_keys.resize(state.threads());
  for (auto& per_thread_keys : thread_keys) {
    per_thread_keys.resize(elements_per_thread, 42);
  }

  for (int thread_idx = 0; thread_idx < state.threads(); thread_idx++) {
    for (size_t i = 0; i < elements_per_thread; i++) {
      thread_keys.at(thread_idx)[i] = distrib(gen);
    }
  }
}

void ZipfianRandomLru::TearDown(const ::benchmark::State& state) {
  if (state.thread_index() != 0) {
    return;
  }
  thread_keys = {};
}
